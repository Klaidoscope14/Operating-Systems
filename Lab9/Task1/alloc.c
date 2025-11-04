#include "alloc.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define PAGE_SIZE 4096

/* Metadata kept outside the 4KB page */

/* Free block node */
typedef struct FreeNode {
    size_t offset;           // offset from page_base
    size_t size;             // size of free block
    struct FreeNode *next;
} FreeNode;

/* Allocated block record */
typedef struct AllocNode {
    char *ptr;              // actual pointer returned to user (page_base + offset)
    size_t size;
    struct AllocNode *next;
} AllocNode;

/* Globals */
static char *page_base = NULL;    // mmap'd 4KB page
static FreeNode *free_list = NULL;
static AllocNode *alloc_list = NULL;

/* Helpers */
static void add_free_block(size_t offset, size_t size) {
    // insert into free_list sorted by offset (ascending)
    FreeNode **pp = &free_list;
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

    // coalesce adjacent free nodes
    FreeNode *cur = free_list;
    while (cur && cur->next) {
        if (cur->offset + cur->size == cur->next->offset) {
            FreeNode *tmp = cur->next;
            cur->size += tmp->size;
            cur->next = tmp->next;
            free(tmp);
        } else {
            cur = cur->next;
        }
    }
}

static void remove_free_node(FreeNode *prev, FreeNode *node) {
    if (!node) return;
    if (prev) prev->next = node->next;
    else free_list = node->next;
    free(node);
}

static AllocNode* find_alloc_node(char *ptr, AllocNode **prev_out) {
    AllocNode *prev = NULL;
    AllocNode *cur = alloc_list;
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

/* API implementations */

int init_alloc(void) {
    if (page_base) {
        // already initialized
        return 0;
    }
    page_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (page_base == MAP_FAILED) {
        perror("mmap");
        page_base = NULL;
        return -1;
    }
    // initial free list: entire page
    free_list = (FreeNode*)malloc(sizeof(FreeNode));
    if (!free_list) {
        perror("malloc");
        munmap(page_base, PAGE_SIZE);
        page_base = NULL;
        return -1;
    }
    free_list->offset = 0;
    free_list->size = PAGE_SIZE;
    free_list->next = NULL;
    alloc_list = NULL;
    return 0;
}

int cleanup_alloc(void) {
    if (!page_base) return 0;
    // free metadata lists
    FreeNode *fn = free_list;
    while (fn) {
        FreeNode *t = fn;
        fn = fn->next;
        free(t);
    }
    free_list = NULL;
    AllocNode *an = alloc_list;
    while (an) {
        AllocNode *t = an;
        an = an->next;
        free(t);
    }
    alloc_list = NULL;

    if (munmap(page_base, PAGE_SIZE) != 0) {
        perror("munmap");
        page_base = NULL;
        return -1;
    }
    page_base = NULL;
    return 0;
}

char *alloc_mem(int size) {
    if (!page_base) {
        // not initialized
        return NULL;
    }
    if (size <= 0) return NULL;
    if (size % 8 != 0) {
        // must be multiple of 8
        return NULL;
    }
    // find first-fit free block
    FreeNode *prev = NULL, *cur = free_list;
    while (cur) {
        if (cur->size >= (size_t)size) {
            // allocate from this block
            size_t alloc_offset = cur->offset;
            // If leftover >= 8, split. Otherwise consume entire block.
            if (cur->size == (size_t)size) {
                // exact consume
                remove_free_node(prev, cur);
            } else {
                cur->offset += size;
                cur->size -= size;
            }
            // record allocation
            AllocNode *an = (AllocNode*)malloc(sizeof(AllocNode));
            if (!an) {
                perror("malloc");
                // On failure to create metadata, we need to rollback allocation:
                // add free block back
                add_free_block(alloc_offset, size);
                return NULL;
            }
            an->ptr = page_base + alloc_offset;
            an->size = size;
            an->next = alloc_list;
            alloc_list = an;
            return an->ptr;
        }
        prev = cur;
        cur = cur->next;
    }
    // no free block large enough
    return NULL;
}

void dealloc_mem(char *ptr) {
    if (!page_base || !ptr) return;
    // ensure ptr inside page
    if (ptr < page_base || ptr >= page_base + PAGE_SIZE) return;
    AllocNode *prev = NULL;
    AllocNode *found = find_alloc_node(ptr, &prev);
    if (!found) {
        // not found; ignore
        return;
    }
    size_t offset = (size_t)(found->ptr - page_base);
    size_t size = found->size;
    // remove from alloc_list
    if (prev) prev->next = found->next;
    else alloc_list = found->next;
    free(found);
    // add to free_list and coalesce within add_free_block
    add_free_block(offset, size);
}