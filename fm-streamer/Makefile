
all:	fm-streamer

fm-streamer : main.cpp ringbuffer.h fm-streamer.o pcm-input.o rds-input.o tcp-output.o file-driver.o
	g++ -g -o fm-streamer main.cpp fm-streamer.o pcm-input.o rds-input.o tcp-output.o file-driver.o -lpthread -lsndfile -lsamplerate -lm -lstdc++

pcm-input.o	: pcm-input.cpp pcm-input.h fm-streamer.h ringbuffer.h
	g++ -I . -c pcm-input.cpp

rds-input.o	: rds-input.cpp rds-input.h fm-streamer.h ringbuffer.h
	g++ -I . -c rds-input.cpp

tcp-output.o	: tcp-output.cpp tcp-output.h  output-driver.h ringbuffer.h
	g++ -I . -c tcp-output.cpp

file-driver.o	: file-driver.cpp file-driver.h output-driver.h
	g++ -I . -c file-driver.cpp

fm-modulator.o	: fm-modulator.cpp fm-modulator.h
	g++ -g -c fm-modulator.cpp

clean	:
	rm *.o fm-streamer

