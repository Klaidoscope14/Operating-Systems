#ifndef EALLOC_H
#define EALLOC_H

int einit_alloc(void);
int ecleanup_alloc(void);
char *ealloc_mem(int size);
void edealloc_mem(char *ptr);

#endif 