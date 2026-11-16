#ifndef TM_TINYSTM_H
#define TM_TINYSTM_H 1

#ifdef TINYSTM_STATS
#include <pthread.h>
#include "thread.h"
#include <time.h>
#include <stdint.h>

#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf





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

#  define TM_ARG                        __tx,
#  define TM_ARG_ALONE                  __tx
#  define TM_ARGDECL                    struct stm_tx* __tx,
#  define TM_ARGDECL_ALONE              struct stm_tx* __tx
#  define TM_CALLABLE                   /* nothing */

#  include <mod_mem.h>
#  include <mod_stats.h>

#  if defined(LIBFIBRE)
#    define ULT_INIT(numCore)     cfibre_init_n(1, numCore)
#  else
#    define ULT_INIT(numCore)     /* nothing */
#  endif

#  define TM_STARTUP(numCore)     if (sizeof(long) != sizeof(void *)) { \
                                    fprintf(stderr, "Error: unsupported long and pointer sizes\n"); \
                                    exit(1); \
                                  } \
                                  ULT_INIT(numCore); \
                                  stm_init(); \
                                  mod_mem_init(0); \
                                  if (getenv("STM_STATS") != NULL) { \
                                    mod_stats_init(); \
                                  }
#  define TM_SHUTDOWN()           if (getenv("STM_STATS") != NULL) { \
                                    unsigned long u; \
                                    if (stm_get_global_stats("global_nb_commits", &u) != 0) \
                                      printf("#commits    : %lu\n", u); \
                                    if (stm_get_global_stats("global_nb_aborts", &u) != 0) \
                                      printf("#aborts     : %lu\n", u); \
                                    if (stm_get_global_stats("global_max_retries", &u) != 0) \
                                      printf("Max retries : %lu\n", u); \
                                  } \
                                  { \
                                    uint64_t total = 0; \
                                    uint64_t total_r = 0; \
                                    uint64_t total_r_c = 0; \
                                    uint64_t total_w = 0; \
                                    uint64_t total_w_c = 0; \
                                    uint64_t total_c = 0; \
                                    uint64_t total_a = 0; \
                                    int cpu; \
                                    printf("\n===== TM Tx Time =====\n"); \
                                    for (cpu = 0; cpu < MAX_CPU; cpu++) { \
                                      if (tm_cpu_tx_time[cpu] != 0) { \
                                        printf("CPU %d : %lu ns\n", \
                                               cpu, \
                                               (unsigned long)tm_cpu_tx_time[cpu]); \
                                        total += tm_cpu_tx_time[cpu]; \
                                      } \
                                    } \
                                    printf("\n===== TM read/write set  =====\n"); \
                                    for (cpu = 0; cpu < MAX_CPU; cpu++) { \
                                      if (tm_cpu_readset_size[cpu] != 0) { \
                                        printf("CPU %d : Read=%lu \n", \
                                               cpu, \
                                               (unsigned long)tm_cpu_readset_size[cpu]); \
                                        total_r += tm_cpu_readset_size[cpu]; \
                                        total_r_c += tm_read_tx_counts[cpu]; \
                                      } \
                                    } \
                                    for (cpu = 0; cpu < MAX_CPU; cpu++) { \
                                      if (tm_cpu_writeset_size[cpu] != 0) { \
                                        printf("CPU %d : Write=%lu\n", \
                                               cpu, \
                                               (unsigned long)tm_cpu_writeset_size[cpu]); \
                                        total_w += tm_cpu_writeset_size[cpu]; \
                                        total_w_c += tm_write_tx_counts[cpu]; \
                                      } \
                                    } \
                                    for (cpu = 0; cpu < MAX_CPU; cpu++) { \
                                      total_c += tm_commit_tx_counts[cpu]; \
                                      total_a += tm_abort_tx_counts[cpu]; \
                                    } \
                                    printf("Total Tx Time : %lu ns\n", \
                                           (unsigned long)total); \
                                    printf("Total Tx :  %lu\n",(unsigned long)total_c); \
                                    printf("Total Tx abort: %lu\n",(unsigned long)total_a); \
                                    printf("Total Tx Time(second) : %.6f s\n",(double)total / 1000000000.0); \
                                    printf("Total Read set : %lu\n", \
                                           (unsigned long)total_r); \
                                    printf("Total Write set : %lu\n", \
                                           (unsigned long)total_w); \
                                    printf("Total Read Tx : %lu\n", \
                                           (unsigned long)total_r_c); \
                                    printf("Total Write Tx : %lu\n", \
                                           (unsigned long)total_w_c); \
                                    printf("Average Read set : %.6f\n", \
                                           (double)(total_r_c == 0 ? 0 : (double)total_r / (double)total_c)); \
                                    printf("Average Write set : %.6f\n", \
                                           (double)(total_w_c == 0 ? 0 : (double)total_w / (double)total_c)); \
                                  } \
                                  stm_exit()

