/* =============================================================================
 *
 * client.c
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
#include "action.h"
#include "client.h"
#include "manager.h"
#include "reservation.h"
#include "thread.h"
#include "types.h"

#ifdef FINE_LOCK
#include "customer.h"
extern LOCK_T *table_locks[4];
#endif

/* =============================================================================
 * client_alloc
 * -- Returns NULL on failure
 * =============================================================================
 */
client_t*
client_alloc (long id,
              manager_t* managerPtr,
              long numOperation,
              long numQueryPerTransaction,
              long queryRange,
              long percentUser)
{
    client_t* clientPtr;

    clientPtr = (client_t*)P_MALLOC(sizeof(client_t));
    if (clientPtr == NULL) {
        return NULL;
    }

    clientPtr->randomPtr = random_alloc();
    if (clientPtr->randomPtr == NULL) {
        return NULL;
    }

    clientPtr->id = id;
    clientPtr->managerPtr = managerPtr;
    random_seed(clientPtr->randomPtr, id);
    clientPtr->numOperation = numOperation;
    clientPtr->numQueryPerTransaction = numQueryPerTransaction;
    clientPtr->queryRange = queryRange;
    clientPtr->percentUser = percentUser;

    return clientPtr;
}


/* =============================================================================
 * client_free
 * =============================================================================
 */
void
client_free (client_t* clientPtr)
{
    P_FREE(clientPtr);
}


/* =============================================================================
 * selectAction
 * =============================================================================
 */
static action_t
selectAction (long r, long percentUser)
{
    action_t action;

    if (r < percentUser) {
        action = ACTION_MAKE_RESERVATION;
    } else if (r & 1) {
        action = ACTION_DELETE_CUSTOMER;
    } else {
        action = ACTION_UPDATE_TABLES;
    }

    return action;
}

#ifndef FINE_LOCK

/* =============================================================================
 * client_run
 * -- Execute list operations on the database
 * =============================================================================
 */
