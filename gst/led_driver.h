#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <inttypes.h>

int led_open_output(char * device_path, unsigned int num_leds);
//one byte per r,g,b per pixel, so 3 * num_leds uint8_t. 
//starting upper left drawing rows at a time.
void led_write_buffer(uint8_t * rgb_buffer);

#endif
