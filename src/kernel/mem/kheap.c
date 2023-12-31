#include <string.h>
#include <serial.h>
#include <printf.h>
#include <paging.h>
#include <kheap.h>
#include <system.h>
#include <math.h>

// // kheap.h -- Interface for kernel heap functions, also provides
//            a placement malloc() for use before the heap is
//            initialised.
//            Based on code from JamesM's kernel development tutorials.
// http://www.jamesmolloy.co.uk/
// Global Var

struct Block * head = NULL;       // First memory block
struct Block * tail = NULL;	  // Last memory block
struct Block * freelist = NULL;	  // All the memory blocks that are freed


void * heap_start;    // Where heap starts (must be page-aligned)
void * heap_end;      // Where heap ends (must be page-aligned)
void * heap_curr;     // Top of heap
void * heap_max;      // Maximum heap_end

int kheap_enabled = 0;
// Defined in paging.c
extern page_directory_t *kpage_dir;

// Defined in link.ld, end of kernel executable, aka. all the memory you can use
extern uint32_t end;

// Defined in kheap.h
extern void * heap_start, * heap_end, * heap_max, * heap_curr;

uint32_t placement_address = (uint32_t)&end;

void * kmalloc_cont(uint32_t sz, int align, uint32_t *phys) {

    if (align == 1 && (placement_address & 0xFFFFF000) )
    {
        // Align the placement address;
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys)
    {
        *phys = placement_address;
    }
    uint32_t tmp = placement_address;
    placement_address += sz;
    return (void*)tmp;

}
/*
   kmalloc wrapper
   when heap is not created, use a placement memory allocator
   when heap is created, use malloc(), the dynamic memory allocator
align: return a page-aligned memory block
phys: return the physical address of the memory block
*/
uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phys)
{
    if (heap_start != NULL)
    {
        // This will guarantee a block that's enough for data of size sz and aligned to 4kb boundary
        if(align) sz = sz + 4096;
        void * addr = malloc(sz);
        uint32_t align_addr = ((uint32_t)addr & 0xFFFFF000) + 0x1000;
        if (phys != 0)
        {
            /*
            page_t *page = get_page(align_addr, 0, kernel_directory);
            *phys = page->frame*0x1000 + (align_addr & 0xFFF);
            */
            uint32_t t = (uint32_t)addr;
            if(align)
                t = align_addr;
             *phys = (uint32_t)virtual2phys(kpage_dir, (void*)t);

        }
        // Return the aligned address
        if(align)
            return align_addr;
        return (uint32_t)addr;
    }
    else
    {
        if (align == 1 && (placement_address & 0xFFFFF000) )
        {
            // Align the placement address;
            placement_address &= 0xFFFFF000;
            placement_address += 0x1000;
        }
        if (phys)
        {
            *phys = placement_address;
        }
        uint32_t tmp = placement_address;
        placement_address += sz;
        return tmp;
    }
}

/*
   kmalloc, align
   */
void * kmalloc_a(uint32_t sz)
{
    return kmalloc_int(sz, 1, 0);
}

/*
   kmalloc, get physical address
   */
uint32_t kmalloc_p(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 0, phys);
}

/*
   kmalloc, align and get physical address
   */
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys)
{
    return kmalloc_int(sz, 1, phys);
}

/*
   just malloc
   */
void * kmalloc(uint32_t sz)
{
    return (void*)kmalloc_int(sz, 0, 0);
}


/*
   malloc a s block and memset
   */
void *kcalloc(uint32_t num, uint32_t size) {
    void * ptr = malloc(num * size);
    memset(ptr, 0, num*size);
    return ptr;
}
/*
   wrapper function for realloc
   */
void * krealloc(void * ptr, uint32_t size) {
    // TODO: optimize realloc, for now, just simeply malloc and move data over
    return realloc(ptr, size);
}

/*
   wrapper function for free
   */
void kfree(void * ptr) {
    free(ptr);
}
/*
   Initialize heap
   */
