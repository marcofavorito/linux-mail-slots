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
#include <linux/list.h>
#include <linux/spinlock.h>


#include "../mailslot.h"

#define DAEMON_FREQ (10 * HZ)


static struct kmem_cache *mailslot_data_cache;
static struct kmem_cache *mailslot_dev_cache;

//long used_minors[4]; //64 * 4 = 256 different instances
static struct mailslot_dev* mailslot_devices[MAILSLOT_NR_DEVS];

//Deallocator daemon function
static void deallocator_fun(struct work_struct *work_arg);
struct delayed_work deallocator_work;


// ********************************************************************************
// ********************************************************************************
// mailslot_dev
// ********************************************************************************
// ********************************************************************************

struct mailslot_dev* get_mailslot_dev(int minor){
    return mailslot_devices[minor];
}

void* alloc_mailslot_dev(int minor){
    if (!mailslot_devices[minor]){
        PRINTK(KERN_INFO, minor, "Allocating struct mailslot_dev\n");
        mailslot_devices[minor] = kmem_cache_alloc(mailslot_dev_cache, GFP_KERNEL);
    }
    else{
        printk("Problem\n");
        //    do nothing - it is already allocated
    }
    memset(mailslot_devices[minor], 0, sizeof(struct mailslot_dev));
    return mailslot_devices[minor];
}


void dealloc_mailslot_dev(struct mailslot_dev* dev){
    unsigned long counter;
    int minor;


    if (!dev) return;

    counter=0;
    minor = dev->minor;
    struct mailslot_data* cur;

    cur = dev->head;
    while(cur) {
        dev->head = dev->head->next;
        dealloc_mailslot_data(cur);
        ++counter;
        cur = dev->head;
    }

    kmem_cache_free(mailslot_dev_cache, dev);
    PRINTK(KERN_INFO, minor, "Freed %d messages.", counter);

}


// ********************************************************************************
// ********************************************************************************
// mailslot_data
// ********************************************************************************
// ********************************************************************************
void* alloc_mailslot_data(void){
    void * addr = kmem_cache_alloc(mailslot_data_cache, GFP_NOWAIT);
    if (!addr) return NULL;
    memset(addr, 0, sizeof(struct mailslot_data));
    return addr;

}

void dealloc_mailslot_data(struct mailslot_data *data){
    if (data) {
        PRINTK_DEBUG(-1, "Freeing ms_data %p and data %p\n", data, data->data);
        if (data->data) dealloc_data(data->data);
        kmem_cache_free(mailslot_data_cache, data);
    }
}

int get_data_size(struct mailslot_data *data){
    return sizeof(struct mailslot_data) + ksize(data->data);
}

// ********************************************************************************
// ********************************************************************************
// mailslot_data
// ********************************************************************************
// ********************************************************************************
void* alloc_data(int count){
    void * addr = kmalloc(sizeof(char)*count, GFP_NOWAIT);
    if (!addr) return NULL;
    memset(addr, 0, sizeof(char)*count);
    return addr;
}

void dealloc_data(void* data){
    kfree(data);
}


// ********************************************************************************
// ********************************************************************************
// misc
// ********************************************************************************
// ********************************************************************************

void init_mm_ds(void){
    memset(mailslot_devices, 0, sizeof(struct mailslot_dev*)*MAILSLOT_NR_DEVS);
    mailslot_dev_cache = kmem_cache_create(DEVICE_NAME, sizeof(struct mailslot_dev), 0, SLAB_HWCACHE_ALIGN, NULL); /* no ctor/dtor */
    mailslot_data_cache = kmem_cache_create(DEVICE_NAME, sizeof(struct mailslot_data), 0, SLAB_HWCACHE_ALIGN, NULL); /* no ctor/dtor */

    INIT_DELAYED_WORK(&deallocator_work, deallocator_fun);
    schedule_delayed_work(&deallocator_work, DAEMON_FREQ);
    return;
}

void deinit_mm_ds(void){
    int i;
    printk(KERN_INFO "%s: removing mailslots\n", DEVICE_NAME);

    cancel_delayed_work(&deallocator_work);
    flush_work(&deallocator_work.work);

    for (i = 0; i < MAILSLOT_NR_DEVS; i++) {
        PRINTK(KERN_INFO, i, "cleaning mailslot %d\n", i);

//        cleanup_mailslot_dev(&mailslot_devices[i]);
        if (mailslot_devices[i]) dealloc_mailslot_dev(mailslot_devices[i]);
        mailslot_devices[i] = NULL;

    }
    kmem_cache_destroy(mailslot_data_cache);
    kmem_cache_destroy(mailslot_dev_cache);

}


/* How much space is free? */
long spacefree(struct mailslot_dev * dev){
    PRINTK_DEBUG(dev->minor, "spacefree: max_storage=%ld - atomic_read(&dev->storage_size)=%ld = %ld\n", dev->conf.max_storage, atomic_long_read(&dev->storage_size), dev->conf.max_storage - atomic_long_read(&dev->storage_size));
    return dev->conf.max_storage - atomic_long_read(&dev->storage_size);
//    return 1000000000 - atomic_read(&dev->storage_size);
//    return 10000;
}

static void deallocator_fun(struct work_struct *work_arg){
    int i;
    struct mailslot_dev * dev;
    printk(KERN_INFO "%s: deallocator daemon called\n", DEVICE_NAME);
    spin_lock(&open_release_lock);
    for (i = 0; i<MAILSLOT_NR_DEVS; i++){


        dev = get_mailslot_dev(i);
        if (!dev) continue;
        if (mutex_trylock(&dev->mutex)){
            if (atomic_read(&dev->no_msg)!=0){
                PRINTK_DEBUG(dev->minor, "no deallocation: there is still some message in the queue\n");
                mutex_unlock(&dev->mutex);
                continue;
            }
        }
        else {
            PRINTK_DEBUG(dev->minor, "no deallocation: the device is locked. the owner is: %ld\n", atomic_long_read(&dev->mutex.owner));
            continue;
        }

        if (atomic_read(&dev->no_sessions)==0){
            dealloc_mailslot_dev(dev);
            mailslot_devices[i]=NULL;
            printk("%s: Freed dev nr %d\n", DEVICE_NAME, i);
        }
        else
            PRINTK_DEBUG(dev->minor, "no deallocation: there is at least one opened session\n");

        mutex_unlock(&dev->mutex);
        continue;
    }
    spin_unlock(&open_release_lock);
    schedule_delayed_work(&deallocator_work, DAEMON_FREQ);

}


/*
 *         if (!dev){
            printk("null\n");
            continue;
        }
        if (dev->no_msg!=0){
            printk("msg: %d\n", dev->no_msg);
            continue;
        }
 */
