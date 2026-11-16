/* =============================================================================
 *
 * router.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <stdlib.h>
#include "coordinate.h"
#include "grid.h"
#include "queue.h"
#include "router.h"
#include "tm.h"
#include "vector.h"

#ifdef STD_QUEUE
#include <queue>
using Queue_t = std::queue<uintptr_t>;

Queue_t* Queue_alloc(int n) {
    return new Queue_t;
}

void Queue_free(Queue_t* q) {
    delete q;
}

void Queue_push(Queue_t* q, void* data) {
    q->push((uintptr_t)data);
}

void* Queue_pop(Queue_t* q) {
    uintptr_t res = q->front(); q->pop();
    return (void*)res;
}

bool_t Queue_isEmpty(Queue_t* q) {
    return (q->empty() ? TRUE : FALSE);
}

void Queue_clear(Queue_t* q) {
    std::queue<uintptr_t> empty;
    std::swap(*q, empty);
}
#endif
typedef enum momentum {
    MOMENTUM_ZERO = 0,
    MOMENTUM_POSX = 1,
    MOMENTUM_POSY = 2,
    MOMENTUM_POSZ = 3,
    MOMENTUM_NEGX = 4,
    MOMENTUM_NEGY = 5,
    MOMENTUM_NEGZ = 6
} momentum_t;

typedef struct point {
    long x;
    long y;
    long z;
    long value;
    momentum_t momentum;
} point_t;

point_t MOVE_POSX = { 1,  0,  0,  0, MOMENTUM_POSX};
point_t MOVE_POSY = { 0,  1,  0,  0, MOMENTUM_POSY};
point_t MOVE_POSZ = { 0,  0,  1,  0, MOMENTUM_POSZ};
point_t MOVE_NEGX = {-1,  0,  0,  0, MOMENTUM_NEGX};
point_t MOVE_NEGY = { 0, -1,  0,  0, MOMENTUM_NEGY};
point_t MOVE_NEGZ = { 0,  0, -1,  0, MOMENTUM_NEGZ};

#ifdef FINE_LOCK
extern LOCK_T* grid_lock;
extern LOCK_T queue_lock;
extern LOCK_T list_lock;
#ifdef WORKSTEAL
extern LOCK_T* queues_lock;
#endif
#endif

/* =============================================================================
 * router_alloc
 * =============================================================================
 */
router_t*
router_alloc (long xCost, long yCost, long zCost, long bendCost)
{
    router_t* routerPtr;

    routerPtr = (router_t*)P_MALLOC(sizeof(router_t));
    if (routerPtr) {
        routerPtr->xCost = xCost;
        routerPtr->yCost = yCost;
        routerPtr->zCost = zCost;
        routerPtr->bendCost = bendCost;
    }

    return routerPtr;
}


/* =============================================================================
 * router_free
 * =============================================================================
 */
void
router_free (router_t* routerPtr)
{
    P_FREE(routerPtr);
}


/* =============================================================================
 * PexpandToNeighbor
 * =============================================================================
 */
static void
#ifdef STD_QUEUE
PexpandToNeighbor (grid_t* myGridPtr,
                   long x, long y, long z, long value, Queue_t* queuePtr)
#else
PexpandToNeighbor (grid_t* myGridPtr,
                   long x, long y, long z, long value, queue_t* queuePtr)
#endif
{
    if (grid_isPointValid(myGridPtr, x, y, z)) {
        long* neighborGridPointPtr = grid_getPointRef(myGridPtr, x, y, z);
        long neighborValue = *neighborGridPointPtr;
        if (neighborValue == GRID_POINT_EMPTY) {
            (*neighborGridPointPtr) = value;
#ifdef STD_QUEUE
            Queue_push(queuePtr, (void*)neighborGridPointPtr);
#else
            PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
#endif
        } else if (neighborValue != GRID_POINT_FULL) {
            /* We have expanded here before... is this new path better? */
            if (value < neighborValue) {
                (*neighborGridPointPtr) = value;
#ifdef STD_QUEUE
                Queue_push(queuePtr, (void*)neighborGridPointPtr);
#else
                PQUEUE_PUSH(queuePtr, (void*)neighborGridPointPtr);
#endif
            }
        }
    }
}

