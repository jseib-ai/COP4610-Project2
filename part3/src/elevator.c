#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Agosto");
MODULE_DESCRIPTION("Elevator Kernel Module");

#define OFFLINE  0
#define IDLE     1
#define LOADING  2
#define UP       3
#define DOWN     4

//Elevator declarations
#define MAX_PETS 5
#define MAX_WEIGHT 50
#define MAX_FLOORS 5

//Pet syscall declarations
#define CHIHUAHUA 0
#define PUG 1
#define PUGHUAHUA 2
#define DACHSHUND 3

//Pet weight declarations
#define CHIHUAHUA_WEIGHT 3
#define PUG_WEIGHT 14
#define PUGHUAHUA_WEIGHT 10
#define DACHSHUND_WEIGHT 16

struct pet //defining a pet
{
    int type;
    int start;
    int dest;
    int weight;
    struct list_head list;
};

struct floorInfo //defining a floor
{
    struct list_head pets;
    int numPets;
};

struct elevatorInfo //defining the elevator
{
    int state;
    int currentFloor;
    int currentWeight;
    int numPets;
    int totalServiced;
    int totalWaiting;
    int stop;
    struct list_head pets;
    struct task_struct *thread;
};

static struct elevatorInfo elevator;
static struct floorInfo floors[MAX_FLOORS];
static struct mutex elevatorMutex;
static struct proc_dir_entry *proc_entry;

extern int (*STUB_start_Elevator)(void);
extern int (*STUB_issue_Request)(int, int, int);
extern int (*STUB_stop_Elevator)(void);

static void __exit elevatorExit(void) //terminates the elevator module
{
    struct pet *p, *tmp;
    int i;

    STUB_start_Elevator = NULL;
    STUB_issue_Request = NULL;
    STUB_stop_Elevator = NULL;

    kthread_stop(elevator.thread);
    proc_remove(proc_entry);

    mutex_lock(&elevatorMutex);
    list_for_each_entry_safe(p, tmp, &elevator.pets, list) 
    {
        list_del(&p->list);
        kfree(p);
    }
    
    for (i = 0; i < MAX_FLOORS; i++) 
    {
        list_for_each_entry_safe(p, tmp, &floors[i].pets, list)
        {
            list_del(&p->list);
            kfree(p);
        }
    }
    mutex_unlock(&elevatorMutex);
}

static int weightGet(int type) //Pet determines weight
{
    switch (type)
    {
        case CHIHUAHUA: 
            return CHIHUAHUA_WEIGHT;
        case PUG: 
            return PUG_WEIGHT;
        case PUGHUAHUA: 
            return PUGHUAHUA_WEIGHT;
        case DACHSHUND: 
            return DACHSHUND_WEIGHT;
        default:
            return 0;
    }
}

static char PetID(int type) 
    {
    switch (type)
        {
            case CHIHUAHUA: 
                return 'C';
            case PUG: 
                return 'P';
            case PUGHUAHUA: 
                return 'H';
            case DACHSHUND: 
                return 'D';
            default:
                return '?';
        }
}

static void petLoad (void) //loading the pets onto the elevator
{
    struct pet *p, *tmp;
    int indexFloor = elevator.currentFloor - 1;
    int loaded = 0;

    list_for_each_entry_safe(p, tmp, &floors[indexFloor].pets, list) 
    {
        if (elevator.numPets >= MAX_PETS)
            break;
        if (elevator.currentWeight + p->weight > MAX_WEIGHT)
            break;
        
        list_del(&p->list);
        floors[indexFloor].numPets--;
        elevator.totalWaiting--;
        list_add_tail(&p->list, &elevator.pets);
        elevator.numPets++;
        elevator.currentWeight += p->weight;
        loaded++;
    }
    return loaded;
}




static void petUnload(void) //for unloading the pets
{
    struct pet *p, *tmp;
    int unloaded = 0;
    
    list_for_each_entry_safe(p, tmp, &elevator.pets, list)
    {
        if (p->dest == elevator.currentFloor) 
        {
            elevator.currentWeight -= p->weight;
            elevator.numPets--;
            elevator.totalServiced++;
            list_del(&p->list);
            kfree(p);
            unloaded++;
        }
    }
    return unloaded;
}    

