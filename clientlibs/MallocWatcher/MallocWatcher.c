#include <malloc.h>
#include <stdio.h>

#include "legerdemain.h"

static int malloccount;
LDM_ORIG_DECL(void *,malloc,size_t);
void *malloc(size_t sz){
  malloccount++;
  if( LDM_ORIG(malloc) == NULL ){
    fprintf(stderr,"[MALLOCWATCHER] Forced To register in replacement malloc\n");
    LDM_REG(malloc);
  }
  return LDM_ORIG(malloc)(sz);
}

static void __attribute__ ((constructor)) init();
static void init(){
  LDM_REG(malloc);
  ldmmsg(stderr,"[MALLOCWATCHER] Loading fancy malloc plugin\n");
}

static void __attribute__ ((constructor)) deinit();
static void deinit(){
  ldmmsg(stderr,"[MALLOCWATCHER] Unloading fancy malloc plugin\n");
  ldmmsg(stderr,"[MALLOCWATCHER] Malloc was called %d times\n",malloccount);
}
