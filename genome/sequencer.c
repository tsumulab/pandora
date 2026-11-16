/* =============================================================================
 *
 * sequencer.c
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Author: Chi Cao Minh
 *
 * =============================================================================
 *
 * Algorithm overview:
 *
 * 1) Remove duplicate segments by using hash-set
 * 2) Match segments using hash-based comparisons
 *    - Cycles are prevented by tracking starts/ends of matched chains
 * 3) Build sequence
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


#include "tm.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "hashtable.h"
#include "segments.h"
#include "sequencer.h"
#include "table.h"
#include "thread.h"
#include "utility.h"
#include "vector.h"
#include "types.h"


struct endInfoEntry {
    bool_t isEnd;
    long jumpToNext;
};

struct constructEntry {
    bool_t isStart;
    char* segment;
    ulong_t endHash;
    struct constructEntry* startPtr;
    struct constructEntry* nextPtr;
    struct constructEntry* endPtr;
    long overlap;
    long length;
#ifdef FINE_LOCK
    LOCK_T lock;
#endif
};

#ifdef FINE_LOCK
extern LOCK_T* lock_step1;
extern LOCK_T** lock_step2a;
#endif

/* =============================================================================
 * hashString
 * -- uses sdbm hash function
 * =============================================================================
 */
static ulong_t
hashString (char* str)
{
    ulong_t hash = 0;
    long c;

    /* Note: Do not change this hashing scheme */
    while ((c = *str++) != '\0') {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return (ulong_t)hash;
}


/* =============================================================================
 * hashSegment
 * -- For hashtable
 * =============================================================================
 */
static ulong_t
hashSegment (const void* keyPtr)
{
    return (ulong_t)hash_sdbm((char*)keyPtr); /* can be any "good" hash function */
}


/* =============================================================================
 * compareSegment
 * -- For hashtable
 * =============================================================================
 */
static long
compareSegment (const pair_t* a, const pair_t* b)
{
    return strcmp((char*)(a->firstPtr), (char*)(b->firstPtr));
}


/* =============================================================================
 * sequencer_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
sequencer_t*
sequencer_alloc (long geneLength, long segmentLength, segments_t* segmentsPtr)
{
    sequencer_t* sequencerPtr;
    long maxNumUniqueSegment = geneLength - segmentLength + 1;
    long i;

    sequencerPtr = (sequencer_t*)P_MALLOC(sizeof(sequencer_t));
    if (sequencerPtr == NULL) {
        return NULL;
    }

    sequencerPtr->uniqueSegmentsPtr =
        hashtable_alloc(geneLength, &hashSegment, &compareSegment, -1, -1);
    if (sequencerPtr->uniqueSegmentsPtr == NULL) {
        return NULL;
    }

    /* For finding a matching entry */
    sequencerPtr->endInfoEntries =
        (endInfoEntry_t*)P_MALLOC(maxNumUniqueSegment * sizeof(endInfoEntry_t));
    for (i = 0; i < maxNumUniqueSegment; i++) {
        endInfoEntry_t* endInfoEntryPtr = &sequencerPtr->endInfoEntries[i];
        endInfoEntryPtr->isEnd = TRUE;
        endInfoEntryPtr->jumpToNext = 1;
    }
    sequencerPtr->startHashToConstructEntryTables =
        (table_t**)P_MALLOC(segmentLength * sizeof(table_t*));
    if (sequencerPtr->startHashToConstructEntryTables == NULL) {
        return NULL;
    }
    for (i = 1; i < segmentLength; i++) { /* 0 is dummy entry */
        sequencerPtr->startHashToConstructEntryTables[i] =
            table_alloc(geneLength, NULL);
        if (sequencerPtr->startHashToConstructEntryTables[i] == NULL) {
            return NULL;
        }
    }
    sequencerPtr->segmentLength = segmentLength;
    sequencerPtr->geneLength = geneLength;

    /* For constructing sequence */
    sequencerPtr->constructEntries =
        (constructEntry_t*)P_MALLOC(maxNumUniqueSegment * sizeof(constructEntry_t));
    if (sequencerPtr->constructEntries == NULL) {
        return NULL;
    }
    for (i= 0; i < maxNumUniqueSegment; i++) {
        constructEntry_t* constructEntryPtr = &sequencerPtr->constructEntries[i];
        constructEntryPtr->isStart = TRUE;
        constructEntryPtr->segment = NULL;
        constructEntryPtr->endHash = 0;
        constructEntryPtr->startPtr = constructEntryPtr;
        constructEntryPtr->nextPtr = NULL;
        constructEntryPtr->endPtr = constructEntryPtr;
        constructEntryPtr->overlap = 0;
        constructEntryPtr->length = segmentLength;
#ifdef FINE_LOCK
        LOCK_INIT(constructEntryPtr->lock);
#endif
    }
    sequencerPtr->hashToConstructEntryTable = table_alloc(geneLength, NULL);
    if (sequencerPtr->hashToConstructEntryTable == NULL) {
        return NULL;
    }

    sequencerPtr->segmentsPtr = segmentsPtr;

    return sequencerPtr;
}

#ifndef FINE_LOCK

/* =============================================================================
 * sequencer_run
 * =============================================================================
 */
