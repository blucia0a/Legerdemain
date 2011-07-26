#include <stdio.h>
#include <unistd.h>

#include "legerdemain.h"

LDM_ORIG_DECL(ssize_t,read,int,void*,size_t);
ssize_t read(int fildes, void *buf, size_t nbyte){
  fprintf(stderr,"Magic Read!\n");
  return LDM_ORIG(read)(fildes,buf,nbyte);  
}

void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
void init(){

  fprintf(stderr,"Calling Scaley_LDM intializer\n");
  LDM_REG(read);
  
}

void deinit(){

}