static int startElevator(void)
{
    mutex_lock(&elevatorMutex);
    if (elevator.state != OFFLINE) //ensures that the elevator doesn't move after going offline
    {
        mutex_unlock(&elevatorMutex);
        return 1;
    }
    elevator.state = IDLE;
    elevator.currentFloor = 1;
    elevator.currentWeight = 0;
    elevator.stop = 0;
    mutex_unlock(&elevatorMutex);
    return 0;
}

static int stopElevator(void) 
{
    mutex_lock(&elevatorMutex);
    if (elevator.stop) //Condition for when you force the elevator to stop while it's stopped
    {
        mutex_unlock(&elevatorMutex);
        return 1;
    }
    elevator.stop = 1;
    mutex_unlock(&elevatorMutex);
    return 0;
}

static int abovePets(void) //checks to see if there's pets above
{
    int i;
    struct pet *p;
    list_for_each_entry(p, &elevator.pets, list) 
    {
        if (p->dest > elevator.currentFloor) //pets in the elevator
            return 1;
    }
    
    for (i = elevator.currentFloor; i < MAX_FLOORS; i++)  //Checking for pets on other floors
    {
        if (floors[i].numPets > 0)
            return 1;
    }
    return 0;
}

static int waitingPets(void) //checks to see if there's pets waiting on floors
{
    int i;
    for (i = 0; i < MAX_FLOORS; i++) 
    {
        if (floors[i].numPets > 0)
            return 1;
    }
    return 0;
}

static int belowPets(void) //checks to see if there's pets below
{
    int i;
    struct pet *p;
    list_for_each_entry(p, &elevator.pets, list) 
    {
        if (p->dest < elevator.currentFloor)
            return 1;
    }
    for (i = 0; i < elevator.currentFloor - 1; i++) 
    {
        if (floors[i].numPets > 0)
            return 1;
    }
    return 0;
}

static int floorRequest(int start, int dest, int type) //implementing requesting floor logic
{
    struct pet *p;
    int indexFloor;

    if (start < 1 || start > MAX_FLOORS ||
        dest < 1 || dest > MAX_FLOORS ||
        start == dest ||
        type < 0 || type > 3)
        return 1; //For when the variables make no sense

    p = kmalloc(sizeof(*p), GFP_KERNEL);
    if (!p)
        return -ENOMEM;

    p->type = type;
    p->start = start;
    p->dest = dest;
    p->weight = weightGet(type);
    INIT_LIST_HEAD(&p->list);

    indexFloor = start - 1;

    mutex_lock(&elevatorMutex);
    list_add_tail(&p->list, &floors[indexFloor].pets);
    floors[indexFloor].numPets++;
    elevator.totalWaiting++;
    mutex_unlock(&elevatorMutex);

    return 0;
}

static int elevatorThread(void *data) //The main logic for moving across floors
{
    int moved;
    
    while (!kthread_should_stop()) 
    {
        mutex_lock(&elevatorMutex);

        if (elevator.state == OFFLINE) 
        {
            mutex_unlock(&elevatorMutex);
            ssleep(1);
            continue;
        }

        if (elevator.state == IDLE) 
        {
            if (elevator.stop && elevator.numPets == 0)
                elevator.state = OFFLINE;
            else if (waitingPets()) 
            {
                if (abovePets())
                    elevator.state = UP;
                else
                    elevator.state = DOWN;
            }
        
            mutex_unlock(&elevatorMutex);
            ssleep(1);
            continue;
        }

        elevator.state = LOADING;
        mutex_unlock(&elevatorMutex);

        mutex_lock(&elevatorMutex);
        moved = petUnload();
        if(!elevator.stop)
            moved += petLoad();
        mutex_unlock(&elevatorMutex);
        if(moved)
            ssleep(1);

        
        mutex_lock(&elevatorMutex);
        if (elevator.stop) 
        {
            if (elevator.numPets == 0) 
            {
                elevator.state = OFFLINE;
                mutex_unlock(&elevatorMutex);
                continue;
            }
        }

        if (abovePets()) 
        {
            elevator.state = UP;
            mutex_unlock(&elevatorMutex);
            ssleep(2);
            mutex_lock(&elevatorMutex);
            if (elevator.currentFloor < MAX_FLOORS)
                elevator.currentFloor++;
            mutex_unlock(&elevatorMutex);
        } 
        else if (belowPets()) 
        {
            elevator.state = DOWN;
            mutex_unlock(&elevatorMutex);
            ssleep(2);
            mutex_lock(&elevatorMutex);
            if (elevator.currentFloor > 1)
                elevator.currentFloor--;
            mutex_unlock(&elevatorMutex);
        } 
        else 
        {
            elevator.state = IDLE;
            mutex_unlock(&elevatorMutex);
        }
    }
    return 0;
}



