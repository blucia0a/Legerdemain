/*Included System Headers*/
/*for size_t*/
#include <sys/types.h>

/*for getenv*/
#include <stdlib.h>

/*for fprintf*/
#include <stdio.h>

/*for readlink*/
#include <unistd.h>

/*for SIG_ constants, sigaction, etc*/
#include <signal.h>

/*for readline() support*/
#include <readline/readline.h>

/*Defined to ensure RTLD_NEXT is defined in dlfcn.h*/
#define __USE_GNU 

/*for dlopen, dlsym, etc*/
#include <dlfcn.h>

/*for get_info()*/
#include "addr2line.h"

/*for applyToTokens*/
#include "Applier.h"

/*for dwarf manipulation support*/
#include "dwarfclient.h"

/*for various project-specific macros and typedefs*/
#include "legerdemain.h"

/*
 * These will be used to preserve the program's 
 * original signal handlers when LDM replaces them
 */
struct sigaction sigABRTSaver;
struct sigaction sigSEGVSaver;
struct sigaction sigTERMSaver;
struct sigaction sigINTSaver;

/*LDM's constructor and destructor*/
void __attribute__ ((constructor)) LDM_init();
void __attribute__ ((destructor)) LDM_deinit();

/*The executable name*/
static char *LDM_ProgramName;

/*Records whether the program is running or not*/
static bool LDM_runstate;

/* 
 * Replacement version of pthread_create --
 * LDM has its own version that tracks additional
 * information about each threads' activities
 */
LDM_ORIG_DECL(int, pthread_create, pthread_t *, const pthread_attr_t *,
              void *(*)(void*), void *);
int pthread_create(pthread_t *thread,
              const pthread_attr_t *attr,
              void *(*start_routine)(void*), void *arg){

  return LDM_ORIG(pthread_create)(thread,attr,start_routine,arg);
}

/*
 * The following functions wrap the dwarf information
 * access support provided by libdwarfclient.  See
 * libdwarfclient for more information about what
 * each of these does.
 */

/*
 * Print the instruction pointer of the first instruction
 * on line 'line' of the file 'file'
 */
void LDM_line_to_addr(char *file, int line){
  void * a = (void*)get_iaddr_of_file_line(file,line);
  fprintf(stderr,"%p\n",a);
}

/*
 * Print the sequence of scopes and the variables
 * in each for the provided file and line.  The scopes
 * will be descended into, starting at the top-most
 * scope of the compilation unit that the file and line 
 * is part of
 */
void LDM_scope_f(char *file, int line){
  show_scopes_by_file_line(file,line);  
}

/*
 * Same functionality as LDM_scope_f(), except searches based
 * on an instruction address, instead of a file:line pair
 */
void LDM_scope_a(void *addr){
  show_scopes_by_addr(addr);  
}

/*
 * Gets the dwarf information for a variable 'varname',
 * given an instruction address in a scope in
 * which to search for a variable named 'varname'.
 */
void LDM_inspect(void *addr,const char *varname){
  show_info_for_scoped_variable(addr,varname);
  return;
}


/*Show line 'line' of file 'fname'*/
void LDM_showsource(char *fname, unsigned int line){

  int lines = 0;
  FILE *f = fopen(fname,"r");
  char buf[8192];
  while( fgets(buf, 8191, f) != NULL && ++lines < line);
  ldmmsg(stderr,"%s",buf);
  fclose(f);

}

/*
 * Print all libraries that were LD_PRELOAD-ed
 * before the program started
 */
void LDM_printlibs(char *c, void *d){

  if( strstr(c,"libldm.so") ){
    return;
  }
  ldmmsg(stderr,"%s ",c);

}

/*
 * Print a stack trace -- this
 * is intended to be used when
 * the program is stopped, during debugging
 */
void print_trace(){
  void *array[10];
  int size;
  char **strings;
  int i;

  size = backtrace (array, 10);
  strings = (char **)backtrace_symbols (array, size);
     
  for (i = 2; i < size; i++){

    fprintf (stderr,"  %s\n", strings[i]);

    /*TODO: get_info is currently broken, and I don't know why*/
    /*
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
    free( temp );*/
  }
     
  free (strings);

}

/*
 * Get the program's name, by looking at /proc/self/exe
 */
char *getProgramName(){
  char *buf = (char*)malloc(1024);
  memset(buf,0,1024);
  readlink("/proc/self/exe",buf,1024);
  return buf;
}

/*
 * The main debug command loop.
 */
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
      char varname[512];
      char insp[8];
      sscanf(line,"%s %lx %s\n",insp,&ad,varname);
      LDM_inspect((void*)ad,(const char *)varname);
      continue;
    }

    if(line && strstr(line,"dumpdwarf")){
      dump_dwarf_info(); 
      continue;
    }
    
    if(line && strstr(line,"ascope")){

        char sc_str[6];
        void *ad;
        int num = sscanf(line,"%s 0x%p\n",sc_str,&ad);
        if( num == 1 ){
          getReturnAddr(ad);  
        }
        fprintf(stderr,"str is %s, address is %p\n",sc_str, ad);
        LDM_scope_a(ad); 
        continue;
    }
    if(line && strstr(line,"fscope")){
        char sc_str[6];
        char fn[1024];
        unsigned ln;
        sscanf(line,"%s %s %u\n",sc_str,fn,&ln);
        fprintf(stderr,"%s %s %u\n",sc_str,fn,ln); 
        LDM_scope_f(fn,ln); 
        continue;
    }
    if(line && strstr(line,"linetoaddr")){
        char sc_str[6];
        char fn[1024];
        unsigned ln;
        sscanf(line,"%s %s %u\n",sc_str,fn,&ln);
        fprintf(stderr,"%s %s %u\n",sc_str,fn,ln); 
        LDM_line_to_addr(fn,ln); 
        continue;
    }
    if(line && strstr(line,"bt") || strstr(line,"backtrace") || strstr(line,"where")){
      print_trace();
      continue;
    }

    ldmmsg(stderr,"Unsupported Command %s!\n", line);

  }
  exit(1);  

}

/*
 * This function is called when any catchable signal is caught.
 * The first thing it does is go into the debugger loop.
 * When the user exits the debugger loop, the program continues
 * by executing the program's original signal handler, or the
 * default handler, if the program has registered no handler.
 */
void terminationHandler(int signum){

  ldmmsg(stderr,"Process %d got signal %d.  Entering LDM Debugger\n",getpid(),signum); 

  LDM_debug();

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

/*
 * Set up the signal handling support
 */
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

/*
 * The initializer implementation.
 * 1)Print welcome
 * 2)Show preloaded libraries
 * 3)Setup signal handling
 * 4)Register replacement pthread_create
 * 5)Get the program's name
 * 6)Enter the debugger loop
 */
void LDM_init(){

  ldmmsg(stderr,"Legerdemain - Brandon Lucia - 2011\n");
  ldmmsg(stderr,"Loading function wrappers via LD_PRELOAD:\n");

  char *preloaded = getenv("LD_PRELOAD");
  applyToTokens(preloaded,": ,",LDM_printlibs,NULL);
  ldmmsg(stderr,"\n");

  setupSignals();
  LDM_REG(pthread_create);

  LDM_ProgramName = getProgramName();
  opendwarf(LDM_ProgramName);

  LDM_debug();
  
}

/*
 * No tasks need to be performed on shutdown currently.
 */
void LDM_deinit(){

}
