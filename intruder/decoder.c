/* =============================================================================
 *
 * decoder.c
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
#include <string.h>
#include "decoder.h"
#include "detector.h"
#include "error.h"
#include "list.h"
#include "vector.h"
#include "map.h"
#include "packet.h"
#include "queue.h"
#include "tm.h"
#include "types.h"


// struct decoder {
//     MAP_T* fragmentedMapPtr;  /* contains list of packet_t* */
//     queue_t* decodedQueuePtr; /* contains decoded_t* */
// };

typedef struct decoded {
    long flowId;
    char* data;
} decoded_t;

typedef struct fragment_array {
    packet_t** packets;
    long numFragment;
    long flowId;
    long size;
} fragment_array_t;


/* =============================================================================
 * decoder_alloc
 * =============================================================================
 */
decoder_t*
decoder_alloc ()
{
    decoder_t* decoderPtr;

    decoderPtr = (decoder_t*)P_MALLOC(sizeof(decoder_t));
    if (decoderPtr) {
        decoderPtr->fragmentedMapPtr = MAP_ALLOC(NULL, NULL);
        assert(decoderPtr->fragmentedMapPtr);
        // decoderPtr->decodedQueuePtr = queue_alloc(1024);
        // assert(decoderPtr->decodedQueuePtr);
    }

    return decoderPtr;
}


/* =============================================================================
 * decoder_free
 * =============================================================================
 */
void
decoder_free (decoder_t* decoderPtr)
{
    // queue_free(decoderPtr->decodedQueuePtr);
    MAP_FREE(decoderPtr->fragmentedMapPtr);
    P_FREE(decoderPtr);
}


/* =============================================================================
 * decoder_process
 * =============================================================================
 */
error_t
decoder_process (decoder_t* decoderPtr, detector_t* detectorPtr, char* bytes, long numByte)
{
    bool_t status;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        return ERROR_SHORT;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;

    long i;

    if (flowId < 0) {
        return ERROR_FLOWID;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        return ERROR_FRAGMENTID;
    }

    if (length < 0) {
        return ERROR_LENGTH;
    }

#if 0
    /*
     * With the above checks, this one is redundant
     */
    if (numFragment < 1) {
        return ERROR_NUMFRAGMENT;
    }
#endif

    /*
     * Add to fragmented map for reassembling
     */

    if (numFragment > 1) {

        MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
        fragment_array_t* fragmentArrayPtr =
            (fragment_array_t*)MAP_FIND(fragmentedMapPtr, (void*)flowId);

        if (fragmentArrayPtr == NULL) {

            fragmentArrayPtr = (fragment_array_t*)P_MALLOC(sizeof(fragment_array_t));
            assert(fragmentArrayPtr);
            fragmentArrayPtr->packets = (packet_t**)P_MALLOC(sizeof(packet_t*) * numFragment);

            fragmentArrayPtr->numFragment = numFragment;
            fragmentArrayPtr->flowId = flowId;
            fragmentArrayPtr->size = 1;
            fragmentArrayPtr->packets[fragmentId] = packetPtr;

            status = MAP_INSERT(fragmentedMapPtr,
                                (void*)flowId,
                                (void*)fragmentArrayPtr);
            assert(status);

        } else {

            long expectedNumFragment = fragmentArrayPtr->numFragment;

            if (numFragment != expectedNumFragment) {
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
                return ERROR_NUMFRAGMENT;
            }

            fragmentArrayPtr->packets[fragmentId] = packetPtr;
            fragmentArrayPtr->size++;

            /*
             * If we have all the fragments we can reassemble them
             */

            if (fragmentArrayPtr->size == numFragment) {

                long numByte = 0;
                for (i = 0; i < numFragment; ++i) {
                    packet_t* fragmentPtr = fragmentArrayPtr->packets[i];
                    assert(fragmentPtr->flowId == flowId);
                    // if (fragmentPtr->fragmentId != i) {
                    //     status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    //     assert(status);
                    //     return ERROR_INCOMPLETE; /* should be sequential */
                    // }
                    numByte += fragmentPtr->length;
                }

                char* data = (char*)P_MALLOC(numByte + 1);
                assert(data);
                data[numByte] = '\0';
                char* dst = data;
                for (i = 0; i < numFragment; ++i) {
                    packet_t* fragmentPtr = fragmentArrayPtr->packets[i];
                    memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                    dst += fragmentPtr->length;
                }
                assert(dst == data + numByte);

                PVECTOR_FREE(fragmentArrayPtr);
                status = MAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);

                return PDETECTOR_PROCESS(detectorPtr, data);
            }

        }

    } else {

        /*
         * This is the only fragment, so it is ready
         */

        if (fragmentId != 0) {
            return ERROR_FRAGMENTID;
        }

        char* data = (char*)P_MALLOC(length + 1);
        assert(data);
        data[length] = '\0';
        memcpy(data, packetPtr->data, length);

        return PDETECTOR_PROCESS(detectorPtr, data);
    }

    return ERROR_NONE;
}


