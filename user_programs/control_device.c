#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{

    char device_name[] = "/dev/shofer_control";
    int device = open(device_name, O_RDONLY);

    if (device < 0) 
    {
        printf("Error opening device %s\n", device_name);
        return -1;
    }

    while (1) 
    {
        // listen for user input
        int d;
        printf("Enter number of bytes to transfer: ");
        scanf("%d", &d);
        if (ioctl(device, d) < 0) {
            printf("Error in ioctl call \n");
            return -1;
        } 
        else 
        {
            printf("Transerring %d bytes from shofer_in to shofer_out\n", d);
        }
    }

    return 0;
}