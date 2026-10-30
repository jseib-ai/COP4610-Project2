#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

#define OFFLINE  0
#define IDLE     1
#define LOADING  2
#define UP       3
#define DOWN     4

#define MAX_PASSENGERS 5
#define MAX_WEIGHT 50

#define CHIHUAHUA 0
#define PUG 1
#define PUGHUAHUA 2
#define DACHSHUND 3

#define CHIHUAHUA_WEIGHT 3
#define PUG_WEIGHT 14
#define PUGHUAHUA_WEIGHT 10
#define DACHSHUND_WEIGHT 16

struct pet 
{
    int breed;
    int start;
    int dest;
    int weight;
    struct list_head list;
};

struct floor_data 
{
    struct list_head pet;
    int numPets;
};

struct elevator_data 
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


module_init(elevator_init);
module_exit(elevator_exit);
