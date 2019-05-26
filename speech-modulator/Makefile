
all:	speech-modulator

speech-modulator : main.cpp tcp-client.o iq-creator.o fft-filter.o ringbuffer.h audiosource.o
	g++ -o speech-modulator main.cpp iq-creator.o fft-filter.o tcp-client.o audiosource.o -lpthread -lsndfile -lsamplerate -lportaudio -lm -lstdc++

tcp-client.o	: tcp-client.cpp tcp-client.h ringbuffer.h
	g++ -I . -c tcp-client.cpp

audiosource.o	: audiosource.cpp audiosource.h
	g++ -c audiosource.cpp

fft-filter.o	: fft-filter.cpp fft-filter.h
	g++ -c fft-filter.cpp

iq-creator.o	: iq-creator.cpp iq-creator.h
	g++ -c iq-creator.cpp

clean	:
	rm *.o speech-modulator

