#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <malloc.h>

#include "legerdemain.h"

typedef struct _threadInitData{
  void *(*start_routine)(void*);
  void *arg;
} threadInitData;

void *threadStartFunc(void *arg){
  /*Do thread init stuff*/
  fprintf(stderr,"Thread Constructor for Thread %lu\n",(unsigned long)pthread_self());
  threadInitData *tid = (threadInitData *)arg;   
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

void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
void init(){
 
  LDM_REG(pthread_create);
  
 
}

void deinit(){
  
}
