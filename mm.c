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

/* Global variables: */
static char *heap_listp; /* Pointer to first block */  

struct freeBlock{
		struct freeBlock *prev;
		struct freeBlock *current;
		struct freeBlock *next;		
		size_t size;
};
typedef volatile struct freeBlock *freeBlockp;

//static int *free_list[5];
//static int (*free_lis);
static struct freeBlock **freeList;





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
	//if ((free_list[0] = mem_sbrk(4 * WSIZE)) == (void *)-1){
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
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	void *bp;

	/* Ignore spurious requests. */
	if (size == 0)
		return (NULL);

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2 * DSIZE;
	else
		asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

	/* Search the free list for a fit. */

	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return (bp);
	}

	/* No fit found.  Get more memory and place the block. */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)  
		return (NULL);
		
	place(bp, asize);
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
	coalesce(bp);
	/*coalesced_address = coalesce(bp);
	place_in_free_list(coalesced_address);*/
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
	if (size < oldsize)
		oldsize = size;
	memcpy(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

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

	if (prev_alloc && next_alloc) {                 /* Case 1 */
		place_in_free_list(bp);
		return (bp);
	} else if (prev_alloc && !next_alloc) {         /* Case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		remove_from_free_list(NEXT_BLKP(bp));
		place_in_free_list(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	

	} else if (!prev_alloc && next_alloc) {         /* Case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		remove_from_free_list(NEXT_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		place_in_free_list(bp);
	} else {                                        /* Case 4 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
		    GET_SIZE(FTRP(NEXT_BLKP(bp)));
		remove_from_free_list(NEXT_BLKP(bp));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		place_in_free_list(bp);
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

	void *bp;

	/* Allocate an even number of words to maintain alignment. */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((bp = mem_sbrk(size)) == (void *)-1)  
		return (NULL);

	/* Initialize free block header/footer and the epilogue header. */
	PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
	PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	

	/* Coalesce if the previous block was free. */
	/*coalesced_address = coalesce(bp);
	place_in_free_list(coalesced_address);*/



	return (coalesce(bp));
}





/*
* Requires: pointer to free memory block
*
* Effects: Place block in appropriate location in freeList;
*/
static void 
place_in_free_list(void* bp) {

	struct freeBlock new_block;
	struct freeBlock list_head;
	size_t block_size;
	//struct freeBlock* list_head_pointer;
	//int* list_head_pointer;
	struct freeBlock *list_head_pointer;



	//block_size = NEXT_BLKP(bp) - bp;
	block_size = GET_SIZE(bp);
	new_block.current = bp;
	new_block.size = block_size;


	switch(block_size)
	{
		case 1:
		case 2:

			list_head_pointer = freeList[0] ;
			//list_head  =  (void *)list_head_pointer;
			if(list_head_pointer == NULL) {
			    new_block.prev = new_block.current;
			    new_block.next = new_block.current;
			} else {
			    new_block.prev = list_head.prev;
			    list_head_pointer->prev->next = new_block.current;
                list_head.prev = new_block.current;
                new_block.next = list_head_pointer;
			}
			freeList[0] = &new_block;
			break;

		case 3:
			list_head_pointer = freeList[1];
			list_head  =  *list_head_pointer;
			list_head.prev = new_block.current;
			new_block.next = list_head_pointer;
			freeList[1] = new_block.current;

			break;

		case  4:
			list_head_pointer = freeList[2];
			list_head  =  *list_head_pointer;
			list_head.prev = new_block.current;
			new_block.next = list_head_pointer;
			freeList[2] = new_block.current;
			break;

		case 5:
		case 6:
		case 7:
		case 8:
			list_head_pointer = freeList[3];
			list_head  =  *list_head_pointer;
			list_head.prev = new_block.current;
			new_block.next = list_head_pointer;
			freeList[3] = new_block.current;
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			list_head_pointer = freeList[4];
			list_head  =  *list_head_pointer;
			list_head.prev = new_block.current;
			new_block.next = list_head_pointer;
			freeList[4] = new_block.current;

			break;
		default:
			printf("\nInvalid size.\n");

			break;

	}

}

/*
* Requires: pointer to free memory block
*
* Effects: Remove recently allocated block in appropriate location in freeList;
*/
static void 
remove_from_free_list(void* bp) {

	//struct freeBlock *old_block;
	struct freeBlock *curr_block;
	int block_size = NEXT_BLKP(bp) - bp;

	switch(block_size)
	{
		case 1:
		case 2:
			curr_block = freeList[0];
			while(curr_block != NULL) {
				if (curr_block->current == bp){
					curr_block->prev->next = curr_block->next;
					curr_block->next->prev = curr_block->prev;
					break;
				}
				curr_block = curr_block->next;
			}
			break;

		case 3:
			curr_block = freeList[1];
			while(curr_block != NULL) {
				if (curr_block->current == bp){
					curr_block->prev->next = curr_block->next;
					curr_block->next->prev = curr_block->prev;
					break;
				}
				curr_block = curr_block->next;
			}
			break;

		case  4:
			curr_block = freeList[2];
			while(curr_block != NULL) {
				if (curr_block->current == bp){
					curr_block->prev->next = curr_block->next;
					curr_block->next->prev = curr_block->prev;
					break;
				}
				curr_block = curr_block->next;
			}
			
			break;

		case 5:
		case 6:
		case 7:
		case 8:
			curr_block = freeList[3];
			while(curr_block != NULL) {
				if (curr_block->current == bp){
					curr_block->prev->next = curr_block->next;
					curr_block->next->prev = curr_block->prev;
					break;
				}
				curr_block = curr_block->next;
			}
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			curr_block = freeList[4];
			while(curr_block != NULL) {
				if (curr_block->current == bp){
					curr_block->prev->next = curr_block->next;
					curr_block->next->prev = curr_block->prev;
					break;
				}
				curr_block = curr_block->next;
			}
			break;
		default:
			printf("\nInvalid size for removal from list.\n");

			break;

	}
}	

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Find a fit for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found. 
 */
static void *
find_fit(size_t asize)
{
	//PPRRROOBBBSS SHOULDNT BE COMMENTED OUT
	//void *bp;
	void *new_mem_location; 
	//?? OR
	//char *new_mem_location; 
	//struct freeBlock a_block;
	struct freeBlock *curr_block;

	switch(asize)
	{
		case 1:
		case 2:
			 //this seems incorrect...curr__block-> should point to bp... and freeList[0] should =curr_block
			curr_block = freeList[0];
			
			if (curr_block == NULL & curr_block->size >= asize) {
				/* request new page of memory and add to free list for size range*/
				freeList[0] += (2 * WSIZE);

				/* Extend the empty heap with a free block of CHUNKSIZE bytes. */
				if ((new_mem_location = extend_heap(CHUNKSIZE/2)) == NULL){
					return (-1);
				}
				freeList[0] = new_mem_location;
				//64/2...32


				break;

				/* etc...etc.. break, need to check check freeList[0] to equal that thing*/


			}
			while(curr_block != NULL) {
				if (curr_block->size >= asize){

					return curr_block->current;
					break;
				}
				curr_block = curr_block->next;
			}
			break;

		case 3:
			curr_block = freeList[1];
			while(curr_block != NULL) {
				if (curr_block->size >= asize){

					return curr_block->current;
					break;
				}
				curr_block = curr_block->next;
			}
			
			break;

		case  4:
			curr_block = freeList[2];
			while(curr_block != NULL) {
				if (curr_block->size >= asize){

					return curr_block->current;
					break;
				}
				curr_block = curr_block->next;
			}
			
			break;

		case 5:
		case 6:
		case 7:
		case 8:
			curr_block = freeList[3];
			while(curr_block != NULL) {
				if (curr_block->size >= asize){

					return curr_block->current;
					break;
				}
				curr_block = curr_block->next;
			}
			
			break;
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			curr_block = freeList[4];
			while(curr_block != NULL) {
				if (curr_block->size >= asize){

					return curr_block->current;
					break;
				}
				curr_block = curr_block->next;
			}
			
			break;
		default:
			printf("\nInvalid size for removal from list.\n");

			break;

	}
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

	if ((csize - asize) >= (2 * DSIZE)) { 
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		remove_from_free_list(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		place_in_free_list(bp);
	} else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
		remove_from_free_list(bp);
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
