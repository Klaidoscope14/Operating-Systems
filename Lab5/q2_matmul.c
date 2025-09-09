#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static int **A, **B, **C;
static int M, K, N;

typedef struct { int i, j; } Cell;

int** alloc_matrix(int r, int c) {
    int **m = (int**)malloc(sizeof(int*) * r);
    if (!m) return NULL;
    m[0] = (int*)malloc(sizeof(int) * r * c);
    if (!m[0]) { free(m); return NULL; }
    for (int i = 1; i < r; i++) m[i] = m[0] + i * c;
    return m;
}
void free_matrix(int **m) {
    if (!m) return;
    free(m[0]);free(m);
}

void* worker(void* arg) {
    Cell *cell = (Cell*)arg;
    int i = cell->i, j = cell->j;
    long long sum = 0;
    for (int p = 0; p < K; p++) sum += (long long)A[i][p] * B[p][j];
    C[i][j] = (int)sum;
    free(cell);
    return NULL;
}

int main(void) {
    printf("Enter M K N: ");
    if (scanf("%d %d %d", &M, &K, &N) != 3 || M<=0 || K<=0 || N<=0) {
        fprintf(stderr, "Invalid sizes\n"); return 1;
    }

    A = alloc_matrix(M, K);
    B = alloc_matrix(K, N);
    C = alloc_matrix(M, N);
    if (!A || !B || !C) { fprintf(stderr, "Allocation failed\n"); return 1; }

    printf("Enter A (%d x %d) row-wise:\n", M, K);
    for (int i = 0; i < M; i++)
        for (int j = 0; j < K; j++)
            scanf("%d", &A[i][j]);

    printf("Enter B (%d x %d) row-wise:\n", K, N);
    for (int i = 0; i < K; i++)
        for (int j = 0; j < N; j++)
            scanf("%d", &B[i][j]);

    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * M * N);
    if (!threads) { fprintf(stderr, "Thread array alloc failed\n"); return 1; }

    int t = 0;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            Cell *cell = (Cell*)malloc(sizeof(Cell));
            if (!cell) { fprintf(stderr, "Arg alloc failed\n"); return 1; }
            cell->i = i; cell->j = j;
            int rc = pthread_create(&threads[t++], NULL, worker, cell);
            if (rc != 0) { fprintf(stderr, "pthread_create failed (%d)\n", rc); return 1; }
        }
    }
    for (int i = 0; i < t; i++) pthread_join(threads[i], NULL);

    printf("Result matrix C = A x B:\n");
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++)
            printf("%d%c", C[i][j], (j+1==N)?'\n':' ');
    }

    free(threads);
    free_matrix(A); free_matrix(B); free_matrix(C);
    return 0;
}