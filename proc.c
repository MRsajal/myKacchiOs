#include "proc.h"
#include "serial.h"
#include "mem.h"

#define PROC_MEM_SIZE 1024
#define PROC_STACK_SIZE 1024

static pcb_t proctab[MAX_PROCS];
static int32_t current_pid = -1;
static int32_t next_pid = 0;
pcb_t* currpid=NULL;

extern void proc_exit(void);
extern void ctxsw(uint32_t** old, uint32_t** new);


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

    uint32_t* stack=mem_alloc(PROC_STACK_SIZE);
    if(!stack){
        serial_puts("Stack allocation failed for new process.\n");
        return -1;
    }

    uint32_t* sp = (uint32_t*)((uint8_t*)stack + PROC_STACK_SIZE);

    *--sp = (uint32_t)proc_exit; // Return address
    *--sp = (uint32_t)func;      // Initial EIP
    *--sp=0;
    *--sp=0;
    *--sp=0;
    *--sp=0;

    int pid = next_pid++;

    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;
    proctab[pid].stack_base = stack;
    proctab[pid].esp = sp;
    proctab[pid].mem = stack;
    proctab[pid].memsz = PROC_STACK_SIZE;

    // void* mem = mem_alloc(PROC_MEM_SIZE);
    // if(!mem){
    //     serial_puts("Memory allocation failed for new process.\n");
    //     return -1;
    // }

    // int32_t pid=next_pid++;

    // proctab[pid].pid = pid;
    // proctab[pid].state = PR_READY;
    // proctab[pid].entry = func;
    // proctab[pid].mem=mem;
    // proctab[pid].memsz=PROC_MEM_SIZE;

    serial_puts("Process created with PID: ");
    serial_put_int(pid);
    serial_puts("\n");

    return pid;
}

void proc_run(void){
    // for (int32_t i=0;i<MAX_PROCS;i++){
    //     if (proctab[i].state == PR_READY){
    //         current_pid = i;
    //         proctab[i].state = PR_CURRENT;

    //         serial_puts("Running process PID: ");
    //         serial_put_int(i);
    //         serial_puts("\n");

    //         proctab[i].entry();
    //         proc_exit();
    //     }
    // }
    currpid=&proctab[0];
    currpid->state=PR_CURRENT;

    currpid->entry();
}
void resched(void){
    int next = -1;
    for(int i=0;i<MAX_PROCS;i++){
        if(proctab[i].state == PR_READY){
            next = i;
            break;
        }
    }
    if(next == -1 || next == current_pid){
        return;
    }
    int old=current_pid;
    current_pid=next;
    
    proctab[next].state=PR_CURRENT;

    if(old >=0 && proctab[old].state == PR_CURRENT){
        proctab[old].state=PR_READY;
    }
    if(old<0){
        asm volatile("movl %0, %%esp"::"r"(proctab[next].esp));
        return;
    }
    ctxsw(&proctab[old].esp,&proctab[next].esp);
}

void yield(void){
    currpid->state=PR_READY;
    resched();
}

void proc_exit(void){
    int pid = currpid->pid;

    proctab[pid].state = PR_TERMINATED;
    mem_free(proctab[pid].mem);
    proctab[pid].mem = NULL;

    resched();

    while (1); /* never returns */
}