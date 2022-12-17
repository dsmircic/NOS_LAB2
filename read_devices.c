#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include <time.h>

#define TIMEOUT 5000
#define NUM_DEVICES 5

int main(int arg, char **argv)
{

    int devices[NUM_DEVICES];
    struct pollfd fds[NUM_DEVICES];

    for (int i = 0; i < NUM_DEVICES; i++)
    {
        char device_name[64];
        sprintf(device_name, "/dev/shofer%d", i);
        devices[i] = open(device_name, 0);
        if (devices[i] < 0)
        {
            perror("Error while opening device\n");
            return 1;
        }

        for (int i = 0; i < NUM_DEVICES; i++)
        {
            fds[i].fd = devices[i];
            fds[i].events = POLLIN;
        }
    }

    while(1)
    {
        int ret = poll(fds, NUM_DEVICES, TIMEOUT);

        if (ret < 0)
        {
            perror("Poll\n");
        }

        else if (ret == 0)
        {
            perror("Timed out\n");
        }

        for (int i = 0; i < NUM_DEVICES; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                char c;
                if (read(devices[i], &c, 1) < 0)
                {
                    perror("Error while reading\n");
                    return 1;
                }
                printf("Read character '%c' from device %d\n", c, i);
            }
        }
    }

    return 0;
}