# define TM_THREAD_ENTER()        struct stm_tx *__tx = stm_init_thread()
# define TM_THREAD_EXIT()         stm_exit_thread_tx(__tx)

#if defined(USE_TLMALLOC)
#  include "thread.h"
#  include "memory.h"
#  define P_MALLOC(size)          memory_get(thread_getId(), size)
#  define P_FREE(ptr)             /* TODO: thread local free is non-trivial */
#  define TM_MALLOC(size)         memory_get(thread_getId(), size)
#  define TM_FREE(ptr)            /* TODO: thread local free is non-trivial */
// #  define TM_MALLOC(size)         stm_malloc(size)
// #  define TM_FREE(ptr)            stm_free(ptr, sizeof(stm_word_t))
#else
#  define P_MALLOC(size)          malloc(size)
#  define P_FREE(ptr)             free(ptr)
#  define TM_MALLOC(size)         stm_malloc(size)
#  define TM_FREE(ptr)            stm_free(ptr, sizeof(stm_word_t))
// #  define TM_MALLOC(size)         malloc(size)
// #  define TM_FREE(ptr)            free(ptr)
#endif

#if defined(THREAD_SWITCHING)
#  if defined(LIBFIBRE)
#    define SWITCH_THREAD()     cfibre_yield()
#  elif defined(MASSIVE)
#    define SWITCH_THREAD()     myth_yield()
#  else
#    define SWITCH_THREAD()     pthread_yield()
#  endif
#else
#  define SWITCH_THREAD()       /* nothing */
#endif

#if 0
#  define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#  define DEBUG_PRINTF(...) /* nothing */
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
#  define DEBUG_NS(...) { \
                          printf(ESCAPE_YELLOW); \
                          printf(__VA_ARGS__); \
                          printf(ESCAPE_NORMAL); \
                        }
#else
#  define DEBUG(...)    /* nothing */
#  define DEBUG_NS(...) /* nothing */
#endif

#define TM_GET_TIME(var)                      \
do {                                          \
    struct timespec __ts;                     \
    clock_gettime(CLOCK_MONOTONIC, &__ts);    \
    (var) = (uint64_t)__ts.tv_sec *           \
            1000000000ULL +                   \
            (uint64_t)__ts.tv_nsec;           \
} while (0)

#  define TM_START(ro)          do { \
                                  TM_GET_TIME(tm_tx_start_time); \
                                  tm_read_set_size = 0; \
                                  tm_write_set_size = 0; \
                                  stm_tx_attr_t _a; \
                                  _a.attrs = (int32_t)(ro << 16); \
                                  sigjmp_buf *_e = stm_start_tx(__tx, _a); \
                                  if (_e != NULL) { \
                                    int reason = sigsetjmp(*_e, 0); \
                                    if (reason > 0) { \
                                      uint64_t __now; \
                                      TM_GET_TIME(__now); \
                                      tm_abort_tx_counts[cpu_getId()]++; \
                                      tm_cpu_tx_time[cpu_getId()] += __now - tm_tx_start_time; \
                                      tm_tx_start_time = __now; \
                                      tm_read_set_size = 0; \
                                      tm_write_set_size = 0; \
                                      DEBUG_PRINTF("> aborted in %s:%d %s()\n", __FILE__, __LINE__, __func__); \
                                      SWITCH_THREAD(); \
                                    } \
                                  } \
                                } while (0)


#  define TM_BEGIN()                  TM_START(0)
#  define TM_BEGIN_RO()               TM_START(1)
#if 1
#  define TM_END()                                                \
do {                                                              \
    stm_commit_tx(__tx);                                          \
    uint64_t __now; \
    TM_GET_TIME(__now); \
    tm_cpu_tx_time[cpu_getId()] +=                                \
        __now - tm_tx_start_time;                                  \
    tm_commit_tx_counts[cpu_getId()]++; \
    if (tm_read_set_size > 0) { \
        tm_cpu_readset_size[cpu_getId()] += tm_read_set_size; \
        tm_read_tx_counts[cpu_getId()]++; \
    } \
    if (tm_write_set_size > 0) { \
        tm_cpu_writeset_size[cpu_getId()] += tm_write_set_size; \
        tm_write_tx_counts[cpu_getId()]++; \
    } \
} while (0)
#else
#  define TM_END()                                                \
do {                                                              \
    stm_commit_tx(__tx);                                          \
    uint64_t __now; \
    TM_GET_TIME(__now); \
    tm_cpu_tx_time[cpu_getId()] +=                                \
        __now - tm_tx_start_time;                                  \
    if (tm_read_set_size > 0) { \
        tm_cpu_readset_size[cpu_getId()] += tm_read_set_size; \
        tm_read_tx_counts[cpu_getId()]++; \
    } \
    if (tm_write_set_size > 0) { \
        tm_cpu_writeset_size[cpu_getId()] += tm_write_set_size; \
        tm_write_tx_counts[cpu_getId()]++; \
    } \
     printf("ReadSet=%lu WriteSet=%lu\n",                          \
           (unsigned long)tm_read_set_size,                       \
           (unsigned long)tm_write_set_size);                     \
} while (0)
#endif
#  define TM_RESTART()                stm_abort(0)

