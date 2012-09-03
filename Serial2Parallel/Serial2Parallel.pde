/* Maximum speed USB Serial to Parallel Output
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2012 Paul Stoffregen, PJRC.COM, LLC (paul@pjrc.com)
 * 
 * This highly optimized example was inspired by Phillip Burgess's work at
 * (http://www.paintyourdragon.com) with Adafruit's LPD light strips.
 *
 * Development of this code was funded by PJRC, from sales of Teensy boards.
 * While you may use this code for any purpose, please consider buying Teensy,
 * to support more optimization work in the future.  :-)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This example converts USB Serial to 8 bit parallel data.  The computer MUST
// transmit in multiples of 64 bytes.  The data bits are output on port D and
// clock signals are output in port B.

#include "usb_private.h"

#define NUM_DIGITAL_PINS 28

void setup() {
	for (int i=0; i < NUM_DIGITAL_PINS; i++) {
		pinMode(i, OUTPUT);
	}
}

void loop() {
	moveDataReallyFast(0xFF, 0x00);
}

// these delays allow for more reliable transmission on long wires
//   6 total "nop" are recommended for best speed
//   9 total may still allow max speed, if you're lucky...
//   more than 9 will slow the speed

#define Setup() asm volatile("nop\nnop\nnop\n")	// setup time before rising edge
#define Hold()  asm volatile("nop\nnop\n")	// hold time after falling edge
#define Pulse()  asm volatile("nop\n")		// lengthen clock pulse width

void moveDataReallyFast(byte strobeOn, byte strobeOff)
{
	unsigned char c;

	if (!usb_configuration) return;
	UENUM = CDC_RX_ENDPOINT;
	while (1) {
		c = UEINTX;
		if (!(c & (1<<RWAL))) {
			if (c & (1<<RXOUTI)) UEINTX = 0x6B;
			return;
		}
		c = UEDATX;
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		c = UEDATX;
		Pulse();
		PORTB = strobeOff;
		Hold();
		PORTD = c;
		Setup();
		PORTB = strobeOn;
		UEINTX = 0x6B;
		Pulse();
		PORTB = strobeOff;
	}
}

#if !defined (CORE_TEENSY_SERIAL)
#error "This program was designed for Teensy 2.0 using Tools > USB Type set to Serial.  Please set the Tools > Board to Teensy 2.0 and USB Type to Serial, or delete this error to try using a different board."
#endif
