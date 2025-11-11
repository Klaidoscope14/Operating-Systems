#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static void clr_cloexec(int fd){
    int flags = fcntl(fd, F_GETFD);
    if(flags!=-1) fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC);
}

int main(int argc, char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s N M\n", argv[0]); return 1;}
    int N=atoi(argv[1]), M=atoi(argv[2]);
    if(N<=0 || M<=0){fprintf(stderr,"N,M > 0\n"); return 1;}

    int (*pipes)[2] = calloc(N, sizeof *pipes);
    pid_t *pids = calloc(N, sizeof *pids);
    if(!pipes || !pids){perror("calloc"); return 1;}

    for(int i=0;i<N;i++){
        if(pipe(pipes[i])==-1){perror("pipe"); return 1;}
    }

    for(int i=0;i<N;i++){
        // child should inherit write-end only
        clr_cloexec(pipes[i][1]);
        pid_t pid=fork();
        if(pid<0){perror("fork"); return 1;}
        if(pid==0){
            // child: close all read ends, except keep my write end
            for(int j=0;j<N;j++){
                close(pipes[j][0]);
                if(j!=i) close(pipes[j][1]);
            }
            char fdstr[16], idstr[16];
            snprintf(fdstr,sizeof(fdstr), "%d", pipes[i][1]);
            snprintf(idstr,sizeof(idstr), "%d", i+1);
            char *args[]={(char*)"journalist", fdstr, idstr, NULL};
            execvp("./journalist", args);
            perror("execvp"); _exit(127);
        } else {
            pids[i]=pid;
            close(pipes[i][1]); // editor only reads
        }
    }

    printf("Editor: Newsroom open. Waiting for %d articles from %d journalists.\n", M, N);
    int published=0;
    while(published < M){
        fd_set rfds; FD_ZERO(&rfds);
        int maxfd=-1;
        for(int i=0;i<N;i++){ FD_SET(pipes[i][0], &rfds); if(pipes[i][0]>maxfd) maxfd=pipes[i][0]; }
        int r=select(maxfd+1, &rfds, NULL, NULL, NULL);
        if(r<0){
            if(errno==EINTR) continue;
            perror("select"); break;
        }
        for(int i=0;i<N && published<M;i++){
            if(FD_ISSET(pipes[i][0], &rfds)){
                char buf[512]; ssize_t n=read(pipes[i][0], buf, sizeof(buf)-1);
                if(n>0){
                    buf[n]='\0';
                    published++;
                    printf("Editor: Published article! [%d/%d] -> \"%s\"\n", published, M, buf);
                }
            }
        }
    }

    // tell all journalists to exit
    for(int i=0;i<N;i++) kill(pids[i], SIGTERM);
    for(int i=0;i<N;i++){ close(pipes[i][0]); }

    for(int i=0;i<N;i++) waitpid(pids[i], NULL, 0);
    printf("Editor: Deadline met! Published %d articles. Newsroom closed.\n", M);
    free(pipes); free(pids);
    return 0;
}