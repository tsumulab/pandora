/* =============================================================================
 *
 * normal.c
 * -- Implementation of normal k-means clustering algorithm
 *
 * =============================================================================
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department, Northwestern University/* =============================================================================
 *
 * normal.c
 * -- Implementation of normal k-means clustering algorithm
 *
 * =============================================================================
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department, Northwestern University
 * email: wkliao@ece.northwestern.edu
 *
 *
 * Edited by:
 *
 * Jay Pisharath
 * Northwestern University.
 *
 * Chi Cao Minh
 * Stanford University
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
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "common.h"
#include "normal.h"
#include "random.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "util.h"

double global_time = 0.0;

typedef struct args {
    double** feature;
    int     nfeatures;
    int     npoints;
    int     nclusters;
    int*    membership;
    double** clusters;
#ifdef LENSIZE_64
    long int**   new_centers_len;
#define CACHE_LINE_SIZE 64
#else
    int**   new_centers_len;
#define CACHE_LINE_SIZE 32
#endif
    double** new_centers;
    int     nthreads;
    long int* global_stask;
    long int* global_etask;
} args_t;

double global_delta;

#ifdef FINE_LOCK
LOCK_T delta_lock;
LOCK_T *new_centers_len_lock;
LOCK_T **new_centers_lock;
LOCK_T *global_stask_lock;
LOCK_T *global_etask_lock;
#endif

#ifdef ORIGINAL_CHUNK
    #define CHUNK 3
#elif defined(LARGE_CHUNK)
    #define CHUNK 128
#else
    #define CHUNK 8
#endif


#ifndef FINE_LOCK
/* =============================================================================
 * work
 * =============================================================================
 */
