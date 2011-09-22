typedef enum _SamplingState{
  SAMPLE_ON = 0,
  SAMPLE_OFF 
} SamplingState;

#define MAX_NUM_THREADS 512

void sampled_thread_monitor_deinit(void *d);
void sampled_thread_monitor_thread_init(void *targ, void*(*thdrtn)(void*));
void sampled_thread_monitor_init(void *data,void (*ph)(SamplingState));
