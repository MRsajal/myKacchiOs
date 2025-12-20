#include "mem.h"
#include "serial.h"

#define HEAP_SIZE 64*1024  // 64 KB heap size

typedef struct mem_block
{
    size_t size;
    int free;
    struct mem_block *next;
} mem_block_t;

static uint8_t heap[HEAP_SIZE];
static mem_block_t *free_list = NULL;

// Initialize the memory manager
void mem_init(void){
    free_list = (mem_block_t*)heap;
    free_list->size = HEAP_SIZE - sizeof(mem_block_t);
    free_list->free = 1;
    free_list->next = NULL;

    serial_puts("Memory manager initialized.\n");
}

// Allocate memory
void *mem_alloc(size_t size){
    mem_block_t *curr = free_list;
    size = (size + 3) & ~3; // Align size to 4 bytes
    while (curr)
    {
        if(curr->free && curr->size >=size){
            if (curr->size >size+sizeof(mem_block_t)){
                mem_block_t* new_block=(mem_block_t*)((uint8_t*)curr+sizeof(mem_block_t)+size);
                new_block->size = curr->size -size - sizeof(mem_block_t);
                new_block->free = 1;
                new_block->next = curr->next;

                curr->next=new_block;
                curr->size = size;
            }
            curr->free = 0;
            return (uint8_t*)curr + sizeof(mem_block_t);
        }
        curr=curr->next;
    }
    return NULL;
}


// Free allocated memory
void mem_free(void *ptr){
    if (!ptr) return;

    mem_block_t* block=(mem_block_t*)((uint8_t*)ptr-sizeof(mem_block_t));
    block->free = 1;

    mem_block_t* curr = free_list;
    while(curr && curr->next){
        if(curr->free && curr->next->free){
            curr->size += sizeof(mem_block_t)+curr->next->size;
            curr->next = curr->next->next;
        }else{
            curr = curr->next;
        }
    }
}