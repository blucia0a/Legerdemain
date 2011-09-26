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
    ldmmsg(stderr,"[TAP Manager] Warning: cannot register thread constructor \"%s\" because there are already %d registered\n",ctrLibName,LDM_MAX_TCTRS);
  }

  void *pluginObject = dlopen(ctrLibName, RTLD_LOCAL | RTLD_LAZY);
  if(pluginObject == NULL){
    ldmmsg(stderr,"[TAP Manager] Couldn't dlopen %s\n",ctrLibName);
    ldmmsg(stderr,"[TAP Manager] %s\n",dlerror());
  }

  void *fptr = dlsym(pluginObject, LDM_STR(LDM_PLUGIN_THD_INIT));

  if(fptr != NULL){

    LDM_thd_ctr[LDM_numCtrs++] = fptr;

  }

  /*it's just a standard plugin.  Load init manually, init_thread 
   *wasn't found, so this loads the library*/
  fptr = dlsym(pluginObject, LDM_STR(LDM_PLUGIN_INIT));

  if(fptr != NULL){

    ((void (*)())fptr)();

  }else{

    ldmmsg(stderr,"[TAP Manager] Warning: can't load %s or %s from %s [%s]\n",LDM_STR(LDM_PLUGIN_THD_INIT),LDM_STR(LDM_PLUGIN_INIT),ctrLibName,dlerror());

  }

}

static void __attribute__ ((constructor))init();
static void __attribute__ ((destructor)) deinit();
static void init(){

  ldmmsg(stderr,"[TAP Manager] Initializing...\n"); 
  LDM_REG(pthread_create);
  char *p = getenv("LDM_TAPS"); 
  if( p != NULL ){
    ldmmsg(stderr,"[TAP Manager] Loading TAP: %s\n",p);
    applyToTokens(p,":,",loadConstructor,NULL);
  }
 
}

static void deinit(){
  
}
