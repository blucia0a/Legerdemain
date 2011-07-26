#include "Applier.h"
void applyToTokens(char *s, const char *delim, void(*process)(char *, void*), void *pArg){

  char *saveString;
  char *st = strtok_r(s,delim,&saveString);

  while(st != NULL){

    process(st,pArg);
    st = strtok_r(NULL,delim,&saveString);

  }   

}
