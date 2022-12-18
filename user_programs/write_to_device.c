#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char generate_random_char();

int main(void)
{

    char device_name[] = "/dev/shofer_in";
    int device = open(device_name, 1);

    if (device < 0)
    {
        printf("Error opening device %s\n", device_name);
        return -1;
    }

    while (1)
    {
        char c = generate_random_char();
        if (write(device, &c, 1) < 0)
        {
            printf("Error writing to device %s", device_name);
            return -1;
        }
        printf("Wrote %c to device %s\n", c, device_name);
        sleep(1);
    }

    return 0;
}

char generate_random_char()
{
    return (char)(rand() % 26 + 65);
}