
all:	iq-modulator

iq-modulator	: main.cpp tcp-client.o ringbuffer.h
	g++ -o iq-modulator main.cpp tcp-client.o -lpthread -lsndfile -lm -lstdc++

tcp-client.o	: tcp-client.cpp tcp-client.h ringbuffer.h
	g++ -I . -c tcp-client.cpp

clean	:
	rm *.o iq-modulator

