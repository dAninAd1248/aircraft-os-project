import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
shm_data = []

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
# SHM_LENGTH=

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs



def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    # 3. Use _libc.shm_open to create the shared memory
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    # 5. Restore the original permission mask (umask)
    # 6. Use mmap to map the shared memory
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    # 8. Return the file descriptor (shm_open), mmap object and memory view



def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    pass


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs

    # TODO: implement the logic to control a takeoff thread
    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    # Use runway1_lock or runway2_lock to simulate runway being locked
    # Use state_lock for safe access to shared variables (planes, takeoffs, total_takeoffs)
    # Simulate the time a takeoff takes with sleep(1)
    # Send SIGUSR1 every 5 local takeoffs
    # Send SIGTERM when the total takeoffs target is reached
    pass


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    # process = 
    # return process


def main():
    global shm_data

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    # fd, memory, data = 
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    # radio_process = 
    # TODO 9: Create and start takeoff controller threads (STRIPS) 
    # TODO 10: Wait for all threads to finish their work
    # TODO 11: Release shared memory and close resources



if __name__ == "__main__":
    main()