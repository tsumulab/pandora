/* 
 * myth_wrap_pthread.c : pthread-like interface
 */

#include <assert.h>
#define should_not_reach_here() assert(0)
#define _OPEN_THREADS
#include <libfibre/cfibre.h>
#include <pthread.h>
#include <sched.h>
#include "libfibre_real_pthread.h"
#include "libfibre_wrap_util_func.h"

/* --------------------------------------------------
   --- basic thread functions (pthread_create, etc.)
   -------------------------------------------------- */
int __wrap(pthread_yield)(void) {
  int ret = cfibre_yield();
  return ret;
}

/* translate pthread attributes to libfibre attributes */
static cfibre_attr_t *
pthread_attr_to_libfibre(const pthread_attr_t * p, cfibre_attr_t * m) {
    if(!p){
        return 0;
    }else{
        int _ = cfibre_attr_init(m);
//casause segmentation fault. so comment out
        //void *tmp;
        //int detached = 0;
        //size_t stack_size;
        //int r = pthread_attr_getdetachstate(p, &detached);
        //cfibre_attr_setdetachstate(m, detached);
        //(void)_;
        //assert(r == 0);
//tmp is// not used!!
        //r = pthread_attr_getstack(p, &tmp, &stack_size);
        //cfibre_attr_setstacksize(m, stack_size);
        //assert(r == 0);
        return m;
    }    
}

int __wrap(pthread_create)(pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg) {
	int ret = 1;
	cfibre_attr_t mattr_[1];
	cfibre_attr_t *mattr = pthread_attr_to_libfibre(attr, mattr_);
	//maybe forcibly set pthread_t into cfibre_t
    assert(sizeof(cfibre_t) <= sizeof(pthread_t));
	ret = cfibre_create((cfibre_t *)thread, mattr, start_routine, arg);
#ifdef DEBUG
	printf("in wrap_create\n");
#endif
	return ret;
}

///* pthread_exit (3)     - terminate calling thread */
void __wrap(pthread_exit)(void *retval) {
  /* since pthread_exit does not return ... */
  cfibre_exit();
}

/* pthread_join (3)     - join with a terminated thread */
int __wrap(pthread_join)(pthread_t thread, void **retval) {
  int ret = 1;
  ret = cfibre_join((cfibre_t)thread, retval);
#ifdef DEBUG
	printf("in wrap_join\n");
#endif
  return ret;
}

/* pthread_tryjoin_np (3) - try to join with a terminated thread */
//int __wrap(pthread_tryjoin_np)(pthread_t thread, void **retval) {
//  int ret;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_tryjoin_body((myth_thread_t)thread, retval);
//  } else {
//    ret = real_pthread_tryjoin_np(thread, retval);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}

///* pthread_timedjoin_np (3) - try to join with a terminated thread */
//int __wrap(pthread_timedjoin_np)(pthread_t thread, void **retval,
//			 const struct timespec *abstime) {
//  int _ = enter_wrapped_func("%x, %p, %p", thread, retval, abstime);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_timedjoin_body((myth_thread_t)thread, retval, abstime);
//  } else {
//    ret = real_pthread_timedjoin_np(thread, retval, abstime);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_JOIN_NP */
//
/* pthread_detach (3)   - detach a thread */
int __wrap(pthread_detach)(pthread_t thread) {
  int ret;
  ret = cfibre_detach((cfibre_t)thread);
  return ret;
}

/* ---------------------------
   --- pthread_self
   --------------------------- */

/* pthread_self (3)     - obtain ID of the calling thread */
/* pthread_self (3posix) - get the calling thread ID */
pthread_t __wrap(pthread_self)(void) {
  pthread_t ret;
  ret = (pthread_t)cfibre_self();
  return ret;
}

/* pthread_equal (3)    - compare thread IDs */
int __wrap(pthread_equal)(pthread_t t1, pthread_t t2) {
  int ret;
  ret = cfibre_equal((cfibre_t)t1, (cfibre_t)t2);
  return ret;
}

/* -----------------------------
   --- thread attributes
   ----------------------------- */

/* pthread_attr_init (3) - initialize and destroy thread attributes object */
int __wrap(pthread_attr_init)(pthread_attr_t *attr) {
  int ret = cfibre_attr_init((cfibre_attr_t *)attr);
  return ret;
}

/* pthread_attr_destroy (3) - initialize and destroy thread attributes object */
int __wrap(pthread_attr_destroy)(pthread_attr_t *attr) {
  int ret = cfibre_attr_destroy((cfibre_attr_t *)attr);
  return ret;
}

/* pthread_attr_getdetachstate (3) - set/get detach state attribute in thread attributes object */
int __wrap(pthread_attr_getdetachstate)
     (const pthread_attr_t *attr, int *detachstate) {
  int ret = __real(pthread_attr_getdetachstate)(attr, detachstate);
  return ret;
}

/* pthread_attr_setdetachstate (3) - set/get detach state attribute in thread attributes object */
int __wrap(pthread_attr_setdetachstate)
     (pthread_attr_t *attr, int detachstate) {
  int ret = __real(pthread_attr_setdetachstate)(attr, detachstate);
  return ret;
}

/* pthread_attr_getguardsize (3) - set/get guard size attribute in thread attributes object */
/* pthread_attr_getguardsize (3posix) - get and set the thread guardsize attribute */
int __wrap(pthread_attr_getguardsize)
     (const pthread_attr_t *attr, size_t *guardsize) {
  int ret = __real(pthread_attr_getguardsize)(attr, guardsize);
  return ret;
}

/* pthread_attr_setguardsize (3) - set/get guard size attribute in thread attributes object */
int __wrap(pthread_attr_setguardsize)(pthread_attr_t *attr, size_t guardsize) {
  int ret = __real(pthread_attr_setguardsize)(attr, guardsize);
  return ret;
}

