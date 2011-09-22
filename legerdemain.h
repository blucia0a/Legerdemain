/*To ensure RTLD_NEXT is defined*/
#ifndef __USE_GNU
#define __USE_GNU
#endif

/*For dynamic loading support*/
#include <dlfcn.h>//for RTLD_NEXT

/*For ucontext_t*/
#include <ucontext.h>

/*Define booleans...*/
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif

#define INIT_CTX 0x1

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
  fprintf(stderr,"Couldn't load function %s: %s\n",#f,dlerror()); \
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

/*Plugin constructor/destructor magic*/
#define LDM_PLUGIN_INIT __attribute__ ((constructor))
#define LDM_PLUGIN_DEINIT __attribute__ ((destructor))

/*Thread destructor magic*/
#define LDM_THD_DTR_DECL(k,f) pthread_key_t k;
#define LDM_REGISTER_THD_DTR(k,f) pthread_key_create(&k,f)
#define LDM_INSTALL_THD_DTR(k) pthread_setspecific(k,(void*)0x1)

/*Get the value of a register as a void pointer*/
#define LDM_GET_REG(ctx,reg) (((ucontext_t*)ctx)->uc_mcontext.gregs[reg]);