static int showProc(struct seq_file *m, void *v) 
{
    struct pet *p;
    int i;
    char *stateStr;

    mutex_lock(&elevatorMutex);

    switch (elevator.state) 
    {
        case OFFLINE:  
            stateStr = "OFFLINE"; break;
        case IDLE:     
            stateStr = "IDLE"; break;
        case LOADING:  
            stateStr = "LOADING"; break;
        case UP:       
            stateStr = "UP"; break;
        case DOWN:     
            stateStr = "DOWN"; break;
        default:       
            stateStr = "UNKNOWN"; break;
    }

    seq_printf(m, "Elevator state: %s\n", stateStr);
    seq_printf(m, "Current floor: %d\n", elevator.currentFloor);
    seq_printf(m, "Current load: %d lbs\n", elevator.currentWeight);

    seq_printf(m, "Elevator status:");
    list_for_each_entry(p, &elevator.pets, list) 
    {
        seq_printf(m, " %c%d", PetID(p->type), p->dest);
    }
    seq_printf(m, "\n");

    for (i = MAX_FLOORS; i >= 1; i--) 
    {
        int indexFloor = i - 1;
        seq_printf(m, "[%s] Floor %d: %d",
                   elevator.currentFloor == i ? "*" : " ",
                   i,
                   floors[indexFloor].numPets);
        list_for_each_entry(p, &floors[indexFloor].pets, list) 
        {
            seq_printf(m, " %c%d", PetID(p->type), p->dest);
        }
        seq_printf(m, "\n");
    }

    seq_printf(m, "\nNumber of pets: %d\n", elevator.numPets);
    seq_printf(m, "Number of pets waiting: %d\n", elevator.totalWaiting);
    seq_printf(m, "Number of pets serviced: %d\n", elevator.totalServiced);

    mutex_unlock(&elevatorMutex);
    return 0;
}

static int open_proc(struct inode *inode, struct file *file) 
{
    return single_open(file, showProc, NULL);
}

static const struct proc_ops elevatorFops = {
    .proc_open = open_proc,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release, }; //operations for the procs



static int __init elevatorInit(void) //initating the elevator
{
    int i;
    mutex_init(&elevatorMutex);

    elevator.state = OFFLINE;
    elevator.currentFloor = 1;
    elevator.currentWeight = 0;
    elevator.numPets = 0;
    elevator.totalServiced = 0;
    elevator.totalWaiting = 0;
    elevator.stop = 0;
    INIT_LIST_HEAD(&elevator.pets); //declaration of elevator variables

    for (i = 0; i < MAX_FLOORS; i++) 
    {
        INIT_LIST_HEAD(&floors[i].pets);
        floors[i].numPets = 0;
    }


    elevator.thread = kthread_run(elevatorThread, NULL, "elevatorThread");
    if (IS_ERR(elevator.thread)) 
    {
        printk(KERN_ERR "elevator: failed to create thread\n");
        return PTR_ERR(elevator.thread);
    }

    proc_entry = proc_create("elevator", 0444, NULL, &elevatorFops);
    if (!proc_entry) 
    {
        kthread_stop(elevator.thread);
        return -ENOMEM;
    }

    STUB_start_Elevator = startElevator;
    STUB_issue_Request = floorRequest;
    STUB_stop_Elevator = stopElevator;
    
    return 0;
}

module_init(elevatorInit); //initialize
module_exit(elevatorExit); //cleanup
