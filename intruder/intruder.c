/* =============================================================================
 *
 * intruder.c
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "decoder.h"
#include "detector.h"
#include "dictionary.h"
#include "packet.h"
#include "stream.h"
#include "thread.h"
#include "timer.h"
#include "tm.h"
#include "map.h"
#include "utility.h"

#define CHUNK_STREAM 256
#define CHUNK_DECODE 16

#ifdef FINE_LOCK
LOCK_T stream_lock;
LOCK_T* table_locks;

#undef CHUNK_SIZE
#define CHUNK_SIZE 128
#endif

enum param_types {
    PARAM_ATTACK = (unsigned char)'a',
    PARAM_LENGTH = (unsigned char)'l',
    PARAM_NUM    = (unsigned char)'n',
    PARAM_SEED   = (unsigned char)'s',
    PARAM_THREAD = (unsigned char)'t',
};

enum param_defaults {
    PARAM_DEFAULT_ATTACK = 10,
    PARAM_DEFAULT_LENGTH = 16,
    PARAM_DEFAULT_NUM    = 1 << 20,
    PARAM_DEFAULT_SEED   = 1,
    PARAM_DEFAULT_THREAD = 1,
};

long global_params[256] = { /* 256 = ascii limit */
    [PARAM_ATTACK] = PARAM_DEFAULT_ATTACK,
    [PARAM_LENGTH] = PARAM_DEFAULT_LENGTH,
    [PARAM_NUM]    = PARAM_DEFAULT_NUM,
    [PARAM_SEED]   = PARAM_DEFAULT_SEED,
    [PARAM_THREAD] = PARAM_DEFAULT_THREAD,
};

typedef struct arg {
  /* input: */
    stream_t* streamPtr;
    decoder_t* decoderPtr;
  /* output: */
    vector_t** errorVectors;
} arg_t;


/* =============================================================================
 * displayUsage
 * =============================================================================
 */
static void
displayUsage (const char* appName)
{
    printf("Usage: %s [options]\n", appName);
    puts("\nOptions:                            (defaults)\n");
    printf("    a <UINT>   Percent [a]ttack     (%i)\n", PARAM_DEFAULT_ATTACK);
    printf("    l <UINT>   Max data [l]ength    (%i)\n", PARAM_DEFAULT_LENGTH);
    printf("    n <UINT>   [n]umber of flows    (%i)\n", PARAM_DEFAULT_NUM);
    printf("    s <UINT>   Random [s]eed        (%i)\n", PARAM_DEFAULT_SEED);
    printf("    t <UINT>   Number of [t]hreads  (%i)\n", PARAM_DEFAULT_THREAD);
    exit(1);
}


/* =============================================================================
 * parseArgs
 * =============================================================================
 */
static void
parseArgs (long argc, char* const argv[])
{
    long i;
    long opt;

    opterr = 0;

    while ((opt = getopt(argc, argv, "a:l:n:s:t:")) != -1) {
        switch (opt) {
            case 'a':
            case 'l':
            case 'n':
            case 's':
            case 't':
                global_params[(unsigned char)opt] = atol(optarg);
                break;
            case '?':
            default:
                opterr++;
                break;
        }
    }

    for (i = optind; i < argc; i++) {
        fprintf(stderr, "Non-option argument: %s\n", argv[i]);
        opterr++;
    }

    if (opterr) {
        displayUsage(argv[0]);
    }
}

#ifndef FINE_LOCK

/* =============================================================================
 * processPackets
 * =============================================================================
 */
