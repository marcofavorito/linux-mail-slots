#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>


#include "../../const.h"

#define BUF_SIZE 100

int main(int argc, char* argv[]) {

    int id = strtol(argv[1], NULL, 10);
    char* path="/dev/mailslot\0";
    char dev[32];

    sprintf(dev, "%s%d", path, id);

    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        printf("%s - err open\n", dev);
        return -1;
    }
    char data[BUF_SIZE];

    int ret;

//    try to don't block
    ioctl(fd,MAILSLOT_IOCTWRITEBLOCK, 0);
    ioctl(fd,MAILSLOT_IOCTREADBLOCK, 0);

    while(scanf("%s", data)>0) {
        ret = pwrite(fd, data, BUF_SIZE, 0);
        if (ret < 0) {
            printf("%s - err write\n", dev);
            return -1;
        }
        ret = pread(fd, data, BUF_SIZE, 0);
        if (ret < 0) {
            printf("%s - err read\n", dev);
            return -1;
        }
        else{
            printf("%s\n", data);
        }
    }
    close(fd);
    return 0;
}



//    ioctl(fd,CMD_MAX_MSG_LEN, 5);
//    ioctl(fd,CMD_BLOCKING_WRITE, 0);
//    ioctl(fd,CMD_BLOCKING_READ, 0);
//    ioctl(fd,CMD_MAX_MSG, 3);

// ret = write(fd, s, l);
// if (ret < 0) {
//     printf("%s - err write\n", dev);
//     return -1;
// }
// ret = write(fd, s, l);
// if (ret < 0) {
//     printf("%s - err write\n", dev);
//     return -1;
// }
//
// ioctl(fd,CMD_MAX_MSG, 5);
//
