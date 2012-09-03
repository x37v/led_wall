#include <iostream>
#include <fstream>
#include <cstdlib>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>

using std::cout;
using std::endl;

//6 pairs of strips, each 64 leds long
//need to send 64 byte packets at a time
//pad out like it was 8 strips

#define NUM_LEDS_PER_PANEL (6 * 2 * 64)
#define NUM_LEDS (8 * NUM_LEDS_PER_PANEL)
#define NUM_LEDS_DIV8 (NUM_LEDS / 8)

uint8_t led[NUM_LEDS][3];

uint8_t reverse_bits(uint8_t b) {
   return
      (b & (0x1 << 0)) << 7 | 
      (b & (0x1 << 1)) << 5 | 
      (b & (0x1 << 2)) << 3 | 
      (b & (0x1 << 3)) << 1 | 
      (b & (0x1 << 4)) >> 1 | 
      (b & (0x1 << 5)) >> 3 | 
      (b & (0x1 << 6)) >> 5 | 
      (b & (0x1 << 7)) >> 7;
}

void write_latch(std::ofstream& out, unsigned int num_leds) {
   unsigned int zeros = 1;//((num_leds + 63) / 64) * 3;

   uint8_t z[64];
   memset(z, 0, 64);
   for (unsigned int i = 0; i < zeros; i++)
      out.write((char *)z, 64);
}

void draw(std::ofstream& out) {
   uint8_t packet[64];
   int packet_byte = 0;

   for (unsigned int i = 0; i < (NUM_LEDS / 8); i++) {
      for (unsigned int c = 0; c < 3; c++) {
         uint8_t local[8];
         for (int j = 0; j < 8; j++)
            local[j] = reverse_bits(0x80 | led[i + j * NUM_LEDS_DIV8][c]);

         for (unsigned int bit = 0; bit < 8; bit++) {
            packet[packet_byte] = 0;
            for (unsigned int l = 0; l < 8; l++)
               packet[packet_byte] |= (((local[l] & (1 << bit)) >> bit) << l);

            packet_byte++;
            if (packet_byte == 64) {
               out.write((char *)packet, 64);
               out.flush();
               packet_byte = 0;
            }
         }
      }
   }
}

int main(int argc, char * argv[]) {
   std::ofstream serial(argv[1], std::ios_base::binary);
   int program = 0;
   if (argc > 2)
      program = atoi(argv[2]);

   cout << "program: " << program << endl;

   //init

   if (!serial.is_open()) {
      cout << "cannot open serial" << endl;
      exit(-1);
   }
   memset(led, 0, NUM_LEDS * 3);

   int i = 0;
   int j = 0;
   int k = 0;

   switch (program) {
      default:
      case 0:
         while(1) {
            memset(led, 0, NUM_LEDS * 3);
            for (k = 0; k < 8; k++) {
              led[i + k * NUM_LEDS_PER_PANEL][j] = 12;
            }

            usleep(10);
            draw(serial);

            usleep(10);
            write_latch(serial, NUM_LEDS);
            serial.flush();

            i += 1;
            if (i >= NUM_LEDS_PER_PANEL) {
               i = 0;
               j = (j + 1) % 3;
            }
         }
      case 1:
         while(1) {
            memset(led, 0, NUM_LEDS * 3);

            //led[i][j] = 127;
            for (int k = 0; k < NUM_LEDS; k++) {
               switch((k + j) % 3) {
                  case 0:
                     led[k][0] = 50;
                     break;
                  case 1:
                     led[k][1] = 50;
                     break;
                  case 2:
                     led[k][2] = 50;
                     break;
               }
            }

            usleep(10);
            draw(serial);

            usleep(10);
            write_latch(serial, NUM_LEDS);
            serial.flush();

            i += 1;
            if (i >= 4) {
               i = 0;
               j = (j + 1) % 3;
            }
         }
   }
   serial.close();
}
