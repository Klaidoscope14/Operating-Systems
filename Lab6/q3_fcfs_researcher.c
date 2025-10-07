// q3_fcfs_researcher.c
// FCFS queue-based fairness with reader batching and Researchers (lowest priority).
// Usage: ./q3_fcfs_researcher <#students> <#librarians> <#researchers> <#accesses>
// Each thread performs <#accesses> then exits (finite runs).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

typedef enum { READER, WRITER, RESEARCHER } ThreadType;

typedef struct Node {
    ThreadType type;
    int id;               // id within its group for printing
    sem_t sem;            // per-node semaphore; thread waits on this to be allowed to proceed
    int awoken;           // 0 = not woken, 1 = woken
    struct Node *next;
} Node;

// Global queue head/tail and its lock
Node *qhead = NULL, *qtail = NULL;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

// Resource and reader-counting primitives
sem_t resource; // exclusive resource for writers (and first/last reader)
pthread_mutex_t rcount_lock = PTHREAD_MUTEX_INITIALIZER;
int readcount = 0;

int manuscript = 0;

int num_students, num_librarians, num_researchers, num_accesses;

// Helper: enqueue node and (if becomes head) wake appropriate node(s)
void enqueue_and_maybe_wake(Node *node) {
    pthread_mutex_lock(&qlock);
    node->next = NULL;
    node->awoken = 0;
    if (qtail == NULL) {
        // empty queue
        qhead = qtail = node;
        // wake head (and if reader, wake consecutive readers)
        node->awoken = 1;
        sem_post(&node->sem);
        if (node->type == READER) {
            // wake following readers if any (batch)
            Node *cur = node->next; // currently NULL; but future enqueues won't be head
            // nothing to do here because queue currently only has node
        }
    } else {
        // append
        qtail->next = node;
        qtail = node;
        // not head, so don't wake now
    }
    pthread_mutex_unlock(&qlock);
}

// Internal helper used by any thread when it finishes to pop its node and wake next head(s)
void pop_and_wake_next() {
    pthread_mutex_lock(&qlock);
    if (qhead == NULL) {
        pthread_mutex_unlock(&qlock);
        return;
    }
    // pop the head (it should correspond to the caller's node)
    Node *old = qhead;
    qhead = old->next;
    if (qhead == NULL) qtail = NULL;

    // free the popped node (caller should have finished using it)
    // But caller's thread may still want to refer to its node (we assume caller popped its own node after it's done).
    // We free old here.
    free(old);

    // If there is a new head, and it has not been awoken yet, wake it.
    if (qhead != NULL) {
        if (!qhead->awoken) {
            // If head is a READER, wake all consecutive readers (batch)
            if (qhead->type == READER) {
                Node *cur = qhead;
                while (cur != NULL && cur->type == READER) {
                    if (!cur->awoken) {
                        cur->awoken = 1;
                        sem_post(&cur->sem);
                    }
                    cur = cur->next;
                }
            } else {
                qhead->awoken = 1;
                sem_post(&qhead->sem);
            }
        }
    }
    pthread_mutex_unlock(&qlock);
}

// Each thread will create its node and enqueue, then wait on its node->sem to be allowed to proceed.
// For readers: batching is handled at enqueue time by the thread that becomes head (we wake head readers).
// For writers: exclusive access.
// Researchers are queued like others; FCFS ensures they run only when their turn comes.

