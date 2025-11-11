#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

typedef struct { pthread_mutex_t m; pthread_cond_t c; } condpair_t;
static int N = 6;
static int F = 4;
static double T_priority = 1.0; // seconds
static int duration_sec = 60;

static volatile int running = 1;
static time_t start_time;

static int forks_available;
static pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waiter_cv = PTHREAD_COND_INITIALIZER;

static condpair_t *ph_cv;         // per-philosopher condition vars
static int *waiting;              // 1 if currently queued
static int *granted;              // waiter granted 2 forks?
static double *req_time;          // absolute seconds since start when requested

// simple queue of philosopher ids
static int *Q; static int qh=0, qt=0, qcap=0;

static int *eats;
static double *wait_sum, *wait_max;
static int *active;               // 1 if philosopher still participating
static double *join_at_sec;       // late join time (sec since start)
static double *leave_at_sec;      // early leave time (sec since start), or negative if not leaving

static double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec/1e9;
}

static double since_start(void) {
    static double t0 = -1;
    if (t0 < 0) t0 = now_sec();
    return now_sec() - t0;
}

static void q_init(int cap) { Q = (int*)malloc(sizeof(int)*cap); qcap=cap; qh=qt=0; }
static int  q_empty(void){ return qh==qt; }
static void q_push(int v){ Q[qt++ % qcap] = v; }
static int  q_pop(void)  { return Q[qh++ % qcap]; }
static int  q_size(void) { return qt-qh; }
static int  q_peek(int idx){ return Q[(qh+idx)%qcap]; }
static void q_erase_idx(int idx){
    int sz = q_size();
    for (int k=idx; k<sz-1; k++) Q[(qh+k)%qcap]=Q[(qh+k+1)%qcap];
    qt--;
}

static void msleep(int ms){ struct timespec ts={ms/1000,(ms%1000)*1000000}; nanosleep(&ts,NULL); }

static void* waiter_thread(void* arg){
    (void)arg;
    while (running) {
        pthread_mutex_lock(&M);
        // choose next candidate
        int chosen = -1;
        double chosen_wait = -1.0;

        // Check if anyone is waiting long enough for priority
        int sz = q_size();
        double now = since_start();
        for (int i = 0; i < sz; i++) {
            int pid = q_peek(i);
            if (!active[pid]) { q_erase_idx(i); sz--; i--; continue; }
            double w = now - req_time[pid];
            if (w >= T_priority && forks_available >= 2) {
                if (w > chosen_wait) { chosen = pid; chosen_wait = w; }
            }
        }

        if (chosen == -1 && sz > 0 && forks_available >= 2) {
            // FIFO if no one crosses T
            chosen = q_peek(0);
        }

        if (chosen != -1 && active[chosen] && forks_available >= 2) {
            // allocate
            forks_available -= 2;
            int sz2 = q_size();
            for (int i=0;i<sz2;i++) if (q_peek(i)==chosen){ q_erase_idx(i); break; }
            waiting[chosen] = 0;
            granted[chosen] = 1;
            pthread_cond_signal(&ph_cv[chosen].c);
        } 
        
        else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += 100 * 1000000; // 100ms
            if (ts.tv_nsec >= 1000000000L) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000L; }
            pthread_cond_timedwait(&waiter_cv, &M, &ts);
        }

        pthread_mutex_unlock(&M);

        if (since_start() >= duration_sec) running = 0;
    }

    pthread_mutex_lock(&M);
    printf("===== Royal Banquet Report =====\n");
    for (int i=0;i<N;i++) {
        double avg = (eats[i] ? (wait_sum[i]/eats[i]) : 0.0);
        printf("Philosopher %d: Ate %d times | Avg Wait: %.2fs | Max Wait: %.2fs\n",
               i, eats[i], avg, wait_max[i]);
    }
    pthread_mutex_unlock(&M);
    return NULL;
}

