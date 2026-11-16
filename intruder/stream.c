/* =============================================================================
 *
 * stream.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "detector.h"
#include "dictionary.h"
#include "map.h"
#include "packet.h"
#include "queue.h"
#include "random.h"
#include "stream.h"
#include "tm.h"
#include "vector.h"


struct stream {
    long percentAttack;
    random_t* randomPtr;
    vector_t* allocVectorPtr;
    queue_t* packetQueuePtr;
    MAP_T* attackMapPtr;
#ifdef DISTRIDUTEDPACKET
    queue_t** packetQueues;
#endif
};


/* =============================================================================
 * stream_alloc
 * =============================================================================
 */
stream_t*
stream_alloc (long percentAttack)
{
    stream_t* streamPtr;

    streamPtr = (stream_t*)P_MALLOC(sizeof(stream_t));
    if (streamPtr) {
        assert(percentAttack >= 0 && percentAttack <= 100);
        streamPtr->percentAttack = percentAttack;
        streamPtr->randomPtr = random_alloc();
        assert(streamPtr->randomPtr);
        streamPtr->allocVectorPtr = vector_alloc(0);
        assert(streamPtr->allocVectorPtr);
        streamPtr->packetQueuePtr = queue_alloc(-1);
        assert(streamPtr->packetQueuePtr);
        streamPtr->attackMapPtr = MAP_ALLOC(NULL, NULL);
        assert(streamPtr->attackMapPtr);
#ifdef DISTRIDUTEDPACKET
        long numThreads = thread_getNumThread();
        assert(numThreads > 0);
        streamPtr->packetQueues = (queue_t**)P_MALLOC(sizeof(queue_t*) * numThreads);
        assert(streamPtr->packetQueues);
        for (long i = 0; i < numThreads; i++) {
            streamPtr->packetQueues[i] = queue_alloc(-1);
            assert(streamPtr->packetQueues[i]);
        }
#endif
    }

    return streamPtr;
}


/* =============================================================================
 * stream_free
 * =============================================================================
 */
void
stream_free (stream_t* streamPtr)
{
    vector_t* allocVectorPtr = streamPtr->allocVectorPtr;
    long a;
    long numAlloc = vector_getSize(allocVectorPtr);

    for (a = 0; a < numAlloc; a++) {
        char* str = (char*)vector_at(allocVectorPtr, a);
        P_FREE(str);
    }

    MAP_FREE(streamPtr->attackMapPtr);
    queue_free(streamPtr->packetQueuePtr);
    vector_free(streamPtr->allocVectorPtr);
    random_free(streamPtr->randomPtr);
    P_FREE(streamPtr);
#ifdef DISTRIDUTEDPACKET
    queue_free(streamPtr->packetQueues);
#endif
}


/* =============================================================================
 * splitIntoPackets
 * -- Packets will be equal-size chunks except for last one, which will have
 *    all extra bytes
 * =============================================================================
 */
static void
splitIntoPackets (char* str,
                  long flowId,
                  random_t* randomPtr,
                  vector_t* allocVectorPtr,
                  queue_t* packetQueuePtr)
{
    long numByte = strlen(str);
    long numPacket = random_generate(randomPtr) % numByte + 1;

    long numDataByte = numByte / numPacket;

    long p;
    for (p = 0; p < (numPacket - 1); p++) {
        bool_t status;
        char* bytes = (char*)P_MALLOC(PACKET_HEADER_LENGTH + numDataByte);
        assert(bytes);
        status = vector_pushBack(allocVectorPtr, (void*)bytes);
        assert(status);
        packet_t* packetPtr = (packet_t*)bytes;
        packetPtr->flowId      = flowId;
        packetPtr->fragmentId  = p;
        packetPtr->numFragment = numPacket;
        packetPtr->length      = numDataByte;
        memcpy(packetPtr->data, (str + p * numDataByte), numDataByte);
        status = queue_push(packetQueuePtr, (void*)packetPtr);
        assert(status);
    }

    bool_t status;
    long lastNumDataByte = numDataByte + numByte % numPacket;
    char* bytes = (char*)P_MALLOC(PACKET_HEADER_LENGTH + lastNumDataByte);
    assert(bytes);
    status = vector_pushBack(allocVectorPtr, (void*)bytes);
    assert(status);
    packet_t* packetPtr = (packet_t*)bytes;
    packetPtr->flowId      = flowId;
    packetPtr->fragmentId  = p;
    packetPtr->numFragment = numPacket;
    packetPtr->length      = lastNumDataByte;
    memcpy(packetPtr->data, (str + p * numDataByte), lastNumDataByte);
    status = queue_push(packetQueuePtr, (void*)packetPtr);
    assert(status);
}


/* =============================================================================
 * stream_generate
 * -- Returns number of attacks generated
 * =============================================================================
 */
#ifdef DISTRIDUTEDPACKET
long
stream_generate (stream_t* streamPtr,
                 dictionary_t* dictionaryPtr,
                 long numFlow,
                 long seed,
                 long maxLength,
                 long NumThread)
#else
long
stream_generate (stream_t* streamPtr,
                 dictionary_t* dictionaryPtr,
                 long numFlow,
                 long seed,
                 long maxLength)
