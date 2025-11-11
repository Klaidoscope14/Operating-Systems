//simple non-preemptive FIFO scheduler simulation
#include <stdio.h>
#include <stdlib.h>

typedef struct Process {
    int pid;
    int cpu_total;
    int blocked_event;    // -1 if not blocked
    struct Process *next;
} Process;

static Process *ready_h = NULL, *ready_t = NULL; //Head and Tail of READY queue
static Process *blocked_h = NULL;                //Head of BLOCKED queue
static Process *running = NULL;                  // Currently running process   
static int next_pid = 1;                         // Next available PID

// Adds process p to the end of the READY queue.
static void enqueue_ready(Process *p){
    p->next = NULL;
    if (!ready_t) ready_h = ready_t = p;
    else { ready_t->next = p; ready_t = p; }
}

// Removes and returns the process at the front (FIFO).
static Process* dequeue_ready(void){
    if (!ready_h) return NULL;
    Process *p = ready_h;
    ready_h = ready_h->next;
    if (!ready_h) ready_t = NULL;
    p->next = NULL;
    return p;
}

// Adds process p to the front of the BLOCKED queue (not FIFO).
static void add_blocked(Process *p){
    p->next = blocked_h;
    blocked_h = p;
}

// Removes and returns the first process in BLOCKED queue that is waiting for event eid.
static Process* remove_blocked_by_event(int eid){
    Process **pp = &blocked_h;
    while (*pp) {
        if ((*pp)->blocked_event == eid) {
            Process *found = *pp;
            *pp = (*pp)->next;
            found->next = NULL;
            return found;
        }
        pp = &((*pp)->next);
    }
    return NULL;
}

// Prints the current state of the scheduler.
static void printState(void){
    printf("STATE | RUNNING: ");
    if (running) printf("%d(cpu=%d)", running->pid, running->cpu_total);
    else printf("None");
    printf(" | READY: ");
    for (Process *p=ready_h; p; p=p->next) printf("%d ", p->pid);
    printf("| BLOCKED: ");
    for (Process *p=blocked_h; p; p=p->next) printf("%d(e=%d) ", p->pid, p->blocked_event);
    puts("");
}

// Creates a new process, assigns it a PID, and adds it to the READY queue.
static void newProcess(void){
    Process *p = (Process*)calloc(1, sizeof(Process));
    p->pid = next_pid++;
    p->cpu_total = 0;
    p->blocked_event = -1;
    enqueue_ready(p);
    printf("New: Process %d created and added to READY queue\n", p->pid);
    printState();
}

// Simulates a CPU event where the running process gets to run for 1 cycle.
static void cpuEvent(void){
    if (!running) {
        running = dequeue_ready();
        if (running) printf("Dispatch: Process %d is now RUNNING\n", running->pid);
        else { puts("CPU: No READY process"); printState(); return; }
    }
    running->cpu_total += 1;
    printf("CPU: Process %d ran for 1 cycle (total: %d)\n", running->pid, running->cpu_total);
    printState();
}

// Blocks the currently running process on a specific event.
static void blockEvent(int eventId){
    if (!running) { puts("Block: No RUNNING process"); printState(); return; }
    printf("Block: Process %d is BLOCKED on event %d\n", running->pid, eventId);
    running->blocked_event = eventId;
    add_blocked(running);
    running = NULL;
    printState();
}

// Unblocks a process waiting on a specific event and moves it to the READY queue.
static void unblockEvent(int eventId){
    Process *p = remove_blocked_by_event(eventId);
    if (!p) { printf("Unblock: No process waiting on event %d\n", eventId); printState(); return; }
    p->blocked_event = -1;
    enqueue_ready(p);
    printf("Unblock: Process %d moved to READY queue\n", p->pid);
    printState();
}

// Simulates a process finishing execution and being removed from the system.
static void doneEvent(void){
    if (!running) { puts("Done: No RUNNING process"); printState(); return; }
    printf("Done: Process %d has finished execution\n", running->pid);
    free(running);
    running = NULL;
    printState();
}

int main(void){
    newProcess();
    cpuEvent();
    cpuEvent();
    blockEvent(101);
    newProcess();
    cpuEvent();
    doneEvent();
    newProcess();
    cpuEvent();
    unblockEvent(101);
    cpuEvent();
    doneEvent();
    cpuEvent();
    doneEvent();
    return 0;
}
