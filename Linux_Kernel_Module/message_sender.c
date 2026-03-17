/**
 * @file message_sender.c
 * @brief User-space program to send messages to a message slot device
 *
 * This program opens a message slot device file, sets the censorship mode,
 * selects a channel, writes a message, and exits.
 *
 * Usage: message_sender <device_file> <channel_id> <censorship_mode> <message>
 *   - device_file: Path to the message slot device (e.g., /dev/slot0)
 *   - channel_id: Non-zero channel identifier
 *   - censorship_mode: 0 to disable censorship, 1 to enable
 *   - message: The message string to send
 *
 * @author Kernel Development Lab
 * @date 2026
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "message_slot.h"

/**
 * @brief Main function for the message sender program
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel, cen;
    ssize_t rc;

    /* Validate command line arguments */
    if (argc != 5) {
        perror("Incorrect number of arguments. Usage: <device_file> <channel_id> <censorship_mode> <message>");
        exit(1);
    }

    channel = atoi(argv[2]);
    cen = atoi(argv[3]);

    /* Open the message slot device file for writing */
    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        exit(1);
    }

    /* Set censorship mode via ioctl */
    if (ioctl(fd, MSG_SLOT_SET_CEN, cen) < 0) {
        perror("Failed to set censorship mode via ioctl");
        exit(1);
    }

    /* Set channel ID via ioctl */
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) < 0) {
        perror("Failed to set channel ID via ioctl");
        exit(1);
    }

    /* Write message to the device */
    rc = write(fd, argv[4], strlen(argv[4]));
    if (rc < 0) {
        perror("Failed to write message to device");
        exit(1);
    }

    close(fd);
    exit(0);
}
