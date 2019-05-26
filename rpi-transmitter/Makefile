
all:		main.cpp transmitter.o mylib.a 
	g++ -g -o rpi-transmitter main.cpp transmitter.o mylib.a -lm -lpthread -lstdc++

mylib.a:	dma.o dsp.o gpio.o iqdmasync.o mailbox.o util.o raspberry_pi_revision.o
	ar -crv mylib.a dma.o dsp.o  gpio.o iqdmasync.o mailbox.o util.o raspberry_pi_revision.o

dma.o	: dma.cpp dma.h
	g++ -g -c dma.cpp

dsp.o	: dsp.cpp dsp.h
	g++ -g -c dsp.cpp

gpio.o	: gpio.cpp gpio.h
	g++ -g -c gpio.cpp

iqdmasync.o :iqdmasync.cpp iqdmasync.h
	g++ -g -c iqdmasync.cpp

mailbox.o	: mailbox.c
	g++ -g -c mailbox.c

util.o		: util.cpp
	g++ -g -c util.cpp

raspberry_pi_revision.o	: raspberry_pi_revision.c
	g++ -g -c raspberry_pi_revision.c

transmitter.o	: transmitter.cpp transmitter.h ringbuffer.h
	g++ -g -c transmitter.cpp

clean	:
	rm *.o mylib.a rpi-transmitter

