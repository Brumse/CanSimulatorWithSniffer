#ifndef OLEDI2C_H
#define OLEDI2C_H

#include <stdint.h>

#define OLED_ADDR 0x3C
#define I2C_BUS "/dev/i2c-1"
#define WIDTH 128
#define HEIGHT 64
#define PAGES (HEIGHT / 8)

int oled_init (void);
void oled_clear (void);
void oled_draw_char (int column, int page, char character);
void oled_draw_string (int column, int page, const char *str);
void oled_flush (void);
void oled_close (void);

#endif