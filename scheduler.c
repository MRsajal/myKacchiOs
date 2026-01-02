#include "scheduler.h"
#include "process.h"
#include "serial.h"

void sched_init(void){
    serial_puts("Scheduler initialized.\n");
}

void sched_start(void){
    serial_puts("Scheduler started.\n");
    proc_run();
}

void sched_yield(void){
    yield();
}