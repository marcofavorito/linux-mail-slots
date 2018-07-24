//
// Created by marcofavorito on 04/11/17.
//

#ifndef CONST_H
#define CONST_H

#define DRIVER_NAME "mailslot"
#define DEVICE_NAME "mailslot"  /* Device file name in /dev/ - not mandatory  */

/*
 * Macros to help with debugging. Set MAILSLOT_DEBUG to 1 enable
 * debugging (which you can do from the Makefile); these macros work
 * in both kernelspace and userspace.
 */

#undef PDEBUG /* undef it, just in case someone else defined it. */
#ifdef MAILSLOT_DEBUG
#  ifdef __KERNEL__
     /* Debugging is on and we are in kernelspace. */
#    define PDEBUG(fmt, args...) printk(KERN_DEBUG "mailslot: " fmt, ## args)
#  else
     /* Debugging is on and we are in userspace. */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* Not debugging: do nothing. */
#endif


/* PDEBUGG is a placeholder that makes it easy to "comment out" the debugging
   statements without deleting them. */
#undef PDEBUGG
#define PDEBUGG(fmt, args...)

#ifndef MAILSLOT_MAJOR
#define MAILSLOT_MAJOR     0 /* dynamic major by default */
#endif

#ifndef MAILSLOT_NR_DEVS
#define MAILSLOT_NR_DEVS   256 /* mailslot0 through mailslot255 */
#endif

#define MAILSLOT_MAX_DATA_UNIT_SIZE 128
#define MAILSLOT_MAX_STORAGE (1<<30)
//#define MAILSLOT_MAX_STORAGE (2048<<10)
//#define MAILSLOT_MAX_STORAGE (1<<10)





/*
 * ioctl definitions
 */

#include <linux/ioctl.h>
/* Use 'm' as magic number */
#define MAILSLOT_IOC_MAGIC  'm'
#define MAILSLOT_IOCRESET    _IO(MAILSLOT_IOC_MAGIC, 0)

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift": switch T and Q atomically
 */
#define MAILSLOT_IOCTMAXSTORAGE         _IOW(MAILSLOT_IOC_MAGIC,  1, int)
#define MAILSLOT_IOCTMAXDATAUNITLEN     _IOW(MAILSLOT_IOC_MAGIC,  2, int)
#define MAILSLOT_IOCTREADBLOCK          _IOW(MAILSLOT_IOC_MAGIC,  3, int)
#define MAILSLOT_IOCTWRITEBLOCK         _IOW(MAILSLOT_IOC_MAGIC,  4, int)
//#define MAILSLOT_IOCTBLOCK              _IOW(MAILSLOT_IOC_MAGIC,  3, int)

#define MAILSLOT_IOC_MAXNR 4

#ifndef MAILSLOT_W_NONBLOCK
#define MAILSLOT_W_NONBLOCK 00000001
#endif

#ifndef MAILSLOT_R_NONBLOCK
#define MAILSLOT_R_NONBLOCK 00000002
#endif

#endif //CONST_H
