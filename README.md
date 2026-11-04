# COP4610 Project 2 – Elevator Kernel Module

A Linux kernel module that simulates an elevator system transporting different types of pets between five floors. The module supports starting and stopping the elevator, processing transportation requests through custom system calls, and displaying live elevator information through `/proc/elevator`.

## Group Members
- John Seibold: [School Email]
- Nathan Agosto: [School Email]

## Division of Labor

### Part 1: System Call Tracing
- Adding syscalls to empty C programs
- Nathan

### Part 2: Timer Kernel Module
- Develop a kernel module called my_timer that calls ktime_get_real_ts64() to obtain the current time
- John

### Part 3: Elevator Kernel Module
- **Responsibilities**:
  - Implement the elevator kernel module.
  - Manage elevator states: OFFLINE, IDLE, LOADING, UP, and DOWN.
  - Implement pet loading and unloading.
  - Enforce elevator capacity and weight restrictions.
  - Implement elevator movement between floors.
  - Create the `/proc/elevator` interface to display elevator status.
  - Connect the elevator module to custom system calls.
- **Assigned to**: [Members]


## File Listing

```text
COP4610-Project2/
├── part1/
│   └── ...
├── part2/
│   └── ...
├── part3/
│   ├── src/
│   │   ├── elevator.c
│   │   └── Makefile
└── README.md
```

## How to Compile & Execute

### Requirements
- **Operating System**: Linux
- **Kernel**: Custom compiled Linux kernel with the project system calls implemented.
- **Compiler**: GCC
- **Build Tool**: GNU Make
- **Dependencies**: Linux kernel headers/build directory matching the running kernel.

### Compilation

Navigate to the elevator module source directory:

```bash
cd part3/src
```

Build the kernel module:

```bash
make
```

This generates the `elevator.ko` kernel module.

### Loading the Kernel Module

Load the module:

```bash
sudo insmod elevator.ko
```

Verify that the module loaded:

```bash
lsmod | grep elevator
```

Check that the proc entry exists:

```bash
cat /proc/elevator
```

### Execution

Navigate to the elevator test directory:

```bash
cd part3/tests/elevator-test
```

Start the elevator:

```bash
./consumer --start
```

Generate pet transportation requests (example: 40 pets):

```bash
./producer 40
```

Watch the elevator's status update:

```bash
watch -n 1 cat /proc/elevator
```

Stop the elevator:

```bash
./consumer --stop
```

Remove the kernel module after the elevator has stopped:

```bash
sudo rmmod elevator
```

### Testing Applications

- **producer.c**: Generates pets and submits transportation requests using the `issue_request()` system call.
- **consumer.c**: Starts or stops the elevator using the `start_elevator()` and `stop_elevator()` system calls.
- **/proc/elevator**: Displays the current elevator state, floor, weight, passengers, waiting pets, and total serviced pets.

## Development Log

Each member records their contributions here.

### [Member 1]

| Date | Work Completed / Notes |
|---|---|
| YYYY-MM-DD | [Description of task] |
| YYYY-MM-DD | [Description of task] |

### John Seibold

| Date | Work Completed / Notes |
| 10/20 | Part 2 |
| 10/30 | Part 3 steps 1-3 - Kernel compilation/syscall testing |
| 11/01 - 11/04 | Part 3 - Documentation, Testing elevator, Debugging | 


## Meetings

We communicated through Canvas mainly, working out timing among different parts of the project, difficulties,
and team roles/work distribution.

## Bugs

- **Bug 1**: Elevator movement logic could repeatedly reverse direction instead of prioritizing passengers already aboard. *FIXED* (I think)
- **Bug 2**: Elevator shutdown behavior needed to ensure passengers already aboard could reach their destinations before transitioning to OFFLINE.
- **Bug 3**: Requests involving invalid floors, such as floor 6, must be rejected because the elevator supports only five floors.


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
