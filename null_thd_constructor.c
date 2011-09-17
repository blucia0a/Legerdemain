#include <stdio.h>
#include "legerdemain.h"

void init_thread(void *targ, void*(*thdrtn)(void*)){
  ldmmsg(stderr,"Running the No-Op Thread Constructor!\n");  
}
