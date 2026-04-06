#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <assert.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdint.h>

#include "canTypes.h"

#define INFLUX_URL "http://localhost:8181/api/v3/write_lp?db=CAN_data"
#define INFLUX_BASE_URL "http://localhost:8181"
#define WRITE_EP "/api/v3/write_lp"
#define INFLUX_PARAMS "?db=CAN_data"
#define CREATE_INFLUX_URL(s) INFLUX_BASE_URL s INFLUX_PARAMS

enum
{
    QUERY_BUF_SIZE = 256
};
enum
{
    HTTP_OK = 200,
    HTTP_NO_CONTENT = 204
};

static volatile sig_atomic_t keep_running = 1;

void handle_exit (int sig)
{
    (void)sig;
    keep_running = 0;
}

void print_can_frame (const struct can_frame *frame)
{
    printf ("%X  %d  ", frame->can_id, frame->len);
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

void write_to_influx (const struct can_frame *frame, CURL *curl)
{
    assert (frame != NULL && "null frame ptr in write_to_influx");
    assert (curl != NULL && "null curl ptr in write_to_influx");

    char data[QUERY_BUF_SIZE];
    memset (data, '\0', QUERY_BUF_SIZE);                                                      // NOLINT
    snprintf (data, sizeof (data), "can_stats,id=%d loc_str=\"%s\",status=%d", frame->can_id, // NOLINT
              location_to_str (frame->data[1]), frame->data[0]);

    curl_easy_setopt (curl, CURLOPT_URL, CREATE_INFLUX_URL (WRITE_EP)); // NOLINT
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, data);

    CURLcode res = curl_easy_perform (curl);
    if (res != CURLE_OK)
    {
        fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res)); // NOLINT
    }
    else
    {
        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == HTTP_NO_CONTENT || http_code == HTTP_OK)
        {
            printf ("Data sent successfully to InfluxDB\n");
        }
        else
        {
            fprintf (stderr, "InfluxDB returned HTTP %ld\n", http_code); // NOLINT
        }
    }
}

int main (int argc, char **argv)
{
    curl_global_init (CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init ();
    if (!curl)
    {
        fprintf (stderr, "Failed to initialize CURL\n"); // NOLINT
        return EXIT_FAILURE;
    }
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

    struct ifreq ifr;                                 // NOLINT
    memset (&ifr, '\0', sizeof (ifr));                // NOLINT
    strncpy (ifr.ifr_name, argv[1], IF_NAMESIZE - 1); // NOLINT
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
        write_to_influx (&frame, curl);
    }
    curl_easy_cleanup (curl);
    curl_global_cleanup ();
    close (can_socket);
    return EXIT_SUCCESS;
}