void kheap_init(void * start, void * end, void * max) {
    heap_start = start;
    heap_end = end;
    heap_max = max;
    heap_curr = start;
    kheap_enabled = 1;
}

/*
   Given the field size in a Block(which contain free/alloc information), extract the size.
这个函数的作用可能是为了确保字段的大小始终为偶数值，可能与内存管理或数据结构的实现有关。
   */
uint32_t getRealSize(uint32_t size) {
    return (size >> 1) << 1;
}

/*
   Print the heap visually, for debug.
   */
void db_print() {
    if(!head) {qemu_printf("your heap is empty now\n");return;}
    //qemu_printf("HEAP:\n");
    uint32_t total = 0;
    uint32_t total_overhead = 0;
    struct Block * curr = head;
    while(1) {
        // print it
        char c = 'A';
        if(isFree(curr)) c = 'F';
        uint32_t a = getRealSize(curr->size);
        qemu_printf("| %u |.%c.| %u | ", a, c, a);
        total = total + getRealSize(curr->size);
        total_overhead = total_overhead + OVERHEAD;
        if(isEnd(curr)) break;
        void * ptr = (void*)curr + OVERHEAD + getRealSize(curr->size);
        curr = ptr;
    }
    qemu_printf("\n total usable bytes: %d", total);
    qemu_printf("\n total overhead bytes: %d", total_overhead);
    qemu_printf("\n total bytes: %d", total + total_overhead);
    qemu_printf("\nfreelist: ");

    struct Block * ite = freelist;
    while(ite) {
        qemu_printf("(%p)->", ite);
        ite = ite->next;
    }
    qemu_printf("\n\n");
    return;
}

/*
 *  Is n the end of all mem blocks ?
 *
 */

int isEnd(struct Block * n) {
    return n == tail;
}
/*
 * Does this block fitit
 *
 * */
int doesItFit(struct Block * n, uint32_t size) {
    return n->size >= getRealSize(size) && isFree(n);
}


/*
https://pages.cs.wisc.edu/~fischer/cs536.s06/course.hold/html/NOTES/12.HEAP-MANAGEMENT.html
hniques for coalescing
There are also several possible ways to solve the second problem (how to coalesce freed storage). One approach is to use a doubly linked list (i.e., each list item has a "previous" as well as a "next" pointer). Also, one bit of the "size" field is reserved to indicate whether the chunk is "free" or "in-use". Now when a chunk is freed, we can check the "free-bit" of the storage that immediately follows the freed chunk (using the freed chunk's "size" bit to locate the "size" field of the following chunk of storage). If that following storage is free, then the two chunks can be coalesced. For example, suppose the situation is like this:

           +------------------------------------+   +--------+
           |                                    |   |        |
           v                                    |   |        v
+---+    +-----------------+ +---------+ +------|---|----+ +--------------+
|   |    |   |   |   |     | |    |    | |    | | | | |  | |   |   |   |  |   
| o---+  |   | \ | o |     | | 10 |    | | 20 | o | o |  | |   | o | \ |  |
|   | |  |   |   | | |     | |    |    | |    |   |   |  | |   | | |   |  |
+---+ |  +---------|-------+ +---------+ +---------------+ +-----|--------+
first |  size prev next       size       size prev next    size prev next
free  |    ^       |                      ^ ^                    |
      |    |       |                      | |                    |
      +----+       +----------------------+ +--------------------+
*/

/*
 * Set the free/alloc bit of the size field
 * free, alloc bit in size. 
 * */
void setFree(uint32_t *size, int x) {
    if(x) {
        *size = *size | 1;
        return;
    }
    *size = *size & 0xFFFFFFFE;
}

/*
 * Check if a block is freed or allocated
 *
 * */
int isFree(struct Block * n) {
    if(!n) return 0;
    return (n->size & 0x1);
}
/*
 * Remove the node from freelist
 *
 * */

