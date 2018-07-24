#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "../../const.h"

#define NO_SAMPLES 8192
#define SAMPLE_RATE 1

static int write_samples[NO_SAMPLES];
static int read_samples[NO_SAMPLES];

static int no_samples = 0;

static int w_fds[256];
static int r_fds[256];

static pthread_t* threads_write;
static pthread_t* threads_read;
static pthread_attr_t* threads_write_attr;
static pthread_attr_t* threads_read_attr;


static unsigned long N_w = 0;
static unsigned long N_r = 0;
static unsigned long M_r = 0;
static unsigned long M_w = 0;
static int blocking_write = 1;
static int blocking_read = 1;
static int no_fd = 0;
static unsigned long msg_len = 0;
static unsigned long max_storage = 0;

static char* write_buf;
static char* read_buf;


_Atomic int writes = 0;
_Atomic int reads = 0;
_Atomic int not_blocked = 1;

_Atomic int barrier = 0;

void cleanup(void){
//    for (int i=0; i<N_w; i++) {
//        if(pthread_cancel(threads_write[i])){
//            fprintf(stderr, "Error canceling write thread\n");
//            return 2;
//        }
//    }
//
//    for (int i=0; i<N_r; i++) {
//        if(pthread_cancel(threads_read[i])){
//            fprintf(stderr, "Error canceling read thread\n");
//            return 2;
//        }
//    }

    free(threads_write);
    free(threads_read);
    free(threads_write_attr);
    free(threads_read_attr);

    for (int i=0; i<no_fd;i++){
        close(w_fds[i]);
        close(r_fds[i]);
    }

    for (int s=0; s<no_samples;s++){
        printf("%d\t%d\t%d\n", s, write_samples[s], read_samples[s]);
    }

    free(write_buf);
    free(read_buf);


}

void handle_signal(int signal){
    // Find out which signal we're handling
    switch (signal) {
        case SIGINT:
            #ifdef DEBUG
            printf("SIGINT handled\n");
            #endif
            cleanup();
            exit(0);
    }
}

void *write_fun(void *args) {
//    unsigned long int id = (unsigned long int) args;
//    char message[32];
//    printf("Thread w%lu writing\n", id);
    int i, fd, ret;
    while(barrier){}
    for (fd = 0; fd < no_fd; fd++) {
        for (i=0; i<M_w; i++) {
//            sprintf(message, "t_id:%lu n:%d", id, i);
//            ret = pwrite(fds[fd], message, strlen(message), 0);
            ret = pwrite(w_fds[fd], write_buf, msg_len, 0);
            if (errno==EAGAIN){
                #ifdef DEBUG
                printf("no memory: %d\n", ret);
                #endif
//		        return 1;
            }
            else if (ret<0){
                #ifdef DEBUG
                printf("some error occurred: %d, errno: %d\n", ret, errno);
                #endif
            }
            else{
                not_blocked = 1;
                ++writes;
//                printf("%lu wrote %d bytes\n", id, ret);
            }
        }
    }
}

void *read_fun(void* args) {

    unsigned long int id = (unsigned long int) args;
//    printf("Thread r%lu reading\n", id);
    char data[msg_len];
    int i,fd, ret;
    while(barrier){}
    for (fd = 0; fd < no_fd; fd++){
        for (i=0; i<M_r; i++) {
            ret=pread(r_fds[fd], data, msg_len, 0);
            if (ret<0) {
                #ifdef DEBUG
                printf("some error occurred: %d, errno: %d\n", ret, errno);
                #endif
                }
            else {
                not_blocked = 1;
                ++reads;
                data[ret-1] = '\0';
//                printf("read %d bytes: %s\n", ret, data);
            }
        }
    }


}


void*sigint_fun(void * args){
    int ret;
    int cur_pid = *((int*)args);
//    printf("curpid %d\n", cur_pid);
    sleep(5);
    while(not_blocked){
        #ifdef DEBUG
        printf("not blocked\n");
        #endif
        not_blocked=0;
        sleep(5);
    }
    #ifdef DEBUG
    printf("kill called.\n");
    #endif
    ret = kill(cur_pid,SIGINT);
    return 0;
}


int main(int argc, char const *argv[]) {
    int i;
    int cur_writes=-1, cur_reads=-1;
    int cur_pid = getpid();
    unsigned long int k;
    char temp[32];


    if (argc == 10){
        N_w =               strtol(argv[1], NULL, 10);
        N_r =               strtol(argv[2], NULL, 10);
        M_w =               strtol(argv[3], NULL, 10);
        M_r =               strtol(argv[4], NULL, 10);
        blocking_write =    strtol(argv[5], NULL, 10);
        blocking_read =     strtol(argv[6], NULL, 10);
        no_fd =             strtol(argv[7], NULL, 10);
        msg_len =           strtol(argv[8], NULL, 10);
        max_storage =       strtol(argv[9], NULL, 10);
    }
    else{
        printf("Usage:\n"
                       "<num_writers>\n"
                       "<num_readers>\n"
                       "<num_writes>\n"
                       "<num_reads>\n"
                       "<blocking_write>\n"
                       "<blocking_read>\n"
                       "<num_fds>\n"
                       "<msg_len>\n"
                       "<max_storage>\n"
        );
        return -1;
    }

    #ifdef DEBUG
    printf("N_w=%lu N_r=%lu M_w=%lu M_r=%lu blocking_write=%d blocking_read=%d no_fd=%d msg_len=%lu max_storage=%lu\n",
                      N_w,N_r,M_w,M_r,blocking_write,blocking_read,no_fd,msg_len,max_storage);
    #endif

    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        printf("\ncan't catch SIGINT\n");
        return 1;
    }

    struct sched_param main_param;
    memset(&main_param, 0, sizeof(struct sched_param));
    sched_setparam(getpid(), &main_param);
