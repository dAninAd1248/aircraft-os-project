#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <pthread.h>
#include <signal.h>

#define TOTAL_TAKEOFFS 20
#define SH_MEMORY_NAME "/shm_pids_"

extern int planes;
extern int takeoffs;
extern int total_takeoffs;

extern int shm_fd;
extern int* sh_memory;

extern pthread_mutex_t state_lock;
extern pthread_mutex_t runway1_lock;
extern pthread_mutex_t runway2_lock;

void MemoryCreate();
void SigHandler2(int signal);
void* TakeOffsFunction(void* arg);

#endif // FUNCTIONS_H

