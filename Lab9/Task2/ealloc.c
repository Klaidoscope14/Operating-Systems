#define _POSIX_C_SOURCE 200112L
#include "ealloc.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 16   // safe upper cap; lab example mentions up to 4 pages but code supports more

typedef struct FreeNode {
    size_t offset;
    size_t size;
    struct FreeNode *next;
} FreeNode;

typedef struct PageDesc {
    char *base;          // mmap'd page base
    FreeNode *free_list; // free blocks inside this page
    struct PageDesc *next;
} PageDesc;

/* Track which allocation belongs to which page */
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
    if (!node) return;
    node->offset = offset;
    node->size = size;
    node->next = *pp;
    *pp = node;
    // coalesce within page
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
    if (base == MAP_FAILED) return NULL;
    PageDesc *p = (PageDesc*)malloc(sizeof(PageDesc));
    if (!p) {
        munmap(base, PAGE_SIZE);
        return NULL;
    }
    p->base = base;
    p->free_list = NULL;
    p->next = pages;
    pages = p;
    // initial free block = entire page
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
    // per lab: sizes multiples of 256; enforce it (optional)
    if (size % 256 != 0) return NULL;
    if (size > PAGE_SIZE) return NULL;

    // search pages for a free block
    PageDesc *p = pages;
    while (p) {
        FreeNode *prev = NULL;
        FreeNode *cur = p->free_list;
        while (cur) {
            if (cur->size >= (size_t)size) {
                size_t off = cur->offset;
                if (cur->size == (size_t)size) {
                    // remove node
                    if (prev) prev->next = cur->next;
                    else p->free_list = cur->next;
                    free(cur);
                } else {
                    cur->offset += size;
                    cur->size -= size;
                }
                // record alloc
                AllocRec *ar = (AllocRec*)malloc(sizeof(AllocRec));
                if (!ar) {
                    // rollback: add free back
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

    // no existing page had space -> create new page on demand
    PageDesc *newp = create_new_page();
    if (!newp) return NULL;
    // allocate from start of new page
    FreeNode *f = newp->free_list;
    if (!f || f->size < (size_t)size) return NULL; // should not happen
    size_t off = f->offset;
    if (f->size == (size_t)size) {
        // remove f
        FreeNode *tmp = f;
        newp->free_list = f->next;
        free(tmp);
    } else {
        f->offset += size;
        f->size -= size;
    }
    AllocRec *ar = (AllocRec*)malloc(sizeof(AllocRec));
    if (!ar) {
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
    // remove alloc record
    if (prev) prev->next = found->next;
    else allocs = found->next;
    free(found);
    // add free in that page and coalesce
    add_free_in_page(p, off, size);
}

int ecleanup_alloc(void) {
    // free metadata structures (but do not unmap pages per lab spec)
    AllocRec *ar = allocs;
    while (ar) {
        AllocRec *t = ar;
        ar = ar->next;
        free(t);
    }
    allocs = NULL;
    // free free_list nodes and page descriptors (but leave pages mapped)
    PageDesc *p = pages;
    while (p) {
        FreeNode *fn = p->free_list;
        while (fn) {
            FreeNode *t = fn;
            fn = fn->next;
            free(t);
        }
        PageDesc *tpage = p;
        p = p->next;
        // free the PageDesc (but not the mapped memory)
        free(tpage);
    }
    pages = NULL;
    num_pages = 0;
    return 0;
}