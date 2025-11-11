#include <sys/types.h>  
#include <stdio.h>
#include <unistd.h>     

int main(void) {
    pid_t pid = getpid();
    printf("PID: %d\n", pid);
    printf("Sleeping for 60 seconds. Inspect me from another terminal...\n");
    fflush(NULL);
    sleep(60);
    printf("Done.\n");
    return 0;
}