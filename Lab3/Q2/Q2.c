#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void) {
    printf("PREFORK");  

    pid_t child1 = fork();
    if (child1 < 0) { 
        perror("fork"); 
        return 1; 
    }

    int is_parent = 0, is_child1 = 0, is_child2 = 0;
    pid_t child2 = -1;

    if (child1 == 0) {
        is_child1 = 1;              // Child 1
    } 
    
    else {
        // Only the parent creates Child 2
        child2 = fork();
        if (child2 < 0) { perror("fork"); return 1; }
        else if (child2 == 0) {
            is_child2 = 1;          // Child 2
        } 
        
        else {
            is_parent = 1;          // Parent
        }
    }

    const char *role = is_parent ? "PARENT" : (is_child1 ? "CHILD 1" : "CHILD 2");

    for (int i = 1; i <= 3; ++i) {
        printf("Message %d from %s | PID: %d | PPID: %d\n",
               i, role, (int)getpid(), (int)getppid());
        fflush(NULL);
        sleep(1);
    }

    if (is_parent) {
        int status;
        waitpid(child1, &status, 0);
        waitpid(child2, &status, 0);
        puts("Both children finished execution");
    }
    return 0;
}