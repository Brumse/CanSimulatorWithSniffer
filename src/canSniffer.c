#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "canTypes.h"
#include "oledi2c.h"

static volatile sig_atomic_t keep_running = 1;

enum
{
    MAX_LINE_LEN = 21,
    NUM_DISPLAY_LINES = 4,
    TOTAL_MESSAGE = NUM_DISPLAY_LINES * 2,
    SPLASH_DELAY_US = 2000000
};

static char message_lines[TOTAL_MESSAGE][MAX_LINE_LEN];
static int line_index = 0;
static int line_count = 0;

void handle_exit (int sig)
{
    (void)sig;
    keep_running = 0;
}

static void format_raw_line (char *out, size_t out_size, const struct can_frame *frame)
{
    if (frame->len >= 2)
    {
        snprintf (out, out_size, "%03X %02X %02X", frame->can_id, frame->data[0], frame->data[1]); // NOLINT
    }
    else
    {
        snprintf (out, out_size, "%03X (short)", frame->can_id); // NOLINT
    }
}

static void format_trans_line (char *out, size_t out_size, const struct can_frame *frame)
{
    if (frame->len >= 2)
    {
        const char *status_str = status_to_str (frame->data[0]);
        const char *loc_str = location_to_str (frame->data[1]);
        snprintf (out, out_size, "%s %s", status_str, loc_str); // NOLINT
    }
    else
    {
        snprintf (out, out_size, "Invalid frame"); // NOLINT
    }
}

void print_can_frame (const struct can_frame *frame)
{
    char raw_line[MAX_LINE_LEN];
    char trans_line[MAX_LINE_LEN];
    format_raw_line (raw_line, sizeof (raw_line), frame);
    format_trans_line (trans_line, sizeof (trans_line), frame);
    printf ("%s -> %s\n", raw_line, trans_line);

    strncpy (message_lines[line_index], raw_line, MAX_LINE_LEN - 1); // NOLINT
    message_lines[line_index][MAX_LINE_LEN - 1] = '\0';
    int next_line = (line_index + 1) % TOTAL_MESSAGE;
    strncpy (message_lines[next_line], trans_line, MAX_LINE_LEN - 1); // NOLINT
    message_lines[next_line][MAX_LINE_LEN - 1] = '\0';

    line_index = (line_index + 2) % TOTAL_MESSAGE;
    if (line_count < NUM_DISPLAY_LINES)
    {
        line_count++;
    }

    oled_clear ();
    int start_idx = (line_index - (line_count * 2) + TOTAL_MESSAGE) % TOTAL_MESSAGE;
    for (int i = 0; i < TOTAL_MESSAGE; i++)
    {
        if (i >= line_count * 2)
        {
            break;
        }
        int idx = (start_idx + i) % TOTAL_MESSAGE;
        oled_draw_string (0, i, message_lines[idx]);
    }
    oled_flush ();
}

int main (int argc, char **argv)
{
    signal (SIGINT, handle_exit);
    if (argc < 2)
    {
        fprintf (stderr, "Use: %s can0\n", argv[0]); // NOLINT
        return EXIT_FAILURE;
    }
    if (oled_init () != 0)
    {
        fprintf (stderr, "Failed to initialise OLED\n"); // NOLINT
        return EXIT_FAILURE;
    }
    oled_clear ();
    oled_draw_string (0, 3, "CAN Sniffer");
    oled_flush ();
    usleep (SPLASH_DELAY_US);
    oled_clear ();
    oled_flush ();

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
    oled_close ();
    close (can_socket);
    return EXIT_SUCCESS;
}
