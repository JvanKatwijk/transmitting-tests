#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
 *
 *    This file is part of the pcm handler
 *
 *    pcm handler is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    pcm handler is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with pcm handler; if not, write to the Free Software
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
#include	"pcm-driver.h"
#include	"rds-driver.h"
#include	"pcm-handler.h"

#define	CURRENT_VERSION	"0.2"
std::atomic<bool>	running;


static
pcmHandler	*my_pcmHandler;

static
void    terminate (int num) {

        running. store (false);
        fprintf (stderr,"Caught signal - Terminating %x\n",num);
	my_pcmHandler	-> stop ();
}
//

void	printHelp	(void) {
	fprintf (stderr, "am modulator\n"
"\toptions are:\n"
"\tf filename\t:use filename as inputfile\n"
"\ta filename\t: play the songs listed in the file\n"
"\tp number\t: use number as port number for the output\n"
"\tu string\t: use string as url of the transmitter\n"
"\th\t:print this message\n");
}

int	main (int argc, char **argv) {
std::string	url		= std::string ("127.0.0.1");
int		port		= 8766;
int		rate		= 44100;
std::string	fileName	= std::string ("");
int		opt;
pcmDriver	*my_pcmDriver;
rdsDriver	*my_rdsDriver;
std::string	afspeellijst;

        while ((opt = getopt (argc, argv, "a:f:p:r:u:h:")) != -1) {
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
	   my_pcmDriver	= new pcmDriver (port, url);
	   my_rdsDriver	= new rdsDriver (port + 1, url);
	} catch (int e) {
	   fprintf (stderr, "creating a driver for output failed, fatal\n");
	   exit (2);
	}

	my_pcmHandler	= new pcmHandler (rate, my_pcmDriver, my_rdsDriver);
	if (!fileName. empty ()) {	// play a single file
	   my_pcmHandler -> go (fileName);
	}
	else {
	   std::ifstream myfile (afspeellijst);
	   std::string currentFile;
	   if (myfile) {
	      while (getline (myfile, currentFile))
	         my_pcmHandler -> go (currentFile);
	      myfile. close ();
	   }
	}
	
	delete my_pcmHandler;
	delete my_pcmDriver;
	delete my_rdsDriver;
	fflush (stdout);
        fflush (stderr);
}

