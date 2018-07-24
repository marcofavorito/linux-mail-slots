#ifndef HELPER_H
#define HELPER_H

#define SUCCESS 0         // const
#define ERROR -1         // const

#define ERROR_HELPER(cond, msg, code) {                             \
    do {                                                      \
        if (cond) {                                           \
            printk(KERN_ERR "%s error: %s\n", DRIVER_NAME, msg); \
            return code;                                     \
        }                                                     \
    } while (0);                                              \
}                                                             \

#define WARNING_HELPER(cond, msg, code) {                                 \
    do {                                                            \
        if (cond) {                                                 \
            printk(KERN_WARNING "%s warning: %s\n", DRIVER_NAME, msg); \
            return code;                                         \
        }                                                           \
    } while (0);                                                    \
}                                                                   \

#ifdef MAILSLOT_DEBUG
#   define PRINTK_DEBUG(minor, fmt, args...) printk("%s: %d: " fmt, DRIVER_NAME, minor, ##args);
#else
#   define PRINTK_DEBUG(minor, fmt, args...)
#endif

#define PRINTK(flag, minor, fmt, args...) printk(flag "%s: %d: " fmt, DRIVER_NAME, minor, ##args); \

#endif //HELPER_H