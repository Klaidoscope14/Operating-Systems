#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t resource;   // controls access to manuscript (readers use it for first/last)
sem_t rmutex;     // protects readcount
sem_t readTry;    // blocks readers when a writer is waiting
sem_t wmutex;     // protects writecount

int readcount = 0;
int writecount = 0;
int manuscript = 0;

int num_students, num_librarians;

void *student(void *arg) {
    int id = *(int*)arg; free(arg);
    while (1) {
        printf("[Student %d] wants to read.\n", id); fflush(stdout);

        sem_wait(&readTry);           // wait unless writers are wanting
        sem_wait(&rmutex);
        readcount++;
        if (readcount == 1) sem_wait(&resource); // first reader locks resource
        sem_post(&rmutex);
        sem_post(&readTry);

        // reading (critical section for read)
        printf("[Student %d] is READING (version %d).\n", id, manuscript); fflush(stdout);
        sleep(1);
        printf("[Student %d] finished READING.\n", id); fflush(stdout);

        sem_wait(&rmutex);
        readcount--;
        if (readcount == 0) sem_post(&resource); // last reader releases
        sem_post(&rmutex);

        sleep(1); // time between attempts
    }
    return NULL;
}

void *librarian(void *arg) {
    int id = *(int*)arg; free(arg);
    while (1) {
        printf("[Librarian %d] wants to write.\n", id); fflush(stdout);

        sem_wait(&wmutex);
        writecount++;
        if (writecount == 1) sem_wait(&readTry); // first writer blocks new readers
        sem_post(&wmutex);

        sem_wait(&resource); // exclusive access
        printf("[Librarian %d] is WRITING.\n", id); fflush(stdout);
        sleep(1);
        manuscript++;
        printf("[Librarian %d] finished WRITING (new version %d).\n", id, manuscript); fflush(stdout);
        sem_post(&resource);

        sem_wait(&wmutex);
        writecount--;
        if (writecount == 0) sem_post(&readTry); // last writer allows readers
        sem_post(&wmutex);

        sleep(1); // time between attempts
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <#students> <#librarians>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    num_students = atoi(argv[1]);
    num_librarians = atoi(argv[2]);

    sem_init(&resource, 0, 1);
    sem_init(&rmutex, 0, 1);
    sem_init(&readTry, 0, 1);
    sem_init(&wmutex, 0, 1);

    pthread_t *stud = malloc(sizeof(pthread_t) * num_students);
    pthread_t *lib  = malloc(sizeof(pthread_t) * num_librarians);

    for (int i = 0; i < num_students; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&stud[i], NULL, student, id);
    }
    for (int i = 0; i < num_librarians; ++i) {
        int *id = malloc(sizeof(int)); *id = i+1;
        pthread_create(&lib[i], NULL, librarian, id);
    }

    // join (will block forever until Ctrl+C)
    for (int i = 0; i < num_students; ++i) pthread_join(stud[i], NULL);
    for (int i = 0; i < num_librarians; ++i) pthread_join(lib[i], NULL);

    // cleanup never reached in normal run
    sem_destroy(&resource);
    sem_destroy(&rmutex);
    sem_destroy(&readTry);
    sem_destroy(&wmutex);
    free(stud); free(lib);
    return 0;
}