//
// Created by marcofavorito on 29/10/17.
//

#ifndef MAILSLOT_H
#define MAILSLOT_H



/* Gives us access to ioctl macros _IO and friends below. */
#include "const.h"
#include "helper.h"
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/fcntl.h>


struct mailslot_data {
    char *data;
    int len;
    struct mailslot_data *next;
};

struct mailslot_conf {
    long max_storage;
    long max_data_unit_size;
};

typedef struct mailslot_dev {
    struct mailslot_data *head, *tail;  /* begin of queue, end of queue */
    atomic_t no_msg;                         /*current number of messages*/
    atomic_long_t storage_size;              /* total storage size currently occupied*/
    atomic_t no_sessions;
    int flags;                         /* bit0: blocking write, bit1: blocking read */
    struct mailslot_conf conf;
    struct mutex mutex;                 /* Mutual exclusion semaphore. */
    struct semaphore rsem;              /* read semaphore */
    wait_queue_head_t wq;
    struct cdev* cdev;	                /* Char device structure. */
    int minor;

} mailslot_dev_t;


static spinlock_t open_release_lock;


void mailslot_setup_dev(struct mailslot_dev *dev, int minor);
ssize_t mailslot_read(struct file *filp,char *buf, size_t count, loff_t *offp );
ssize_t mailslot_write(struct file *filp, const char *buf, size_t count, loff_t * offp);
long mailslot_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int mailslot_release(struct inode *inode, struct file *filp);
#include "mm/mailslot_mm.h"




#endif //MAILSLOT_H