void
sequencer_run (void* argPtr)
{
    TM_THREAD_ENTER();

    long threadId = thread_getId();

    sequencer_t* sequencerPtr = (sequencer_t*)argPtr;

    hashtable_t*      uniqueSegmentsPtr;
    endInfoEntry_t*   endInfoEntries;
    table_t**         startHashToConstructEntryTables;
    constructEntry_t* constructEntries;
    table_t*          hashToConstructEntryTable;

    uniqueSegmentsPtr               = sequencerPtr->uniqueSegmentsPtr;
    endInfoEntries                  = sequencerPtr->endInfoEntries;
    startHashToConstructEntryTables = sequencerPtr->startHashToConstructEntryTables;
    constructEntries                = sequencerPtr->constructEntries;
    hashToConstructEntryTable       = sequencerPtr->hashToConstructEntryTable;

    segments_t* segmentsPtr         = sequencerPtr->segmentsPtr;
    assert(segmentsPtr);
    vector_t*   segmentsContentsPtr = segmentsPtr->contentsPtr;
    long        numSegment          = vector_getSize(segmentsContentsPtr);
    long        segmentLength       = segmentsPtr->length;
    long        geneLength          = sequencerPtr->geneLength;

    long i;
    long j;
    long i_start;
    long i_stop;
    long numUniqueSegment;
    long substringLength;
    long entryIndex;

    /*
     * Step 1: Remove duplicate segments
     */
    long numThread = thread_getNumThread();
    {
        /* Choose disjoint segments [i_start, i_stop) for each thread */
        i_start = threadId * numSegment / numThread;
        i_stop = (threadId + 1) * numSegment / numThread;

        assert(i_start <= numSegment && i_stop <= numSegment);
    }

#ifdef USE_LOCAL_HASHTABLE
    hashtable_t* localHashtablePtr;
    localHashtablePtr =
        hashtable_alloc(geneLength, &hashSegment, &compareSegment, -1, -1);
    assert(localHashtablePtr);
    for (i = i_start; i < i_stop; ++i) {
        void* segment = vector_at(segmentsContentsPtr, i);
        hashtable_insert(localHashtablePtr, segment, segment);
    }

    {
        list_t* chainPtr;
        list_iter_t it;
        long num = localHashtablePtr->numBucket;
        for (i = 0; i < num; ++i) {
            chainPtr = localHashtablePtr->buckets[i];
            list_iter_reset(&it, chainPtr);

            while (list_iter_hasNext(&it, chainPtr)) {
                void* segment =
                    (void*)((pair_t*)list_iter_next(&it, chainPtr))->firstPtr;
                TM_BEGIN();
                {
                    TMHASHTABLE_INSERT(uniqueSegmentsPtr, segment, segment);
                }
                TM_END();
            }
        }

        hashtable_free(localHashtablePtr);
    }
#else
    for (i = i_start; i < i_stop; i+=CHUNK_STEP1) {
        TM_BEGIN();
        {
            long ii;
            long ii_stop = MIN(i_stop, (i+CHUNK_STEP1));
            for (ii = i; ii < ii_stop; ii++) {
                void* segment = vector_at(segmentsContentsPtr, ii);
                TMHASHTABLE_INSERT(uniqueSegmentsPtr,
                                   segment,
                                   segment);
            } /* ii */
        }
        TM_END();
    }
#endif

    thread_barrier_wait();

    /*
     * Step 2a: Iterate over unique segments and compute hashes.
     *
     * For the gene "atcg", the hashes for the end would be:
     *
     *     "t", "tc", and "tcg"
     *
     * And for the gene "tcgg", the hashes for the start would be:
     *
     *    "t", "tc", and "tcg"
     *
     * The names are "end" and "start" because if a matching pair is found,
     * they are the substring of the end part of the pair and the start
     * part of the pair respectively. In the above example, "tcg" is the
     * matching substring so:
     *
     *     (end)    (start)
     *     a[tcg] + [tcg]g  = a[tcg]g    (overlap = "tcg")
     */

    /* uniqueSegmentsPtr is constant now */
    numUniqueSegment = hashtable_getSize(uniqueSegmentsPtr);
    entryIndex = 0;

    if (threadId == 0) {
        entryIndex = 0;
        for (i = 0; i < uniqueSegmentsPtr->numBucket; ++i) {
            list_t* chainPtr = uniqueSegmentsPtr->buckets[i];
            list_iter_t it;
            list_iter_reset(&it, chainPtr);

            while (list_iter_hasNext(&it, chainPtr)) {
                char* segment =
                    (char*)((pair_t*)list_iter_next(&it, chainPtr))->firstPtr;
                constructEntries[entryIndex].segment = segment;
                ++entryIndex;
            }
        }
    }

    thread_barrier_wait();

    {
        /* Choose disjoint segments [i_start,i_stop) for each thread */
        i_start = threadId * numUniqueSegment / numThread;
        i_stop = (threadId + 1) * numUniqueSegment / numThread;

        assert(i_start <= numUniqueSegment && i_stop <= numUniqueSegment);
    }

    for (i = i_start; i < i_stop; i++) {
        long j;
        constructEntry_t* constructEntryPtr;
        char* segment;
        ulong_t startHash;
        bool_t status;

        constructEntryPtr = &constructEntries[i];
        segment = constructEntryPtr->segment;

        /*
         * Save hashes (sdbm algorithm) of segment substrings
         *
         * endHashes will be computed for shorter substrings after matches
         * have been made (in the next phase of the code). This will reduce
         * the number of substrings for which hashes need to be computed.
         *
         * Since we can compute startHashes incrementally, we go ahead
         * and compute all of them here.
         */
        /* constructEntryPtr is local now */
        constructEntryPtr->endHash = (ulong_t)hashString(&segment[1]);

        startHash = 0;
#ifdef CHUNK_STEP2A
        for (j = 1; j < segmentLength; j+=CHUNK_STEP2A) {
            long temp_hash = startHash;
            TM_BEGIN();
            {
                startHash = temp_hash;
                long jj;
                long jj_stop = MIN(segmentLength, j+CHUNK_STEP2A);
                for (jj = j; jj < jj_stop; ++jj) {
                    startHash = (ulong_t)segment[jj-1] +
                                (startHash << 6) + (startHash << 16) - startHash;
                    status = TMTABLE_INSERT(startHashToConstructEntryTables[jj],
                                            (ulong_t)startHash,
                                            (void*)constructEntryPtr);
                    assert(status);
                } /* ii */
            }
            TM_END();
        }
#else
        for (j = 1; j < segmentLength; j++) {
            startHash = (ulong_t)segment[j-1] +
                        (startHash << 6) + (startHash << 16) - startHash;
            TM_BEGIN();
            status = TMTABLE_INSERT(startHashToConstructEntryTables[j],
                                    (ulong_t)startHash,
                                    (void*)constructEntryPtr);
            TM_END();
            assert(status);
        }
#endif

        /*
         * For looking up construct entries quickly
         */
        /* This process seems futile. Therefore, I commented it out. */
#if 0
        startHash = (ulong_t)segment[j-1] +
                    (startHash << 6) + (startHash << 16) - startHash;
        TM_BEGIN();
        status = TMTABLE_INSERT(hashToConstructEntryTable,
                                (ulong_t)startHash,
                                (void*)constructEntryPtr);
        TM_END();
        assert(status);
#endif
    }

    thread_barrier_wait();

    /*
     * Step 2b: Match ends to starts by using hash-based string comparison.
     */
    for (substringLength = segmentLength-1; substringLength > 0; substringLength--) {

        table_t* startHashToConstructEntryTablePtr =
            startHashToConstructEntryTables[substringLength];
        list_t** buckets = startHashToConstructEntryTablePtr->buckets;
        long numBucket = startHashToConstructEntryTablePtr->numBucket;

        long index_start;
        long index_stop;

        {
            /* Choose disjoint segments [index_start, index_stop) for each thread */
            index_start = threadId * numUniqueSegment / numThread;
            index_stop = (threadId + 1) * numUniqueSegment / numThread;

            assert(index_start <= numUniqueSegment && index_stop <= numUniqueSegment);
        }

        /* Iterating over disjoint itervals in the range [0, numUniqueSegment) */
        for (entryIndex = index_start;
             entryIndex < index_stop;
             entryIndex += endInfoEntries[entryIndex].jumpToNext)
        {
            if (!endInfoEntries[entryIndex].isEnd) {
                continue;
            }

            /*  ConstructEntries[entryIndex] is local data */
            constructEntry_t* endConstructEntryPtr =
                &constructEntries[entryIndex];
            char* endSegment = endConstructEntryPtr->segment;
            ulong_t endHash = endConstructEntryPtr->endHash;

            list_t* chainPtr = buckets[endHash % numBucket]; /* buckets: constant data */
            list_iter_t it;
            list_iter_reset(&it, chainPtr);

            /* Linked list at chainPtr is constant */
            while (list_iter_hasNext(&it, chainPtr)) {

                constructEntry_t* startConstructEntryPtr =
                    (constructEntry_t*)list_iter_next(&it, chainPtr);
                char* startSegment = startConstructEntryPtr->segment;
                long newLength = 0;

                /* endConstructEntryPtr is local except for properties startPtr/endPtr/length */
                TM_BEGIN();

                /* Check if matches */
                if (TM_SHARED_READ(startConstructEntryPtr->isStart) &&
                    (TM_SHARED_READ_P(endConstructEntryPtr->startPtr) != startConstructEntryPtr) &&
                    (strncmp(startSegment,
                             &endSegment[segmentLength - substringLength],
                             substringLength) == 0))
                {
                    TM_SHARED_WRITE(startConstructEntryPtr->isStart, FALSE);

                    constructEntry_t* startConstructEntry_endPtr;
                    constructEntry_t* endConstructEntry_startPtr;

                    /* Update endInfo (appended something so no longer end) */
                    TM_LOCAL_WRITE(endInfoEntries[entryIndex].isEnd, FALSE);

                    /* Update segment chain construct info */
                    startConstructEntry_endPtr =
                        (constructEntry_t*)TM_SHARED_READ_P(startConstructEntryPtr->endPtr);
                    endConstructEntry_startPtr =
                        (constructEntry_t*)TM_SHARED_READ_P(endConstructEntryPtr->startPtr);

                    assert(startConstructEntry_endPtr);
                    assert(endConstructEntry_startPtr);
                    TM_SHARED_WRITE_P(startConstructEntry_endPtr->startPtr,
                                      endConstructEntry_startPtr);
                    TM_LOCAL_WRITE_P(endConstructEntryPtr->nextPtr,
                                     startConstructEntryPtr);
                    TM_SHARED_WRITE_P(endConstructEntry_startPtr->endPtr,
                                      startConstructEntry_endPtr);
                    TM_SHARED_WRITE(endConstructEntryPtr->overlap, substringLength);
                    newLength = (long)TM_SHARED_READ(endConstructEntry_startPtr->length) +
                                (long)TM_SHARED_READ(startConstructEntryPtr->length) -
                                substringLength;
                    TM_SHARED_WRITE(endConstructEntry_startPtr->length, newLength);
                } /* if (matched) */

                TM_END();

                if (!endInfoEntries[entryIndex].isEnd) { /* if there was a match */
                    break;
                }
            } /* iterate over chain */

        } /* for (endIndex < numUniqueSegment) */

        thread_barrier_wait();

        /*
         * Step 2c: Update jump values and hashes
         *
         * endHash entries of all remaining ends are updated to the next
         * substringLength. Additionally jumpToNext entries are updated such
         * that they allow to skip non-end entries. Currently this is sequential
         * because parallelization did not perform better.
.        */

        if (threadId == 0) {
            if (substringLength > 1) {
                long index = segmentLength - substringLength + 1;
                /* initialization if j and i: with i being the next end after j=0 */
                for (i = 1; !endInfoEntries[i].isEnd; i+=endInfoEntries[i].jumpToNext) {
                    /* find first non-null */
                }
                /* entry 0 is handled seperately from the loop below */
                endInfoEntries[0].jumpToNext = i;
                if (endInfoEntries[0].isEnd) {
                    constructEntry_t* constructEntryPtr = &constructEntries[0];
                    char* segment = constructEntryPtr->segment;
                    constructEntryPtr->endHash = (ulong_t)hashString(&segment[index]);
                }
                /* Continue scanning (do not reset i) */
                for (j = 0; i < numUniqueSegment; i+=endInfoEntries[i].jumpToNext) {
                    if (endInfoEntries[i].isEnd) {
                        constructEntry_t* constructEntryPtr = &constructEntries[i];
                        char* segment = constructEntryPtr->segment;
                        constructEntryPtr->endHash = (ulong_t)hashString(&segment[index]);
                        endInfoEntries[j].jumpToNext = MAX(1, (i - j));
                        j = i;
                    }
                }
                endInfoEntries[j].jumpToNext = i - j;
            }
        }

        thread_barrier_wait();

    } /* for (substringLength > 0) */


    thread_barrier_wait();

    /*
     * Step 3: Build sequence string
     */
    if (threadId == 0) {

        long totalLength = 0;

        for (i = 0; i < numUniqueSegment; i++) {
            constructEntry_t* constructEntryPtr = &constructEntries[i];
            if (constructEntryPtr->isStart) {
              totalLength += constructEntryPtr->length;
            }
        }

        sequencerPtr->sequence = (char*)P_MALLOC((totalLength+1) * sizeof(char));
        char* sequence = sequencerPtr->sequence;
        assert(sequence);

        char* copyPtr = sequence;
        long sequenceLength = 0;

        for (i = 0; i < numUniqueSegment; i++) {
            constructEntry_t* constructEntryPtr = &constructEntries[i];
            /* If there are several start segments, we append in arbitrary order  */
            if (constructEntryPtr->isStart) {
                long newSequenceLength = sequenceLength + constructEntryPtr->length;
                assert( newSequenceLength <= totalLength );
                copyPtr = sequence + sequenceLength;
                sequenceLength = newSequenceLength;
                do {
                    long numChar = segmentLength - constructEntryPtr->overlap;
                    if ((copyPtr + numChar) > (sequence + newSequenceLength)) {
                        TM_PRINT0("ERROR: sequence length != actual length\n");
                        break;
                    }
                    memcpy(copyPtr,
                           constructEntryPtr->segment,
                           (numChar * sizeof(char)));
                    copyPtr += numChar;
                } while ((constructEntryPtr = constructEntryPtr->nextPtr) != NULL);
                assert(copyPtr <= (sequence + sequenceLength));
            }
        }

        assert(sequence != NULL);
        sequence[sequenceLength] = '\0';
    }

    TM_THREAD_EXIT();
}

