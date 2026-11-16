#ifndef TM_POWERHTM_H
#define TM_POWERHTM_H 1

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <htmintrin.h>
#include <htmxlintrin.h>
#include <sched.h>
#include "memory.h"
#include "thread.h"
#include "types.h"

#define pthread_yield sched_yield
#ifdef LIBFIBRE
#error "POWER does not support libfibre."
#endif
/* -----------------------------------------------------------
 * Basic defines
 * ----------------------------------------------------------- */
#define MAIN(argc, argv)          int main (int argc, char** argv)
#define MAIN_RETURN(val)          return val

#define GOTO_SIM()                /* nothing */
#define GOTO_REAL()               /* nothing */
#define IS_IN_SIM()               (0)

#define SIM_GET_NUM_CPU(var)      /* nothing */

#define TM_PRINTF                 printf
#define TM_PRINT0                 printf
#define TM_PRINT1                 printf
#define TM_PRINT2                 printf
#define TM_PRINT3                 printf

/* -----------------------------------------------------------
 * Memory allocation (optionally thread-local)
 * ----------------------------------------------------------- */
#if defined(USE_TLMALLOC)
#  define P_MEMORY_STARTUP(numThread)   do { \
                                          bool_t status; \
                                          status = memory_init((numThread), ((1<<28) / (numThread)), 2); \
                                          assert(status); \
                                        } while (0)
#  define P_MEMORY_SHUTDOWN()           memory_destroy()
#  define P_MALLOC(size)                memory_get(thread_getId(), size)
#  define P_FREE(ptr)                   /* TODO: Implement thread-local free */
#  define TM_MALLOC(size)               memory_get(thread_getId(), size)
#  define TM_FREE(ptr)                  /* TODO: Implement thread-local free */
#else
#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */
#  define P_MALLOC(size)                malloc(size)
#  define P_FREE(ptr)                   free(ptr)
#  define TM_MALLOC(size)               malloc(size)
#  define TM_FREE(ptr)                  free(ptr)
#endif

/* -----------------------------------------------------------
 * Debug (optional)
 * ----------------------------------------------------------- */
#if 0
#  define ESCAPE_YELLOW "\x1b[33m"
#  define ESCAPE_NORMAL "\x1b[0m"
#  define DEBUG(...)    { printf(ESCAPE_YELLOW); printf(__VA_ARGS__); printf(ESCAPE_NORMAL); }
#  define DEBUG_PRINTF(...) DEBUG(__VA_ARGS__)
#else
#  define DEBUG(...)              /* nothing */
#  define DEBUG_PRINTF(...)       /* nothing */
#endif

/* -----------------------------------------------------------
 * Cache line alignment
 * ----------------------------------------------------------- */
#if defined(__x86_64__) || defined(__i386)
#  define L1_CACHE_SIZE         (32L * 1024L)
#  define L1_BLOCK_SIZE         64L
#else
#  define L1_CACHE_SIZE         (512L * 1024L)
#  define L1_BLOCK_SIZE         128L
#endif

#define L1_BLOCKS_PER_SET        8L
#define L1_NUM_SETS             (L1_CACHE_SIZE / (L1_BLOCK_SIZE * L1_BLOCKS_PER_SET))
#define CACHE_ALIGNMENT         (L1_NUM_SETS * L1_BLOCK_SIZE)

#ifndef __ALIGN__
#define __ALIGN__ __attribute__((aligned(CACHE_ALIGNMENT)))
#endif

extern __thread int RetryCtr;
extern __thread int HTMEnd_LOCK_Tra;
extern __thread int HTMEnd_LOCK_Per;

/* -----------------------------------------------------------
 * Lock definitions
 * ----------------------------------------------------------- */
typedef volatile long lock_t;

#define LOCK_INITIALIZER 0
static lock_t __htm_global_lock __ALIGN__ = LOCK_INITIALIZER;

#define IS_LOCKED(l) (__atomic_load_n((l), __ATOMIC_SEQ_CST) == 1)
#define LOCK(l)      while (__atomic_exchange_n((l), 1, __ATOMIC_SEQ_CST)) pthread_yield()
#define UNLOCK(l)    __atomic_store_n((l), 0, __ATOMIC_SEQ_CST)

/* -----------------------------------------------------------
 * Abort reason checks
 * ----------------------------------------------------------- */
#define TM_ABORT_TYPE             texasr_t

#define TX_ABORTED(s)             _TEXASR_ABORT(s)
#define IS_ABORT_TX_CONFLICT(s)    _TEXASR_TRANSACTION_CONFLICT(s)
#define IS_ABORT_CAPACITY(s)       _TEXASR_FOOTPRINT_OVERFLOW(s)
#define IS_ABORT_ILLEGAL(s)        _TEXASR_DISALLOWED(s)
#define IS_ABORT_NESTED(s)         _TEXASR_NESTING_OVERFLOW(s)
#define IS_ABORT_NON_TX_CONFLICT(s) _TEXASR_NON_TRANSACTIONAL_CONFLICT(s)
#define IS_ABORT_TLB_CONFLICT(s)   _TEXASR_TRANSLATION_INVALIDATION_CONFLICT(s)
#define IS_ABORT_IMPL_SPECIFIC(s) _TEXASR_IMPLEMENTAION_SPECIFIC(s)
#define IS_ABORT_FETCH_CONFLICT(s) _TEXASR_INSTRUCTION_FETCH_CONFLICT(s)
#define IS_ABORT_PERSISTENT(s)     _TEXASR_FAILURE_PERSISTENT(s)
#define IS_ABORT_SUSPENDED_CONFLICT(s) _TEXASR_SELF_INDUCED_CONFLICT(s)

