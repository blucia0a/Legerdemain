#ifndef _APPLY_TO_TOKENS_
#define _APPLY_TO_TOKENS_
#include <string.h>
void applyToTokens(char *s, const char *delim, void(*process)(char *, void*), void *pArg);
#endif