#  define TM_EARLY_RELEASE(var)       /* nothing */

#  include <wrappers.h>

#  define TM_SHARED_READ(var)                                     \
({                                                                \
    tm_read_set_size++;                                           \
    stm_load_tx(__tx, (volatile stm_word_t *)(void *)&(var));      \
})

/* Pointer accesses pass the transaction descriptor explicitly (stock TinySTM
 * API), like every other access here: stm_load_ptr()/stm_store_ptr() would look
 * the descriptor up in thread-local storage on every access, which under
 * libfibre is a cfibre_getspecific() call. Same as the code measured in the paper. */
#  define TM_SHARED_READ_P(var)                                   \
({                                                                \
    tm_read_set_size++;                                           \
    (void *)stm_load_tx(__tx, (volatile stm_word_t *)(void *)&(var)); \
})
#  define TM_SHARED_READ_F(var)                                   \
({                                                                \
    tm_read_set_size++;                                           \
    stm_load_float((volatile float *)(void *)&(var));             \
})
#  define TM_SHARED_READ_D(var)                                   \
({                                                                \
    tm_read_set_size++;                                           \
    stm_load_double((volatile double *)(void *)&(var));           \
})

#  define TM_SHARED_WRITE(var, val)                               \
do {                                                              \
    tm_write_set_size++;                                          \
    stm_store_tx(__tx,                                            \
        (volatile stm_word_t *)(void *)&(var),                    \
        (stm_word_t)val);                                         \
} while (0)

#  define TM_SHARED_WRITE_P(var, val)                             \
do {                                                              \
    tm_write_set_size++;                                          \
    stm_store_tx(__tx, (volatile stm_word_t *)(void *)&(var),     \
                 (stm_word_t)(val));                              \
} while (0)

#  define TM_SHARED_WRITE_F(var, val)                             \
do {                                                              \
    tm_write_set_size++;                                          \
    stm_store_float((volatile float *)(void *)&(var), val);       \
} while (0)
#  define TM_SHARED_WRITE_D(var, val)                             \
do {                                                              \
    tm_write_set_size++;                                          \
    stm_store_double((volatile double *)(void *)&(var), val);     \
} while (0)

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

#else
#include <pthread.h>
#include "thread.h"

#  include <stdio.h>

#  define MAIN(argc, argv)              int main (int argc, char** argv)
#  define MAIN_RETURN(val)              return val

#  define GOTO_SIM()                    /* nothing */
#  define GOTO_REAL()                   /* nothing */
#  define IS_IN_SIM()                   (0)

#  define SIM_GET_NUM_CPU(var)          /* nothing */

#  define TM_PRINTF                     printf
#  define TM_PRINT0                     printf
#  define TM_PRINT1                     printf
#  define TM_PRINT2                     printf
#  define TM_PRINT3                     printf

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

#  define TM_ARG                        __tx,
#  define TM_ARG_ALONE                  __tx
#  define TM_ARGDECL                    struct stm_tx* __tx,
#  define TM_ARGDECL_ALONE              struct stm_tx* __tx
#  define TM_CALLABLE                   /* nothing */

#  include <mod_mem.h>
#  include <mod_stats.h>

#  if defined(LIBFIBRE)
#    define ULT_INIT(numCore)     cfibre_init_n(1, numCore)
#  else
#    define ULT_INIT(numCore)     /* nothing */
#  endif

#  define TM_STARTUP(numCore)     if (sizeof(long) != sizeof(void *)) { \
                                    fprintf(stderr, "Error: unsupported long and pointer sizes\n"); \
                                    exit(1); \
                                  } \
                                  ULT_INIT(numCore); \
                                  stm_init(); \
                                  mod_mem_init(0); \
                                  if (getenv("STM_STATS") != NULL) { \
                                    mod_stats_init(); \
                                  }
