#include "proc.h"
#include "serial.h"
#include "mem.h"

#define PROC_MEM_SIZE 1024

static pcb_t proctab[MAX_PROCS];
static int32_t current_pid = -1;
static int32_t next_pid = 0;

static void serial_put_int(int32_t num){
    int8_t buf[12];
    int32_t i=0;

    if (num==0){
        serial_putc('0');
        return;
    }
    while(num>0){
        buf[i++]=(num%10)+'0';
        num/=10;
    }
    while(i--){
        serial_putc(buf[i]);
    }
}

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
int32_t proc_create(void (*func)(void)){
    if (next_pid >= MAX_PROCS){
        return -1;
    }

    void* mem = mem_alloc(PROC_MEM_SIZE);
    if(!mem){
        serial_puts("Memory allocation failed for new process.\n");
        return -1;
    }

    int32_t pid=next_pid++;

    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;
    proctab[pid].mem=mem;
    proctab[pid].memsz=PROC_MEM_SIZE;

    serial_puts("Process created with PID: ");
    serial_put_int(pid);
    serial_puts("\n");

    return pid;
}

void proc_run(void){
    for (int32_t i=0;i<MAX_PROCS;i++){
        if (proctab[i].state == PR_READY){
            current_pid = i;
            proctab[i].state = PR_CURRENT;

            serial_puts("Running process PID: ");
            serial_put_int(i);
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
    
    pcb_t* p = &proctab[current_pid];
    if(p->mem){
        mem_free(p->mem);
        p->mem = NULL;
        p->memsz = 0;
    }

    serial_puts("Terminating process PID: ");
    serial_put_int(current_pid);
    serial_puts("\n");

    proctab[current_pid].state = PR_TERMINATED;
    current_pid = -1;
}