void
processPackets (void* argPtr)
{
    TM_THREAD_ENTER();

    long threadId = thread_getId();

    stream_t*   streamPtr    = ((arg_t*)argPtr)->streamPtr;
    decoder_t*  decoderPtr   = ((arg_t*)argPtr)->decoderPtr;
    vector_t**  errorVectors = ((arg_t*)argPtr)->errorVectors;

    detector_t* detectorPtr = PDETECTOR_ALLOC();
    assert(detectorPtr);
    PDETECTOR_ADDPREPROCESSOR(detectorPtr, &preprocessor_toLower);

    vector_t* errorVectorPtr = errorVectors[threadId];

    int i, j;
    bool_t is_finish = FALSE;
    long flowIds[CHUNK_DECODE];
    long found;

    while (!is_finish) {
#ifdef DISTRIDUTEDPACKET
        char* bytes_array[CHUNK_STREAM];
        for (i = 0; i < CHUNK_STREAM; ++i) {
            bytes_array[i] = stream_getPacket(streamPtr,threadId);
            if (!bytes_array[i]) {
                break;
            }
        }
#else
        char* bytes_array[CHUNK_STREAM];
        TM_BEGIN();
        for (i = 0; i < CHUNK_STREAM; ++i) {
            bytes_array[i] = TMSTREAM_GETPACKET(streamPtr);
            if (!bytes_array[i]) {
                break;
            }
        }
        TM_END();
#endif

        for (i = 0; i < CHUNK_STREAM && !is_finish; i += CHUNK_DECODE) {
#ifndef FINE_TX
            TM_BEGIN();
#endif
            found = 0;
            long ii;
            long ii_stop = MIN(CHUNK_STREAM, (i+CHUNK_DECODE));
            for (ii = i; ii < ii_stop; ++ii) {
                packet_t* packetPtr = (packet_t*)bytes_array[ii];
                if (!packetPtr) {
                    is_finish = TRUE;
                    break;
                }
#ifdef FINE_TX
                TM_BEGIN();
#endif
                error_t error;
                error = TMDECODER_PROCESS(decoderPtr,
                                          detectorPtr,
                                          bytes_array[ii],
                                          (PACKET_HEADER_LENGTH + packetPtr->length));
                
#ifdef FINE_TX
                TM_END();
#endif
                if (error) {
                    if (error == ERROR_SIGNATURE) {
                        flowIds[found] = packetPtr->flowId;
                        ++found;
                    } else {
                        /*
                        * Currently, stream_generate() does not create these errors.
                        */
                        assert(0);
                        bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                         (void*)packetPtr->flowId);
                        assert(status);
                    }
                }
            }
#ifndef FINE_TX
            TM_END();
#endif
            for (ii = 0; ii < found; ++ii) {
                bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                (void*)flowIds[ii]);
                assert(status);
            }
#if 0
            char* data;
            long decodedFlowId;
            TM_BEGIN();
            data = TMDECODER_GETCOMPLETE(decoderPtr, &decodedFlowId);
            TM_END();
            if (data) {
                error_t error = PDETECTOR_PROCESS(detectorPtr, data);
                P_FREE(data);
                if (error) {
                    bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                    (void*)decodedFlowId);
                    assert(status);
                }
            }
#endif
        }
    }

    PDETECTOR_FREE(detectorPtr);

    TM_THREAD_EXIT();
}

#else

/* =============================================================================
 * processPackets
 * =============================================================================
 */
void
processPackets (void* argPtr)
{
    TM_THREAD_ENTER();

    long threadId = thread_getId();

    stream_t*   streamPtr    = ((arg_t*)argPtr)->streamPtr;
    decoder_t*  decoderPtr   = ((arg_t*)argPtr)->decoderPtr;
    vector_t**  errorVectors = ((arg_t*)argPtr)->errorVectors;

    detector_t* detectorPtr = PDETECTOR_ALLOC();
    assert(detectorPtr);
    PDETECTOR_ADDPREPROCESSOR(detectorPtr, &preprocessor_toLower);

    vector_t* errorVectorPtr = errorVectors[threadId];

    int i, j;
    bool_t is_finish = FALSE;

    while (!is_finish) {

        char* bytes_array[CHUNK_SIZE];
        LOCK(stream_lock);
        for (i = 0; i < CHUNK_SIZE; ++i) {
            bytes_array[i] = TMSTREAM_GETPACKET(streamPtr);
            if (!bytes_array[i]) {
                break;
            }
        }
        UNLOCK(stream_lock);

        for (j = 0; j < CHUNK_SIZE; ++j) {
            packet_t* packetPtr = (packet_t*)bytes_array[j];
            if (!packetPtr) {
                is_finish = TRUE;
                break;
            }

            long flowId = packetPtr->flowId;
            error_t error;

            MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
            long idx = fragmentedMapPtr->hash(flowId) % fragmentedMapPtr->numBucket;

            LOCK(table_locks[idx]);
            error = TMDECODER_PROCESS(decoderPtr,
                                      detectorPtr,
                                      bytes_array[j],
                                      (PACKET_HEADER_LENGTH + packetPtr->length));
            UNLOCK(table_locks[idx]);
            if (error) {
                if (error == ERROR_SIGNATURE) {
                    bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                    (void*)flowId);
                    assert(status);
                } else {
                    /*
                    * Currently, stream_generate() does not create these errors.
                    */
                    assert(0);
                    bool_t status = PVECTOR_PUSHBACK(errorVectorPtr, (void*)flowId);
                    assert(status);
                }
            }
#if 0
            char* data;
            long decodedFlowId;
            TM_BEGIN();
            data = TMDECODER_GETCOMPLETE(decoderPtr, &decodedFlowId);
            TM_END();
            if (data) {
                error_t error = PDETECTOR_PROCESS(detectorPtr, data);
                P_FREE(data);
                if (error) {
                    bool_t status = PVECTOR_PUSHBACK(errorVectorPtr,
                                                    (void*)decodedFlowId);
                    assert(status);
                }
            }
#endif
        }
    }

    PDETECTOR_FREE(detectorPtr);

    TM_THREAD_EXIT();
}

