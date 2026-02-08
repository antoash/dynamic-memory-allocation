#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define ALLOC_BLOCK                 16 
#define HEADER_SIZE                 8   
#define FOOTER_SIZE                 8
#define ALIGN_UP(size, align)       ((size + (align-1)) & ~(align-1)) 
#define SET_HEADER(size, set_flag)  ((size<<0x4) | set_flag)
#define DEBUG_PRINT(n)              printf("DEBUG VALUE %x\n", n)
#define DEBUG_PRINT_PTR(n)          printf("DEBUG POINTER %p\n", n)

// Implicit free list implementation 

// prologue + epilogue
// coalescing blocks
// linking blocks
// 16 byte alignment

typedef struct Block 
{
    uint64_t header; // last 4 bits -> allocation status
    unsigned char payload[0]; // this will contain the footer too
} block_t;

bool malloc_init()
{

    block_t* prologue = (block_t*) sbrk(16);
    prologue->header = SET_HEADER(0, 1);

    block_t* epilogue = sbrk(16);
    epilogue->header = SET_HEADER(0, 1);

    return true;
}

void* my_malloc(size_t sz)
{
    int alloc_size = ALIGN_UP(sz, 16) + HEADER_SIZE + FOOTER_SIZE;
    block_t* new_alloc = sbrk(alloc_size);
    uint64_t* footer = (uint64_t*)((char*)new_alloc + alloc_size - FOOTER_SIZE); 

    new_alloc->header = SET_HEADER(alloc_size, 0x1);
    *footer = new_alloc->header;

    return ((uint64_t*)new_alloc+1);
}

void free(void* alloc)
{
    block_t* alloc_start = (block_t*)((uint64_t*)alloc - 1);
    int size = alloc_start->header>>0x4;
    uint64_t* footer = (uint64_t*)((char*)alloc_start + (alloc_start->header>>0x4) - FOOTER_SIZE);

    alloc_start->header &= ~0xF; 
    *footer &= ~0xF;
}

int main()
{
    bool start = false;

    if (malloc_init())
    {
        start = true;
    }

    int* alloc1 = NULL;
    int* alloc2 = NULL;
    int num = 4;

    alloc1 = (int*)(my_malloc(sizeof(int)));

    *alloc1 = num;

    alloc2 = (int*)(my_malloc(2));

    free((void*)alloc1);
    free((void*)alloc1);

//-------------------------------------------------------END----------------------------------------------------------------//
    void* end = sbrk(0);
    // printf("prolg address               = %p \n", prolg);
    // printf("prolg->header address = %p \n", &(prolg->header));
    // printf("prolg->payload address = %p \n", &(prolg->payload)); 
    
    printf("alloc1 address              = %p \n", alloc1);
    // printf("alloc1->header address      = %p \n", (void*)&(alloc1->header));
    // printf("alloc1->payload address     = %p \n", (void*)&(alloc1->payload)); 
    // printf("alloc1->footer address      = %p \n", (void*)((char*)alloc1 + aligned_sz - HEADER_SIZE));

    // printf("num address = %p \n", num);
    // printf("Address of num: %p matches address of alloc1->payload: %p \n", num, (void*)(alloc1+1));
    // printf("num: %p matches alloc1->payload: %p \n", *num, alloc1->payload[0]);

    printf("alloc2 address              = %p \n", alloc2);
    // printf("alloc2->header address      = %p \n", (void*)&(alloc2->header));
    // printf("alloc2->payload address     = %p \n", (void*)&(alloc2->payload)); 
    // printf("alloc2->footer address      = %p \n", (void*)footer);
    
    // printf("alloc3 address              = %p \n", alloc3);
    // printf("alloc3->header address      = %p \n", (void*)&(alloc3->header));
    // printf("alloc3->payload address     = %p \n", (void*)&(alloc3->payload)); 
    // printf("alloc3->footer address      = %p \n", (void*)footer3);

    // printf("alloc4 address              = %p \n", alloc4);
    // printf("alloc4->header address      = %p \n", (void*)&(alloc4->header));
    // printf("alloc4->payload address     = %p \n", (void*)&(alloc4->payload)); 
    // printf("alloc4->footer address      = %p \n", (void*)footer4);

    // printf("sbrk end                    = %p \n", end);
}