void
client_run (void* argPtr)
{
    TM_THREAD_ENTER();

    long myId = thread_getId() % thread_getNumCore();
    client_t* clientPtr = ((client_t**)argPtr)[myId];

    manager_t* managerPtr = clientPtr->managerPtr;
    random_t*  randomPtr  = clientPtr->randomPtr;

    long numOperation           = clientPtr->numOperation;
    long numQueryPerTransaction = clientPtr->numQueryPerTransaction;
    long queryRange             = clientPtr->queryRange;
    long percentUser            = clientPtr->percentUser;

    long* types  = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ids    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ops    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* prices = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));

    long i;

    long start, end;
    long thrId = thread_getId() / thread_getNumCore();
    // start = thrId * numOperation / THREADS_PER_CORE;
    // end = (thrId + 1) * numOperation / THREADS_PER_CORE;
    start = thrId * numOperation / thread_getNumThreadPerCore();
    end = (thrId + 1) * numOperation / thread_getNumThreadPerCore();
    // if (thrId == THREADS_PER_CORE - 1) {
    if (thrId == thread_getNumThreadPerCore() - 1) {
        end = numOperation;
    }

    for (i = start; i < end; i++) {

        long r = random_generate(randomPtr) % 100;
        action_t action = selectAction(r, percentUser);

        switch (action) {

            case ACTION_MAKE_RESERVATION: {
                long maxPrices[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long maxIds[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long n;
                long numQuery = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long customerId = random_generate(randomPtr) % queryRange + 1;
                for (n = 0; n < numQuery; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                }
                bool_t isFound = FALSE;
                TM_BEGIN();
                for (n = 0; n < numQuery; n++) {
                    long t = types[n];
                    long id = ids[n];
                    long price = -1;
                    switch (t) {
                        case RESERVATION_CAR:
                            if (MANAGER_QUERY_CAR(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_CAR_PRICE(managerPtr, id);
                            }
                            break;
                        case RESERVATION_FLIGHT:
                            if (MANAGER_QUERY_FLIGHT(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_FLIGHT_PRICE(managerPtr, id);
                            }
                            break;
                        case RESERVATION_ROOM:
                            if (MANAGER_QUERY_ROOM(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_ROOM_PRICE(managerPtr, id);
                            }
                            break;
                        default:
                            assert(0);
                    }
                    if (price > maxPrices[t]) {
                        maxPrices[t] = price;
                        maxIds[t] = id;
                        isFound = TRUE;
                    }
                } /* for n */
                if (isFound) {
                    MANAGER_ADD_CUSTOMER(managerPtr, customerId);
                }
                if (maxIds[RESERVATION_CAR] > 0) {
                    MANAGER_RESERVE_CAR(managerPtr,
                                        customerId, maxIds[RESERVATION_CAR]);
                }
                if (maxIds[RESERVATION_FLIGHT] > 0) {
                    MANAGER_RESERVE_FLIGHT(managerPtr,
                                           customerId, maxIds[RESERVATION_FLIGHT]);
                }
                if (maxIds[RESERVATION_ROOM] > 0) {
                    MANAGER_RESERVE_ROOM(managerPtr,
                                         customerId, maxIds[RESERVATION_ROOM]);
                }
                TM_END();
                break;
            }

            case ACTION_DELETE_CUSTOMER: {
                long customerId = random_generate(randomPtr) % queryRange + 1;
                TM_BEGIN();
                long bill = MANAGER_QUERY_CUSTOMER_BILL(managerPtr, customerId);
                if (bill >= 0) {
                    MANAGER_DELETE_CUSTOMER(managerPtr, customerId);
                }
                TM_END();
                break;
            }

            case ACTION_UPDATE_TABLES: {
                long numUpdate = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long n;
                for (n = 0; n < numUpdate; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                    ops[n] = random_generate(randomPtr) % 2;
                    if (ops[n]) {
                        prices[n] = ((random_generate(randomPtr) % 5) * 10) + 50;
                    }
                }
                TM_BEGIN();
                for (n = 0; n < numUpdate; n++) {
                    long t = types[n];
                    long id = ids[n];
                    long doAdd = ops[n];
                    if (doAdd) {
                        long newPrice = prices[n];
                        switch (t) {
                            case RESERVATION_CAR:
                                MANAGER_ADD_CAR(managerPtr, id, 100, newPrice);
                                break;
                            case RESERVATION_FLIGHT:
                                MANAGER_ADD_FLIGHT(managerPtr, id, 100, newPrice);
                                break;
                            case RESERVATION_ROOM:
                                MANAGER_ADD_ROOM(managerPtr, id, 100, newPrice);
                                break;
                            default:
                                assert(0);
                        }
                    } else { /* do delete */
                        switch (t) {
                            case RESERVATION_CAR:
                                MANAGER_DELETE_CAR(managerPtr, id, 100);
                                break;
                            case RESERVATION_FLIGHT:
                                MANAGER_DELETE_FLIGHT(managerPtr, id);
                                break;
                            case RESERVATION_ROOM:
                                MANAGER_DELETE_ROOM(managerPtr, id, 100);
                                break;
                            default:
                                assert(0);
                        }
                    }
                }
                TM_END();
                break;
            }

            default:
                assert(0);

        } /* switch (action) */

    } /* for i */

    P_FREE(types);
    P_FREE(ids);
    P_FREE(ops);
    P_FREE(prices);

    TM_THREAD_EXIT();
}

#else /* FINE_LOCK */

typedef struct reserve_lock {
    long id;
    long type;
} reserve_lock_t;

int
compare_reserve_lock(const void* a, const void* b)
{
    const reserve_lock_t* aRl = (reserve_lock_t*)a;
    const reserve_lock_t* bRl = (reserve_lock_t*)b;

    if (aRl->type == bRl->type) {
        return (int)(aRl->id - bRl->id);
    }
    return (int)(aRl->type - bRl->type);
}

/*
 * To avoid self dead-lock,
 * check for previously locked items of the same type and hash(id).
 */
bool_t
isIncludeSamePrevious(reserve_lock_t* rl, int idx)
{
    long type = rl[idx].type;
    long id = rl[idx].id;
    for (int m = idx-1; m >= 0; --m) {
        if (type == rl[m].type && id == rl[m].id) {
            return TRUE;
        }
    }
    return FALSE;
}

/* =============================================================================
 * client_run
 * -- Execute list operations on the database
 * =============================================================================
 */
void
client_run (void* argPtr)
{
    TM_THREAD_ENTER();

    long myId = thread_getId() % thread_getNumCore();
    client_t* clientPtr = ((client_t**)argPtr)[myId];

    manager_t* managerPtr = clientPtr->managerPtr;
    random_t*  randomPtr  = clientPtr->randomPtr;

    long numOperation           = clientPtr->numOperation;
    long numQueryPerTransaction = clientPtr->numQueryPerTransaction;
    long queryRange             = clientPtr->queryRange;
    long percentUser            = clientPtr->percentUser;

    long* types  = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ids    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* ops    = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));
    long* prices = (long*)P_MALLOC(numQueryPerTransaction * sizeof(long));

    long  cl_id; // customer_lock
    reserve_lock_t* rl = (reserve_lock_t*)P_MALLOC(numOperation * sizeof(reserve_lock_t));

    MAP_T* reservationTables[3];
    reservationTables[RESERVATION_CAR] = managerPtr->carTablePtr;
    reservationTables[RESERVATION_FLIGHT] = managerPtr->flightTablePtr;
    reservationTables[RESERVATION_ROOM] = managerPtr->roomTablePtr;
    MAP_T* customerTablePtr = managerPtr->customerTablePtr;

    long i;

    long start, end;
    // long thrId = thread_getId() / thread_getNumCore();
    // start = thrId * numOperation / THREADS_PER_CORE;
    // end = (thrId + 1) * numOperation / THREADS_PER_CORE;
    // if (thrId == THREADS_PER_CORE - 1) {
    long thrId = thread_getId() / thread_getNumCore();
    start = thrId * numOperation / thread_getNumThreadPerCore();
    end = (thrId + 1) * numOperation / thread_getNumThreadPerCore();
    // if (thrId == THREADS_PER_CORE - 1) {
    if (thrId == thread_getNumThreadPerCore() - 1) {
        end = numOperation;
    }

    for (i = start; i < end; i++) {

        long r = random_generate(randomPtr) % 100;
        action_t action = selectAction(r, percentUser);

        switch (action) {

            case ACTION_MAKE_RESERVATION: {
                long maxPrices[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long maxIds[NUM_RESERVATION_TYPE] = { -1, -1, -1 };
                long n;
                long numQuery = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long customerId = random_generate(randomPtr) % queryRange + 1;
                for (n = 0; n < numQuery; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                }
                bool_t isFound = FALSE;

                for (n = 0; n < numQuery; n++) {
                    long t = types[n];
                    long id = ids[n];
                    rl[n].type = t;
                    rl[n].id = reservationTables[t]->hash((void*)id)
                                        % reservationTables[t]->numBucket;
                }
                /* To avoid deadlock, sort by priority. */
                qsort(rl, numQuery, sizeof(reserve_lock_t), compare_reserve_lock);

                cl_id = customerTablePtr->hash((void*)customerId) % customerTablePtr->numBucket;
                LOCK(table_locks[3][cl_id]);
                for (n = 0; n < numQuery; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        LOCK(table_locks[type][id]);
                    }
                }

                for (n = 0; n < numQuery; n++) {
                    long t = types[n];
                    long id = ids[n];
                    long price = -1;
                    switch (t) {
                        case RESERVATION_CAR:
                            if (MANAGER_QUERY_CAR(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_CAR_PRICE(managerPtr, id);
                            }
                            break;
                        case RESERVATION_FLIGHT:
                            if (MANAGER_QUERY_FLIGHT(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_FLIGHT_PRICE(managerPtr, id);
                            }
                            break;
                        case RESERVATION_ROOM:
                            if (MANAGER_QUERY_ROOM(managerPtr, id) >= 0) {
                                price = MANAGER_QUERY_ROOM_PRICE(managerPtr, id);
                            }
                            break;
                        default:
                            assert(0);
                    }
                    if (price > maxPrices[t]) {
                        maxPrices[t] = price;
                        maxIds[t] = id;
                        isFound = TRUE;
                    }
                } /* for n */
                if (isFound) {
                    MANAGER_ADD_CUSTOMER(managerPtr, customerId);
                }
                if (maxIds[RESERVATION_CAR] > 0) {
                    MANAGER_RESERVE_CAR(managerPtr,
                                        customerId, maxIds[RESERVATION_CAR]);
                }
                if (maxIds[RESERVATION_FLIGHT] > 0) {
                    MANAGER_RESERVE_FLIGHT(managerPtr,
                                           customerId, maxIds[RESERVATION_FLIGHT]);
                }
                if (maxIds[RESERVATION_ROOM] > 0) {
                    MANAGER_RESERVE_ROOM(managerPtr,
                                         customerId, maxIds[RESERVATION_ROOM]);
                }

                for (n = 0; n < numQuery; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        UNLOCK(table_locks[type][id]);
                    }
                }
                UNLOCK(table_locks[3][cl_id]);
                break;
            }

            case ACTION_DELETE_CUSTOMER: {
                long customerId = random_generate(randomPtr) % queryRange + 1;

                cl_id = customerTablePtr->hash((void*)customerId) % customerTablePtr->numBucket;
                LOCK(table_locks[3][cl_id]);
                customer_t* customerPtr = (customer_t*)TMMAP_FIND(managerPtr->customerTablePtr, customerId);
                if (customerPtr == NULL) {
                    UNLOCK(table_locks[3][cl_id]);
                    break;
                }

                list_t* reservationInfoListPtr;
                list_iter_t it;
                reservationInfoListPtr = customerPtr->reservationInfoListPtr;
                long size = TMLIST_GETSIZE(reservationInfoListPtr);
                long n = 0;
                TMLIST_ITER_RESET(&it, reservationInfoListPtr);
                while (TMLIST_ITER_HASNEXT(&it, reservationInfoListPtr)) {
                    reservation_info_t* reservationInfoPtr;
                    reservationInfoPtr =
                        (reservation_info_t*)TMLIST_ITER_NEXT(&it, reservationInfoListPtr);
                    long type = reservationInfoPtr->type;
                    long id = reservationInfoPtr->id;
                    rl[n].type = type;
                    rl[n].id = reservationTables[type]->hash((void*)id)
                                    % reservationTables[type]->numBucket;
                    n++;
                }
                qsort(rl, size, sizeof(reserve_lock_t), compare_reserve_lock);

                for (n = 0; n < size; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        LOCK(table_locks[type][id]);
                    }
                }

                long bill = MANAGER_QUERY_CUSTOMER_BILL(managerPtr, customerId);
                if (bill >= 0) {
                    MANAGER_DELETE_CUSTOMER(managerPtr, customerId);
                }

                for (n = 0; n < size; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        UNLOCK(table_locks[type][id]);
                    }
                }
                UNLOCK(table_locks[3][cl_id]);

                break;
            }

            case ACTION_UPDATE_TABLES: {
                long numUpdate = random_generate(randomPtr) % numQueryPerTransaction + 1;
                long n;
                for (n = 0; n < numUpdate; n++) {
                    types[n] = random_generate(randomPtr) % NUM_RESERVATION_TYPE;
                    ids[n] = (random_generate(randomPtr) % queryRange) + 1;
                    ops[n] = random_generate(randomPtr) % 2;
                    if (ops[n]) {
                        prices[n] = ((random_generate(randomPtr) % 5) * 10) + 50;
                    }
                }

                for (n = 0; n < numUpdate; n++) {
                    long t = types[n];
                    long id = ids[n];
                    rl[n].type = t;
                    rl[n].id = reservationTables[t]->hash((void*)id)
                                        % reservationTables[t]->numBucket;
                }
                qsort(rl, numUpdate, sizeof(reserve_lock_t), compare_reserve_lock);

                for (n = 0; n < numUpdate; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        LOCK(table_locks[type][id]);
                    }
                }

                for (n = 0; n < numUpdate; n++) {
                    long t = types[n];
                    long id = ids[n];
                    long doAdd = ops[n];
                    if (doAdd) {
                        long newPrice = prices[n];
                        switch (t) {
                            case RESERVATION_CAR:
                                MANAGER_ADD_CAR(managerPtr, id, 100, newPrice);
                                break;
                            case RESERVATION_FLIGHT:
                                MANAGER_ADD_FLIGHT(managerPtr, id, 100, newPrice);
                                break;
                            case RESERVATION_ROOM:
                                MANAGER_ADD_ROOM(managerPtr, id, 100, newPrice);
                                break;
                            default:
                                assert(0);
                        }
                    } else { /* do delete */
                        switch (t) {
                            case RESERVATION_CAR:
                                MANAGER_DELETE_CAR(managerPtr, id, 100);
                                break;
                            case RESERVATION_FLIGHT:
                                MANAGER_DELETE_FLIGHT(managerPtr, id);
                                break;
                            case RESERVATION_ROOM:
                                MANAGER_DELETE_ROOM(managerPtr, id, 100);
                                break;
                            default:
                                assert(0);
                        }
                    }
                }

                for (n = 0; n < numUpdate; n++) {
                    long type = rl[n].type;
                    long id = rl[n].id;
                    if (!isIncludeSamePrevious(rl, n)) {
                        UNLOCK(table_locks[type][id]);
                    }
                }
                break;
            }

            default:
                assert(0);

        } /* switch (action) */

    } /* for i */

    P_FREE(rl);

    P_FREE(types);
    P_FREE(ids);
    P_FREE(ops);
    P_FREE(prices);

    TM_THREAD_EXIT();
}

#endif

/* =============================================================================
 *
 * End of client.c
 *
 * =============================================================================
 */