#else /* FINE_LOCK */

/* =============================================================================
 * sequencer_run
 * =============================================================================
 */
void
sequencer_run (void* argPtr)
{
    TM_THREAD_ENTER();

    long threadId = thread_getId();

    sequencer_t* sequencerPtr = (sequencer_t*)argPtr;

    hashtable_t*      uniqueSegmentsPtr;
    endInfoEntry_t*   endInfoEntries;
    table_t**         startHashToConstructEntryTables;
    constructEntry_t* constructEntries;
    table_t*          hashToConstructEntryTable;

    uniqueSegmentsPtr               = sequencerPtr->uniqueSegmentsPtr;
    endInfoEntries                  = sequencerPtr->endInfoEntries;
    startHashToConstructEntryTables = sequencerPtr->startHashToConstructEntryTables;
    constructEntries                = sequencerPtr->constructEntries;
    hashToConstructEntryTable       = sequencerPtr->hashToConstructEntryTable;

    segments_t* segmentsPtr         = sequencerPtr->segmentsPtr;
    assert(segmentsPtr);
    vector_t*   segmentsContentsPtr = segmentsPtr->contentsPtr;
    long        numSegment          = vector_getSize(segmentsContentsPtr);
    long        segmentLength       = segmentsPtr->length;
    long        geneLength          = sequencerPtr->geneLength;

    long i;
    long j;
    long i_start;
    long i_stop;
    long numUniqueSegment;
    long substringLength;
    long entryIndex;

    /*
     * Step 1: Remove duplicate segments
     */
    long numThread = thread_getNumThread();
    {
        /* Choose disjoint segments [i_start, i_stop) for each thread */
        i_start = threadId * numSegment / numThread;
        i_stop = (threadId + 1) * numSegment / numThread;

        assert(i_start <= numSegment && i_stop <= numSegment);
    }

#ifdef LOCAL_HASHTABLE
    hashtable_t* localHashtablePtr;
    localHashtablePtr =
        hashtable_alloc(geneLength, &hashSegment, &compareSegment, -1, -1);
    assert(localHashtablePtr);
    for (i = i_start; i < i_stop; ++i) {
        void* segment = vector_at(segmentsContentsPtr, i);
        hashtable_insert(localHashtablePtr, segment, segment);
    }

    {
        list_t* chainPtr;
        list_iter_t it;
        ulong_t lock_idx;
        long num = localHashtablePtr->numBucket;
        for (i = 0; i < num; ++i) {
            chainPtr = localHashtablePtr->buckets[i];
            list_iter_reset(&it, chainPtr);

            while (list_iter_hasNext(&it, chainPtr)) {
                void* segment =
                    (void*)((pair_t*)list_iter_next(&it, chainPtr))->firstPtr;
                lock_idx = uniqueSegmentsPtr->hash(segment)
                                        % uniqueSegmentsPtr->numBucket;
                LOCK(lock_step1[lock_idx]);
                {
                    hashtable_insert(uniqueSegmentsPtr, segment, segment);
                }
                UNLOCK(lock_step1[lock_idx]);
            }
        }

        hashtable_free(localHashtablePtr);
    }
#else
    for (i = i_start; i < i_stop; i++) {
        void* segment = vector_at(segmentsContentsPtr, i);
        ulong_t lock_idx = uniqueSegmentsPtr->hash(segment)
                                % uniqueSegmentsPtr->numBucket;
        LOCK(lock_step1[lock_idx]);
        hashtable_insert(uniqueSegmentsPtr, segment, segment);
        UNLOCK(lock_step1[lock_idx]);
    }
#endif

    thread_barrier_wait();

    /*
     * Step 2a: Iterate over unique segments and compute hashes.
     *
     * For the gene "atcg", the hashes for the end would be:
     *
     *     "t", "tc", and "tcg"
     *
     * And for the gene "tcgg", the hashes for the start would be:
     *
     *    "t", "tc", and "tcg"
     *
     * The names are "end" and "start" because if a matching pair is found,
     * they are the substring of the end part of the pair and the start
     * part of the pair respectively. In the above example, "tcg" is the
     * matching substring so:
     *
     *     (end)    (start)
     *     a[tcg] + [tcg]g  = a[tcg]g    (overlap = "tcg")
     */
#ifdef CHUNK_STEP2A
        if (threadId == 0) {
            printf("\nIf fine-grained lock is used,"
                   "Step 2a is not processed in chunks.\n");
        }
#endif

    /* uniqueSegmentsPtr is constant now */
    numUniqueSegment = hashtable_getSize(uniqueSegmentsPtr);
    entryIndex = 0;

    if (threadId == 0) {
        entryIndex = 0;
        for (i = 0; i < uniqueSegmentsPtr->numBucket; ++i) {
            list_t* chainPtr = uniqueSegmentsPtr->buckets[i];
            list_iter_t it;
            list_iter_reset(&it, chainPtr);

            while (list_iter_hasNext(&it, chainPtr)) {
                char* segment =
                    (char*)((pair_t*)list_iter_next(&it, chainPtr))->firstPtr;
                constructEntries[entryIndex].segment = segment;
                ++entryIndex;
            }
        }
    }

    thread_barrier_wait();

    {
        /* Choose disjoint segments [i_start,i_stop) for each thread */
        i_start = threadId * numUniqueSegment / numThread;
        i_stop = (threadId + 1) * numUniqueSegment / numThread;

        assert(i_start <= numUniqueSegment && i_stop <= numUniqueSegment);
    }

    for (i = i_start; i < i_stop; i++) {
        constructEntry_t* constructEntryPtr;
        char* segment;
        long j;
        ulong_t startHash;
        bool_t status;
        ulong_t lock_idx;

        constructEntryPtr = &constructEntries[i];
        segment = constructEntryPtr->segment;

        /*
         * Save hashes (sdbm algorithm) of segment substrings
         *
         * endHashes will be computed for shorter substrings after matches
         * have been made (in the next phase of the code). This will reduce
         * the number of substrings for which hashes need to be computed.
         *
         * Since we can compute startHashes incrementally, we go ahead
         * and compute all of them here.
         */
        /* constructEntryPtr is local now */
        constructEntryPtr->endHash = (ulong_t)hashString(&segment[1]);

        startHash = 0;
        for (j = 1; j < segmentLength; j++) {
            startHash = (ulong_t)segment[j-1] +
                        (startHash << 6) + (startHash << 16) - startHash;
            lock_idx = startHash % startHashToConstructEntryTables[j]->numBucket;
            LOCK(lock_step2a[j][lock_idx]);
            status = table_insert(startHashToConstructEntryTables[j],
                                  (ulong_t)startHash,
                                  (void*)constructEntryPtr);
            UNLOCK(lock_step2a[j][lock_idx]);
            assert(status);
        }

        /*
         * For looking up construct entries quickly
         */
        /* This process seems futile. Therefore, I commented it out. */
#if 0
        startHash = (ulong_t)segment[j-1] +
                    (startHash << 6) + (startHash << 16) - startHash;
        TM_BEGIN();
        status = TMTABLE_INSERT(hashToConstructEntryTable,
                                (ulong_t)startHash,
                                (void*)constructEntryPtr);
        TM_END();
        assert(status);
#endif
    }

    thread_barrier_wait();

    /*
     * Step 2b: Match ends to starts by using hash-based string comparison.
     */
    for (substringLength = segmentLength-1; substringLength > 0; substringLength--) {

        table_t* startHashToConstructEntryTablePtr =
            startHashToConstructEntryTables[substringLength];
        list_t** buckets = startHashToConstructEntryTablePtr->buckets;
        long numBucket = startHashToConstructEntryTablePtr->numBucket;

        long index_start;
        long index_stop;

        {
            /* Choose disjoint segments [index_start, index_stop) for each thread */
            index_start = threadId * numUniqueSegment / numThread;
            index_stop = (threadId + 1) * numUniqueSegment / numThread;

            assert(index_start <= numUniqueSegment && index_stop <= numUniqueSegment);
        }

        /* Iterating over disjoint itervals in the range [0, numUniqueSegment) */
        for (entryIndex = index_start;
             entryIndex < index_stop;
             entryIndex += endInfoEntries[entryIndex].jumpToNext)
        {
            if (!endInfoEntries[entryIndex].isEnd) {
                continue;
            }

            /*  ConstructEntries[entryIndex] is local data */
            constructEntry_t* endConstructEntryPtr =
                &constructEntries[entryIndex];

            constructEntry_t* endConstructEntryPtr_startPtr;
            while (1) {
                endConstructEntryPtr_startPtr = endConstructEntryPtr->startPtr;
                LOCK(endConstructEntryPtr_startPtr->lock);
                if (endConstructEntryPtr_startPtr == endConstructEntryPtr->startPtr) {
                    break;
                }
                UNLOCK(endConstructEntryPtr_startPtr->lock);
            }

            char* endSegment = endConstructEntryPtr->segment;
            ulong_t endHash = endConstructEntryPtr->endHash;

            list_t* chainPtr = buckets[endHash % numBucket]; /* buckets: constant data */
            list_iter_t it;
            list_iter_reset(&it, chainPtr);

            bool_t isLock_startCEP;

            /* Linked list at chainPtr is constant */
            while (list_iter_hasNext(&it, chainPtr)) {
                isLock_startCEP = FALSE;

                constructEntry_t* startConstructEntryPtr =
                    (constructEntry_t*)list_iter_next(&it, chainPtr);
                char* startSegment = startConstructEntryPtr->segment;
                long newLength = 0;

                if (endConstructEntryPtr_startPtr == startConstructEntryPtr ||
                    (strncmp(startSegment,
                             &endSegment[segmentLength - substringLength],
                             substringLength) != 0))
                {
                    continue;
                }

                if ((startConstructEntryPtr != endConstructEntryPtr) &&
                    (startConstructEntryPtr != endConstructEntryPtr_startPtr))
                {
                    LOCK(startConstructEntryPtr->lock);
                    isLock_startCEP = TRUE;
                }

                /* Check if matches */
                if (startConstructEntryPtr->isStart) {
                    startConstructEntryPtr->isStart = FALSE;

                    constructEntry_t* startConstructEntry_endPtr;
                    constructEntry_t* endConstructEntry_startPtr;

                    /* Update endInfo (appended something so no longer end) */
                    endInfoEntries[entryIndex].isEnd = FALSE;

                    /* Update segment chain construct info */
                    startConstructEntry_endPtr =
                        (constructEntry_t*)startConstructEntryPtr->endPtr;
                    endConstructEntry_startPtr =
                        (constructEntry_t*)endConstructEntryPtr->startPtr;

                    assert(startConstructEntry_endPtr);
                    assert(endConstructEntry_startPtr);
                    startConstructEntry_endPtr->startPtr = endConstructEntry_startPtr;
                    endConstructEntryPtr->nextPtr = startConstructEntryPtr;
                    endConstructEntry_startPtr->endPtr = startConstructEntry_endPtr;
                    endConstructEntryPtr->overlap = substringLength;
                    newLength = (long)endConstructEntry_startPtr->length +
                                (long)startConstructEntryPtr->length -
                                substringLength;
                    endConstructEntry_startPtr->length = newLength;
                } /* if (matched) */

                if (isLock_startCEP) {
                    UNLOCK(startConstructEntryPtr->lock);
                }

                if (!endInfoEntries[entryIndex].isEnd) { /* if there was a match */
                    break;
                }
            } /* iterate over chain */

            UNLOCK(endConstructEntryPtr_startPtr->lock);
        } /* for (endIndex < numUniqueSegment) */

        thread_barrier_wait();

        /*
         * Step 2c: Update jump values and hashes
         *
         * endHash entries of all remaining ends are updated to the next
         * substringLength. Additionally jumpToNext entries are updated such
         * that they allow to skip non-end entries. Currently this is sequential
         * because parallelization did not perform better.
.        */

        if (threadId == 0) {
            if (substringLength > 1) {
                long index = segmentLength - substringLength + 1;
                /* initialization if j and i: with i being the next end after j=0 */
                for (i = 1; !endInfoEntries[i].isEnd; i+=endInfoEntries[i].jumpToNext) {
                    /* find first non-null */
                }
                /* entry 0 is handled seperately from the loop below */
                endInfoEntries[0].jumpToNext = i;
                if (endInfoEntries[0].isEnd) {
                    constructEntry_t* constructEntryPtr = &constructEntries[0];
                    char* segment = constructEntryPtr->segment;
                    constructEntryPtr->endHash = (ulong_t)hashString(&segment[index]);
                }
                /* Continue scanning (do not reset i) */
                for (j = 0; i < numUniqueSegment; i+=endInfoEntries[i].jumpToNext) {
                    if (endInfoEntries[i].isEnd) {
                        constructEntry_t* constructEntryPtr = &constructEntries[i];
                        char* segment = constructEntryPtr->segment;
                        constructEntryPtr->endHash = (ulong_t)hashString(&segment[index]);
                        endInfoEntries[j].jumpToNext = MAX(1, (i - j));
                        j = i;
                    }
                }
                endInfoEntries[j].jumpToNext = i - j;
            }
        }

        thread_barrier_wait();

    } /* for (substringLength > 0) */


    thread_barrier_wait();

    /*
     * Step 3: Build sequence string
     */
    if (threadId == 0) {

        long totalLength = 0;

        for (i = 0; i < numUniqueSegment; i++) {
            constructEntry_t* constructEntryPtr = &constructEntries[i];
            if (constructEntryPtr->isStart) {
              totalLength += constructEntryPtr->length;
            }
        }

        sequencerPtr->sequence = (char*)P_MALLOC((totalLength+1) * sizeof(char));
        char* sequence = sequencerPtr->sequence;
        assert(sequence);

        char* copyPtr = sequence;
        long sequenceLength = 0;

        for (i = 0; i < numUniqueSegment; i++) {
            constructEntry_t* constructEntryPtr = &constructEntries[i];
            /* If there are several start segments, we append in arbitrary order  */
            if (constructEntryPtr->isStart) {
                long newSequenceLength = sequenceLength + constructEntryPtr->length;
                assert( newSequenceLength <= totalLength );
                copyPtr = sequence + sequenceLength;
                sequenceLength = newSequenceLength;
                do {
                    long numChar = segmentLength - constructEntryPtr->overlap;
                    if ((copyPtr + numChar) > (sequence + newSequenceLength)) {
                        TM_PRINT0("ERROR: sequence length != actual length\n");
                        break;
                    }
                    memcpy(copyPtr,
                           constructEntryPtr->segment,
                           (numChar * sizeof(char)));
                    copyPtr += numChar;
                } while ((constructEntryPtr = constructEntryPtr->nextPtr) != NULL);
                assert(copyPtr <= (sequence + sequenceLength));
            }
        }

        assert(sequence != NULL);
        sequence[sequenceLength] = '\0';
    }

    TM_THREAD_EXIT();
}

