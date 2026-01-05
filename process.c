#include "process.h"
#include "serial.h"
#include "memory.h"
#include "scheduler.h"

#define PROC_STACK_SIZE 4096

pcb_t proctab[MAX_PROCS];
int32_t current_pid = -1;
pcb_t *currpid = NULL;

static int first_dispatch = 1;

extern void ctxsw(uint32_t **old, uint32_t **new);

/* -------------------------------------------------- */
/* Utility                                            */
/* -------------------------------------------------- */

static void serial_put_int(int32_t num) {
    char buf[12];
    int i = 0;

    if (num == 0) {
        serial_putc('0');
        return;
    }

    if (num < 0) {
        serial_putc('-');
        num = -num;
    }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i--) {
        serial_putc(buf[i]);
    }
}

//SLEEP
void sleep(int ticks){
    if(ticks <=0 || currpid == NULL) return;
    currpid->sleep_ticks = ticks;
    currpid->state = PR_SLEEP;
    resched();
}
void proc_tick(void) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proctab[i].state == PR_SLEEP) {
            proctab[i].sleep_ticks--;
            if (proctab[i].sleep_ticks <= 0) {
                proctab[i].state = PR_READY;
            }
        }
    }
}
void wait(int event) {
    currpid->wait_event = event;
    currpid->state = PR_WAIT;
    resched();
}
void wakeup(int event) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proctab[i].state == PR_WAIT &&
            proctab[i].wait_event == event) {

            proctab[i].wait_event = -1;
            proctab[i].state = PR_READY;
        }
    }
}


/* -------------------------------------------------- */
/* Process Manager Init                               */
/* -------------------------------------------------- */
void proc_run(void) {
    /* Find first ready process (should be PID 0 - null_process) */
    int next = -1;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proctab[i].state == PR_READY) {
            next = i;
            break;
        }
    }
    
    if (next == -1) {
        serial_puts("ERROR: No ready process to run!\n");
        while (1);
    }
    
    /* Set up initial process */
    proctab[next].state = PR_CURRENT;
    current_pid = next;
    currpid = &proctab[next];
    
    /* First dispatch (bootstrap) */
    first_dispatch = 0;
    asm volatile(
        "movl %0, %%esp \n"
        "ret            \n"
        :
        : "r"(proctab[next].esp)
    );

    
    /* Should never reach here */
    while (1);
}


void proc_init(void) {
    for (int i = 0; i < MAX_PROCS; i++) {
        proctab[i].pid = -1;
        proctab[i].state = PR_TERMINATED;
        proctab[i].entry = NULL;
        proctab[i].stack_base = NULL;
        proctab[i].esp = NULL;
        proctab[i].mem = NULL;
        proctab[i].memsz = 0;
        proctab[i].sleep_ticks = 0;
        proctab[i].wait_event = -1;
        proctab[i].priority = 1;
        proctab[i].dyn_priority = 1;
    }

    serial_puts("Process manager initialized.\n");
}

/* -------------------------------------------------- */
/* Process Creation                                   */
/* -------------------------------------------------- */

int32_t proc_create(void (*func)(void)) {
    int pid;

    for (pid = 0; pid < MAX_PROCS; pid++) {
        if (proctab[pid].state == PR_TERMINATED)
            break;
    }

    if (pid == MAX_PROCS)
        return -1;

    uint32_t *stack = mem_alloc(PROC_STACK_SIZE);
    if (!stack) {
        serial_puts("Stack allocation failed.\n");
        return -1;
    }

    uint32_t *sp = (uint32_t *)((uint32_t)stack + PROC_STACK_SIZE);
    sp = (uint32_t *)((uint32_t)sp & ~0xF);   // align

    *--sp = (uint32_t)proc_exit;  // return address
    *--sp = (uint32_t)func;       // first EIP

    proctab[0].priority = 0;
    proctab[0].dyn_priority = 0;
    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;
    proctab[pid].stack_base = stack;
    proctab[pid].esp = sp;
    proctab[pid].mem = stack;
    proctab[pid].memsz = PROC_STACK_SIZE;
    proctab[pid].priority = 1;
    proctab[pid].dyn_priority = 1;

    serial_puts("Process created with PID: ");
    serial_put_int(pid);
    serial_puts("\n");

    return pid;
}


/* -------------------------------------------------- */
/* Process Exit                                       */
/* -------------------------------------------------- */

void proc_exit(void) {
    int pid = currpid->pid;

    proctab[pid].state = PR_TERMINATED;
    mem_free(proctab[pid].mem);

    proctab[pid].mem = NULL;
    proctab[pid].esp = NULL;

    serial_puts("Process exited: ");
    serial_put_int(pid);
    serial_puts("\n");

    currpid = NULL;
    current_pid = -1;

    resched();
    while (1);
}

/* -------------------------------------------------- */
/* Process List                                       */
/* -------------------------------------------------- */

void proc_list(void) {
    serial_puts("PID\tSTATE\n");
    serial_puts("----------------\n");

    for (int i = 0; i < MAX_PROCS; i++) {
        if (proctab[i].state != PR_TERMINATED) {
            serial_put_int(i);
            serial_puts("\t");

            switch (proctab[i].state) {
                case PR_CURRENT: serial_puts("RUNNING"); break;
                case PR_READY:   serial_puts("READY");   break;
                case PR_SLEEP:   serial_puts("SLEEP");   break;
                case PR_WAIT:    serial_puts("WAIT");    break;
                default:         serial_puts("UNKNOWN"); break;
            }

            serial_puts("\n");
        }
    }
    serial_puts("\n");
}


int getpid(void) {
    if (currpid == NULL) return -1;
    return currpid->pid;
}