#ifdef FINE_LOCK
/* =============================================================================
 * compareGridPoints
 * =============================================================================
 */
int compareGridPoints(const void * n1, const void * n2)
{
	if (*(long *)n1 > *(long *)n2)
	{
		return 1;
	}
	else if (*(long *)n1 < *(long *)n2)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
#endif
/* =============================================================================
 * PdoExpansion
 * =============================================================================
 */
static bool_t
#ifdef STD_QUEUE
PdoExpansion (router_t* routerPtr, grid_t* myGridPtr, Queue_t* queuePtr,
              coordinate_t* srcPtr, coordinate_t* dstPtr)
#else
PdoExpansion (router_t* routerPtr, grid_t* myGridPtr, queue_t* queuePtr,
              coordinate_t* srcPtr, coordinate_t* dstPtr)
#endif
{
    long xCost = routerPtr->xCost;
    long yCost = routerPtr->yCost;
    long zCost = routerPtr->zCost;

    /*
     * Potential Optimization: Make 'src' the one closest to edge.
     * This will likely decrease the area of the emitted wave.
     */

#ifdef STD_QUEUE
    Queue_clear(queuePtr);
#else
    PQUEUE_CLEAR(queuePtr);
#endif
    long* srcGridPointPtr =
        grid_getPointRef(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z);
#ifdef STD_QUEUE
    Queue_push(queuePtr, (void*)srcGridPointPtr);
#else
    PQUEUE_PUSH(queuePtr, (void*)srcGridPointPtr);
#endif
    grid_setPoint(myGridPtr, srcPtr->x, srcPtr->y, srcPtr->z, 0);
    grid_setPoint(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z, GRID_POINT_EMPTY);
    long* dstGridPointPtr =
        grid_getPointRef(myGridPtr, dstPtr->x, dstPtr->y, dstPtr->z);
    bool_t isPathFound = FALSE;

#ifdef STD_QUEUE
    while (!Queue_isEmpty(queuePtr)) {
#else
    while (!PQUEUE_ISEMPTY(queuePtr)) {
#endif

#ifdef STD_QUEUE
        long* gridPointPtr = (long*)Queue_pop(queuePtr);
#else
        long* gridPointPtr = (long*)PQUEUE_POP(queuePtr);
#endif
        if (gridPointPtr == dstGridPointPtr) {
            isPathFound = TRUE;
            break;
        }

        long x;
        long y;
        long z;
        grid_getPointIndices(myGridPtr, gridPointPtr, &x, &y, &z);
        long value = (*gridPointPtr);

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        PexpandToNeighbor(myGridPtr, x+1, y,   z,   (value + xCost), queuePtr);
        PexpandToNeighbor(myGridPtr, x-1, y,   z,   (value + xCost), queuePtr);
        PexpandToNeighbor(myGridPtr, x,   y+1, z,   (value + yCost), queuePtr);
        PexpandToNeighbor(myGridPtr, x,   y-1, z,   (value + yCost), queuePtr);
        PexpandToNeighbor(myGridPtr, x,   y,   z+1, (value + zCost), queuePtr);
        PexpandToNeighbor(myGridPtr, x,   y,   z-1, (value + zCost), queuePtr);

    } /* iterate over work queue */

#if DEBUG
    printf("Expansion (%li, %li, %li) -> (%li, %li, %li):\n",
           srcPtr->x, srcPtr->y, srcPtr->z,
           dstPtr->x, dstPtr->y, dstPtr->z);
    grid_print(myGridPtr);
#endif /*  DEBUG */

    return isPathFound;
}


/* =============================================================================
 * traceToNeighbor
 * =============================================================================
 */
static void
traceToNeighbor (grid_t* myGridPtr,
                 point_t* currPtr,
                 point_t* movePtr,
                 bool_t useMomentum,
                 long bendCost,
                 point_t* nextPtr)
{
    long x = currPtr->x + movePtr->x;
    long y = currPtr->y + movePtr->y;
    long z = currPtr->z + movePtr->z;

    if (grid_isPointValid(myGridPtr, x, y, z) &&
        !grid_isPointEmpty(myGridPtr, x, y, z) &&
        !grid_isPointFull(myGridPtr, x, y, z))
    {
        long value = grid_getPoint(myGridPtr, x, y, z);
        long b = 0;
        if (useMomentum && (currPtr->momentum != movePtr->momentum)) {
            b = bendCost;
        }
        if ((value + b) <= nextPtr->value) { /* '=' favors neighbors over current */
            nextPtr->x = x;
            nextPtr->y = y;
            nextPtr->z = z;
            nextPtr->value = value;
            nextPtr->momentum = movePtr->momentum;
        }
    }
}


/* =============================================================================
 * PdoTraceback
 * =============================================================================
 */
static vector_t*
PdoTraceback (grid_t* gridPtr, grid_t* myGridPtr,
              coordinate_t* dstPtr, long bendCost)
{
    vector_t* pointVectorPtr = PVECTOR_ALLOC(0);
    assert(pointVectorPtr);

    point_t next;
    next.x = dstPtr->x;
    next.y = dstPtr->y;
    next.z = dstPtr->z;
    next.value = grid_getPoint(myGridPtr, next.x, next.y, next.z);
    next.momentum = MOMENTUM_ZERO;

    while (1) {

        long* gridPointPtr = grid_getPointRef(gridPtr, next.x, next.y, next.z);
        PVECTOR_PUSHBACK(pointVectorPtr, (void*)gridPointPtr);
        grid_setPoint(myGridPtr, next.x, next.y, next.z, GRID_POINT_FULL);

        /* Check if we are done */
        if (next.value == 0) {
            break;
        }
        point_t curr = next;

        /*
         * Check 6 neighbors
         *
         * Potential Optimization: Only need to check 5 of these
         */
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, TRUE, bendCost, &next);
        traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, TRUE, bendCost, &next);

#if DEBUG
        printf("(%li, %li, %li)\n", next.x, next.y, next.z);
#endif /* DEBUG */
        /*
         * Because of bend costs, none of the neighbors may appear to be closer.
         * In this case, pick a neighbor while ignoring momentum.
         */
        if ((curr.x == next.x) &&
            (curr.y == next.y) &&
            (curr.z == next.z))
        {
            next.value = curr.value;
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_POSZ, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGX, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGY, FALSE, bendCost, &next);
            traceToNeighbor(myGridPtr, &curr, &MOVE_NEGZ, FALSE, bendCost, &next);

            if ((curr.x == next.x) &&
                (curr.y == next.y) &&
                (curr.z == next.z))
            {
                PVECTOR_FREE(pointVectorPtr);
#if DEBUG
                puts("[dead]");
#endif
                return NULL; /* cannot find path */
            }
        }
    }

