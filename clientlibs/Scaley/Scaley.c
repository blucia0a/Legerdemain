#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "legerdemain.h"

#include "glib.h"

long numAllocatedBytes;
long numFreedBytes;
long numStdinBytes;

long bytesSinceLastRead;
long lastReadBytes;

#define S_LEN 10
float allocPerStreamHisto[S_LEN];

int inited = 0;

GHashTable *blockSizes;

LDM_ORIG_DECL(void,free,void *ptr);
void free(void *ptr){
  
  if(LDM_ORIG(free) == NULL){
    /*Bug: Leak these bytes right now!
     *     This is rare, but I am not sure how to
     *     fix this at the moment*/
    return;
  }

  LDM_ORIG(free)(ptr);

  if(inited){
    size_t sz = 0;
    sz = (size_t)g_hash_table_lookup(blockSizes,(gconstpointer)ptr);
    if( sz != 0 ){
      numFreedBytes += sz;
      g_hash_table_remove(blockSizes,(gconstpointer)ptr);
    }
    bytesSinceLastRead -= sz;
  }

}

LDM_ORIG_DECL(void *,malloc,size_t);
void *malloc(size_t sz){

  void *ret;

  if(LDM_ORIG(malloc) == NULL){
    LDM_REG(malloc);
  }

  ret = LDM_ORIG(malloc)(sz);

  if(inited){
    g_hash_table_insert(blockSizes, (gpointer)ret, (gpointer)sz);
    numAllocatedBytes += sz;
    bytesSinceLastRead += sz;
  }

  return ret;
}

LDM_ORIG_DECL(int,read,int,void *,size_t);
int read(int fd,void *buf,size_t sz){

  int i;
  float sum = 0.0;
  for( i = 0; i < S_LEN - 1; i++ ){
    allocPerStreamHisto[i] = allocPerStreamHisto[i+1];
    sum += allocPerStreamHisto[i];
  }

  allocPerStreamHisto[S_LEN-1] = (float)bytesSinceLastRead;
  sum += allocPerStreamHisto[S_LEN-1];

  float avg = sum/(float)S_LEN; 
  if( avg > 0.0 ){
    ldmmsg(stderr,"[Scaley] Potential Scaling Problem: %f Bytes Allocated / Stream Read\n",avg);
  }

  if( fd == 0 ){
    numStdinBytes += sz;
  }
  lastReadBytes = sz;
  bytesSinceLastRead = 0;

  return LDM_ORIG(read)(fd,buf,sz);

}

static void deinit(){

  ldmmsg(stderr,"[Scaley] ---- Summary Statistics ----\n");
  ldmmsg(stderr,"[Scaley] Allocated: %ld\n",numAllocatedBytes);
  ldmmsg(stderr,"[Scaley] Freed: %ld\n",numFreedBytes);
  ldmmsg(stderr,"[Scaley] Streamed In: %ld\n",numStdinBytes);

}

void LDM_PLUGIN_INIT(){

  LDM_REG(read);

  if( LDM_ORIG(malloc) == NULL ){
    LDM_REG(malloc);
  }
  if( LDM_ORIG(free) == NULL ){
    LDM_REG(free);
  }

  GMemVTable malvt; 
  malvt.malloc = (gpointer(*)(gsize))(LDM_ORIG(malloc));
  malvt.realloc = realloc;
  malvt.free = (void(*)(gpointer))(LDM_ORIG(free));
  malvt.calloc = NULL;
  malvt.try_malloc = NULL;
  malvt.try_realloc = NULL;

  g_mem_set_vtable(&malvt); 
  blockSizes = g_hash_table_new(g_direct_hash,g_direct_equal);


  inited = 1;


  LDM_PLUGIN_DEINIT(deinit);
  
}
