#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{

    char device_name[] = "/dev/shofer_out";
    int device = open(device_name, O_RDONLY);

    if (device < 0)
    {
        printf("Error opening device %s\n", device_name);
        return -1;
    }

    while (1)
    {
        char c;
        if (read(device, &c, 1) < 0)
        {
            printf("Error reading from device %s", device_name);
            return -1;
        }
        printf("Read: %c\n", c);
        sleep(1);
    }

    return 0;
}