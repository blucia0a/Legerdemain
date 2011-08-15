#include <sys/types.h>//for size_t
#include <stdlib.h>//for getenv 
#include <stdio.h>//for fprintf
#include <unistd.h>//for read
#include <signal.h>//for SIG_*, sigaction, etc
#include <readline/readline.h>

#define __USE_GNU //for RTLD_NEXT -- goofy GNU stuff
#include <dlfcn.h>//for dlopen, dlsym, etc

#include "addr2line.h"

#include "Applier.h"

#include "dwarfclient.h"

#include "legerdemain.h"

#define MAX_THREADS 512
pthread_t allThreads[MAX_THREADS];

struct sigaction sigABRTSaver;
struct sigaction sigSEGVSaver;
struct sigaction sigTERMSaver;
struct sigaction sigINTSaver;

void __attribute__ ((constructor)) LDM_init();
void __attribute__ ((destructor)) LDM_deinit();

static bool LDM_runstate;

LDM_ORIG_DECL(int, pthread_create, pthread_t *, const pthread_attr_t *,
              void *(*)(void*), void *);
int pthread_create(pthread_t *thread,
              const pthread_attr_t *attr,
              void *(*start_routine)(void*), void *arg){

  return LDM_ORIG(pthread_create)(thread,attr,start_routine,arg);
}

void LDM_inspect(void *addr){

  Dl_info d;
  memset(&d,0,sizeof(d));
  dladdr(addr, &d);
  fprintf(stderr,"%lx = %lu [%s, defined in %s]",addr,(unsigned long)*((unsigned long *)addr),d.dli_sname, d.dli_fname);
  return;

}

void LDM_showsource(char *fname, unsigned int line){

  int lines = 0;
  FILE *f = fopen(fname,"r");
  char buf[8192];
  while( fgets(buf, 8191, f) != NULL && ++lines < line);
  ldmmsg(stderr,"%s",buf);
  fclose(f);

}

void LDM_printlibs(char *c, void *d){

  if( strstr(c,"libldm.so") ){
    return;
  }
  ldmmsg(stderr,"%s ",c);

}

void print_trace(){
  void *array[10];
  int size;
  char **strings;
  int i;

  size = backtrace (array, 10);
  strings = (char **)backtrace_symbols (array, size);
     
  for (i = 2; i < size; i++){

    fprintf (stderr,"  %s  ", strings[i]);
    if( strstr(strings[i],"/libc.") ){
      fprintf(stderr,"\n");
      continue;
    }
    int len = strlen(strings[i]);
    char *temp = (char *)malloc(len);
    strncpy(temp,strings[i],len);
    char *c;
    if( (c = strrchr(temp,'(')) ){
      while( *c ){
        *c=0; 
        c++;
      }
      fprintf(stderr," %s",get_info(array[i],temp));
    }else if( (c = strrchr(temp,' ')) ){
      while( *c ){
        *c=0;
        c++;
      } 
      fprintf(stderr," %s",get_info(array[i],temp));
    }
    fprintf(stderr,"\n");
    free( temp );
  }
     
  free (strings);

}



void LDM_debug(){

  print_trace();
  while(1){

    char *line = readline("LDM>");
    
    /*Decide what to do based on the command!*/
    if(line && !strncmp(line,"",1)){
      continue;
    }

    if(line && strstr(line,"quit")){
      exit(0);
    }
    
    if(line && strstr(line,"cont")){
      if( LDM_runstate == true ){
        ldmmsg(stderr,"Continuing.\n");
        return;
      }else{
        ldmmsg(stderr,"Can't continue - Not Running!\n");
      }
    }
    
    if(line && strstr(line,"run")){
      if( LDM_runstate == false ){
        return;
      }else{
        ldmmsg(stderr,"Can't run - Already running!\n");
      }
    }

    if(line && strstr(line,"show")){
      char fname[2048];
      char show[5];
      int lineno;
      sscanf(line,"%s %s %d\n",show,fname,&lineno);
      LDM_showsource(fname,lineno);
      continue;
    }

    if(line && strstr(line,"inspect")){
      unsigned long ad;
      char insp[8];
      sscanf(line,"%s %lx\n",insp,&ad);
      LDM_inspect((void*)ad);
      continue;
    }

    if(line && strstr(line,"dumpdwarf")){
      char buf[1024];
      memset(buf,0,1024);
      fprintf(stderr,"Getting executable name %s\n");
      readlink("/proc/self/exe",buf,1024);
      fprintf(stderr,"The filename is %s\n",buf);
      getdwarfdata(buf); 
    }

    ldmmsg(stderr,"Unsupported Command %s!\n", line);

  }
  exit(1);  

}


void terminationHandler(int signum){

  ldmmsg(stderr,"Process %d got signal %d.  Entering LDM Debugger\n",getpid(),signum); 

  LDM_debug();

  /*The following code calls the program's signal handlers after returning from the debugger*/
  /*I don't know what the right behavior is for this case*/
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
  segv_sa.sa_flags = SA_ONSTACK;
  sigaction(SIGSEGV,&segv_sa,&sigSEGVSaver);
  sigaction(SIGTERM,&segv_sa,&sigTERMSaver);
  sigaction(SIGABRT,&segv_sa,&sigABRTSaver);
  sigaction(SIGINT,&segv_sa,&sigINTSaver);
  
}

void LDM_init(){

  ldmmsg(stderr,"Legerdemain - Brandon Lucia - 2011\n");
  ldmmsg(stderr,"Loading function wrappers via LD_PRELOAD:\n");

  char *preloaded = getenv("LD_PRELOAD");
  applyToTokens(preloaded,": ,",LDM_printlibs,NULL);
  ldmmsg(stderr,"\n");

  setupSignals();
  LDM_REG(pthread_create);

  LDM_debug();
  
}

void LDM_deinit(){

}
