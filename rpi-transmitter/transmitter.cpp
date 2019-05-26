#include	"transmitter.h"

#define MAX_SAMPLERATE 200000
#define IQBURST 4000

	Transmitter::Transmitter (int frequency, int sampleRate, int harmonic):
	                           iqtest (frequency, sampleRate,
	                                     14, 4 * IQBURST, MODE_IQ) {
	this	-> harmonic	= harmonic;
	FifoSize		= IQBURST * 4;
	theBuffer		= new RingBuffer<int16_t> (4 * 32768);
	iqtest. SetPLLMasterLoop (3,4,0);
//	iqtest. print_clock_tree ();
//	iqtest. SetPLLMasterLoop(5,6,0);
	threadHandle	= std::thread (&Transmitter::run, this);
}

	Transmitter::~Transmitter	(void) {
	if (running. load ()) {
	   running. store (false);
	   usleep (1000);
	   threadHandle. join ();
	}
	iqtest.stop ();
	delete theBuffer;
}

void	Transmitter::send (int16_t *buffer, int size) {
	while (theBuffer -> GetRingBufferWriteAvailable () < size)
	   usleep (100);
	theBuffer	-> putDataIntoBuffer (buffer, size);
}

static
std::complex<float> CIQBuffer [IQBURST];	
static
int16_t IQBuffer [IQBURST * 2];
void	Transmitter::run (void) {

	running. store (true);
	while (running. load ()) {
	   int CplxSampleNumber = 0;
	   while (running. load () &&
	         (this -> theBuffer -> GetRingBufferReadAvailable () < IQBURST * 2))
	      usleep (100);
	   int nbread = this -> theBuffer -> getDataFromBuffer (IQBuffer, IQBURST * 2);

	   if (nbread > 0) {
	      for (int i = 0; i < nbread / 2; i++) {
	          CIQBuffer [i] =
	                  std::complex<float> (IQBuffer [i * 2    ] / 32768.0,
	                                       IQBuffer [i * 2 + 1] / 32768.0); 
	      }
	   }
	   for (int i = nbread / 2; i < IQBURST; i ++)
	      CIQBuffer [i] = std::complex<float> (0, 0);

	   iqtest. SetIQSamples (CIQBuffer, IQBURST, harmonic);
	}
	fprintf (stderr, "transmitter stops\n");
}	

