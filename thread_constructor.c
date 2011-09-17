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

  ldmmsg(stderr,"Running thread constructors for thread %lu\n",(unsigned long)pthread_self());
  threadInitData *tid = (threadInitData *)arg;   

  int i;
  for(i = 0; i < LDM_numCtrs; i++){
    if(LDM_thd_ctr[i] != NULL){
      LDM_thd_ctr[i]( tid->arg, tid->start_routine );
    }
  }

  return (tid->start_routine(tid->arg)); 

}

LDM_ORIG_DECL(int, pthread_create, pthread_t *, const pthread_attr_t *, void *(*)(void*), void *);
int pthread_create( pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){

  threadInitData *tid = (threadInitData*)malloc(sizeof(*tid));
  tid->start_routine = start_routine;
  tid->arg = arg;
  int ret = LDM_ORIG(pthread_create)(thread,attr,threadStartFunc,(void*)tid);
  return ret;

}

void loadConstructor(char *ctrLibName, void *data){

  if(LDM_numCtrs == LDM_MAX_TCTRS){
    ldmmsg(stderr,"Warning: cannot register thread constructor \"%s\" because there are already %d registered\n",ctrLibName,LDM_MAX_TCTRS);
  }

  void *pluginObject = dlopen(ctrLibName, RTLD_LOCAL | RTLD_LAZY);
  if(pluginObject == NULL){
    ldmmsg(stderr,"Couldn't dlopen %s\n",ctrLibName);
    ldmmsg(stderr,"%s\n",dlerror());
  }
  void *fptr = dlsym(pluginObject, "init_thread");

  if(fptr != NULL){

    LDM_thd_ctr[LDM_numCtrs++] = fptr;

  }else{

    ldmmsg(stderr,"Warning: can't load thread constructor init_thread() from %s\n",ctrLibName);

  }

}

void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
void init(){

  ldmmsg(stderr,"Running libthread_constructor initializer\n"); 
  
  char *p = getenv("LDM_TCTR"); 
  ldmmsg(stderr,"Trying to Load %s\n",p);
  applyToTokens(p,":,",loadConstructor,NULL);


  LDM_REG(pthread_create);
 
}

void deinit(){
  
}