/* =============================================================================
 * TMdecoder_process
 * =============================================================================
 */
error_t
TMdecoder_process (TM_ARGDECL decoder_t* decoderPtr, detector_t* detectorPtr, char* bytes, long numByte)
{
    bool_t status;

    /*
     * Basic error checking
     */

    if (numByte < PACKET_HEADER_LENGTH) {
        return ERROR_SHORT;
    }

    packet_t* packetPtr = (packet_t*)bytes;
    long flowId      = packetPtr->flowId;
    long fragmentId  = packetPtr->fragmentId;
    long numFragment = packetPtr->numFragment;
    long length      = packetPtr->length;

    long i;

    if (flowId < 0) {
        return ERROR_FLOWID;
    }

    if ((fragmentId < 0) || (fragmentId >= numFragment)) {
        return ERROR_FRAGMENTID;
    }

    if (length < 0) {
        return ERROR_LENGTH;
    }

#if 0
    /*
     * With the above checks, this one is redundant
     */
    if (numFragment < 1) {
        return ERROR_NUMFRAGMENT;
    }
#endif

    /*
     * Add to fragmented map for reassembling
     */

    if (numFragment > 1) {

        MAP_T* fragmentedMapPtr = decoderPtr->fragmentedMapPtr;
        fragment_array_t* fragmentArrayPtr =
            (fragment_array_t*)TMMAP_FIND(fragmentedMapPtr, (void*)flowId);

        if (fragmentArrayPtr == NULL) {

            fragmentArrayPtr = (fragment_array_t*)TM_MALLOC(sizeof(fragment_array_t));
            assert(fragmentArrayPtr);
            fragmentArrayPtr->packets = (packet_t**)TM_MALLOC(sizeof(packet_t*) * numFragment);

            fragmentArrayPtr->numFragment = numFragment;
            fragmentArrayPtr->flowId = flowId;
            fragmentArrayPtr->size = 1;
            fragmentArrayPtr->packets[fragmentId] = packetPtr;

            status = TMMAP_INSERT(fragmentedMapPtr,
                                (void*)flowId,
                                (void*)fragmentArrayPtr);
            assert(status);

        } else {

            long expectedNumFragment = fragmentArrayPtr->numFragment;

            if (numFragment != expectedNumFragment) {
                status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);
                return ERROR_NUMFRAGMENT;
            }

            TM_SHARED_WRITE_P(fragmentArrayPtr->packets[fragmentId], packetPtr);
            TM_SHARED_WRITE(fragmentArrayPtr->size, TM_SHARED_READ(fragmentArrayPtr->size)+1);

            /*
             * If we have all the fragments we can reassemble them
             */

            if (TM_SHARED_READ(fragmentArrayPtr->size) == numFragment) {

                long numByte = 0;
                for (i = 0; i < numFragment; ++i) {
                    packet_t* fragmentPtr =
                        TM_SHARED_READ_P(fragmentArrayPtr->packets[i]);
                    assert(fragmentPtr->flowId == flowId);
                    // if (fragmentPtr->fragmentId != i) {
                    //     status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                    //     assert(status);
                    //     return ERROR_INCOMPLETE; /* should be sequential */
                    // }
                    numByte += fragmentPtr->length;
                }

                char* data = (char*)TM_MALLOC(numByte + 1);
                assert(data);
                data[numByte] = '\0';
                char* dst = data;
                for (i = 0; i < numFragment; ++i) {
                    packet_t* fragmentPtr =
                        TM_SHARED_READ_P(fragmentArrayPtr->packets[i]);
                    memcpy(dst, fragmentPtr->data, fragmentPtr->length);
                    dst += fragmentPtr->length;
                }
                assert(dst == data + numByte);
                TM_FREE(fragmentArrayPtr);
                status = TMMAP_REMOVE(fragmentedMapPtr, (void*)flowId);
                assert(status);

                return PDETECTOR_PROCESS(detectorPtr, data);
            }

        }

    } else {

        /*
         * This is the only fragment, so it is ready
         */

        if (fragmentId != 0) {
            return ERROR_FRAGMENTID;
        }

        char* data = (char*)TM_MALLOC(length + 1);
        assert(data);
        data[length] = '\0';
        memcpy(data, packetPtr->data, length);

        return PDETECTOR_PROCESS(detectorPtr, data);
    }

    return ERROR_NONE;
}


