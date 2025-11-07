#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int planes = 0;
int takeoffs = 0;
int* pids[3] = {NULL};
const char* shm_name = NULL;
int sh_memory_open = -1;

void SigHandler1(int signal) {
  takeoffs = takeoffs + 5;
  printf("Takeoff clearance %d\n", takeoffs);
  kill((*pids)[2], SIGUSR1);
};

void SigHandler2(int signal) {
  planes += 5;
  printf("Planes: %d\n", planes);
  kill((*pids)[0], SIGUSR2);
  if (planes - takeoffs >= 10 && planes < 50) {
    printf("RUNWAY OVERLOADED!!!! \n");
  }
};

void SigTerm(int signal) {
  printf(":::: End of operations ::::\nTakeoffs: %d Planes: %d\n", takeoffs,
         planes);
  printf("air pid: %d\n", (*pids)[0]);
  printf("radio pid: %d\n", (*pids)[1]);
  printf("ground pid: %d\n:::::::::::::::::::::::::::\n", (*pids)[2]);
  kill((*pids)[2], SIGTERM);
  close(sh_memory_open);
  shm_unlink(shm_name);
  exit(0);
};

int main(int argc, char* argv[]) {
  printf("radio pid: %d\n", getpid());
  if (argc != 2) {
    fprintf(stderr, "Usage: %s arguments\n", argv[0]);
    return 1;
  }
  shm_name = argv[1];

  sh_memory_open = shm_open(shm_name, O_RDWR, 0666);
  if (sh_memory_open == -1) {
    perror("shm_open_error_radio");
    shm_unlink(shm_name);
    return 1;
  }
  int* sh_memory = mmap(NULL, sizeof(int) * 3, PROT_READ | PROT_WRITE,
                        MAP_SHARED, sh_memory_open, 0);
  if (sh_memory == MAP_FAILED) {
    perror("mmap_error_radio");
    return 1;
  }
  *pids = (int*)sh_memory;

  signal(SIGUSR1, SigHandler1);
  signal(SIGUSR2, SigHandler2);
  signal(SIGTERM, SigTerm);

  while (takeoffs <= 100) pause();
}