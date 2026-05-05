#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdint.h>

#include "canTypes.h"

static volatile sig_atomic_t keep_running = 1;

void handle_exit (int sig)
{
    (void)sig;
    keep_running = 0;
}

void print_can_frame (const struct can_frame *frame)
{
    printf ("%X   %d", frame->can_id, frame->len);
    for (int i = 0; i < frame->len; i++)
    {
        printf ("%02X ", frame->data[i]);
    }

    if (frame->len >= 2)
    {
        const char *status_str = status_to_str (frame->data[0]);
        const char *loc_str = location_to_str (frame->data[1]);

        printf (" Status: %s, Location: %s", status_str, loc_str);
    }

    printf ("\n");
}

int main (int argc, char **argv)
{
    signal (SIGINT, handle_exit);
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
    addr.can_ifindex = ifr.ifr_ifindex; // NOLINT
    if (bind (can_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        perror ("Bind failed\n");
        close (can_socket);
        return EXIT_FAILURE;
    }
    struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt (can_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof (struct timeval));
    while (keep_running)
    {
        struct can_frame frame;
        ssize_t nbytes = read (can_socket, &frame, sizeof (struct can_frame));
        if (nbytes < 0)
        {
            continue;
        }
        if (nbytes < (int)sizeof (struct can_frame))
        {
            perror ("Incomplete frame\n");
            continue;
        }

        print_can_frame (&frame);
    }
    close (can_socket);
    return EXIT_SUCCESS;
}
