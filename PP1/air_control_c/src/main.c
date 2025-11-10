
#include "../include/functions.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

int main() {
  // 1. Create shared memory and store our PID
  MemoryCreate();
  printf("[air_control] created shared memory, air pid=%d\n", getpid());
  fflush(stdout);

  // 3. Configure SIGUSR2 handler to increment planes by 5
  signal(SIGUSR2, SigHandler2);

  // 4. Launch the radio executable
  pid_t child = fork();
  if (child < 0) {
    perror("fork");
    return 1;
  }

  if (child == 0) {
    // child: execute radio. Working directory when tests run expects radio at
    // ../radio/build/radio
    const char* radio_path = "../radio/build/radio";
    // Ensure the radio executable has execute permissions (in case it's mounted without +x)
    chmod(radio_path, 0755);
    execl(radio_path, "radio", SH_MEMORY_NAME, (char*)NULL);
    // if execl returns, it failed. Attempt to compile from source and retry.
    perror("execl radio");
    fflush(stderr);
    fprintf(stderr, "[air_control] attempting to compile fallback radio from ../radio/src/main.c...\n");
    fflush(stderr);
    int rc = system("gcc -O2 -Wall -Wextra -o /tmp/radio_exec ../radio/src/main.c");
    if (rc == 0) {
      chmod("/tmp/radio_exec", 0755);
      execl("/tmp/radio_exec", "radio", SH_MEMORY_NAME, (char*)NULL);
      // if still failing, report and exit
      perror("execl fallback radio");
    } else {
      fprintf(stderr, "[air_control] gcc failed to build fallback radio (rc=%d)\n", rc);
    }
    _exit(1);
  }

  // parent: store radio PID in shared memory (second position)
  if (sh_memory != NULL) {
    sh_memory[1] = (int)child;
    printf("[air_control] stored radio pid=%d in shared memory\n", (int)child);
    fflush(stdout);
  }

  // 6. Launch 5 controller threads
  const int THREADS = 5;
  pthread_t threads[THREADS];

  for (int i = 0; i < THREADS; ++i) {
    if (pthread_create(&threads[i], NULL, TakeOffsFunction, NULL) != 0) {
      perror("pthread_create");
      // continue trying to create remaining threads
    }
  }

  // Wait for threads to finish
  for (int i = 0; i < THREADS; ++i) {
    pthread_join(threads[i], NULL);
  }

  // Ensure radio is terminated
  if (sh_memory && sh_memory[1] > 0) {
    kill(sh_memory[1], SIGTERM);
  }

  // cleanup shared memory
  if (shm_fd != -1) close(shm_fd);
  shm_unlink(SH_MEMORY_NAME);

  // wait for child
  waitpid(child, NULL, 0);

  // Reap any other exited child processes (best-effort, non-blocking)
  {
    int status;
    pid_t wp;
    while ((wp = waitpid(-1, &status, WNOHANG)) > 0) {
      // reaped child wp
    }
  }

  return 0;
}