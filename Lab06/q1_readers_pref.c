#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

int manuscript_data = 0;
sem_t mutex; // protects readcount
sem_t wrt;   // allows writers exclusive access
int readcount = 0;

int num_students, num_librarians, num_accesses;

void *student(void *arg) {
    int id = *(int*)arg;
    free(arg);
    for (int k = 0; k < num_accesses; ++k) {
        printf("Student %d is waiting to read.\n", id); fflush(stdout);

        sem_wait(&mutex);
        readcount++;
        if (readcount == 1) sem_wait(&wrt); // first reader locks writers out
        sem_post(&mutex);

        printf("Student %d is now reading (version: %d).\n", id, manuscript_data); fflush(stdout);
        sleep(1); // simulate reading
        printf("Student %d has finished reading.\n", id); fflush(stdout);

        sem_wait(&mutex);
        readcount--;
        if (readcount == 0) sem_post(&wrt); // last reader releases writers
        sem_post(&mutex);

        sleep(1); // wait before next attempt
    }
    return NULL;
}

void *librarian(void *arg) {
    int id = *(int*)arg;
    free(arg);
    for (int k = 0; k < num_accesses; ++k) {
        printf("Librarian %d is waiting to write.\n", id); fflush(stdout);
        sem_wait(&wrt); // exclusive access
        printf("Librarian %d is now writing.\n", id); fflush(stdout);
        sleep(1); // simulate writing
        manuscript_data++;
        printf("Librarian %d has finished writing (new version: %d).\n", id, manuscript_data); fflush(stdout);
        sem_post(&wrt);
        sleep(1); // wait before next attempt
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <#students> <#librarians> <#accesses>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    num_students = atoi(argv[1]);
    num_librarians = atoi(argv[2]);
    num_accesses = atoi(argv[3]);

    sem_init(&mutex, 0, 1);
    sem_init(&wrt, 0, 1);

    pthread_t *students = malloc(sizeof(pthread_t) * num_students);
    pthread_t *librarians = malloc(sizeof(pthread_t) * num_librarians);

    for (int i = 0; i < num_students; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&students[i], NULL, student, id);
    }
    for (int i = 0; i < num_librarians; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&librarians[i], NULL, librarian, id);
    }

    for (int i = 0; i < num_students; ++i) pthread_join(students[i], NULL);
    for (int i = 0; i < num_librarians; ++i) pthread_join(librarians[i], NULL);

    printf("All threads have completed their tasks.\n");
    fflush(stdout);

    sem_destroy(&mutex);
    sem_destroy(&wrt);
    free(students);
    free(librarians);
    return 0;
}