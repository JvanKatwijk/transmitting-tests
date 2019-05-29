#include        <unistd.h>
#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<sys/time.h>
#include	<math.h>
#include	<atomic>
#include	<complex>
#include	<vector>
#include	"tcp-client.h"

#include	<sndfile.h>
#include	<samplerate.h>

std::atomic<bool> running;
static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
}

#define	BUF_SIZE	2048
int16_t	outBuffer [2 * BUF_SIZE];

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

int	main (int argc, char **argv) {
int	phase	= 0;
tcpClient theGenerator (8765, "127.0.0.1");
SNDFILE	*infile;
SF_INFO	sf_info;
int32_t		sampleCounter	= 0;
uint64_t	nextStop;
int		inputRate;
int		outputRate	= 192000;
int		outputLimit	= BUF_SIZE;
int		inputSize;
float		ratio;
std::vector<float>      bi (0);
std::vector<float>      bo (0);
int	error;
SRC_STATE       *converter =
                        src_new (SRC_SINC_MEDIUM_QUALITY, 2, &error);
SRC_DATA        *src_data =
                        new SRC_DATA;

        if (converter == NULL) {
           fprintf (stderr, "error creating a converter %d\n", error);
	   exit (1);
        }

	if (argc < 2) {
	   fprintf (stderr, "please specify an input file");
	   exit (1);
	}

	infile = sf_open (argv [1], SFM_READ, &sf_info);
	if (infile == NULL) {
	   fprintf (stderr, "could not open %s\n", argv [1]);
	   exit (1);
	}
	
	inputRate	= sf_info. samplerate;
	if ((sf_info. samplerate != 96000) && (sf_info. samplerate == 192000)) {
	   fprintf (stderr, "Inputrate is %d, should be 96000 or 192000\n",
	                                      sf_info. samplerate);
	   exit (1);
	}
	ratio	= (double)outputRate / inputRate;
	inputSize	= ceil (outputLimit / ratio);
	bi. resize (inputSize);
	bo. resize (outputLimit);

	for (int i = 0; i < 64; i++) {
           struct sigaction sa;

           std::memset (&sa, 0, sizeof(sa));
           sa. sa_handler = terminate;
           sigaction (i, &sa, NULL);
        }

	running. store (true);

	nextStop	= getMyTime ();
	while (running. load ()) {
	   int readCount	= sf_readf_float (infile, bi. data (), BUF_SIZE);
	   if (readCount <= 0) {
	      sf_seek (infile, 0, SEEK_SET);
	      fprintf (stderr, "eof reached\n");
	      usleep (100000);
	      continue;
	   }
	
	   src_data     -> input_frames         = readCount;
	   src_data     -> output_frames        = outputLimit / 2;
           int res      = src_process (converter, src_data);
	   for (int i = 0; i < src_data -> output_frames_gen; i ++) {
	      outBuffer [2 * i] = (int16_t)(bo [2 * i] * 16384);
	      outBuffer [2 * i + 1] = (int16_t)(bo [2 * i + 1] * 16384);
	   }
	   if (running. load ())
	      theGenerator. sendData (outBuffer, readCount * 2);
	   sampleCounter += src_data -> output_frames_gen;
	   if (sampleCounter > outputRate / 4) {
              sampleCounter             -= outputRate / 4;
              uint64_t currentTime      = getMyTime ();
              if (nextStop > currentTime)
                 usleep (nextStop - currentTime);
              nextStop += 1000000 / 4;
           }
	}
	theGenerator. stop ();
}
