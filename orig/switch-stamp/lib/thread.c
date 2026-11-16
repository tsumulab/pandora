/* =============================================================================
 *
 * thread.c
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

#define _GNU_SOURCE
#define __USE_GNU

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "thread.h"
#include "types.h"

#include <sched.h>
#include <sys/sysinfo.h>

static THREAD_LOCAL_T    global_threadId        = 0;
static long              global_numCore         = 1;
static long              global_numThread       = 1;
static THREAD_BARRIER_T* global_barrierPtr      = NULL;
static long*             global_threadIds       = NULL;
static THREAD_ATTR_T     global_threadAttr;
static THREAD_T*         global_threads         = NULL;
static void            (*global_funcPtr)(void*) = NULL;
static void*             global_argPtr          = NULL;
static volatile bool_t   global_doShutdown      = FALSE;

#ifdef COARSE_LOCK
#include "tm.h"
LOCK_T _single_global_lock;
#endif

#if defined(HTM)
#include <immintrin.h>

enum {
    END_Tx,         // 0
    END_LOCK,       // 1
    CONFLICT_ABORT, // 2
    CAPACITY_ABORT, // 3
    ILLEGAL,        // 4
    NESTED,         // 5
    ZERO,           // 6
    OTHER,          // 7
};

unsigned long HTM_Stats[40][8] = {0};
unsigned long total_HTM_Stats[8] = {0};

void inc_abortreason(unsigned int abortcode, int cpuid) {
    if (abortcode & _XABORT_CONFLICT) {
        HTM_Stats[cpuid][CONFLICT_ABORT]++;
    } else if (abortcode & _XABORT_CAPACITY) {
        HTM_Stats[cpuid][CAPACITY_ABORT]++;
    } else if (abortcode & _XABORT_DEBUG) {
        HTM_Stats[cpuid][ILLEGAL]++;
    } else if (abortcode & _XABORT_NESTED) {
        HTM_Stats[cpuid][NESTED]++;
    } else if (abortcode == 0) {
        HTM_Stats[cpuid][ZERO]++;
    } else {
        HTM_Stats[cpuid][OTHER]++;
    }
}

void print_stats() {
    for (int i = 0; i < 40; i++) {
        total_HTM_Stats[END_Tx]         += HTM_Stats[i][END_Tx];
        total_HTM_Stats[END_LOCK]       += HTM_Stats[i][END_LOCK];
        total_HTM_Stats[CONFLICT_ABORT] += HTM_Stats[i][CONFLICT_ABORT];
        total_HTM_Stats[CAPACITY_ABORT] += HTM_Stats[i][CAPACITY_ABORT];
        total_HTM_Stats[ILLEGAL]        += HTM_Stats[i][ILLEGAL];
        total_HTM_Stats[NESTED]         += HTM_Stats[i][NESTED];
        total_HTM_Stats[ZERO]           += HTM_Stats[i][ZERO];
        total_HTM_Stats[OTHER]          += HTM_Stats[i][OTHER];
    }
    printf("\n====================================\n");
    printf(" # HTMEnd_Tx    : %10lu\n", total_HTM_Stats[END_Tx]);
    printf(" # HTMEnd_LOCK  : %10lu\n", total_HTM_Stats[END_LOCK]);
    printf(" # HTMTxConflict: %10lu\n", total_HTM_Stats[CONFLICT_ABORT]);
    printf(" # HTMCapacity  : %10lu\n", total_HTM_Stats[CAPACITY_ABORT]);
    printf(" # HTMIllegal   : %10lu\n", total_HTM_Stats[ILLEGAL]);
    printf(" # HTMNested    : %10lu\n", total_HTM_Stats[NESTED]);
    printf(" # HTM0         : %10lu\n", total_HTM_Stats[ZERO]);
    printf(" # HTMother     : %10lu\n", total_HTM_Stats[OTHER]);
    printf("====================================\n");
}

#endif

void
print_info()
{
    long numCore = global_numCore;
    long numThread = global_numThread;
    printf(
        "=============== Execution Info ===============\n"
        "  * Number of Cores            : %4ld\n"
        "  * Number of Threads per Core : %4ld\n"
        "  * Total Number of Threads    : %4ld\n"
#  if defined(THREAD_SWITCHING)
        "     - thread switching (yield) : on\n"
#  else
        "     - thread switching (yield) : off\n"
#  endif
        #if defined(LIBFIBRE)
        "  * Kind of Thread Impl: libfibre\n"
#else
        "  * Kind of Thread Impl : pthread\n"
#endif
#if defined(TINYSTM)
        "  * Kind of Backend: Tinystm\n"
#elif defined(COARSE_LOCK)
        "  * Kind of Backend: Coarse-grained lock\n"
#elif defined(FINE_LOCK)
        "  * Kind of Backend: Fine-grained lock\n"
#elif defined(HTM) && defined(RTM)
        "  * Kind of Backend: RTM (Intel TSX)\n"
#endif
#if defined(USE_TLMALLOC)
        "  * Kind of Memory Allocator: TLMalloc\n"
#elif defined(USE_TCMALLOC)
        "  * Kind of Memory Allocator: TCMalloc\n"
#else
        "  * Kind of Memory Allocator: malloc\n"
#endif
#if defined(ENABLE_BIND)
        "  * Core-bind: on\n"
#else
        "  * Core-bind: off\n"
#endif
        "==============================================\n\n"
    , numCore, numThread / numCore, numThread);
}


/* =============================================================================
 * threadWait
 * -- Synchronizes all threads to start/stop parallel section
 * =============================================================================
 */
