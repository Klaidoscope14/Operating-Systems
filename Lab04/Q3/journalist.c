#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t stop=0;
static void on_term(int s){ (void)s; stop=1; }

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  c;
    int stage; // 0=research, 1=write, 2=submit
    int jid;
    int story_no;
    int wfd;
    char topic[128];
    char article[256];
} State;

void *researcher(void *arg){
    State *S=(State*)arg;
    while(!stop){
        pthread_mutex_lock(&S->m);
        while(S->stage!=0 && !stop) pthread_cond_wait(&S->c, &S->m);
        if(stop){ pthread_mutex_unlock(&S->m); break; }
        S->story_no++;
        snprintf(S->topic,sizeof(S->topic),"Story %d from Journalist %d", S->story_no, S->jid);
        printf("Journalist %d, Researcher: Found topic \"%s\"\n", S->jid, S->topic);
        S->stage=1;
        pthread_cond_broadcast(&S->c);
        pthread_mutex_unlock(&S->m);
        usleep(50000);
    }
    return NULL;
}

void *writer(void *arg){
    State *S=(State*)arg;
    while(!stop){
        pthread_mutex_lock(&S->m);
        while(S->stage!=1 && !stop) pthread_cond_wait(&S->c, &S->m);
        if(stop){ pthread_mutex_unlock(&S->m); break; }
        snprintf(S->article,sizeof(S->article),"Article on %s", S->topic);
        printf("Journalist %d, Writer: Writing article on \"%s\"\n", S->jid, S->topic);
        S->stage=2;
        pthread_cond_broadcast(&S->c);
        pthread_mutex_unlock(&S->m);
        usleep(50000);
    }
    return NULL;
}

void *submitter(void *arg){
    State *S=(State*)arg;
    while(!stop){
        pthread_mutex_lock(&S->m);
        while(S->stage!=2 && !stop) pthread_cond_wait(&S->c, &S->m);
        if(stop){ pthread_mutex_unlock(&S->m); break; }
        printf("Journalist %d, Submitter: Submitting article.\n", S->jid);
        dprintf(S->wfd, "%s\n", S->article);
        S->stage=0;
        pthread_cond_broadcast(&S->c);
        pthread_mutex_unlock(&S->m);
        usleep(50000);
    }
    return NULL;
}

int main(int argc, char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s write_fd journalist_id\n", argv[0]); return 1;}
    signal(SIGPIPE, SIG_IGN);
    struct sigaction sa={0}; sa.sa_handler=on_term; sigaction(SIGTERM,&sa,NULL);

    State S; memset(&S,0,sizeof S);
    pthread_mutex_init(&S.m,NULL); pthread_cond_init(&S.c,NULL);
    S.stage=0; S.wfd=atoi(argv[1]); S.jid=atoi(argv[2]); S.story_no=0;

    pthread_t tr, tw, ts;
    pthread_create(&tr,NULL,researcher,&S);
    pthread_create(&tw,NULL,writer,&S);
    pthread_create(&ts,NULL,submitter,&S);

    // wait for termination
    while(!stop) pause();

    pthread_mutex_lock(&S.m);
    pthread_cond_broadcast(&S.c);
    pthread_mutex_unlock(&S.m);

    pthread_join(tr,NULL); pthread_join(tw,NULL); pthread_join(ts,NULL);
    printf("Journalist %d: Received termination signal. Exiting.\n", S.jid);
    return 0;
}