void removeNodeFromFreelist(struct Block * x) {
    if(!x) return;
    if(x->prev) {
        x->prev->next = x->next;
        if(x->next)
            x->next->prev = x->prev;
    }
    else {
        freelist = x->next;
        if(freelist)
            freelist->prev = NULL;
    }
}
/*
 * Insert the node to freelist
 *
 * */
void addNodeToFreelist(struct Block * x) {
    if(!x) return;
    x->next = freelist;
    if(freelist)
        freelist->prev = x;
    freelist = x;
    freelist->prev = NULL;
}

/*
 * Find the bestfit block in the memory pool
 * 找最小适合
 * */
// // struct Block: size|prev|next;
struct Block * bestfit(uint32_t size) {
    // extend this, may be, optimize for certain size of type
    if(!freelist) return NULL;
    struct Block * curr = freelist;
    struct Block * currBest = NULL;
    while(curr) {
        if(doesItFit(curr, size)) {
            if(currBest == NULL || curr->size < currBest->size)
                currBest = curr;
        }
        curr = curr ->next;
    }
    return currBest;;
}

/*
 * Given a block , find its previous block
 *
 * */
struct Block * getPrevBlock(struct Block * n) {
    if(n == head) return NULL;
    // get previous block size
    void * ptr = n;
    uint32_t * uptr = ptr - sizeof(uint32_t);
    uint32_t prev_size = getRealSize(*uptr);
    void * ret = ptr - OVERHEAD - prev_size;
    return ret;
}
/*
 * Given a block , find its next block
 *
 * */
struct Block * getNextBlock(struct Block * n) {
    if(n == tail) return NULL;
    void * ptr = n;
    ptr = ptr + OVERHEAD + getRealSize(n->size);
    return ptr;
}

/*
 * The real malloc function
 * reference: http://www.jamesmolloy.co.uk/tutorial_html/7.-The%20Heap.html
 * 
 7.2.1. Allocation
Allocation is straightforward, if a little long-winded. Most of the steps are error-checking and creating new holes to minimise memory leaks.

Search the index table to find the smallest hole that will fit the requested size. As the table is ordered, this just entails iterating through until we find a hole which will fit.
If we didn't find a hole large enough, then:
Expand the heap.
If the index table is empty (no holes have been recorded) then add a new entry to it.
Else, adjust the last header's size member and rewrite the footer.
To ease the number of control-flow statements, we can just recurse and call the allocation function again, trusting that this time there will be a hole large enough.
Decide if the hole should be split into two parts. This will normally be the case - we usually will want much less space than is available in the hole. The only time this will not happen is if there is less free space after allocating the block than the header/footer takes up. In this case we can just increase the block size and reclaim it all afterwards.
If the block should be page-aligned, we must alter the block starting address so that it is and create a new hole in the new unused area.
If it is not, we can just delete the hole from the index.
Write the new block's header and footer.
If the hole was to be split into two parts, do it now and write a new hole into the index.
Return the address of the block + sizeof(header_t) to the user.

Bad assumption. NEVER assume anything with malloc() apart from what is explicitly in the man page.

First, the space is rounded up to your system's idea of the hardest thing to align. Your system seems to give 16 bytes minimum. I would guess this is 8 bytes for the internal header and 8 bytes for a double.

The reason this happens: suppose you malloc space for an array of 10 structs. The compiler needs to generate code to access the members the same in each item. So they need to be padded to a standard size. The worst case is where a double might be in the struct - it needs 8-byte alignment on some systems. Most mallocs just assume 8 or 16 byte alignment, some figure it out from machine.h or similar.

Second, the sequence of mallocs can depend on all kinds of stuff. It all looks simple when you do the first few. But when space has been freed, malloc can go back and re-use it, or keep giving you fresh space, or anything else it likes. Maybe you did some debug with fptintf (stderr...). Well, stdio allocates buffers the first time you access the stream, so that was maybe 2048 bytes being allocated just when you didn't expect it.


The basic job of malloc and friends is to manage the fact that the OS can generally only (efficiently) deal with large allocations (whole pages and extents of pages), while programs often need smaller chunks and finer-grained management.

So what malloc (generally) does, is that the first time it is called, it allocates a larger amount of memory from the system (via mmap or sbrk -- maybe one page or maybe many pages), and uses a small amount of that for some data structures to track the heap use (where the heap is, what parts are in use and what parts are free) and then marks the rest of that space as free. It then allocates the memory you requested from that free space and keeps the rest available for subsequent malloc calls.

So the first time you call malloc for eg 16 bytes, it will uses mmap or sbrk to allocate a large chunk (maybe 4K or maybe 64K or maybe 16MB or even more) and initialize that as mostly free and return you a pointer to 16 bytes somewhere. A second call to malloc for another 16 bytes will just return you another 16 bytes from that pool -- no need to go back to the OS for more.

As your program goes ahead mallocing more memory it will just come from this pool, and free calls will return memory to the free pool. If it generally allocates more than it frees, eventually that free pool will run out, and at that point, malloc will call the system (mmap or sbrk) to get more memory to add to the free pool.

This is why if you monitor a process that is allocating and freeing memory with malloc/free with some sort of process monitor, you will generally just see the memory use go up (as the free pool runs out and more memory is requested from the system), and generally will not see it go down -- even though memory is being freed, it generally just goes back to the free pool and is not unmapped or returned to the system. There are some exceptions -- particularly if very large blocks are involved -- but generally you can't rely on any memory being returned to the system until the process exits.
 * */
