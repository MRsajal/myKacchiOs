/* kernel.c - Main kernel with null process */
#include "types.h"
#include "serial.h"
#include "string.h"
#include "process.h"
#include "memory.h"
#include "scheduler.h"

#define MAX_INPUT 128
static int user_pid=1;
void userProcess(void);
void int_to_string(int32_t value, char *str);


void null_process(void){
    char input[MAX_INPUT];
    int pos;
    /* Main loop - the "null process" */
    while (1) {
        serial_puts("kacchiOS> ");
        pos = 0;
        
        /* Read input line */
        while (1) {
            char c = serial_getc();
            
            /* Handle Enter key */
            if (c == '\r' || c == '\n') {
                input[pos] = '\0';
                serial_puts("\n");
                break;
            }
            /* Handle Backspace */
            else if ((c == '\b' || c == 0x7F) && pos > 0) {
                pos--;
                serial_puts("\b \b");  /* Erase character on screen */
            }
            /* Handle normal characters */
            else if (c >= 32 && c < 127 && pos < MAX_INPUT - 1) {
                input[pos++] = c;
                serial_putc(c);  /* Echo character */
            }
        }
        if(strcmp(input,"help")==0){
            serial_puts("Commands:\n");
            serial_puts("new - create new process\n");
            serial_puts("psa - list running processes\n");
            serial_puts("\n");
        }
        else if(strcmp(input,"new")==0){
            char pid_str[12];
            int pid = proc_create((void(*)(void))userProcess);
            int_to_string(pid, pid_str);
            serial_puts("[+] New process created with PID: ");
            serial_puts(pid_str);
            serial_puts("\n");

            sched_yield();
        }
        else if(strcmp(input,"psa")==0){
            serial_puts("Current running processes:\n");
            proc_list();
        }
        else if(pos>0){
            serial_puts("Unknown command: ");
            serial_puts(input);
            serial_puts("\n");
        }
        
        /* Echo back the input */
        // if (pos > 0) {
        //     serial_puts("You typed: ");
        //     serial_puts(input);
        //     serial_puts("\n");
        // }
    }
    
}



void int_to_string(int32_t value, char *str) {
    int i = 0;
    int is_negative = 0;

    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value != 0) {
        str[i++] = (value % 10) + '0';
        value /= 10;
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    /* Reverse string */
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}


void procA(void){
    while(1){
        serial_puts("Process A is running.\n");
        sched_yield();
    }
}
void procB(void){
    while(1){
        serial_puts("Process B is running.\n");
        sched_yield();
    }
}

void userProcess(void){
    char pid_str[12];

    serial_puts("[User Task Started]\n");

    serial_puts("User Process PID: ");
    int_to_string(user_pid, pid_str);
    serial_puts(pid_str);
    serial_puts("\n");
    for(volatile int i=0;i<10000000;i++); // Delay loop
    proc_init();
    serial_puts("[User Task Finished]\n");
    proc_exit();   // 🔥 MUST add this → removes PCB + returns to null
}


void kmain(void) {
    serial_init();
    mem_init();

    serial_puts("\n===============================");
    serial_puts("\n    kacchiOS - Minimal Baremetal OS\n");
    serial_puts("===============================\n");
    serial_puts("Hello from kacchiOS!\n");

    proc_init();
    proc_create(null_process);  /* PID 0 */
    sched_start();

    /* Should never reach here */
    for (;;) {
        __asm__ volatile ("hlt");
    }
}