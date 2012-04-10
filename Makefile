SRC = capture.c
#OBJ = ${SRC:.c=.o}
#
#.c.o:
#	@echo CC $<
#	@${CC} -c ${CFLAGS} -o $*.o $<
#
#capture: ${OBJ}
#	@${CC} -o $@ ${OBJ}

capture: ${SRC}
	@${CC} -o $@ ${SRC}

clean:
	rm -f capture *.o
