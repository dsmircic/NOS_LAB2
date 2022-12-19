#include <stio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (void)
{
    char device_name[] = "/dev/message_queue";
    int device = open(device_name, O_WRONLY);

    if (device < 0)
    {
        printf("Error opening device %s", device_name);
        return -1;
    }

    
    while (1)
    {
        char message[];
        printf("Enter message: ");
        scanf("%s", message);

        if (write(device, message, strlen(message)) < 0)
            printf("Error writing to device %s", device_name);
        else
            printf("Message sent: %s\n", message);

        if (read(device, message, strlen(message)) < 0)
            printf("Error reading from device %s", device_name);
        else
            printf("Message received: %s\n", message);
    }

    return 0;
}