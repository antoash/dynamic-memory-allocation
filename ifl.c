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
    unsigned char payload[0];
} block_t;

bool malloc_init();
void* my_malloc(size_t);
void my_free(void*);
uint64_t get_size(block_t*);
uint64_t get_alloc_status(block_t*);

static block_t* alloc_head = NULL;
static block_t* pl = NULL;
static block_t* el = NULL;

int main()
{
    bool start = false;

    if (malloc_init())
    {
        start = true;
    }

    int* alloc1 = (int*)(my_malloc(sizeof(int)));
    int* alloc2 = (int*)(my_malloc(2));
    int num = 4;

    *alloc1 = 4;
    *alloc2 = 33;
    my_free((void*)alloc1);
    // my_free((void*)alloc2);
    int* alloc3 = (int*)(my_malloc(sizeof(int)));
    void* end = sbrk(0);
    
}

uint64_t get_size(block_t* block)
{
    return (uint64_t)(block->header>>0x4);
}

uint64_t get_alloc_status(block_t* block)
{
    return (uint64_t)(block->header & 0xF);
}

block_t* next_block(block_t* block)
{
    return (block_t*)((char*)(block) + get_size(block));
}

bool malloc_init()
{
// PADDING | PROLOGUE HEADER | PROLOGUE FOOTER | EPILOGUE HEADER
    block_t* start = (block_t*) sbrk(8 * 4); 
    pl = (block_t*)((char*)start + 8);
    pl->header = SET_HEADER(16, 1);

    block_t* epilogue = (block_t*)((char*)pl + 16);
    el = epilogue;
    epilogue->header = SET_HEADER(0, 1);

    alloc_head = (block_t*)((char*)pl + 16);
    return true;
}

void split(block_t* head, uint64_t old_size, uint64_t new_size)
{   
    uint64_t remaining_size = old_size - new_size;
    if (remaining_size < 16) {
        head->header = SET_HEADER(old_size, 1);
        block_t* footer = (block_t*)((char*)head + old_size - FOOTER_SIZE);
        footer->header = SET_HEADER(old_size, 1);
        return;
    }

    head->header = SET_HEADER(new_size, 1);

    block_t* footer = (block_t*)((char*)head + new_size - FOOTER_SIZE);
    footer->header = SET_HEADER(new_size, 1);

    block_t* new_block = (block_t*)((char*)head + new_size);
    new_block->header = SET_HEADER(remaining_size, 0);

    footer = (block_t*)((char*)new_block + remaining_size -FOOTER_SIZE);
    footer->header = SET_HEADER(remaining_size, 0);
}

void allocate(block_t* head, uint64_t size)
{
    int old_size = get_size(head);
    if (old_size > size) // update this to ensure the min block size is respected
    {
        split(head, old_size, size);
    }
    else
    {
        head->header = SET_HEADER(size, 1);
    }
}

block_t* extend_heap(uint64_t size)
{
    block_t* new_alloc = el;
    
    if (sbrk(size) == (void*)-1)
        return NULL;

    new_alloc->header = SET_HEADER(size, 0);
    block_t* footer = (block_t*)((char*)new_alloc + size - FOOTER_SIZE);
    footer->header = SET_HEADER(size, 0);
    
    el = (block_t*)((char*)new_alloc + size);
    el->header = SET_HEADER(0, 1);

    return new_alloc;
}

block_t* find_alloc(uint64_t size)
{
    block_t* head = alloc_head; 
    
    while(get_size(head)>0) 
    {
        if (!get_alloc_status(head) && get_size(head) >= size)
        {
            allocate(head, size);
            return head;
        }
        head = next_block(head);
    }
    block_t* new_alloc = extend_heap(size);
    if (!new_alloc) return NULL;

    allocate(new_alloc, size);
    return new_alloc;
}

void* my_malloc(size_t sz)
{
    int alloc_size = ALIGN_UP(sz + HEADER_SIZE + FOOTER_SIZE, 16); 
    block_t* new_alloc = NULL;
    uint64_t* footer = NULL;

    new_alloc = find_alloc(alloc_size);

    return ((uint64_t*)new_alloc+1);
}

void my_free(void* alloc)
{
    block_t* alloc_start = (block_t*)((uint64_t*)alloc - 1);
    int size = alloc_start->header>>0x4;
    uint64_t* footer = (uint64_t*)((char*)alloc_start + (alloc_start->header>>0x4) - FOOTER_SIZE);

    alloc_start->header &= ~0xF; 
    *footer &= ~0xF;
}