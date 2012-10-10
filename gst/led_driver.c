#include "led_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <termios.h>
#include <unistd.h>



//FILE * serial = NULL;
int serial = -1;
unsigned int num_leds;
unsigned int num_leds_div_8;
uint8_t * led = NULL;
#ifdef COMPUTE_FRAME_DELAY
struct timeval time_last;
#endif

//set_interface_attribs and set_blocking from http://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c

int set_interface_attribs (int fd, int speed, int parity) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    fprintf(stderr, "error %d from tcgetattr", errno);
    return -1;
  }

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // ignore break signal
  tty.c_lflag = 0;                // no signaling chars, no echo,
  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  //tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
  {
    fprintf(stderr, "error %d from tcsetattr", errno);
    return -1;
  }
  return 0;
}

void set_blocking (int fd, int should_block) {
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0) {
    fprintf(stderr, "error %d from tggetattr", errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    fprintf(stderr, "error %d setting term attributes", errno);
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

void write_latch(int out, unsigned int num_leds) {
   unsigned int zeros = 1;//((num_leds + 63) / 64) * 3;

   uint8_t z[64];
   memset(z, 0, 64);
   for (unsigned int i = 0; i < zeros; i++) {
      write(out, z, sizeof(char) * 64);
   }
}

void draw(int out) {
   uint8_t packet[64];
   int packet_byte = 0;
#ifdef COMPUTE_FRAME_DELAY
   struct timeval time_now;
#endif

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
               write(out, packet, sizeof(char) * 64);
               packet_byte = 0;
               memset(packet, 0, 64);
            }
         }
      }
/*
      if (packet_byte != 0) {
        fwrite(packet, sizeof(char), 64, out);
        packet_byte = 0;
      }
*/
   }

#ifdef COMPUTE_FRAME_DELAY
   memcpy(&time_last, &time_now, sizeof(struct timeval));
   gettimeofday(&time_now, NULL);
   if (time_now.tv_usec > time_last.tv_usec) {
      printf("%d\n", (time_now.tv_sec - time_last.tv_sec - 1) * 1000000 + (time_now.tv_usec + 1000000 - time_last.tv_usec));
   } else {
      printf("%d\n", (time_now.tv_sec - time_last.tv_sec) * 1000000 + (time_now.tv_usec - time_last.tv_usec));
   }
#endif
}

int led_open_output(char * device_path, unsigned int led_count) {
   num_leds = led_count;
   num_leds_div_8 = num_leds / 8;

   led = (uint8_t *)malloc(sizeof(uint8_t) * 3 * num_leds);
   memset(led, 0, sizeof(uint8_t) * 3 * num_leds);

   //serial = fopen(device_path, "w");
   serial = open(device_path, O_RDWR | O_NOCTTY | O_SYNC);
   if (!serial) {
      fprintf(stderr, "cannot open serial\n");
      return 0;
   }
   set_interface_attribs(serial, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
   set_blocking(serial, 1);                // set blocking

#ifdef COMPUTE_FRAME_DELAY
   gettimeofday(&time_last, NULL);
#endif

   return 1;
}

void led_close() {
  if (serial != -1) {
    close(serial);
    serial = -1;
  }
}

void led_write_buffer(uint8_t * rgb_buffer) {
   //map buffer
   memcpy(led, rgb_buffer, 3 * num_leds * sizeof(uint8_t));
   
   usleep(2);
   draw(serial);

   usleep(2);
   write_latch(serial, num_leds);
}

