#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "thread.h"
#include <fcntl.h>

struct task {
    // pthread_t tid;
    int tid;
    struct  {
        int in;//pipefd[0] read
        int out;//pipefd[1] write
    } pipe;
};
struct  semaphore {
    bool lock;
    int count;
    struct task thrs[1024];//max queue size
    int t_idx;//waiting queue tail
    void (*p) (struct semaphore *s,int tid);
    void (*v) (struct semaphore *s,int tid);

};
typedef struct semaphore *sem;//sem = semaphore *
bool TestAndSet(bool *target);
void acquire(sem s,int tid);
void release(sem s,int tid);
void Sleep(int fid);
void Dequeue(sem s);
void Destroy(sem s);
void *active(void *aegp);
static int REASONABLE_THREAD_MAX=500;


//initial semaphore
// int a; a=10
struct  semaphore global_sem= {
    .lock=false,
    .count=0,
    .p=acquire,
    .v=release,
};

int countNo;
pthread_mutex_t gLock;
int main(int argc, char const *argv[])
{
    sem s=&global_sem;//pointer to variable
    int nhijos;
    threads** threadS;
    pthread_t *tid;
    pthread_mutex_init(&gLock,NULL);



    countNo=0;
    if (argc > 1) {
        nhijos = atoi(argv[1]);
        if ((nhijos <= 0) || (nhijos > REASONABLE_THREAD_MAX)) {
            printf("invalid argument for thread count\n");
            exit(EXIT_FAILURE);
        }
        threadS=(threads**)malloc(sizeof(threads*)*nhijos);
        tid=(pthread_t*)malloc(sizeof(pthread_t)*nhijos);

        for (int i = 0; i < nhijos; ++i) {
            threadS[i] = (threads *)malloc(sizeof (threads));
            if(threadS[i]!=NULL) {
                threadS[i] ->no=0;
                threadS[i] ->tid=i;
            } else {
                printf("threadS=NULL\n");
                exit(1);
            }
            pipe((int *)&(s->thrs[i].pipe));//initial pipe
            fcntl(s->thrs[i].pipe.out, F_SETFL, O_NONBLOCK | O_WRONLY);//set pipe

        }
        for (int i = 0; i < nhijos; i++) {
            pthread_create(&tid[i], NULL, &active, threadS[i] );
        }
        for (int i = 0; i < nhijos; i++) {
            pthread_join( tid[i], NULL );
        }
    }
    pthread_mutex_destroy(&gLock);
}
void *active(void *aegp)
{
    sem s=&global_sem;//pointer to variable
    threads *arg=(threads *)aegp;
    int no=arg->no;
    int tid=arg->tid;

    s->p(s,tid); //acquire
    printf("count ~~~~~%d\n",s->count );
    //critical section
    countNo++;
    no=countNo;
    s->v(s,tid); //release
    pthread_exit(0);
}
//atomic: only one cpu(thread) can do TestAndSet

void acquire(sem s,int tid)
{
    int fid;
    int tmp;
    printf("b acquire %d\n",tid ); 
    while( __atomic_test_and_set(&s->lock,true))
        ;
    #if 0
    while( __sync_lock_test_and_set (&s->lock,true))
        ;
    #endif    
    printf("a acquire %d\n",tid );

    if (s->count >= 3) { //if only allow 2 threads to get source
        //put thread in the stack
        s->thrs[s->t_idx].tid=tid;
    tmp=s->t_idx;
    printf("Enqueue%d %d\n",s->t_idx,s->count);
    fid=s->thrs[s->t_idx].pipe.in;
    s->t_idx++;
    s->lock=false;
     //unlock before sleep,if don't do this and thread will sleep and nobody to unlock
        Sleep(fid);
    } else {
        s->count++;
        s->lock=false;
    }
}
void release(sem s,int tid)
{
    printf("b release %d\n",tid );
    while( __atomic_test_and_set(&s->lock,true))
        ;
     #if 0
    while( __sync_lock_test_and_set (&s->lock,true))
        ;
    #endif    
    printf("a release %d\n",tid );

    if(s->t_idx > 0) {   //if wait queue not empty
        Dequeue(s); //dequeue thread from wait queue
    } else {
        s->count--;
    }
    s->lock=false;
}
void Sleep(int fid)
{
    char signal_t='a';
    //block1);
    read(fid,&signal_t,1);
}
void Dequeue(sem s)
{
    char signal_t='a';
    s->t_idx--;
    write(s->thrs[s->t_idx].pipe.out,&signal_t,1);
    printf("Dequeue %d\n",s->t_idx );
}
