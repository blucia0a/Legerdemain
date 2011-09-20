#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdlib.h>
#include "Applier.h"

#include "legerdemain.h"

#define LDM_MAX_TCTRS 16
void (*LDM_thd_ctr[LDM_MAX_TCTRS])(void*,void*(*)(void*));
int LDM_numCtrs;

typedef struct _threadInitData{
  void *(*start_routine)(void*);
  void *arg;
} threadInitData;

void *threadStartFunc(void *arg){

  ldmmsg(stderr,"[THREAD CTR] Initializing Thread %lu\n",(unsigned long)pthread_self());
  threadInitData *tid = (threadInitData *)arg;   

  int i;
  for(i = 0; i < LDM_numCtrs; i++){
    if(LDM_thd_ctr[i] != NULL){
      LDM_thd_ctr[i]( tid->arg, tid->start_routine );
    }
  }

  return (tid->start_routine(tid->arg)); 

}

LDM_ORIG_DECL(int,pthread_create,pthread_t *, const pthread_attr_t *, void *(*)(void*), void *);
int pthread_create( pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){

  threadInitData *tid = (threadInitData*)malloc(sizeof(*tid));
  tid->start_routine = start_routine;
  tid->arg = arg;
  int ret = LDM_ORIG(pthread_create)(thread,attr,threadStartFunc,(void*)tid);
  return ret;

}

void loadConstructor(char *ctrLibName, void *data){

  if(LDM_numCtrs == LDM_MAX_TCTRS){
    ldmmsg(stderr,"[THREAD CTR] Warning: cannot register thread constructor \"%s\" because there are already %d registered\n",ctrLibName,LDM_MAX_TCTRS);
  }

  void *pluginObject = dlopen(ctrLibName, RTLD_LOCAL | RTLD_LAZY);
  if(pluginObject == NULL){
    ldmmsg(stderr,"[THREAD CTR] Couldn't dlopen %s\n",ctrLibName);
    ldmmsg(stderr,"[THREAD CTR] %s\n",dlerror());
  }
  void *fptr = dlsym(pluginObject, "init_thread");

  if(fptr != NULL){

    LDM_thd_ctr[LDM_numCtrs++] = fptr;

  }else{

    ldmmsg(stderr,"[THREAD CTR] Warning: can't load thread constructor init_thread() from %s\n",ctrLibName);

  }

}

static void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
static void init(){

  ldmmsg(stderr,"[THREAD CTR] Initializing...\n"); 
  LDM_REG(pthread_create);
  char *p = getenv("LDM_TCTR"); 
  if( p != NULL ){
    ldmmsg(stderr,"[THREAD CTR] Thread Constructor: %s\n",p);
    applyToTokens(p,":,",loadConstructor,NULL);
  }
 
}

void deinit(){
  
}
