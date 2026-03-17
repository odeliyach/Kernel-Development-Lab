#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char *argv[]) {
    int fd;
    unsigned int channel, cen;
    ssize_t rc;

    if (argc != 5)/*You should validate that the correct number of command line arguments is passed.*/
     {
        perror("Incorrect number of arguments. should be: <device_file> <channel_id> <message>");
        exit(1);

    }

    channel = atoi(argv[2]);
    cen = atoi(argv[3]);

    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device file");
        exit(1);

    }

    if (ioctl(fd, MSG_SLOT_SET_CEN, cen) < 0) {
        perror("Failed to set censorship mode via ioctl");
       exit(1);

    }

    if (ioctl(fd, MSG_SLOT_CHANNEL, channel) < 0) {
               perror("Failed to set channel ID via ioctl");

        exit(1);

    }

    rc = write(fd, argv[4], strlen(argv[4]));
    if (rc < 0) {
        perror("Failed to write message to device");
        exit(1);

    }

    close(fd);
    exit(0);

}
