/**
 * @file message_slot.c
 * @brief Message slot kernel module implementation
 *
 * This module implements a character device driver for message slots, providing
 * an IPC mechanism with multiple concurrent message channels. Each message slot
 * device can have multiple channels, and each channel stores a single message
 * that persists until overwritten.
 *
 * Features:
 * - Multiple message slots identified by minor device numbers
 * - Multiple channels per slot identified by channel IDs
 * - Per-file-descriptor channel selection via ioctl
 * - Optional censorship mode (replaces every 4th character with '#')
 * - Atomic read/write operations
 *
 * @author Kernel Development Lab
 * @date 2026
 */

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* Core kernel functionality */
#include <linux/module.h>   /* Module macros and functions */
#include <linux/fs.h>       /* File operations and char device registration */
#include <linux/uaccess.h>  /* User space memory access (get_user, put_user) */
#include <linux/slab.h>     /* Memory allocation (kmalloc, kfree) */

#include "message_slot.h"

MODULE_LICENSE("GPL");

/* ==================== Data Structures ==================== */

/**
 * @struct channel
 * @brief Represents a single message channel within a message slot
 *
 * Channels form a linked list per message slot. Each channel stores
 * one message that persists until overwritten.
 */
typedef struct channel {
    unsigned int id;           /**< Channel identifier */
    char *message;             /**< Pointer to stored message */
    size_t len;                /**< Length of stored message */
    struct channel *next;      /**< Next channel in linked list */
} channel_t;

/**
 * @struct slot
 * @brief Represents a message slot device
 *
 * Each slot corresponds to a device file with a unique minor number.
 * Slots form a linked list and contain their own linked list of channels.
 */
typedef struct slot {
    int minor;                 /**< Minor device number */
    channel_t *channels;       /**< Head of channel linked list */
    struct slot *next;         /**< Next slot in linked list */
} slot_t;

/**
 * @struct fd_state
 * @brief Stores per-file-descriptor state
 *
 * This structure is stored in file->private_data and maintains
 * the channel ID and censorship mode for each open file descriptor.
 */
typedef struct fd_state {
    unsigned int channel_id;   /**< Currently selected channel ID (0 = none) */
    int censor;                /**< Censorship mode: 0 = disabled, 1 = enabled */
} fd_state_t;

/** @brief Head of the global linked list of message slots */
static slot_t *slots = NULL;

/* ==================== Helper Functions ==================== */

/**
 * @brief Find or create a message slot for a given minor number
 *
 * Searches the global slots list for a slot with the specified minor number.
 * If not found, allocates and initializes a new slot.
 *
 * @param minor The minor device number
 * @return Pointer to the slot, or NULL on allocation failure
 */
static slot_t *get_slot(int minor) {
    slot_t *s = slots;

    /* Search for existing slot */
    while (s) {
        if (s->minor == minor)
            return s;
        s = s->next;
    }

    /* Slot not found: create a new one */
    s = kmalloc(sizeof(slot_t), GFP_KERNEL);
    if (!s)
        return NULL;

    s->minor = minor;
    s->channels = NULL;
    s->next = slots;
    slots = s;
    return s;
}

/**
 * @brief Find or create a channel within a message slot
 *
 * Searches the slot's channel list for a channel with the specified ID.
 * If not found, allocates and initializes a new channel.
 *
 * @param slot Pointer to the message slot
 * @param id Channel identifier
 * @return Pointer to the channel, or NULL on allocation failure
 */
static channel_t *get_channel(slot_t *slot, unsigned int id) {
    channel_t *c = slot->channels;

    /* Search for existing channel */
    while (c) {
        if (c->id == id)
            return c;
        c = c->next;
    }

    /* Channel not found: create a new one */
    c = kmalloc(sizeof(channel_t), GFP_KERNEL);
    if (!c)
        return NULL;

    c->id = id;
    c->message = NULL;
    c->len = 0;
    c->next = slot->channels;
    slot->channels = c;
    return c;
}

/* ==================== File Operations ==================== */

/**
 * @brief Handle device file open operation
 *
 * Allocates and initializes per-file-descriptor state. The state is stored
 * in file->private_data and includes the channel ID and censorship mode.
 *
 * @param inode Pointer to the inode structure
 * @param file Pointer to the file structure
 * @return 0 on success, -ENOMEM on allocation failure
 */
static int device_open(struct inode *inode, struct file *file) {
    fd_state_t *state = kmalloc(sizeof(fd_state_t), GFP_KERNEL);
    if (!state)
        return -ENOMEM;

    state->channel_id = 0;
    state->censor = 0;
    file->private_data = state;
    return 0;
}

/**
 * @brief Handle ioctl operations on the device
 *
 * Supports two commands:
 * - MSG_SLOT_CHANNEL: Set the channel ID for this file descriptor
 * - MSG_SLOT_SET_CEN: Set the censorship mode for this file descriptor
 *
 * @param file Pointer to the file structure
 * @param ioctl_command_id The ioctl command identifier
 * @param ioctl_param The parameter for the ioctl command
 * @return 0 on success, -EINVAL for invalid command or parameter
 */
static long device_ioctl(struct file *file, unsigned int ioctl_command_id,
                         unsigned long ioctl_param) {
    fd_state_t *state = file->private_data;

    if (ioctl_command_id == MSG_SLOT_CHANNEL) {
        /* Channel ID must be non-zero */
        if (ioctl_param == 0)
            return -EINVAL;
        state->channel_id = (unsigned int)ioctl_param;
        return 0;
    }

    if (ioctl_command_id == MSG_SLOT_SET_CEN) {
        /* Censorship mode must be 0 or 1 */
        if (ioctl_param != 0 && ioctl_param != 1)
            return -EINVAL;
        state->censor = (int)ioctl_param;
        return 0;
    }

    return -EINVAL;
}

