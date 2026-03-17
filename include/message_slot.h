/**
 * @file message_slot.h
 * @brief Header file for the message slot kernel module
 *
 * This file defines the interface for a message slot character device driver
 * that provides an IPC mechanism with multiple message channels.
 *
 * @author Kernel Development Lab
 * @date 2026
 */

#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

/** @brief Major device number for message slot devices */
#define MAJOR_NUM 235

/**
 * @brief IOCTL command to set the channel ID for a file descriptor
 * @param channel_id An unsigned int specifying the non-zero channel ID
 */
#define MSG_SLOT_CHANNEL  _IOW(MAJOR_NUM, 0, unsigned int)

/**
 * @brief IOCTL command to set censorship mode for a file descriptor
 * @param mode An unsigned int: 0 for disabled, 1 for enabled
 */
#define MSG_SLOT_SET_CEN  _IOW(MAJOR_NUM, 1, unsigned int)

/** @brief Maximum message length in bytes */
#define MAX_MESSAGE_LEN 128

#endif /* MESSAGE_SLOT_H */
