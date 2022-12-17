#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#define NUM_DEVICES 5

char generate_random_char();

int main (void)
{
    int devices[NUM_DEVICES];

    for (int i = 0; i < NUM_DEVICES; i++)
    {
        char device_name[64];
        sprintf(device_name, "/dev/shofer%d", i);
        devices[i] = open(device_name, 1);
        if (devices[i] < 0)
        {
            perror("Error while opening device\n");
            return 1;
        }
    }

    while (1)
    {
        struct pollfd fds[NUM_DEVICES];
        for (int i = 0; i < NUM_DEVICES; i++)
        {
            fds[i].fd = devices[i];
            fds[i].events = POLLOUT;
        }

        int ret = poll(fds, NUM_DEVICES, -1);
        if (ret < 0)
        {
            perror("Poll failed\n");
        }

        for (int i = 0; i < 5; i++)
        {
            char c = generate_random_char();
            if (fds[i].revents & POLLOUT)
            {
                write(fds[i].fd, &c, 1);
            }
        }

    }

    return 0;
}

char generate_random_char()
{
    return 'A' + (random() % 26);
}