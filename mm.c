/* 
 * Simple, 32-bit and 64-bit clean allocator based on an implicit free list,
 * first fit placement, and boundary tag coalescing, as described in the
 * CS:APP3e text.  Blocks are aligned to double-word boundaries.  This
 * yields 8-byte aligned blocks on a 32-bit processor, and 16-byte aligned
 * blocks on a 64-bit processor.  However, 16-byte alignment is stricter
 * than necessary; the assignment only requires 8-byte alignment.  The
 * minimum block size is four words.
 *
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	/* Team name */
	"Team Smarties", 
	"Jolisa Brown",
	
	"jmb26",
	
	"Vidisha Ganesh",
	
	"vg19",
	
	
};

/* Basic constants and macros: */
#define WSIZE      sizeof(void *) /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 12)      /* Extend heap by this amount (bytes) */

#define MAX(x, y)  ((x) > (y) ? (x) : (y))  

/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p. */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer. */
#define HDRP(bp)  ((char *)(bp) - WSIZE)
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks. */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/*Given a size, return pointer to index for given size range in freeList */
#define GET_INDEX(size) ((size) == (0) ? (-1) : (size) < (3) ? (0) : (size) < (5) ? (1) : (size) < (9) ? (2) : (size) < (17) ? (3) : (size) < (33) ? (4) : (size) < (65) ? (5) : (size) < (129) ? (6) : (size) < (257) ? (7) : (size) < (513) ? (8) : (size) < (1025) ? (9) : (size) < (2049) ? (10) : (size) < (4096) ? (11) : (size) < (8192) ? (12) : (size) < (16385) ? (13) : (size) < (32768) ? (14) : (size) < (65536) ? (15) : (size) < (131073) ? (16) : (size) < (262145) ? (17) : (size) < (524289) ? (18) : (size) < (1048577) ? (19) : (size) < (2097153) ? (20) : (size) < (4194305) ? (21) : (size) < (8388609) ? (22) : (size) < (16777217) ? (23) : (size) < (33554433) ? (24) : (size) < (67108865) ? (25) : (26))
//#define GET_INDEX(size) ceil(log(size)/log(2));
//#define GET_INDEX(size) ceil((float)size/100.0);
/* Global variables: */
static char *heap_listp; /* Pointer to first block */  

struct freeBlock{
		struct freeBlock *prev;
		struct freeBlock *next;		
		
};


//static int *free_list[5];
//static int (*free_lis);
//static struct freeBlock **freeList;
//do we point? to the pointer list?
//static int **ptr_list;

//we're going to initialize a struct that contains pointers to the next and prev structs


struct freeBlock *array_heads;

int free_list_size = 26;



/* Function prototypes for internal helper routines: */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void place_in_free_list(void* bp);

/* Function prototypes for heap consistency checker routines: */
static void checkblock(void *bp);
static void checkheap(bool verbose);
static void printblock(void *bp);
//static void print_free_list();
static void remove_from_free_list(void* bp);


/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
// int
// mm_init(void) 
// {

// 	/* Create the initial empty heap. */
// 	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
// 		return (-1);
// 	PUT(heap_listp, 0);                            /* Alignment padding */
// 	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
// 	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
// 	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
// 	heap_listp += (2 * WSIZE);

// 	/* Extend the empty heap with a free block of CHUNKSIZE bytes. */
// 	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
// 		return (-1);
// 	return (0);
// }

int
mm_init(void) 
{

	/* Create the initial empty heap. */
	//leave some space for the array of dummy headers
	//technically the space needed is (2 X WSIZE) for each of the freeblock heads
	if ((array_heads = mem_sbrk(free_list_size * sizeof(struct freeBlock))) == (void *)-1) {
	    return (-1);
	}
	//create the dummy heads array -> each head points to itself
	int i;
    for(i = 0; i < free_list_size; i++) {
        array_heads[i].prev = &array_heads[i];
        array_heads[i].next = &array_heads[i];
    }


	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1){
		return (-1);
	}
	PUT(heap_listp, 0);                            /* Alignment padding */
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
	/*  We believe this is the pointer to the free memory address??*/
	heap_listp += (2 * WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes. */
	if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
		return (-1);

	return (0);
}
/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *
mm_malloc(size_t size) 
{
//    printf("Starting to malloc a block of size: %d\n", (int) size);
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	void *bp;

//	printf("The size we want to malloc is: %d\n", (int)size);

	/* Ignore spurious requests. */
	if (size == 0)
		return (NULL);

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
	    asize = 2 * DSIZE;
	//trying out 4
//		asize = 2 * DSIZE;

	else {
	    asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
//	    printf("Adjusted input size to asize: %d\n", (int) asize);

	}

	/* Search the free list for a fit. */

	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
//		printf("finished malloc-ing with a block of adjusted size: %d\n", (int) asize);
//		print_free_list();
//		printf("This is the block returned to be malloc-ed: %p\n", bp);
		return (bp);
	}

	/* No fit found.  Get more memory and place the block. */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)  
		return (NULL);
		
	place(bp, asize);
