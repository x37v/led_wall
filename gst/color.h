#ifndef LEDWALL_COLOR_H
#define LEDWALL_COLOR_H

#include <inttypes.h>

#define BLACK_THRESH 22

inline uint8_t gamma_map(int v);
void ycrcb2rgb(int y, uint8_t cr, uint8_t cb, int * r, int * g, int * b);

#endif
