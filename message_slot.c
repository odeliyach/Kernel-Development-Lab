#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/slab.h>   /* for memset. NOTE - not string.h!*/

#include "message_slot.h"

MODULE_LICENSE("GPL");


 /* a data structure to describe individual channels within a message slot */
typedef struct channel {
    unsigned int id;
    char *message;
    size_t len;
    struct channel *next;
} channel_t;
/*a data structure to describe individual message slots*/
typedef struct slot {
    int minor;
    channel_t *channels;
    struct slot *next;
} slot_t;
/* a data structure to describe the state of an open file descriptor */
typedef struct fd_state {
    unsigned int channel_id;
    int censor;
} fd_state_t;

static slot_t *slots = NULL;

static slot_t *get_slot(int minor) {
    slot_t *s = slots;
    /* Search for existing slot */
    while (s) {
        if (s->minor == minor)/* Slot already exists */
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

static channel_t *get_channel(slot_t *slot, unsigned int id) {
    channel_t *c = slot->channels;
     /* Search for existing channel */
    while (c) 
    {
        if (c->id == id)/* Channel already exists */
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

static int device_open(struct inode *inode, struct file *file) {
    fd_state_t *state = kmalloc(sizeof(fd_state_t), GFP_KERNEL);/*Allocate memory using kmalloc() with GFP_KERNEL flag*/
    if (!state)
        return -ENOMEM;

    state->channel_id = 0;
    state->censor = 0;
    file->private_data = state;
    return 0;
}

static long device_ioctl(struct file *file, unsigned int   ioctl_command_id, unsigned long ioctl_param) {
    fd_state_t *state = file->private_data;

    if (ioctl_command_id == MSG_SLOT_CHANNEL) {
        if (ioctl_param == 0) /*If the passed channel id is 0 (for MSG_SLOT_CHANNEL), the ioctl() returns -1 and errno is set to EINVAL*/
            return -EINVAL;
        state->channel_id = (unsigned int)ioctl_param;
        return 0;
    }

    if (ioctl_command_id == MSG_SLOT_SET_CEN) {
        if (ioctl_param != 0 && ioctl_param != 1)/*מצב כזה זה לא עם או בלי צנזורה אלא משהו לא מוגדר*/
            return -EINVAL;
        state->censor = (int)ioctl_param;
        return 0;
    }

    return -EINVAL;/*If the passed command is not MSG_SLOT_CHANNEL or MSG_SLOT_SET_CEN, the ioctl() returns -1 and errno is set to EINVAL.*/
}

static ssize_t device_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    fd_state_t state = *(fd_state_t *)file->private_data;
    slot_t *slot;
    channel_t *channel;
    char temp_buf[MAX_MESSAGE_LEN];
    int i;

    if (state.channel_id == 0)    /*If no channel has been set on the file descriptor, returns -1 and errno is set to EINVAL */
        return -EINVAL;

    if (len == 0 || len > MAX_MESSAGE_LEN)/*If the passed message length is 0 or more than 128, returns -1 and errno is set to EMSGSIZE */
        return -EMSGSIZE;

    /* Copy from user one byte at a time */
    for (i = 0; i < len; i++) {
        if (get_user(temp_buf[i], &buffer[i]) != 0)
            return -EFAULT;
    }

    /* Apply censorship if needed */
    if (state.censor) 
    {
        for (i = 3; i < len; i += 4)
            temp_buf[i] = '#';
    }
    slot = get_slot(iminor(file_inode(file)));
    if (!slot)/* In any other error case (for example, failing to allocate memory), returns -1 and errno is set appropriatel*/
        return -ENOMEM;

    channel = get_channel(slot, state.channel_id);
    if (!channel)/* In any other error case (for example, failing to allocate memory), returns -1 and errno is set appropriatel*/
        return -ENOMEM;

    /* Allocate memory for message and copy from temp buffer */
    kfree(channel->message);
    channel->message = kmalloc(len, GFP_KERNEL);
    if (!channel->message)
        return -ENOMEM;

    for (i = 0; i < len; i++)
        channel->message[i] = temp_buf[i];

    channel->len = len;

    return len;
}

static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t len, loff_t *offset) {
    fd_state_t *state = file->private_data;
    slot_t *slot;
    channel_t *channel;
    int i;


    if (state->channel_id == 0)/*If no channel has been set on the file descriptor, returns -1 and errno is set to EINVAL*/
        return -EINVAL;

    slot = get_slot(iminor(file_inode(file)));
    if (!slot) /* In any other error case (for example, failing to allocate memory), returns -1 and errno is set appropriatel*/
        return -ENOMEM;

    channel = get_channel(slot, state->channel_id);
    if (!channel || !channel->message) /*If no message exists on the channel, returns -1 and errno is set to EWOULDBLOCK*/
        return -EWOULDBLOCK;

    if (len < channel->len)/**If the provided buffer length is too small to hold the last message written on the channel, returns -1 and errno is set to ENOSPC */
        return -ENOSPC;
    for (i = 0; i < channel->len; i++) {
        if (put_user(channel->message[i], &buffer[i]) != 0)
            return -EFAULT; 
    }

    return channel->len;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
};


//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev(MAJOR_NUM, "message_slot", &fops);

  // Negative values signify an error
  if( rc < 0 ) {
    printk(KERN_ERR "Failed to register message_slot\n");
    return rc;
  }
  //printk(KERN_INFO "message_slot loaded successfully\n");
  return 0;
}
//---------------------------------------------------------------

static void __exit msgslot_exit(void) {
    slot_t *s;
    channel_t *c;
    /*שחרר כל הזיכרון שהוקצה עבור slots ו־channels.*/
    while (slots) {
        s = slots;
        slots = slots->next;

        while (s->channels) {
            c = s->channels;
            s->channels = c->next;
            kfree(c->message);
            kfree(c);
        }
        kfree(s);
    }
    // Unregister the device
  // Should always succeed
    unregister_chrdev(MAJOR_NUM, "message_slot");
}
//---------------------------------------------------------------
module_init(simple_init);
module_exit(msgslot_exit);

//========================= END OF FILE =========================