/* =============================================================================
 * decoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
decoder_getComplete (decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = queue_pop(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = decodedPtr->flowId;
        data = decodedPtr->data;
        P_FREE(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}


/* =============================================================================
 * TMdecoder_getComplete
 * -- If none, returns NULL
 * =============================================================================
 */
char*
TMdecoder_getComplete (TM_ARGDECL  decoder_t* decoderPtr, long* decodedFlowIdPtr)
{
    char* data;
    decoded_t* decodedPtr = TMQUEUE_POP(decoderPtr->decodedQueuePtr);

    if (decodedPtr) {
        *decodedFlowIdPtr = decodedPtr->flowId;
        data = decodedPtr->data;
        TM_FREE(decodedPtr);
    } else {
        *decodedFlowIdPtr = -1;
        data = NULL;
    }

    return data;
}


/* #############################################################################
 * TEST_DECODER
 * #############################################################################
 */
#ifdef TEST_DECODER

#include <stdio.h>


int
main ()
{
    decoder_t* decoderPtr;

    puts("Starting...");

    decoderPtr = decoder_alloc();
    assert(decoderPtr);

    long numDataByte = 3;
    long numPacketByte = PACKET_HEADER_LENGTH + numDataByte;

    char* abcBytes = (char*)malloc(numPacketByte);
    assert(abcBytes);
    packet_t* abcPacketPtr;
    abcPacketPtr = (packet_t*)abcBytes;
    abcPacketPtr->flowId = 1;
    abcPacketPtr->fragmentId = 0;
    abcPacketPtr->numFragment = 2;
    abcPacketPtr->length = numDataByte;
    abcPacketPtr->data[0] = 'a';
    abcPacketPtr->data[1] = 'b';
    abcPacketPtr->data[2] = 'c';

    char* defBytes = (char*)malloc(numPacketByte);
    assert(defBytes);
    packet_t* defPacketPtr;
    defPacketPtr = (packet_t*)defBytes;
    defPacketPtr->flowId = 1;
    defPacketPtr->fragmentId = 1;
    defPacketPtr->numFragment = 2;
    defPacketPtr->length = numDataByte;
    defPacketPtr->data[0] = 'd';
    defPacketPtr->data[1] = 'e';
    defPacketPtr->data[2] = 'f';

    assert(decoder_process(decoderPtr, abcBytes, numDataByte) == ERROR_SHORT);

    abcPacketPtr->flowId = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FLOWID);
    abcPacketPtr->flowId = 1;

    abcPacketPtr->fragmentId = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->fragmentId = 2;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->fragmentId = 2;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_FRAGMENTID);
    abcPacketPtr->fragmentId = 0;

    abcPacketPtr->length = -1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_LENGTH);
    abcPacketPtr->length = numDataByte;

    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    defPacketPtr->numFragment = 3;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_NUMFRAGMENT);
    defPacketPtr->numFragment = 2;

    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    defPacketPtr->fragmentId = 0;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_INCOMPLETE);
    defPacketPtr->fragmentId = 1;

    long flowId;
    assert(decoder_process(decoderPtr, defBytes, numPacketByte) == ERROR_NONE);
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    char* str = decoder_getComplete(decoderPtr, &flowId);
    assert(strcmp(str, "abcdef") == 0);
    free(str);
    assert(flowId == 1);

    abcPacketPtr->numFragment = 1;
    assert(decoder_process(decoderPtr, abcBytes, numPacketByte) == ERROR_NONE);
    str = decoder_getComplete(decoderPtr, &flowId);
    assert(strcmp(str, "abc") == 0);
    free(str);
    abcPacketPtr->numFragment = 2;
    assert(flowId == 1);

    str = decoder_getComplete(decoderPtr, &flowId);
    assert(str == NULL);
    assert(flowId == -1);

    decoder_free(decoderPtr);

    free(abcBytes);
    free(defBytes);

    puts("All tests passed.");

    return 0;
}


#endif /* TEST_DECODER */


/* =============================================================================
 *
 * End of decoder.c
 *
 * =============================================================================
 */
