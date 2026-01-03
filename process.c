#include "process.h"
#include "serial.h"
#include "memory.h"

#define PROC_MEM_SIZE 1024
#define PROC_STACK_SIZE 4096

static pcb_t proctab[MAX_PROCS];
static int32_t current_pid = -1;
pcb_t* currpid=NULL;

// Track whether we've done the very first dispatch (bootstrapping into a process)
static int first_dispatch = 1;

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
        proctab[i].entry = NULL;
        proctab[i].stack_base = NULL;
        proctab[i].esp = NULL;
        proctab[i].mem = NULL;
        proctab[i].memsz = 0;
        proctab[i].has_run = 0;
    }
    serial_puts("Process manager initialized.\n");
}

// Create a new process
int32_t proc_create(void (*func)(void)){
    int pid;
    for(pid=0;pid<MAX_PROCS;pid++){
        if(proctab[pid].state == PR_TERMINATED){
            break;
        }
    }
    if(pid==MAX_PROCS){
        return -1;
    }

    uint32_t* stack=mem_alloc(PROC_STACK_SIZE);
    if(!stack){
        serial_puts("Stack allocation failed for new process.\n");
        return -1;
    }

    uint32_t* sp = (uint32_t*)(((uint32_t)stack + PROC_STACK_SIZE) & ~0xF);

    *--sp = (uint32_t)proc_exit; /* return address after func */
    *--sp = (uint32_t)func;      /* EIP target for first ret */


    //int pid = next_pid++;

    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;
    proctab[pid].stack_base = stack;
    proctab[pid].esp = sp;
    proctab[pid].mem = stack;
    proctab[pid].memsz = PROC_STACK_SIZE;
    proctab[pid].has_run=0;

    serial_puts("Process created with PID: ");
    serial_put_int(pid);
    serial_puts("\n");

    return pid;
}

void proc_run(void){
    current_pid= -1;
    resched();
}

void resched(void){
    // Always keep PID 0 as the idle/null process fallback
    if (proctab[0].state == PR_TERMINATED || proctab[0].pid != 0) {
        // If PID 0 was somehow not set up, do nothing (shouldn't happen)
        return;
    }

    int old = current_pid;
    int next = -1;

    // Round-robin: start searching from old+1
    int start = (old < 0) ? 0 : old;
    for (int i = 1; i <= MAX_PROCS; i++) {
        int pid = (start + i) % MAX_PROCS;
        if (proctab[pid].state == PR_READY) {
            next = pid;
            break;
        }
    }

    // If no READY process found, run idle (PID 0)
    if (next == -1) {
        next = 0;
    }

    // If we're already running 'next', nothing to do
    if (next == old && old >= 0) {
        return;
    }

    // Update states
    if (old >= 0 && proctab[old].state == PR_CURRENT) {
        proctab[old].state = PR_READY;
    }
    proctab[next].state = PR_CURRENT;

    current_pid = next;
    currpid = &proctab[next];

    // First ever dispatch: we cannot ctxsw from a non-existent context.
    if (first_dispatch) {
        first_dispatch = 0;
        asm volatile(
            "movl %0, %%esp \n"
            "jmp  *%1      \n"
            :
            : "r"(proctab[next].esp),
              "r"(proctab[next].entry)
        );
        // never returns
        while (1) { }
    }

    // Normal context switch
    ctxsw(&proctab[old].esp, &proctab[next].esp);
}

void yield(void){
    if(currpid){
        // Keep PID 0 always READY so the shell keeps running
        if (currpid->pid != 0) {
            currpid->state = PR_READY;
        } else {
            currpid->state = PR_READY;
        }
    }
    resched();
}


void proc_exit(void){
    int pid = currpid->pid;

    proctab[pid].state = PR_TERMINATED;
    mem_free(proctab[pid].mem);
    proctab[pid].mem = NULL;

    serial_puts("Process exited: ");
    serial_put_int(pid);
    serial_puts("\n");

    currpid = NULL;
    current_pid = -1;
    resched();

    while (1); /* never returns */
}

void proc_list(void){
    serial_puts("PID\tState\t\tEntry\n");
    serial_puts("---\t-----\t\t-----\n");
    
    for(int i = 0; i < MAX_PROCS; i++){
        if(proctab[i].state != PR_TERMINATED){
            serial_put_int(i);
            serial_puts("\t");
            
            switch(proctab[i].state){
                case PR_CURRENT:
                    serial_puts("RUNNING\t\t");
                    break;
                case PR_READY:
                    serial_puts("READY\t\t");
                    break;
                case PR_TERMINATED:
                    serial_puts("TERMINATED\t");
                    break;
                default:
                    serial_puts("UNKNOWN\t\t");
                    break;
            }
            
            serial_puts("0x");
            // Print entry point address in hex
            uint32_t addr = (uint32_t)proctab[i].entry;
            char hex_str[9];
            for(int j = 7; j >= 0; j--){
                int digit = (addr >> (j * 4)) & 0xF;
                hex_str[7-j] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            }
            hex_str[8] = '\0';
            serial_puts(hex_str);
            serial_puts("\n");
        }
    }
    serial_puts("\n");
}