

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */
#include <linux/semaphore.h>
#include <linux/slab.h>

#include "mailslot.h"
#include "helper.h"

////    printk(KERN_INFO " %s\n", msg);
////    int temp = len;
////
////    if(*offp >= len) return 0;
////
////    temp = len - *offp;
////
////    if(count>temp)
////    {
////        count=temp;
////    }
////    copy_to_user(buf,msg+(*offp), count);
////
////    *offp += count;




ssize_t mailslot_read(struct file *filp,char *buf, size_t count, loff_t *offp )
{

    int minor;
    struct mailslot_dev *dev;
    struct mailslot_data * temp;
    int blocking_read;
    int copied, l;
    int ret;

    PRINTK_DEBUG(minor, "read called\n");
    PRINTK_DEBUG(minor, "count=%lu offp=%ld\n", count, *offp);

//    struct mailslot_dev *dev = filp->private_data;
//    struct mailslot_dev *dev = &mailslot_devices[minor];
    minor = iminor(filp->f_inode);
    dev = get_mailslot_dev(minor);

    if (*offp!=0){
        PRINTK(KERN_ERR, minor, "read: offset is not zero\n");
        return ERROR;
    }

//    blocking_read = !(filp->f_flags & O_NONBLOCK);
    blocking_read = !(dev->flags & MAILSLOT_R_NONBLOCK);

    if (blocking_read){
        if (down_interruptible(&dev->rsem)) return -ERESTARTSYS;
        if (mutex_lock_interruptible(&dev->mutex)) return -ERESTARTSYS;
    }
    else {
        if (down_trylock(&dev->rsem)){
            PRINTK(KERN_ERR, minor, "read: no message available right now\n");
            return -EAGAIN;
        }
        if (!mutex_trylock(&dev->mutex)){
            PRINTK(KERN_ERR, minor, "read: resource not available\n");
            return -EAGAIN;
        }
    }

    l = dev->head->len;

    if (count < l){
        PRINTK(KERN_ERR, minor, "read: the buffer is too small\n");
        return ERROR;
    }

    char aux[l];
    memcpy(aux, dev->head->data, l);
    ret = l;

    temp = dev->head;
    if (dev->head->next) dev->head = dev->head->next;
    else dev->head=NULL;

    PRINTK_DEBUG(minor, "read: old storage: %ld and freeing %d. l: %d\n", atomic_long_read(&dev->storage_size), sizeof(struct mailslot_dev) + sizeof(char)*l, l);
    PRINTK_DEBUG(minor,"read: freeing %d\n", get_data_size(temp));
    atomic_long_sub(get_data_size(temp), &dev->storage_size);
    dealloc_mailslot_data(temp);
    atomic_dec(&dev->no_msg);
    PRINTK_DEBUG(minor,"read: new storage: %ld\n", dev->storage_size.counter);
    PRINTK_DEBUG(minor,"read: remained number of messages %d\n", dev->no_msg);

    mutex_unlock(&dev->mutex);
    wake_up_interruptible_all(&dev->wq);

    *offp+=l;

    copied = copy_to_user(buf, aux, l);
    if (copied){
        PRINTK(KERN_ERR, minor, "read: some error occurred during 'copy_to_user'. return value: %d\n", DEVICE_NAME, copied);
        return -EFAULT;
    }
    PRINTK_DEBUG(minor, "copy to user %s, len: %d\n", aux, l);

    return l;
}
