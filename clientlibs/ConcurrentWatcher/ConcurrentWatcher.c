
#ifndef __USE_GNU
#define __USE_GNU
#define _GNU_SOURCE
#include <ucontext.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <pthread.h>

#include "drprobe.h"
#include "legerdemain.h"

#define MAX_NUM_THREADS 512
int (*myPthreadCreate)(pthread_t *thread,
              const pthread_attr_t *attr,
              void *(*start_routine)(void*), void *arg);
int (*myPthreadLock)(pthread_mutex_t *mutex);
int (*myPthreadUnlock)(pthread_mutex_t *mutex);

pthread_t monitorThread;

pthread_mutex_t threadListLock;
pthread_t threadList[MAX_NUM_THREADS];

LDM_THD_DTR_DECL(dkey,deinitthread);

void *gsrcAddr;
void *gsinkAddr;

__thread void *srcAddr;
__thread void *sinkAddr;
__thread unsigned long srcFreq;
__thread unsigned long sinkFreq;
__thread unsigned long threadId;


void *monitor(void *p){

  fprintf(stderr,"Monitor is %lu\n",(unsigned long)pthread_self());
  char *pp = getenv("LDM_RATE");
  unsigned long r;
  if(pp != NULL){
    sscanf(pp,"%lu",&r);
  }else{
    r = 1000;
  }
  while(1){

    //sleep a hundredth of a second
    usleep(r);

    myPthreadLock(&threadListLock);
    int i;
    for(i = 0; i < MAX_NUM_THREADS; i++){
      if( threadList[i] != (pthread_t)0 ){ 
        //Poke all live threads
        pthread_kill(threadList[i],SIGUSR1);
      }
    }
    myPthreadUnlock(&threadListLock);
  }
 
}

void handlePoke(int signum, siginfo_t *sinfo, void *ctx){

  if( srcAddr != NULL ){
    drp_watch_inst((unsigned long)srcAddr,0);
  }

  if( sinkAddr != NULL ){
    drp_watch_inst((unsigned long)sinkAddr,1);
  }
 
}

void handleTrap(int signum, siginfo_t *sinfo, void *ctx){
 
  void *ad = (void*)(((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP]);
  int which = drp_explain();
  if(which == 1){
    sinkFreq++;
  }else if(which == 0){
    srcFreq++;
  }
  drp_unwatch(0); 
  drp_unwatch(1); 

  return;
}

void deinit_thread(void *d){

  myPthreadLock(&threadListLock);
  drp_unwatch(0);
  drp_unwatch(1);
  int i;
  for(i = 0; i < MAX_NUM_THREADS; i++){
    if( pthread_equal(pthread_self(),threadList[i]) ){ 
      //Poke all live threads
      threadList[i] = (pthread_t)0;
    }
  }
  myPthreadUnlock(&threadListLock);

  fprintf(stderr,"%lu %lu %lu\n",threadId,srcFreq,sinkFreq);

}


void LDM_PLUGIN_INIT watch_addr_thd_ctr_init(){


  void *h = NULL;
  h = dlopen("libpthread.so.0",RTLD_LAZY);
  if(h == NULL){
    fprintf(stderr,"couldn't open pthreads %s\n",dlerror());
  }else{
    myPthreadCreate = dlsym(h,"pthread_create");
    myPthreadLock = dlsym(h,"pthread_mutex_lock");
    myPthreadUnlock= dlsym(h,"pthread_mutex_unlock");
    if(myPthreadCreate == NULL){
      fprintf(stderr,"Couldn't get pthread_create\n");
    }
    if(myPthreadLock == NULL){
      fprintf(stderr,"Couldn't get pthread_create\n");
    }
    if(myPthreadUnlock == NULL){
      fprintf(stderr,"Couldn't get pthread_create\n");
    }
  }

  drp_init();
   
  int i; 
  for(i = 0; i < MAX_NUM_THREADS; i++){
    threadList[i] = (pthread_t)0;
  }

  pthread_mutex_init(&threadListLock,NULL);

  LDM_REGISTER_THD_DTR(dkey,deinit_thread);

  myPthreadCreate(&monitorThread,NULL,monitor,NULL);

  char *srAd = getenv("LDM_SRC");
  char *siAd = getenv("LDM_SINK");

  if(srAd != NULL){
    sscanf(srAd,"%lx",&gsrcAddr);
  }

  if(siAd != NULL){
    sscanf(siAd,"%lx",&gsinkAddr);
  }

  struct sigaction sact;
  sact.sa_sigaction = handleTrap;
  sact.sa_flags = SA_SIGINFO;
  sigaction(SIGTRAP,&sact,NULL);



}

void init_thread(void *targ, void*(*thdrtn)(void*)){

  if(thdrtn == monitor){
    return; 
  }

  LDM_INSTALL_THD_DTR(dkey);

  struct sigaction zact;
  zact.sa_sigaction = handlePoke;
  zact.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR1,&zact,NULL);

  srcAddr = gsrcAddr;
  sinkAddr = gsinkAddr;
  
  myPthreadLock(&threadListLock);
  int i;
  for(i = 0; i < MAX_NUM_THREADS; i++){
    if( threadList[i] == (pthread_t)0 ){ 
      //Poke all live threads
      threadList[i] = pthread_self();
      threadId = i;
      break;
    }
  }
  myPthreadUnlock(&threadListLock);

  fprintf(stderr,"[WATCHER] Setting instruction watchpoints on %p and %p\n",srcAddr,sinkAddr);
  if( srcAddr != NULL ){
    drp_watch_inst((unsigned long)srcAddr,0);
  }

  if( sinkAddr != NULL ){
    drp_watch_inst((unsigned long)sinkAddr,1);
  }
  
}


