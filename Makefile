SRC = capture.c
OBJ = ${SRC:.c=.o}

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $*.o $<

capture: ${OBJ}
	@${CC} -o $@ ${OBJ}

clean:
	rm -f capture *.o