#endif

/* =============================================================================
 * sequencer_free
 * =============================================================================
 */
void
sequencer_free (sequencer_t* sequencerPtr)
{
    long i;

    table_free(sequencerPtr->hashToConstructEntryTable);
    P_FREE(sequencerPtr->constructEntries);
    for (i = 1; i < sequencerPtr->segmentLength; i++) {
        table_free(sequencerPtr->startHashToConstructEntryTables[i]);
    }
    P_FREE(sequencerPtr->startHashToConstructEntryTables);
    P_FREE(sequencerPtr->endInfoEntries);
#if 0
    /* TODO: fix mixed sequential/parallel allocation */
    hashtable_free(sequencerPtr->uniqueSegmentsPtr);
    if (sequencerPtr->sequence != NULL) {
        P_FREE(sequencerPtr->sequence);
    }
#endif
    P_FREE(sequencerPtr);
}


/* =============================================================================
 * TEST_SEQUENCER
 * =============================================================================
 */
#ifdef TEST_SEQUENCER


#include <assert.h>
#include <stdio.h>
#include "segments.h"


char* gene1 = "gatcggcagc";
char* segments1[] = {
    "atcg",
    "gcag",
    "tcgg",
    "cagc",
    "gatc",
    NULL
};

char* gene2 = "aaagc";
char* segments2[] = {
    "aaa",
    "aag",
    "agc",
    NULL
};

