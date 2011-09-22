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

static int active;
void *monitor(void *p){

  /*The sampling monitor thread*/

  ldmmsg(stderr,"[ConcurrentWatcher] Monitor is %lu\n",(unsigned long)pthread_self());

  /*Get the sampling rate*/
  char *pp = getenv("LDM_RATE");
  unsigned long r;
  if(pp != NULL){
    sscanf(pp,"%lu",&r);
  }else{
    r = 10000;
  }

  /*Get the sampling period -- TODO: cut this?*/
  pp = getenv("LDM_PERIOD");
  unsigned long per;
  if(pp != NULL){
    sscanf(pp,"%lu",&per);
  }else{
    per = r;
  }
   
  /*If the rate is NULL, then we will never poke threads*/ 
  if( !r ){
    return;
  }
 
  /*Main monitor loop -- sleep, then poke each thread*/
  while(1){

    //Sleep for the gov't mandated interval 
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
 
    /*Update the global sampling state, so new
 *    threads know what state to be in when they start
 *    */
    active = !active;

    myPthreadUnlock(&threadListLock);

  }
 
}

void handlePoke(int signum, siginfo_t *sinfo, void *ctx){

 /* The purpose of a Poke is to toggle a thread from 
 *  monitoring to not-monitoring.  That switch-over
 *  happens in here.  There could be user code executed
 *  at each poke, but in this case there is not
 *  */
  
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

  /*User trap handler.*/

  /*Get the instruction pointer*/
  void *ad = (void*)LDM_GET_REG(ctx,REG_RIP);
 
  /*Call out to X86Decode, disassemble the insn, get operands*/ 
  void *opAddrs[4];/*NOTE: 4 overprovisions, but this is a bug risk*/
  memset(opAddrs,0,4*sizeof(void*));
  getOperandAddresses(ad,(ucontext_t *)ctx,opAddrs);

  /*Which watchpoint fired?*/
  int which = drp_explain();

  if(which == 0){
 
    /*The source watchpoint fired.*/
    /*Update the last writer to be this thread*/
    myPthreadLock(&lastSrcLock);
    g_hash_table_insert(lastWriter, (gpointer)opAddrs[0], (gpointer)threadId);
    myPthreadUnlock(&lastSrcLock);

    srcFreq++;

  }else if(which == 1){


    /*The sink watchpoint fired.*/
    /*Get the last writer*/
    unsigned long last;
    myPthreadLock(&lastSrcLock);
    last = (unsigned long)g_hash_table_lookup(lastWriter,(gconstpointer)opAddrs[1]);
    myPthreadUnlock(&lastSrcLock);

    /*It was a communication to the sink from the source
      if the last writer was a different thread
    */
    if( last && (threadId != last) ){
      sinkComm++;
    }
    sinkFreq++;

  }

  /*NOTE: There is an atomicity issue -- between the release
          of the lock, and the execution of the insn, there is
          a delay, during which another thread could come along
          and change the last writer's value.
  */

  return;

}

void deinit_thread(void *d){

  /*Sampling monitor thread destructor stuff*/

  /*A thread is ending*/
  myPthreadLock(&threadListLock);

  /*Unwatch your watchpoints*/
  drp_unwatch(0);
  drp_unwatch(1);

  /*Remove yourself from the set of 
   *threads that will be poked
   */
  int i;
  for(i = 0; i < MAX_NUM_THREADS; i++){
    if( pthread_equal(pthread_self(),threadList[i]) ){ 

      threadList[i] = (pthread_t)0;

    }
  }
  myPthreadUnlock(&threadListLock);


  /*User thread destructor stuff*/

  /*Produce thread output*/
  fprintf(stderr,"%lu %lu %lu %lu (%f%%)\n",threadId,srcFreq,sinkFreq,sinkComm,100*((float)sinkComm/(float)sinkFreq));

}

void init_thread(void *targ, void*(*thdrtn)(void*)){

  /*Don't do initialization stuff for the monitor thread*/
  if(thdrtn == monitor){
    return; 
  }

  /*Set up the user initialization stuff*/ 
  
  /*Install the thread destructor function associated with dkey*/
  LDM_INSTALL_THD_DTR(dkey);

  /*Watchpoint registration*/
  fprintf(stderr,"[WATCHER] Setting instruction watchpoints on %p and %p\n",srcAddr,sinkAddr);
  srcAddr = gsrcAddr;
  sinkAddr = gsinkAddr;
  if( srcAddr != NULL ){
    drp_watch_inst((unsigned long)srcAddr,0);
  }

  if( sinkAddr != NULL ){
    drp_watch_inst((unsigned long)sinkAddr,1);
  }
 
  /*Set up the sampling monitor initialization stuff*/ 

  /*Register the poke handler*/
  struct sigaction zact;
  zact.sa_sigaction = handlePoke;
  zact.sa_flags = SA_SIGINFO;
  sigaction(SIGUSR1,&zact,NULL);

  /*Add this thread to the pokable list*/
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

  /*Sync this thread to the current sampling state*/
  if( active ){
    watchState = WATCHING;
  }else{
    watchState = WAITING;
  }
  myPthreadUnlock(&threadListLock);

  
}


static void LDM_PLUGIN_INIT init(){

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



