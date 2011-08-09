#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv){
  
    int x = 0;
    fprintf(stderr,"Addr of x is %p\n",&x);
    x = ((int *)0x16)[0];
    
    printf("%d\n",x);
    
}
