#ifndef TM_H
#define TM_H 1

#include <pthread.h>
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

#  include <string.h>
#  include <stm.h>
#  include "thread.h"

#    define TM_ARG                        STM_SELF,
#    define TM_ARG_ALONE                  STM_SELF
#    define TM_ARGDECL                    STM_THREAD_T* TM_ARG
#    define TM_ARGDECL_ALONE              STM_THREAD_T* TM_ARG_ALONE
#    define TM_CALLABLE                   /* nothing */

#      include "norec.h"
#  if defined(LIBFIBRE)
#    define ULT_INIT(numCore)     cfibre_init_n(1, numCore)
#  else
#    define ULT_INIT(numCore)     /* nothing */
#  endif

#      define TM_STARTUP(numCore)     ULT_INIT(numCore);\
					STM_STARTUP()
#      define TM_SHUTDOWN()             STM_SHUTDOWN()

#      define TM_THREAD_ENTER()         TM_ARGDECL_ALONE = STM_NEW_THREAD(); \
                                        STM_INIT_THREAD(TM_ARG_ALONE, thread_getId())
#      define TM_THREAD_EXIT()          STM_FREE_THREAD(TM_ARG_ALONE)

#if defined(USE_TLMALLOC)
#include "thread.h"
#include "memory.h"
#    define P_MALLOC(size)              memory_get(thread_getId(), size)
#    define P_FREE(ptr)                 /* TODO: thread local free is non-trivial */
#    define TM_MALLOC(size)             memory_get(thread_getId(), size)
#    define TM_FREE(ptr)                /* TODO: thread local free is non-trivial */
#else
#      define P_MALLOC(size)            malloc(size)
#      define P_FREE(ptr)               /* free(ptr) */
#      define TM_MALLOC(size)           malloc(size)
#      define TM_FREE(ptr)              /* STM_FREE(ptr) */
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
#    define TM_BEGIN()                  STM_BEGIN_WR()
#    define TM_BEGIN_RO()               STM_BEGIN_RD()
#else
#define STM_BEGIN_SWITCH(isReadOnly)       do { \
                                                STM_JMPBUF_T STM_JMPBUF; \
                                                int reason = sigsetjmp(STM_JMPBUF, 1); \
                                                if (reason > 0) { \
                                                    DEBUG_PRINTF("> aborted in %s:%d %s()\n", __FILE__, __LINE__, __func__); \
                                                    SWITCH_THREAD();  \
                                                } \
                                                TxStart(STM_SELF, &STM_JMPBUF); \
                                            } while (0) /* enforce comma */
#    define TM_BEGIN()                  STM_BEGIN_SWITCH(0)
#    define TM_BEGIN_RO()               STM_BEGIN_SWITCH(1)
#endif
#    define TM_END()                    STM_END()
#    define TM_RESTART()                STM_RESTART()

#  define TM_EARLY_RELEASE(var)         /* nothing */

#  define TM_SHARED_READ(var)           STM_READ(var)
#  define TM_SHARED_READ_P(var)         STM_READ_P(var)
#  define TM_SHARED_READ_F(var)         STM_READ_F(var)
#  define TM_SHARED_READ_D(var)         STM_READ_D(var)

#  define TM_SHARED_WRITE(var, val)     STM_WRITE((var), val)
#  define TM_SHARED_WRITE_P(var, val)   STM_WRITE_P((var), val)
#  define TM_SHARED_WRITE_F(var, val)   STM_WRITE_F((var), val)
#  define TM_SHARED_WRITE_D(var, val)   STM_WRITE_D((var), val)

#  define TM_LOCAL_WRITE(var, val)      STM_LOCAL_WRITE(var, val)
#  define TM_LOCAL_WRITE_P(var, val)    STM_LOCAL_WRITE_P(var, val)
#  define TM_LOCAL_WRITE_F(var, val)    STM_LOCAL_WRITE_D(var, val)

#endif /* TM_H */
