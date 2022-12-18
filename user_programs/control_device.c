#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(void)
{

    char device_name[] = "/dev/shofer_control";
    int device = open(device_name, 1);

    if (device < 0) 
    {
        printf("Error opening device %s\n", device_name);
        return -1;
    }

    while (1) 
    {
        // listen for user input
        int d;
        scanf("%d", &d);
        if (ioctl(device, d) < 0) {
            printf("Error writing to device %s", device_name);
            return -1;
        } 
        else 
        {
            printf("Transerring %d bytes from shofer_in to shofer_out\n", d);
        }
    }

    return 0;
}