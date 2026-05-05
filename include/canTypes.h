#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>

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
    LOC_FRONT_RIGHT = 0x11,
    LOC_FRONT_LEFT = 0x12,
    LOC_REAR = 0x20,
    LOC_REAR_RIGHT = 0x21,
    LOC_REAR_LEFT = 0x22,
    LOC_LEFT = 0x30,
    LOC_RIGHT = 0x40,
    LOC_UNKNOWN = 0x00,
} ComponentLocation;

const char *status_to_str (uint8_t status);
const char *location_to_str (uint8_t loc);

#endif