static void
work (void* argPtr)
{
    TM_THREAD_ENTER();

    args_t* args = (args_t*)argPtr;
    double** feature         = args->feature;
    int     nfeatures       = args->nfeatures;
    int     npoints         = args->npoints;
    int     nclusters       = args->nclusters;
    int*    membership      = args->membership;
    double** clusters        = args->clusters;
#ifdef LENSIZE_64
    long int**   new_centers_len = args->new_centers_len;
#else
    int**   new_centers_len = args->new_centers_len;
#endif
    double** new_centers     = args->new_centers;
    int     nthreads         = args->nthreads;
    long int* global_stask        = args->global_stask;
    long int* global_etask        = args->global_etask;
    
    double delta = 0.0;
    int index;
    int i;
    int j;
    int n;
    long int t;
    int k;
    long int start;
    long int stop;
    int myId;
    int* movecounts = (int*)calloc(nclusters , sizeof(int));
    assert(movecounts);
    double** local_new_centers;
    local_new_centers = (double**)calloc(nclusters , sizeof(double*));
    assert(local_new_centers);
    for (i = 0; i < nclusters; i++) {
        local_new_centers[i] = (double*)calloc(nfeatures , sizeof(double));
    assert(local_new_centers[i]);
    }

    myId = thread_getId();

    TM_BEGIN();
    start = TM_SHARED_READ(global_stask[myId]);
    t = TM_SHARED_READ(global_etask[myId]);
    TM_END();


    while (start < t) {
        stop = (((start + CHUNK) < t) ? (start + CHUNK) : t);
        for (i = start; i < stop; i++) {
            

            index = common_findNearestPoint(feature[i],
                                            nfeatures,
                                            clusters,
                                            nclusters);
            
            /*
             * If membership changes, increase delta by 1.
             * membership[i] cannot be changed by other threads
             */
            if (membership[i] != index) {
                delta += 1.0;
            }

            /* Assign the membership to object i */
            /* membership[i] can't be changed by other thread */          
            /* Update new cluster centers : sum of objects located within */
            if (membership[i] < 0){
                movecounts[index]++;
#ifdef NO_BATCH
            TM_BEGIN();
            for (j = 0; j < nfeatures; j++) {
                TM_SHARED_WRITE_D(
                    new_centers[index][j],
                    (TM_SHARED_READ_D(new_centers[index][j]) + feature[i][j])
                );
            }
            TM_END();
#else
                for (j = 0; j < nfeatures; j++) {
                    local_new_centers[index][j]=
                    local_new_centers[index][j] + feature[i][j];
                }
#endif
                    
            }else{

                if (membership[i] != index){
                
                    movecounts[membership[i]]--;
                    movecounts[index]++;
#ifdef NO_BATCH
                    TM_BEGIN();
                    for(j = 0; j < nfeatures; j++){
                        TM_SHARED_WRITE_D(new_centers[index][j],
                        (TM_SHARED_READ_D(new_centers[index][j]) + feature[i][j] ));
                        TM_SHARED_WRITE_D(new_centers[membership[i]][j],
                        (TM_SHARED_READ_D(new_centers[membership[i]][j]) - feature[i][j] ));
                        
                    }
                    TM_END();
#else
                    for(j = 0; j < nfeatures; j++){
                        local_new_centers[index][j]=
                        local_new_centers[index][j] + feature[i][j];
                        local_new_centers[membership[i]][j]=
                        local_new_centers[membership[i]][j] - feature[i][j];
                    }
#endif
                             
                }
            }
               
               
            membership[i] = index;
                      
        }

        /* Update task */
        TM_BEGIN();
        t = TM_SHARED_READ(global_etask[myId]);
        //stop = TM_SHARED_READ(global_stask[myId]) + CHUNK;
        if (stop < t) {
            TM_SHARED_WRITE(global_stask[myId],(TM_SHARED_READ(global_stask[myId])+CHUNK));
            TM_END();
            start = stop;
        } 
        else{
            TM_END();
            break;
        }
    }
    #ifdef WORKSTEAL
        TASKSTEAL:
        n = (rand() % nthreads);
        //n = 1;
        for (k = 0; k<nthreads; k++){
            if((myId + n + k)%nthreads!=myId){

            TM_BEGIN();
            t = TM_SHARED_READ(global_etask[((myId+n+k)% nthreads)]);
            //start = TM_SHARED_READ(global_stask[((myId+n+k) % nthreads)])+CHUNK;
            stop = TM_SHARED_READ(global_stask[((myId+n+k) % nthreads)])+CHUNK+CHUNK;
            if (stop < t) {
                TM_SHARED_WRITE(global_etask[(myId+n+k)%nthreads],(TM_SHARED_READ(global_etask[(myId+n+k)%nthreads])-CHUNK));
                TM_END();
                    stop = t;
                    start  = stop - CHUNK;
                for (i = start; i < stop; i++) {
                    index = common_findNearestPoint(feature[i],
                                                    nfeatures,
                                                    clusters,
                                                    nclusters);
                    if (membership[i] != index) {
                        delta += 1.0;
                    }
                    if (membership[i] < 0){
                        movecounts[index]++;
#ifdef NO_BATCH
                    TM_BEGIN();
                    for (j = 0; j < nfeatures; j++) {
                        TM_SHARED_WRITE_D(
                            new_centers[index][j],
                            (TM_SHARED_READ_D(new_centers[index][j]) + feature[i][j])
                        );
                    }
                    TM_END();
#else
                        for (j = 0; j < nfeatures; j++) {
                            local_new_centers[index][j]=
                            local_new_centers[index][j] + feature[i][j];
                        }
#endif
                    }else{

                        if (membership[i] != index){
                                
                            movecounts[membership[i]]--;
                            movecounts[index]++;
#ifdef NO_BATCH
                            TM_BEGIN();
                            for(j = 0; j < nfeatures; j++){
                                TM_SHARED_WRITE_D(new_centers[index][j],
                                (TM_SHARED_READ_D(new_centers[index][j]) + feature[i][j] ));
                                TM_SHARED_WRITE_D(new_centers[membership[i]][j],
                                (TM_SHARED_READ_D(new_centers[membership[i]][j]) - feature[i][j] ));
                                
                            }
                            TM_END();
#else                
                            for(j = 0; j < nfeatures; j++){
                                local_new_centers[index][j]=
                                local_new_centers[index][j] + feature[i][j];
                                local_new_centers[membership[i]][j]=
                                local_new_centers[membership[i]][j] - feature[i][j];
                            }
#endif
                        }
                    }
                    membership[i] = index;
                }
                goto TASKSTEAL;

            }else{
                TM_END();
            }
            }
        }
   	#endif
#if 1
        TM_BEGIN();
        for (i = 0; i < nclusters; i++){
            TM_SHARED_WRITE(*new_centers_len[i],
            TM_SHARED_READ(*new_centers_len[i]) + movecounts[i]);
        }
        TM_END();
#else
        for (i = 0; i < nclusters; i++){
            TM_BEGIN();
	        if(movecounts[i] != 0){
	            TM_SHARED_WRITE(*new_centers_len[i],
                TM_SHARED_READ(*new_centers_len[i]) + movecounts[i]);
	        }
            TM_END();
	    }
#endif
#ifndef NO_BATCH
        for(i = 0; i < nclusters; i++){
            TM_BEGIN();
            for(j = 0;j < nfeatures; j++){
            if(local_new_centers[i][j] != 0.0){
                TM_SHARED_WRITE_D(new_centers[i][j],TM_SHARED_READ_D(new_centers[i][j])+local_new_centers[i][j]);
                }
            }
            TM_END();
        }
#endif

    	TM_BEGIN();
    	TM_SHARED_WRITE_D(global_delta, TM_SHARED_READ_D(global_delta) + delta);
    	TM_END();

    P_FREE(movecounts);
    P_FREE(local_new_centers);
    TM_THREAD_EXIT();
}