#define HTM_ABORT_REASON(s)        ((texasr_t)__builtin_get_texasr())
#define HTM_ABORT_PERSISTENT(s)    (IS_ABORT_PERSISTENT(s))

/* -----------------------------------------------------------
 * Transaction macros
 * ----------------------------------------------------------- */
#if defined(THREAD_SWITCHING)
#  if defined(LIBFIBRE)
#    define SWITCH_THREAD()     cfibre_yield()
#  else
#    define SWITCH_THREAD()     pthread_yield()
#  endif
#else
#  define SWITCH_THREAD()        /* nothing */
#endif

#define TM_START() do { \
                        int persistent_counter = 0; \
                        int transient_counter = 0; \
                        while (1) { \
                            u_int32_t __tx_status = __builtin_tbegin(0); \
                            if (__tx_status != 0) { \
                                if (IS_LOCKED(&__htm_global_lock)) { \
                                    __builtin_tabort(0xab); \
                                } \
                                break; \
                            } else { \
                                if (IS_LOCKED(&__htm_global_lock)) { \
                                    while (IS_LOCKED(&__htm_global_lock)) SWITCH_THREAD(); \
                                } else { \
                                    texasr_t abort_reason __ALIGN__ = HTM_ABORT_REASON(__tx_status); \
                                    inc_abortreason(abort_reason, cpu_getId()); \
                                    if (HTM_ABORT_PERSISTENT(abort_reason)) { \
                                        persistent_counter++; \
                                    } else { \
                                        transient_counter++; \
                                        SWITCH_THREAD(); \
                                    } \
                                    if (transient_counter >= 30) { \
                                        HTMEnd_LOCK_Tra++; \
                                        LOCK(&__htm_global_lock); \
                                        break; \
                                    } \
                                    if (persistent_counter >= 1) { \
                                        HTMEnd_LOCK_Per++; \
                                        LOCK(&__htm_global_lock); \
                                        break; \
                                    } \
                                } \
                                RetryCtr++; \
                            } \
                        } \
                    } while (0)

#define TM_END() do { \
                    if (IS_LOCKED(&__htm_global_lock)) { \
                        UNLOCK(&__htm_global_lock); \
                        HTM_Stats[cpu_getId()][1]++;  \
                    } else { \
                        __builtin_tend(0); \
                        HTM_Stats[cpu_getId()][0]++;\
                    } \
                } while (0)

#define TM_BEGIN()                  TM_START()
#define TM_BEGIN_ID(x)              TM_BEGIN()
#define TM_BEGIN_RO()               TM_BEGIN()
#define TM_BEGIN_RO_ID()            TM_BEGIN()
#define TM_RESTART()               

/* -----------------------------------------------------------
 * TM Memory Access
 * ----------------------------------------------------------- */
#define TM_EARLY_RELEASE(var)           /* nothing */

#define TM_SHARED_READ(var)             (var)
#define TM_SHARED_READ_P(var)           (var)
#define TM_SHARED_READ_F(var)           (var)
#define TM_SHARED_READ_D(var)           (var)

#define TM_SHARED_WRITE(var, val)       ({ (var) = (val); (var); })
#define TM_SHARED_WRITE_P(var, val)     ({ (var) = (val); (var); })
#define TM_SHARED_WRITE_F(var, val)     ({ (var) = (val); (var); })
#define TM_SHARED_WRITE_D(var, val)     ({ (var) = (val); (var); })

#define TM_LOCAL_WRITE(var, val)        ({ (var) = (val); (var); })
#define TM_LOCAL_WRITE_P(var, val)      ({ (var) = (val); (var); })
#define TM_LOCAL_WRITE_F(var, val)      ({ (var) = (val); (var); })
#define TM_LOCAL_WRITE_D(var, val)      ({ (var) = (val); (var); })

/* -----------------------------------------------------------
 * Misc
 * ----------------------------------------------------------- */
#if defined(LIBFIBRE)
#  define ULT_INIT(numCore)        cfibre_init_n(1, numCore)
#else
#  define ULT_INIT(numCore)        /* nothing */
#endif

#define TM_STARTUP(numCore)         ULT_INIT(numCore)
#define TM_SHUTDOWN()               print_stats()

#define TM_THREAD_ENTER()           /* nothing */
#define TM_THREAD_EXIT()            /* nothing */

#  define TM_ARG                /* nothing */
#  define TM_ARG_ALONE          /* nothing */
#  define TM_ARGDECL            /* nothing */
#  define TM_ARGDECL_ALONE      /* nothing */
#  define TM_CALLABLE           /* nothing */

#endif /* TM_POWERHTM_H */