#  define TM_SHUTDOWN()           if (getenv("STM_STATS") != NULL) { \
                                    unsigned long u; \
                                    if (stm_get_global_stats("global_nb_commits", &u) != 0) \
                                      printf("#commits    : %lu\n", u); \
                                    if (stm_get_global_stats("global_nb_aborts", &u) != 0) \
                                      printf("#aborts     : %lu\n", u); \
                                    if (stm_get_global_stats("global_max_retries", &u) != 0) \
                                      printf("Max retries : %lu\n", u); \
                                  } \
                                  stm_exit()

# define TM_THREAD_ENTER()        struct stm_tx *__tx = stm_init_thread()
# define TM_THREAD_EXIT()         stm_exit_thread_tx(__tx)

#if defined(USE_TLMALLOC)
#  include "thread.h"
#  include "memory.h"
#  define P_MALLOC(size)          memory_get(thread_getId(), size)
#  define P_FREE(ptr)             /* TODO: thread local free is non-trivial */
#  define TM_MALLOC(size)         memory_get(thread_getId(), size)
#  define TM_FREE(ptr)            /* TODO: thread local free is non-trivial */
// #  define TM_MALLOC(size)         stm_malloc(size)
// #  define TM_FREE(ptr)            stm_free(ptr, sizeof(stm_word_t))
#else
#  define P_MALLOC(size)          malloc(size)
#  define P_FREE(ptr)             free(ptr)
#  define TM_MALLOC(size)         stm_malloc(size)
#  define TM_FREE(ptr)            stm_free(ptr, sizeof(stm_word_t))
// #  define TM_MALLOC(size)         malloc(size)
// #  define TM_FREE(ptr)            free(ptr)
#endif

#if defined(THREAD_SWITCHING)
#  if defined(LIBFIBRE)
#    define SWITCH_THREAD()     cfibre_yield()
#  elif defined(MASSIVE)
#    define SWITCH_THREAD()     myth_yield()
#  else
#    define SWITCH_THREAD()     pthread_yield()
#  endif
#else
#  define SWITCH_THREAD()       /* nothing */
#endif

#if 0
#  define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#  define DEBUG_PRINTF(...) /* nothing */
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
#  define DEBUG_NS(...) { \
                          printf(ESCAPE_YELLOW); \
                          printf(__VA_ARGS__); \
                          printf(ESCAPE_NORMAL); \
                        }
#else
#  define DEBUG(...)    /* nothing */
#  define DEBUG_NS(...) /* nothing */
#endif

#  define TM_START(ro)          do { \
                                  stm_tx_attr_t _a; \
                                  _a.attrs = (int32_t)(ro << 16); \
                                  sigjmp_buf *_e = stm_start_tx(__tx, _a); \
                                  if (_e != NULL) { \
                                    int reason = sigsetjmp(*_e, 0); \
                                    if (reason > 0) { \
                                      DEBUG_PRINTF("> aborted in %s at %s:%d \n", __func__, __FILE__, __LINE__); \
                                      SWITCH_THREAD(); \
                                    } \
                                  } \
                                } while (0)


#  define TM_BEGIN()                  TM_START(0)
#  define TM_BEGIN_RO()               TM_START(1)
#  define TM_END()                    stm_commit_tx(__tx)
#  define TM_RESTART()                stm_abort(0)

#  define TM_EARLY_RELEASE(var)       /* nothing */

#  include <wrappers.h>

#  define TM_SHARED_READ(var)           stm_load_tx(__tx, (volatile stm_word_t *)(void *)&(var))
/* explicit descriptor, as above (no thread-local lookup per access) */
#  define TM_SHARED_READ_P(var)         (void *)stm_load_tx(__tx, (volatile stm_word_t *)(void *)&(var))
#  define TM_SHARED_READ_F(var)         stm_load_float((volatile float *)(void *)&(var))
#  define TM_SHARED_READ_D(var)         stm_load_double((volatile double *)(void *)&(var))

#  define TM_SHARED_WRITE(var, val)     stm_store_tx(__tx, (volatile stm_word_t *)(void *)&(var), (stm_word_t)val)
#  define TM_SHARED_WRITE_P(var, val)   stm_store_tx(__tx, (volatile stm_word_t *)(void *)&(var), (stm_word_t)(val))
#  define TM_SHARED_WRITE_F(var, val)   stm_store_float((volatile float *)(void *)&(var), val)
#  define TM_SHARED_WRITE_D(var, val)   stm_store_double((volatile double *)(void *)&(var), val)

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})
#endif

#endif /* TM_TINYSTM_H */