void *malloc(uint32_t size) {
    if(size == 0) return NULL;
    // round up requested size to a multiple of ALIGNMENT 
    // calculate real size that's used, round it to multiple of 16
    // 为了对齐 是16的倍数给用户。
    // The MALLOCALIGN environment variable can be set to the default alignment desired for every malloc() allocation. An example is
    // MALLOCALIGN=16;  export MALLOCALIGN

    // The MALLOCALIGN environment variable can be set to any power of 2 value greater than or equal to the size of a pointer in the corresponding run mode (4 bytes for 32-bit mode, 8 bytes for 64-bit mode). For 32-bit vector-enabled programs, this environment variable can be set to 16, so all malloc()s will be suitably aligned for vector data types if necessary. Note that 64-bit vector programs will already receive 16-byte-aligned allocations.
    // Understanding the default allocation policy
    uint32_t roundedSize = ((size + 15)/16) * 16;                                  /// think twice how you round
    // OVERHEAD (sizeof(struct Block) + sizeof(unsigned int))
    // why add sizeof(unsigned int)?
    //  malloc overhead refers to the size of an internal malloc construct that is required for each block in the bucket. This internal construct is 8 bytes long for 32-bit applications and 16 bytes long for 64-bit applications. It is not part of the allocatable space available to the user, but is part of the total size of each bucket.
    // Allocated memory contains an 8- or 16-byte overhead for the size of the chunk and usage flags (similar to a dope vector). Unallocated chunks also store pointers to other free chunks in the usable space area, making the minimum chunk size 16 bytes on 32-bit systems and 24/32 (depends on alignment) bytes on 64-bit systems.
    uint32_t blockSize = roundedSize + OVERHEAD;
    // find bestfit in avl tree, note: this bestfit function will remove the best-fit node when there is more than one such node in tree.
    struct Block * best;
    best = bestfit(roundedSize);

    uint32_t * trailingSize = NULL;
    if(best) {
        // and! put a SIZE to the last four byte of the chunk
        // and! put a SIZE to the last four byte of the chunk
        // and! put a SIZE to the last four byte of the chunk
        void * ptr = (void*)best;
        // next block address
        void * saveNextBlock = getNextBlock(best);
        // 给的块大小
        uint32_t chunkSize = getRealSize(best->size) + OVERHEAD;
        // 多出来的量
        uint32_t rest = chunkSize - blockSize; // what's left
        
        uint32_t whichSize;
        // avoid integer underflow, equivalent to if(rest - OVERHEAD < 8)
        if(rest < 8 + OVERHEAD) whichSize = chunkSize;
        else whichSize = blockSize;
        best->size = whichSize - OVERHEAD;
        setFree(&(best->size), 0);
        void * base = ptr;
        // 把size给写到::最后
        // 写大小的位置
        trailingSize = ptr + whichSize - sizeof(uint32_t);
        // 写值 
        *trailingSize = best->size;
        ptr = (void*)(trailingSize + 1);

        if(rest < 8 + OVERHEAD) goto noSplit;
        // if size is enough, a) make it a separate memory chunk  b) merge it with the next block
        if(rest >= 8) {
            if(base != tail && isFree(saveNextBlock)) {
                // choice b)  merge!
                // gather info about next block
                void * nextblock = saveNextBlock;
                struct Block * n_nextblock = nextblock;
                // remove next from list because it no longer exists(just unlink it)
                removeNodeFromFreelist(n_nextblock);
                // merge!
                struct Block * t = ptr;
                t->size = rest - OVERHEAD + getRealSize(n_nextblock->size) + OVERHEAD;
                setFree(&(t->size), 1);
                ptr = ptr + sizeof(struct Block) + getRealSize(t->size);
                trailingSize = ptr;
                *trailingSize = t->size;

                if(nextblock == tail){
                    // I don't want to set it to tail now, instead, reclaim it
                    tail = t;
                    //int reclaimSize = getRealSize(t->size) + OVERHEAD;
                    //ksbrk(-reclaimSize);
                    //goto noSplit;
                }
                // then add merged one into the front of the list
                addNodeToFreelist(t);
            }
            else {
                // choice a)  seperate!
                struct Block * putThisBack = ptr;
                putThisBack->size = rest - OVERHEAD;
                setFree(&(putThisBack->size), 1);
                trailingSize = ptr + sizeof(struct Block) + getRealSize(putThisBack->size);
                *trailingSize = putThisBack->size;
                if(base == tail){
                    tail = putThisBack;
                    //int reclaimSize = getRealSize(putThisBack->size) + OVERHEAD;
                    //ksbrk(-reclaimSize);
                    //goto noSplit;
                }
                addNodeToFreelist(putThisBack);

            }
        }
noSplit:
        // return it!
        removeNodeFromFreelist(base);
        // 有效空间开始地址  
        return base + sizeof(struct Block);
    }
    else {
        // ------------------------------------
        // int brk(void *addr);
        // void *sbrk(intptr_t increment);
        // ------------------------------------

        // 
        //    The brk() and sbrk() functions are used to change the amount of space allocated for the calling process. The change is made by resetting the process' break value and allocating the appropriate amount of space. The amount of allocated space increases as the break value increases. The newly-allocated space is set to 0. However, if the application first decrements and then increments the break value, the contents of the reallocated space are unspecified.
        // The brk() function sets the break value to addr and changes the allocated space accordingly.
        // The sbrk() function adds incr bytes to the break value and changes the allocated space accordingly. If incr is negative, the amount of allocated space is decreased by incr bytes. The current value of the program break is returned by sbrk(0).
        // The behaviour of brk() and sbrk() is unspecified if an application also uses any other memory functions (such as malloc(), mmap(), free()). Other functions may use these other memory functions silently.
        // It is unspecified whether the pointer returned by sbrk() is aligned suitably for any purpose.
        // These interfaces need not be reentrant.
        // ==============================

        // :( no blocks fit my need!  use sbrk, initialize some meta data and return it!
        // wait! I can still optimize! if the tail block is freed, then I can sbrk less
        /*
           if(isFree(tail)) {
        // Calcullate how much memory to sbrk
        uint32_t needToSbrk = blockSize - getRealSize(tail->size) - OVERHEAD;
        ksbrk(needToSbrk);
        removeNodeFromFreelist(tail);
        // mark allocated
        tail->size = blockSize - OVERHEAD;
        setFree(&(tail->size), 0);
        trailingSize = (void*)tail + sizeof(struct Block) + getRealSize(tail->size);
         *trailingSize = tail->size;
         return tail + 1;
         }*/
        // 新开辟空间
        uint32_t realsize = blockSize;
        // reallocate heap memory.
        struct Block * ret = ksbrk(realsize);
        ASSERT(ret != NULL &&  "Heap is running out of space\n");
        if(!head) head = ret;
        void * ptr = ret;
        void * save = ret;
        tail = ptr;

        // after sbrk(), split the block into half [blockSize  | the rest], and put the rest into the tree.
        ret->size = blockSize - OVERHEAD;
        setFree(&(ret->size), 0);
        ptr = ptr + blockSize - sizeof(uint32_t);
        trailingSize = ptr;
        *trailingSize = ret->size;
        // now, return it!
        return save + sizeof(struct Block);
    }
}

