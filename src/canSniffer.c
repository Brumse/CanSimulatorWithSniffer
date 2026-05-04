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

static char ok_message_lines[TOTAL_MESSAGE][MAX_LINE_LEN];
static int ok_line_index = 0;
static int ok_line_count = 0;

static char error_message_lines[TOTAL_MESSAGE][MAX_LINE_LEN];
static int error_line_index = 0;
static int error_line_count = 0;

static OledDisplay display_ok;
static OledDisplay display_error;

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
        snprintf (out, out_size, "- %s %s", status_str, loc_str); // NOLINT
    }
    else
    {
        snprintf (out, out_size, "Invalid frame"); // NOLINT
    }
}

static void update_display (OledDisplay *display, char msg_lines[TOTAL_MESSAGE][MAX_LINE_LEN], int *index, int *count,
                            const struct can_frame *frame)
{
    char raw_line[MAX_LINE_LEN];
    char trans_line[MAX_LINE_LEN];
    format_raw_line (raw_line, sizeof (raw_line), frame);
    format_trans_line (trans_line, sizeof (trans_line), frame);

    strncpy (msg_lines[*index], raw_line, MAX_LINE_LEN - 1); // NOLINT
    msg_lines[*index][MAX_LINE_LEN - 1] = '\0';
    int next = (*index + 1) % TOTAL_MESSAGE;

    strncpy (msg_lines[next], trans_line, MAX_LINE_LEN - 1); // NOLINT
    msg_lines[next][MAX_LINE_LEN - 1] = '\0';

    *index = (*index + 2) % TOTAL_MESSAGE;
    if (*count < NUM_DISPLAY_LINES)
    {
        (*count)++;
    }

    oled_clear (display);
    int start_idx = (*index - (*count * 2) + TOTAL_MESSAGE) % TOTAL_MESSAGE;
    for (int i = 0; i < TOTAL_MESSAGE; i++)
    {
        if (i >= *count * 2)
        {
            break;
        }
        int line = (start_idx + i) % TOTAL_MESSAGE;
        oled_draw_string (display, 0, i, msg_lines[line]);
    }
    oled_flush (display);
}

void print_can_frame (const struct can_frame *frame)
{
    char raw_line[MAX_LINE_LEN];
    char trans_line[MAX_LINE_LEN];
    format_raw_line (raw_line, sizeof (raw_line), frame);
    format_trans_line (trans_line, sizeof (trans_line), frame);
    printf ("%s -> %s\n", raw_line, trans_line);

    if (frame->len >= 2 && frame->data[0] == STATUS_OK)
    {
        update_display (&display_ok, ok_message_lines, &ok_line_index, &ok_line_count, frame);
    }
    else
    {
        update_display (&display_error, error_message_lines, &error_line_index, &error_line_count, frame);
    }
}

static void splash_and_clear (OledDisplay *display, const char *label)
{
    oled_clear (display);
    oled_draw_string (display, 0, 3, label);
    oled_flush (display);
    usleep (SPLASH_DELAY_US);
    oled_clear (display);
    oled_flush (display);
}

int main (int argc, char **argv)
{
    signal (SIGINT, handle_exit);
    if (argc < 2)
    {
        fprintf (stderr, "Use: %s can0\n", argv[0]); // NOLINT
        return EXIT_FAILURE;
    }

    if (oled_init (&display_ok, "/dev/i2c-1") != 0)
    {
        fprintf (stderr, "Failed to init OK display (i2c-1)\n"); // NOLINT
        return EXIT_FAILURE;
    }
    if (oled_init (&display_error, "/dev/i2c-0") != 0)
    {
        fprintf (stderr, "Failed to init ERROR display (i2c-0)\n"); // NOLINT
        oled_close (&display_ok);
        return EXIT_FAILURE;
    }

    splash_and_clear (&display_ok, "CAN Sniffer (OK)");
    splash_and_clear (&display_error, "CAN Sniffer (ERR)");

    int can_socket = socket (PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0)
    {
        perror ("Socket creation failed\n");
        return EXIT_FAILURE;
    }

    struct ifreq ifr = { 0 };
    strncpy (ifr.ifr_name, argv[1], IF_NAMESIZE - 1); // NOLINT
    ifr.ifr_name[IF_NAMESIZE - 1] = '\0';             // NOLINT
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
    oled_close (&display_ok);
    oled_close (&display_error);
    close (can_socket);
    return EXIT_SUCCESS;
}
