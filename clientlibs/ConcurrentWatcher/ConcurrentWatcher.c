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
#include "sampled_thread_monitor.h"

#define MAX_NUM_THREADS 512

/*Declare that we'll be using a thread destructor*/
LDM_THD_DTR_DECL(dkey,deinit_thread);

/*Data related to communication monitoring*/
/*Global version of src and sink addresses*/
void *gsrcAddr;
void *gsinkAddr;

/*Thread-local version of src and sink addresses*/
__thread void *srcAddr;
__thread void *sinkAddr;

/*Hash table mapping a memory location to the pthread_t that last wrote it */
pthread_mutex_t lastWriterLock;
GHashTable *lastWriter;

/*Per-Thread counters of src and sink freq, 
 *and the count of communicating sink execs.
*/
__thread unsigned long srcFreq;
__thread unsigned long sinkFreq;
__thread unsigned long sinkComm;


void getPoked(SamplingState s,ucontext_t *ctx){

  if( s == SAMPLE_ON){

    if( srcAddr != NULL ){

      drp_watch_inst((unsigned long)srcAddr,0);

    }
  
    if( sinkAddr != NULL ){

      drp_watch_inst((unsigned long)sinkAddr,1);

    }

  }else{
    
    if( srcAddr != NULL ){

      drp_unwatch(0); 

    }

    if( sinkAddr != NULL ){

      drp_unwatch(1); 

    }

  }

}


/*This function gets called whenever a watchpoint fires*/
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
    pthread_mutex_lock(&lastWriterLock);
    g_hash_table_insert(lastWriter, (gpointer)opAddrs[0], (gpointer)pthread_self());
    pthread_mutex_unlock(&lastWriterLock);

    srcFreq++;

  }else if(which == 1){


    /*The sink watchpoint fired.*/
    /*Get the last writer*/
    unsigned long last;
    pthread_mutex_lock(&lastWriterLock);
    last = (unsigned long)g_hash_table_lookup(lastWriter,(gconstpointer)opAddrs[1]);
    pthread_mutex_unlock(&lastWriterLock);

    /*It was a communication to the sink from the source
      if the last writer was a different thread
    */
    if( last && !pthread_equal(pthread_self(),(pthread_t)last) ){
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

static void deinit_thread(void *d){

  sampled_thread_monitor_deinit(d);
  
  /*Unwatch your watchpoints*/
  drp_unwatch(0);
  drp_unwatch(1);
  
  /*Produce thread output*/
  fprintf(stderr,"%lu %lu %lu %lu (%f%%)\n",pthread_self(),srcFreq,sinkFreq,sinkComm,100*((float)sinkComm/(float)sinkFreq));

}

void init_thread(void *targ, void *(*thdrtn)(void*)){

  /*Set up the user initialization stuff*/ 
  
  /*Install the thread destructor function associated with dkey*/
  LDM_INSTALL_THD_DTR(dkey);

  /*Watchpoint registration*/
  srcAddr = gsrcAddr;
  sinkAddr = gsinkAddr;
  fprintf(stderr,"[WATCHER] Setting instruction watchpoints on %p and %p\n",srcAddr,sinkAddr);
  if( srcAddr != NULL ){
    drp_watch_inst((unsigned long)srcAddr,0);
  }

  if( sinkAddr != NULL ){
    drp_watch_inst((unsigned long)sinkAddr,1);
  }
 
  sampled_thread_monitor_thread_init(targ,thdrtn);

}

static void LDM_PLUGIN_INIT init(){

  /*Register the thread destructor routine*/
  LDM_REGISTER_THD_DTR(dkey,deinit_thread);
 
  /*set up the X86 Decoding support code*/ 
  setupX86Decoder(); 

  /*Create the map from address to last-writer-thread*/
  lastWriter = g_hash_table_new(g_direct_hash,g_direct_equal);
  pthread_mutex_init(&lastWriterLock,NULL);

  /*Set up watchpoint stuff, and get addresses to watch from the environment*/
  drp_init();

  char *srAd = getenv("LDM_SRC");
  char *siAd = getenv("LDM_SINK");

  if(srAd != NULL){
    sscanf(srAd,"%lx",&gsrcAddr);
  }

  if(siAd != NULL){
    sscanf(siAd,"%lx",&gsinkAddr);
  }

  /*Register a callback to run when a watchpoint gets whacked*/
  struct sigaction sact;
  sact.sa_sigaction = handleTrap;
  sact.sa_flags = SA_SIGINFO;
  sigaction(SIGTRAP,&sact,NULL);

  void *d = NULL;
  sampled_thread_monitor_init(d,getPoked);
  
  
  /*Init this thread, so it dumps its data when it finishes*/
  //init_thread(0x0,(void*(*)(void*))0x1);

}
