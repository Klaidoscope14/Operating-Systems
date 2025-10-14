#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define N 5  // Number of philosophers and forks

sem_t forks[N];
sem_t mutex;
int state[N];  // 0 = thinking, 1 = hungry, 2 = eating
int fork_owned[N]; // which philosopher owns each fork
pthread_t philosophers[N];

void print_states() {
    printf("States: ");
    for (int i = 0; i < N; i++)
        printf("%d ", state[i]);
    printf("\n");
}

void *philosopher(void *num) {
    int id = *(int *)num;
    while (1) {
        printf("Philosopher %d is thinking...\n", id);
        sleep(rand() % 3 + 1); // thinking time
        state[id] = 1;
        printf("Philosopher %d is hungry.\n", id);

        // Grab left fork
        sem_wait(&forks[id]);
        fork_owned[id] = id;
        printf("Philosopher %d grabs fork %d (left)\n", id, id);

        sleep(1); // simulate delay between picking forks

        // Grab right fork
        sem_wait(&forks[(id + 1) % N]);
        fork_owned[(id + 1) % N] = id;
        printf("Philosopher %d grabs fork %d (right)\n", id, (id + 1) % N);

        // Eat
        state[id] = 2;
        printf("Philosopher %d starts eating...\n", id);
        sleep(rand() % 3 + 1);
        printf("Philosopher %d ends eating and releases forks %d (left) and %d (right)\n",
               id, id, (id + 1) % N);

        // Release forks
        sem_post(&forks[id]);
        sem_post(&forks[(id + 1) % N]);
        fork_owned[id] = -1;
        fork_owned[(id + 1) % N] = -1;
        state[id] = 0;
    }
    return NULL;
}

int detect_deadlock() {
    int hungry_count = 0;
    for (int i = 0; i < N; i++) {
        if (state[i] == 1)
            hungry_count++;
    }
    return hungry_count == N; // all hungry = deadlock
}

void recover_deadlock() {
    int victim = rand() % N;
    printf("\n[Parent detects DEADLOCK! Initiating recovery...]\n");
    printf("[Parent preempts Philosopher %d, forcing fork release]\n", victim);

    sem_post(&forks[victim]);
    sem_post(&forks[(victim + 1) % N]);
    fork_owned[victim] = -1;
    fork_owned[(victim + 1) % N] = -1;
    state[victim] = 0;
}

int main() {
    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        sem_init(&forks[i], 0, 1);
        fork_owned[i] = -1;
    }
    sem_init(&mutex, 0, 1);

    int ids[N];
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &ids[i]);
    }

    while (1) {
        sleep(5);
        if (detect_deadlock())
            recover_deadlock();
    }

    for (int i = 0; i < N; i++)
        pthread_join(philosophers[i], NULL);

    for (int i = 0; i < N; i++)
        sem_destroy(&forks[i]);
    sem_destroy(&mutex);

    return 0;
}