//	printf("finished malloc-ing a block of adjusted size: %d\n", (int) asize);
//	print_free_list();
//	printf("This is the block returned to be malloc-ed: %p\n", bp);

	return (bp);
} 

/* 
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */
void
mm_free(void *bp)
{
	size_t size;

	/* Ignore spurious requests. */
	if (bp == NULL)
		return;

	/* Free and coalesce the block. */
	size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	//place_in_free_list(bp);
	coalesce(bp);

}

/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */
void *
mm_realloc(void *ptr, size_t size)
{

//    printf("Realloc happening\n");
	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		mm_free(ptr);
		return (NULL);
	}

	/* If oldptr is NULL, then this is just malloc. */
	if (ptr == NULL)
		return (mm_malloc(size));

	newptr = mm_malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if (newptr == NULL)
		return (NULL);

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(ptr));
	if (size < oldsize) {
		//oldsize = size;
		return ptr;
	}
		
		//oldsize = size;
	//check whether free block to the left is large enough
	if ((!GET_ALLOC(FTRP(PREV_BLKP(ptr)))) && (GET_SIZE(HDRP(PREV_BLKP(ptr))) > size)) {
		return PREV_BLKP(ptr);
	}
	//check whether free block to the right is large enough
	if ((!GET_ALLOC(FTRP(NEXT_BLKP(ptr)))) && (GET_SIZE(HDRP(NEXT_BLKP(ptr))) > size)) {
		return NEXT_BLKP(ptr);
	}
	oldsize = size;
	memcpy(newptr, ptr, oldsize);
	//memmove(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

//	printf("finished realloc");

	return (newptr);
}

/*
 * The following routines are internal helper routines.
 */

/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *
coalesce(void *bp) 
{
	size_t size = GET_SIZE(HDRP(bp));
	bool prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	bool next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	//printf("\nThis is the initial size of the new block in coalesce %d \n", bp);

	if (prev_alloc && next_alloc) {                 /* Case 1 - no coalescing*/
//		printf("\n1: This is the new size of pointer in coalesce %d\n", (int) GET_SIZE(HDRP(bp)));
		place_in_free_list((bp));

		return (bp);
	} else if (prev_alloc && !next_alloc) {         /* Case 2 - coalesce with next block (on right) */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		remove_from_free_list(NEXT_BLKP(bp));
		//should we leave this as it was here or keep the change to move it down??
		//place_in_free_list(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
//		printf("\n2: This is the new size of pointer in coalesce %d\n", (int) GET_SIZE(HDRP(bp)));
		place_in_free_list((bp));
	

	} else if (!prev_alloc && next_alloc) {         /* Case 3 - coalesce with previous block (on left) */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		remove_from_free_list(PREV_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
//		printf("\n3: This is the new size of pointer in coalesce %d\n", (int) GET_SIZE(HDRP(bp)));
		place_in_free_list((bp));
	} else {                                        /* Case 4 - coalesce with both prev and next blocks */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
		    GET_SIZE(FTRP(NEXT_BLKP(bp)));
		remove_from_free_list(NEXT_BLKP(bp));
		remove_from_free_list(PREV_BLKP(bp));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
//		printf("\n4: This is the new size of pointer in coalesce %d\n", (int) GET_SIZE(HDRP(bp)));
		place_in_free_list((bp));
	}
	return (bp);
}




/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void *
extend_heap(size_t words) 
{
	size_t size;
//	printf("\nThis is the number of words of sz 8 passed into extend_heap %d\n", (int) words);

	void *bp;

	/* Allocate an even number of words to maintain alignment. */
	//do we need to make changes to our freeList here?? are multiple blocks created here?
	//Check whether 2 needs to be 4
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((bp = mem_sbrk(size)) == (void *)-1)  
		return (NULL);

	/* Initialize free block header/footer and the epilogue header. */
	PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
	PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	return (coalesce(bp));
}





/*
* Requires: pointer to free memory block
*
* Effects: Place block in appropriate location in freeList;
*/
static void 
place_in_free_list(void* bp) {

//    printf("Starting place in free list\n");

    struct freeBlock *dummy_head;
    size_t block_size;
    block_size = GET_SIZE(HDRP(bp));
	int index = GET_INDEX(block_size);
	struct freeBlock *new_block = bp;

	dummy_head = &array_heads[index];
	new_block->prev = dummy_head;
	new_block->next = dummy_head->next;
	dummy_head->next->prev = new_block;
	dummy_head->next = new_block;

//	printf("Finished placing into free list with size: %d and next address is: %p\n", (int)block_size, new_block);

}//place in free list

/*
* Requires: pointer to free memory block
*
* Effects: Remove recently allocated block in appropriate location in freeList;
*/
static void 
remove_from_free_list(void* bp) {

//    printf("starting remove from free list\n");

	struct freeBlock *current = bp;
	current->prev->next = current->next;
    current->next->prev = current->prev;	


	/*int block_size = GET_SIZE(HDRP(bp));
	int index = GET_INDEX(block_size);
    struct freeBlock *dummy_head = &array_heads[index];
    struct freeBlock * current = dummy_head->next;

        while(current != dummy_head) {
            if(current == bp) { //if we find the block to delete
                current->prev->next = current->next;
                current->next->prev = current->prev;
                break;
            }
            current = current->next;
        }*/
//            printf("Finished removing from free list with size: %d\n", (int)block_size);
}

/*
 * Requires:
 *   None.
 * Effects:
 *   Find a fit for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found. 
 */
static void *
find_fit(size_t asize)
{
//    printf("Starting found fit func on size: %d\n", (int) asize);
//    print_free_list();

	struct freeBlock *current;
	int first_index = GET_INDEX(asize);
	//printf("The index we are starting at is: %d\n", first_index);
	struct freeBlock *dummy_head;
//	void *new_mem_location;

	/* find appropriate size range beginning at smallest possible fit, repopulate size range if neccesary*/
	for (int index = first_index ; index < free_list_size; index++) {
        dummy_head = &array_heads[index];
        current = dummy_head->next;
		/* if memory is available in size range iterate to find block large enough*/
		while(current != dummy_head) {
		    //WANTED TO TRY GET_SIZE(HDRP(current)), but that didn't work
			if (GET_SIZE(HDRP(current)) >= asize){
//			    printf("found fit! of size: %d\n", (int) GET_SIZE(HDRP(current)));
				return current;
			}
			current = current->next;
		}

	}

//	printf("could not find fit, need to extend heap\n");

	return NULL;


//
//    /* if no memory of appropriate size range available create memory and take first block*/
//    if ((new_mem_location = extend_heap(CHUNKSIZE/WSIZE)) == NULL){
//        return NULL;
//    }
//    printf("Finished finding fit and had to extend the heap\n");
//
//    return new_mem_location;



}

/* 
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size. 
 */
static void
place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
//	printf("We're about to PLACE, and the block we're placing into has size: %d\n", (int) csize);

	if ((csize - asize) >= (4 * DSIZE)) {
//	    printf("\nPLACE FUNC - fragmenting block\n");
//	    print_free_list();
	    remove_from_free_list(bp);
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		place_in_free_list(bp);
//		print_free_list();
	} else {
//	    printf("\nPLACE FUNC - not fragmenting block\n");
//	    print_free_list();
	    remove_from_free_list(bp);
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
//		print_free_list();
	}
}

