// Ground control: produce and manage plane traffic, signal radio to forward to air_control.

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define PLANES_LIMIT 20

int planes = 0;
int takeoffs = 0;
static int overloaded_state = 0; // 0: normal, 1: overload reported
static int sigusr2_sent = 0; // limit how many SIGUSR2 we send to radio (max 4)

int shm_fd = -1;
int* sh_memory = NULL;

void SigTermHandler(int signum) {
  (void)signum;
  if (shm_fd != -1) close(shm_fd);
  printf("[ground] SIGTERM received pid=%d, finalization of operations...\n", getpid());
  fflush(stdout);
  exit(0);
}

void SigUsr1Handler(int signum) {
  (void)signum;
  takeoffs += 5;
  printf("[ground] SIGUSR1 received: takeoffs+=5 -> takeoffs=%d (before reducing planes)\n", takeoffs);
  fflush(stdout);
  // Reflect that 5 planes have taken off
  if (planes >= 5) {
    planes -= 5;
  } else {
    planes = 0;
  }
  if (planes < 10) {
    overloaded_state = 0; // reset overload when below threshold
  }
  printf("[ground] after processing takeoffs: planes=%d\n", planes);
  fflush(stdout);

  // Stop traffic once we've observed all 20 takeoffs to avoid inflating radio's plane count
  if (takeoffs >= 20) {
    // Cancel periodic timer so Traffic() stops adding planes and sending SIGUSR2
    struct itimerval stop = {0};
    setitimer(ITIMER_REAL, &stop, NULL);
  }
}

void Traffic(int signum) {
  (void)signum;
  // If we've already reached the target number of takeoffs, do not add more planes
  if (takeoffs >= 20) {
    return;
  }
  // Check overload (print once per crossing)
  if (planes >= 10) {
    if (!overloaded_state) {
      printf("RUNWAY OVERLOADED (ground pid=%d, planes=%d)\n", getpid(), planes);
      fflush(stdout);
      overloaded_state = 1;
    }
  } else if (overloaded_state) {
    overloaded_state = 0;
  }

  if (planes < PLANES_LIMIT) {
    int add = 5;
    if (planes + add > PLANES_LIMIT) add = PLANES_LIMIT - planes;
    if (add > 0) {
      int before = planes;
      planes += add;
      printf("[ground] Traffic: added %d planes (before=%d after=%d)\n", add, before, planes);
      fflush(stdout);
      // send SIGUSR2 to radio so radio forwards to air_control
      if (sigusr2_sent < 4 && sh_memory && sh_memory[1] > 0) {
        printf("[ground] sending SIGUSR2 to radio pid=%d\n", sh_memory[1]);
        fflush(stdout);
        kill(sh_memory[1], SIGUSR2);
        sigusr2_sent++;
      } else if (!(sh_memory && sh_memory[1] > 0)) {
        printf("[ground] no radio pid in shm to send SIGUSR2 (sh_memory[1]=%d)\n", sh_memory ? sh_memory[1] : 0);
        fflush(stdout);
      } else {
        // Reached max number of SIGUSR2 sends; do not notify radio further
        printf("[ground] max SIGUSR2 sends reached (%d), not notifying radio\n", sigusr2_sent);
        fflush(stdout);
      }
    }
  }
}

int main(int argc, char* argv[]) {
  const char* name = "/shm_pids_";

  // Try to open the shared memory created by air_control. Retry a few times
  // to avoid a race where ground starts slightly before air_control creates it.
  int attempts = 0;
  while ((shm_fd = shm_open(name, O_RDWR, 0666)) == -1 && attempts < 50) {
    attempts++;
    usleep(100000); // 100ms
  }
  if (shm_fd == -1) {
    perror("shm_open ground");
    fprintf(stderr, "ground: failed to open shared memory after %d attempts\n", attempts);
    return 1;
  }

  sh_memory = mmap(NULL, sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED,
                   shm_fd, 0);
  if (sh_memory == MAP_FAILED) {
    perror("mmap ground");
    close(shm_fd);
    return 1;
  }

  // Store our PID in position 2
  sh_memory[2] = getpid();

  // Configure signal handlers
  signal(SIGTERM, SigTermHandler);
  signal(SIGUSR1, SigUsr1Handler);
  signal(SIGALRM, Traffic);

  // Configure periodic timer every 500ms
  struct itimerval timer;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000; // 500 ms
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000; // first trigger

  if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
    perror("setitimer");
    return 1;
  }

  // Loop until terminated
  while (1) pause();

  return 0;
}