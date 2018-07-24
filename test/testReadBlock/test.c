/*
 * test if read blocks or not.
 * receive as input the flag block/no-block
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include <unistd.h>
#include <fcntl.h>


#include "../../const.h"

#define N 1
#define M 1
static int blocking = 1;
static int fd;

void *write_fun(void *args) {

    long unsigned int  id = (long unsigned int ) args;
    char message[32];
//    printf("Thread w%d writing\n", id);
    int i, ret;
    scanf("%s", message);
    ret = pwrite(fd, message, strlen(message), 0);
    if (ret<0){
        printf("some error occurred\n");
    }
    else{
//        printf("written %d bytes\n", ret);
    }

}

void *read_fun(void* args) {
//    int fd = *(int*)args;
    long unsigned int id = (long unsigned int ) args;
//    printf("Thread r%d reading\n", id);
    char data[512];
    int i, ret;
    for (i=0; i<M; i++) {
        ret=pread(fd, data, 512, 0);
        if (ret<0)
            printf("some error occurred: %d\n", ret);
        else {
            data[ret] = '\0';
//            printf("%d %s\n", ret, data);
            printf("%s\n", data);

        }
    }
}


int main(int argc, char const *argv[]) {
    int i;
    long unsigned int k;

    if (argc > 1){
        blocking = strtol(argv[1], NULL, 10);
    }

    if (blocking) fd = open("/dev/mailslot0", O_RDWR & ~O_NONBLOCK);
    else fd = open("/dev/mailslot0", O_RDWR | O_NONBLOCK);

    ioctl(fd, MAILSLOT_IOCTWRITEBLOCK, 1);
    ioctl(fd, MAILSLOT_IOCTREADBLOCK, 1);

    if(fd < 0)
        return 1;

    pthread_t threads_write[N];
    pthread_t threads_read[N];

    for (i=0; i<N; i++) {
        k = i;
        if (pthread_create(&threads_read[i], NULL, read_fun, (void *) k)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    for (i=0; i<N; i++) {
        k = i;
        if(pthread_create(&threads_write[i], NULL, write_fun, (void*)k)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }

    }


    for (i=0; i<N; i++) {
         k = i;


        if(pthread_join(threads_read[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }

        if(pthread_join(threads_write[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
    }


    close(fd);
    return 0;
}