char* gene3 = "aaacaaagaaat";
char* segments3[] = {
    "aaac",
    "aaag",
    "aaat",
    NULL
};

char* gene4 = "ttggctacgtatcgcacggt";
char* segments4[] = {
    "cgtatcgc",
    "tcgcacgg",
    "gtatcgca",
    "tatcgcac",
    "atcgcacg",
    "ttggctac",
    "ctacgtat",
    "acgtatcg",
    "ctacgtat",
    "cgtatcgc",
    "atcgcacg",
    "ggctacgt",
    "tacgtatc",
    "tcgcacgg",
    "ttggctac",
    "ggctacgt",
    "atcgcacg",
    "tatcgcac",
    "cgtatcgc",
    "acgtatcg",
    "gtatcgca",
    "gtatcgca",
    "cgcacggt",
    "tatcgcac",
    "ttggctac",
    "atcgcacg",
    "acgtatcg",
    "gtatcgca",
    "ttggctac",
    "tggctacg",
    NULL
};

char* gene5 = "gatcggcagctggtacggcg";
char* segments5[] = {
    "atcggcag",
    "gtacggcg",
    "gatcggca",
    "cagctggt",
    "tggtacgg",
    "gatcggca",
    "gatcggca",
    "tcggcagc",
    "ggtacggc",
    "tggtacgg",
    "tcggcagc",
    "gcagctgg",
    "gatcggca",
    "gctggtac",
    "gatcggca",
    "ctggtacg",
    "ggcagctg",
    "tcggcagc",
    "gtacggcg",
    "gcagctgg",
    "ggcagctg",
    "tcggcagc",
    "cagctggt",
    "tggtacgg",
    "cagctggt",
    "gcagctgg",
    "gctggtac",
    "cggcagct",
    "agctggta",
    "ctggtacg",
    NULL
};

