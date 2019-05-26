#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the fm modulator
 *
 *    fm modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fm modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fm modulator; if not, write to the Free Software
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
#include	"tcp-driver.h"
#include	"file-driver.h"
#include	"fm-modulator.h"

#define	CURRENT_VERSION	"0.2"
std::atomic<bool>	running;


static
fmModulator	*my_fmModulator;

static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
	my_fmModulator	-> stop ();
}
//

void	printHelp	(void) {
	fprintf (stderr, "am modulator\n"
"\toptions are:\n"
"\tf filename\t:use filename as inputfile\n"
"\tp number\t: use number as port number for the output\n"
"\tr number\t: use number as output sample rate\n"
"\tu string\t: use string as url of the transmitter\n"
"\to filename\t: write the output to a pcm file filename\n"
"\ta filename\t: play the songs listed in the file\n"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {
std::string	url		= std::string ("127.0.0.1");
int		port		= 8765;
int		rate		= 192000;
std::string	fileName	= std::string ("");
int		opt;
std::string	ofileName	= std::string ("");
bool	fileOut			= false;
outputDriver	*theGenerator;
std::string	afspeellijst;

        while ((opt = getopt (argc, argv, "a:f:p:r:u:o:h:")) != -1) {
           switch (opt) {
	      case 'f':
	         fileName	= std::string (optarg);
	         break;

	      case 'p':
                 port		= atoi (optarg);
                 break;

	      case 'a':
	         afspeellijst	= std::string (optarg);
	         break;

	      case 'r':
	         rate		= atoi (optarg);
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

	if (fileName. empty () && afspeellijst. empty ()) {
	   fprintf (stderr, "you should specify a filename\n");
	   exit (1);
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
	      theGenerator 	= new fileDriver (ofileName, rate);
	   else
	      theGenerator	= new tcpDriver (port, url);
	} catch (int e) {
	   fprintf (stderr, "creating a driver for output failed, fatal\n");
	   exit (2);
	}

	my_fmModulator	= new fmModulator (rate, theGenerator);
	if (!fileName. empty ()) {	// play a single file
	   my_fmModulator -> go (fileName);
	}
	else {
	   std::ifstream myfile (afspeellijst);
	   std::string currentFile;
	   if (myfile) {
	      while (getline (myfile, currentFile))
	         my_fmModulator -> go (currentFile);
	      myfile. close ();
	   }
	}
	
	delete my_fmModulator;
	delete theGenerator;
	fflush (stdout);
        fflush (stderr);
}

