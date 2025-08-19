// q4.c — parent "scheduler" controlling 3 children with SIGSTOP/SIGCONT
#define _XOPEN_SOURCE 700
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>   
#include <sys/select.h>
#include <string.h>

static void child_even(void){
    raise(SIGSTOP); // wait to be scheduled
    for (int i = 0; i <= 16; i += 2) {
        printf("[P1 - Even] %d\n", i);
        fflush(stdout);
        sleep(1);
    }
    _exit(0);
}
static void child_odd(void){
    raise(SIGSTOP);
    for (int i = 1; i < 16; i += 2) {
        printf("[P2 - Odd] %d\n", i);
        fflush(stdout);
        sleep(1);
    }
    _exit(0);
}
static void child_chars(void){
    raise(SIGSTOP);
    for (char c = 'A'; c <= 'J'; ++c) {
        printf("[P3 - Char] %c\n", c);
        fflush(stdout);
        sleep(1);
    }
    _exit(0);
}

/* Simple circular queue for PIDs */
#define QMAX 32
typedef struct { pid_t a[QMAX]; int h, t, n; } Q;
static void q_init(Q *q){ q->h = q->t = q->n = 0; }
static int  q_empty(Q *q){ return q->n == 0; }
static void q_push(Q *q, pid_t x){ if(q->n<QMAX){ q->a[q->t]=x; q->t=(q->t+1)%QMAX; q->n++; } }
static pid_t q_pop(Q *q){ if(q->n==0) return -1; pid_t x=q->a[q->h]; q->h=(q->h+1)%QMAX; q->n--; return x; }

int main(void){
    pid_t p1 = fork();
    if (p1 == 0) child_even();

    int st;
    waitpid(p1, &st, WUNTRACED); // ensure stopped

    pid_t p2 = fork();
    if (p2 == 0) child_odd();
    waitpid(p2, &st, WUNTRACED);

    pid_t p3 = fork();
    if (p3 == 0) child_chars();
    waitpid(p3, &st, WUNTRACED);

    Q ready; q_init(&ready);
    q_push(&ready, p1); q_push(&ready, p2); q_push(&ready, p3);

    int alive = 3;

    while (alive > 0 && !q_empty(&ready)) {
        pid_t pid = q_pop(&ready);
        if (pid <= 0) break;

        printf("=== Press Enter to BLOCK the process (or let it finish) ===\n");
        fflush(stdout);

        kill(pid, SIGCONT);

        for (;;) {
            // 1) Check if child exited
            int status;
            pid_t r = waitpid(pid, &status, WNOHANG);
            if (r == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    printf("[PID %d] Finished\n", (int)pid);
                    fflush(stdout);
                    alive--;
                    break; // move to next ready PID
                }
            }

            // 2) Poll stdin for Enter (non-blocking via select)
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            struct timeval tv; tv.tv_sec = 1; tv.tv_usec = 0; // 1s tick
            int sel = select(STDIN_FILENO+1, &rfds, NULL, NULL, &tv);

            if (sel > 0 && FD_ISSET(STDIN_FILENO, &rfds)) {
                // drain the line
                int ch = getchar();
                while (ch != '\n' && ch != EOF) ch = getchar();

                // stop the child and re-enqueue
                kill(pid, SIGSTOP);
                waitpid(pid, &status, WUNTRACED); // ensure actually stopped
                printf("Scheduler: PID %d blocked, re-enqueued\n", (int)pid);
                fflush(stdout);
                q_push(&ready, pid);
                break;
            }
            // else: timeout → loop again to give child more time to run
        }
    }

    // Reap any remaining children (safety)
    while (alive > 0) {
        pid_t r = wait(&st);
        if (r <= 0) break;
        alive--;
    }

    puts("All processes finished.");
    return 0;
}