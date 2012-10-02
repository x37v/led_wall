CFLAGS += -I/usr/include/gstreamer-0.10/ -I/usr/include/glib-2.0/ -std=c99

all: gstledwallvideosink.so

gstledwallvideosink.o: gstledwallvideosink.c gstledwallvideosink.h
	gcc -Wall -fPIC $(CFLAGS) `pkg-config --cflags gstreamer-0.10 gstreamer-video-0.10 glib-2.0` -c -o gstledwallvideosink.o gstledwallvideosink.c

gstledwallvideosink.so: gstledwallvideosink.o
	gcc -shared -o gstledwallvideosink.so gstledwallvideosink.o `pkg-config --libs gstreamer-0.10 gstreamer-video-0.10 glib-2.0`

clean:
	rm -f *.so *.o
