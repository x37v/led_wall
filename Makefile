SRC = capture.c \
		led_driver.cpp
OBJ = capture.o \
		led_driver.o

#CFLAGS += -g -O2
CFLAGS += -O3

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $*.o $<

.cpp.o:
	@echo CXX $<
	@${CXX} -c ${CFLAGS} -lstdc++ -o $*.o $<

capture: ${OBJ}
	@echo CXX $<
	@${CXX} -o $@ ${OBJ}

clean:
	rm -f capture *.o

program:
	teensy_loader_cli -mmcu=at90usb1286 -w Serial2Parallel.cpp.hex
