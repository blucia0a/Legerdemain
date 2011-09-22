#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>

#include "sampled_thread_monitor.h"
#include "legerdemain.h"

static pthread_t monitorThread;
static pthread_mutex_t threadListLock;
static pthread_t threadList[MAX_NUM_THREADS];
__thread SamplingState samplingState; 
__thread unsigned long threadId;

/*This gets called in handlePoke, and implements the user's poke handling*/
void (*clientPokeHandler)(SamplingState s);

int (*myPthreadCreate)(pthread_t *thread,
              const pthread_attr_t *attr,
              void *(*start_routine)(void*), void *arg);
int (*myPthreadLock)(pthread_mutex_t *mutex);
int (*myPthreadUnlock)(pthread_mutex_t *mutex);


static int active;
static void *monitor(void *p){

  /*The sampling monitor thread*/

  ldmmsg(stderr,"[SAMPLER] Monitor is %lu\n",(unsigned long)pthread_self());

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

static void handlePoke(int signum, siginfo_t *sinfo, void *ctx){

 /* The purpose of a Poke is to toggle a thread from 
 *  monitoring to not-monitoring.  That switch-over
 *  happens in here.  There could be user code executed
 *  at each poke, but in this case there is not
 *  */
  
  if( samplingState == SAMPLE_OFF ){

    if(clientPokeHandler){
      clientPokeHandler(SAMPLE_ON);
    }
    samplingState = SAMPLE_ON;

  }else{

    if(clientPokeHandler){
      clientPokeHandler(SAMPLE_OFF);
    }
    samplingState = SAMPLE_OFF;

  }
 
}

void sampled_thread_monitor_deinit(void *d){

  /*Sampling monitor thread destructor stuff*/

  /*A thread is ending*/
  myPthreadLock(&threadListLock);

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

}

void sampled_thread_monitor_thread_init(void *targ, void*(*thdrtn)(void*)){

  /*Don't do initialization stuff for the monitor thread*/
  if(thdrtn == monitor){
    return; 
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
    samplingState = SAMPLE_ON;
  }else{
    samplingState = SAMPLE_OFF;
  }
  
  myPthreadUnlock(&threadListLock);

}

void sampled_thread_monitor_init(void *data,void (*ph)(SamplingState)){

  clientPokeHandler = ph;
  void *h = NULL;

  int i; 
  for(i = 0; i < MAX_NUM_THREADS; i++){
    threadList[i] = (pthread_t)0;
  }

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
      fprintf(stderr,"Couldn't get pthread_lock\n");
    }
    if(myPthreadUnlock == NULL){
      fprintf(stderr,"Couldn't get pthread_unlock\n");
    }
  }

  pthread_mutex_init(&threadListLock,NULL);
  myPthreadCreate(&monitorThread,NULL,monitor,data);

}
