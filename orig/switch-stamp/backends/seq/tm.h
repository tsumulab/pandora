#ifndef TM_H
#define TM_H 1

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

#  define TM_ARG                        /* nothing */
#  define TM_ARG_ALONE                  /* nothing */
#  define TM_ARGDECL                    /* nothing */
#  define TM_ARGDECL_ALONE              /* nothing */
#  define TM_CALLABLE                   /* nothing */

#  if defined(LIBFIBRE)
#    define ULT_INIT(numCore)     cfibre_init_n(1, numCore)
#  else
#    define ULT_INIT(numCore)     /* nothing */
#  endif

#  define TM_STARTUP(numThread)         ULT_INIT(numThread)
#  define TM_SHUTDOWN()                 /* nothing */

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

#  define TM_BEGIN()                    /* nothing */
#  define TM_BEGIN_RO()                 /* nothing */
#  define TM_END()                      /* nothing */
#  define TM_RESTART()                  assert(0)

#  define TM_EARLY_RELEASE(var)         /* nothing */

#  define TM_SHARED_READ(var)        	(var)
#  define TM_SHARED_READ_P(var)      	(var)
#  define TM_SHARED_READ_F(var)         (var)
#  define TM_SHARED_READ_D(var)         (var)

#  define TM_SHARED_WRITE(var, val)     ({var = val; var;})
#  define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})
#  define TM_SHARED_WRITE_D(var, val)   ({var = val; var;})

#  define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#  define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})
#  define TM_LOCAL_WRITE_D(var, val)    ({var = val; var;})

#endif /* TM_H */