
all:	fm-modulator

fm-modulator : main.cpp ringbuffer.h fm-modulator.o file-driver.o  tcp-driver.o
	g++ -g -o fm-modulator main.cpp fm-modulator.o file-driver.o tcp-driver.o -lpthread -lsndfile -lsamplerate -lm -lstdc++

tcp-driver.o	: tcp-driver.cpp tcp-driver.h  output-driver.h ringbuffer.h
	g++ -I . -c tcp-driver.cpp

file-driver.o	: file-driver.cpp file-driver.h output-driver.h
	g++ -I . -c file-driver.cpp

fm-modulator.o	: fm-modulator.cpp fm-modulator.h
	g++ -g -c fm-modulator.cpp

clean	:
	rm *.o fm-modulator

