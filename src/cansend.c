#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "canTypes.h"

enum
{
    SLEEP = 3000000,
    FRAME_ID = 0x100,
    LOC_INDEX_FRONT = 0,
    LOC_INDEX_FRONT_RIGHT = 1,
    LOC_INDEX_FRONT_LEFT = 2,
    LOC_INDEX_REAR = 3,
    LOC_INDEX_REAR_RIGHT = 4,
    LOC_INDEX_REAR_LEFT = 5,
    LOC_INDEX_LEFT = 6,
    LOC_INDEX_RIGHT = 7,
    NUM_FRAME = 8
};
static volatile sig_atomic_t keep_running = 1;
void handle_exit (int sig)
{
    (void)sig;
    keep_running = 0;
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
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind (can_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
        perror ("Bind failed\n");
        close (can_socket);
        return EXIT_FAILURE;
    }

    srand (time (NULL));
    while (keep_running)
    {
        for (int i = 0; i < NUM_FRAME && keep_running; i++)
        {
            struct can_frame frame;
            frame.can_id = FRAME_ID + i;
            frame.len = 2;

            int random = rand () % 3;
            if (random == 0)
            {
                frame.data[0] = STATUS_OK;
            }
            else if (random == 1)
            {
                frame.data[0] = STATUS_WARNING;
            }
            else
            {
                frame.data[0] = STATUS_ERROR;
            }

            switch (i)
            {
            case LOC_INDEX_FRONT:
                frame.data[1] = LOC_FRONT;
                break;
            case LOC_INDEX_FRONT_RIGHT:
                frame.data[1] = LOC_FRONT_RIGHT;
                break;
            case LOC_INDEX_FRONT_LEFT:
                frame.data[1] = LOC_FRONT_LEFT;
                break;
            case LOC_INDEX_REAR:
                frame.data[1] = LOC_REAR;
                break;
            case LOC_INDEX_REAR_RIGHT:
                frame.data[1] = LOC_REAR_RIGHT;
                break;
            case LOC_INDEX_REAR_LEFT:
                frame.data[1] = LOC_REAR_LEFT;
                break;
            case LOC_INDEX_LEFT:
                frame.data[1] = LOC_LEFT;
                break;
            case LOC_INDEX_RIGHT:
                frame.data[1] = LOC_RIGHT;
                break;
            default:
                frame.data[1] = LOC_UNKNOWN;
                break;
            }

            if (write (can_socket, &frame, sizeof (frame)) != (int)sizeof (frame))
            {
                perror ("Write failed");
                close (can_socket);
                return EXIT_FAILURE;
            }
            printf ("Sent: 0x%03X %02X %02X\n", frame.can_id, frame.data[0], frame.data[1]);
            usleep (SLEEP);
        }
    }
    close (can_socket);
    return EXIT_SUCCESS;
}