/**
 * @brief Write a message to the current channel
 *
 * Writes a message to the channel associated with this file descriptor.
 * If censorship is enabled, every 4th character (indices 3, 7, 11, etc.)
 * is replaced with '#'. The write operation is atomic - either the entire
 * message is written or none of it is.
 *
 * @param file Pointer to the file structure
 * @param buffer User-space buffer containing the message
 * @param len Length of the message
 * @param offset File offset (unused)
 * @return Number of bytes written on success, negative error code on failure
 * @retval -EINVAL No channel has been set on the file descriptor
 * @retval -EMSGSIZE Message length is 0 or exceeds MAX_MESSAGE_LEN
 * @retval -EFAULT Failed to copy data from user space
 * @retval -ENOMEM Memory allocation failed
 */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t len, loff_t *offset) {
    fd_state_t state = *(fd_state_t *)file->private_data;
    slot_t *slot;
    channel_t *channel;
    char temp_buf[MAX_MESSAGE_LEN];
    int i;

    /* Validate that a channel has been set */
    if (state.channel_id == 0)
        return -EINVAL;

    /* Validate message length */
    if (len == 0 || len > MAX_MESSAGE_LEN)
        return -EMSGSIZE;

    /* Copy message from user space one byte at a time */
    for (i = 0; i < len; i++) {
        if (get_user(temp_buf[i], &buffer[i]) != 0)
            return -EFAULT;
    }

    /* Apply censorship if enabled: replace every 4th character with '#' */
    if (state.censor) {
        for (i = 3; i < len; i += 4)
            temp_buf[i] = '#';
    }

    /* Get or create the message slot for this device */
    slot = get_slot(iminor(file_inode(file)));
    if (!slot)
        return -ENOMEM;

    /* Get or create the channel */
    channel = get_channel(slot, state.channel_id);
    if (!channel)
        return -ENOMEM;

    /* Allocate memory for the message and copy from temp buffer */
    kfree(channel->message);
    channel->message = kmalloc(len, GFP_KERNEL);
    if (!channel->message)
        return -ENOMEM;

    for (i = 0; i < len; i++)
        channel->message[i] = temp_buf[i];

    channel->len = len;

    return len;
}

/**
 * @brief Read a message from the current channel
 *
 * Reads the last message written to the channel associated with this file
 * descriptor. The message is returned exactly as stored - if it was written
 * with censorship enabled, the censored version is returned. The read
 * operation is atomic - either the entire message is read or none of it is.
 *
 * @param file Pointer to the file structure
 * @param buffer User-space buffer to receive the message
 * @param len Length of the user-space buffer
 * @param offset File offset (unused)
 * @return Number of bytes read on success, negative error code on failure
 * @retval -EINVAL No channel has been set on the file descriptor
 * @retval -ENOMEM Failed to get message slot
 * @retval -EWOULDBLOCK No message exists on the channel
 * @retval -ENOSPC Buffer is too small to hold the message
 * @retval -EFAULT Failed to copy data to user space
 */
static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t len, loff_t *offset) {
    fd_state_t *state = file->private_data;
    slot_t *slot;
    channel_t *channel;
    int i;

    /* Validate that a channel has been set */
    if (state->channel_id == 0)
        return -EINVAL;

    /* Get the message slot for this device */
    slot = get_slot(iminor(file_inode(file)));
    if (!slot)
        return -ENOMEM;

    /* Get the channel */
    channel = get_channel(slot, state->channel_id);
    if (!channel || !channel->message)
        return -EWOULDBLOCK;

    /* Validate buffer size */
    if (len < channel->len)
        return -ENOSPC;

    /* Copy message to user space one byte at a time */
    for (i = 0; i < channel->len; i++) {
        if (put_user(channel->message[i], &buffer[i]) != 0)
            return -EFAULT;
    }

    return channel->len;
}

/* ==================== Device Setup ==================== */

/**
 * @brief File operations structure for the message slot device
 *
 * This structure maps file operations to our handler functions.
 * The .owner field prevents the module from being unloaded while in use.
 */
struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = device_read,
    .write          = device_write,
    .open           = device_open,
    .unlocked_ioctl = device_ioctl,
};

/**
 * @brief Module initialization function
 *
 * Registers the character device with the kernel. This function is called
 * when the module is loaded with insmod.
 *
 * @return 0 on success, negative error code on failure
 */
static int __init simple_init(void)
{
    int rc;

    /* Register the character device and obtain major number */
    rc = register_chrdev(MAJOR_NUM, "message_slot", &fops);

    /* Negative return value indicates an error */
    if (rc < 0) {
        printk(KERN_ERR "message_slot: Failed to register device (error %d)\n", rc);
        return rc;
    }

    printk(KERN_INFO "message_slot: Module loaded successfully\n");
    return 0;
}

/**
 * @brief Module cleanup function
 *
 * Frees all allocated memory and unregisters the character device.
 * This function is called when the module is unloaded with rmmod.
 */
static void __exit msgslot_exit(void) {
    slot_t *s;
    channel_t *c;

    /* Free all allocated memory for slots and channels */
    while (slots) {
        s = slots;
        slots = slots->next;

        /* Free all channels in this slot */
        while (s->channels) {
            c = s->channels;
            s->channels = c->next;
            kfree(c->message);
            kfree(c);
        }
        kfree(s);
    }

    /* Unregister the character device */
    unregister_chrdev(MAJOR_NUM, "message_slot");

    printk(KERN_INFO "message_slot: Module unloaded successfully\n");
}

module_init(simple_init);
module_exit(msgslot_exit);
