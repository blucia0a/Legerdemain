#include <pthread.h>
#include <stdio.h>
#include "legerdemain.h"


//int (*LDM_pthread_mutex_lock)(pthread_mutex_t *mutex);
//int (*LDM_pthread_mutex_unlock)(pthread_mutex_t *mutex);

LDM_ORIG_DECL(int,pthread_mutex_lock,pthread_mutex_t *);
int pthread_mutex_lock(pthread_mutex_t *lock){
  ldmmsg(stderr,"[LockWatcher] Lock\n");
  return LDM_ORIG(pthread_mutex_lock)(lock);
}

LDM_ORIG_DECL(int,pthread_mutex_unlock,pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *lock){
  ldmmsg(stderr,"[LockWatcher] Unlock\n");
  return LDM_ORIG(pthread_mutex_unlock)(lock);
}

static void __attribute__ ((constructor)) init();
static void init(){

  ldmmsg(stderr,"Initializing LockWatcher Plugin!\n");
  LDM_REG(pthread_mutex_lock);
  LDM_REG(pthread_mutex_unlock);

}
