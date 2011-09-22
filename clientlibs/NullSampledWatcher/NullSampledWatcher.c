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

#include "legerdemain.h"
#include "sampled_thread_monitor.h"

#define MAX_NUM_THREADS 512

/*Declare that we'll be using a thread destructor*/
LDM_THD_DTR_DECL(dkey,CW_deinit);

void getPoked(SamplingState s,ucontext_t *ctx){

  if( s == SAMPLE_ON){

    /*Report the RIP each time you enter a sampling period*/
    void *ad = (void*)LDM_GET_REG(ctx,REG_RIP);
    fprintf(stderr,"Zapped at %p!\n",ad);

  }else{

    /*Could also do something as you leave a sampling 
 *    period as well*/

  }

}

void deinit_thread(void *d){

  sampled_thread_monitor_deinit(d);
  fprintf(stderr,"Done with all that sampling in thread %lu!\n",(unsigned long)pthread_self());

}

void init_thread(void *targ, void *(*thdrtn)(void*)){

  /*Install the thread destructor function associated with dkey*/
  LDM_INSTALL_THD_DTR(dkey);

  /*register this thread with the sampling framework*/
  sampled_thread_monitor_thread_init(targ,thdrtn);

}

static void LDM_PLUGIN_INIT init(){

  /*Register the thread destructor routine*/
  LDM_REGISTER_THD_DTR(dkey,deinit_thread);

  void *d = NULL;
  /*initialize the sampling framework*/
  sampled_thread_monitor_init(d,getPoked);
  
}
