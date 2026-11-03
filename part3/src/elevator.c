#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Agosto");
MODULE_DESCRIPTION("Elevator Kernel Module");

#define OFFLINE  0
#define IDLE     1
#define LOADING  2
#define UP       3
#define DOWN     4

//Ellevator declarations
#define MAX_PETS 5
#define MAX_WEIGHT 50

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
    int breed;
    int start;
    int dest;
    int weight;
    struct list_head list;
};

struct floorInfo //defining a floor
{
    struct list_head pet;
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
    int stopping;
    struct list_head pets;
    struct task_struct *thread;
};

static struct elevatorInfo elevator;
static struct mutex elevatorMutex;
static struct procDirEntry *procEntry;

extern int (*STUB_startElevator)(void);
extern int (*STUB_issueRequest)(int, int, int);
extern int (*STUB_stopElevator)(void);

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
    elevator.stopping = 0;
    INIT_LIST_HEAD(&elevator.pets);

    for (i = 0; i < MAX_FLOORS; i++) 
    {
        INIT_LIST_HEAD(&floors[i].pets);
        floors[i].numPets = 0;
    }

    STUB_startElevator = startElevator_fn;
    STUB_issueRequest = issueRequest_fn;
    STUB_stopElevator = stopElevator_fn;

    elevator.thread = kthread_run(elevatorThread_fn, NULL, "elevatorThread");
    if (IS_ERR(elevator.thread)) {
        printk(KERN_ERR "elevator: failed to create thread\n");
        return PTR_ERR(elevator.thread);
    }

    proc_entry = proc_create("elevator", 0, NULL, &elevatorFops);
    if (!proc_entry) 
    {
        kthread_stop(elevator.thread);
        return -ENOMEM;
    }

    printk(KERN_INFO "Pet Elevator: module loaded\n");
    return 0;
}

static int weightAcquire(int type) //Pet determines weight
{
    switch (type)
    {
        case CHIHUAHUA: 
            return CHIHUAHUA_WEIGHT;
        case PUG: 
            return PUG_WEIGHT;
        case PUGHUAHUA: 
            return PUGHUAHUA_WEIGHT;
        case CHIHUAHUA: 
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
                return C;
            case PUG: 
                return P;
            case PUGHUAHUA: 
                return PC;
            case CHIHUAHUA: 
                return D;
            default:
                return ?;
        }
}

static void loadPets(void) 
{
    struct pet *p, *tmp;
    int floor_idx = elevator.currentFloor - 1;

    list_for_each_entry_safe(p, tmp, &floors[floor_idx].pets, list) 
    {
        if (elevator.numPets >= MAX_PETS)
            break;
        if (elevator.currentWeight + p->weight > MAX_WEIGHT)
            break;
        list_del(&p->list);
        floors[floor_idx].numPets--;
        elevator.totalWaiting--;
        list_add_tail(&p->list, &elevator.pets);
        elevator.numPets++;
        elevator.currentWeight += p->weight;
        ssleep(1);
    }
}

static void petUnload(void) //for unloading the pets
{
    struct pet *p, *tmp;
    list_for_each_entry_safe(p, tmp, &elevator.pets, list)
    {
        if (p->dest == elevator.currentFloor) 
        {
            elevator.currentWeight -= p->weight;
            elevator.numPets--;
            elevator.totalServiced++;
            list_del(&p->list);
            kfree(p);
            ssleep(1);
        }
    }
}

static int openProc(struct inode *inode, struct file *file) 
{
    return single_open(file, showProc, NULL);
}

static int openProc(struct seq_file *m, void *v) {
    struct pet *p;
    int i;
    char *stateStr;

    mutex_lock(&elevator_mutex);

    switch (elevator.state) 
    {
        case OFFLINE:  
            stateStr = "OFFLINE";  break;
        case IDLE:     
            stateStr = "IDLE";     break;
        case LOADING:  
            stateStr = "LOADING";  break;
        case UP:       
            stateStr = "UP";       break;
        case DOWN:     
            stateStr = "DOWN";     break;
        default:       
            stateStr = "UNKNOWN";  break;
    }

    seq_printf(m, "Elevator state: %s\n", state_str);
    seq_printf(m, "Current floor: %d\n", elevator.currentFloor);
    seq_printf(m, "Current load: %d lbs\n", elevator.currentWeight);

    seq_printf(m, "Elevator status:");
    list_for_each_entry(p, &elevator.pets, list) 
    {
        seq_printf(m, " %c%d", get_type_char(p->type), p->dest);
    }
    seq_printf(m, "\n\n");

    for (i = MAX_FLOORS; i >= 1; i--) {
        int floor_idx = i - 1;
        seq_printf(m, "[%s] Floor %d: %d",
                   elevator.current_floor == i ? "*" : " ",
                   i,
                   floors[floor_idx].num_passengers);
        list_for_each_entry(p, &floors[floor_idx].passengers, list) {
            seq_printf(m, " %c%d", get_type_char(p->type), p->dest_floor);
        }
        seq_printf(m, "\n");
    }

    seq_printf(m, "\nNumber of passengers: %d\n", elevator.num_passengers);
    seq_printf(m, "Number of passengers waiting: %d\n", elevator.total_waiting);
    seq_printf(m, "Number of passengers serviced: %d\n", elevator.total_serviced);

    mutex_unlock(&elevatorMutex);
    return 0;
}

//https://github.com/OS-Project-2/linux-kernel-elevator-scheduler/blob/master/part3/src/elevator.c#L356 (Link I found for a similar project)
module_init(elevator_init); //initialize
module_exit(elevator_exit); //cleanup