///* pthread_attr_getschedparam (3) - set/get scheduling parameter attributes in thread attrib... */
//int __wrap(pthread_attr_getschedparam)(const pthread_attr_t *attr,
//				       struct sched_param *param) {
//  int _ = enter_wrapped_func("%p, %p", attr, param);
//  int ret = real_pthread_attr_getschedparam(attr, param);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setschedparam (3) - set/get scheduling parameter attributes in thread attrib... */
//int __wrap(pthread_attr_setschedparam)(pthread_attr_t *attr,
//				       const struct sched_param *param) {
//  int _ = enter_wrapped_func("%p, %p", attr, param);
//  int ret = real_pthread_attr_setschedparam(attr, param);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_getschedpolicy (3) - set/get scheduling policy attribute in thread attribute... */
//int __wrap(pthread_attr_getschedpolicy)
//     (const pthread_attr_t *attr, int *policy) {
//  int _ = enter_wrapped_func("%p, %p", attr, policy);
//  int ret = real_pthread_attr_getschedpolicy(attr, policy);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setschedpolicy (3) - set/get scheduling policy attribute in thread attribute... */
//int __wrap(pthread_attr_setschedpolicy)
//     (pthread_attr_t *attr, int policy) {
//  int _ = enter_wrapped_func("%p, %d", attr, policy);
//  int ret = real_pthread_attr_setschedpolicy(attr, policy);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_getinheritsched (3) - set/get inherit-scheduler attribute in thread attribut... */
//int __wrap(pthread_attr_getinheritsched)
//     (const pthread_attr_t *attr, int *inheritsched) {
//  int _ = enter_wrapped_func("%p, %p", attr, inheritsched);
//  int ret = real_pthread_attr_getinheritsched(attr, inheritsched);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setinheritsched (3) - set/get inherit-scheduler attribute in thread attribut... */
//int __wrap(pthread_attr_setinheritsched)
//     (pthread_attr_t *attr, int inheritsched) {
//  int _ = enter_wrapped_func("%p, %d", attr, inheritsched);
//  int ret = real_pthread_attr_setinheritsched(attr, inheritsched);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_getscope (3) - set/get contention scope attribute in thread attributes object */
//int __wrap(pthread_attr_getscope)
//     (const pthread_attr_t *attr, int *scope) {
//  int _ = enter_wrapped_func("%p, %p", attr, scope);
//  int ret = real_pthread_attr_getscope(attr, scope);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setscope (3) - set/get contention scope attribute in thread attributes object */
//int __wrap(pthread_attr_setscope)(pthread_attr_t *attr, int scope) {
//  int _ = enter_wrapped_func("%p, %d", attr, scope);
//  int ret = real_pthread_attr_setscope(attr, scope);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if 0				/* deprecated */
///* pthread_attr_getstackaddr (3) - set/get stack address attribute in thread attributes object */
//int __wrap(pthread_attr_getstackaddr)
//     (const pthread_attr_t *attr, void **stackaddr) {
//  int _ = enter_wrapped_func("%p, %p", attr, stackaddr);
//  int ret = real_pthread_attr_getstackaddr(attr, stackaddr);
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setstackaddr (3) - set/get stack address attribute in thread attributes object */
//int __wrap(pthread_attr_setstackaddr)
//     (pthread_attr_t *attr, void *stackaddr) {
//  int _ = enter_wrapped_func("%p, %p", attr, stackaddr);
//  int ret = real_pthread_attr_setstackaddr(attr, stackaddr);
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif
//
///* pthread_attr_getstacksize (3) - set/get stack size attribute in thread attributes object */
//int __wrap(pthread_attr_getstacksize)
//     (const pthread_attr_t *attr, size_t *stacksize) {
//  int _ = enter_wrapped_func("%p, %p", attr, stacksize);
//  int ret = real_pthread_attr_getstacksize(attr, stacksize);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setstacksize (3) - set/get stack size attribute in thread attributes object */
//int __wrap(pthread_attr_setstacksize)
//     (pthread_attr_t *attr, size_t stacksize) {
//  int _ = enter_wrapped_func("%p, %lu", attr, stacksize);
//  int ret = real_pthread_attr_setstacksize(attr, stacksize);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_getstack (3) - set/get stack attributes in thread attributes object */
//int __wrap(pthread_attr_getstack)(const pthread_attr_t *attr,
//				  void **stackaddr, size_t *stacksize) {
//  int _ = enter_wrapped_func("%p, %p, %p", attr, stackaddr, stacksize);
//  int ret = real_pthread_attr_getstack(attr, stackaddr, stacksize);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_attr_setstack (3) - set/get stack attributes in thread attributes object */
//int __wrap(pthread_attr_setstack)(pthread_attr_t *attr,
//				  void *stackaddr, size_t stacksize) {
//  int _ = enter_wrapped_func("%p, %p, %lu", attr, stackaddr, stacksize);
//  int ret = real_pthread_attr_setstack(attr, stackaddr, stacksize);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if defined(HAVE_PTHREAD_ATTR_SETAFFINITY_NP)
///* pthread_attr_setaffinity_np (3) - set/get CPU affinity attribute in thread attributes object */
//int __wrap(pthread_attr_setaffinity_np)(pthread_attr_t *attr,
//					size_t cpusetsize,
//					const cpu_set_t *cpuset) {
//  int _ = enter_wrapped_func("%p, %lu, %p", attr, cpusetsize, cpuset);
//  int ret = real_pthread_attr_setaffinity_np(attr, cpusetsize, cpuset);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_ATTR_SETAFFINITY_NP */
//
//#if defined(HAVE_PTHREAD_ATTR_GETAFFINITY_NP)
///* pthread_attr_getaffinity_np (3) - set/get CPU affinity attribute in thread attributes object */
//int __wrap(pthread_attr_getaffinity_np)(const pthread_attr_t *attr,
//					size_t cpusetsize, cpu_set_t *cpuset) {
//  int _ = enter_wrapped_func("%p, %lu, %p", attr, cpusetsize, cpuset);
//  int ret = real_pthread_attr_getaffinity_np(attr, cpusetsize, cpuset);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_ATTR_GETAFFINITY_NP */
//
//#if defined(HAVE_PTHREAD_ATTR_NP)
///* Get the default attributes used by pthread_create in this process.  */
//int __wrap(pthread_getattr_default_np)(pthread_attr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_getattr_default_np(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* Set the default attributes to be used by pthread_create in this
//   process.  */
//int __wrap(pthread_setattr_default_np)(const pthread_attr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_setattr_default_np(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_getattr_np (3) - get attributes of created thread */
//int __wrap(pthread_getattr_np)(pthread_t thread, pthread_attr_t *attr) {
//  int _ = enter_wrapped_func("%x, %p", thread, attr);
//  int ret = real_pthread_getattr_np(thread, attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_ATTR_NP */
//
//
///* pthread_setschedparam (3) - set/get scheduling policy and parameters of a thread */
///* pthread_setschedparam (3posix) - dynamic thread scheduling parameters access (REALTIME TH... */
//int __wrap(pthread_setschedparam)(pthread_t thread, int policy,
//				  const struct sched_param *param) {
//  int _ = enter_wrapped_func("%x, %d, %p", thread, policy, param);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setschedparam(thread, policy, param); 
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_getschedparam (3) - set/get scheduling policy and parameters of a thread */
//int __wrap(pthread_getschedparam)(pthread_t thread, int *policy,
//				  struct sched_param *param) {
//  int _ = enter_wrapped_func("%x, %p, %p", thread, policy, param);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_getschedparam(thread, policy, param);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if defined(HAVE_PTHREAD_SCHEDPRIO)
///* pthread_setschedprio (3) - set scheduling priority of a thread */
///* pthread_setschedprio (3posix) - dynamic thread scheduling parameters access (REALTIME THR... */
//int __wrap(pthread_setschedprio)(pthread_t thread, int prio) {
//  int _ = enter_wrapped_func("%x, %d", thread, prio);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setschedprio(thread, prio);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif
//
//#if defined(HAVE_PTHREAD_NAME_NP)
///* pthread_getname_np (3) - set/get the name of a thread */
//int __wrap(pthread_getname_np)(pthread_t thread, char *name, size_t len) {
//  int _ = enter_wrapped_func("%x, %s, %lu", thread, name, len);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_getname_np(thread, name, len);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_setname_np (3) - set/get the name of a thread */
//int __wrap(pthread_setname_np)(
//#if PTHREAD_SETNAME_ARITY == 2
//			       pthread_t thread,
//#endif
//			       const char *name) {
//#if PTHREAD_SETNAME_ARITY == 2
//  int _ = enter_wrapped_func("%x, %s", thread, name);
//#else
//  int _ = enter_wrapped_func("%s", name);
//#endif
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setname_np(
//#if PTHREAD_SETNAME_ARITY == 2
//				  thread,
//#endif
//				  name);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_NAME_NP */
//
//#if defined(HAVE_PTHREAD_CONCURRENCY)
///* pthread_getconcurrency (3) - set/get the concurrency level */
//int __wrap(pthread_getconcurrency)(void) {
//  int _ = enter_wrapped_func(0);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_getconcurrency_body();
//  } else {
//    ret = real_pthread_getconcurrency();
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_setconcurrency (3) - set/get the concurrency level */
///* pthread_setconcurrency (3posix) - set the level of concurrency */
//int __wrap(pthread_setconcurrency)(int new_level) {
//  int _ = enter_wrapped_func("%d", new_level);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setconcurrency(new_level);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_CONCURRENCY */
//
//#if defined(HAVE_PTHREAD_YIELD)
///* pthread_yield (3)    - yield the processor */
//int __wrap(pthread_yield)(void) {
//  int _ = enter_wrapped_func(0);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_yield_body();
//  } else {
//    ret = real_pthread_yield();
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_YIELD */
//
//#if defined(HAVE_PTHREAD_SETAFFINITY_NP)
///* pthread_setaffinity_np (3) - set/get CPU affinity of a thread */
//int __wrap(pthread_setaffinity_np)
//     (pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset) {
//  int _ = enter_wrapped_func("%x, %lu, %p", thread, cpusetsize, cpuset);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setaffinity_np(thread, cpusetsize, cpuset);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_SETAFFINITY_NP */
//
//#if defined(HAVE_PTHREAD_GETAFFINITY_NP)
///* pthread_getaffinity_np (3) - set/get CPU affinity of a thread */
//int __wrap(pthread_getaffinity_np)(pthread_t thread, size_t cpusetsize,
//				   cpu_set_t *cpuset) {
//  int _ = enter_wrapped_func("%x, %lu, %p", thread, cpusetsize, cpuset);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_getaffinity_np(thread, cpusetsize, cpuset);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_GETAFFINITY_NP */
//
//
///* ------------------------------
//   --- cancel 
//   ------------------------------ */
//
///* pthread_setcancelstate (3) - set cancelability state and type */
///* pthread_setcancelstate (3posix) - set cancelability state */
//int __wrap(pthread_setcancelstate)(int state, int *oldstate) {
//  int _ = enter_wrapped_func("%d, %p", state, oldstate);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setcancelstate(state, oldstate);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_setcanceltype (3) - set cancelability state and type */
//int __wrap(pthread_setcanceltype)(int type, int *oldtype) {
//  int _ = enter_wrapped_func("%d, %p", type, oldtype);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_setcanceltype(type, oldtype);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_cancel (3)   - send a cancellation request to a thread */
//int __wrap(pthread_cancel)(pthread_t thread) {
//  int _ = enter_wrapped_func("%x", thread);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_cancel(thread);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_testcancel (3) - request delivery of any pending cancellation request */
///* pthread_testcancel (3posix) - set cancelability state */
//void __wrap(pthread_testcancel)(void) {
//  enter_wrapped_func(0);
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//  } else {
//    real_pthread_testcancel();
//  }
//  leave_wrapped_func(0);
//}
//
///* ----------------------------------
//   --- once 
//   ---------------------------------- */
//
///* pthread_once (3posix) - dynamic package initialization */
//int __wrap(pthread_once)(pthread_once_t *once_control,
//			 void (*init_routine)(void)) {
//  int _ = enter_wrapped_func("%p, %p", once_control, init_routine);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_once_body((myth_once_t *)once_control, init_routine);
//  } else {
//    ret = real_pthread_once(once_control, init_routine);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//// pthread_once_t once_control = PTHREAD_ONCE_INIT;
//
/* -------------------------
   --- mutex 
   ------------------------- */

static inline int pthread_mutex_type_to_libfibre(int t) {
  switch (t) {
  case PTHREAD_MUTEX_NORMAL:
    return CFIBRE_MUTEX_DEFAULT;
  case PTHREAD_MUTEX_ERRORCHECK:
    return CFIBRE_MUTEX_ERRORCHECK;
  case PTHREAD_MUTEX_RECURSIVE:
    return CFIBRE_MUTEX_RECURSIVE;
  default:
    return CFIBRE_MUTEX_DEFAULT;
  }
}

static cfibre_mutexattr_t *
pthread_mutexattr_to_libfibre(const pthread_mutexattr_t * p,
			  cfibre_mutexattr_t * m) {
	if (!p) {
		return 0;
	} else {
		int _ = cfibre_mutexattr_init(m);
		int t;
		int r = pthread_mutexattr_gettype(p, &t);
		cfibre_mutexattr_settype(m, pthread_mutex_type_to_libfibre(t));
		return m;
	}
}

/* pthread_mutex_init (3posix) - destroy and initialize a mutex */
int __wrap(pthread_mutex_init)(pthread_mutex_t *restrict mutex,
			       const pthread_mutexattr_t *restrict attr) {
	int ret;
	cfibre_mutexattr_t mattr_[1];
	cfibre_mutexattr_t * mattr = pthread_mutexattr_to_libfibre(attr, mattr_);
	ret = cfibre_mutex_init((cfibre_mutex_t *)mutex, mattr);
	return ret;
}
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//

/* pthread_mutex_destroy (3posix) - destroy and initialize a mutex */
int __wrap(pthread_mutex_destroy)(pthread_mutex_t *mutex) {
	int ret;
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_mutex_destroy((cfibre_mutex_t *)mutex);
	return ret;
}

/* do our best to handle mutex initialized by
   pthread_t m = PTHREAD_MUTEX_INITIALIZER;
   if such a mutex is passed, we reinitialize it,
   as if myth_muex_init is called.
 */

//forbid MUTEX_INITIALIZER
//static int myth_handle_PTHREAD_MUTEX_INITIALIZER(pthread_mutex_t * pm) {
//  cfibre_mutex_t * m = (cfibre_mutex_t *)pm;
//  volatile int * magic_p = (volatile int *)&m->magic;
//  int magic = * magic_p;
//  if (magic != cfibre_mutex_magic_no) {
//    if (magic != cfibre_mutex_magic_no_initializing
//	&& __sync_bool_compare_and_swap(magic_p, magic, cfibre_mutex_magic_no_initializing)) {
//      cfibre_mutex_t mi = MYTH_MUTEX_INITIALIZER;
//      mi.magic = cfibre_mutex_magic_no_initializing;
//      *m = mi;
//      cfibre_rwbarrier();
//      *magic_p = cfibre_mutex_magic_no;
//    } else {
//      while (*magic_p == cfibre_mutex_magic_no_initializing) { }
//      cfibre_assert(*magic_p == cfibre_mutex_magic_no);
//    }
//  }
//  return 0;
//}

/* pthread_mutex_trylock (3posix) - lock and unlock a mutex */
int __wrap(pthread_mutex_trylock)(pthread_mutex_t *mutex) {
	int ret;
	//if mutex isnt initialized, reinitialize.
	//but I dont care this.
	//libfibre_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_mutex_trylock((cfibre_mutex_t *)mutex);
	return ret;
}

/* pthread_mutex_lock (3posix) - lock and unlock a mutex */
int __wrap(pthread_mutex_lock)(pthread_mutex_t *mutex) {
	int ret;
	//cfibre_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_mutex_lock((cfibre_mutex_t *)mutex);
	return ret;
}

/* pthread_mutex_timedlock (3posix) - lock a mutex */
int __wrap(pthread_mutex_timedlock)(pthread_mutex_t *restrict mutex,
				    const struct timespec *restrict abstime) {
	int ret;
	//cfibre_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_mutex_timedlock((cfibre_mutex_t *)mutex, abstime);
	return ret;
}

/* pthread_mutex_unlock (3posix) - lock and unlock a mutex */
int __wrap(pthread_mutex_unlock)(pthread_mutex_t *mutex) {
	int ret;
	//cfibre_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_mutex_unlock((cfibre_mutex_t *)mutex);
	return ret;
}

///* pthread_mutex_getprioceiling (3posix) - get and set the priority ceiling of a mutex (REAL... */
//int __wrap(pthread_mutex_getprioceiling)(const pthread_mutex_t *restrict mutex,
//					 int *restrict prioceiling) {
//  int _ = enter_wrapped_func("%p, %p", mutex, prioceiling);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    //myth_handle_PTHREAD_MUTEX_INITIALIZER(mutex); contradicts const
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_mutex_getprioceiling(mutex, prioceiling);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutex_setprioceiling (3posix) - change the priority ceiling of a mutex (REALTIME ... */
//int __wrap(pthread_mutex_setprioceiling)
//     (pthread_mutex_t *restrict mutex, int prioceiling,
//      int *restrict old_ceiling) {
//  int _ = enter_wrapped_func("%p, %d, %p", mutex, prioceiling, old_ceiling);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_mutex_setprioceiling(mutex, prioceiling, old_ceiling);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if defined(HAVE_PTHREAD_MUTEX_CONSISTENT)
///* pthread_mutex_consistent (3posix) - mark state protected by robust mutex as consistent */
//int __wrap(pthread_mutex_consistent)(pthread_mutex_t *mutex) {
//  int _ = enter_wrapped_func("%p", mutex);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_handle_PTHREAD_MUTEX_INITIALIZER(mutex);
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_mutex_consistent(mutex);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_MUTEX_CONSISTENT */
//
///* pthread_mutexattr_init (3posix) - initialize the mutex attributes object */
//int __wrap(pthread_mutexattr_init)(pthread_mutexattr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_mutexattr_init(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_destroy (3posix) - destroy and initialize the mutex attributes object */
//int __wrap(pthread_mutexattr_destroy)(pthread_mutexattr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_mutexattr_destroy(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_getpshared (3posix) - get and set the process-shared attribute */
//int __wrap(pthread_mutexattr_getpshared)
//     (const pthread_mutexattr_t *restrict attr, int *restrict pshared) {
//  int _ = enter_wrapped_func("%p, %p", attr, pshared);
//  int ret = real_pthread_mutexattr_getpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_setpshared (3posix) - set the process-shared attribute */
//int __wrap(pthread_mutexattr_setpshared)(pthread_mutexattr_t *attr,
//					 int pshared) {
//  int _ = enter_wrapped_func("%p, %d", attr, pshared);
//  int ret = real_pthread_mutexattr_setpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_gettype (3posix) - get and set the mutex type attribute */
//int __wrap(pthread_mutexattr_gettype)
//     (const pthread_mutexattr_t *restrict attr, int *restrict type) {
//  int _ = enter_wrapped_func("%p, %p", attr, type);
//  int ret = real_pthread_mutexattr_gettype(attr, type);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_settype (3posix) - set the mutex type attribute */
//int __wrap(pthread_mutexattr_settype)(pthread_mutexattr_t *attr, int type) {
//  int _ = enter_wrapped_func("%p, %d", attr, type);
//  int ret = real_pthread_mutexattr_settype(attr, type);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_getprotocol (3posix) - get and set the protocol attribute of the mutex ... */
//int __wrap(pthread_mutexattr_getprotocol)
//     (const pthread_mutexattr_t *restrict attr, int *restrict protocol) {
//  int _ = enter_wrapped_func("%p, %p", attr, protocol);
//  int ret = real_pthread_mutexattr_getprotocol(attr, protocol);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_setprotocol (3posix) - set the protocol attribute of the mutex attribut... */
//int __wrap(pthread_mutexattr_setprotocol)
//     (pthread_mutexattr_t *attr, int protocol) {
//  int _ = enter_wrapped_func("%p, %d", attr, protocol);
//  int ret = real_pthread_mutexattr_setprotocol(attr, protocol);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_getprioceiling (3posix) - get and set the prioceiling attribute of the ... */
//int __wrap(pthread_mutexattr_getprioceiling)
//     (const pthread_mutexattr_t *restrict attr, int *restrict prioceiling) {
//  int _ = enter_wrapped_func("%p, %d", attr, prioceiling);
//  int ret = real_pthread_mutexattr_getprioceiling(attr, prioceiling);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_setprioceiling (3posix) - set the prioceiling attribute of the mutex at... */
//int __wrap(pthread_mutexattr_setprioceiling)
//     (pthread_mutexattr_t *attr, int prioceiling) {
//  int _ = enter_wrapped_func("%p, %d", attr, prioceiling);
//  int ret = real_pthread_mutexattr_setprioceiling(attr, prioceiling);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_mutexattr_getrobust (3posix) - get and set the mutex robust attribute */
//#if defined(HAVE_PTHREAD_MUTEXATTR_GETROBUST)
//int __wrap(pthread_mutexattr_getrobust)
//     (const pthread_mutexattr_t *restrict attr, int *restrict robust) {
//  int _ = enter_wrapped_func("%p, %p", attr, robust);
//  int ret = real_pthread_mutexattr_getrobust(attr, robust);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_MUTEXATTR_GETROBUST */
//
//#if defined(HAVE_PTHREAD_MUTEXATTR_SETROBUST)
///* pthread_mutexattr_setrobust (3posix) - get and set the mutex robust attribute */
//int __wrap(pthread_mutexattr_setrobust)
//     (pthread_mutexattr_t *attr, int robust) {
//  int _ = enter_wrapped_func("%p, %d", attr, robust);
//  int ret = real_pthread_mutexattr_setrobust(attr, robust);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif	/* HAVE_PTHREAD_MUTEXATTR_SETROBUST */
//
///* ---------------------------
//   --- reader-writer lock 
//   --------------------------- */
//
//#if defined(HAVE_PTHREAD_RWLOCK)
//
//static inline int pthread_rwlock_kind_to_myth(int k) {
//  switch (k) {
//  case PTHREAD_RWLOCK_PREFER_READER_NP:
//    return MYTH_RWLOCK_PREFER_READER;
//  case PTHREAD_RWLOCK_PREFER_WRITER_NP:
//    return MYTH_RWLOCK_PREFER_WRITER;
//  case PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP:
//    return MYTH_RWLOCK_PREFER_WRITER_NONRECURSIVE;
//  default:
//    return MYTH_RWLOCK_PREFER_INVALID;
//  }
//}
//
//static inline myth_rwlockattr_t *
//pthread_rwlockattr_to_myth(const pthread_rwlockattr_t * p,
//			   myth_rwlockattr_t * m) {
//  if (!p) {
//    return 0;
//  } else {
//    int _ = myth_rwlockattr_init_body(m);
//    int k;
//    int r = pthread_rwlockattr_getkind_np(p, &k);
//    (void)_;
//    assert(r == 0);
//    m->kind = pthread_rwlock_kind_to_myth(k);
//    return m;
//  }
//}
//
//int __wrap(pthread_rwlock_init)(pthread_rwlock_t *restrict rwlock,
//				const pthread_rwlockattr_t *restrict attr) {
//  int _ = enter_wrapped_func("%p, %p", rwlock, attr);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_rwlockattr_t mattr_[1];
//    myth_rwlockattr_t * mattr = pthread_rwlockattr_to_myth(attr, mattr_);
//    assert(sizeof(myth_rwlock_t) <= sizeof(pthread_rwlock_t));
//    ret = myth_rwlock_init_body((myth_rwlock_t *)rwlock, mattr);
//  } else {
//    ret = real_pthread_rwlock_init(rwlock, attr);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//// pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
//
///* pthread_rwlock_destroy (3posix) - destroy and initialize a read-write lock object */
//int __wrap(pthread_rwlock_destroy)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_destroy_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_destroy(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_rdlock (3posix) - lock a read-write lock object for reading */
//int __wrap(pthread_rwlock_rdlock)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_rdlock_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_rdlock(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_tryrdlock (3posix) - lock a read-write lock object for reading */
//int __wrap(pthread_rwlock_tryrdlock)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_tryrdlock_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_tryrdlock(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_timedrdlock (3posix) - lock a read-write lock for reading */
//int __wrap(pthread_rwlock_timedrdlock)
//     (pthread_rwlock_t *restrict rwlock,
//      const struct timespec *restrict abstime) {
//  int _ = enter_wrapped_func("%p, %p", rwlock, abstime);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_timedrdlock_body((myth_rwlock_t *)rwlock, abstime);
//  } else {
//    ret = real_pthread_rwlock_timedrdlock(rwlock, abstime);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_wrlock (3posix) - lock a read-write lock object for writing */
//int __wrap(pthread_rwlock_wrlock)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_wrlock_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_wrlock(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_trywrlock (3posix) - lock a read-write lock object for writing */
//int __wrap(pthread_rwlock_trywrlock)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_trywrlock_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_trywrlock(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_timedwrlock (3posix) - lock a read-write lock for writing */
//int __wrap(pthread_rwlock_timedwrlock)
//     (pthread_rwlock_t *restrict rwlock,
//      const struct timespec *restrict abstime) {
//  int _ = enter_wrapped_func("%p, %p", rwlock, abstime);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_timedwrlock_body((myth_rwlock_t *)rwlock, abstime);
//  } else {
//    ret = real_pthread_rwlock_timedwrlock(rwlock, abstime);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlock_unlock (3posix) - unlock a read-write lock object */
//int __wrap(pthread_rwlock_unlock)(pthread_rwlock_t *rwlock) {
//  int _ = enter_wrapped_func("%p", rwlock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_rwlock_unlock_body((myth_rwlock_t *)rwlock);
//  } else {
//    ret = real_pthread_rwlock_unlock(rwlock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_init (3posix) - initialize the read-write lock attributes object */
//int __wrap(pthread_rwlockattr_init)(pthread_rwlockattr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_rwlockattr_init(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_destroy (3posix) - destroy and initialize the read-write lock attribut... */
//int __wrap(pthread_rwlockattr_destroy)(pthread_rwlockattr_t *attr) {
//  int _ = enter_wrapped_func("%p", attr);
//  int ret = real_pthread_rwlockattr_destroy(attr);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_getpshared (3posix) - get and set the process-shared attribute of the ... */
//int __wrap(pthread_rwlockattr_getpshared)
//     (const pthread_rwlockattr_t *restrict attr, int *restrict pshared) {
//  int _ = enter_wrapped_func("%p, %p", attr, pshared);
//  int ret = real_pthread_rwlockattr_getpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_setpshared (3posix) - set the process-shared attribute of the read-wri... */
//int __wrap(pthread_rwlockattr_setpshared)
//     (pthread_rwlockattr_t *attr, int pshared) {
//  int _ = enter_wrapped_func("%p, %d", attr, pshared);
//  int ret = real_pthread_rwlockattr_setpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_getkind_np (3) - set/get the read-write lock kind of the thread read-w... */
//int __wrap(pthread_rwlockattr_getkind_np)(const pthread_rwlockattr_t *attr,
//					  int *pref) {
//  int _ = enter_wrapped_func("%p, %p", attr, pref);
//  int ret = real_pthread_rwlockattr_getkind_np(attr, pref);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_rwlockattr_setkind_np (3) - set/get the read-write lock kind of the thread read-w... */
//int __wrap(pthread_rwlockattr_setkind_np)(pthread_rwlockattr_t *attr,
//					  int pref) {
//  int _ = enter_wrapped_func("%p, %d", attr, pref);
//  int ret = real_pthread_rwlockattr_setkind_np(attr, pref);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif
//
///* ------------------------------
//   --- condition variables 
//   ------------------------------ */
//
static cfibre_condattr_t *
pthread_condattr_to_cfibre(const pthread_condattr_t * p, cfibre_condattr_t * m) {
  if (!p) {
    return 0;
  } else {
    *m = (cfibre_condattr_t){0};
    return m;
  }
}

int __wrap(pthread_cond_init)(pthread_cond_t *restrict cond,
			      const pthread_condattr_t *restrict attr) {
	int ret;
	cfibre_condattr_t mattr_[1];
	cfibre_condattr_t * mattr = pthread_condattr_to_cfibre(attr, mattr_);
	//assert(sizeof(cfibre_cond_t) <= sizeof(pthread_cond_t));
	ret = cfibre_cond_init((cfibre_cond_t *)cond, mattr);
#ifdef DEBUG
	printf("in wrap init cond\n");
#endif
	return ret;
}
// pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
//
/* pthread_cond_destroy (3posix) - destroy and initialize condition variables */
int __wrap(pthread_cond_destroy)(pthread_cond_t *cond) {
	int ret;
	ret = cfibre_cond_destroy((cfibre_cond_t *)cond);
#ifdef DEBUG
	printf("in wrap destroy cond\n");
#endif
	return ret;
}

/* pthread_cond_signal (3posix) - signal a condition */
int __wrap(pthread_cond_signal)(pthread_cond_t *cond) {
	int ret;
	ret = cfibre_cond_signal((cfibre_cond_t *)cond);
#ifdef DEBUG
	printf("in wrap signal cond\n");
#endif
	return ret;
}

/* pthread_cond_broadcast (3posix) - broadcast or signal a condition */
int __wrap(pthread_cond_broadcast)(pthread_cond_t *cond) {
	int ret;
	ret = cfibre_cond_broadcast((cfibre_cond_t *)cond);
#ifdef DEBUG
	printf("in wrap broadcast cond\n");
#endif
	return ret;
}

int __wrap(pthread_cond_wait)(pthread_cond_t *restrict cond,
			      pthread_mutex_t *restrict mutex) {
	int ret;
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_cond_wait((cfibre_cond_t *)cond, (cfibre_mutex_t *)mutex);
#ifdef DEBUG
	printf("in wrap wait cond\n");
#endif
	return ret;
}

/* pthread_cond_timedwait (3posix) - wait on a condition */
int __wrap(pthread_cond_timedwait)(pthread_cond_t *restrict cond,
				   pthread_mutex_t *restrict mutex,
				   const struct timespec *restrict abstime) {
	int ret;
    assert(sizeof(cfibre_mutex_t) <= sizeof(pthread_mutex_t));
	ret = cfibre_cond_timedwait((cfibre_cond_t *)cond,
						   (cfibre_mutex_t *)mutex, abstime);
#ifdef DEBUG
	printf("in wrap timedwait cond\n");
#endif
	return ret;
}

/* pthread_condattr_init (3posix) - initialize the condition variable attributes object */
int __wrap(pthread_condattr_init)(pthread_condattr_t *attr) {
  int ret = __real(pthread_condattr_init)(attr);
  return ret;
}

/* pthread_condattr_destroy (3posix) - destroy and initialize the condition variable attribu... */
int __wrap(pthread_condattr_destroy)(pthread_condattr_t *attr) {
  int ret = __real(pthread_condattr_destroy)(attr);
  return ret;
}

/* pthread_condattr_getpshared (3posix) - get and set the process-shared condition variable ... */
int __wrap(pthread_condattr_getpshared)
     (const pthread_condattr_t *restrict attr, int *restrict pshared) {
  int ret = __real(pthread_condattr_getpshared)(attr, pshared);
  return ret;
}

/* pthread_condattr_setpshared (3posix) - set the process-shared condition variable attribute */
int __wrap(pthread_condattr_setpshared)(pthread_condattr_t *attr,
					int pshared) {
  int ret = __real(pthread_condattr_setpshared)(attr, pshared);
  return ret;
}

//#if defined(HAVE_PTHREAD_CONDATTR_CLOCK)
///* pthread_condattr_getclock (3posix) - get and set the clock selection condition variable a... */
//int __wrap(pthread_condattr_getclock)
//     (const pthread_condattr_t *restrict attr,
//      clockid_t *restrict clock_id) {
//  int _ = enter_wrapped_func("%p, %p", attr, clock_id);
//  int ret = real_pthread_condattr_getclock(attr, clock_id);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_condattr_setclock (3posix) - set the clock selection condition variable attribute */
//int __wrap(pthread_condattr_setclock)(pthread_condattr_t *attr,
//				      clockid_t clock_id) {
//  int _ = enter_wrapped_func("%p, %x", attr, clock_id);
//  (void)_;
//  int ret = real_pthread_condattr_setclock(attr, clock_id);
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_CONDATTR_CLOCK */
//
///* ----------------------
//   --- spin locks
//   ---------------------- */
//
//#if defined(HAVE_PTHREAD_SPIN)
//int __wrap(pthread_spin_init)(pthread_spinlock_t *lock, int pshared) {
//  int _ = enter_wrapped_func("%p %d", lock, pshared);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    assert(sizeof(myth_spinlock_t) <= sizeof(pthread_spinlock_t));
//    ret = myth_spin_init_body((myth_spinlock_t *)lock);
//  } else {
//    ret = real_pthread_spin_init(lock, pshared);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_spin_destroy (3posix) - destroy or initialize a spin lock object */
//int __wrap(pthread_spin_destroy)(pthread_spinlock_t *lock) {
//  int _ = enter_wrapped_func("%p", lock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_spin_destroy_body((myth_spinlock_t *)lock);
//  } else {
//    ret = real_pthread_spin_destroy(lock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_spin_lock (3posix) - lock a spin lock object */
//int __wrap(pthread_spin_lock)(pthread_spinlock_t *lock) {
//  int _ = enter_wrapped_func("%p", lock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_spin_lock_body((myth_spinlock_t *)lock);
//  } else {
//    ret = real_pthread_spin_lock(lock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* pthread_spin_trylock (3posix) - lock a spin lock object */
//int __wrap(pthread_spin_trylock)(pthread_spinlock_t *lock) {
//  int _ = enter_wrapped_func("%p", lock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_spin_trylock_body((myth_spinlock_t *)lock);
//  } else {
//    ret = real_pthread_spin_trylock(lock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//
///* pthread_spin_unlock (3posix) - unlock a spin lock object */
//int __wrap(pthread_spin_unlock)(pthread_spinlock_t *lock) {
//  int _ = enter_wrapped_func("%p", lock);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_spin_unlock_body((myth_spinlock_t *)lock);
//  } else {
//    ret = real_pthread_spin_unlock(lock);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#endif	 /* defined(HAVE_PTHREAD_SPIN) */
//
//
//
//
//#if defined(HAVE_PTHREAD_BARRIER)
//
///* ----------------------------
//   --- barrier 
//   ---------------------------- */
//
static cfibre_barrierattr_t *
pthread_barrierattr_to_libfibre(const pthread_barrierattr_t * p,
			    cfibre_barrierattr_t * m) {
  if (!p) {
    return 0;
  } else {
	(*m) = (cfibre_barrier_t){0};
    return m;
  }
}

int __wrap(pthread_barrier_init)(pthread_barrier_t *restrict barrier,
				 const pthread_barrierattr_t *restrict attr,
				 unsigned count) {
	int ret;
    cfibre_barrierattr_t mattr_[1];
    cfibre_barrierattr_t * mattr = pthread_barrierattr_to_libfibre(attr, mattr_);
    ret = cfibre_barrier_init((cfibre_barrier_t *)barrier, mattr, count);
#ifdef DEBUG
	printf("in wrap init barrier\n");
#endif
	return ret;
}

/* pthread_barrier_destroy (3posix) - destroy and initialize a barrier object */
int __wrap(pthread_barrier_destroy)(pthread_barrier_t *barrier) {
	int ret;
    assert(sizeof(cfibre_barrier_t) <= sizeof(pthread_barrier_t));
	ret = cfibre_barrier_destroy((cfibre_barrier_t *)barrier);
#ifdef DEBUG
	printf("in wrap destroy barrier\n");
#endif
	return ret;
}

/* pthread_barrier_wait (3posix) - synchronize at a barrier */
int __wrap(pthread_barrier_wait)(pthread_barrier_t *barrier) {
	int ret;
    assert(sizeof(cfibre_barrier_t) <= sizeof(pthread_barrier_t));
	ret = cfibre_barrier_wait((cfibre_barrier_t *)barrier);
#ifdef DEBUG
	printf("in wrap barrier wait\n");
#endif
	return ret;
}

/* pthread_barrierattr_init (3posix) - initialize the barrier attributes object */
int __wrap(pthread_barrierattr_init)(pthread_barrierattr_t *attr) {
  int ret = __real(pthread_barrierattr_init)(attr);
  return ret;
}

/* pthread_barrierattr_destroy (3posix) - destroy and initialize the barrier attributes object */
int __wrap(pthread_barrierattr_destroy)(pthread_barrierattr_t *attr) {
  int ret = __real(pthread_barrierattr_destroy)(attr);
  return ret;
}

///* pthread_barrierattr_getpshared (3posix) - get and set the process-shared attribute of the... */
//int __wrap(pthread_barrierattr_getpshared)
//     (const pthread_barrierattr_t *restrict attr, int *restrict pshared) {
//  int _ = enter_wrapped_func("%p, %p", attr, pshared);
//  int ret = real_pthread_barrierattr_getpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
///* pthread_barrierattr_setpshared (3posix) - set the process-shared attribute of the barrier... */
//int __wrap(pthread_barrierattr_setpshared)
//     (pthread_barrierattr_t *attr, int pshared) {
//  int _ = enter_wrapped_func("%p, %d", attr, pshared);
//  int ret = real_pthread_barrierattr_setpshared(attr, pshared);
//  (void)_;
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#endif	/* HAVE_PTHREAD_BARRIER */
//
/* --------------------------------
   --- thread local storage 
   -------------------------------- */

/* pthread_key_create (3posix) - thread-specific data key creation */
// pthread_key_t is larger than cfibre_key_t.so possibly cause bag.
int __wrap(pthread_key_create)(pthread_key_t *key, void (*destructor)(void *)) {
	int ret;
	ret = cfibre_key_create((cfibre_key_t *)key, destructor);
#ifdef DEBUG
	printf("in wrap key create\n");
#endif
	return ret;
}

/* pthread_key_delete (3posix) - thread-specific data key deletion */
int __wrap(pthread_key_delete)(pthread_key_t key) {
	int ret;
    assert(sizeof(cfibre_barrier_t) <= sizeof(pthread_barrier_t));
	ret = cfibre_key_delete((cfibre_key_t)key);
#ifdef DEBUG
	printf("in wrap key delete\n");
#endif
	return ret;
}

/* pthread_getspecific (3posix) - thread-specific data management */
void * __wrap(pthread_getspecific)(pthread_key_t key) {
	void * ret;
    assert(sizeof(cfibre_barrier_t) <= sizeof(pthread_barrier_t));
	ret = cfibre_getspecific((cfibre_key_t)key);
#ifdef DEBUG
	printf("in wrap specific key\n");
#endif
	return ret;
}
/* pthread_setspecific (3posix) - thread-specific data management */
int __wrap(pthread_setspecific)(pthread_key_t key, const void *value) {
	int ret;
    assert(sizeof(cfibre_barrier_t) <= sizeof(pthread_barrier_t));
	ret = cfibre_setspecific((cfibre_key_t)key, value);
#ifdef DEBUG
	printf("in wrap set value into tls\n");
#endif
	return ret;
}

///* -------------------------------------------
//   --- functions querying and affecting threads
//   ------------------------------------------- */
//
//#if defined(HAVE_PTHREAD_GETCPUCLOCKID)
///* pthread_getcpuclockid (3) - retrieve ID of a threads CPU time clock */
//int __wrap(pthread_getcpuclockid)(pthread_t thread, clockid_t *clock_id) {
//  int _ = enter_wrapped_func("%x, %p", thread, clock_id);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOENT;
//  } else {
//    ret = real_pthread_getcpuclockid(thread, clock_id);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* defined(HAVE_PTHREAD_GETCPUCLOCKID) */
//
//#if 0
///* pthread_atfork (3posix) - register fork handlers */
//int __wrap(pthread_atfork)(void (*prepare)(void), void (*parent)(void),
//			   void (*child)(void)) {
//  int _ = enter_wrapped_func("%p, %p, %p", prepare, parent, child);
//  int x = real_pthread_atfork(prepare, parent, child);
//  leave_wrapped_func("%d", x);
//  return x;
//}
//#endif
//
///* -----------------------------
//   --- kill 
//   ----------------------------- */
///* pthread_kill (3)     - send a signal to a thread */
///* pthread_kill (3posix) - send a signal to a thread */
//int __wrap(pthread_kill)(pthread_t thread, int sig) {
//  int _ = enter_wrapped_func("%x, %d", thread, sig);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOENT;
//  } else {
//    ret = real_pthread_kill(thread, sig);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if 0
///* pthread_kill_other_threads_np (3) - terminate all other threads in process */
//void __wrap(pthread_kill_other_threads_np)(void) {
//  enter_wrapped_func(0);
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//  } else {
//    real_pthread_kill_other_threads_np();
//  }
//  leave_wrapped_func(0);
//}
//#endif
//
//#if defined(HAVE_PTHREAD_SIGQUEUE)
///* pthread_sigqueue (3) - queue a signal and data to a thread */
//int __wrap(pthread_sigqueue)(pthread_t thread, int sig,
//			     const union sigval value) {
//  int _ = enter_wrapped_func("%x, %d, ...", thread, sig);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_sigqueue(thread, sig, value);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//#endif /* HAVE_PTHREAD_SIGQUEUE */
//
///* pthread_sigmask (3)  - examine and change mask of blocked signals */
///* pthread_sigmask (3posix) - examine and change blocked signals */
//int __wrap(pthread_sigmask)(int how, const sigset_t *set, sigset_t *oldset) {
//  int _ = enter_wrapped_func("%d, %p, %p", how, set, oldset);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    myth_wrap_pthread_warn_non_conforming();
//    ret = ENOSYS;
//  } else {
//    ret = real_pthread_sigmask(how, set, oldset);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
///* --------------------------------
//   --- thread-related functions 
//   -------------------------------- */
int __wrap(sched_yield)(void) {
//  int _ = enter_wrapped_func(0);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_yield_body();
//  } else {
//    ret = real_sched_yield();
//  }
//  leave_wrapped_func(0);
//  return ret;
  int ret = cfibre_yield();
  return ret;
}
//
//unsigned int __wrap(sleep)(unsigned int s) {
//  int _ = enter_wrapped_func("%u", s);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_sleep_body(s);
//  } else {
//    ret = real_sleep(s);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//int __wrap(usleep)(useconds_t usec) {
//  int _ = enter_wrapped_func("%u", usec);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_usleep_body(usec);
//  } else {
//    ret = real_usleep(usec);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//int __wrap(nanosleep)(const struct timespec *req, struct timespec *rem) {
//  int _ = enter_wrapped_func("%p, %p", req, rem);
//  int ret;
//  (void)_;
//  if (myth_should_wrap_pthread()) {
//    ret = myth_nanosleep_body(req, rem);
//  } else {
//    ret = real_nanosleep(req, rem);
//  }
//  leave_wrapped_func("%d", ret);
//  return ret;
//}
//
//#if 0 
//
///* -------------------------
//   --- clean-up handlers 
//   ------------------------- */
//
///* pthread_cleanup_push (3) - push and pop thread cancellation clean-up handlers */
//void __wrap(pthread_cleanup_push)(void (*routine)(void *),
//				  void *arg) {
//  enter_wrapped_func("%p, %p", routine, arg);
//  real_pthread_cleanup_push(routine, arg;
//  leave_wrapped_func("%d", x);
//}
//
///* pthread_cleanup_push_defer_np (3) - push and pop thread cancellation clean-up handlers wh... */
//void __wrap(pthread_cleanup_push_defer_np)(void (*routine)(void *),
//					   void *arg) {
//  enter_wrapped_func("%p, %p", routine, arg);
//  real_pthread_cleanup_push_defer_np(routine, arg);
//  leave_wrapped_func("%d", x);
//}
//
///* pthread_cleanup_pop (3) - push and pop thread cancellation clean-up handlers */
//void __wrap(pthread_cleanup_pop)(int execute) {
//  enter_wrapped_func("%d", execute);
//  real_pthread_cleanup_pop(execute);
//  leave_wrapped_func("%d", x);
//}
//
///* pthread_cleanup_pop_restore_np (3) - push and pop thread cancellation clean-up handlers w... */
//void __wrap(pthread_cleanup_pop_restore_np)(int execute) {
//  enter_wrapped_func("%d", execute);
//  real_pthread_cleanup_pop_restore_np(execute);
//  leave_wrapped_func("%d", x);
//}
//
//#endif
//
///* define all functions man -k pthread_ displays */
//
//#if 0
//pthread_atfork (3posix) - register fork handlers
//pthread_attr_destroy (3) - initialize and destroy thread attributes object
//pthread_attr_destroy (3posix) - destroy and initialize the thread attributes object
//pthread_attr_getaffinity_np (3) - set/get CPU affinity attribute in thread attributes object
//pthread_attr_getdetachstate (3) - set/get detach state attribute in thread attributes object
//pthread_attr_getdetachstate (3posix) - get and set the detachstate attribute
//pthread_attr_getguardsize (3) - set/get guard size attribute in thread attributes object
//pthread_attr_getguardsize (3posix) - get and set the thread guardsize attribute
//pthread_attr_getinheritsched (3) - set/get inherit-scheduler attribute in thread attribut...
//pthread_attr_getinheritsched (3posix) - get and set the inheritsched attribute (REALTIME ...
//pthread_attr_getschedparam (3) - set/get scheduling parameter attributes in thread attrib...
//pthread_attr_getschedparam (3posix) - get and set the schedparam attribute
//pthread_attr_getschedpolicy (3) - set/get scheduling policy attribute in thread attribute...
//pthread_attr_getschedpolicy (3posix) - get and set the schedpolicy attribute (REALTIME TH...
//pthread_attr_getscope (3) - set/get contention scope attribute in thread attributes object
//pthread_attr_getscope (3posix) - get and set the contentionscope attribute (REALTIME THRE...
//pthread_attr_getstack (3) - set/get stack attributes in thread attributes object
//pthread_attr_getstack (3posix) - get and set stack attributes
//pthread_attr_getstackaddr (3) - set/get stack address attribute in thread attributes object
//pthread_attr_getstacksize (3) - set/get stack size attribute in thread attributes object
//pthread_attr_getstacksize (3posix) - get and set the stacksize attribute
//pthread_attr_init (3) - initialize and destroy thread attributes object
//pthread_attr_init (3posix) - initialize the thread attributes object
//pthread_attr_setaffinity_np (3) - set/get CPU affinity attribute in thread attributes object
//pthread_attr_setdetachstate (3) - set/get detach state attribute in thread attributes object
//pthread_attr_setdetachstate (3posix) - set the detachstate attribute
//pthread_attr_setguardsize (3) - set/get guard size attribute in thread attributes object
//pthread_attr_setguardsize (3posix) - set the thread guardsize attribute
//pthread_attr_setinheritsched (3) - set/get inherit-scheduler attribute in thread attribut...
//pthread_attr_setinheritsched (3posix) - set the inheritsched attribute (REALTIME THREADS)
//pthread_attr_setschedparam (3) - set/get scheduling parameter attributes in thread attrib...
//pthread_attr_setschedparam (3posix) - set the schedparam attribute
//pthread_attr_setschedpolicy (3) - set/get scheduling policy attribute in thread attribute...
//pthread_attr_setschedpolicy (3posix) - set the schedpolicy attribute (REALTIME THREADS)
//pthread_attr_setscope (3) - set/get contention scope attribute in thread attributes object
//pthread_attr_setscope (3posix) - set the contentionscope attribute (REALTIME THREADS)
//pthread_attr_setstack (3) - set/get stack attributes in thread attributes object
//pthread_attr_setstack (3posix) - set the stack attribute
//pthread_attr_setstackaddr (3) - set/get stack address attribute in thread attributes object
//pthread_attr_setstacksize (3) - set/get stack size attribute in thread attributes object
//pthread_attr_setstacksize (3posix) - set the stacksize attribute
//pthread_barrier_destroy (3posix) - destroy and initialize a barrier object
//pthread_barrier_wait (3posix) - synchronize at a barrier
//pthread_barrierattr_destroy (3posix) - destroy and initialize the barrier attributes object
//pthread_barrierattr_getpshared (3posix) - get and set the process-shared attribute of the...
//pthread_barrierattr_init (3posix) - initialize the barrier attributes object
//pthread_barrierattr_setpshared (3posix) - set the process-shared attribute of the barrier...
//pthread_cancel (3)   - send a cancellation request to a thread
//pthread_cancel (3posix) - cancel execution of a thread
//pthread_cleanup_pop (3) - push and pop thread cancellation clean-up handlers
//pthread_cleanup_pop (3posix) - establish cancellation handlers
//pthread_cleanup_pop_restore_np (3) - push and pop thread cancellation clean-up handlers w...
//pthread_cleanup_push (3) - push and pop thread cancellation clean-up handlers
//pthread_cleanup_push_defer_np (3) - push and pop thread cancellation clean-up handlers wh...
//pthread_cond_broadcast (3posix) - broadcast or signal a condition
//pthread_cond_destroy (3posix) - destroy and initialize condition variables
//pthread_cond_signal (3posix) - signal a condition
//pthread_cond_timedwait (3posix) - wait on a condition
//pthread_condattr_destroy (3posix) - destroy and initialize the condition variable attribu...
//pthread_condattr_getclock (3posix) - get and set the clock selection condition variable a...
//pthread_condattr_getpshared (3posix) - get and set the process-shared condition variable ...
//pthread_condattr_init (3posix) - initialize the condition variable attributes object
//pthread_condattr_setclock (3posix) - set the clock selection condition variable attribute
//pthread_condattr_setpshared (3posix) - set the process-shared condition variable attribute
//pthread_create (3)   - create a new thread
//pthread_create (3posix) - thread creation
//pthread_detach (3)   - detach a thread
//pthread_detach (3posix) - detach a thread
//pthread_equal (3)    - compare thread IDs
//pthread_equal (3posix) - compare thread IDs
//pthread_exit (3)     - terminate calling thread
//pthread_exit (3posix) - thread termination
//pthread_getaffinity_np (3) - set/get CPU affinity of a thread
//pthread_getattr_np (3) - get attributes of created thread
//pthread_getconcurrency (3) - set/get the concurrency level
//pthread_getconcurrency (3posix) - get and set the level of concurrency
//pthread_getcpuclockid (3) - retrieve ID of a threads CPU time clock
//pthread_getcpuclockid (3posix) - access a thread CPU-time clock (ADVANCED REALTIME THREADS)
//pthread_getname_np (3) - set/get the name of a thread
//pthread_getschedparam (3) - set/get scheduling policy and parameters of a thread
//pthread_getschedparam (3posix) - dynamic thread scheduling parameters access (REALTIME TH...
//pthread_getspecific (3posix) - thread-specific data management
//pthread_join (3)     - join with a terminated thread
//pthread_join (3posix) - wait for thread termination
//pthread_key_create (3posix) - thread-specific data key creation
//pthread_key_delete (3posix) - thread-specific data key deletion
//pthread_kill (3)     - send a signal to a thread
//pthread_kill (3posix) - send a signal to a thread
//pthread_kill_other_threads_np (3) - terminate all other threads in process
//pthread_mutex_consistent (3posix) - mark state protected by robust mutex as consistent
//pthread_mutex_destroy (3posix) - destroy and initialize a mutex
//pthread_mutex_getprioceiling (3posix) - get and set the priority ceiling of a mutex (REAL...
//pthread_mutex_init (3posix) - destroy and initialize a mutex
//pthread_mutex_lock (3posix) - lock and unlock a mutex
//pthread_mutex_setprioceiling (3posix) - change the priority ceiling of a mutex (REALTIME ...
//pthread_mutex_timedlock (3posix) - lock a mutex
//pthread_mutex_trylock (3posix) - lock and unlock a mutex
//pthread_mutexattr_destroy (3posix) - destroy and initialize the mutex attributes object
//pthread_mutexattr_getprioceiling (3posix) - get and set the prioceiling attribute of the ...
//pthread_mutexattr_getprotocol (3posix) - get and set the protocol attribute of the mutex ...
//pthread_mutexattr_getpshared (3posix) - get and set the process-shared attribute
//pthread_mutexattr_getrobust (3posix) - get and set the mutex robust attribute
//pthread_mutexattr_gettype (3posix) - get and set the mutex type attribute
//pthread_mutexattr_init (3posix) - initialize the mutex attributes object
//pthread_mutexattr_setprioceiling (3posix) - set the prioceiling attribute of the mutex at...
//pthread_mutexattr_setprotocol (3posix) - set the protocol attribute of the mutex attribut...
//pthread_mutexattr_setpshared (3posix) - set the process-shared attribute
//pthread_mutexattr_setrobust (3posix) - get and set the mutex robust attribute
//pthread_mutexattr_settype (3posix) - set the mutex type attribute
//pthread_once (3posix) - dynamic package initialization
//pthread_rwlock_destroy (3posix) - destroy and initialize a read-write lock object
//pthread_rwlock_rdlock (3posix) - lock a read-write lock object for reading
//pthread_rwlock_timedrdlock (3posix) - lock a read-write lock for reading
//pthread_rwlock_timedwrlock (3posix) - lock a read-write lock for writing
//pthread_rwlock_tryrdlock (3posix) - lock a read-write lock object for reading
//pthread_rwlock_trywrlock (3posix) - lock a read-write lock object for writing
//pthread_rwlock_unlock (3posix) - unlock a read-write lock object
//pthread_rwlock_wrlock (3posix) - lock a read-write lock object for writing
//pthread_rwlockattr_destroy (3posix) - destroy and initialize the read-write lock attribut...
//pthread_rwlockattr_getkind_np (3) - set/get the read-write lock kind of the thread read-w...
//pthread_rwlockattr_getpshared (3posix) - get and set the process-shared attribute of the ...
//pthread_rwlockattr_init (3posix) - initialize the read-write lock attributes object
//pthread_rwlockattr_setkind_np (3) - set/get the read-write lock kind of the thread read-w...
//pthread_rwlockattr_setpshared (3posix) - set the process-shared attribute of the read-wri...
//pthread_self (3)     - obtain ID of the calling thread
//pthread_self (3posix) - get the calling thread ID
//pthread_setaffinity_np (3) - set/get CPU affinity of a thread
//pthread_setcancelstate (3) - set cancelability state and type
//pthread_setcancelstate (3posix) - set cancelability state
//pthread_setcanceltype (3) - set cancelability state and type
//pthread_setconcurrency (3) - set/get the concurrency level
//pthread_setconcurrency (3posix) - set the level of concurrency
//pthread_setname_np (3) - set/get the name of a thread
//pthread_setschedparam (3) - set/get scheduling policy and parameters of a thread
//pthread_setschedparam (3posix) - dynamic thread scheduling parameters access (REALTIME TH...
//pthread_setschedprio (3) - set scheduling priority of a thread
//pthread_setschedprio (3posix) - dynamic thread scheduling parameters access (REALTIME THR...
//pthread_setspecific (3posix) - thread-specific data management
//pthread_sigmask (3)  - examine and change mask of blocked signals
//pthread_sigmask (3posix) - examine and change blocked signals
//pthread_sigqueue (3) - queue a signal and data to a thread
//pthread_spin_destroy (3posix) - destroy or initialize a spin lock object
//pthread_spin_lock (3posix) - lock a spin lock object
//pthread_spin_unlock (3posix) - unlock a spin lock object
//pthread_testcancel (3) - request delivery of any pending cancellation request
//pthread_testcancel (3posix) - set cancelability state
//pthread_timedjoin_np (3) - try to join with a terminated thread
//pthread_tryjoin_np (3) - try to join with a terminated thread
//pthread_yield (3)    - yield the processor
//#endif

