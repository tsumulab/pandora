#include "libfibre_wrap_util_func.h"

int __real(pthread_create)(pthread_t *thread, const pthread_attr_t *attr,void *(*start_routine) (void *), void *arg);
void __real(pthread_exit)(void *retval);
int __real(pthread_join)(pthread_t thread, void **retval);
int __real(pthread_attr_getdetachstate)(const pthread_attr_t *attr, int *detachstate);
int __real(pthread_attr_setdetachstate)(pthread_attr_t *attr, int detachstate);
int __real(pthread_attr_getguardsize)(const pthread_attr_t *attr, size_t *guardsize);
int __real(pthread_attr_setguardsize)(pthread_attr_t *attr, size_t guardsize);
int __real(pthread_barrierattr_init)(pthread_barrierattr_t *attr);
int __real(pthread_barrierattr_destroy)(pthread_barrierattr_t *attr);
int __real(pthread_condattr_init)(pthread_condattr_t *attr);
int __real(pthread_condattr_destroy)(pthread_condattr_t *attr);
int __real(pthread_condattr_getpshared)(const pthread_condattr_t *restrict attr, int *restrict pshared);
int __real(pthread_condattr_setpshared)(pthread_condattr_t *attr,	int pshared);


int __real(pthread_mutex_init)(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr);
// int __real(pthread_mutex_destroy)(pthread_mutex_t *mutex);
// int __wrap(pthread_mutex_lock)(pthread_mutex_t *mutex)
// int __wrap(pthread_mutex_unlock)(pthread_mutex_t *mutex)

int __real(pthread_condattr_init)(pthread_condattr_t *attr);
int __real(pthread_condattr_destroy)(pthread_condattr_t *attr);
int __real(pthread_condattr_getpshared)(const pthread_condattr_t *restrict attr, int *restrict pshared);
int __real(pthread_condattr_setpshared)(pthread_condattr_t *attr,	int pshared);
