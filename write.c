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
#include "mm/mailslot_mm.h"
#include "helper.h"

static int mailslot_getwritespace(struct mailslot_dev *dev, struct file *filp, int required_space)
{

    DEFINE_WAIT(wait);

    PRINTK_DEBUG(dev->minor, "space free: %ld <=> required space: %d\n", spacefree(dev), required_space);
    while (spacefree(dev) <= required_space) { /* full */
        mutex_unlock(&dev->mutex);
        if (filp->f_flags & O_NONBLOCK) {
//        if (!dev->conf.write_blocking) {
            PRINTK(KERN_ERR, dev->minor, "nonblocking write: returning EAGAIN\n", current->comm);
            return -EAGAIN;
        }


        prepare_to_wait(&dev->wq, &wait, TASK_INTERRUPTIBLE);
        if (spacefree(dev) <= 0)
            schedule();
        finish_wait(&dev->wq, &wait);
        if (signal_pending(current))
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */

        if (mutex_lock_interruptible(&dev->mutex))
            return -ERESTARTSYS;
    }
    return 0;
}


ssize_t mailslot_write(struct file *filp, const char *buf, size_t count, loff_t * offp)
{
    int blocking_write;
    int minor;
    int required_space;
    int retval;
    char aux[count];
    struct mailslot_dev *dev;
    struct mailslot_data * ms_ptr;
    char * data_ptr;


//    struct mailslot_dev *dev = filp->private_data;
    minor = iminor(filp->f_inode);
    dev = get_mailslot_dev(minor);

    //    blocking_write = !(filp->f_flags & O_NONBLOCK);
    blocking_write = !(dev->flags & MAILSLOT_W_NONBLOCK);

    PRINTK_DEBUG(minor, "write called. blocking: %d\n", blocking_write);
    PRINTK_DEBUG(minor, "write: current available space: %l\n", spacefree(dev));

    if (count > dev->conf.max_data_unit_size || count > dev->conf.max_storage){
        PRINTK(KERN_ERR, minor, "write: size of message too high: count %d, max %d, max data unit %d\n", count, dev->conf.max_storage, dev->conf.max_data_unit_size);
        return -1;
    }

//    copy from user the message and store into a temp buffer
    if (retval = copy_from_user(aux, buf, count)){
        PRINTK(KERN_ERR, minor, "write: retval=%d count=%d\n", retval, count);
        retval = -1;
        return retval;
    }

    if (blocking_write) {
        if (mutex_lock_interruptible(&dev->mutex)) return -ERESTARTSYS;
    }
    else if (!mutex_trylock(&dev->mutex)) {
        PRINTK(KERN_ERR, minor, "Resource not available.\n");
        return -EAGAIN;
    }

    required_space = sizeof(struct mailslot_data) + sizeof(char)*count;
    retval = mailslot_getwritespace(dev, filp, required_space);
    if (retval){
        mutex_unlock(&dev->mutex);
        return retval; /* getwritespace called mutex_unlock(&dev->mutex) */
    }

//    struct mailslot_data * ms_ptr = kmalloc(sizeof(struct mailslot_data), GFP_NOWAIT);
//    struct mailslot_data * ms_ptr = kmem_cache_alloc(mailslot_data_cache, GFP_NOWAIT);
    PRINTK_DEBUG(minor, "allocating %d and %d\n", sizeof(struct mailslot_data), count);
    ms_ptr = alloc_mailslot_data();
    if (!ms_ptr){
        mutex_unlock(&dev->mutex);
        PRINTK(KERN_ERR, minor, "No memory\n");
        return -ENOMEM;
    }
    data_ptr = alloc_data(count);
    if (!data_ptr){
        mutex_unlock(&dev->mutex);
        PRINTK(KERN_ERR, minor, "No memory\n");
        return -ENOMEM;
    }
    memset(ms_ptr, 0, sizeof(struct mailslot_data));
    memset(data_ptr, 0, sizeof(char)*count);
    ms_ptr->data = data_ptr;
    ms_ptr->len = count;
    PRINTK_DEBUG(minor, "data size: %d\n", get_data_size(ms_ptr));
    atomic_long_add(get_data_size(ms_ptr), &dev->storage_size);
    PRINTK_DEBUG(minor, "now storage_size is : %l\n", dev->storage_size.counter);
//    printk("%d %d %d\n", ksize(ms_ptr->data), sizeof(struct mailslot_data), get_data_size(ms_ptr));

    if (atomic_read(&dev->no_msg) == 0){
        dev->tail = ms_ptr;
        dev->head = dev->tail;
    }
    else{
        dev->tail->next = ms_ptr;
        dev->tail = dev->tail->next;
    }

    dev->tail->next = NULL;

    memcpy(dev->tail->data, aux, count);
    dev->tail->len = count;
    atomic_inc(&dev->no_msg);
    PRINTK_DEBUG(minor, "new no_msg: %d\n", dev->no_msg.counter);

    up(&dev->rsem);
    mutex_unlock(&dev->mutex);


    retval = count;
    return retval;

}