#endif

/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    GOTO_REAL();

    /*
     * Initialization
     */

    parseArgs(argc, (char** const)argv);
    long numCore = global_params[PARAM_THREAD];
    long numThread = numCore * THREADS_PER_CORE;
    {
        char* s = getenv("THR_PER_CORE");
        if (s != NULL)  {
            numThread = numCore * strtol(s, NULL, 10);
        }
    }

    SIM_GET_NUM_CPU(numCore);
    TM_STARTUP(numCore);
    P_MEMORY_STARTUP(numThread);
    thread_startup(numCore, numThread);

#if defined(HTM) || defined(STM)
    printf("===========================\n");
    printf(" * CHUNK_STREAM = %d\n", CHUNK_STREAM);
    printf(" * CHUNK_DECODE = %d\n", CHUNK_DECODE);
    printf("===========================\n\n");
#endif

    long percentAttack = global_params[PARAM_ATTACK];
    long maxDataLength = global_params[PARAM_LENGTH];
    long numFlow       = global_params[PARAM_NUM];
    long randomSeed    = global_params[PARAM_SEED];
    printf("Percent attack  = %li\n", percentAttack);
    printf("Max data length = %li\n", maxDataLength);
    printf("Num flow        = %li\n", numFlow);
    printf("Random seed     = %li\n", randomSeed);

    dictionary_t* dictionaryPtr = dictionary_alloc();
    assert(dictionaryPtr);
    stream_t* streamPtr = stream_alloc(percentAttack);
    assert(streamPtr);
#ifdef DISTRIDUTEDPACKET
    long numAttack = stream_generate(streamPtr,
                                     dictionaryPtr,
                                     numFlow,
                                     randomSeed,
                                     maxDataLength,
                                     numThread);
#else
    long numAttack = stream_generate(streamPtr,
                                     dictionaryPtr,
                                     numFlow,
                                     randomSeed,
                                     maxDataLength);
#endif
    printf("Num attack      = %li\n", numAttack);

    decoder_t* decoderPtr = decoder_alloc();
    assert(decoderPtr);

    vector_t** errorVectors = (vector_t**)P_MALLOC(numThread * sizeof(vector_t*));
    assert(errorVectors);
    long i;
    for (i = 0; i < numThread; i++) {
        vector_t* errorVectorPtr = vector_alloc(0);
        assert(errorVectorPtr);
        errorVectors[i] = errorVectorPtr;
    }

#ifdef FINE_LOCK
    LOCK_INIT(stream_lock);
    long nbucket = decoderPtr->fragmentedMapPtr->numBucket;
    table_locks = (LOCK_T*)P_MALLOC(nbucket * sizeof(LOCK_T));
    assert(table_locks);
    for (i = 0; i < nbucket; ++i) {
        LOCK_INIT(table_locks[i]);
    }
#endif

    arg_t arg;
    arg.streamPtr    = streamPtr;
    arg.decoderPtr   = decoderPtr;
    arg.errorVectors = errorVectors;

    /*
     * Run transactions
     */

    TIMER_T startTime;
    TIMER_READ(startTime);
    GOTO_SIM();
#ifdef OTM
#pragma omp parallel
    {
        processPackets((void*)&arg);
    }
#else
    thread_start(processPackets, (void*)&arg);
#endif
    GOTO_REAL();
    TIMER_T stopTime;
    TIMER_READ(stopTime);
    printf("Time = %f seconds\n", TIMER_DIFF_SECONDS(startTime, stopTime));

    /*
     * Check solution
     */

    long numFound = 0;
    for (i = 0; i < numThread; i++) {
        vector_t* errorVectorPtr = errorVectors[i];
        long e;
        long numError = vector_getSize(errorVectorPtr);
        numFound += numError;
        for (e = 0; e < numError; e++) {
            long flowId = (long)vector_at(errorVectorPtr, e);
            bool_t status = stream_isAttack(streamPtr, flowId);
            assert(status);
        }
    }
    printf("Num found       = %li\n", numFound);
    assert(numFound == numAttack);

    /*
     * Clean up
     */

    for (i = 0; i < numThread; i++) {
        vector_free(errorVectors[i]);
    }
    P_FREE(errorVectors);
    decoder_free(decoderPtr);
    stream_free(streamPtr);
    dictionary_free(dictionaryPtr);

#ifdef FINE_LOCK
    P_FREE(table_locks);
#endif

    TM_SHUTDOWN();
    P_MEMORY_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(0);
}


/* =============================================================================
 *
 * End of intruder.c
 *
 * =============================================================================
 */
