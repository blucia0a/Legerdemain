#include <sys/types.h>//for size_t
#include <stdlib.h>//for getenv 
#include <stdio.h>//for fprintf
#include <unistd.h>//for read

#define __USE_GNU //for RTLD_NEXT -- goofy GNU stuff
#include <dlfcn.h>//for dlopen, dlsym, etc

#include "Applier.h"

#include "legerdemain.h"

void __attribute__ ((constructor)) LDM_init();
void __attribute__ ((destructor)) LDM_deinit();

void LDM_printVar(char *c, void *d){
  if( strstr(c,"libldm.so") ){
    return;
  }
  ldmmsg(stderr,"%s ",c);
}

void LDM_init(){

  ldmmsg(stderr,"Legerdemain - Brandon Lucia - 2011\n");
  ldmmsg(stderr,"Loading function wrappers via LD_PRELOAD:\n");
  char *preloaded = getenv("LD_PRELOAD");
  applyToTokens(preloaded,": ,",LDM_printVar,NULL);
  ldmmsg(stderr,"\n");
  
}

void LDM_deinit(){

}
