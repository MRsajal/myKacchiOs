#include "scheduler.h"
#include "process.h"
#include "serial.h"

extern pcb_t proctab[MAX_PROCS];
extern int32_t current_pid;
extern pcb_t *currpid;

/* -------------------------------------------------- */
/* Scheduler Init                                      */
/* -------------------------------------------------- */

void sched_init(void){
    serial_puts("Scheduler initialized.\n");
}

void sched_start(void){
    serial_puts("Scheduler started.\n");
    proc_run();
}

/* -------------------------------------------------- */
/* Yield                                               */
/* -------------------------------------------------- */

void sched_yield(void){
    asm volatile("movl %%esp, %0" : "=r"(currpid->esp));
    currpid->state = PR_READY;
    resched();
}

/* -------------------------------------------------- */
/* Scheduler Core                                      */
/* -------------------------------------------------- */

void resched(void){
    aging_update();

    int old = current_pid;
    int next = -1;
    int best_prio = -1;

    for (int i = 0; i < MAX_PROCS; i++) {
        if (proctab[i].state == PR_READY) {
            if (proctab[i].dyn_priority > best_prio) {
                best_prio = proctab[i].dyn_priority;
                next = i;
            }
        }
    }

    if (next == -1)
        next = 0;

    /* DO NOT switch to same process */
    if (next == old && old >= 0)
        return;

    proctab[next].dyn_priority = proctab[next].priority;

    if (old >= 0 && proctab[old].state == PR_CURRENT)
        proctab[old].state = PR_READY;

    proctab[next].state = PR_CURRENT;
    current_pid = next;
    currpid = &proctab[next];

    asm volatile(
        "movl %0, %%esp \n"
        "ret            \n"
        :
        : "r"(proctab[next].esp)
    );

    while (1);
}

/* -------------------------------------------------- */
/* Aging                                               */
/* -------------------------------------------------- */

void aging_update(void){
    for (int i = 1; i < MAX_PROCS; i++) {
        if (proctab[i].state == PR_READY) {
            proctab[i].dyn_priority++;
        }
    }
}
