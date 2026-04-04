#include <signal.h>
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

enum
{
    TEST_CAN_ID = 0x123,
    TEST_STATUS = 0x01,
    TEST_LOCATION = 0x10
};

int main (int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "Use: %s can0\n", argv[0]); // NOLINT
        return EXIT_FAILURE;
    }

    int can_socket = socket (PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0)
    {
        perror ("Socket creation failed\n");
        return EXIT_FAILURE;
    }

    struct ifreq ifr = { 0 };
    strncpy (ifr.ifr_name, argv[1], IF_NAMESIZE - 1); // NOLINT
    ifr.ifr_name[IF_NAMESIZE - 1] = '\0';
    if (ioctl (can_socket, SIOCGIFINDEX, &ifr) < 0)
    {
        perror ("Interface not found\n");
        close (can_socket);
        return EXIT_FAILURE;
    }

    struct sockaddr_can addr = { 0 };
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind (can_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        perror ("Bind failed\n");
        close (can_socket);
        return EXIT_FAILURE;
    }

    struct can_frame frame;
    frame.can_id = TEST_CAN_ID;
    frame.len = 2;
    frame.data[0] = TEST_STATUS;   // STATUS_ERROR
    frame.data[1] = TEST_LOCATION; // LOC_FRONT

    if (write (can_socket, &frame, sizeof (struct can_frame)) != (int)sizeof (struct can_frame))
    {
        perror ("Write failed\n");
        close (can_socket);
        return EXIT_FAILURE;
    }
    close (can_socket);
    return EXIT_SUCCESS;
}