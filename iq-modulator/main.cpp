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
#include	"tcp-client.h"

#include	<sndfile.h>

std::atomic<bool> running;
static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
}

#define	BUF_SIZE	4800
float	buffer	[2 * BUF_SIZE];
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

uint64_t	nextStop;

	if (argc < 2) {
	   fprintf (stderr, "please specify an input file");
	   exit (1);
	}

	infile = sf_open (argv [1], SFM_READ, &sf_info);
	if (infile == NULL) {
	   fprintf (stderr, "could not open %s\n", argv [1]);
	   exit (1);
	}

	if (sf_info. samplerate != 96000) {
	   fprintf (stderr, "Inputrate is %d, should be 96000\n",
	                                      sf_info. samplerate);
	   exit (1);
	}

	for (int i = 0; i < 64; i++) {
           struct sigaction sa;

           std::memset (&sa, 0, sizeof(sa));
           sa. sa_handler = terminate;
           sigaction (i, &sa, NULL);
        }

	running. store (true);

	nextStop	= getMyTime ();
	while (running. load ()) {
	   uint64_t currTime = getMyTime ();
	   if (currTime < nextStop) 
	      usleep (nextStop - currTime);
	   nextStop += 50000;
	   int readCount	= sf_readf_float (infile, buffer, BUF_SIZE);
	   if (readCount <= 0) {
	      sf_seek (infile, 0, SEEK_SET);
	      fprintf (stderr, "eof reached\n");
	      usleep (100000);
	      continue;
	   }
	
	   for (int i = 0; i < readCount; i ++) {
	      outBuffer [2 * i] = (int16_t)(buffer [2 * i] * 16384);
	      outBuffer [2 * i + 1] = (int16_t)(buffer [2 * i + 1] * 16384);
	   }
	   if (running. load ())
	      theGenerator. sendData (outBuffer, readCount * 2);
	}
	theGenerator. stop ();
}
