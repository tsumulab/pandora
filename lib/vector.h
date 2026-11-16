/* =============================================================================
 *
 * vector.h
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


#ifndef VECTOR_H
#define VECTOR_H 1


#include "tm.h"
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct vector {
    long size;
    long capacity;
    void** elements;
} vector_t;


/* =============================================================================
 * vector_alloc
 * -- Returns NULL if failed
 * =============================================================================
 */
vector_t*
vector_alloc (long initCapacity);


/* =============================================================================
 * Pvector_alloc
 * -- Returns NULL if failed
 * =============================================================================
 */
vector_t*
Pvector_alloc (long initCapacity);


/* =============================================================================
 * TMvector_alloc
 * -- Returns NULL if failed
 * =============================================================================
 */
vector_t*
TMvector_alloc (TM_ARGDECL long initCapacity);


/* =============================================================================
 * vector_free
 * =============================================================================
 */
void
vector_free (vector_t* vectorPtr);


/* =============================================================================
 * Pvector_free
 * =============================================================================
 */
void
Pvector_free (vector_t* vectorPtr);


/* =============================================================================
 * TMvector_free
 * =============================================================================
 */
void
TMvector_free (TM_ARGDECL vector_t* vectorPtr);


/* =============================================================================
 * vector_at
 * -- Returns NULL if failed
 * =============================================================================
 */
void*
vector_at (vector_t* vectorPtr, long i);


/* =============================================================================
 * TMvector_at
 * -- Returns NULL if failed
 * =============================================================================
 */
void*
TMvector_at (TM_ARGDECL vector_t* vectorPtr, long i);


/* =============================================================================
 * vector_write_at
 * -- Returns FALSE if fail, else TRUE
 * =============================================================================
 */
bool_t
vector_write_at (vector_t* vectorPtr, long i, void* dataPtr);


/* =============================================================================
 * vector_write_at
 * -- Returns FALSE if fail, else TRUE
 * =============================================================================
 */
bool_t
TMvector_write_at (TM_ARGDECL vector_t* vectorPtr, long i, void* dataPtr);


/* =============================================================================
 * vector_pushBack
 * -- Returns FALSE if fail, else TRUE
 * =============================================================================
 */
bool_t
vector_pushBack (vector_t* vectorPtr, void* dataPtr);


/* =============================================================================
 * Pvector_pushBack
 * -- Returns FALSE if fail, else TRUE
 * =============================================================================
 */
bool_t
Pvector_pushBack (vector_t* vectorPtr, void* dataPtr);


/* =============================================================================
 * TMvector_pushBack
 * -- Returns FALSE if fail, else TRUE
 * =============================================================================
 */
bool_t
TMvector_pushBack (TM_ARGDECL vector_t* vectorPtr, void* dataPtr);


/* =============================================================================
 * vector_popBack
 * -- Returns NULL if fail, else returns last element
 * =============================================================================
 */
void*
vector_popBack (vector_t* vectorPtr);


/* =============================================================================
 * TMvector_popBack
 * -- Returns NULL if fail, else returns last element
 * =============================================================================
 */
void*
TMvector_popBack (TM_ARGDECL vector_t* vectorPtr);


/* =============================================================================
 * vector_getSize
 * =============================================================================
 */
long
vector_getSize (vector_t* vectorPtr);


/* =============================================================================
 * TMvector_getSize
 * =============================================================================
 */
long
TMvector_getSize (TM_ARGDECL vector_t* vectorPtr);


/* =============================================================================
 * vector_clear
 * =============================================================================
 */
void
vector_clear (vector_t* vectorPtr);


/* =============================================================================
 * TMvector_clear
 * =============================================================================
 */
void
TMvector_clear (TM_ARGDECL vector_t* vectorPtr);


/* =============================================================================
 * vector_sort
 * =============================================================================
 */
void
vector_sort (vector_t* vectorPtr, int (*compare) (const void*, const void*));


/* =============================================================================
 * vector_copy
 * =============================================================================
 */
bool_t
vector_copy (vector_t* dstVectorPtr, vector_t* srcVectorPtr);


/* =============================================================================
 * Pvector_copy
 * =============================================================================
 */
bool_t
Pvector_copy (vector_t* dstVectorPtr, vector_t* srcVectorPtr);


#define PVECTOR_ALLOC(n)            Pvector_alloc(n)
#define PVECTOR_FREE(v)             Pvector_free(v)
#define PVECTOR_PUSHBACK(v, d)      Pvector_pushBack(v, d)
#define PVECTOR_POPBACK(v)          vector_popBack(v)
#define PVECTOR_AT(v, i)            vector_at(v, i)
#define PVECTOR_WRITE_AT(v, i, d)   vector_write_at(v, i, d)
#define PVECTOR_GETSIZE(v)          vector_getSize(v)
#define PVECTOR_CLEAR(v)            vector_clear(v)
#define PVECTOR_SORT(v, cmp)        vector_sort(v, cmp)
#define PVECTOR_COPY(dst, src)      Pvector_copy(dst, src)

#define TMVECTOR_ALLOC(n)           TMvector_alloc(TM_ARG n)
#define TMVECTOR_FREE(v)            TMvector_free(TM_ARG v)
#define TMVECTOR_PUSHBACK(v, d)     TMvector_pushBack(TM_ARG v, d)
#define TMVECTOR_POPBACK(v)         TMvector_popBack(TM_ARG v)
#define TMVECTOR_AT(v, i)           TMvector_at(TM_ARG v, i)
#define TMVECTOR_WRITE_AT(v, i, d)  TMvector_write_at(TM_ARG v, i, d)
#define TMVECTOR_GETSIZE(v)         TMvector_getSize(TM_ARG v)
#define TMVECTOR_CLEAR(v)           TMvector_clear(TM_ARG v)

#ifdef __cplusplus
}
#endif


#endif /* VECTOR_H */


/* =============================================================================
 *
 * End of vector.h
 *
 * =============================================================================
 */