char* gene6 = "ttggtgagccgtaagactcc";
char* segments6[] = {
    "cgtaagac",
    "taagactc",
    "gtgagccg",
    "gagccgta",
    "gccgtaag",
    "tgagccgt",
    "gccgtaag",
    "cgtaagac",
    "ttggtgag",
    "agccgtaa",
    "gccgtaag",
    "aagactcc",
    "ggtgagcc",
    "ttggtgag",
    "agccgtaa",
    "gagccgta",
    "aagactcc",
    "ttggtgag",
    "gtaagact",
    "ccgtaaga",
    "ttggtgag",
    "gagccgta",
    "ggtgagcc",
    "gagccgta",
    "gccgtaag",
    "aagactcc",
    "gtaagact",
    "ccgtaaga",
    "tgagccgt",
    "ttggtgag",
    NULL
};

char* gene7 = "gatcggcagctggtacggcg";
char* segments7[] = {
    "atcggcag",
    "gtacggcg",
    "gatcggca",
    "cagctggt",
    "tggtacgg",
    "gatcggca",
    "gatcggca",
    "tcggcagc",
    "ggtacggc",
    "tggtacgg",
    "tcggcagc",
    "gcagctgg",
    "gatcggca",
    "gctggtac",
    "gatcggca",
    "ctggtacg",
    "ggcagctg",
    "tcggcagc",
    "gtacggcg",
    "gcagctgg",
    "ggcagctg",
    "tcggcagc",
    "cagctggt",
    "tggtacgg",
    "cagctggt",
    "gcagctgg",
    "gctggtac",
    "cggcagct",
    "agctggta",
    "ctggtacg",
    NULL
};

