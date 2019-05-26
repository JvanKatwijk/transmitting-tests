#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the fm streamer
 *
 *    fm streamer is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fm streamer is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fm streamer; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include        <unistd.h>
#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include        <samplerate.h>
#include        <math.h>
#include        <atomic>
#include        <sys/time.h>
#include	<fstream>
#include	"output-driver.h"
#include	"pcm-input.h"
#include	"rds-input.h"
#include	"tcp-output.h"
#include	"file-driver.h"
#include	"fm-streamer.h"

#define	CURRENT_VERSION	"0.2"
std::atomic<bool>	running;

static
fmStreamer	*my_fmStreamer;

static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
	my_fmStreamer	-> stop ();
}
//

void	printHelp	(void) {
	fprintf (stderr, "am modulator\n"
"\toptions are:\n"
"\tf filename\t:use filename as inputfile\n"
"\tp number\t: use number as port number for the output\n"
"\tr number\t: use number as output sample rate\n"
"\ta number\t: use number as input sample rate\n"
"\tu string\t: use string as url of the transmitter\n"
"\to filename\t: write the output to a pcm file filename\n"
"\ti filename\t: use number as port number for the input\n"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {
std::string	url		= std::string ("127.0.0.1");
int		outPort		= 8765;
int		outRate		= 192000;
int		inRate		= 44100;
int		inPort		= 8766;
int		opt;
std::string	ofileName	= std::string ("");
bool	fileOut			= false;
outputDriver	*theGenerator;
std::string	afspeellijst;
pcmInput	*pcmHandler;
rdsInput	*rdsHandler;
RingBuffer<floatSample> *sampleBuffer;
RingBuffer<char> *rdsBuffer;

        while ((opt = getopt (argc, argv, "i:a:p:r:u:o:h:")) != -1) {
           switch (opt) {
	      case 'i':
	         inPort		= atoi (optarg);
	         break;

	      case 'p':
                 outPort	= atoi (optarg);
                 break;

	      case 'r':
	         outRate	= atoi (optarg);
	         break;

	      case 'a':
	         inRate		= atoi (optarg);
	         break;

	      case 'u':
	         url		= std::string (optarg);
	         break;

	      case 'o':
	         ofileName	= std::string (optarg);
	         fileOut	= true;
	         break;

	      case 'h':
              default:
	         printHelp ();
	         exit (1);
           }
        }

	for (int i = 0; i < 64; i++) {
	   struct sigaction sa;
	   std::memset (&sa, 0, sizeof(sa));
	   sa. sa_handler = terminate;
	   sigaction (i, &sa, NULL);
	}

	running. store (false);
	try {
	   if (fileOut) 
	      theGenerator 	= new fileDriver (ofileName, outRate);
	   else
	      theGenerator	= new tcpOutput (outPort, url);
	} catch (int e) {
	   fprintf (stderr, "creating a driver for output failed, fatal\n");
	   exit (2);
	}

	sampleBuffer	= new RingBuffer<floatSample> (32768);
	rdsBuffer	= new RingBuffer<char> (1024);
	pcmHandler	= new pcmInput (inPort, sampleBuffer);
	rdsHandler	= new rdsInput (inPort + 1, rdsBuffer);
	my_fmStreamer	= new fmStreamer (inRate,
	                                  outRate,
	                                  theGenerator,
	                                  sampleBuffer,
	                                  rdsBuffer);
	pcmHandler	-> start ();
	rdsHandler	-> start ();
	my_fmStreamer	-> start ();
	running. store (true);
	while (running. load ())
	   usleep (100000);
	delete my_fmStreamer;
	delete theGenerator;
	fflush (stdout);
        fflush (stderr);
}

