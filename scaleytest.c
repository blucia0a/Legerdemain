#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void foo(int x, int y, char z, long q){
  int s = 0xbeefbeef;
  int t = 0xabababab; 
  fprintf(stderr,"s=%p t=%p x=%p, y=%p, z=%p, q=%p\n",&s, &t, &x, &y, &z, &q);  
  *((int*)0x0) = 0x19931993;

}

int main(int argc, char **argv){

    int x = 0xfeedf00d;
    fprintf(stderr,"argc=%p argv=%p x=%p\n",&argc, &argv, &x);  

    foo(0xaaaa,0xbbbb,'Z',0xabcdabcdabcdabc);
    while( scanf("%d",&x) ){
      int *vx = (int*)malloc(sizeof(int)); 
      *vx = x;
    }
    
    printf("%d\n",x);
    
}

