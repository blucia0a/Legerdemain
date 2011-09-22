#include <pthread.h>
#include <stdio.h>
#include "legerdemain.h"

LDM_ORIG_DECL(int,pthread_mutex_lock,pthread_mutex_t *);
int pthread_mutex_lock(pthread_mutex_t *lock){

  return LDM_ORIG(pthread_mutex_lock)(lock);

}

LDM_ORIG_DECL(int,pthread_mutex_unlock,pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *lock){

  return LDM_ORIG(pthread_mutex_unlock)(lock);

}

static void __attribute__ ((constructor)) init();
static void init(){

  ldmmsg(stderr,"[LockWatcher] Initializing LockWatcher Plugin!\n");
  LDM_REG(pthread_mutex_lock);
  LDM_REG(pthread_mutex_unlock);

}

static void __attribute__ ((destructor)) deinit();
static void deinit(){

  ldmmsg(stderr,"[LockWatcher] Deinitializing LockWatcher Plugin!\n");

}
