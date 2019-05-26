#
/*
 *    Copyright (C)  2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the transmitter
 *
 *    transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __AUDIO_SOURCE__
#define	__AUDIO_SOURCE__
#include	<portaudio.h>
#include	<stdio.h>
#include	<string>
#include	<complex>
#include	<vector>
#include	"ringbuffer.h"

class	audioSource {
public:
	                audioSource		(void);
			~audioSource		(void);
	void		stop			(void);
	void		restart			(void);
	bool		selectDevice		(int16_t);
	bool		selectDefaultDevice	(void);
	int		getSamples		(std::complex<float> *, int);
private:
	int16_t		numberofDevices		(void);
	bool		setupChannels		(void);
	std::string	inputChannelwithRate	(int16_t, int32_t);
	int16_t		invalidDevice		(void);
	bool		isValidDevice		(int16_t);
	int32_t		cardRate		(void);

	bool		inputrateIsSupported	(int16_t, int32_t);
	int32_t		CardRate;
	int32_t		size;
	bool		portAudio;
	bool		readerRunning;
	int16_t		numofDevices;
	int		paCallbackReturn;
	int16_t		bufSize;
	PaStream	*istream;
	RingBuffer<float>	*_I_Buffer;
	PaStreamParameters	inputParameters;
	std::vector<int16_t> inTable;
protected:
static	int		paCallback_i	(const void	*input,
	                                 void		*output,
	                                 unsigned long	framesperBuffer,
	                                 const PaStreamCallbackTimeInfo *timeInfo,
					 PaStreamCallbackFlags statusFlags,
	                                 void		*userData);
};

#endif