void *student(void *arg) {
    int id = *(int*)arg; free(arg);

    for (int iter = 0; iter < num_accesses; ++iter) {
        // create node and enqueue
        Node *n = malloc(sizeof(Node));
        memset(n, 0, sizeof(Node));
        n->type = READER;
        n->id = id;
        sem_init(&n->sem, 0, 0);

        enqueue_and_maybe_wake(n);

        // Wait until queue grants us permission
        sem_wait(&n->sem); // when awoken, proceed to reading

        // Reader protocol (first reader locks resource)
        pthread_mutex_lock(&rcount_lock);
        readcount++;
        if (readcount == 1) sem_wait(&resource);
        pthread_mutex_unlock(&rcount_lock);

        // CRITICAL: reading
        printf("[Student %d] START READING (version %d)\n", id, manuscript); fflush(stdout);
        sleep(1);
        printf("[Student %d] END READING\n", id); fflush(stdout);

        // finish reader protocol
        pthread_mutex_lock(&rcount_lock);
        readcount--;
        if (readcount == 0) sem_post(&resource);
        pthread_mutex_unlock(&rcount_lock);

        // Now remove our node and wake next(s)
        pop_and_wake_next();

        sem_destroy(&n->sem); // safe to destroy after waking logic because thread done with sem
        // Note: we freeed node inside pop_and_wake_next; to avoid double-free, don't free here.

        sleep(1); // gap between attempts
    }
    return NULL;
}

void *librarian(void *arg) {
    int id = *(int*)arg; free(arg);

    for (int iter = 0; iter < num_accesses; ++iter) {
        Node *n = malloc(sizeof(Node));
        memset(n, 0, sizeof(Node));
        n->type = WRITER;
        n->id = id;
        sem_init(&n->sem, 0, 0);

        enqueue_and_maybe_wake(n);

        sem_wait(&n->sem); // wait for head grant

        // Writer protocol: exclusive access
        sem_wait(&resource);
        printf("[Librarian %d] START WRITING\n", id); fflush(stdout);
        sleep(1);
        manuscript++;
        printf("[Librarian %d] END WRITING (new version %d)\n", id, manuscript); fflush(stdout);
        sem_post(&resource);

        pop_and_wake_next();

        sem_destroy(&n->sem);
        sleep(1);
    }
    return NULL;
}

void *researcher(void *arg) {
    int id = *(int*)arg; free(arg);

    for (int iter = 0; iter < num_accesses; ++iter) {
        Node *n = malloc(sizeof(Node));
        memset(n, 0, sizeof(Node));
        n->type = RESEARCHER;
        n->id = id;
        sem_init(&n->sem, 0, 0);

        // Enqueue like others. FCFS queue ensures researchers wait their turn.
        enqueue_and_maybe_wake(n);

        sem_wait(&n->sem); // wait for head grant (only when no earlier requests are left and it's head)

        // Researcher acts as a writer but must have lowest priority already by queueing.
        sem_wait(&resource);
        printf("[Researcher %d] START WRITING (RESEARCH)\n", id); fflush(stdout);
        sleep(1);
        manuscript++;
        printf("[Researcher %d] END WRITING (RESEARCH) (new version %d)\n", id, manuscript); fflush(stdout);
        sem_post(&resource);

        pop_and_wake_next();

        sem_destroy(&n->sem);
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <#students> <#librarians> <#researchers> <#accesses>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    num_students = atoi(argv[1]);
    num_librarians = atoi(argv[2]);
    num_researchers = atoi(argv[3]);
    num_accesses = atoi(argv[4]);

    sem_init(&resource, 0, 1);

    pthread_t *students = malloc(sizeof(pthread_t) * num_students);
    pthread_t *librarians = malloc(sizeof(pthread_t) * num_librarians);
    pthread_t *researchers = malloc(sizeof(pthread_t) * num_researchers);

    for (int i = 0; i < num_students; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&students[i], NULL, student, id);
    }
    for (int i = 0; i < num_librarians; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&librarians[i], NULL, librarian, id);
    }
    for (int i = 0; i < num_researchers; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&researchers[i], NULL, researcher, id);
    }

    // wait for all threads
    for (int i = 0; i < num_students; ++i) pthread_join(students[i], NULL);
    for (int i = 0; i < num_librarians; ++i) pthread_join(librarians[i], NULL);
    for (int i = 0; i < num_researchers; ++i) pthread_join(researchers[i], NULL);

    printf("All threads completed. Final manuscript version: %d\n", manuscript);
    fflush(stdout);

    // cleanup
    sem_destroy(&resource);
    free(students); free(librarians); free(researchers);
    return 0;
}