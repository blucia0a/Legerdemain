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
  /*
  void *h = NULL;
  h = dlopen("/lib/libpthread.so.0",RTLD_LAZY);
  if(h == NULL){
    fprintf(stderr,"couldn't open pthreads %s\n",dlerror());
  }else{
    LDM_pthread_mutex_lock = dlsym(h,"pthread_mutex_lock");
    LDM_pthread_mutex_unlock = dlsym(h,"pthread_mutex_unlock");
    if(LDM_pthread_mutex_lock == NULL){
      fprintf(stderr,"Couldn't get lock\n");
    }
    if(LDM_pthread_mutex_unlock == NULL){
      fprintf(stderr,"Couldn't get unlock\n");
    }
  }
  */
}
