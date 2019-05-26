#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the speech modulator
 *    speech modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    speech modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with speech modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include        <unistd.h>
#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<samplerate.h>
#include	<math.h>
#include	<atomic>
#include	<sys/time.h>
#include	<complex>
#include	"tcp-client.h"
#include	"audiosource.h"
#include	"iq-creator.h"
#include	"fft-filter.h"
std::atomic<bool> running;


static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
}


static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}
//
void    printHelp       (void) {
        fprintf (stderr, "speech modulator\n"
"\toptions are:\n"
"\ti fileName\t: use fileName as ini file\n"
"\tp number\t: use number as port number for the output\n"
"\tr number\t: use number as output sample rate\n"
"\tu string\t: use string as url of the transmitter\n"
"\ts string\t: transmit only sideband ('u' or 'l')"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {
audioSource	theMicrophone;
int	error;
uint64_t	nextStop;
int             opt;
std::string	url		= "127.0.0.1";
int             port            = 8765;
int             rate            = 96000;
int		band		= 0;

        while ((opt = getopt (argc, argv, "p:r:u:s:h:")) != -1) {
           switch (opt) {
              case 'p':
                 port           = atoi (optarg);
                 break;

	      case 'r':
                 rate           = atoi (optarg);
                 break;

              case 'u':
                 url            = std::string (optarg);
                 break;

	      case 's':
	         band		= optarg [0] == 'u' ? 1 : 2;
	         break;

              case 'h':
              default:
                 printHelp ();
                 exit (1);
           }
        }


	iq_creator makeIQ (2, 1);
	fftFilter	filterBand (1024, 51);
	int	lowerBand	= band == 0 ? -5000 :
	                          band == 1 ?   500 : -5000;
	int	upperBand	= band == 0 ?  5000 :
	                          band == 1 ?  5000 : -500;
	filterBand. setBand (lowerBand, upperBand, rate);
	                         
	tcpClient theGenerator (port, url. c_str ());
	SRC_STATE	*converter =
			src_new (SRC_SINC_BEST_QUALITY, 2, &error);
	SRC_DATA	*src_data =
			new SRC_DATA;


	if (converter == NULL) {
	   fprintf (stderr, "error creating a converter %d\n", error);
	   exit (1);
	}

	running. store (true);

	for (int i = 0; i < 64; i++) {
           struct sigaction sa;

           std::memset (&sa, 0, sizeof(sa));
           sa. sa_handler = terminate;
           sigaction (i, &sa, NULL);
        }

	if (!theMicrophone. selectDefaultDevice ()) {
	   fprintf (stderr, "sorry, the micro does not work\n");
	   exit (2);
	}

	int	outputLimit	= 2 * rate / 10;
	float	ratio		= rate / 44100.0;
	int	bufferSize	= ceil (outputLimit / ratio);
	std::vector<float>	bi (bufferSize);
	std::vector<float>	bo (outputLimit);
	src_data -> data_in          = bi. data ();
	src_data -> data_out         = bo. data ();
	src_data -> src_ratio        = rate / 44100.0;
	src_data -> end_of_input     = 0;
	nextStop                     = getMyTime ();
	while (running. load ()) {
	   std::complex<float> buffer [bufferSize / 2];
	   int res;
	   int readCount = theMicrophone. getSamples (buffer, bufferSize / 2);
	   for (int i = 0; i < readCount; i ++) {
	      bi [2 * i    ] = real (buffer [i]);
	      bi [2 * i + 1] = imag (buffer [i]);
	   }
	    
	   src_data	-> input_frames		= readCount;
	   src_data	-> output_frames	= outputLimit / 2;
	   res		= src_process (converter, src_data);
//
//	Ee do not have to add delays, since we
//	depend on the clock of the microphone

	   for (int i = 0; i < src_data -> output_frames_gen; i ++) {
	      float sample	= src_data -> data_out [i * 2];
	      sample += bo [i * 2 + 1];
	      sample /= 2;
	      std::complex<float> csample = makeIQ. Pass ( 10 * sample);
	      csample	=  filterBand. Pass (csample);
	      int16_t intSample [2];
	      if (band == 0) {		// add a carrier
	         intSample [0] = (int16_t)((sample + 1) * 32767);
	         intSample [1] = (int16_t)(0);
	      }
	      else {
	         intSample [0] = (int16_t)(real (csample) * 32767);
	         intSample [1] = (int16_t)(imag (csample) * 32767);
	      }
	      try {
	         theGenerator. sendData (intSample, 2);
	      } catch (int e) {
	         running. store (false);
	      }
	   }
	}
	theMicrophone. stop ();
}