/*
 * The real free function
 *
 * */
void free(void *ptr) {
    // front previous, current, behind
    struct Block * curr = ptr - sizeof(struct Block);
    struct Block * prev = getPrevBlock(curr);
    struct Block * next = getNextBlock(curr);

    if(isFree(prev) && isFree(next)) {
        prev->size = getRealSize(prev->size) + 2*OVERHEAD + getRealSize(curr->size) + getRealSize(next->size);
        setFree(&(prev->size), 1);
        // 最后设置下一个大小
        uint32_t * trailingSize = (void*)prev + sizeof(struct Block) + getRealSize(prev->size);
        *trailingSize = prev->size;
        // if next used to be tail, set prev = tail
        if(tail == next) tail = prev;
        removeNodeFromFreelist(next);
    }
    else if(isFree(prev)) {
        prev->size = getRealSize(prev->size) + OVERHEAD + getRealSize(curr->size);
        setFree(&(prev->size), 1);
        uint32_t * trailingSize = (void*)prev + sizeof(struct Block) + getRealSize(prev->size);
        *trailingSize = prev->size;
        if(tail == curr) tail = prev;
    }
    else if(isFree(next)) {
        // change size to curr's size + OVERHEAD + next's size
        curr->size = getRealSize(curr->size) + OVERHEAD + getRealSize(next->size);
        setFree(&(curr->size), 1);
        uint32_t * trailingSize = (void*)curr + sizeof(struct Block) + getRealSize(curr->size);
        *trailingSize = curr->size;
        if(tail == next) tail = curr;
        removeNodeFromFreelist(next);
        addNodeToFreelist(curr);
    }
    else {
        // just mark curr freed
        setFree(&(curr->size), 1);
        uint32_t * trailingSize = (void*)curr + sizeof(struct Block) + getRealSize(curr->size);
        *trailingSize = curr->size;
        addNodeToFreelist(curr);
    }
}