char* gene8 = "ttggtgagccgtaagactcc";
char* segments8[] = {
    "cgtaagac",
    "taagactc",
    "gtgagccg",
    "gagccgta",
    "gccgtaag",
    "tgagccgt",
    "gccgtaag",
    "cgtaagac",
    "ttggtgag",
    "agccgtaa",
    "gccgtaag",
    "aagactcc",
    "ggtgagcc",
    "ttggtgag",
    "agccgtaa",
    "gagccgta",
    "aagactcc",
    "ttggtgag",
    "gtaagact",
    "ccgtaaga",
    "ttggtgag",
    "gagccgta",
    "ggtgagcc",
    "gagccgta",
    "gccgtaag",
    "aagactcc",
    "gtaagact",
    "ccgtaaga",
    "tgagccgt",
    "ttggtgag",
    NULL
};


static segments_t*
createSegments (char* segments[])
{
    long i = 0;
    segments_t* segmentsPtr = (segments_t*)malloc(sizeof(segments));

    segmentsPtr->length = strlen(segments[0]);
    segmentsPtr->contentsPtr = vector_alloc(1);

    while (segments[i] != NULL) {
        bool_t status = vector_pushBack(segmentsPtr->contentsPtr,
                                        (void*)segments[i]);
        assert(status);
        i++;
    }

    segmentsPtr->minNum = vector_getSize(segmentsPtr->contentsPtr);

    return segmentsPtr;
}


