#include <malloc.h>
#include <stdio.h>

#include "legerdemain.h"

static int inited;
static int malloccount;

LDM_ORIG_DECL(void *,malloc,size_t);
void *malloc(size_t sz){
  malloccount++;
  if(!inited){
    LDM_REG(malloc);
  }
  return LDM_ORIG(malloc)(sz);
}

static void deinit(){
  ldmmsg(stderr,"[MALLOCWATCHER] Unloading fancy malloc plugin\n");
  ldmmsg(stderr,"[MALLOCWATCHER] Malloc was called %d times\n",malloccount);
}

void LDM_PLUGIN_INIT(){

  if(inited){return;}
  LDM_REG(malloc);
  ldmmsg(stderr,"[MALLOCWATCHER] Loading fancy malloc plugin\n");
  inited = 1;
  LDM_PLUGIN_DEINIT(deinit);

}

