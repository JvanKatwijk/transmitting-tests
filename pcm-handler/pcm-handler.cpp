#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the pcm handler
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

#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<sndfile.h>
#include	<samplerate.h>
#include	<math.h>
#include	<atomic>
#include	<complex>
#include	<unistd.h>
#include	<vector>
#include	"pcm-handler.h"

		pcmHandler::pcmHandler (int rate,
	                                pcmDriver *my_pcmDriver,
	                                rdsDriver *my_rdsDriver) {
	this	-> rate			= rate;
	this	-> my_pcmDriver		= my_pcmDriver;
	this	-> my_rdsDriver		= my_rdsDriver;
	running. store (false);
}

	pcmHandler::~pcmHandler (void) {
	running. store (false);
}

void	pcmHandler::stop (void) {
	running. store (false);
}


void	pcmHandler::go (std::string fileName) {
SF_INFO	sf_info;
SNDFILE	*infile		= sf_open (fileName. c_str (), SFM_READ, &sf_info);
int32_t	outputLimit	= 2 * rate / 10;
double	ratio;
int	bufferSize;
std::vector<float>      bi (0);
std::vector<float>      bo (0);
int     error;

SRC_STATE       *converter =
                        src_new (SRC_SINC_MEDIUM_QUALITY, 2, &error);
SRC_DATA        *src_data =
                        new SRC_DATA;

	if (infile == NULL) {
	   fprintf (stderr, "could not open %s\n", fileName. c_str ());
	   return; 
	}
        if (converter == NULL) {
           fprintf (stderr, "error creating a converter %d\n", error);
           return;
        }

        ratio           = (double)rate / sf_info. samplerate;
        bufferSize      = ceil (outputLimit / ratio);
        bi. resize (bufferSize);
        bo. resize (outputLimit);

	running. store (true);
        src_data -> data_in          = bi. data ();
        src_data -> data_out         = bo. data ();
        src_data -> src_ratio        = ratio;
        src_data -> end_of_input     = 0;

	fprintf (stderr, "reading and converting, %f %d %d\n",
	                          ratio, rate, sf_info. samplerate);
	my_rdsDriver	-> sendSample (fileName);
	while (running. load ()) {
	   int  readCount;
	   readCount    = sf_readf_float (infile, bi. data (), bufferSize / 2);
	   if (readCount <= 0) {
	      sf_seek (infile, 0, SEEK_SET);
	      fprintf (stderr, "eof reached\n");
	      sf_close (infile);
	      usleep (10000);
	      running. store (false);
	      return;
	   }

	   if (sf_info. channels == 1) {
	      float temp [bufferSize / 2];
              memcpy (temp, bi. data (), readCount * sizeof (float));
              for (int j = 0; j < readCount; j ++) {
                 bi [2 * j] = temp [j];
                 bi [2 * j + 1] = 0;
              }
           }

	   src_data     -> input_frames         = readCount;
           src_data     -> output_frames        = outputLimit / 2;
           int res      = src_process (converter, src_data);

	   for (int i = 0; i < src_data -> output_frames_gen; i ++)
	      my_pcmDriver ->
	            sendSample (std::complex<float> (bo [2 * i],
	                                             bo [2 * i + 1]));
	}
}

