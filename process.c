#include "process.h"
#include "serial.h"
#include "memory.h"

#define PROC_STACK_SIZE 4096

static pcb_t proctab[MAX_PROCS];
static int32_t current_pid = -1;
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

/* -------------------------------------------------- */
/* Process Manager Init                               */
/* -------------------------------------------------- */

void proc_init(void) {
    for (int i = 0; i < MAX_PROCS; i++) {
        proctab[i].pid = -1;
        proctab[i].state = PR_TERMINATED;
        proctab[i].entry = NULL;
        proctab[i].stack_base = NULL;
        proctab[i].esp = NULL;
        proctab[i].mem = NULL;
        proctab[i].memsz = 0;
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

    proctab[pid].pid = pid;
    proctab[pid].state = PR_READY;
    proctab[pid].entry = func;
    proctab[pid].stack_base = stack;
    proctab[pid].esp = sp;
    proctab[pid].mem = stack;
    proctab[pid].memsz = PROC_STACK_SIZE;

    serial_puts("Process created with PID: ");
    serial_put_int(pid);
    serial_puts("\n");

    return pid;
}

/* -------------------------------------------------- */
/* Scheduler Core                                     */
/* -------------------------------------------------- */

void resched(void) {
    int old = current_pid;
    int next = -1;

    int start = (old < 0) ? 0 : old;

    /* Round-robin search */
    for (int i = 1; i <= MAX_PROCS; i++) {
        int pid = (start + i) % MAX_PROCS;
        if (proctab[pid].state == PR_READY) {
            next = pid;
            break;
        }
    }

    /* No READY process → idle (PID 0) */
    if (next == -1)
        next = 0;

    /* Already running */
    if (next == old && old >= 0)
        return;

    /* Update states */
    if (old >= 0 && proctab[old].state == PR_CURRENT)
        proctab[old].state = PR_READY;

    proctab[next].state = PR_CURRENT;
    current_pid = next;
    currpid = &proctab[next];

    /* First dispatch (bootstrap) */
    if (first_dispatch) {
        first_dispatch = 0;
        asm volatile(
            "movl %0, %%esp \n"
            "jmp  *%1      \n"
            :
            : "r"(proctab[next].esp),
              "r"(proctab[next].entry)
        );
        while (1);
    }

    /* Normal context switch */
    ctxsw(&proctab[old].esp, &proctab[next].esp);
}

/* -------------------------------------------------- */
/* Yield                                              */
/* -------------------------------------------------- */

void sched_yield(void) {
    if (currpid)
        currpid->state = PR_READY;
    resched();
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
                default:         serial_puts("UNKNOWN"); break;
            }
            serial_puts("\n");
        }
    }
    serial_puts("\n");
}
