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
#include <linux/spinlock.h>
#include "mailslot.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Favorito");
MODULE_DESCRIPTION("A mailslot service in Linux inspired by the original in Windows.");
MODULE_VERSION("1.0.0");


int mailslot_major =   MAILSLOT_MAJOR;
int mailslot_minor =   0;
int mailslot_nr_devs = MAILSLOT_NR_DEVS;	/* number of bare mailslot devices */

struct cdev cdevs[MAILSLOT_NR_DEVS];
static spinlock_t open_release_lock;


static int mailslot_open(struct inode *inode, struct file *filp)
{
    int minor;
    struct mailslot_dev *dev;


    minor = iminor(inode);

    PRINTK_DEBUG(minor, "Open called\n");

    if (minor<0 || minor >= MAILSLOT_NR_DEVS) {
        printk(KERN_ERR "%s: minor number %d not supported\n", DRIVER_NAME, minor);
        return -ENODEV;
    }

    spin_lock(&open_release_lock);
    dev = get_mailslot_dev(minor);
    if (!dev){
        PRINTK(KERN_INFO, minor, "Setting up mailslot_dev struct\n");
        dev = alloc_mailslot_dev(minor);
        if (!dev){
            PRINTK(KERN_ERR, minor, "no memory for set up a new mailslot_dev struct\n");
            return -ENOMEM;
        }
        mailslot_setup_dev(dev, minor);
    }

    atomic_inc(&dev->no_sessions);
    spin_unlock(&open_release_lock);

    PRINTK_DEBUG(minor, "opened new session for process: %d\n", current->pid);
    PRINTK_DEBUG(minor, "no sessions: %d\n", atomic_read(&dev->no_sessions));

    return 0;
}

int mailslot_release(struct inode *inode, struct file *filp)
{

    int minor;
    struct mailslot_dev *dev;
    minor = iminor(inode);

    spin_lock(&open_release_lock);
    dev = get_mailslot_dev(minor);

    atomic_dec(&dev->no_sessions);
    spin_unlock(&open_release_lock);

    PRINTK_DEBUG(minor, "closed session for process: %d\n", current->pid);

    return 0;
}


static struct file_operations mailslot_fops = {
        .owner = THIS_MODULE,
        .read = mailslot_read,
        .open =  mailslot_open,
        .write = mailslot_write,
        .release = mailslot_release,
        .unlocked_ioctl = mailslot_ioctl,
};


void mailslot_setup_dev(struct mailslot_dev *dev, int minor){
    //    Initialize data structure for synchronization
    mutex_init(&dev->mutex);
    sema_init(&dev->rsem, 0);
    init_waitqueue_head(&dev->wq);

    //    set up default configurations
    dev->conf.max_storage =         MAILSLOT_MAX_STORAGE;
    dev->conf.max_data_unit_size =  MAILSLOT_MAX_DATA_UNIT_SIZE;
    dev->no_msg.counter = 0;
    dev->no_sessions.counter = 0;
    dev->storage_size.counter = 0;

    dev->minor = minor;
    dev->cdev = &cdevs[minor];


}

void mailslot_setup_cdev(int minor){
    int err, devno;

    devno = MKDEV(mailslot_major, mailslot_minor + minor);

    cdev_init(&cdevs[minor], &mailslot_fops);
    cdevs[minor].owner = THIS_MODULE;
    cdevs[minor].ops = &mailslot_fops;
    err = cdev_add(&cdevs[minor], devno, 1);

    PRINTK(KERN_WARNING, minor, "initialized device no: %d\n", devno);

    /* Fail gracefully if need be. */
    if (err) PRINTK(KERN_NOTICE, minor, "Error %d adding mailslot\n", err);

}


void mailslot_cleanup_module(void)
{
    int i;

//    if (!mailslot_devices) {
//        return; /* nothing else to release */
//    }

    deinit_mm_ds();
    for  (i = 0; i< MAILSLOT_NR_DEVS; i++)
        cdev_del(&cdevs[i]);

//    kfree(mailslot_devices);
//    mailslot_devices = NULL; /* pedantic */

    unregister_chrdev_region(MKDEV(mailslot_major, mailslot_minor), MAILSLOT_NR_DEVS);
    printk(KERN_INFO "%s: device unregistered, it was assigned major number %d\n", DRIVER_NAME, mailslot_major);

}

int mailslot_init_module(void)
{
    int ret, i;

    /*
	 * Get a range of minor numbers to work with, asking for a dynamic major
	 * unless directed otherwise at load time.
	 */
    dev_t dev = 0;
    if (mailslot_major){
//        static allocation
        dev = MKDEV(mailslot_major, mailslot_minor);
        ret = register_chrdev_region(dev, MAILSLOT_NR_DEVS, DEVICE_NAME);
    } else {
//        dynamic allocation
        ret = alloc_chrdev_region(&dev, mailslot_minor, MAILSLOT_NR_DEVS, DEVICE_NAME);
        mailslot_major = MAJOR(dev);
    }
    if (ret < 0) {
        printk(KERN_WARNING "can't get major %d\n", DRIVER_NAME, mailslot_major);
        return ret;
    }

    printk(KERN_INFO "%s: device registered, it is assigned major number %d\n", DRIVER_NAME, mailslot_major);

    init_mm_ds();
    for (i = 0; i<MAILSLOT_NR_DEVS; i++){
        mailslot_setup_cdev(i);
    }
    spin_lock_init(&open_release_lock);



    return 0;
//    fail:
//        mailslot_cleanup_module();
//        return ret;
}


module_init(mailslot_init_module);
module_exit(mailslot_cleanup_module);