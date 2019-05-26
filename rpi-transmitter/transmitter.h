
#ifndef	__TRANSMITTER__
#define	__TRANSMITTER__

#include        <unistd.h>
#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<thread>
#include	<atomic>
#include	"ringbuffer.h"
#include        "raspberry_pi_revision.h"
#include        "util.h"
#include        "mailbox.h"
#include        "iqdmasync.h"
#include        "gpio.h"
#include        "dma.h"

class Transmitter {
public:
	
        Transmitter	(int frequency, int samplerate, int harmonic);
	~Transmitter	(void);
void	send		(int16_t *, int);
void	run		(void);
private:
	int		harmonic;
	iqdmasync	iqtest;
        int		FifoSize;
        RingBuffer<int16_t> *theBuffer;
	std::thread	threadHandle;
	std::atomic<bool>	running;
};

#endif

