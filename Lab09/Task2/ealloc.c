// ealloc.c
#include "ealloc.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* macOS uses MAP_ANON instead of MAP_ANONYMOUS; make it portable */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define PAGE_SIZE 4096
#define MAX_PAGES 16   /* configurable cap */

/* free node inside a page */
typedef struct FreeNode {
    size_t offset;
    size_t size;
    struct FreeNode *next;
} FreeNode;

/* page descriptor */
typedef struct PageDesc {
    char *base;          /* mmap'd page base */
    FreeNode *free_list; /* free blocks inside this page */
    struct PageDesc *next;
} PageDesc;

/* allocation record */
typedef struct AllocRec {
    char *ptr;
    size_t size;
    PageDesc *page;
    struct AllocRec *next;
} AllocRec;

static PageDesc *pages = NULL;
static AllocRec *allocs = NULL;
static int num_pages = 0;

int einit_alloc(void) {
    pages = NULL;
    allocs = NULL;
    num_pages = 0;
    return 0;
}

static void add_free_in_page(PageDesc *p, size_t offset, size_t size) {
    FreeNode **pp = &p->free_list;
    while (*pp && (*pp)->offset < offset) pp = &(*pp)->next;
    FreeNode *node = (FreeNode*)malloc(sizeof(FreeNode));
    if (!node) {
        perror("malloc");
        return;
    }
    node->offset = offset;
    node->size = size;
    node->next = *pp;
    *pp = node;

    /* coalesce adjacent free nodes */
    FreeNode *cur = p->free_list;
    while (cur && cur->next) {
        if (cur->offset + cur->size == cur->next->offset) {
            FreeNode *tmp = cur->next;
            cur->size += tmp->size;
            cur->next = tmp->next;
            free(tmp);
        } else cur = cur->next;
    }
}

static PageDesc* create_new_page(void) {
    if (num_pages >= MAX_PAGES) return NULL;
    char *base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (base == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    PageDesc *p = (PageDesc*)malloc(sizeof(PageDesc));
    if (!p) {
        perror("malloc");
        munmap(base, PAGE_SIZE);
        return NULL;
    }
    p->base = base;
    p->free_list = NULL;
    p->next = pages;
    pages = p;
    add_free_in_page(p, 0, PAGE_SIZE);
    num_pages++;
    return p;
}

static AllocRec* find_allocrec(char *ptr, AllocRec **prev_out) {
    AllocRec *prev = NULL;
    AllocRec *cur = allocs;
    while (cur) {
        if (cur->ptr == ptr) {
            if (prev_out) *prev_out = prev;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

char *ealloc_mem(int size) {
    if (size <= 0) return NULL;
    if (size % 256 != 0) return NULL; /* per lab: multiples of 256 */
    if ((size_t)size > PAGE_SIZE) return NULL;

    PageDesc *p = pages;
    while (p) {
        FreeNode *prev = NULL;
        FreeNode *cur = p->free_list;
        while (cur) {
            if (cur->size >= (size_t)size) {
                size_t off = cur->offset;
                if (cur->size == (size_t)size) {
                    if (prev) prev->next = cur->next;
                    else p->free_list = cur->next;
                    free(cur);
                } else {
                    cur->offset += size;
                    cur->size -= size;
                }
                AllocRec *ar = (AllocRec*)malloc(sizeof(AllocRec));
                if (!ar) {
                    perror("malloc");
                    add_free_in_page(p, off, size);
                    return NULL;
                }
                ar->ptr = p->base + off;
                ar->size = size;
                ar->page = p;
                ar->next = allocs;
                allocs = ar;
                return ar->ptr;
            }
            prev = cur;
            cur = cur->next;
        }
        p = p->next;
    }

    /* create a new page on demand */
    PageDesc *newp = create_new_page();
    if (!newp) return NULL;
    FreeNode *f = newp->free_list;
    if (!f || f->size < (size_t)size) return NULL;
    size_t off = f->offset;
    if (f->size == (size_t)size) {
        FreeNode *tmp = f;
        newp->free_list = f->next;
        free(tmp);
    } else {
        f->offset += size;
        f->size -= size;
    }
    AllocRec *ar = (AllocRec*)malloc(sizeof(AllocRec));
    if (!ar) {
        perror("malloc");
        add_free_in_page(newp, off, size);
        return NULL;
    }
    ar->ptr = newp->base + off;
    ar->size = size;
    ar->page = newp;
    ar->next = allocs;
    allocs = ar;
    return ar->ptr;
}

void edealloc_mem(char *ptr) {
    if (!ptr) return;
    AllocRec *prev = NULL;
    AllocRec *found = find_allocrec(ptr, &prev);
    if (!found) return;
    PageDesc *p = found->page;
    size_t off = (size_t)(found->ptr - p->base);
    size_t size = found->size;
    if (prev) prev->next = found->next;
    else allocs = found->next;
    free(found);
    add_free_in_page(p, off, size);
}

int ecleanup_alloc(void) {
    AllocRec *ar = allocs;
    while (ar) {
        AllocRec *t = ar;
        ar = ar->next;
        free(t);
    }
    allocs = NULL;

    PageDesc *p = pages;
    while (p) {
        FreeNode *fn = p->free_list;
        while (fn) {
            FreeNode *t = fn;
            fn = fn->next;
            free(t);
        }
        PageDesc *tp = p;
        p = p->next;
        /* We keep mmap'd pages per assignment note. If you want to unmap them,
           call munmap(tp->base, PAGE_SIZE) here before free(tp). */
        free(tp);
    }
    pages = NULL;
    num_pages = 0;
    return 0;
}