static void
tester (char* gene, char* segments[])
{
    segments_t* segmentsPtr;
    sequencer_t* sequencerPtr;

    segmentsPtr = createSegments(segments);
    sequencerPtr = sequencer_alloc(strlen(gene), segmentsPtr->length, segmentsPtr);

    sequencer_run((void*)sequencerPtr);

    printf("gene     = %s\n", gene);
    printf("sequence = %s\n", sequencerPtr->sequence);
    assert(strcmp(sequencerPtr->sequence, gene) == 0);

    sequencer_free(sequencerPtr);
}


int
main ()
{
    bool_t status = memory_init(1, 4, 2);
    assert(status);
    thread_startup(1);

    puts("Starting...");

    /* Simple test */
    tester(gene1, segments1);

    /* Simple test with aliasing segments */
    tester(gene2, segments2);

    /* Simple test with non-overlapping segments */
    tester(gene3, segments3);

    /* Complex tests */
    tester(gene4, segments4);
    tester(gene5, segments5);
    tester(gene6, segments6);
    tester(gene7, segments7);
    tester(gene8, segments8);

    puts("Passed all tests.");

    return 0;
}


#endif /* TEST_SEQUENCER */


/* =============================================================================
 *
 * End of sequencer.c
 *
 * =============================================================================
 */
