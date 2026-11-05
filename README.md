# COP4610 Project 2 – Elevator Kernel Module

A Linux kernel module that simulates an elevator system transporting different types of pets between five floors. The module supports starting and stopping the elevator, processing transportation requests through custom system calls, and displaying live elevator information through `/proc/elevator`.

## Group Members
- John Seibold: jcs22l@fsu.edu
- Nathan Agosto: na24l@fsu.edu

## Division of Labor

### Part 1: System Call Tracing - Nathan
  - Adding syscalls to empty C programs

### Part 2: Timer Kernel Module - John
  - Develop a kernel module called my_timer that calls ktime_get_real_ts64() to obtain the current time

### Part 3: Elevator Kernel Module
  - Kernel compilation and syscall testing - John
  - Implement the elevator kernel module - Nathan
  - Manage elevator states: OFFLINE, IDLE, LOADING, UP, and DOWN - Nathan
  - Implement pet loading and unloading - Nathan
  - Enforce elevator capacity and weight restrictions - Nathan
  - Implement elevator movement between floors - John
  - Create the `/proc/elevator` interface to display elevator status - Nathan
  - Connect the elevator module to custom system calls - John


## File Listing

COP4610-Project2/
├── part1/
│   └── empty.c
|   |__ empty.trace
|   |__ part1.c
|   |__ part1.trace
|   |__ Makefile
├── part2/
│   └── src/
|   |   |-- my_timer.c
    |__ Makefile
├── part3/
│   ├── src/
│   │   ├── elevator.c
│   │   └── Makefile
|   |___syscalls.c
└── README.md


## How to Compile & Execute

### Requirements
- **Operating System**: Linux
- **Kernel**: Custom compiled Linux kernel with the project system calls implemented.
- **Compiler**: GCC
- **Build Tool**: GNU Make
- **Dependencies**: Linux kernel headers/build directory matching the running kernel.

### Compilation

Part 2 – Kernel Module: Timer

From the project directory:
cd part2
make

Load the module into the running kernel:
sudo insmod src/my_timer.ko

Check that the module loaded:
lsmod | grep my_timer

To remove the module:
sudo rmmod my_timer

Part 3: Elevator kernel module
Navigate to the elevator module source directory:
cd part3/src

Build the kernel module:
make

This generates the `elevator.ko` kernel module.

### Loading the Kernel Module

Load the module:
sudo insmod elevator.ko

Verify that the module loaded:
lsmod | grep elevator

Check that the proc entry exists:
cat /proc/elevator


### Testing Applications

- **producer.c**: Generates pets and submits transportation requests using the `issue_request()` system call.
- **consumer.c**: Starts or stops the elevator using the `start_elevator()` and `stop_elevator()` system calls.
- **/proc/elevator**: Displays the current elevator state, floor, weight, passengers, waiting pets, and total serviced pets.

## Development Log

### Nathan Agosto
| Date | Work Completed / Notes |
|---|---|
| YYYY-MM-DD | [Description of task] |
| YYYY-MM-DD | [Description of task] |

### John Seibold
| Date | Work Completed / Notes |
| 10/20 | Part 2  - Mytimer kernel module |
| 10/30 | Part 3 steps 1-3 - Kernel compilation/syscall testing |
| 11/01 - 11/04 | Part 3 - Documentation, Testing elevator, Debugging | 

### Nathan Agosto
| Date | Work Completed / Notes |
| 10/17 | Part 1  - System-call tracing |
| 10/29 - 11/2 | Part 3 steps 4 - Elevator Implementation |
| 11/3-11/4 | Part 3 - Elevator cleanup, makefile |


## Meetings

We communicated through Canvas mainly, working out timing among different parts of the project, difficulties,
and team roles/work distribution.

## Bugs

- **Bug 1**: Elevator movement logic could repeatedly reverse direction instead of prioritizing passengers already aboard. *FIXED* (I think)
- **Bug 2**: Elevator shutdown behavior needed to ensure passengers already aboard could reach their destinations before transitioning to OFFLINE.


## Considerations

- The elevator supports five floors.
- The maximum elevator capacity is five pets.
- The maximum combined passenger weight is 50 pounds.
- Pet types have different weights:
  - Chihuahua: 3 lbs
  - Pug: 14 lbs
  - Pughua(h)ua: 10 lbs
  - Dachshund: 16 lbs
- Pets board in FIFO
- Requests must use valid starting and destination floors.
- Starting and destination floors cannot be the same.
- The elevator uses a kernel thread to manage movement and passenger operations.
- A mutex protects shared elevator data.
- The `/proc/elevator` interface provides a way to monitor the elevator without directly interacting with kernel memory.
- The custom system calls allow user-space programs to communicate with the elevator kernel module.

- ##The skeleton of this README was created by ChatGPT


