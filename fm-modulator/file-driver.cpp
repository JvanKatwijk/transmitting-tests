#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the fm modulator
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

#define	BUF_SIZE	512

#include	"file-driver.h"

	fileDriver::fileDriver	(std::string fileName, int rate) {
	sf_info. samplerate	= rate;
	sf_info. channels	= 2;
	sf_info. format		= SF_FORMAT_WAV | SF_FORMAT_FLOAT;
	oFile			= sf_open (fileName. c_str (),
	                                   SFM_WRITE, &sf_info);
	if (oFile == NULL)
	   throw (23);
	buffer. resize (BUF_SIZE);
        localBuffer. resize (2 * BUF_SIZE);
        bufferP			= 0;
}

	fileDriver::~fileDriver	(void) {
	   sf_close (oFile);
	}

void	fileDriver::sendSample	(std::complex<float> sample) {
	buffer [bufferP] = sample;
	bufferP ++;
	if (bufferP >= BUF_SIZE) {
	   for (int i = 0; i < BUF_SIZE; i ++) {
	      localBuffer [2 * i    ] = real (buffer [i]);
	      localBuffer [2 * i + 1] = imag (buffer [i]);
	   }
	   sf_writef_float (oFile, localBuffer. data (), BUF_SIZE);
	   bufferP = 0;
	}
}

