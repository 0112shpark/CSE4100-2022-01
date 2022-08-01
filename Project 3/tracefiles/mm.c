/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * 
 * segregate-list로 구현한 mm_malloc이다. seg_list가 list_p로 관리되며 
 * free block 할당과 해제에 관한 insert_, delete_함수가 있다. coalesce함수로 
 * 앞뒤 fre block들을 합쳐준다. best-fit search로 memory utilization이 높다.
 * mm_free의 mm_check를 주석 해제하고 실행하면 heap checking을 실행한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>


#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20181632",
    /* Your full name*/
    "Seonghyeon Park",
    /* Your email address */
    "000112shpark@naver.com",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Basic constantsand macros*/
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp)+GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE(((char*)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE(((char*)(bp)-DSIZE)))

/* Given block ptr, read next or previous block*/
#define PREV_BLK_RD(bp) (GET(bp))
#define NEXT_BLK_RD(bp) (GET((char*)(bp)+WSIZE))

/* Given block ptr, write next or previous block*/
#define PREV_BLK_WT(bp, ptr) (PUT(bp, ptr))
#define NEXT_BLK_WT(bp, ptr) (PUT((char*)(bp)+WSIZE, ptr))

static char* heap_listp;
size_t seg_lists = 12;
static char ** list_p;

static int get_index(size_t size);
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void place(void* bp, size_t asize, int remove);
static void* find_fit(size_t asize);
static void delete_(void* bp);
static void insert_(void* bp);
static int mm_check(void);



/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{ 
    /*Create the inital seg list*/
    if ((list_p = mem_sbrk(seg_lists * WSIZE)) == ((void*)-1))
        return -1;

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == ((void*)-1))
        return -1;
   
    
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2 * WSIZE);

    for (int i = 0; i < seg_lists; i++) { /* Create empty seg list */
        list_p[i] = NULL;
    }

    return 0;
}


