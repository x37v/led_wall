SRC = \
			gstledwallvideosink.c  \
			color.c \
			led_driver.cpp
		
OBJ = \
			gstledwallvideosink.o  \
			color.o \
			led_driver.o

CFLAGS += -I/usr/include/gstreamer-0.10/ -I/usr/include/glib-2.0/ -Wall
CFLAGS += `pkg-config --cflags gstreamer-0.10 gstreamer-video-0.10 glib-2.0` -I..
CFLAGS += -g -O2 -fPIC
#CFLAGS += -O3
LDFLAGS += `pkg-config --libs gstreamer-0.10 gstreamer-video-0.10 glib-2.0` -lstdc++

all: gstledwallvideosink.so

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -std=c99 -o $*.o $<

.cpp.o:
	@echo CXX $<
	@${CXX} -c ${CFLAGS} -o $*.o $<

gstledwallvideosink.so: ${OBJ}
	gcc -shared -o gstledwallvideosink.so ${LDFLAGS} ${OBJ}

clean:
	rm -f *.so ${OBJ}
