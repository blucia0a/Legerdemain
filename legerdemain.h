#define __USE_GNU
#include <dlfcn.h>//for RTLD_NEXT
#include <ucontext.h>

#ifndef bool
#define bool int
#define true 1
#define false 0
#endif


/*Construct the name of the pointer to the original function*/
#define LDM_ORIG(f) __LDM ## f


/*Declare the pointer to the original function*/
#define LDM_ORIG_DECL(ret, f, args...) \
ret (* LDM_ORIG(f))(args)


/*Register a replacement version of function f*/
#define LDM_REG(f, args...) \
dlerror(); \
LDM_ORIG(f) = dlsym(RTLD_NEXT, #f); \
if(LDM_ORIG(f) == NULL){ \
  fprintf(stderr,"Couldn't load function: %s\n",dlerror()); \
}


/*Call the original version that was found by
 * LDM_REG
 */
#define LDM_CALL_ORIGINAL(f, args...) \
LDM_ORIG(f)(args);


/*Print a message that is prepended with [LDM] -- for error monitoring, etc */
#define ldmmsg(...) fprintf(stderr,"[LDM] ");fprintf(__VA_ARGS__)


/*Get the current return address -- 
 *faster than calling backtrace()*/
#define getReturnAddr(x) \
x = (void*)0xfeedf00d; \
ucontext_t ucontext; \
if( getcontext(&ucontext) != -1 ){ \
  x = ((void**)ucontext.uc_mcontext.gregs[REG_RBP])[1]; \
}
