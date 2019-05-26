
all:	pcm-handler

pcm-handler : main.cpp ringbuffer.h pcm-handler.o pcm-driver.o rds-driver.o
	g++ -g -o pcm-handler main.cpp pcm-handler.o pcm-driver.o rds-driver.o -lpthread -lsndfile -lsamplerate -lm -lstdc++

pcm-driver.o	: pcm-driver.cpp pcm-driver.h ringbuffer.h
	g++ -I . -c pcm-driver.cpp

rds-driver.o	: rds-driver.cpp rds-driver.h ringbuffer.h
	g++ -I . -c rds-driver.cpp

pcm-handler.o	: pcm-handler.cpp pcm-handler.h
	g++ -g -c pcm-handler.cpp

clean	:
	rm *.o pcm-handler