/* 
 * The remaining routines are heap consistency checker routines. 
 */

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Perform a minimal check on the block "bp".
 */
static void
checkblock(void *bp) 
{

	if ((uintptr_t)bp % DSIZE)
		printf("Error: %p is not doubleword aligned\n", bp);
	if (GET(HDRP(bp)) != GET(FTRP(bp)))
		printf("Error: header does not match footer\n");
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency. 
 */
void
checkheap(bool verbose) 
{
    //modify it to go through the lists and check every block in the list to check that
    //header and footer match, and check that the "end" points back to the "dummy-head"
	void *bp;

	if (verbose)
		printf("Heap (%p):\n", heap_listp);

	if (GET_SIZE(HDRP(heap_listp)) != DSIZE ||
	    !GET_ALLOC(HDRP(heap_listp)))
		printf("Bad prologue header\n");
	checkblock(heap_listp);

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose)
			printblock(bp);
		checkblock(bp);
	}

	if (verbose)
		printblock(bp);
	if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp)))
		printf("Bad epilogue header\n");
}

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Print the block "bp".
 */
static void
printblock(void *bp) 
{
	size_t hsize, fsize;
	bool halloc, falloc;

	checkheap(false);
	hsize = GET_SIZE(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));  
	fsize = GET_SIZE(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));  

	if (hsize == 0) {
		printf("%p: end of heap\n", bp);
		return;
	}

	printf("%p: header: [%zu:%c] footer: [%zu:%c]\n", bp, 
	    hsize, (halloc ? 'a' : 'f'), 
	    fsize, (falloc ? 'a' : 'f'));
}

//static void
//print_free_list()
//{
//    printf("\n\nAbout to print out the free list\n");
//    struct freeBlock *dummy_head;
//    struct freeBlock *current;
//    int i;
//    for (i = 0; i < 10; i++) {
//        dummy_head = &array_heads[i];
//        current = dummy_head->next;
//        while(current != dummy_head) {
//            printf("\nThe size of free block is: %d, and the location is: %p\n", (int) GET_SIZE(HDRP(current)), current);
//            current = current->next;
//        }
//    }
//    printf("finished printing free list\n\n");
//}
