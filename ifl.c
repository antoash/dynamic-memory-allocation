#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Implicit free list implementation 

// prologue + epilogue
// coalesce + split
// 16 byte alignment

// The malloc function will return a pointer to the payload.
// However, all operations should be done from the head of the block. 

#define ALLOC_BLOCK                 16 
#define HEADER_SIZE                 8   
#define FOOTER_SIZE                 8
#define ALIGN_UP(size, align)       ((size + (align-1)) & ~(align-1)) 
#define SET_HEADER(size, set_flag)  ((size<<0x4) | set_flag)
#define DEBUG_PRINT(n)              printf("DEBUG VALUE %x\n", n)
#define DEBUG_PRINT_PTR(n)          printf("DEBUG POINTER %p\n", n)

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
    assert(malloc_init());

    printf("===== TEST 1: Basic Allocation =====\n");
    int* a = (int*)my_malloc(sizeof(int));
    *a = 10;
    printf("a payload = %d @ %p\n", *a, a);

    printf("\n===== TEST 2: Multiple Allocations =====\n");
    int* b = (int*)my_malloc(8);
    int* c = (int*)my_malloc(24);
    int* d = (int*)my_malloc(1);
    printf("b=%p c=%p d=%p\n", b, c, d);

    printf("\n===== TEST 3: Free Without Coalesce =====\n");
    my_free(c);   // middle block free (no neighbors free)

    printf("\n===== TEST 4: Coalesce With Next Only =====\n");
    my_free(b);   // b should coalesce with c

    printf("\n===== TEST 5: Coalesce With Next Only =====\n");
    int* e = (int*)my_malloc(16);
    my_free(e);   // standalone free
    int* f = (int*)my_malloc(16);
    my_free(f);   // coalesce with previous

    printf("\n===== TEST 6: Heap Extension =====\n");
    int* x = (int*)my_malloc(8);
    int* y = (int*)my_malloc(8);
    int* z = (int*)my_malloc(8); // extend heap

    my_free(x);
    my_free(z);
    my_free(y); // y should merge with x and its neighboring free block

    int* g = (int*)my_malloc(8);
    my_free(g);
    int* h = (int*)my_malloc(8);

    printf("\n===== TEST 7: Large Allocation =====\n");
    int* big = (int*)my_malloc(5000);
    assert(big != NULL);
    printf("big block at %p\n", big);

    printf("\n===== TEST 8: First Block After Prologue =====\n");
    my_free(a);
    int* first = (int*)my_malloc(8);
    my_free(first);

    printf("\n===== TEST 10: Stress Small Allocations =====\n");
    void* arr[20];
    for (int i = 0; i < 20; i++)
    {
        if(i>1)
        {
            arr[i] = my_malloc(1);
        }
        else
        {
            arr[i] = my_malloc(8);
        }
    }

    for (int i = 0; i < 20; i++)
        printf("%p\n",arr[i]);
    
    printf("-----\n");
    
    for (int i = 0; i < 20; i += 2)
        my_free(arr[i]);

    for (int i = 1; i < 20; i += 2)
        my_free(arr[i]);

    printf("\n===== TEST COMPLETE =====\n");

    printf("\nFinal Addresses:\n");
    printf("Prologue  = %p\n", pl);
    printf("Epilogue  = %p\n", el);
    printf("Head      = %p\n", alloc_head);

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
    if (remaining_size < 16) { // this should only return -> place this as a condition prior to split in allocate function. In allocate, it should move onto the next allocation
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
    if (sbrk(size) == (void*)-1)
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
        int alloc_size = ALIGN_UP(size + HEADER_SIZE + FOOTER_SIZE, 16); 
        if (!get_alloc_status(head) && get_size(head) >= alloc_size)
        {
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

void* my_malloc(size_t size)
{
    block_t* new_alloc = NULL;
    uint64_t* footer = NULL;

    new_alloc = find_alloc(size);

    return ((char*)new_alloc+8);
}

void coalesce(block_t* block, state_t state, uint64_t curr_size)
{
    switch (state)
    {
        case p_n:
        {
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
            block_t* next = next_block(block);
            block_t* new_footer = next_footer(next);

            uint64_t next_size = get_size(next);
            uint64_t new_size = next_size + curr_size;

            block->header = SET_HEADER(new_size, 0);            
            new_footer->header = SET_HEADER(new_size, 0);
            break;
        }

        case o_p:
        {
            block_t* prev = prev_block(block); 
            block_t* new_footer = next_footer(block);
            
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

    block_t* prev_foot = prev_footer(alloc_start);
    block_t* next = next_block(alloc_start);

    if (!get_alloc_status(prev_foot) && get_size(prev_foot)>0 && !get_alloc_status(next) && get_size(next)>0)
    {
        coalesce(alloc_start, p_n, size);
        DEBUG_PRINT(1);
    }
    else if (!get_alloc_status(prev_foot) && get_size(prev_foot)>0)
    {
        coalesce(alloc_start, o_p, size);
        DEBUG_PRINT(2);

    }
    else if (!get_alloc_status(next) && get_size(next)>0)
    {
        coalesce(alloc_start, o_n, size);
        DEBUG_PRINT(3);
    }
}