#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct Block {
    int start;
    int size;
    int allocated;
    int id;
    struct Block *next;
} Block;

Block* make_block(int start, int size, int allocated, int id) {
    Block *b = malloc(sizeof(Block));
    b->start = start;
    b->size = size;
    b->allocated = allocated;
    b->id = id;
    b->next = NULL;
    return b;
}

void print_blocks(Block *head) {
    Block *p = head;
    printf("Memory map:\n");
    while (p) {
        if (p->allocated) printf("[Alloc id=%d size=%d] ", p->id, p->size);
        else printf("[Free size=%d] ", p->size);

        p = p->next;
    }
    printf("\n");
}

void merge_free(Block *head) {
    Block *p = head;
    while (p && p->next) {
        if (!p->allocated && !p->next->allocated) {
            p->size += p->next->size;
            Block *tmp = p->next;
            p->next = tmp->next;
            free(tmp);
        } 
        
        else p = p->next;
    }
}

int allocate_first_fit(Block *head, int size, int alloc_id) {
    Block *p = head;
    while (p) {
        if (!p->allocated && p->size >= size) {
            if (p->size == size) {
                p->allocated = 1;
                p->id = alloc_id;
            } 
            
            else {
                Block *newb = make_block(p->start + size, p->size - size, 0, -1);
                newb->next = p->next;
                p->next = newb;
                p->size = size;
                p->allocated = 1;
                p->id = alloc_id;
            }
            return 1;
        }
        p = p->next;
    }
    return 0;
}

void deallocate(Block *head, int alloc_id) {
    Block *p = head;
    while (p) {
        if (p->allocated && p->id == alloc_id) {
            p->allocated = 0;
            p->id = -1;
            break;
        }
        p = p->next;
    }
    merge_free(head);
}

int total_free(Block *head) {
    int sum = 0;
    Block *p = head;
    while (p) {
        if (!p->allocated) sum += p->size;
        p = p->next;
    }
    return sum;
}

int largest_free(Block *head) {
    int mx = 0;
    Block *p = head;
    while (p) {
        if (!p->allocated && p->size > mx) mx = p->size;
        p = p->next;
    }
    return mx;
}

int main() {
    srand(time(NULL));
    int total_mem, num_ops;
    printf("Enter total memory size (KB): ");
    if (scanf("%d", &total_mem) != 1) return 0;

    printf("Enter number of operations: ");
    if (scanf("%d", &num_ops) != 1) return 0;

    Block *head = make_block(0, total_mem, 0, -1);
    int next_alloc_id = 1;
    int *allocated_ids = malloc(sizeof(int) * num_ops);
    int allocated_count = 0;

    printf("\nOperations:\n");
    for (int op = 1; op <= num_ops; op++) {
        int do_alloc;

        if (allocated_count == 0) do_alloc = 1;
        else do_alloc = rand() % 2;

        if (do_alloc) {
            int max_size = total_mem / 2;
            if (max_size < 1) max_size = total_mem;
            int size = (rand() % max_size) + 1;

            printf("Operation %d: Allocate %d KB -> ", op, size);
            int ok = allocate_first_fit(head, size, next_alloc_id);

            if (ok) {
                printf("Allocated as id %d\n", next_alloc_id);
                allocated_ids[allocated_count++] = next_alloc_id;
                next_alloc_id++;
            } 
            
            else printf("Allocation failed\n");
        } 
        
        else {
            int idx = rand() % allocated_count;
            int aid = allocated_ids[idx];
            printf("Operation %d: Deallocate block %d -> ", op, aid);
            deallocate(head, aid);
            for (int k = idx; k < allocated_count - 1; k++)
                allocated_ids[k] = allocated_ids[k + 1];
            allocated_count--;
            printf("Done\n");
        }
        print_blocks(head);
    }

    int totalfree = total_free(head);
    int largest = largest_free(head);
    int external_frag = totalfree - largest;
    double frag_ratio = totalfree == 0 ? 0.0 : (double)external_frag / totalfree;

    printf("\nFinal Summary:\n");
    printf("Total free space: %d KB\n", totalfree);
    printf("Largest contiguous free block: %d KB\n", largest);
    printf("External fragmentation: %d KB\n", external_frag);
    printf("Fragmentation ratio: %.3f\n", frag_ratio);

    free(allocated_ids);
    Block *p = head;
    while (p) {
        Block *q = p->next;
        free(p);
        p = q;
    }
}