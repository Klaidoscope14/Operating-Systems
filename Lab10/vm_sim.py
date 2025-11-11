import threading
import queue
import time
import random

NUM_PROCESSES = 3
REF_LEN = 15
PAGE_RANGE = 8
NUM_FRAMES = 4

request_queue = queue.Queue()
ready_queue = queue.Queue()
terminate_event = threading.Event()

page_tables = {pid: {} for pid in range(NUM_PROCESSES)}
frame_to_owner = {}
frame_lock = threading.Lock()
lru_list = []

def log(s):
    print(s)

def process_thread(proc_id):
    refs = [random.randint(0, PAGE_RANGE - 1) for _ in range(REF_LEN)]
    log(f"Process {proc_id} created, refs: {refs}")
    for page in refs:
        ready_queue.put(proc_id)
        request_queue.put((proc_id, page))
        time.sleep(0.05 + random.random() * 0.05)
    log(f"Process {proc_id} finished.")
    request_queue.put(("DONE", proc_id))

def scheduler_thread():
    log("Scheduler started (FCFS).")
    while not terminate_event.is_set():
        time.sleep(0.2)
    log("Scheduler terminating.")

def mmu_thread():
    log("MMU started.")
    free_frames = list(range(NUM_FRAMES))
    page_faults = 0

    def load_page(proc, page):
        nonlocal page_faults
        page_faults += 1
        if free_frames:
            f = free_frames.pop(0)
            log(f"Page Fault handled for Process {proc}, Page {page} -> Frame {f}")
        else:
            lru_list.sort(key=lambda x: x[0])
            victim_frame = lru_list.pop(0)[1]
            vproc, vpage = frame_to_owner.pop(victim_frame)
            if vpage in page_tables[vproc]:
                del page_tables[vproc][vpage]
            f = victim_frame
            log(f"Replaced Frame {f} of Process {vproc} Page {vpage} with Process {proc} Page {page}")
        page_tables[proc][page] = f
        frame_to_owner[f] = (proc, page)
        now = time.time()
        lru_list.append((now, f))

    while True:
        item = request_queue.get()
        if item[0] == "DONE":
            pid_done = item[1]
            log(f"Process {pid_done} completed.")
            continue
        proc, page = item
        log(f"Process {proc} requests page {page}")
        if page in page_tables[proc]:
            frame = page_tables[proc][page]
            now = time.time()
            for i, (t, f) in enumerate(lru_list):
                if f == frame:
                    lru_list.pop(i)
                    break
            lru_list.append((now, frame))
            log(f"Page hit: Process {proc} Page {page} in Frame {frame}")
        else:
            log(f"Page fault: Process {proc} Page {page}")
            time.sleep(0.1)
            with frame_lock:
                load_page(proc, page)
        time.sleep(0.01)

def master_thread():
    log("Master started.")
    sched = threading.Thread(target=scheduler_thread, daemon=True)
    mmu = threading.Thread(target=mmu_thread, daemon=True)
    sched.start()
    mmu.start()
    procs = []
    for pid in range(NUM_PROCESSES):
        t = threading.Thread(target=process_thread, args=(pid,))
        t.start()
        procs.append(t)
        time.sleep(0.02)
    for t in procs:
        t.join()
    terminate_event.set()
    log("Master terminating. All processes completed.")
    time.sleep(0.2)

if __name__ == "__main__":
    random.seed(42)
    master_thread()