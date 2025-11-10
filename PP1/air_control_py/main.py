# File: air_control_py/main.py
import ctypes
import mmap
import os
import signal
import struct
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
SHM_NAME = b"/shm_pids_" 
INT_SIZE = struct.calcsize("i")
SHM_LENGTH = 3 * INT_SIZE 

planes = 0
takeoffs = 0
total_takeoffs = 0

state_lock = threading.Lock()
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()

def _get_errno() -> int:
    return ctypes.get_errno()

def _write_slot(mm: mmap.mmap, index: int, value: int) -> None:
    struct.pack_into("i", mm, index * INT_SIZE, int(value))

def _read_slot(mm: mmap.mmap, index: int) -> int:
    return struct.unpack_from("i", mm, index * INT_SIZE)[0]

def create_shared_memory():
    """Create POSIX shared memory: returns (fd, mm)."""
    old_umask = os.umask(0)
    try:
        fd = _libc.shm_open(SHM_NAME, os.O_CREAT | os.O_RDWR, 0o666)
        if fd == -1:
            err = _get_errno()
            raise OSError(err, f"shm_open failed: {os.strerror(err)}")
        if _libc.ftruncate(fd, SHM_LENGTH) == -1:
            err = _get_errno()
            os.close(fd)
            raise OSError(err, f"ftruncate failed: {os.strerror(err)}")
    finally:
        os.umask(old_umask)

    mm = mmap.mmap(fd, SHM_LENGTH, prot=mmap.PROT_READ | mmap.PROT_WRITE, flags=mmap.MAP_SHARED)
    mm[:] = b"\x00" * SHM_LENGTH
    _write_slot(mm, 0, os.getpid())
    return fd, mm

def HandleUSR2(signum, frame):
    """SIGUSR2: +5 planes available."""
    global planes
    with state_lock:
        planes += 5

def _locate_radio_binary() -> str:
    candidates = [
        "./radio",
        os.path.join(os.path.dirname(__file__), "radio"),
        os.path.join(os.path.dirname(__file__), "..", "radio"),
    ]
    for p in candidates:
        if os.path.exists(p) and os.access(p, os.X_OK):
            return p
    return "./radio"

def launch_radio(mm: mmap.mmap) -> subprocess.Popen:
    """Launch radio and record its PID in shared memory."""
    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    radio_path = _locate_radio_binary()
    proc = subprocess.Popen([radio_path, SHM_NAME.decode("ascii")], preexec_fn=_unblock_sigusr2)
    _write_slot(mm, 1, proc.pid)
    return proc

def TakeOffFunction(agent_id: int, mm: mmap.mmap):
    """Thread worker for controlling takeoffs."""
    global planes, takeoffs, total_takeoffs

    while True:
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                break

        # Try to acquire any runway
        runway = None
        if runway1_lock.acquire(blocking=False):
            runway = runway1_lock
        elif runway2_lock.acquire(blocking=False):
            runway = runway2_lock
        else:
            time.sleep(0.001)
            continue

        with state_lock:
            if planes > 0 and total_takeoffs < TOTAL_TAKEOFFS:
                planes -= 1
                takeoffs += 1
                total_takeoffs += 1
                if takeoffs == 5:
                    rpid = _read_slot(mm, 1)
                    if rpid > 0:
                        try:
                            os.kill(rpid, signal.SIGUSR1)
                        except ProcessLookupError:
                            pass
                    takeoffs = 0

        time.sleep(1)  # simulate takeoff
        runway.release()

    # Send termination to radio
    rpid = _read_slot(mm, 1)
    if rpid > 0:
        try:
            os.kill(rpid, signal.SIGTERM)
        except ProcessLookupError:
            pass

def main():
    # Configure SIGUSR2 handler
    signal.signal(signal.SIGUSR2, HandleUSR2)

    # Create shared memory and store own PID
    fd, mm = create_shared_memory()

    # Launch radio and record PID
    radio_proc = launch_radio(mm)

    # Start controller threads
    threads = []
    for i in range(STRIPS):
        t = threading.Thread(target=TakeOffFunction, args=(i, mm), daemon=False)
        threads.append(t)
        t.start()

    # Wait for completion
    for t in threads:
        t.join()

    # Ensure radio is terminated
    try:
        radio_proc.wait(timeout=1)
    except Exception:
        try:
            radio_proc.terminate()
        except Exception:
            pass

    # Cleanup: close descriptors and mapping. Do not unlink; ground may still read.
    mm.flush()
    mm.close()
    os.close(fd)

if __name__ == "__main__":
    # No prints. Logging handled externally by tests if needed.
    main()
