CC = g++
LIBS = -lncurses -lm 

all:
	${CC} -o cpid cpid.cpp ${LIBS}
clean:
	rm *.o
run:
	./cpid