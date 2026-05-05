#include "canTypes.h"

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
    case LOC_FRONT_RIGHT:
        return "FRONT_RIGHT";
    case LOC_FRONT_LEFT:
        return "FRONT_LEFT";
    case LOC_REAR:
        return "REAR";
    case LOC_REAR_RIGHT:
        return "REAR_RIGHT";
    case LOC_REAR_LEFT:
        return "REAR_LEFT";
    case LOC_LEFT:
        return "LEFT";
    case LOC_RIGHT:
        return "RIGHT";
    default:
        return "UNKNOWN_LOC";
    }
}