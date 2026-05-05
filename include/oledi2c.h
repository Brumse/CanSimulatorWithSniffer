#ifndef OLEDI2C_H
#define OLEDI2C_H

#include <stdint.h>

#define OLED_ADDR 0x3C
#define I2C_BUS "/dev/i2c-1"
#define I2C_BUS2 "/dev/i2c-0"
#define WIDTH 128
#define HEIGHT 64
#define PAGES (HEIGHT / 8)

typedef struct
{
    int fd;
    uint8_t framebuffer[WIDTH * PAGES];
} OledDisplay;

int oled_init (OledDisplay *display, const char *i2c_bus);
void oled_clear (OledDisplay *display);
void oled_draw_char (OledDisplay *display, int column, int page, char character);
void oled_draw_string (OledDisplay *display, int column, int page, const char *str);
void oled_flush (OledDisplay *display);
void oled_close (OledDisplay *display);
#endif