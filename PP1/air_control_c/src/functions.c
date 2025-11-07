int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
}

// void SigHandler2(int signal) {}

void* TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
}