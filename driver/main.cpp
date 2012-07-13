#include <iostream>
#include <fstream>
#include <cstdlib>
#include <inttypes.h>
#include <string.h>

using std::cout;
using std::endl;

//6 pairs of strips, each 64 leds long
//need to send 64 byte packets at a time
//pad out like it was 8 strips

#define NUM_LEDS (8 * 2 * 64)
#define NUM_LEDS_DIV8 (NUM_LEDS / 8)

uint8_t led[NUM_LEDS][3];

void write_latch(std::ofstream& out, unsigned int num_leds) {
   unsigned int zeros = ((num_leds + 63) / 64) * 3;
   uint8_t z[64];
   memset(z, 0, 64);
   for (unsigned int i = 0; i < zeros; i+= 64) {
      out.write((char *)z, 64);
   }
}

void draw(std::ofstream& out) {
   uint8_t packet[64];
   int packet_byte = 0;

   for (unsigned int i = 0; i < (NUM_LEDS / 8); i++) {
      for (unsigned int c = 0; c < 3; c++) {
         const uint8_t local[8] = {
            led[i][c],
            led[i +     NUM_LEDS_DIV8][c],
            led[i + 2 * NUM_LEDS_DIV8][c],
            led[i + 3 * NUM_LEDS_DIV8][c],
            led[i + 4 * NUM_LEDS_DIV8][c],
            led[i + 5 * NUM_LEDS_DIV8][c],
            led[i + 6 * NUM_LEDS_DIV8][c],
            led[i + 7 * NUM_LEDS_DIV8][c],
         };

         for (unsigned int bit = 0; bit < 8; bit++) {
            packet[packet_byte] = 0;
            for (unsigned int l = 0; l < 8; l++)
               packet[packet_byte] |= (((local[l] & (1 << bit)) >> bit) << l);
            packet_byte++;
            if (packet_byte == 64) {
               out.write((char *)packet, 64);
               packet_byte = 0;
            }
         }

      }
   }
}

int main(int argc, char * argv[]) {
   std::ofstream serial(argv[1], std::ios_base::binary);

   //init
   for (int i = 0; i < NUM_LEDS; i++) {
      led[i][0] = 0x80 | 127;
      led[i][1] = 0x80;
      led[i][2] = 0x80;
   }

   if (!serial.is_open()) {
      cout << "cannot open serial" << endl;
      exit(-1);
   }

   draw(serial);
   write_latch(serial, NUM_LEDS);

   serial.flush();
   serial.close();
}