#else

/* =============================================================================
 * work
 * =============================================================================
 */
static void
work (void* argPtr)
{
    TM_THREAD_ENTER();

    args_t* args = (args_t*)argPtr;
    double** feature         = args->feature;
    int     nfeatures       = args->nfeatures;
    int     npoints         = args->npoints;
    int     nclusters       = args->nclusters;
    int*    membership      = args->membership;
    double** clusters        = args->clusters;
#ifdef LENSIZE_64
    long int**   new_centers_len = args->new_centers_len;
#else
    int**   new_centers_len = args->new_centers_len;
#endif
    double** new_centers     = args->new_centers;
    int     nthreads         = args->nthreads;
    long int* global_stask        = args->global_stask;
    long int* global_etask        = args->global_etask;
    double delta = 0.0;
    int index;
    int i;
    int j;
    int n;
    long int t;
    int k;
    long int start;
    long int stop;
    int myId;
    int* movecounts = (int*)calloc(nclusters , sizeof(int));
    assert(movecounts);
    double** local_new_centers;
    local_new_centers = (double**)calloc(nclusters , sizeof(double*));
    assert(local_new_centers);
    for (i = 0; i < nclusters; i++) {
        local_new_centers[i] = (double*)calloc(nfeatures , sizeof(double));
    assert(local_new_centers[i]);
    }

    myId = thread_getId();
    LOCK(global_stask_lock[myId]);
    start = TM_SHARED_READ(global_stask[myId]);
    UNLOCK(global_stask_lock[myId]);
    LOCK(global_etask_lock[myId]);
    t = TM_SHARED_READ(global_etask[myId]);
    UNLOCK(global_etask_lock[myId]);

    while (start < t) {
        stop = (((start + CHUNK) < t) ? (start + CHUNK) : t);
        for (i = start; i < stop; i++) {
            

            index = common_findNearestPoint(feature[i],
                                            nfeatures,
                                            clusters,
                                            nclusters);
            /*
             * If membership changes, increase delta by 1.
             * membership[i] cannot be changed by other threads
             */
            if (membership[i] != index) {
                delta += 1.0;
            }

            /* Assign the membership to object i */
            /* membership[i] can't be changed by other thread */      

            /* Update new cluster centers : sum of objects located within */
            if (membership[i] < 0){
                movecounts[index]++;
                for (j = 0; j < nfeatures; j++) {
                    local_new_centers[index][j]=
                    local_new_centers[index][j] + feature[i][j];
                }
                    
            }else{

                if (membership[i] != index){
                
                    movecounts[membership[i]]--;
                    movecounts[index]++;
                    for(j = 0; j < nfeatures; j++){
                        local_new_centers[index][j]=
                        local_new_centers[index][j] + feature[i][j];
                        local_new_centers[membership[i]][j]=
                        local_new_centers[membership[i]][j] - feature[i][j];
                    }
                             
                }
            }
               
               
            membership[i] = index;
                      
        }
        /* Update task */
        LOCK(global_stask_lock[myId]);
        LOCK(global_etask_lock[myId]);
        t = TM_SHARED_READ(global_etask[myId]);
        //stop = TM_SHARED_READ(global_stask[myId]) + CHUNK;
        if (stop < t) {
            TM_SHARED_WRITE(global_stask[myId],(TM_SHARED_READ(global_stask[myId])+CHUNK));
            UNLOCK(global_etask_lock[myId]);
            UNLOCK(global_stask_lock[myId]);
            start = stop;
        } 
        else{
            UNLOCK(global_etask_lock[myId]);
            UNLOCK(global_stask_lock[myId]);
            
            break;
        }
    }
        TASKSTEAL:
        n = (rand() % nthreads);
        for (k = 0; k<nthreads; k++){
            if((myId + n + k)%nthreads!=myId){

            LOCK(global_stask_lock[((myId+n+k)%nthreads)]);
            LOCK(global_etask_lock[((myId+n+k)%nthreads)]);
            t = TM_SHARED_READ(global_etask[((myId+n+k)% nthreads)]);
            //start = TM_SHARED_READ(global_stask[((myId+n+k) % nthreads)])+CHUNK;
            stop = TM_SHARED_READ(global_stask[((myId+n+k) % nthreads)])+CHUNK+CHUNK;
            if (stop < t) {
                TM_SHARED_WRITE(global_etask[(myId+n+k)%nthreads],(TM_SHARED_READ(global_etask[(myId+n+k)%nthreads])-CHUNK));
                UNLOCK(global_etask_lock[((myId+n+k)%nthreads)]);
                UNLOCK(global_stask_lock[((myId+n+k)%nthreads)]);
                    stop = t;
                    start  = stop - CHUNK;

                for (i = start; i < stop; i++) {
                    index = common_findNearestPoint(feature[i],
                                                    nfeatures,
                                                    clusters,
                                                    nclusters);
                    if (membership[i] != index) {
                        delta += 1.0;
                    }
                    if (membership[i] < 0){
                        movecounts[index]++;
                        for (j = 0; j < nfeatures; j++) {
                            local_new_centers[index][j]=
                            local_new_centers[index][j] + feature[i][j];
                        }
                    }else{

                        if (membership[i] != index){
                                
                            movecounts[membership[i]]--;
                            movecounts[index]++;                
                            for(j = 0; j < nfeatures; j++){
                                local_new_centers[index][j]=
                                local_new_centers[index][j] + feature[i][j];
                                local_new_centers[membership[i]][j]=
                                local_new_centers[membership[i]][j] - feature[i][j];
                            }
                        }
                    }
                    membership[i] = index;
                }
                goto TASKSTEAL;

            }else{
                UNLOCK(global_etask_lock[((myId+n+k)%nthreads)]);
                UNLOCK(global_stask_lock[((myId+n+k)%nthreads)]);
            }
            }
        }
   	
        
        for (i = 0; i < nclusters; i++){
            LOCK(new_centers_len_lock[i]);
            TM_SHARED_WRITE(*new_centers_len[i],
            TM_SHARED_READ(*new_centers_len[i]) + movecounts[i]);
            UNLOCK(new_centers_len_lock[i]);
        }

        for(i = 0; i < nclusters; i++){
            for(j = 0;j < nfeatures; j++){
            if(local_new_centers[i][j] != 0.0){
                LOCK(new_centers_lock[i][j]);
                TM_SHARED_WRITE_D(new_centers[i][j],TM_SHARED_READ_D(new_centers[i][j])+local_new_centers[i][j]);
                UNLOCK(new_centers_lock[i][j]);
                }
            }
        }
        
      

    	LOCK(delta_lock);
    	TM_SHARED_WRITE_D(global_delta, TM_SHARED_READ_D(global_delta) + delta);
    	UNLOCK(delta_lock);

    P_FREE(movecounts);
    P_FREE(local_new_centers);
    TM_THREAD_EXIT();
}

