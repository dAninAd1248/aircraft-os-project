
#define PLANES_LIMIT 20
int planes = 0;
int takeoffs = 0;
int traffic = 0;

void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
}

int main(int argc, char* argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  // 3. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.
  // 2. Configure the timer to execute the Traffic function.
}