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
#include "config.h"

long mailslot_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int minor;
    int b;
    struct mailslot_dev *dev;
    int nonblock_bitmask;

    int err, tmp, retval;
    retval = 0;
    err = 0;
    minor = iminor(filp->f_inode);
    dev = get_mailslot_dev(minor);
    /*
     * Extract the type and number bitfields, and don't decode incorrect
     * cmds: return ENOTTY (inappropriate ioctl) before access_ok().
     */
    if (_IOC_TYPE(cmd) != MAILSLOT_IOC_MAGIC){
        return -ENOTTY;
        PRINTK(KERN_ERR, minor,"ioctl: magic number not valid: %d!=%d\n", _IOC_TYPE(cmd), MAILSLOT_IOC_MAGIC);
    }

    if (_IOC_NR(cmd) > MAILSLOT_IOC_MAXNR){
        return -ENOTTY;
        PRINTK(KERN_ERR, minor,"ioctl: command not valid: %d\n", cmd);
    }


    /*
     * The direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while access_ok is
     * kernel-oriented, so the concept of "read" and "write" is reversed.
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;


    switch(cmd) {
//        case MAILSLOT_IOCTBLOCK:
            // set/unset block flag.
//            printk("%s: current I/O blocking mode: %d\n", DEVICE_NAME, filp->f_flags & O_NONBLOCK);
//            b = (arg!=0?1:0);
//            if (b)
//            // Clear the NONBLOCK bit, since we want to block (b=1)
//                filp->f_flags &= ~O_NONBLOCK;
//            else
//            // Set the NONBLOCK bit, since we don't want to block (b=0)
//                filp->f_flags |= O_NONBLOCK;
//            printk("%s: I/O blocking mode set to: %d %d\n", DEVICE_NAME, b, filp->f_flags & O_NONBLOCK);
//            break;
        case MAILSLOT_IOCTWRITEBLOCK:
        case MAILSLOT_IOCTREADBLOCK:
            nonblock_bitmask = cmd==MAILSLOT_IOCTWRITEBLOCK? MAILSLOT_W_NONBLOCK:MAILSLOT_R_NONBLOCK;
            PRINTK_DEBUG(minor, "ioctl: modifying %s settings\n", nonblock_bitmask==MAILSLOT_W_NONBLOCK?"write":"read");
            PRINTK_DEBUG(minor, "ioctl: current I/O blocking mode: %d\n", dev->flags & nonblock_bitmask);
            b = (arg!=0?1:0);
            if (b)
                // Clear the NONBLOCK bit, since we want to block (b=1)
                dev->flags &= ~nonblock_bitmask;
            else
                // Set the NONBLOCK bit, since we don't want to block (b=0)
                dev->flags |= nonblock_bitmask;
            PRINTK_DEBUG(minor, "ioctl: I/O blocking mode set to %d, non-block flag set to %d\n", b, dev->flags & nonblock_bitmask);
            break;
        case MAILSLOT_IOCTMAXSTORAGE:
            if (arg > MAX_STORAGE_SIZE ||
                arg < MIN_STORAGE_SIZE){
                PRINTK(KERN_ERR, minor, "ioctl: chosen storage size too large or too small. %lu\n", arg);
                return -EINVAL;
            }
            dev->conf.max_storage = (long)arg;
            PRINTK_DEBUG(minor, "ioctl: New max storage is: %lu\n", dev->conf.max_storage);
            break;

        case MAILSLOT_IOCTMAXDATAUNITLEN:
            if (arg > MAX_DATAUNIT_SIZE ||
                arg < MIN_DATAUNIT_SIZE){
                PRINTK(KERN_ERR, minor, "ioctl: chosen data unit size too large or too small. %lu\n", arg);
                return -EINVAL;
            }
            dev->conf.max_data_unit_size = (long)arg;
            PRINTK_DEBUG(minor, "ioctl: New max dataunit size is: %lu\n", dev->conf.max_data_unit_size);
            break;

        default:  /* Redundant, as cmd was checked against MAXNR. */
            return -ENOTTY;
    }
    return retval;

}