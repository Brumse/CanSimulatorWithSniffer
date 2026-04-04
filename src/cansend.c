#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main (int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "Use: %s can0\n", argv[0]);
        return EXIT_FAILURE;
    }

    int s = socket (PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0)
    {
        fprintf (stderr, "Socket creation failed\n");
        return EXIT_FAILURE;
    }

    struct ifreq ifr;
    memset (&ifr, 0, sizeof (ifr));
    strcpy (ifr.ifr_name, argv[1]);
    if (ioctl (s, SIOCGIFINDEX, &ifr) < 0)
    {
        fprintf (stderr, "Interface not found\n");
        close (s);
        return EXIT_FAILURE;
    }

    struct sockaddr_can addr;
    memset (&addr, 0, sizeof (addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind (s, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        fprintf (stderr, "Bind failed\n");
        close (s);
        return EXIT_FAILURE;
    }

    struct can_frame frame;
    frame.can_id = 0x123;
    frame.len = 5;
    memcpy (frame.data, "hello", 5);

    if (write (s, &frame, sizeof (struct can_frame)) != (int)sizeof (struct can_frame))
    {
        fprintf (stderr, "Write failed\n");
        close (s);
        return EXIT_FAILURE;
    }
    close (s);
    return EXIT_SUCCESS;
}