/*
 * The real realloc function
 *

* 
*/
void *realloc(void *ptr, uint32_t size) {
    uint32_t * trailingSize = NULL;
    if(!ptr) return malloc(size);
    // size is 0, release the space. 
    if(size == 0 && ptr != NULL) {
        free(ptr);
        return NULL;
    }

    // think twice how you round
    uint32_t roundedSize = ((size + 15)/16) * 16;                                  /// think twice how you round
    uint32_t blockSize = roundedSize + OVERHEAD;
    struct Block * nextBlock, * prevBlock;
    // shrink or expand ?
    // shrink:
    // now, we would just return the same address, later we may split this block
    // expand:
    // first, try if the actual size of the memory block is enough to to hold the current size
    // second, if not, try if merging the next block works
    // third, if none of the above works, malloc another block, move all the data there, and then free the original block
    struct Block * nptr = ptr - sizeof(struct Block);
    nextBlock = getNextBlock(nptr);
    prevBlock = getPrevBlock(nptr);
    if(nptr->size == size) return ptr;
    if(nptr->size < size) {
        // Expand, size of the block is just not enough
        if(tail != nptr && isFree(nextBlock) && (getRealSize(nptr->size) + OVERHEAD + getRealSize(nextBlock->size)) >= roundedSize) {
            // Merge with the next block, and return !
            // change size to curr's size + OVERHEAD + next's size
            removeNodeFromFreelist(nextBlock);
            nptr->size = getRealSize(nptr->size) + OVERHEAD + getRealSize(nextBlock->size);
            setFree(&(nptr->size), 0);
            trailingSize = (void*)nptr + sizeof(struct Block) + getRealSize(nptr->size);
            *trailingSize = nptr->size;
            if(tail == nextBlock) {
                // set it to tail for now, or we can reclaim it
                tail = nptr;
            }
            return nptr + 1;
        }
        // hey ! try merging with the previous block!
        else if(head != nptr && isFree(prevBlock) && (getRealSize(nptr->size) + OVERHEAD + getRealSize(prevBlock->size)) >= roundedSize) {
            //db_print();
            uint32_t originalSize = getRealSize(nptr->size);
            // hey! one more thing to do , copy data over to new block
            removeNodeFromFreelist(prevBlock);
            prevBlock->size = originalSize + OVERHEAD + getRealSize(prevBlock->size);
            setFree(&(prevBlock->size), 0);
            trailingSize = (void*)prevBlock + sizeof(struct Block) + getRealSize(prevBlock->size);
            *trailingSize = prevBlock->size;
            if(tail == nptr) {
                tail = prevBlock;
            }
            memcpy(prevBlock+1, ptr, originalSize);
            return prevBlock + 1;
        }
        // Move to somewhere else
        void * newplace = malloc(size);
        // Copy data over
        memcpy(newplace, ptr, getRealSize(nptr->size));
        // Free original one
        free(ptr);
        return newplace;
    }
    else {
        // Shrink/Do nothing, you can leave it as it's, but yeah... shrink it
        // What's left after shrinking the original block
        uint32_t rest = getRealSize(nptr->size) + OVERHEAD - blockSize;
        if(rest < 8 + OVERHEAD) return ptr;

        nptr->size = blockSize - OVERHEAD;
        setFree(&(nptr->size), 0);
        trailingSize = (void*)nptr + sizeof(struct Block) + getRealSize(nptr->size);
        *trailingSize = nptr->size;
        /*
           if(tail == nptr) {
           ksbrk(-reclaimSize);
           return ptr;
           }
           */
        struct Block * splitBlock = (void*)trailingSize + sizeof(uint32_t);

        // set the next, if the next of the next is also freed.. then merge!!
        // wait... what if after merge, I get a much much more bigger block than I even need? split again hahahahahah fuck....:*
        // instead of spliting after merge, let's give splitBlock
        if(nextBlock && isFree(nextBlock)) {
            splitBlock->size = rest + getRealSize(nextBlock->size);
            setFree(&(splitBlock->size), 1);
            trailingSize = (void*)splitBlock + sizeof(struct Block) + getRealSize(splitBlock->size);
            *trailingSize = splitBlock->size;

            // remove next block from freelist
            removeNodeFromFreelist(nextBlock);
            // This can be deleted when you correctly implemented malloc()
            if(tail == nextBlock) {
                tail = splitBlock;
            }
            // add splitblock to freelist
            addNodeToFreelist(splitBlock);

            return ptr;
        }
        // separate !
        splitBlock->size = rest - OVERHEAD;
        setFree(&(splitBlock->size), 1);
        trailingSize = (void*) splitBlock + sizeof(struct Block) + getRealSize(splitBlock->size);
        *trailingSize = splitBlock->size;
        // add this mo** f**r to the freelist!
        addNodeToFreelist(splitBlock);

        return ptr;
    }
}
