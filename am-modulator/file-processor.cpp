#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include        <sndfile.h>
#include        <samplerate.h>
#include        <math.h>
#include        <atomic>
#include        <sys/time.h>
#include        <complex>
#include        "tcp-client.h"
#include        "file-processor.h"
#include	"iq-creator.h"

static inline
uint64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((uint64_t)tv. tv_sec * 1000000 + (uint64_t)tv. tv_usec);
}


	fileProcessor::fileProcessor (SNDFILE	*inputfile,
	                              SF_INFO	*sf_info,
	                              int	rate,
	                              tcpClient	*theGenerator,
	                              RingBuffer<std::complex<float>> *scopeBuffer) {
	this	-> inFile	= inputfile;
	this	-> sf_info	= sf_info;
	this	-> rate		= rate;
	this	-> theGenerator	= theGenerator;
	this	-> scopeBuffer	= scopeBuffer;
	this	-> running. store (true);
	start ();
}

	fileProcessor::~fileProcessor	(void) {
	if (isRunning ()) {
	   running. store (false);
           msleep (100);
           while (isRunning ()) {
              usleep (100);
           }
	}
}

void	fileProcessor::run	(void) {
	if (sf_info -> samplerate  == rate) 
	   processFileDirect ();
	else
	   processFileConvert ();
}

void	fileProcessor::processFileDirect (void) {
uint64_t	nextStop;
float	inBuffer [2 * rate / 20];
int	sampleCounter	= 0;

	running. store (true);
	nextStop	= getMyTime ();
	while (running. load ()) {
	   
	   int readCount        = sf_readf_float (inFile, inBuffer, rate / 20);
           if (readCount <= 0) {
              sf_seek (inFile, 0, SEEK_SET);
              fprintf (stderr, "eof reached\n");
	      sf_close (inFile);
              usleep (100000);
	      running. store (false);
	      return;
           }
	   sampleCounter	+= readCount;
	   if (sampleCounter >= rate / 4) {
	      sampleCounter -= rate / 4;
	      uint64_t currentTime	= getMyTime ();
	      if (nextStop > currentTime)
	      usleep (nextStop - currentTime);
	      nextStop += 1000000 / 4;
	   }
	   for (int i = 0; i < readCount; i ++) {
	      float sample	= inBuffer [i * sf_info -> channels];
	      if (sf_info -> channels == 2) {
	         sample += inBuffer [i * sf_info -> channels + 1];
	         sample /= 2;
	      }
	      std::complex<float> newS = 
	                   std::complex<float> (sample + 1, 0);
	      int16_t intSample [2];
	      intSample [0] = (int16_t)(real (newS) * 16384);
	      intSample [1] = (int16_t)(imag (newS) * 16384);
	      theGenerator -> sendData (intSample, 2);
	      scopeBuffer	-> putDataIntoBuffer (&newS, 1);
	      if (scopeBuffer -> GetRingBufferReadAvailable () >= 1024)
	         emit showSpectrum ();
	   }
	}
}
//
//	In most cases, the sample rate in the PCM file differs
//	from what we need, in which case we need a conversion
void	fileProcessor::processFileConvert (void) {
int32_t	outputLimit	= 2 * rate / 10;
double	ratio		= (double)rate / sf_info -> samplerate;
int	bufferSize	= ceil (outputLimit / ratio);
std::vector<float>	bi (bufferSize);
std::vector<float>	bo (outputLimit);
int	error;
int	sampleCounter	= 0;
uint64_t	nextStop;
SRC_STATE	*converter =
			src_new (SRC_SINC_BEST_QUALITY, 2, &error);
SRC_DATA	*src_data =
			new SRC_DATA;
iq_creator	makeIQ (2, 1);
	if (converter == NULL) {
	   fprintf (stderr, "error creating a converter %d\n", error);
	   return;
	}

	fprintf (stderr,
                "Starting converter with ratio %f (in %d, out %d)\n",
                                              ratio, sf_info -> samplerate,
	                                              rate);
	running. store (true);
	src_data -> data_in          = bi. data ();
	src_data -> data_out         = bo. data ();
	src_data -> src_ratio        = ratio;
	src_data -> end_of_input     = 0;
	nextStop                     = getMyTime ();
	while (running. load ()) {
	   int	readCount;
	   readCount	= sf_readf_float (inFile, bi. data (), bufferSize / 2);
           if (readCount <= 0) {
              sf_seek (inFile, 0, SEEK_SET);
              fprintf (stderr, "eof reached\n");
	      sf_close (inFile);
              usleep (100000);
	      running. store (false);
	      return;
           }

	   if (sf_info -> channels == 1) {
	      float temp [bufferSize / 2];
	      memcpy (temp, bi. data (), readCount * sizeof (float));
	      for (int j = 0; j < readCount; j ++) {
	         bi [2 * j] = temp [j];
	         bi [2 * j + 1] = 0;
	      }
	   }
	    
	   src_data	-> input_frames		= readCount;
	   src_data	-> output_frames	= outputLimit / 2;
	   int res	= src_process (converter, src_data);
//
//	The samplerate is now "rate", we do not want to overload
//	the socket, so we built in a break after rate/4 samples
	   sampleCounter	+= src_data -> output_frames_gen;
	   if (sampleCounter > rate / 4) {
	      sampleCounter -= rate / 4;
	      uint64_t currentTime	= getMyTime ();
	      if (nextStop > currentTime)
	      usleep (nextStop - currentTime);
	      nextStop += 1000000 / 4;
	   }
	   for (int i = 0; i < src_data -> output_frames_gen; i ++) {
	      float sample	=
	                src_data -> data_out [i * sf_info -> channels];
	      if (sf_info -> channels == 2) {
	         sample += bo [i * sf_info -> channels + 1];
	         sample /= 2;
	      }
	      std::complex<float>xxx = std::complex<float> (sample + 1, 0);
//	      xxx = makeIQ. Pass (sample);
	      int16_t intSample [2];
	      intSample [0] = (int16_t)(real (xxx) * 16384);
	      intSample [1] = (int16_t)(imag (xxx) * 16384);
	      theGenerator -> sendData (intSample, 2);
	      scopeBuffer       -> putDataIntoBuffer (&xxx, 1);
	      if (scopeBuffer -> GetRingBufferReadAvailable () >= 1024)
	         emit showSpectrum ();
	   }
	}
}
