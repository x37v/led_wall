#include "led_driver.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/time.h>

using std::cout;
using std::endl;

#define COMPUTE_FRAME_DELAY

namespace {
   std::ofstream serial;
   unsigned int num_leds;
   unsigned int num_leds_div_8;
   uint8_t * led = NULL;
   struct timeval time_last;
}

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
   struct timeval time_now;

   //we are writing for 7 panels at once
   for (unsigned int i = 0; i < (num_leds / 8); i++) {

     //for each color write a byte
      for (unsigned int c = 0; c < 3; c++) {
         uint8_t local[8];
         //address each panel
         for (int j = 0; j < 8; j++)
            local[j] = reverse_bits(0x80 | led[3 * (i + j * num_leds_div_8) + c]);

         for (unsigned int bit = 0; bit < 8; bit++) {
            packet[packet_byte] = 0;
            for (unsigned int l = 0; l < 8; l++)
               packet[packet_byte] |= (((local[l] & (1 << bit)) >> bit) << l);

            packet_byte++;
            if (packet_byte == 64) {
               out.write((char *)packet, 64);
               out.flush();
               packet_byte = 0;
               memset(packet, 0, 64);
            }
         }
      }
      /*
      if (packet_byte != 0) {
        out.write((char *)packet, 64);
        out.flush();
        packet_byte = 0;
      }
      */
   }

#ifdef COMPUTE_FRAME_DELAY
   memcpy(&time_last, &time_now, sizeof(struct timeval));
   gettimeofday(&time_now, NULL);
   if (time_now.tv_usec > time_last.tv_usec) {
      cout << (time_now.tv_sec - time_last.tv_sec - 1) * 1000000 + (time_now.tv_usec + 1000000 - time_last.tv_usec) << endl;
   } else {
      cout << (time_now.tv_sec - time_last.tv_sec) * 1000000 + (time_now.tv_usec - time_last.tv_usec) << endl;
   }
#endif
}

int led_open_output(char * device_path, unsigned int led_count) {
   num_leds = led_count;
   num_leds_div_8 = num_leds / 8;

   led = new uint8_t[3 * num_leds];
   memset(led, 0, sizeof(uint8_t) * 3 * num_leds);

   serial.open(device_path, std::ios_base::binary);
   if (!serial.is_open()) {
      cout << "cannot open serial" << endl;
      return 0;
   }

   gettimeofday(&time_last, NULL);

   return 1;
}

void led_write_buffer(uint8_t * rgb_buffer) {
   //map buffer
   memcpy(led, rgb_buffer, 3 * num_leds * sizeof(uint8_t));
   
   usleep(2);
   draw(serial);

   usleep(2);
   write_latch(serial, num_leds);
   serial.flush();
}

