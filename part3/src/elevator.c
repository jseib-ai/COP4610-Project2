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
#define MAX_PASSENGERS 5
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
static struct proc_dir_entry *proc_entry;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

static int weightAcquire(int type) 
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

static char get_type_char(int type) 
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

static void petUnload(void) //for unloading the pets
{
    struct pet *p, *tmp;
    list_for_each_entry_safe(p, tmp, &elevator.pets, list)
        {
            /*if (p->dest_floor == elevator.current_floor) {
            elevator.current_weight -= p->weight;
            elevator.num_passengers--;
            elevator.total_serviced++;
            list_del(&p->list);
            kfree(p);
            ssleep(1);
        }
    }*/
}


//https://github.com/OS-Project-2/linux-kernel-elevator-scheduler/blob/master/part3/src/elevator.c#L356 (Link I found for a similar project)
module_init(elevator_init); //initialize
module_exit(elevator_exit); //cleanup