static void* extend_heap(size_t words)
{
    void* bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
   
    if ((long)(bp = mem_sbrk(size)) == - 1)
        return NULL;
     
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
        
    return (bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void* bp;
 
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)//최소
        asize = 2 * DSIZE;
    else
    {
        asize = DSIZE * ((size + (DSIZE)+(DSIZE - 1)) / DSIZE);
    }
        

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize, 1);//remove from list and store splited bolck
       //mm_check();
        return bp;

    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize, 0);//store splited block
    //mm_check();
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    //mm_check();
}
static int get_index(size_t size) { //return index of the seg list based on size 2^n
    
   //printf("size:%d\n", size);
    if (size < 16)
        return 1;
    else if (size < 64)
        return 2;
    else if (size < 128)
        return 3;
    else if (size < 512)
        return 4;
    else if (size < 2048)
        return 5;
    else if (size < 8192)
        return 6;
    else if (size < 65536)
        return 7;
    else if (size < 131072)
        return 8;
    else if (size < 262114)
        return 9;
    else if (size < 524228)
        return 10;
    else if (size < 1048576)
        return 11;
    else
        return 0;

}
static void delete_(void* bp)
{
    int index = get_index(GET_SIZE(HDRP(bp)));

    if (PREV_BLK_RD(bp))//previous node exists
    {
        if (NEXT_BLK_RD(bp))//next block exists
        {
            NEXT_BLK_WT(PREV_BLK_RD(bp), NEXT_BLK_RD(bp));
        }
        else//last block
        {
            NEXT_BLK_WT(PREV_BLK_RD(bp), NULL);
        }
    }
    else //if not, it is first block
    {
        list_p[index] = NEXT_BLK_RD(bp);
    }

    if (NEXT_BLK_RD(bp))//next block exists
    {
        if (PREV_BLK_RD(bp)) { //if previous block exists
            PREV_BLK_WT(NEXT_BLK_RD(bp), PREV_BLK_RD(bp));
        }
        else //first block
        {
            PREV_BLK_WT(NEXT_BLK_RD(bp), NULL);
        }
       
    }
    return;
}
/*insert free blocks into free seg-list*/
static void insert_(void* bp)
{
    int index = get_index(GET_SIZE(HDRP(bp)));
    NEXT_BLK_WT(bp, list_p[index]);//place block in the beginning
    PREV_BLK_WT(bp, NULL);

    if (list_p[index] != NULL)//if block exists
    {
        PREV_BLK_WT(list_p[index], bp);
    }
    //printf("%d\n", index);
    list_p[index] = bp;
    return;
}
/* coalesce free blocks with front and back blocks*/
static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))) ;
    size_t size = GET_SIZE(HDRP(bp));
    void* next;
    void* prev;

    if (prev_alloc && next_alloc) { /* Case 1- when there are no free blocks nearby */
        insert_(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2- when next block is free block */
        next = NEXT_BLKP(bp);
        delete_(next); // remove from free list
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_(bp);//insert coalesced free block
    }

    else if (!prev_alloc && next_alloc) { /* Case 3- when previous block is free block */
        prev = PREV_BLKP(bp);
        delete_(prev);
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_(bp);
    }

    else { /* Case 4- when both of front and back blocks are free*/
        next = NEXT_BLKP(bp);
        prev = PREV_BLKP(bp);
        delete_(prev);
        delete_(next);
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        insert_(bp);

    }
    return bp;
}
static void* find_fit(size_t asize) { //best fit 
    
    void* bp;
    void* best=NULL;
    size_t size;
    size_t temp;
    size_t least_size= 1000000;

    int index = get_index(asize);
    for (int i = index; i < seg_lists; i++)
    {
        bp = list_p[i];//start searching from the beginning
        while (bp) {
            size = GET_SIZE(HDRP(bp));
            if ((asize <= size)) {
                temp = size;
                if (temp < least_size) {
                    least_size = temp;
                    best = bp;
                }
            }
            bp = NEXT_BLK_RD(bp);
        }
        if (best != NULL) { //found from first list
            return best;
        }
    }
    /*NO FIT*/
    return best;
}
/*Using given ptr and size, split block with given size, and insert left empty block to free list*/
static void place(void* bp, size_t asize, int remove) { 
 /* remove is used when we need to get free block from list.
    remove is not used when we extend the heap*/

    size_t csize = GET_SIZE(HDRP(bp));
    void* split;
    if (remove) {
        //remove from free list, and add splited block and store again.
        if ((csize - asize) >= (2 * DSIZE)) {//enough space left
            delete_(bp);
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            split = NEXT_BLKP(bp);
            PUT(HDRP(split), PACK(csize - asize, 0));
            PUT(FTRP(split), PACK(csize - asize, 0));
            insert_(split);
        }
        else {
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
            delete_(bp);
        }
    }
    else {//do not remove, just add to the list- extend_heap
        if ((csize - asize) >= (2 * DSIZE)) {//enough space left
            
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            split = NEXT_BLKP(bp);
            PUT(HDRP(split), PACK(csize - asize, 0));
            PUT(FTRP(split), PACK(csize - asize, 0));
            insert_(split);
        }
        else {
            PUT(HDRP(bp), PACK(csize, 1));
            PUT(FTRP(bp), PACK(csize, 1));
            
        }
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{ 
    void* newptr;
    void* oldptr = ptr;
    void* nextptr;
    size_t copySize = GET_SIZE(HDRP(oldptr));//ptr size
    size_t regsize = DSIZE * ((size + (DSIZE)+(DSIZE - 1)) / DSIZE);//size to realloc
    size_t next_size;
   

    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }
    
    if (regsize <= copySize)//realloc smaller size
    {
        return ptr;
    }
    else //realloc bigger size
    {
        nextptr = NEXT_BLKP(oldptr);//points next block
        next_size = GET_SIZE(HDRP(nextptr));//next block size
        size_t added_size = copySize + next_size;
        if (added_size >= regsize && !GET_ALLOC(HDRP(nextptr))) //check if next block is free
        {
            delete_(nextptr);
           
              PUT(HDRP(oldptr), PACK(added_size, 1));
              PUT(FTRP(oldptr), PACK(added_size, 1));

               return oldptr;
            

        }
    }


    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
        return mm_malloc(size);

   
   
    newptr = mm_malloc(size);
    /* If realloc() fails the original block is left untouched  */
    if (newptr==NULL)
        return NULL;

    /* Copy the old data. */
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);

    /* Free the old block. */
    mm_free(ptr);

    return newptr;
}
/* check heap consistency */
static int mm_check(void)
{
    int error = 0;
    void* blkptr;
    
    /*check if all free blocks in seg-list marked as free*/
    for (int i = 0; i < seg_lists; i++)
    {
        blkptr = list_p[i];
        while (blkptr)
        {
            if (GET_ALLOC(blkptr))
            {
                printf("Error: Free blocks marked not free!\n");
                error = -1;
            }
            blkptr = NEXT_BLK_RD(blkptr);
        }
        
    }

    /*check blocks*/
    for (blkptr = heap_listp; GET_SIZE(HDRP(blkptr)) > 0; blkptr = NEXT_BLKP(blkptr))
    {
        if (GET(HDRP(blkptr)) != GET(FTRP(blkptr)))
        {
            /* check block's header and footer information*/
            printf("Error: Block's header and footer info does not match!\n");
                error = -1;
        }
        if ((!GET_ALLOC(HDRP(blkptr))) && (!GET_ALLOC(HDRP(NEXT_BLKP(blkptr)))))
        {   /*check if two consecutive free blocks escape from coalescing */
            printf("Error: Two consecutive blocks are free!\n");
                error = -1;
        }
    }
    
    return error;
}



