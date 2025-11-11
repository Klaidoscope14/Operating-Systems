// test_alloc.c
#include <stdio.h>
#include <string.h>
#include "alloc.h"

int main(void) {
    if (init_alloc() != 0) {
        fprintf(stderr, "init failed\n");
        return 1;
    }

    char *a = alloc_mem(64);
    char *b = alloc_mem(128);
    char *c = alloc_mem(256);
    if (!a || !b || !c) {
        fprintf(stderr, "alloc failed\n");
        cleanup_alloc();
        return 1;
    }

    strcpy(a, "hello from a");
    sprintf(b, "b-size:%d", 128);
    printf("%s\n", a);
    printf("%s\n", b);

    dealloc_mem(b);
    // allocate a smaller block which should fit into freed b area
    char *d = alloc_mem(64);
    if (d) printf("allocated d at %p\n", (void*)d);

    dealloc_mem(a);
    dealloc_mem(c);
    dealloc_mem(d);

    if (cleanup_alloc() != 0) {
        fprintf(stderr, "cleanup failed\n");
        return 1;
    }
    printf("Done\n");
    return 0;
}