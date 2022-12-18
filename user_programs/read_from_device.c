#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{

    char device_name[] = "/dev/shofer_out";
    int device = open(device_name, 0);

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
        printf("Read %c\n", c, device_name);
        sleep(1);
    }

    return 0;
}