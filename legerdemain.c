#include <sys/types.h>//for size_t
#include <stdlib.h>//for getenv 
#include <stdio.h>//for fprintf
#include <unistd.h>//for read
#include <signal.h>//for SIG_*, sigaction, etc
#include <readline/readline.h>

#define __USE_GNU //for RTLD_NEXT -- goofy GNU stuff
#include <dlfcn.h>//for dlopen, dlsym, etc

#include "Applier.h"

#include "legerdemain.h"

struct sigaction sigABRTSaver;
struct sigaction sigSEGVSaver;
struct sigaction sigTERMSaver;
struct sigaction sigINTSaver;
    

void __attribute__ ((constructor)) LDM_init();
void __attribute__ ((destructor)) LDM_deinit();

void LDM_printVar(char *c, void *d){

  if( strstr(c,"libldm.so") ){
    return;
  }
  ldmmsg(stderr,"%s ",c);
  

}

void LDM_debug(){

  while(1){
    /*TODO: apparently if read() is replaced, readline goes haywire*/
    char *line = readline("[LDM]>");  
    if(line && strstr(line,"quit")){
      exit(0);
    }
    fprintf(stderr,"Unsupported Command!\n");
  }
  exit(1);  

}


void terminationHandler(int signum){

  ldmmsg(stderr,"Process %d died with signal %d.  Entering LDM Debugger\n",getpid(),signum); 

  /*The following code calls the program's signal handlers before entering the debugger*/
  if( signum == SIGSEGV){

    if( sigSEGVSaver.sa_handler != SIG_DFL &&
        sigSEGVSaver.sa_handler != SIG_IGN){
      (*sigSEGVSaver.sa_handler)(signum);  
    }
        

  }else if( signum == SIGTERM){
    
    if( sigTERMSaver.sa_handler != SIG_DFL &&
        sigTERMSaver.sa_handler != SIG_IGN){
      (*sigTERMSaver.sa_handler)(signum);  
    }

  }else if( signum == SIGABRT){
    if( sigABRTSaver.sa_handler != SIG_DFL &&
        sigABRTSaver.sa_handler != SIG_IGN){
      (*sigABRTSaver.sa_handler)(signum);  
    }
  }else if ( signum == SIGINT ){
    if( sigINTSaver.sa_handler != SIG_DFL &&
        sigINTSaver.sa_handler != SIG_IGN){
      (*sigINTSaver.sa_handler)(signum);  
    }
  }
 
  LDM_debug();
 
  signal(signum, SIG_DFL);
  kill(getpid(), signum);

}

void setupSignals(){

  memset(&sigABRTSaver, 0, sizeof(struct sigaction));
  memset(&sigSEGVSaver, 0, sizeof(struct sigaction));
  memset(&sigTERMSaver, 0, sizeof(struct sigaction));
  memset(&sigINTSaver, 0, sizeof(struct sigaction));
      
  /*Register Signal handler for SEGV and TERM*/
  struct sigaction segv_sa;
  segv_sa.sa_handler = terminationHandler;
  sigemptyset(&segv_sa.sa_mask);
  segv_sa.sa_flags = SA_RESTART;
  sigaction(SIGSEGV,&segv_sa,&sigSEGVSaver);
  sigaction(SIGTERM,&segv_sa,&sigTERMSaver);
  sigaction(SIGABRT,&segv_sa,&sigABRTSaver);
  sigaction(SIGINT,&segv_sa,&sigINTSaver);
  
}

void LDM_init(){

  ldmmsg(stderr,"Legerdemain - Brandon Lucia - 2011\n");
  ldmmsg(stderr,"Loading function wrappers via LD_PRELOAD:\n");

  char *preloaded = getenv("LD_PRELOAD");
  applyToTokens(preloaded,": ,",LDM_printVar,NULL);
  ldmmsg(stderr,"\n");

  setupSignals();
  
}

void LDM_deinit(){

}
