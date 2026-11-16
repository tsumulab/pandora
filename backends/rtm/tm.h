#ifndef TM_RTM_H
#define TM_RTM_H 1

#  include <stdio.h>

#  define MAIN(argc, argv)          int main (int argc, char** argv)
#  define MAIN_RETURN(val)          return val

#  define GOTO_SIM()                /* nothing */
#  define GOTO_REAL()               /* nothing */
#  define IS_IN_SIM()               (0)

#  define SIM_GET_NUM_CPU(var)      /* nothing */

#  define TM_PRINTF           printf
#  define TM_PRINT0           printf
#  define TM_PRINT1           printf
#  define TM_PRINT2           printf
#  define TM_PRINT3           printf

#if defined(USE_TLMALLOC)
#  include "memory.h"
#  define P_MEMORY_STARTUP(numThread)   do { \
                                          bool_t status; \
                                          status = memory_init((numThread), \
                                                               ((1<<28) / numThread), \
                                                               2); \
                                          assert(status); \
                                        } while (0) /* enforce comma */
#  define P_MEMORY_SHUTDOWN()           memory_destroy()
#else
#  define P_MEMORY_STARTUP(numThread)   /* nothing */
#  define P_MEMORY_SHUTDOWN()           /* nothing */
#endif

#  include <assert.h>
#  include "memory.h"
#  include "thread.h"
#  include "types.h"

#  include <immintrin.h>
#  include <rtmintrin.h>

#  define TM_ARG                /* nothing */
#  define TM_ARG_ALONE          /* nothing */
#  define TM_ARGDECL            /* nothing */
#  define TM_ARGDECL_ALONE      /* nothing */
#  define TM_CALLABLE           /* nothing */

#  if defined(LIBFIBRE)
#    define ULT_INIT(numCore)     cfibre_init_n(1, numCore)
#  else
#    define ULT_INIT(numCore)     /* nothing */
#  endif

#  define TM_STARTUP(numCore)           ULT_INIT(numCore)
#  define TM_SHUTDOWN()                 print_stats()

#  define TM_THREAD_ENTER()             /* nothing */
#  define TM_THREAD_EXIT()              /* nothing */

#if defined(USE_TLMALLOC)
#  include "thread.h"
#  include "memory.h"
#  define P_MALLOC(size)                memory_get(thread_getId(), size)
#  define P_FREE(ptr)                   /* TODO: thread local free is non-trivial */
#  define TM_MALLOC(size)               memory_get(thread_getId(), size)
#  define TM_FREE(ptr)                  /* TODO: thread local free is non-trivial */
#else
#  define P_MALLOC(size)                malloc(size)
#  define P_FREE(ptr)                   free(ptr)
#  define TM_MALLOC(size)               malloc(size)
#  define TM_FREE(ptr)                  free(ptr)
#endif

#if 0
#  define ESCAPE_YELLOW "\x1b[33m"
#  define ESCAPE_NORMAL "\x1b[0m"

#  define DEBUG(...)    { \
                          printf(ESCAPE_YELLOW); \
                          printf(__VA_ARGS__); \
                          printf(ESCAPE_NORMAL); \
                          char buf[10]; \
                          /* fgets(buf, 10, stdin); */ \
                        }
#  define DEBUG_PRINTF(...) { \
                              printf(ESCAPE_YELLOW); \
                              printf(__VA_ARGS__); \
                              printf(ESCAPE_NORMAL); \
                            }
#else
#  define DEBUG(...)    /* nothing */
#  define DEBUG_PRINTF(...) /* nothing */
#endif


#if defined(__x86_64__) || defined(__i386)
#  define L1_CACHE_SIZE         (32L*1024L)
#  define L1_BLOCK_SIZE         64L // 64 bytes per block/line
#  define L1_BLOCKS_PER_SET      8L // 8-way set associative
#  define L1_WORDS_PER_BLOCK     8L
#  define L1_NUM_SETS           (L1_CACHE_SIZE/(L1_BLOCK_SIZE*L1_BLOCKS_PER_SET))
#  define L1_SET_BLOCK_OFFSET   ((L1_NUM_SETS*L1_BLOCK_SIZE)/sizeof(long))
#  define CACHE_ALIGNMENT       (L1_NUM_SETS*L1_BLOCK_SIZE)
#endif

#ifndef __ALIGN__
#define __ALIGN__ __attribute__((aligned(CACHE_ALIGNMENT)))
#endif

#define lock_t volatile long
#define LOCK_INITIALIZER 0
static lock_t __htm_global_lock __ALIGN__ = LOCK_INITIALIZER;

#define IS_LOCKED(l) (__atomic_load_n(l, __ATOMIC_SEQ_CST) == 1)
#define LOCK(l)      while (__atomic_exchange_n(l,1, __ATOMIC_SEQ_CST)) {}
#define UNLOCK(l)    __atomic_store_n(l, 0, __ATOMIC_RELEASE)

#define HTM_ABORT_REASON(s)     (s & 0x3E)
#define HTM_ABORT_PERSISTENT(s) (!(s & _XABORT_RETRY))

#if defined(THREAD_SWITCHING)
#  if defined(LIBFIBRE)
#  define SWITCH_THREAD()     cfibre_yield()
#  else
#  define SWITCH_THREAD()     pthread_yield()
#  endif
#else
#  define SWITCH_THREAD()     /* nothing */
#endif

