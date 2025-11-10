#include "../include/functions.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int shm_fd = -1;
int* sh_memory = NULL;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway2_lock = PTHREAD_MUTEX_INITIALIZER;

void MemoryCreate() {
  // Create shared memory to store 3 integers (air, radio, ground PIDs)
  const char* name = SH_MEMORY_NAME;
  /* Ensure no stale shm object exists */
  shm_unlink(name);

  shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open_error");
    exit(1);
  }

  if (ftruncate(shm_fd, sizeof(int) * 3) == -1) {
    perror("ftruncate_error");
    close(shm_fd);
    shm_unlink(name);
    exit(1);
  }

  void* map = mmap(NULL, sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED,
                   shm_fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap_error");
    close(shm_fd);
    shm_unlink(name);
    exit(1);
  }

  sh_memory = (int*)map;
  // initialize entries
  sh_memory[0] = getpid();
  sh_memory[1] = 0;
  sh_memory[2] = 0;
}

void SigHandler2(int signal) {
  // Increment planes by 5 when SIGUSR2 received
  (void)signal;
  pthread_mutex_lock(&state_lock);
  planes += 5;
  pthread_mutex_unlock(&state_lock);
}

void* TakeOffsFunction(void* arg) {
  (void)arg;
  int have_runway = 0;  // 1 => runway1, 2 => runway2

  while (1) {
    // Check termination condition safely
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS) {
      pthread_mutex_unlock(&state_lock);
      break;
    }
    pthread_mutex_unlock(&state_lock);

    // Try to acquire one of the runways
    if (pthread_mutex_trylock(&runway1_lock) == 0) {
      have_runway = 1;
    } else if (pthread_mutex_trylock(&runway2_lock) == 0) {
      have_runway = 2;
    } else {
      // no runway available, wait a bit and retry
      usleep(1000);
      continue;
    }

    // We have exclusive use of a runway now
    pthread_mutex_lock(&state_lock);
    if (planes > 0 && total_takeoffs < TOTAL_TAKEOFFS) {
      planes -= 1;
      takeoffs += 1;
      total_takeoffs += 1;

      // debug: log progress so test harness can detect progress
      printf("[air_control] total_takeoffs=%d planes=%d takeoffs_local=%d\n",
             total_takeoffs, planes, takeoffs);
      fflush(stdout);

      // Every 5 local takeoffs, inform radio with SIGUSR1
      if (takeoffs >= 5) {
        takeoffs = 0;
        if (sh_memory && sh_memory[1] > 0) {
          kill(sh_memory[1], SIGUSR1);
        }
      }
    }
    pthread_mutex_unlock(&state_lock);

    // simulate takeoff time
    sleep(1);

    // release runway
    if (have_runway == 1) {
      pthread_mutex_unlock(&runway1_lock);
    } else if (have_runway == 2) {
      pthread_mutex_unlock(&runway2_lock);
    }
    have_runway = 0;

    // After doing a takeoff, check if we've reached the global goal
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS) {
      // send SIGTERM to radio
      if (sh_memory && sh_memory[1] > 0) {
        printf(
            "[air_control] reached TOTAL_TAKEOFFS (%d), sending SIGTERM to "
            "radio pid=%d\n",
            total_takeoffs, sh_memory[1]);
        fflush(stdout);
        kill(sh_memory[1], SIGTERM);
      }
      pthread_mutex_unlock(&state_lock);
      break;
    }
    pthread_mutex_unlock(&state_lock);
  }

  return NULL;
}