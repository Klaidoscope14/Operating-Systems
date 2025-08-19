//simple non-preemptive FIFO scheduler simulation
#include <stdio.h>
#include <stdlib.h>

typedef struct Process {
    int pid;
    int cpu_total;
    int blocked_event;    // -1 if not blocked
    struct Process *next;
} Process;

static Process *ready_h = NULL, *ready_t = NULL;
static Process *blocked_h = NULL;
static Process *running = NULL;
static int next_pid = 1;

static void enqueue_ready(Process *p){
    p->next = NULL;
    if (!ready_t) ready_h = ready_t = p;
    else { ready_t->next = p; ready_t = p; }
}

static Process* dequeue_ready(void){
    if (!ready_h) return NULL;
    Process *p = ready_h;
    ready_h = ready_h->next;
    if (!ready_h) ready_t = NULL;
    p->next = NULL;
    return p;
}

static void add_blocked(Process *p){
    p->next = blocked_h;
    blocked_h = p;
}

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

static void newProcess(void){
    Process *p = (Process*)calloc(1, sizeof(Process));
    p->pid = next_pid++;
    p->cpu_total = 0;
    p->blocked_event = -1;
    enqueue_ready(p);
    printf("New: Process %d created and added to READY queue\n", p->pid);
    printState();
}

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

static void blockEvent(int eventId){
    if (!running) { puts("Block: No RUNNING process"); printState(); return; }
    printf("Block: Process %d is BLOCKED on event %d\n", running->pid, eventId);
    running->blocked_event = eventId;
    add_blocked(running);
    running = NULL;
    printState();
}

static void unblockEvent(int eventId){
    Process *p = remove_blocked_by_event(eventId);
    if (!p) { printf("Unblock: No process waiting on event %d\n", eventId); printState(); return; }
    p->blocked_event = -1;
    enqueue_ready(p);
    printf("Unblock: Process %d moved to READY queue\n", p->pid);
    printState();
}

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