#define TM_START()    do { \
                        int tries = 16; \
                        int per_cnt = 2; \
                        while(1) { \
                          unsigned int __status = _xbegin(); \
                          if (__status == _XBEGIN_STARTED) { \
                            if(IS_LOCKED(&__htm_global_lock)) \
                              _xabort(0xab); \
                            break; \
                          } else { \
                            unsigned int abort_reason __ALIGN__ = HTM_ABORT_REASON(__status); \
                            inc_abortreason(abort_reason, cpu_getId()); \
                            DEBUG_PRINTF("> aborted in %s at %s:%d \n", __func__, __FILE__, __LINE__); \
                            if (HTM_ABORT_PERSISTENT(abort_reason)) { \
                              per_cnt--; \
                            } else {\
                              tries--; \
                              SWITCH_THREAD(); \
                            } \
                            if (per_cnt <= 0 || tries <= 0) { \
                              LOCK(&__htm_global_lock); \
                              break; \
                            } \
                          } \
                          while (IS_LOCKED(&__htm_global_lock)) {SWITCH_THREAD();} \
                        } \
                      } while (0)

#define TM_END()    do {\
                      if(!IS_LOCKED(&__htm_global_lock)) { \
                        _xend(); \
                        HTM_Stats[cpu_getId()][0]++; \
                      } else { \
                        UNLOCK(&__htm_global_lock); \
                        HTM_Stats[cpu_getId()][1]++; \
                      } \
                    } while (0)

#  define TM_BEGIN()                  TM_START()
#  define TM_BEGIN_ID(x)              TM_BEGIN()
#  define TM_BEGIN_RO()               TM_BEGIN()
#  define TM_BEGIN_RO_ID()            TM_BEGIN()
#  define TM_RESTART()                _xabort(0xab);

#  define TM_EARLY_RELEASE(var)       /* nothing */

#  define TM_SHARED_READ(var)         (var)
#  define TM_SHARED_READ_P(var)       (var)
#  define TM_SHARED_READ_F(var)       (var)
#  define TM_SHARED_READ_D(var)       (var)

#  define TM_SHARED_WRITE(var,val)    ({var=val;var;})
#  define TM_SHARED_WRITE_P(var,val)  ({var=val;var;})
#  define TM_SHARED_WRITE_F(var,val)  ({var=val;var;})
#  define TM_SHARED_WRITE_D(var,val)  ({var=val;var;})

#  define TM_LOCAL_WRITE(var,val)     ({var=val;var;})
#  define TM_LOCAL_WRITE_P(var,val)   ({var=val;var;})
#  define TM_LOCAL_WRITE_F(var,val)   ({var=val;var;})
#  define TM_LOCAL_WRITE_D(var,val)   ({var=val;var;})

// enum {
//     END_Tx,         // 0
//     END_LOCK,       // 1
//     CONFLICT_ABORT, // 2
//     CAPACITY_ABORT, // 3
//     ILLEGAL,        // 4
//     NESTED,         // 5
//     ZERO,           // 6
//     OTHER,          // 7
// };

// static volatile unsigned long HTM_Stats[40][8] = {0};
// static volatile unsigned long total_HTM_Stats[8] = {0};

// static
// void inc_abortreason(unsigned int abortcode, int cpuid) {
//     if (abortcode & _XABORT_CONFLICT) {
//         HTM_Stats[cpuid][CONFLICT_ABORT]++;
//     } else if (abortcode & _XABORT_CAPACITY) {
//         HTM_Stats[cpuid][CAPACITY_ABORT]++;
//     } else if (abortcode & _XABORT_DEBUG) {
//         HTM_Stats[cpuid][ILLEGAL]++;
//     } else if (abortcode & _XABORT_NESTED) {
//         HTM_Stats[cpuid][NESTED]++;
//     } else if (abortcode == 0) {
//         HTM_Stats[cpuid][ZERO]++;
//     } else {
//         HTM_Stats[cpuid][OTHER]++;
//     }
// }

// static
// void print_stats() {
//     for (int i = 0; i < 40; i++) {
//         total_HTM_Stats[END_Tx]         += HTM_Stats[i][END_Tx];
//         total_HTM_Stats[END_LOCK]       += HTM_Stats[i][END_LOCK];
//         total_HTM_Stats[CONFLICT_ABORT] += HTM_Stats[i][CONFLICT_ABORT];
//         total_HTM_Stats[CAPACITY_ABORT] += HTM_Stats[i][CAPACITY_ABORT];
//         total_HTM_Stats[ILLEGAL]        += HTM_Stats[i][ILLEGAL];
//         total_HTM_Stats[NESTED]         += HTM_Stats[i][NESTED];
//         total_HTM_Stats[ZERO]           += HTM_Stats[i][ZERO];
//         total_HTM_Stats[OTHER]          += HTM_Stats[i][OTHER];
//     }
//     printf("\n====================================\n");
//     printf(" # HTMEnd_Tx    : %10lu\n", total_HTM_Stats[END_Tx]);
//     printf(" # HTMEnd_LOCK  : %10lu\n", total_HTM_Stats[END_LOCK]);
//     printf(" # HTMTxConflict: %10lu\n", total_HTM_Stats[CONFLICT_ABORT]);
//     printf(" # HTMCapacity  : %10lu\n", total_HTM_Stats[CAPACITY_ABORT]);
//     printf(" # HTMIllegal   : %10lu\n", total_HTM_Stats[ILLEGAL]);
//     printf(" # HTMNested    : %10lu\n", total_HTM_Stats[NESTED]);
//     printf(" # HTM0         : %10lu\n", total_HTM_Stats[ZERO]);
//     printf(" # HTMother     : %10lu\n", total_HTM_Stats[OTHER]);
//     printf("====================================\n");
// }


#endif
