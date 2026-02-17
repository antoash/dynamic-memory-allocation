#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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

// The malloc function will return a pointer to the payload.
// However, all operations should be done from the head of the block. 
// Every allocation will be created 

typedef enum state 
{
    p_n = 0, // prev and next allocations along with current are empty
    o_p = 1, // only prev allocation and current are free
    o_n = 2 // only next allocation and current are free
}state_t;

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

    return 0;
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

block_t* prev_block(block_t* block)
{
    block_t* footer = (block_t*)((char*)block - 8);
    uint64_t prev_size = get_size(footer);
    return (block_t*)((char*)(block) - prev_size);
}

block_t* prev_footer(block_t* block)
{
    return (block_t*)((char*)(block) - 8);
}

block_t* next_footer(block_t* block)
{
    // find the immediate footer
    uint64_t size = get_size(block);
    return (block_t*)((char*)block + size - FOOTER_SIZE);
}

block_t* get_block_head(block_t* block)
{
    return (block_t*)((char*)(block) - 8);
}

bool malloc_init()
{
// PADDING | PROLOGUE HEADER | PROLOGUE FOOTER | EPILOGUE HEADER
    block_t* start = (block_t*) sbrk(8 * 4); 
    pl = (block_t*)((char*)start + 8);
    pl->header = SET_HEADER(16, 1);

    block_t* footer = (block_t*)((char*)pl + 8);
    footer->header = SET_HEADER(16, 1);

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

void allocate(block_t* head, uint64_t new_size)
{
    int old_size = get_size(head);
    if (old_size > new_size) // update this to ensure the min block size is respected
    {
        split(head, old_size, new_size);
    }
    else
    {
        head->header = SET_HEADER(new_size, 1);
        block_t* footer = (block_t*)((char*)head + new_size - FOOTER_SIZE);
        footer->header = SET_HEADER(new_size, 1);
    }
}

block_t* extend_heap(uint64_t size)
{
    block_t* new_alloc = el;
    
    // epilogue is required when extending the allocation
    if (sbrk(size + 8) == (void*)-1)
        return NULL;

    new_alloc->header = SET_HEADER(size, 0);
    block_t* footer = (block_t*)((char*)new_alloc + size - FOOTER_SIZE);

    footer->header = SET_HEADER(size, 0);
    
    el = (block_t*)((char*)footer + 8);
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
            int alloc_size = ALIGN_UP(size + HEADER_SIZE + FOOTER_SIZE, 16); 
            // allocate(head, size);
            allocate(head, alloc_size);
            return head;
        }
        head = next_block(head);
    }
    int alloc_size = ALIGN_UP(size + HEADER_SIZE + FOOTER_SIZE + 8, 16); // adding the epilogue to alignment 
    block_t* new_alloc = extend_heap(alloc_size);
    if (!new_alloc) return NULL;

    allocate(new_alloc, alloc_size);
    return new_alloc;
}

void* my_malloc(size_t sz)
{
    // int alloc_size = ALIGN_UP(sz + HEADER_SIZE + FOOTER_SIZE, 16); 
    block_t* new_alloc = NULL;
    uint64_t* footer = NULL;

    // new_alloc = find_alloc(alloc_size);
    new_alloc = find_alloc(sz);

    return ((uint64_t*)new_alloc+1);
}

void coalesce(block_t* block, state_t s, uint64_t curr_size)
{
    switch (s)
    {
        case p_n:
        {
            // new size = prev block + current block + next block
            // update prev header
            // update next footer
            block_t* prev = prev_block(block); 
            block_t* next = next_block(block);
            block_t* new_footer = next_footer(next);

            uint64_t prev_size = get_size(prev);
            uint64_t next_size = get_size(next);

            uint64_t new_size = next_size + prev_size + curr_size;

            prev->header = SET_HEADER(new_size, 0);
            new_footer->header = SET_HEADER(new_size, 0);
            break;
        }
        case o_n:
        {
            // new size = next size + curr size
            // update current header
            // update next footer
            block_t* next = next_block(block);
            block_t* new_footer = next_footer(next);

            // new size = next size + curr size
            uint64_t next_size = get_size(next);
            uint64_t new_size = next_size + curr_size;

            // update current header
            block->header = SET_HEADER(new_size, 0);
            
            // update next footer
            new_footer->header = SET_HEADER(new_size, 0);
            break;
        }

        case o_p:
        {
            // new size = prev size + curr size
            // update prev header
            // update current footer

            block_t* prev = prev_block(block); 
            block_t* new_footer = next_footer(block);
            
            // new size = prev size + curr size
            uint64_t prev_size = get_size(prev);
            uint64_t new_size = prev_size + curr_size;

            prev->header = SET_HEADER(new_size, 0);
            new_footer->header = SET_HEADER(new_size, 0);
            break;
        }

        default:
            break; 
    }

}

void my_free(void* alloc)
{
    printf("%p\n", alloc);
    block_t* alloc_start = get_block_head(alloc); 
    int size = get_size(alloc_start);
    uint64_t* footer = (uint64_t*)((char*)alloc_start + size - FOOTER_SIZE);

    alloc_start->header &= ~0xF;
    *footer &= ~0xF;

    block_t* prev = prev_footer(alloc_start);
    block_t* next = next_block(alloc_start);

    uint64_t prev_state = get_alloc_status(prev);
    uint64_t next_state = get_alloc_status(next);

    if (!prev_state && get_size(prev)>0 && !next_state && get_size(next)>0)
    {
        // coalesce
        coalesce(alloc_start, p_n, size);
        DEBUG_PRINT(1);
    }
    else if (!prev_state && get_size(prev)>0)
    {
        // coalesce
        coalesce(alloc_start, o_p, size);
        DEBUG_PRINT(2);

    }
    else if (!next_state && get_size(next)>0)
    {
        // coalesce
        coalesce(alloc_start, o_n, size);
        DEBUG_PRINT(3);
    }
}