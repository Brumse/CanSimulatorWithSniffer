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

#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdint.h>

#define INFLUX_URL "http://localhost:8086/write?db=CAN_Data"

static volatile sig_atomic_t keep_running = 1;

typedef enum
{
    STATUS_OK = 0x00,
    STATUS_ERROR = 0x01,
    STATUS_WARNING = 0x02,
    STATUS_UNKNOWN = 0xFF
} SystemStatus;

typedef enum
{
    LOC_FRONT = 0x10,
    LOC_REAR = 0x20,
    LOC_LEFT = 0x30,
    LOC_RIGHT = 0x40,
    LOC_UNKNOWN = 0x00
} ComponentLocation;

const char *status_to_str (uint8_t status)
{
    switch (status)
    {
    case STATUS_OK:
        return "OK";
    case STATUS_ERROR:
        return "ERROR";
    case STATUS_WARNING:
        return "WARNING";
    default:
        return "UNKNOWN_STATUS";
    }
}
const char *location_to_str (uint8_t loc)
{
    switch (loc)
    {
    case LOC_FRONT:
        return "FRONT";
    case LOC_REAR:
        return "REAR";
    case LOC_LEFT:
        return "LEFT";
    case LOC_RIGHT:
        return "RIGHT";
    default:
        return "UNKNOWN_LOC";
    }
}

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
void init_influx_db() {
    CURL *curl = curl_easy_init();
    if (!curl) return;

    // InfluxDB 1.8 uses the /query endpoint to create databases
    // URL-encoded: CREATE DATABASE CAN_Data
    const char* init_url = "http://localhost:8086/query?q=CREATE+DATABASE+CAN_Data";

    curl_easy_setopt(curl, CURLOPT_URL, init_url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // We don't need a body for this specific GET/POST query
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ""); 

    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to connect to InfluxDB: %s\n", curl_easy_strerror(res));
        sleep(5);
        res = curl_easy_perform(curl);
        return;
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200) {
            printf("Database 'CAN_Data' initialized or already exists.\n");
        }
    }

    curl_easy_cleanup(curl);
}

void log_to_influx (const struct can_frame *frame)
{
    CURL *curl = curl_easy_init ();
    if (!curl)
    {
        fprintf (stderr, "Failed to initialize CURL\n");
        return;
    }

    char data[256];
    snprintf (data, sizeof (data), "can_stats,id=%X,loc=%s status=%d", frame->can_id, location_to_str (frame->data[1]),
              frame->data[0]);

    curl_easy_setopt (curl, CURLOPT_URL, INFLUX_URL);
    curl_easy_setopt (curl, CURLOPT_POSTFIELDS, data);

    CURLcode res = curl_easy_perform (curl);
    if (res != CURLE_OK)
    {
        fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
    }
    else
    {
        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 204)
        {
            printf ("[DB] Data sent successfully to InfluxDB\n");
        }
        else
        {
            fprintf (stderr, "InfluxDB returned HTTP %ld\n", http_code);
        }
    }
    curl_easy_cleanup (curl);
}

int main (int argc, char **argv)
{
    curl_global_init (CURL_GLOBAL_ALL);
    init_influx_db();
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
        log_to_influx (&frame);
    }

    curl_global_cleanup ();
    close (can_socket);
    return EXIT_SUCCESS;
}
