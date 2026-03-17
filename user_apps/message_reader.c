/**
 * @file message_reader.c
 * @brief User-space program to read messages from a message slot device
 *
 * This program opens a message slot device file, selects a channel,
 * reads a message, prints it to stdout, and exits.
 *
 * Usage: message_reader <device_file> <channel_id>
 *   - device_file: Path to the message slot device (e.g., /dev/slot0)
 *   - channel_id: Non-zero channel identifier
 *
 * @author Kernel Development Lab
 * @date 2026
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "../include/message_slot.h"

/**
 * @brief Main function for the message reader program
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel;
    char buf[MAX_MESSAGE_LEN];
    ssize_t n;

    /* Validate command line arguments */
    if (argc != 3) {
        perror("Incorrect number of arguments. Usage: <device_file> <channel_id>");
        exit(1);
    }

    channel = atoi(argv[2]);

    /* Open the message slot device file for reading */
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        exit(1);
    }

    /* Set channel ID via ioctl */
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) < 0) {
        perror("Failed to set the channel id");
        exit(1);
    }

    /* Read message from the device */
    n = read(fd, buf, MAX_MESSAGE_LEN);
    if (n < 0) {
        perror("Failed to read from the message slot");
        exit(1);
    }

    /* Write message to stdout */
    if (write(STDOUT_FILENO, buf, n) != n) {
        perror("Failed to write to standard output");
        close(fd);
        exit(1);
    }

    close(fd);
    exit(0);
}