#if DEBUG
    puts("");
#endif /* DEBUG */

    return pointVectorPtr;
}


/* =============================================================================
 * router_solve
 * =============================================================================
 */
void
router_solve (void* argPtr)
{
    TM_THREAD_ENTER();

    router_solve_arg_t* routerArgPtr = (router_solve_arg_t*)argPtr;
    router_t* routerPtr = routerArgPtr->routerPtr;
    maze_t* mazePtr = routerArgPtr->mazePtr;
    vector_t* myPathVectorPtr = PVECTOR_ALLOC(0);
    assert(myPathVectorPtr);
#ifdef USE_DISTRIBUTED_QUEUE
    queue_t** workQueues = mazePtr->workQueues;
    int myId = thread_getId() ;
    int myIdcopy = myId;
    long numThread = thread_getNumThread();
#else
    queue_t* workQueuePtr = mazePtr->workQueuePtr;
#endif
    grid_t* gridPtr = mazePtr->gridPtr;
    grid_t* myGridPtr =
        PGRID_ALLOC(gridPtr->width, gridPtr->height, gridPtr->depth);
    assert(myGridPtr);
    long bendCost = routerPtr->bendCost;
#ifdef STD_QUEUE
    Queue_t* myExpansionQueuePtr = Queue_alloc(-1);
#else
    queue_t* myExpansionQueuePtr = PQUEUE_ALLOC(-1);
#endif
#ifdef PRINT_RETRY_NUM
    long int retry_num = 0;
#endif
    /*
     * Iterate over work list to route each path. This involves an
     * 'expansion' and 'traceback' phase for each source/destination pair.
     */
    while (1) {

        pair_t* coordinatePairPtr;
#if defined(FINE_LOCK) && defined(USE_DISTRIBUTED_QUEUE) && defined(WORKSTEAL)
        TASKSTEAL:
        LOCK(queues_lock[myId]);
        if (TMQUEUE_ISEMPTY(workQueues[myId])) {
            coordinatePairPtr = NULL;
        } else {
            coordinatePairPtr = (pair_t*)TMQUEUE_POP(workQueues[myId]);
        }
        UNLOCK(queues_lock[myId]);
        if (coordinatePairPtr == NULL) {
            myId = (myId +1)%numThread;
            if (myIdcopy == (int)myId){
                break;
            }
            goto TASKSTEAL;
        }
#elif defined(USE_DISTRIBUTED_QUEUE) && defined(WORKSTEAL)
        TASKSTEAL:
        TM_BEGIN();
        if (TMQUEUE_ISEMPTY(workQueues[myId])) {
            coordinatePairPtr = NULL;
        } else {
            coordinatePairPtr = (pair_t*)TMQUEUE_POP(workQueues[myId]);
        }
        TM_END();
        if (coordinatePairPtr == NULL) {
            myId = (myId +1)%numThread;
            if (myIdcopy == (int)myId){
                break;
            }
            goto TASKSTEAL;
        }
#elif defined(USE_DISTRIBUTED_QUEUE)
        if (PQUEUE_ISEMPTY(workQueues[myId])) {
            coordinatePairPtr = NULL;
        } else {
            coordinatePairPtr = (pair_t*)PQUEUE_POP(workQueues[myId]);
        }
        if (coordinatePairPtr == NULL) {
                break;
        }
#elif defined(FINE_LOCK)
        LOCK(queue_lock);
        if (TMQUEUE_ISEMPTY(workQueuePtr)) {
            coordinatePairPtr = NULL;
        } else {
            coordinatePairPtr = (pair_t*)TMQUEUE_POP(workQueuePtr);
        }
        UNLOCK(queue_lock);
        if (coordinatePairPtr == NULL) {
            break;
        }
#else
        TM_BEGIN();
        if (TMQUEUE_ISEMPTY(workQueuePtr)) {
            coordinatePairPtr = NULL;
        } else {
            coordinatePairPtr = (pair_t*)TMQUEUE_POP(workQueuePtr);
        }
        TM_END();
        if (coordinatePairPtr == NULL) {
            break;
        }       
#endif
        coordinate_t* srcPtr = (coordinate_t*)coordinatePairPtr->firstPtr;
        coordinate_t* dstPtr = (coordinate_t*)coordinatePairPtr->secondPtr;

        bool_t success = FALSE;
        vector_t* pointVectorPtr = NULL;

#ifndef FINE_LOCK
#ifndef USE_TMGRID_VALIDATION
        TM_BEGIN();
        grid_copy(myGridPtr, gridPtr); /* ok if not most up-to-date */
        if (PdoExpansion(routerPtr, myGridPtr, myExpansionQueuePtr,
                         srcPtr, dstPtr)) {
            pointVectorPtr = PdoTraceback(gridPtr, myGridPtr, dstPtr, bendCost);
            /*
             * TODO: fix memory leak
             *
             * pointVectorPtr will be a memory leak if we abort this transaction
             */
            if (pointVectorPtr) {
                TMGRID_ADDPATH(gridPtr, pointVectorPtr);
                TM_LOCAL_WRITE(success, TRUE);
            }
        }
        TM_END();
#else
        RETRY:
        grid_copy(myGridPtr, gridPtr); /* ok if not most up-to-date */
        if (PdoExpansion(routerPtr, myGridPtr, myExpansionQueuePtr,
                          srcPtr, dstPtr)) {
            pointVectorPtr = PdoTraceback(gridPtr, myGridPtr, dstPtr, bendCost);
             /*
              * TODO: fix memory leak
              *
              * pointVectorPtr will be a memory leak if we abort this transaction
              */
            if (pointVectorPtr) {
                /* 
                *TM_BEGIN();
                *if (!TMGRID_VALIDATION(gridPtr, pointVectorPtr)) {
                *    TM_END();
                *    goto RETRY;
                *}
                * */
                TM_BEGIN();
                long i;
                long n = vector_getSize(pointVectorPtr);
                for (i = 1; i < (n-1); i++) {
                    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
                    long value = (long)TM_SHARED_READ(*gridPointPtr);
                    if (value != GRID_POINT_EMPTY) {
                        TM_END();
#ifdef PRINT_RETRY_NUM
                        retry_num++;
#endif
                        goto RETRY;
                    }
                }
                TMGRID_ADDPATH(gridPtr, pointVectorPtr);
                TM_LOCAL_WRITE(success, TRUE);
                TM_END();
            }
        }
#endif
#else
        RETRY:
        grid_copy(myGridPtr, gridPtr); /* ok if not most up-to-date */
        if (PdoExpansion(routerPtr, myGridPtr, myExpansionQueuePtr,
                          srcPtr, dstPtr)) {
            pointVectorPtr = PdoTraceback(gridPtr, myGridPtr, dstPtr, bendCost);
             /*
              * TODO: fix memory leak
              *
              * pointVectorPtr will be a memory leak if we abort this transaction
              */
            if (pointVectorPtr) {
                long i;
                long n = vector_getSize(pointVectorPtr);
                long * gridPointPtr_arr = (long*)P_MALLOC((n-2)*sizeof(long));
                assert(gridPointPtr_arr);
                for (i = 1; i < (n-1); i++) {
                    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
                    long index = (gridPointPtr - gridPtr->points);
                    gridPointPtr_arr[i-1] = index;
                }
                qsort(gridPointPtr_arr, n - 2, sizeof(long*), compareGridPoints);
                for(i =1;i<(n-1);i++){
                    LOCK(grid_lock[gridPointPtr_arr[i-1]]);
                }
                for (i = 1; i < (n-1); i++) {
                    long* gridPointPtr = (long*)vector_at(pointVectorPtr, i);
                    long value = (long)TM_SHARED_READ(*gridPointPtr);
                    if (value != GRID_POINT_EMPTY) {
                        for(long j =1;j<(n-1);j++){
                            UNLOCK(grid_lock[gridPointPtr_arr[j-1]]);
                        }
                        P_FREE(gridPointPtr_arr);
                        goto RETRY;
                    }
                }
                TMGRID_ADDPATH(gridPtr, pointVectorPtr);
                TM_LOCAL_WRITE(success, TRUE);
                for(i =1;i<(n-1);i++){
                    UNLOCK(grid_lock[gridPointPtr_arr[i-1]]);
                }
                P_FREE(gridPointPtr_arr);
            }
        }
#endif

        if (success) {
            bool_t status = PVECTOR_PUSHBACK(myPathVectorPtr,
                                             (void*)pointVectorPtr);
            assert(status);
        }

    }

    /*
     * Add my paths to global list
     */
    list_t* pathVectorListPtr = routerArgPtr->pathVectorListPtr;
#ifndef FINE_LOCK
    TM_BEGIN();
    TMLIST_INSERT(pathVectorListPtr, (void*)myPathVectorPtr);
    TM_END();
#else
    LOCK(list_lock);
    TMLIST_INSERT(pathVectorListPtr, (void*)myPathVectorPtr);
    UNLOCK(list_lock);
#endif
    PGRID_FREE(myGridPtr);
#ifdef STD_QUEUE
    Queue_free(myExpansionQueuePtr);
#else
    PQUEUE_FREE(myExpansionQueuePtr);
#endif

#if DEBUG
    puts("\nFinal Grid:");
    grid_print(gridPtr);
#endif /* DEBUG */
#ifdef PRINT_RETRY_NUM
    printf("core_num:%ld,thread_num:%ld,retry_num:%ld\n",cpu_getId(),thread_getId(),retry_num);
#endif
    TM_THREAD_EXIT();
}


/* =============================================================================
 *
 * End of router.c
 *
 * =============================================================================
 */
