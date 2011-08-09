#include <stdio.h>
#include <assert.h>
#include "addr2line.h"

int main(int argc, char *argv){

  char *TheResult = get_info((unsigned long)0x4005be,"./scaleytest");
  assert(TheResult != 0);
  fprintf(stderr, "%s\n", TheResult);

}
