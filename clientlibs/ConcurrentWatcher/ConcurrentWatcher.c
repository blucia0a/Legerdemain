#ifndef __USE_GNU
#define __USE_GNU
#define _GNU_SOURCE
#include <ucontext.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

#include <pthread.h>
#include <glib.h>

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

GHashTable *lastWriter;

__thread void *srcAddr;
__thread void *sinkAddr;
__thread unsigned long srcFreq;
__thread unsigned long sinkFreq;
__thread unsigned long sinkComm;
__thread unsigned long threadId;

pthread_t lastSrc;
void *lastD1;
void *lastD2;
pthread_mutex_t lastSrcLock;

typedef enum _CW_WatchState{
  WATCHING = 0,
  WAITING
} CW_WatchState;
__thread CW_WatchState watchState; 

int active;
void *monitor(void *p){

  ldmmsg(stderr,"[ConcurrentWatcher] Monitor is %lu\n",(unsigned long)pthread_self());
  char *pp = getenv("LDM_RATE");
  unsigned long r;
  if(pp != NULL){
    sscanf(pp,"%lu",&r);
  }else{
    r = 10000;
  }

  pp = getenv("LDM_PERIOD");
  unsigned long per;
  if(pp != NULL){
    sscanf(pp,"%lu",&per);
  }else{
    per = r;
  }
  
  while(1){

    //sleep a hundredth of a second
    if( !r ){
      return;
    }
    if(active){
      usleep(rand() % r);
    }else{
      usleep(rand() % per);
    }

    myPthreadLock(&threadListLock);
    int i;
    for(i = 0; i < MAX_NUM_THREADS; i++){
      if( threadList[i] != (pthread_t)0 ){ 

        //Poke all live threads
        pthread_kill(threadList[i],SIGUSR1);
      }
    }
    active = !active;
    
    
    myPthreadUnlock(&threadListLock);


  }
 
}

void handlePoke(int signum, siginfo_t *sinfo, void *ctx){

  if( watchState == WAITING ){

    if( srcAddr != NULL ){

      drp_watch_inst((unsigned long)srcAddr,0);

    }
  
    if( sinkAddr != NULL ){

      drp_watch_inst((unsigned long)sinkAddr,1);

    }
    watchState = WATCHING;

  }else{

    if( srcAddr != NULL ){
      drp_unwatch(0); 
    }

    if( sinkAddr != NULL ){
      drp_unwatch(1); 
    }
    watchState = WAITING;

  }
 
}

void handleTrap(int signum, siginfo_t *sinfo, void *ctx){

  void *ad = (void*)(((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP]);
  void *ptr = 0;
  void *opAddrs[4];/*BUG: Can't be more than 4....*/

  memset(opAddrs,0,4*sizeof(void*));
  getOperandAddresses(ad,(ucontext_t *)ctx,opAddrs);

  int which = drp_explain();
  if(which == 0){

    myPthreadLock(&lastSrcLock);
    g_hash_table_insert(lastWriter, (gpointer)opAddrs[0], (gpointer)threadId);
    myPthreadUnlock(&lastSrcLock);
    srcFreq++;

  }else if(which == 1){


    pthread_t ls;
    void *ld1;
    void *ld2;

    unsigned long last;

    myPthreadLock(&lastSrcLock);
    last = (unsigned long)g_hash_table_lookup(lastWriter,(gconstpointer)opAddrs[1]);
    myPthreadUnlock(&lastSrcLock);

    if( last && (threadId != last) ){
      sinkComm++;
    }
    sinkFreq++;

  }

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

  fprintf(stderr,"%lu %lu %lu %lu (%f%%)\n",threadId,srcFreq,sinkFreq,sinkComm,100*((float)sinkComm/(float)sinkFreq));

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

  if( active ){
    watchState = WATCHING;
  }else{
    watchState = WAITING;
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


void LDM_PLUGIN_INIT watch_addr_thd_ctr_init(){

  setupX86Decoder(); 
  lastWriter = g_hash_table_new(g_direct_hash,g_direct_equal);

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
  pthread_mutex_init(&lastSrcLock,NULL);

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

  init_thread(0x0,(void*(*)(void*))0x1);

}



