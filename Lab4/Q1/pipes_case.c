#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(int argc, char **argv){
    int p1[2], p2[2];
    if(pipe(p1)==-1 || pipe(p2)==-1){perror("pipe"); return 1;}
    pid_t pid=fork();
    if(pid<0){perror("fork"); return 1;}

    if(pid==0){ // child
        close(p1[1]); close(p2[0]);
        char buf[1024]; ssize_t n=read(p1[0], buf, sizeof(buf)-1);
        if(n>0){
            buf[n]='\0';
            for(ssize_t i=0;i<n;i++){
                unsigned char c=buf[i];
                if(isalpha(c)) buf[i]= islower(c)? toupper(c): tolower(c);
            }
            write(p2[1], buf, strlen(buf));
        }
        close(p1[0]); close(p2[1]); _exit(0);
    }
    
    else{ // parent
        close(p1[0]); close(p2[1]);
        const char *msg = (argc>1)? argv[1] : "Hi There\n";
        write(p1[1], msg, strlen(msg));
        close(p1[1]);
        char out[1024]; ssize_t m=read(p2[0], out, sizeof(out)-1);
        if(m>0){ out[m]='\0'; printf("%s", out); }
        close(p2[0]); wait(NULL);
    }
    return 0;
}