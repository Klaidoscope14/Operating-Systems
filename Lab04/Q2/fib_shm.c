#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_SEQUENCE 10
typedef struct {
    long fib_sequence[MAX_SEQUENCE];
    int sequence_size;
} shared_data;

int main(int argc, char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s n<=%d\n", argv[0], MAX_SEQUENCE); return 1;}
    char *e; long n=strtol(argv[1], &e, 10);
    if(*e!='\0' || n<0 || n>MAX_SEQUENCE){fprintf(stderr,"n must be 0..%d\n", MAX_SEQUENCE); return 1;}

    char name[64]; snprintf(name,sizeof(name),"/fib_shm_%d", getpid());
    int fd=shm_open(name, O_CREAT|O_RDWR, 0600);
    if(fd==-1){perror("shm_open"); return 1;}
    if(ftruncate(fd, sizeof(shared_data))==-1){perror("ftruncate"); return 1;}
    shared_data *sd=mmap(NULL, sizeof(shared_data), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(sd==MAP_FAILED){perror("mmap"); return 1;}
    sd->sequence_size=(int)n;

    pid_t pid=fork();
    if(pid<0){perror("fork"); return 1;}
    if(pid==0){
        // child writes sequence
        if(n>=1) sd->fib_sequence[0]=0;
        if(n>=2) sd->fib_sequence[1]=1;
        for(int i=2;i<n;i++) sd->fib_sequence[i]=sd->fib_sequence[i-1]+sd->fib_sequence[i-2];
        _exit(0);
    } 
    
    else {
        wait(NULL);
        for(int i=0;i<sd->sequence_size;i++){
            if(i) printf(" ");
            printf("%ld", sd->fib_sequence[i]);
        }
        if(n>0) puts("");
        munmap(sd, sizeof(shared_data));
        shm_unlink(name);
        close(fd);
    }
    return 0;
}