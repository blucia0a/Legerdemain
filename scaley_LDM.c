#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include "legerdemain.h"

static unsigned long numInputReads;
static unsigned long numBytesAllocated;
static unsigned long numAllocations;

LDM_ORIG_DECL(int,getchar,void);
int getchar(void){
  numInputReads++;
  return LDM_ORIG(getchar)();
}

LDM_ORIG_DECL(int,scanf,const char *format,...);
int scanf(const char *format,...){
  
  numInputReads++;
  va_list argp;
  va_start(argp, format);
  int rval = vscanf(format,argp);
  va_end(argp);
  return rval;

}

LDM_ORIG_DECL(void*,malloc,size_t);
void *malloc(size_t size){

  numBytesAllocated += size;
  numAllocations++;
  return LDM_ORIG(malloc)(size);

}

void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
void init(){

  fprintf(stderr,"Calling Scaley_LDM intializer\n");
  LDM_REG(getchar);
  LDM_REG(scanf); 
  LDM_REG(malloc); 
  
}

void deinit(){

  fprintf(stderr,"stdin Reads: %d. allocations: %d. bytes allocated: %d. %f bytes allocated per stream read\n",numInputReads,numAllocations,numBytesAllocated,(float)numBytesAllocated/(float)numInputReads);

}
