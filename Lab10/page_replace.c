#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PAGE_RANGE 10
#define REF_LEN 30

int rand_ref[REF_LEN];

int simulate_fifo(int frames) {
    int i, faults = 0, next = 0;

    int *frame = malloc(frames * sizeof(int));
    int *in_frame = malloc(PAGE_RANGE * sizeof(int));

    for (i = 0; i < PAGE_RANGE; i++) in_frame[i] = 0;
    for (i = 0; i < frames; i++) frame[i] = -1;

    for (i = 0; i < REF_LEN; i++) {
        int p = rand_ref[i];
        if (!in_frame[p]) {
            faults++;
            if (frame[next] != -1) in_frame[frame[next]] = 0;
            frame[next] = p;
            in_frame[p] = 1;
            next = (next + 1) % frames;
        }
    }

    free(frame);
    free(in_frame);
    return faults;
}

int simulate_lru(int frames) {
    int i, j, faults = 0, timec = 0;

    int *frame = malloc(frames * sizeof(int));
    int *time_stamp = malloc(frames * sizeof(int));

    for (i = 0; i < frames; i++) {
        frame[i] = -1;
        time_stamp[i] = 0;
    }

    for (i = 0; i < REF_LEN; i++) {
        int p = rand_ref[i];
        timec++;
        int found = -1;

        for (j = 0; j < frames; j++)
            if (frame[j] == p) { found = j; break; }

        if (found != -1) time_stamp[found] = timec;

        else {
            faults++;
            int empty = -1;

            for (j = 0; j < frames; j++)
                if (frame[j] == -1) { empty = j; break; }

            if (empty != -1) {
                frame[empty] = p;
                time_stamp[empty] = timec;
            } 
            
            else {
                int lru_idx = 0;
                for (j = 1; j < frames; j++)
                    if (time_stamp[j] < time_stamp[lru_idx]) lru_idx = j;
                frame[lru_idx] = p;
                time_stamp[lru_idx] = timec;
            }
        }
    }

    free(frame);
    free(time_stamp);
    return faults;
}

int main() {
    srand(time(NULL));
    for (int i = 0; i < REF_LEN; i++) rand_ref[i] = rand() % PAGE_RANGE;
    printf("Reference string:\n");

    for (int i = 0; i < REF_LEN; i++) printf("%d ", rand_ref[i]);
    printf("\n\nFrames | FIFO faults | LRU faults\n");

    printf("-------+-------------+------------\n");

    for (int frames = 1; frames <= 7; frames++) {
        int f_fifo = simulate_fifo(frames);
        int f_lru = simulate_lru(frames);
        printf("  %2d   |     %3d     |    %3d\n", frames, f_fifo, f_lru);
    }
}