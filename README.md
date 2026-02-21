**overview**
- implementing a simple implicit free list based memory allocator to better understand dynamic memory allocation by stripping it down to its foundational concepts.
- each allocation block contains a Header - 8 bytes, Footer - 8 bytes and Payload - size determined by user.
- size and allocation status of a block is stored in the header and footer. The last 4 bits are reserved for allocation status - '0' if allocated, '1' if unallocated. 
- padding is added to ensure alignment is maintained.
- the end of the heap is identified with an Epilogue block, with size defined as '0' and allocation status set.      

**notes during development**     
- [Dynamic Memory Allocation: Basic Concepts](https://www.cs.cmu.edu/~213/lectures/13-malloc-basic.pdf) - starting out with this to understand basic concepts to get started - internal/external fragmentation, `brk`/`sbrk`, coalescing + splitting etc.    
- pointer arithmetic -> when adding 1, 2, 3 to a pointer address, the increment depends on the size of the data type of object it points to.      
- figuring out alignment!     
	- there are two types of alignment        
		- aligning the total size as a multiple of the minimum word size          
		- aligning the memory space address to be a multiple of 16/8 etc.         
			- a simple mask is used: `~(ALIGN - 1)`                  
			- operation to round up to the next equivalent reference point (always ensures the mask will round up to the multiple not down)`(SIZE + (ALIGN - 1))`      
			- full formula - `(SIZE + (ALIGN - 1)) & ~(ALIGN - 1)`      
			- Ref: [Generating Aligned Memory - Embedded Artistry](https://embeddedartistry.com/blog/2017/02/22/generating-aligned-memory/)      
- able to implement skeleton code for this memory allocator      
- trying to deref `sbrk(0)` will lead to seg faults (was not aware of this) - the address returned is just outside of the current allocated portion of the heap.      
- learning to use gdb is a game changer! able to live debug is so much better than relying on `printf` for seg faults. this video was enough to get a hang of the basics - Ref: [GDB Tutorial](https://www.youtube.com/watch?v=svG6OPyKsrw). making good progress with splitting and coalescing.        
- added a footer to make it easier to coalesce and split blocks. the excel demo part of this video really helped with visualizing how the allocation/freeing is supposed to deal with split/coalesce. Ref: [CS 208 Allocator Design and the Implicit Free List](https://www.youtube.com/watch?v=xiPew2TmsDE)                
- current implementation doesn't handle fragmentation well - no algorithm to improve throughput + mem utilization, it works on finding the first block that meets requirements. this can be improved.          