//    int sched_pol = sched_getscheduler(0);
    int prio = sched_get_priority_max(SCHED_RR);
    main_param.sched_priority = prio;
//    printf("%d\n", prio);

    if (sched_setscheduler(getpid(), SCHED_RR, &main_param)){
        printf("Error in setting the scheduling policy: %s\n", strerror(errno));
        return 1;
    }

    memset(w_fds,0,sizeof(int)*256);
    memset(r_fds,0,sizeof(int)*256);

    read_buf = (char*) malloc(sizeof(char)*(msg_len));
    write_buf = (char*) malloc(sizeof(char)*(msg_len));
    memset(write_buf, 'a', msg_len);
    write_buf[msg_len-1] = '\0';



    memset(write_samples, 0, NO_SAMPLES*sizeof(int));
    memset(read_samples, 0, NO_SAMPLES*sizeof(int));

//    #ifdef DEBUG printf("message: %s\n", write_buf);

#ifdef DEBUG
    printf("Opening file descriptors...\n");
#endif
    for (i=0; i<no_fd; i++){
        sprintf(temp, "/dev/mailslot%d", i);
        if (N_w){
            if (blocking_write) w_fds[i] = open(temp, O_WRONLY & ~O_NONBLOCK);
            else w_fds[i] = open(temp, O_WRONLY | O_NONBLOCK);

            if(w_fds[i] < 0){
                printf("Problems with %s\n", temp);
                return 1;
            }
            else{
                ioctl(w_fds[i], MAILSLOT_IOCTMAXDATAUNITLEN, msg_len);
                ioctl(w_fds[i], MAILSLOT_IOCTMAXSTORAGE, max_storage);
            }
        }

        if(N_r){
            if (blocking_read) r_fds[i] = open(temp, O_RDONLY & ~O_NONBLOCK);
            else r_fds[i] = open(temp, O_RDONLY | O_NONBLOCK);

            if(r_fds[i] < 0){
                printf("Problems with %s\n", temp);
                return 1;
            }
            else{
                ioctl(r_fds[i], MAILSLOT_IOCTMAXDATAUNITLEN, msg_len);
                ioctl(r_fds[i], MAILSLOT_IOCTMAXSTORAGE, max_storage);
            }
        }


    }

#ifdef DEBUG
    printf("Creating threads...\n");
#endif

    threads_write = (pthread_t*) malloc(sizeof(pthread_t)*N_w);
    threads_read = (pthread_t*) malloc(sizeof(pthread_t)*N_r);
    memset(threads_write, 0, N_w*sizeof(pthread_t));
    memset(threads_read, 0, N_r*sizeof(pthread_t));


    threads_write_attr = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*N_w);
    threads_read_attr = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*N_r);
    memset(threads_write_attr, 0, N_w*sizeof(pthread_attr_t));
    memset(threads_read_attr, 0, N_r*sizeof(pthread_attr_t));

//    int pid; /* process identifier */
//    pid=fork();
//    if (pid < 0){printf("Cannot fork!!\n");exit(1);}
//    if (pid == 0)
//    {
//        #ifdef DEBUG
//        printf("Child process. Creating new threads\n");
//        #endif

    for (i=0; i<N_w; i++) {
        k = i;

//        /* initialized with default attributes */
//        pthread_attr_init(&threads_write_attr[i]);

        struct sched_param param;
        pthread_attr_setinheritsched(&threads_write_attr[i], PTHREAD_EXPLICIT_SCHED);
        pthread_attr_getschedparam(&threads_write_attr[i], &param);
        param.sched_priority = 40;
        pthread_attr_setschedpolicy(&threads_write_attr[i], SCHED_RR);
        pthread_attr_setschedparam(&threads_write_attr[i], &param);



        if(pthread_create(&threads_write[i], &threads_write_attr[i], write_fun, (void*)k)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }

    for (i=0; i<N_r; i++) {
        k = i;

        struct sched_param param;
        pthread_attr_setinheritsched(&threads_read_attr[i], PTHREAD_EXPLICIT_SCHED);
        pthread_attr_getschedparam(&threads_read_attr[i], &param);
        param.sched_priority = 40;
        pthread_attr_setschedpolicy(&threads_read_attr[i], SCHED_RR);
        pthread_attr_setschedparam(&threads_read_attr[i], &param);

        if (pthread_create(&threads_read[i], &threads_read_attr[i], read_fun, (void *) k)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }
//        exit(0);
//    }

#ifdef DEBUG
    printf("Start sampling...\n");
#endif
    ++no_samples;
    barrier = 0;
    sleep(SAMPLE_RATE);
    while(1){
        if (cur_writes==writes && cur_reads==reads)
            break;
        cur_writes = writes;
        cur_reads  = reads;
        write_samples[no_samples] = cur_writes;
        read_samples[no_samples]  = cur_reads;
        ++no_samples;
        sleep(SAMPLE_RATE);
    }

    #ifdef DEBUG
    printf("Joining threads\n");
    #endif

    pthread_t sigint_thread;
    pthread_create(&sigint_thread, NULL, sigint_fun, &cur_pid);

    //joining
    for (i=0; i<N_r; i++) {
        if (pthread_join(threads_read[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
    }

    for (i=0; i<N_w; i++) {
        if(pthread_join(threads_write[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            return 2;
        }
    }

    if(pthread_cancel(sigint_thread)){
        fprintf(stderr, "Error canceling thread\n");
        return 2;
    }

    #ifdef DEBUG
    printf("from main\n");
    #endif
    cleanup();

    return 0;
}
