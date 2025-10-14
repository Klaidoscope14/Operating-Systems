#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#define P 5  // number of processes
#define R 3  // number of resources

int available[R] = {3, 3, 2};
int max[P][R] = {
    {7, 5, 3},
    {3, 2, 2},
    {9, 0, 2},
    {2, 2, 2},
    {4, 3, 3}
};
int allocation[P][R] = {
    {0, 1, 0},
    {2, 0, 0},
    {3, 0, 2},
    {2, 1, 1},
    {0, 0, 2}
};
int need[P][R];
pthread_mutex_t lock;

bool is_safe() {
    int work[R];
    bool finish[P] = {0};
    for (int i = 0; i < R; i++)
        work[i] = available[i];

    int count = 0;
    while (count < P) {
        bool found = false;
        for (int p = 0; p < P; p++) {
            if (!finish[p]) {
                int j;
                for (j = 0; j < R; j++)
                    if (need[p][j] > work[j])
                        break;
                if (j == R) {
                    for (int k = 0; k < R; k++)
                        work[k] += allocation[p][k];
                    finish[p] = true;
                    found = true;
                    count++;
                }
            }
        }
        if (!found)
            return false;
    }
    return true;
}

void *process_code(void *arg) {
    int p = *(int *)arg;
    sleep(rand() % 3 + 1);
    pthread_mutex_lock(&lock);
    printf("\nProcess %d making a request...\n", p);
    int req[R];
    for (int i = 0; i < R; i++)
        req[i] = rand() % (need[p][i] + 1);

    printf("Request: ");
    for (int i = 0; i < R; i++)
        printf("%d ", req[i]);
    printf("\n");

    bool can_allocate = true;
    for (int i = 0; i < R; i++) {
        if (req[i] > need[p][i] || req[i] > available[i]) {
            can_allocate = false;
            break;
        }
    }

    if (can_allocate) {
        for (int i = 0; i < R; i++) {
            available[i] -= req[i];
            allocation[p][i] += req[i];
            need[p][i] -= req[i];
        }

        if (is_safe()) {
            printf("System is safe. Request granted for Process %d.\n", p);
        } else {
            printf("System UNSAFE! Rolling back Process %d request.\n", p);
            for (int i = 0; i < R; i++) {
                available[i] += req[i];
                allocation[p][i] -= req[i];
                need[p][i] += req[i];
            }
        }
    } else {
        printf("Request cannot be granted to Process %d (exceeds limits).\n", p);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    pthread_t threads[P];
    srand(time(NULL));

    pthread_mutex_init(&lock, NULL);
    for (int i = 0; i < P; i++)
        for (int j = 0; j < R; j++)
            need[i][j] = max[i][j] - allocation[i][j];

    printf("Initial state ready. Starting processes...\n");

    int ids[P];
    for (int i = 0; i < P; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, process_code, &ids[i]);
    }

    for (int i = 0; i < P; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&lock);
    printf("\nSimulation complete.\n");
    return 0;
}