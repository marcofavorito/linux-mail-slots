#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define NO_THREADS 256
#define NO_INC 10000

static int no_inc = 0;
static int no_threads = 0;

int non_atomic = 0;
_Atomic int num = 0;
_Atomic int barrier = 1;

void *th_fun(void *args) {
    while(barrier) continue;
    for (int i=0; i<no_inc; i++){
        ++num;
        ++non_atomic;
    }
}


int main(int argc, char* argv[]){
//    printf("%d %d\n", sizeof(int), sizeof(void*));
//    char ciao[30];
//    while(scanf("%s", ciao)>0) printf("%s\n", ciao);
    no_threads = strtol(argv[1],NULL,10);
    no_inc = strtol(argv[2], NULL, 10);

    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t)*no_threads);
    for (int i=0; i<no_threads;i++){
        pthread_create(&threads[i], NULL, th_fun, NULL);
    }

    barrier = 0;

    for (int i=0; i<no_threads;i++){
        pthread_join(threads[i], NULL);
    }
    free(threads);
    printf("%d, %d\n", num, non_atomic);
    return 0;
}