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
SHM_LENGTH = 3 * (ctypes.sizeof(ctypes.c_int))

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs


def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    name = "/shmMem"
    name = name.encode("utf-8") #utf-8 is the default method
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    umask = os.umask(777)

    # 3. Use _libc.shm_open to create the shared memory
    fd = _libc.shm_open(name, os.O_RDONLY, 0o660)
    if(fd == -1):
        err = ctypes.errno()
        raise OSError(err, f"shm_open failed: {os.strerror(err)}")
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    if (_libc.ftruncate(fd, SHM_LENGTH) == -1):
        err = ctypes.errno()
        raise OSError(err, f"ftruncate failed: {os.strerror(err)}")
    # 5. Restore the original permission mask (umask)
    umask = os.umask(umask)
    # 6. Use mmap to map the shared memory
    map = mmap.mmap(0,SHM_LENGTH, mmap.PROT_READ, mmap.MAP_SHARED,fd,0)
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    view = memoryview(map)
    if(len(view) != 3):
        map.close()
        os.close(fd)
        raise RuntimeError(f"Expected a length of 3, got {len(view)} elements")
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return view



def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    planes += 5



def TakeOffFunction(agent_id: int):
    
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs
    runway1_lock = threading.Lock
    runway2_lock = threading.Lock
    state_lock = threading.Lock
    # TODO: implement the logic to control a takeoff thread
    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    while(total_takeoffs < TOTAL_TAKEOFFS):
        # Use runway1_lock or runway2_lock to simulate runway being locked

        if(runway1_lock.locked() == False):
            runway1_lock.acquire()
            currentRunWay = 1
        elif(runway2_lock.locked() == False):
            runway2_lock.acquire()
            currentRunWay = 2
        else:
            time.sleep(0.0001)
            continue

        if(planes != 0):
            # Use state_lock for safe access to shared variables (planes, takeoffs, total_takeoffs)
            state_lock.acquire()
            planes -= 1
            takeoffs, total_takeoffs += 1
            if(takeoffs == 5):
                takeoffs = 0
                # Send SIGUSR1 every 5 local takeoffs
                os.kill()
            state_lock.release()
        # Simulate the time a takeoff takes with sleep(1)
        time.sleep(1)
        if(currentRunWay == 1):
            runway1_lock.release()
        else:
            runway2_lock.release()
    if(total_takeoffs >= TOTAL_TAKEOFFS):
        # Send SIGTERM when the total takeoffs target is reached
        # os.kill(pid, signal.SIGTERM)
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
    signal.signal(signal.SIGUSR2, HandleUSR2)

    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    # fd, memory, data = 
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    # radio_process = 
    # TODO 9: Create and start takeoff controller threads (STRIPS) 
    # TODO 10: Wait for all threads to finish their work
    # TODO 11: Release shared memory and close resources



if __name__ == "__main__":
    main()