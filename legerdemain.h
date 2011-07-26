#define __USE_GNU
#include <dlfcn.h>//for RTLD_NEXT

//#define LDM_PREFIX __sc_
#define LDM_ORIG(f) __LDM ## f
#define LDM_ORIG_DECL(ret, f, args...) \
ret (* LDM_ORIG(f))(args)

#define LDM_REG(f, args...) \
dlerror(); \
LDM_ORIG(f) = dlsym(RTLD_NEXT, #f); \
if(LDM_ORIG(f) == NULL){ \
  fprintf(stderr,"Couldn't load function: %s\n",dlerror()); \
}

#define ldmmsg(...) fprintf(stderr,"[LDM] ");fprintf(__VA_ARGS__)
