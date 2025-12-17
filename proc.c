#include "proc.h"
#include "serial.h"

static pcb_t proctab[MAX_PROCS];
static int current_pid = -1;
static int next_pid = 0;

// Initialize the process manager
void proc_init(void){
    for (int i=0;i<MAX_PROCS;i++){
        proctab[i].pid = -1;
        proctab[i].state = PR_TERMINATED;
        proctab[i].entry = 0;
    }
    serial_puts("Process manager initialized.\n");
}

// Create a new process
int* proc_create(void (*func)(void)){
    if (next_pid >= MAX_PROCS){
        return -1;
    }
    int pid=next_pid++;

    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;

    serial_puts("Process created with PID: ");
    char pid_str[4];
    itoa(pid, pid_str, 10);
    serial_puts(pid_str);
    serial_puts("\n");

    return pid;
}

void proc_run(void){
    for (int i=0;i<MAX_PROCS;i++){
        if (proctab[i].state == PR_READY){
            current_pid = i;
            proctab[i].state = PR_CURRENT;

            serial_puts("Running process PID: ");
            char pid_str[4];
            itoa(i, pid_str, 10);
            serial_puts(pid_str);
            serial_puts("\n");

            proctab[i].entry();
            proc_exit();
        }
    }
}

void proc_exit(void){
    if (current_pid<0){
        return;
    }
    serial_puts("Terminating process PID: ");
    char pid_str[4];
    itoa(current_pid, pid_str, 10);
    serial_puts(pid_str);
    serial_puts("\n");

    proctab[current_pid].state = PR_TERMINATED;
    current_pid = -1;
}