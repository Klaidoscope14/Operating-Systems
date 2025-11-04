#include <stdio.h>
#include <string.h>
#include "ealloc.h"

int main(void) {
    einit_alloc();

    char *a = ealloc_mem(256);
    char *b = ealloc_mem(512);
    char *c = ealloc_mem(1024);

    if (!a || !b || !c) {
        fprintf(stderr, "ealloc failed\n");
        return 1;
    }

    strcpy(a, "hello elastic");
    printf("%s\n", a);

    edealloc_mem(b);
    char *d = ealloc_mem(256); // should reuse freed 512->256 area
    if (d) printf("d allocated at %p\n", (void*)d);

    edealloc_mem(a);
    edealloc_mem(c);
    edealloc_mem(d);

    ecleanup_alloc();
    printf("elastic done\n");
    return 0;
}