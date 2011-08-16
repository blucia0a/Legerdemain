#include <stdio.h>
#include <unistd.h>

#include "legerdemain.h"


LDM_ORIG_DECL(int,getchar,void);
int getchar(void){
  fprintf(stderr,"Magic getchar!\n");
  return LDM_ORIG(getchar)();
}

void __attribute__ ((constructor))init();
void __attribute__ ((destructor)) deinit();
void init(){

  fprintf(stderr,"Calling Scaley_LDM intializer\n");
  LDM_REG(getchar);
  
}

void deinit(){

}