static void
threadWait (void* argPtr)
{
    long threadId = *(long*)argPtr;

    THREAD_LOCAL_SET(global_threadId, (long)threadId);

#if !defined(LIBFIBRE) && defined(ENABLE_BIND)
    int nprocs = get_nprocs();
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET((threadId+1) % nprocs, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
#endif

    while (1) {
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for start parallel */
        if (global_doShutdown) {
            break;
        }
        global_funcPtr(global_argPtr);
        THREAD_BARRIER(global_barrierPtr, threadId); /* wait for end parallel */
        if (threadId == 0) {
            break;
        }
    }
}


/* =============================================================================
 * thread_startup
 * -- Create pool of secondary threads
 * -- numThread is total number of threads (primary + secondaries)
 * =============================================================================
 */
void
thread_startup (long numCore, long numThread)
{
    long i;

    global_numCore    = numCore;
    global_numThread  = numThread;
    global_doShutdown = FALSE;

    print_info();

    /* Set up barrier */
    assert(global_barrierPtr == NULL);
    global_barrierPtr = THREAD_BARRIER_ALLOC(numThread);
    assert(global_barrierPtr);
    THREAD_BARRIER_INIT(global_barrierPtr, numThread);

    /* Set up ids */
    THREAD_LOCAL_INIT(global_threadId);
    assert(global_threadIds == NULL);
    global_threadIds = (long*)malloc(numThread * sizeof(long));
    assert(global_threadIds);
    for (i = 0; i < numThread; i++) {
        global_threadIds[i] = i;
    }

    /* Set up thread list */
    assert(global_threads == NULL);
    global_threads = (THREAD_T*)malloc(numThread * sizeof(THREAD_T));
    assert(global_threads);

    /* Set up pool */
    THREAD_ATTR_INIT(global_threadAttr);
    for (i = 1; i < numThread; i++) {
        THREAD_CREATE(global_threads[i],
                      global_threadAttr,
                      &threadWait,
                      &global_threadIds[i]);
    }

    /*
     * Wait for primary thread to call thread_start
     */
}


/* =============================================================================
 * thread_start
 * -- Make primary and secondary threads execute work
 * -- Should only be called by primary thread
 * -- funcPtr takes one arguments: argPtr
 * =============================================================================
 */
void
thread_start (void (*funcPtr)(void*), void* argPtr)
{
    global_funcPtr = funcPtr;
    global_argPtr = argPtr;

    long threadId = 0; /* primary */
    threadWait((void*)&threadId);
}


/* =============================================================================
 * thread_shutdown
 * -- Primary thread kills pool of secondary threads
 * =============================================================================
 */
void
thread_shutdown ()
{
    /* Make secondary threads exit wait() */
    global_doShutdown = TRUE;
    THREAD_BARRIER(global_barrierPtr, 0);

    long numThread = global_numThread;

    long i;
    for (i = 1; i < numThread; i++) {
        THREAD_JOIN(global_threads[i]);
    }

    THREAD_BARRIER_FREE(global_barrierPtr);
    global_barrierPtr = NULL;

    free(global_threadIds);
    global_threadIds = NULL;

    free(global_threads);
    global_threads = NULL;

    global_numThread = 1;
}


/* =============================================================================
 * thread_barrier_alloc
 * =============================================================================
 */
thread_barrier_t*
thread_barrier_alloc (long numThread)
{
    thread_barrier_t* barrierPtr;

    assert(numThread > 0);
    assert((numThread & (numThread - 1)) == 0); /* must be power of 2 */
    barrierPtr = (thread_barrier_t*)malloc(numThread * sizeof(thread_barrier_t));
    if (barrierPtr != NULL) {
        barrierPtr->numThread = numThread;
    }

    return barrierPtr;
}


/* =============================================================================
 * thread_barrier_free
 * =============================================================================
 */
void
thread_barrier_free (thread_barrier_t* barrierPtr)
{
    free(barrierPtr);
}


/* =============================================================================
 * thread_barrier_init
 * =============================================================================
 */
void
thread_barrier_init (thread_barrier_t* barrierPtr)
{
    long i;
    long numThread = barrierPtr->numThread;

    for (i = 0; i < numThread; i++) {
        barrierPtr[i].count = 0;
        THREAD_MUTEX_INIT(barrierPtr[i].countLock);
        THREAD_COND_INIT(barrierPtr[i].proceedCond);
        THREAD_COND_INIT(barrierPtr[i].proceedAllCond);
    }
}


/* =============================================================================
 * thread_barrier
 * -- Simple logarithmic barrier
 * =============================================================================
 */
void
thread_barrier (thread_barrier_t* barrierPtr, long threadId)
{
    long i = 2;
    long base = 0;
    long index;
    long numThread = barrierPtr->numThread;

    if (numThread < 2) {
        return;
    }

    do {
        index = base + threadId / i;
        if ((threadId % i) == 0) {
            THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
            barrierPtr[index].count++;
            while (barrierPtr[index].count < 2) {
                THREAD_COND_WAIT(barrierPtr[index].proceedCond,
                                 barrierPtr[index].countLock);
            }
            THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
        } else {
            THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
            barrierPtr[index].count++;
            if (barrierPtr[index].count == 2) {
                THREAD_COND_SIGNAL(barrierPtr[index].proceedCond);
            }
            while (THREAD_COND_WAIT(barrierPtr[index].proceedAllCond,
                                    barrierPtr[index].countLock) != 0)
            {
                /* wait */
            }
            THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
            break;
        }
        base = base + numThread / i;
        i *= 2;
    } while (i <= numThread);

    for (i /= 2; i > 1; i /= 2) {
        base = base - numThread / i;
        index = base + threadId / i;
        THREAD_MUTEX_LOCK(barrierPtr[index].countLock);
        barrierPtr[index].count = 0;
        THREAD_COND_SIGNAL(barrierPtr[index].proceedAllCond);
        THREAD_MUTEX_UNLOCK(barrierPtr[index].countLock);
    }
}


/* =============================================================================
 * barrier_normal_alloc
 * =============================================================================
 */
barrier_normal_t *barrier_normal_alloc() {
    return (barrier_normal_t *)malloc(sizeof(barrier_normal_t));
}


/* =============================================================================
 * barrier_normal_free
 * =============================================================================
 */
void barrier_normal_free(barrier_normal_t *b) {
    free(b);
}


/* =============================================================================
 * barrier_normal_init
 * =============================================================================
 */
void barrier_normal_init(barrier_normal_t *b, int n) {
    THREAD_COND_INIT(b->complete);
    THREAD_MUTEX_INIT(b->mutex);
    b->count = n;
    b->crossing = 0;
}


/* =============================================================================
 * barrier_normal_cross
 * -- Simple normal barrier
 * =============================================================================
 */
void barrier_normal_cross(barrier_normal_t *b) {
    THREAD_MUTEX_LOCK(b->mutex);
    /* One more thread through */
    b->crossing++;
    /* If not all here, wait */
    if (b->crossing < b->count) {
        THREAD_COND_WAIT(b->complete, b->mutex);
    } else {
        /* Reset for next time */
        b->crossing = 0;
        THREAD_COND_BROADCAST(b->complete);
    }
    THREAD_MUTEX_UNLOCK(b->mutex);
}


/* =============================================================================
 * thread_getId
 * -- Call after thread_start() to get cpu ID inside parallel region
 * =============================================================================
 */
long
cpu_getId()
{
    return (long)sched_getcpu();
}


/* =============================================================================
 * thread_getId
 * -- Call after thread_start() to get thread ID inside parallel region
 * =============================================================================
 */
long
thread_getId()
{
    return (long)THREAD_LOCAL_GET(global_threadId);
}


/* =============================================================================
 * thread_getNumCore
 * -- Call after thread_start() to get number of Cores inside parallel region
 * =============================================================================
 */
long
thread_getNumCore()
{
    return global_numCore;
}


/* =============================================================================
 * thread_getNumThread
 * -- Call after thread_start() to get number of threads inside parallel region
 * =============================================================================
 */
long
thread_getNumThread()
{
    return global_numThread;
}


/* =============================================================================
 * thread_barrier_wait
 * -- Call after thread_start() to synchronize threads inside parallel region
 * =============================================================================
 */
void
thread_barrier_wait()
{
#ifndef SIMULATOR
    long threadId = thread_getId();
    (void) threadId;
#endif /* !SIMULATOR */
    THREAD_BARRIER(global_barrierPtr, threadId);
}


/* =============================================================================
 * TEST_THREAD
 * =============================================================================
 */
#ifdef TEST_THREAD


#include <stdio.h>
#include <unistd.h>


#define NUM_THREADS    (4)
#define NUM_ITERATIONS (3)



void
printId (void* argPtr)
{
    long threadId = thread_getId();
    long numThread = thread_getNumThread();
    long i;

    for ( i = 0; i < NUM_ITERATIONS; i++ ) {
        thread_barrier_wait();
        if (threadId == 0) {
            sleep(1);
        } else if (threadId == numThread-1) {
            usleep(100);
        }
        printf("i = %li, tid = %li\n", i, threadId);
        if (threadId == 0) {
            puts("");
        }
        fflush(stdout);
    }
}


int
main ()
{
    puts("Starting...");

    /* Run in parallel */
    thread_startup(NUM_THREADS);
    /* Start timing here */
    thread_start(printId, NULL);
    thread_start(printId, NULL);
    thread_start(printId, NULL);
    /* Stop timing here */
    thread_shutdown();

    puts("Done.");

    return 0;
}


#endif /* TEST_THREAD */


/* =============================================================================
 *
 * End of thread.c
 *
 * =============================================================================
 */