#endif
{
    long numAttack = 0;

    long      percentAttack  = streamPtr->percentAttack;
    random_t* randomPtr      = streamPtr->randomPtr;
    vector_t* allocVectorPtr = streamPtr->allocVectorPtr;
    queue_t*  packetQueuePtr = streamPtr->packetQueuePtr;
    MAP_T*    attackMapPtr   = streamPtr->attackMapPtr;
#ifdef DISTRIDUTEDPACKET
    queue_t** packetQueues    =streamPtr->packetQueues;
#endif   

    detector_t* detectorPtr = detector_alloc();
    assert(detectorPtr);
    detector_addPreprocessor(detectorPtr, &preprocessor_toLower);

    random_seed(randomPtr, seed);
    queue_clear(packetQueuePtr);

    long range = '~' - ' ' + 1;
    assert(range > 0);

    long f;
    for (f = 1; f <= numFlow; f++) {
        char* str;
        if ((random_generate(randomPtr) % 100) < percentAttack) {
            long s = random_generate(randomPtr) % global_numDefaultSignature;
            str = dictionary_get(dictionaryPtr, s);
            bool_t status =
                MAP_INSERT(attackMapPtr, (void*)f, (void*)str);
            assert(status);
            numAttack++;
        } else {
            /*
             * Create random string
             */
            long length = (random_generate(randomPtr) % maxLength) + 1;
            str = (char*)P_MALLOC((length + 1) * sizeof(char));
            bool_t status = vector_pushBack(allocVectorPtr, (void*)str);
            assert(status);
            long l;
            for (l = 0; l < length; l++) {
                str[l] = ' ' + (char)(random_generate(randomPtr) % range);
            }
            str[l] = '\0';
            char* str2 = (char*)P_MALLOC((length + 1) * sizeof(char));
            assert(str2);
            strcpy(str2, str);
            error_t error = detector_process(detectorPtr, str2); /* updates in-place */
            if (error == ERROR_SIGNATURE) {
                bool_t status = MAP_INSERT(attackMapPtr,
                                           (void*)f,
                                           (void*)str);
                assert(status);
                numAttack++;
            }
            P_FREE(str2);
        }
        splitIntoPackets(str, f, randomPtr, allocVectorPtr, packetQueuePtr);
    }
#ifdef DISTRIDUTEDPACKET
    long threadIndex = 0;
    while (!queue_isEmpty(packetQueuePtr)) {
        void* packet = queue_pop(packetQueuePtr);
        queue_push(packetQueues[threadIndex], packet);
        threadIndex = (threadIndex + 1) % NumThread;
    }
#endif
    // queue_shuffle(packetQueuePtr, randomPtr);

    detector_free(detectorPtr);

    return numAttack;
}


/* =============================================================================
 * stream_getPacket
 * -- If none, returns NULL
 * =============================================================================
 */
#ifdef DISTRIDUTEDPACKET
char*
stream_getPacket (stream_t* streamPtr,long threadId)
{
    return queue_pop(streamPtr->packetQueues[threadId]);
}
#else
char*
stream_getPacket (stream_t* streamPtr)
{
    return queue_pop(streamPtr->packetQueuePtr);
}
#endif

/* =============================================================================
 * TMstream_getPacket
 * -- If none, returns NULL
 * =============================================================================
 */
#ifdef DISTRIDUTEDPACKET
char*
TMstream_getPacket (TM_ARGDECL stream_t* streamPtr,long threadId)
{
    return (char*)TMQUEUE_POP(streamPtr->packetQueues[threadId]);
}
#else
char*
TMstream_getPacket (TM_ARGDECL stream_t* streamPtr)
{
    return (char*)TMQUEUE_POP(streamPtr->packetQueuePtr);
}
#endif


/* =============================================================================
 * stream_isAttack
 * =============================================================================
 */
bool_t
stream_isAttack (stream_t* streamPtr, long flowId)
{
    return MAP_CONTAINS(streamPtr->attackMapPtr, (void*)flowId);
}


/* #############################################################################
 * TEST_STREAM
 * #############################################################################
 */
#ifdef TEST_STREAM


#include <assert.h>
#include <stdio.h>


int
main ()
{
    long percentAttack = 10;
    long numFlow = 100;
    long seed = 0;
    long maxLength = 20;

    puts("Starting...");

    stream_t* streamPtr = stream_alloc(percentAttack);
    assert(streamPtr);

    dictionary_t* dictionaryPtr = dictionary_alloc();
    assert(dictionaryPtr);

    stream_generate(streamPtr, dictionaryPtr, numFlow, seed, maxLength);

    char* bytes;
    while ((bytes = stream_getPacket(streamPtr))) {
        packet_t* packetPtr = (packet_t*)bytes;
        long  flowId      = packetPtr->flowId;
        long  fragmentId  = packetPtr->fragmentId;
        long  numFragment = packetPtr->numFragment;
        long  length      = packetPtr->length;
        char* data        = packetPtr->data;
        long l;
        printf("flow=%2li frag=%2li num=%2li data=", flowId, fragmentId, numFragment);
        for (l = 0; l < length; l++) {
            printf("%c", data[l]);
        }
        puts("");
    }

    stream_free(streamPtr);

    puts("Done.");

    return 0;
}


#endif /* TEST_STREAM */


/* =============================================================================
 *
 * End of stream.c
 *
 * =============================================================================
 */
