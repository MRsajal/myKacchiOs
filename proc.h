#ifndef _PROC_H_
#define _PROC_H_

#include "types.h"

#define MAX_PROCS 8

typedef enum
{
    PR_CURRENT,
    PR_READY,
    PR_TERMINATED
} proc_state_t;

typedef struct process
{
    int pid;
    proc_state_t state;
    void (*entry)(void);

} pcb_t;

void proc_init(void);
pcb_t *proc_create(void (*func)(void));
void proc_run(void);
void proc_exit(void);

#endif