static void request_to_eat(int id){
    pthread_mutex_lock(&M);
    if (!waiting[id]) {
        waiting[id] = 1;
        req_time[id] = since_start();
        q_push(id);
        pthread_cond_signal(&waiter_cv);
    }
    while (!granted[id] && active[id] && running)
        pthread_cond_wait(&ph_cv[id].c, &M);
    double w = since_start() - req_time[id];
    granted[id] = 0; // consume grant
    pthread_mutex_unlock(&M);

    // record waiting time
    if (w < 0) w = 0;
    if (w > wait_max[id]) wait_max[id] = w;
    wait_sum[id] += w;
}

static void release_forks(void){
    pthread_mutex_lock(&M);
    forks_available += 2;
    pthread_cond_signal(&waiter_cv);
    pthread_mutex_unlock(&M);
}

static void* philosopher(void* arg){
    long id = (long)arg;

    // Late join?
    if (join_at_sec[id] > 0) {
        int ms = (int)(join_at_sec[id]*1000);
        msleep(ms);
        printf("Philosopher %ld arrives late to the banquet!\n", id);
    }

    active[id] = 1;
    printf("Philosopher %ld is thinking...\n", id);

    while (running && active[id]) {
        msleep(100 + rand()%300);

        if (leave_at_sec[id] > 0 && since_start() >= leave_at_sec[id]) {
            active[id] = 0;
            printf("Philosopher %ld leaves the banquet early!\n", id);
            break;
        }

        request_to_eat((int)id);
        if (!running || !active[id]) break;

        printf("Philosopher %ld is eating...\n", id);
        msleep(100 + rand()%250);

        eats[id]++;
        release_forks();
    }
    return NULL;
}

int main(int argc, char **argv){
    if (argc >= 2) N = atoi(argv[1]);
    if (argc >= 3) F = atoi(argv[2]);
    if (argc >= 4) T_priority = atof(argv[3]);
    if (argc >= 5) duration_sec = atoi(argv[4]);
    if (N < 2) N = 2;
    if (F < 2) F = 2;
    if (F >= N) F = N-1; 

    srand((unsigned)time(NULL));
    forks_available = F;

    ph_cv = (condpair_t*)malloc(sizeof(condpair_t)*N);
    waiting = (int*)calloc(N,sizeof(int));
    granted = (int*)calloc(N,sizeof(int));
    req_time = (double*)calloc(N,sizeof(double));
    eats = (int*)calloc(N,sizeof(int));
    wait_sum = (double*)calloc(N,sizeof(double));
    wait_max = (double*)calloc(N,sizeof(double));
    active = (int*)calloc(N,sizeof(int));
    join_at_sec = (double*)calloc(N,sizeof(double));
    leave_at_sec = (double*)calloc(N,sizeof(double));
    q_init(4*N+8);

    join_at_sec[0] = 5.0;        
    if (N > 3) leave_at_sec[3] = 30.0;

    for (int i=0;i<N;i++){
        pthread_mutex_init(&ph_cv[i].m, NULL);
        pthread_cond_init(&ph_cv[i].c, NULL);
    }

    pthread_t W; pthread_create(&W, NULL, waiter_thread, NULL);

    pthread_t *P = (pthread_t*)malloc(sizeof(pthread_t)*N);
    for (long i=0;i<N;i++) pthread_create(&P[i], NULL, philosopher, (void*)i);
    while (since_start() < duration_sec) msleep(200);

    running = 0;
    pthread_mutex_lock(&M);
    pthread_cond_broadcast(&waiter_cv);
    for (int i=0;i<N;i++) pthread_cond_broadcast(&ph_cv[i].c);
    pthread_mutex_unlock(&M);

    for (int i=0;i<N;i++) pthread_join(P[i], NULL);
    pthread_join(W, NULL);

    for (int i=0;i<N;i++){
        pthread_mutex_destroy(&ph_cv[i].m);
        pthread_cond_destroy(&ph_cv[i].c);
    }
    free(P); free(Q);
    free(ph_cv); free(waiting); free(granted); free(req_time);
    free(eats); free(wait_sum); free(wait_max); free(active);
    free(join_at_sec); free(leave_at_sec);
}