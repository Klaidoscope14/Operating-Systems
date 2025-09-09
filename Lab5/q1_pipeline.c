#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define N 50
#define BUF1_SIZE 10
#define BUF2_SIZE 10
#define NUM_PRODUCERS 2
#define NUM_PROCESSORS 2

typedef struct {
    int *data;
    int cap, head, tail, count;
    pthread_mutex_t m;
    pthread_cond_t not_full, not_empty;
} BoundedBuffer;

static BoundedBuffer buf1, buf2;
static int next_to_produce = 0;
static int processed_count = 0;
static int consumed_count = 0;

static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

static FILE *logf = NULL;

void bb_init(BoundedBuffer *b, int cap) {
    b->data = (int*)malloc(sizeof(int) * cap);
    b->cap = cap; b->head = b->tail = b->count = 0;
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->not_full, NULL);
    pthread_cond_init(&b->not_empty, NULL);
}
void bb_destroy(BoundedBuffer *b) {
    free(b->data);
    pthread_mutex_destroy(&b->m);
    pthread_cond_destroy(&b->not_full);
    pthread_cond_destroy(&b->not_empty);
}
void bb_put(BoundedBuffer *b, int x) {
    pthread_mutex_lock(&b->m);
    while (b->count == b->cap) pthread_cond_wait(&b->not_full, &b->m);
    b->data[b->tail] = x;
    b->tail = (b->tail + 1) % b->cap;
    b->count++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->m);
}
int bb_get(BoundedBuffer *b) {
    pthread_mutex_lock(&b->m);
    while (b->count == 0) pthread_cond_wait(&b->not_empty, &b->m);
    int x = b->data[b->head];
    b->head = (b->head + 1) % b->cap;
    b->count--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->m);
    return x;
}

void* producer(void* arg) {
    long id = (long)arg;
    for (;;) {
        int my_index;
        pthread_mutex_lock(&count_lock);
        if (next_to_produce >= N) {
            pthread_mutex_unlock(&count_lock);
            break;
        }
        my_index = next_to_produce++;
        pthread_mutex_unlock(&count_lock);

        int val = rand() % 100;
        bb_put(&buf1, val);

        pthread_mutex_lock(&print_lock);
        printf("Producer %ld produced: %d (total produced: %d)\n",
               id, val, my_index + 1);
        pthread_mutex_unlock(&print_lock);
        // (Optional) tiny sleep to interleave outputs
        // usleep(1000);
    }
    return NULL;
}

void* processor(void* arg) {
    long id = (long)arg;
    for (;;) {
        int x = bb_get(&buf1);
        if (x == -1) {
            // Sentinel: stop this processor.
            break;
        }
        int y = x * x;
        bb_put(&buf2, y);

        int now_processed;
        pthread_mutex_lock(&count_lock);
        now_processed = ++processed_count;
        pthread_mutex_unlock(&count_lock);

        pthread_mutex_lock(&print_lock);
        printf("Processor %ld processed %d -> %d (total processed: %d)\n",
               id, x, y, now_processed);
        pthread_mutex_unlock(&print_lock);
    }
    return NULL;
}

void* consumer(void* arg) {
    (void)arg;
    int *logarr = (int*)malloc(sizeof(int)*N);
    int idx = 0;
    while (1) {
        if (idx >= N) break; // consumed exactly N
        int y = bb_get(&buf2);
        logarr[idx++] = y;

        int now_consumed;
        pthread_mutex_lock(&count_lock);
        now_consumed = ++consumed_count;
        pthread_mutex_unlock(&count_lock);

        pthread_mutex_lock(&print_lock);
        printf("Consumer consumed: %d (total consumed: %d)\n",
               y, now_consumed);
        pthread_mutex_unlock(&print_lock);
    }
    // write log
    for (int i = 0; i < N; i++) fprintf(logf, "%d\n", logarr[i]);
    free(logarr);
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));
    logf = fopen("log.txt", "w");
    if (!logf) { perror("log.txt"); return 1; }

    bb_init(&buf1, BUF1_SIZE);
    bb_init(&buf2, BUF2_SIZE);

    pthread_t prod[NUM_PRODUCERS], proc[NUM_PROCESSORS], cons;

    for (long i = 1; i <= NUM_PRODUCERS; i++)
        pthread_create(&prod[i-1], NULL, producer, (void*)i);

    for (long i = 1; i <= NUM_PROCESSORS; i++)
        pthread_create(&proc[i-1], NULL, processor, (void*)i);

    pthread_create(&cons, NULL, consumer, NULL);

    // Wait producers, then inject sentinels for processors:
    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(prod[i], NULL);
    for (int s = 0; s < NUM_PROCESSORS; s++) bb_put(&buf1, -1);

    for (int i = 0; i < NUM_PROCESSORS; i++) pthread_join(proc[i], NULL);
    pthread_join(cons, NULL);

    pthread_mutex_lock(&print_lock);
    printf("=== Simulation Complete ===\n");
    printf("Produced: %d | Processed: %d | Consumed: %d\n",
           next_to_produce, processed_count, consumed_count);
    printf("Results written to log.txt\n");
    pthread_mutex_unlock(&print_lock);

    bb_destroy(&buf1);
    bb_destroy(&buf2);
    fclose(logf);
    return 0;
}