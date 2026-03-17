#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel;
    char buf[MAX_MESSAGE_LEN];
    ssize_t n;

    if (argc != 3) /*You should validate that the correct number of command line arguments is passed.*/
    {
        perror("Incorrect number of arguments. should be: <device_file> <channel_id>");
        exit(1);
    }

    channel = atoi(argv[2]);

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        exit(1);
    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) < 0) {
        perror("Failed to set the channel id");
        exit(1);
    }

    n = read(fd, buf, MAX_MESSAGE_LEN);
    if (n < 0) {
        perror("Failed to read from the message slot");
        exit(1);
    }
    
    if (write(STDOUT_FILENO, buf, n) != n) {
        perror("Failed to write to standard output");
        close(fd);
        exit(1);
    }
    close(fd);
    exit(0);
}
