/*Included System Headers*/
/*for backtrace_symbols*/
#include <execinfo.h>

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

static void *curDebugInstr;
static void *curDebugFramePtr;


/*LDM's constructor and destructor*/
void __attribute__ ((constructor)) LDM_init();
void __attribute__ ((destructor)) LDM_deinit();

/*The executable name*/
static char *LDM_ProgramName;

/*Records whether the program is running or not*/
static bool LDM_runstate;

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

long LDM_locate_var(void *addr,char *var){
  long loc = get_location_of_scoped_variable(addr,var);
  return loc;
}

DC_type *LDM_gettype_var(void *addr,char *var){
  return get_type_of_scoped_variable(addr,var);
}

void LDM_vars_a(void *addr){

  show_vars_by_scope_addr(addr);

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
  //show_info_for_scoped_variable(addr,varname);
  //fprintf(stderr,"done showing the info\n");
  long loc = LDM_locate_var(addr,(char *)varname);
  DC_type *t = LDM_gettype_var(addr,(char*)varname);
  if( loc < 0 ){
    loc += 16;
  }
  fprintf(stderr,"%s ",t->name);
  int i;
  for(i = 0; i < t->indirectionLevel; i++){
    fprintf(stderr,"*");
  }
  fprintf(stderr,"%s",varname);
  for(i = 0; i < t->arrayLevel; i++){
    fprintf(stderr,"[]");
  }
  fprintf(stderr," = ");
  for(i = 1; i <= t->byteSize; i++){
    fprintf(stderr,"%hhx",*((unsigned char*)(curDebugFramePtr + loc + (t->byteSize-i))));
  }
  fprintf(stderr,"\n");
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
  void * array[10];
  int size;
  char **strings;
  int i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
     
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

void print_help(){
  fprintf(stderr,"\
help - this message\n \
quit - exit the program\n \
cont - continue executing.  Leaves the LDM debugger.\n \
run  - run the program.\n \
source - show <file> <line>. Shows the source code at <line> in <file>.\n \
inspect - shows the value of a memory location [currently unsafe!].\n \
dumpdwarf - shows all debug info in the exeuctable.\n \
ascope - ascope <instruction address>.  Prints a chain of scopes leading down to the innermost one enclosing address <instruction address>.\n \
fscope - fscope <file> <line>. Prints a chain of scopes leading down to the innermost one enclosing the first instruction of the code on line <line> in <file>\n \
linetoaddr - linetoaddr <file> <line>.  Prints the instruction address of the first instruction of he code on line <line> in <file>.\n \
bt/backtrace/where - Prints the call stack.\n \
\n");
}

/*
 * The main debug command loop.
 */
void LDM_debug(ucontext_t *ctx){

  int curDebugFrame = 0;
  if(ctx == NULL){
    getcontext(ctx);
  }

  print_trace();

  if(ctx != (ucontext_t*)INIT_CTX){

    fprintf(stderr,"Instruction: %p\n",ctx->uc_mcontext.gregs[REG_RIP]);
    fprintf(stderr,"Frame Pointer: %p\n",ctx->uc_mcontext.gregs[REG_RBP]);
    fprintf(stderr,"Stack Pointer: %p\n",ctx->uc_mcontext.gregs[REG_RSP]);

    curDebugInstr = (void*)ctx->uc_mcontext.gregs[REG_RIP];
    curDebugFramePtr = (void*)ctx->uc_mcontext.gregs[REG_RBP];

  }else{
    curDebugInstr = (void*)0x0;
    curDebugFramePtr = (void*)0x0;
  }

  while(1){

    char *line = readline("LDM>");
  
     
 
    /*Decide what to do based on the command!*/
    if(line && !strncmp(line,"",1)){
      continue;
    }
    
    add_history(line);
    
    if(line && strstr(line,"help")){
      print_help();
    }

    if(line && strstr(line,"fptr")){
      if(ctx && ctx != (ucontext_t*)INIT_CTX){
        fprintf(stderr,"%lx\n",ctx->uc_mcontext.gregs[REG_RBP]);
      }else{
        fprintf(stderr,"No Frame Pointer Available\n");
      }
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

    if(line && strstr(line,"source")){
      char fname[2048];
      char show[5];
      int lineno;
      sscanf(line,"%s %s %d\n",show,fname,&lineno);
      LDM_showsource(fname,lineno);
      continue;
    }
    
    if(line && strstr(line,"deref")){
      unsigned long ad;
      char insp[8];
      int num = sscanf(line,"%s %lx\n",insp,&ad);
      fprintf(stderr,"%lx\n",*((unsigned long *)ad));
      continue;
    }

    if(line && strstr(line,"showvars")){
      LDM_vars_a(curDebugInstr); 
      continue;
    }

    if(line && strstr(line,"up")){
      
      if(ctx && ctx != (ucontext_t*)INIT_CTX){

        int i;

        /*TODO: Bounds check this somehow*/
        curDebugFrame++;

        curDebugFramePtr = (void*)ctx->uc_mcontext.gregs[REG_RBP];
        for(i = 0; i < curDebugFrame; i++){
          fprintf(stderr,"Next Frame Pointer: %p.\n",curDebugFramePtr);
          
          curDebugFramePtr = ((void**)curDebugFramePtr)[0];
        }

        curDebugInstr = ((void**)curDebugFramePtr)[1];

        fprintf(stderr,"Frame Pointer: %p.  Instruction: %p\n",curDebugFramePtr,curDebugInstr);

      }else{

        fprintf(stderr,"No Frame Pointer Available\n");

      }
      
      continue; 

    }
    
    if(line && strstr(line,"down")){

      if(ctx && ctx != (ucontext_t*)INIT_CTX){

        int i;
        if(curDebugFrame > 0){
          curDebugFrame--;
        }

        curDebugFramePtr = (void*)ctx->uc_mcontext.gregs[REG_RBP];
        for(i = 0; i < curDebugFrame; i++){
          curDebugFramePtr = ((void**)curDebugFramePtr)[0];
        }

        if( curDebugFrame > 0 ){
          curDebugInstr = ((void**)curDebugFramePtr)[1];
        }else{
          curDebugInstr = (void*)ctx->uc_mcontext.gregs[REG_RIP];
        }
        fprintf(stderr,"Frame Pointer: %p.  Instruction: %p\n",curDebugFramePtr,curDebugInstr);

      }else{

        fprintf(stderr,"No Frame Pointer Available\n");

      }
      
      continue; 

    }
    
    if(line && strstr(line,"string")){
      unsigned long ad;
      char insp[8];
      sscanf(line,"%s %lx\n",insp,&ad);

      int len = 0;
      char *c = (char *)ad;
      while(*c++ != 0){len++; }

      char str[len+1];
      strncpy(str,c,len);

      fprintf(stderr,"%s\n",str);
      continue;
    }

    if(line && strstr(line,"inspect")){
      unsigned long ad;
      char varname[512];
      char insp[8];
      int num = sscanf(line,"%s %s %lx\n",insp,varname,&ad);
      if(num == 2){
        ad = (unsigned long)curDebugInstr;
      }
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
          LDM_scope_a(curDebugInstr);
        }
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
void terminationHandler(int signum, siginfo_t *sinf, void *uctx){


  ucontext_t *ctx = (ucontext_t *)uctx;
  ldmmsg(stderr,"Process %d got signal %d.  Entering LDM Debugger\n",getpid(),signum); 

  LDM_debug(ctx);

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
  segv_sa.sa_sigaction = terminationHandler;
  sigemptyset(&segv_sa.sa_mask);
  segv_sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
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


  char *dbg = getenv("LDM_DEBUG");

  LDM_ProgramName = getProgramName();
  opendwarf(LDM_ProgramName);

  if( dbg != NULL ){
    LDM_debug((ucontext_t*)INIT_CTX);
  }
  
}

/*
 * No tasks need to be performed on shutdown currently.
 */
void LDM_deinit(){

}
