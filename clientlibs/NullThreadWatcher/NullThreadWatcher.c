#include <stdio.h>
#include <pthread.h>

#include "legerdemain.h"

/*Declare that we'll be using a thread destructor*/
LDM_THD_DTR_DECL(dkey,CW_deinit);

void deinit_thread(void *d){

  fprintf(stderr,"Done with thread %lu!\n",(unsigned long)pthread_self());

}

void init_thread(void *targ, void *(*thdrtn)(void*)){

  /*Install the thread destructor function associated with dkey*/
  LDM_INSTALL_THD_DTR(dkey);

}

static void LDM_PLUGIN_INIT init(){

  /*Register the thread destructor routine*/
  LDM_REGISTER_THD_DTR(dkey,deinit_thread);
  
}
