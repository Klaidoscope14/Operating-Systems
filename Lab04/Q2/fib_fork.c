#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s n\n", argv[0]); return 1;}
    char *e; long n=strtol(argv[1], &e, 10);
    if(*e!='\0' || n<0){fprintf(stderr,"n must be non-negative\n"); return 1;}

    pid_t pid=fork();
    if(pid<0){perror("fork"); return 1;}
    
    if(pid==0){
        long long a=0,b=1;
        for(long i=0;i<n;i++){
            if(i==0) printf("%lld", a);
            else if(i==1) printf(" %lld", b);
            else { long long c=a+b; a=b; b=c; printf(" %lld", b); }
        }
        if(n>0) puts("");
        _exit(0);
    } 
    
    else {
        int st; waitpid(pid,&st,0);
    }
    return 0;
}
