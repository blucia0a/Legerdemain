#include <stdio.h>
#include <pthread.h>

#include "legerdemain.h"

/*Declare that we'll be using a thread destructor*/
LDM_THD_DTR_DECL(dkey,deinit_thread);

static void deinit_thread(void *d){

  fprintf(stderr,"Done with thread %lu!\n",(unsigned long)pthread_self());

}

void LDM_PLUGIN_THD_INIT(void *targ, void *(*thdrtn)(void*)){

  /*Install the thread destructor function associated with dkey*/
  LDM_INSTALL_THD_DTR(dkey);

}

void LDM_PLUGIN_INIT(){

  /*Register the thread destructor routine*/
  LDM_REGISTER_THD_DTR(dkey,deinit_thread);
  
}
