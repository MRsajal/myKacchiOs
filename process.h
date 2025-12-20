#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "types.h"

#define MAX_PROCS 100

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
    uint32_t *stack_base;
    uint32_t *esp;
    void *mem;
    size_t memsz;

} pcb_t;

void proc_init(void);
int32_t proc_create(void (*func)(void));
void proc_run(void);
void proc_exit(void);
void resched(void);
void yield(void);

#endif