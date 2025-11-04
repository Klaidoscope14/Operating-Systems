#ifndef ALLOC_H
#define ALLOC_H

int init_alloc(void);
int cleanup_alloc(void);
char *alloc_mem(int size);
void dealloc_mem(char *ptr);

#endif 