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

#define PREEMP_IIR(sig, sigp, outp) \
        (5.30986 * sig + -4.79461 * sigp + 0.48475 * outp)

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
	pos			= 0;
	nextPhase		= 0;
	samp			= {0, 0};
	samp_s			= {0, 0},
	samp_p			= {0, 0};
	samp_sp			= {0, 0};

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

	   modulateData (bo. data (),
	                 src_data -> output_frames_gen, sf_info -> channels);
	}
}

void	fileProcessor::modulateData (float *bo, int amount, int channels) {
int	i;
float sample;

	for (i = 0; i < amount; i ++) {
	   double clock		= 2 * M_PI * (double)pos * 19000.0 / rate;
	   double pilot		= sin (clock);
	   double carrier	= sin (2 * clock);

	   samp_s.l  = bo [i * channels];
	   if ((channels == 2) && (rate >= 192000)) {
	      samp_s.r		= bo [i * channels + 1];
	      samp.l		= PREEMP_IIR (samp_s.l, samp_sp.l, samp_p.l);
	      samp.r		= PREEMP_IIR (samp_s.r, samp_sp.r, samp_p.r);

	      double mono	= (samp.l + samp.r);
	      double lmr	= samp.l - samp.r;
	      sample		= 0.55 * mono + 0.1 * pilot +
                                              0.35 * lmr * carrier;
	      samp_sp.l = samp_s.l;
	      samp_sp.r = samp_s.r;
	      samp_p.l  = samp.l;
	      samp_p.r  = samp.r;
	      pos++;
	   }
	   else
	   if (channels == 2) 
	      sample	= (samp.l + bo [i * 2]) / 2;
	   else
	      sample	= samp_s.l;

	   nextPhase += 5 * sample;
	   if (nextPhase >= 2 * M_PI)
	      nextPhase -= 2 * M_PI;

	   std::complex<float>xxx =
	              std::complex<float> (cos (nextPhase), sin (nextPhase));

	   int16_t intSample [2];
	   intSample [0] = (int16_t)(real (xxx) * 16384);
	   intSample [1] = (int16_t)(imag (xxx) * 16384);
	   theGenerator -> sendData (intSample, 2);
	   scopeBuffer       -> putDataIntoBuffer (&xxx, 1);
	   if (scopeBuffer -> GetRingBufferReadAvailable () >= 1024)
	      emit showSpectrum ();
	}
}
