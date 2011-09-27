#include <stdio.h>
#include <pthread.h>

#include "legerdemain.h"

unsigned long numAllocatedBytes;
unsigned long numStdinBytes;

LDM_ORIG_DECL(void *,malloc,size_t);
void *malloc(size_t sz){
  if(LDM_ORIG(malloc) == NULL){
    LDM_REG(malloc);
  }
  numAllocatedBytes += sz;
  return LDM_ORIG(malloc)(sz);
}

LDM_ORIG_DECL(int,read,int,void *,size_t);
int read(int fd,void *buf,size_t sz){

  if( fd == 0 ){
    numStdinBytes += sz;
  }

  return LDM_ORIG(read)(fd,buf,sz);

}

static void deinit(){
  ldmmsg(stderr,"[Scaley] Streaming Input Consumption:Allocation Ratio: %f\n",(float)numStdinBytes/(float)numAllocatedBytes);
}

void LDM_PLUGIN_INIT(){

  LDM_REG(read);
  LDM_REG(malloc);
  LDM_PLUGIN_DEINIT(deinit);
  
}