#endif

/* =============================================================================
 * normal_exec
 * =============================================================================
 */
double**
normal_exec (int       nthreads,
             double**   feature,    /* in: [npoints][nfeatures] */
             int       nfeatures,
             int       npoints,
             int       nclusters,
             double     threshold,
             int*      membership,
             random_t* randomPtr) /* out: [npoints] */
{
    int i;
    int j;
    int loop = 0;
#ifdef LENSIZE_64
    long int** new_centers_len;
#else
    int** new_centers_len; /* [nclusters]: no. of points in each cluster */
#endif
    double delta;
    double** clusters;      /* out: [nclusters][nfeatures] */
    double** new_centers;   /* [nclusters][nfeatures] */
    void* alloc_memory = NULL;
    long int* global_stask;
    long int* global_etask;
    args_t args;
    TIMER_T start;
    TIMER_T stop;

    /* Allocate space for returning variable clusters[] */
    clusters = (double**)P_MALLOC(nclusters * sizeof(double*));
    assert(clusters);
    clusters[0] = (double*)P_MALLOC(nclusters * nfeatures * sizeof(double));
    assert(clusters[0]);
    for (i = 1; i < nclusters; i++) {
        clusters[i] = clusters[i-1] + nfeatures;
    }
        global_etask = (long int*)P_MALLOC(nthreads*sizeof(long int));
        global_stask = (long int*)P_MALLOC(nthreads*sizeof(long int));
        assert(global_etask && global_stask);

    /* Randomly pick cluster centers */
    for (i = 0; i < nclusters; i++) {
        int n = (int)(random_generate(randomPtr) % npoints);
        for (j = 0; j < nfeatures; j++) {
            clusters[i][j] = feature[n][j];
        }
    }

    for (i = 0; i < npoints; i++) {
        membership[i] = -1;
    }

    /*
     * Need to initialize new_centers_len and new_centers[0] to all 0.
     * Allocate clusters on different cache lines to reduce false sharing.
     */
    {
#ifdef LENSIZE_64
        int cluster_size = sizeof(long int) + sizeof(double) * nfeatures;
#else
        int cluster_size = sizeof(int) + sizeof(double) * nfeatures;
#endif
        const int cacheLineSize = CACHE_LINE_SIZE;
        cluster_size += (cacheLineSize-1) - ((cluster_size-1) % cacheLineSize);
        alloc_memory = calloc(nclusters, cluster_size);
#ifdef LENSIZE_64
        new_centers_len = (long int**)P_MALLOC(nclusters * sizeof(long int*));
#else
        new_centers_len = (int**)P_MALLOC(nclusters * sizeof(int*));
#endif
        new_centers = (double**)P_MALLOC(nclusters * sizeof(double*));
        assert(alloc_memory && new_centers && new_centers_len);
        for (i = 0; i < nclusters; i++) {
#ifdef LENSIZE_64
            new_centers_len[i] = (long int*)((char*)alloc_memory + cluster_size * i);
            new_centers[i] = (double*)((char*)alloc_memory + cluster_size * i + sizeof(long int));
#else
            new_centers_len[i] = (int*)((char*)alloc_memory + cluster_size * i);
            new_centers[i] = (double*)((char*)alloc_memory + cluster_size * i + sizeof(int));
#endif
        }
        //global_task = (int*)calloc(nthreads , sizeof(int));
    }

#ifdef FINE_LOCK
    /*
     * Allocate resources and initialize them
     * in order to use lock (pthread_mutex or pthread_spinlock).
     */
    new_centers_len_lock = (LOCK_T*) P_MALLOC(nclusters * sizeof(LOCK_T));
    new_centers_lock = (LOCK_T**) P_MALLOC(nclusters * sizeof(LOCK_T*));
    assert(new_centers_len_lock && new_centers_lock);
    for (i = 0; i < nclusters; i++) {
        new_centers_lock[i] = (LOCK_T*) P_MALLOC(npoints * sizeof(LOCK_T));
        assert(new_centers_lock[i]);
    }

    LOCK_INIT(delta_lock);
    for (i = 0; i < nclusters; i++) {
        LOCK_INIT(new_centers_len_lock[i]);
        for (j = 0; j < npoints; j++) {
            LOCK_INIT(new_centers_lock[i][j]);
        }
    }
    global_etask_lock = (LOCK_T*) P_MALLOC(nthreads * sizeof(LOCK_T));
    global_stask_lock = (LOCK_T*) P_MALLOC(nthreads * sizeof(LOCK_T));
    assert(global_etask_lock && global_stask_lock);
    for (i = 0; i < nthreads; i++){
        LOCK_INIT(global_etask_lock[i]);
        LOCK_INIT(global_stask_lock[i]);
    }
#endif

    TIMER_READ(start);

    GOTO_SIM();

    do {
        delta = 0.0;

        args.feature         = feature;
        args.nfeatures       = nfeatures;
        args.npoints         = npoints;
        args.nclusters       = nclusters;
        args.membership      = membership;
        args.clusters        = clusters;
        args.new_centers_len = new_centers_len;
        args.new_centers     = new_centers;
        args.nthreads        = nthreads;
        args.global_etask    = global_etask;
        args.global_stask    = global_stask;

        global_delta = delta;
        if (nthreads != 1){
            for (int k = 0; k < nthreads-1; k++){
                global_stask[k] = (npoints / nthreads) * k;
                global_etask[k] = (npoints / nthreads)* (k+1);
            }
        }
        global_stask[nthreads-1] = (npoints / nthreads)* (nthreads - 1);
        global_etask[nthreads-1] = npoints;

#ifdef OTM
#pragma omp parallel
        {
            work(&args);
        }
#else
        thread_start(work, &args);
#endif

        

        delta = global_delta;

        /* Replace old cluster centers with new_centers */
        for (i = 0; i < nclusters; i++) {
            for (j = 0; j < nfeatures; j++) {
                if (new_centers_len[i] != NULL && *new_centers_len[i] > 0) {
                    clusters[i][j] = new_centers[i][j] / *new_centers_len[i];
                }
            }
        }

        delta /= npoints;

    } while ((delta > threshold) && (loop++ < 500));
#if 0
    printf("loop=%d\n",loop);
#endif
    GOTO_REAL();

    TIMER_READ(stop);
    global_time += TIMER_DIFF_SECONDS(start, stop);

    P_FREE(alloc_memory);
    P_FREE(new_centers);
    P_FREE(new_centers_len);
    P_FREE(global_etask);
    P_FREE(global_stask);

    return clusters;
}


/* =============================================================================
 *
 * End of normal.c
 *
 * =============================================================================
 */

 
