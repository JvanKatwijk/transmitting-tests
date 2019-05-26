#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of transmitter
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

#include	"audiosource.h"
#include	<stdio.h>

	audioSource::audioSource (void) {
int32_t	i;
	this	-> CardRate	= 44100;
	_I_Buffer		= new RingBuffer<float>(4 * 32768);
	portAudio		= false;
	readerRunning		= false;
	if (Pa_Initialize () != paNoError) {
	   fprintf (stderr, "Initializing Pa for input failed\n");
	   return;
	}

	portAudio	= true;

	fprintf (stderr, "Hostapis: %d\n", Pa_GetHostApiCount ());

	for (i = 0; i < Pa_GetHostApiCount (); i ++)
	   fprintf (stderr, "Api %d is %s\n", i, Pa_GetHostApiInfo (i) -> name);

	numofDevices	= Pa_GetDeviceCount ();
	inTable. resize (numofDevices);
	for (i = 0; i < numofDevices; i ++)
	   inTable [i] = -1;
	setupChannels ();
	istream		= NULL;
}

	audioSource::~audioSource	(void) {
	if ((istream != NULL) && !Pa_IsStreamStopped (istream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (istream);
	   while (!Pa_IsStreamStopped (istream))
	      Pa_Sleep (1);
	   readerRunning = false;
	}

	if (istream != NULL)
	   Pa_CloseStream (istream);

	if (portAudio)
	   Pa_Terminate ();

	delete	_I_Buffer;
}

bool	audioSource::selectDevice (int16_t idx) {
PaError err;
int16_t	inputDevice;

	if (idx	== 0)
	   return false;

	fprintf (stderr, "device index = %d\n", idx);
	inputDevice	= idx;
	if (!isValidDevice (inputDevice)) {
	   fprintf (stderr, "invalid device (%d) selected\n", inputDevice);
	   return false;
	}

	if ((istream != NULL) && !Pa_IsStreamStopped (istream)) {
	   paCallbackReturn = paAbort;
	   (void) Pa_AbortStream (istream);
	   while (!Pa_IsStreamStopped (istream))
	      Pa_Sleep (1);
	   readerRunning = false;
	}

	if (istream != NULL)
	   Pa_CloseStream (istream);

	inputParameters. device		= inputDevice;
	inputParameters. channelCount		= 2;
	inputParameters. sampleFormat		= paFloat32;
	inputParameters. suggestedLatency	= 
	                          Pa_GetDeviceInfo (inputDevice) ->
	                                      defaultHighOutputLatency;
	bufSize	= (int)((float)inputParameters. suggestedLatency);
//
	inputParameters. hostApiSpecificStreamInfo = NULL;
//
	fprintf (stderr, "Suggested size for inputbuffer = %d\n", bufSize);
	err = Pa_OpenStream (&istream,
	                     &inputParameters,
	                     NULL,
	                     CardRate,
	                     bufSize,
	                     0,
	                     this	-> paCallback_i,
	                     this
	      );

	if (err != paNoError) {
	   fprintf (stderr, "Open istream error\n");
	   return false;
	}
	fprintf (stderr, "stream opened\n");
	paCallbackReturn = paContinue;
	err = Pa_StartStream (istream);
	if (err != paNoError) {
	   fprintf (stderr, "Open startstream error\n");
	   return false;
	}
	readerRunning	= true;
	return true;
}

void	audioSource::restart	(void) {
PaError err;

	if (!Pa_IsStreamStopped (istream))
	   return;

	_I_Buffer	-> FlushRingBuffer ();
	paCallbackReturn = paContinue;
	err = Pa_StartStream (istream);
	if (err == paNoError)
	   readerRunning	= true;
}

void	audioSource::stop	(void) {
	if (Pa_IsStreamStopped (istream))
	   return;

	paCallbackReturn	= paAbort;
	(void)Pa_StopStream	(istream);
	while (!Pa_IsStreamStopped (istream))
	   Pa_Sleep (1);
	readerRunning		= false;
}
//
//	helper
bool	audioSource::inputrateIsSupported (int16_t device, int32_t Rate) {
PaStreamParameters *inputParameters =
	           (PaStreamParameters *)alloca (sizeof (PaStreamParameters)); 

	inputParameters -> device		= device;
	inputParameters -> channelCount		= 2;	/* I and Q	*/
	inputParameters -> sampleFormat		= paFloat32;
	inputParameters -> suggestedLatency	= 0;
	inputParameters -> hostApiSpecificStreamInfo = NULL;

	return Pa_IsFormatSupported (NULL, inputParameters, Rate) ==
	                                          paFormatIsSupported;
}
/*
 * 	... and the callback
 */

int	audioSource::paCallback_i (const void*		inputBuffer,
                                   void*		outputBuffer,
		                   unsigned long	framesPerBuffer,
		                   const PaStreamCallbackTimeInfo *timeInfo,
	                           PaStreamCallbackFlags	statusFlags,
	                           void				*userData) {
RingBuffer<float>	*inB;
float	*inp		= (float *)inputBuffer;
audioSource *ud		= reinterpret_cast <audioSource *>(userData);
uint32_t	i;
	(void)statusFlags;
	(void)outputBuffer;
	(void)timeInfo;
	if (ud -> paCallbackReturn == paContinue) {
	   inB = (reinterpret_cast < audioSource *> (userData)) -> _I_Buffer;
	   inB -> putDataIntoBuffer (inputBuffer, 2 * framesPerBuffer);
	}
	return ud -> paCallbackReturn;
}

int	audioSource::getSamples	(std::complex<float> *buffer, int32_t amount) {
float b [2 * amount];
int	content = _I_Buffer	-> getDataFromBuffer (b, 2 * amount);
	for (int i = 0; i < content / 2; i ++)
	   buffer [i] = std::complex <float> (b [2 * i], b [2 * i + 1]);
	return content / 2;
}

int16_t	audioSource::invalidDevice	(void) {
	return numofDevices + 128;
}

bool	audioSource::isValidDevice (int16_t dev) {
	return 0 <= dev && dev < numofDevices;
}

bool	audioSource::selectDefaultDevice (void) {
	return selectDevice (Pa_GetDefaultInputDevice ());
}

int32_t	audioSource::cardRate	(void) {
	return CardRate;
}

bool	audioSource::setupChannels (void) {
uint16_t	icnt	= 1;
uint16_t	i;

	for (i = 0; i <  numofDevices; i ++) {
	   std::string si = 
	             inputChannelwithRate (i, 48000);
	   fprintf (stderr, "Investigating Device %d\n", i);

	   if (!si.empty ()) {
	      inTable [icnt] = i;
	      fprintf (stderr,
	               " (input):item %d wordt stream %d (%s)\n", icnt, i,
	                      si. c_str ());
	      icnt ++;
	   }
	}

	fprintf (stderr, "added items to list\n");
	return icnt > 1;
}

//
int16_t	audioSource::numberofDevices	(void) {
	return numofDevices;
}

std::string audioSource::inputChannelwithRate (int16_t ch, int32_t rate) {
const	PaDeviceInfo *deviceInfo;
std::string name = "";

	if ((ch < 0) || (ch >= numofDevices))
	   return name;

	deviceInfo = Pa_GetDeviceInfo (ch);
	if (deviceInfo == NULL)
	   return std::string ("");

	if (deviceInfo -> maxInputChannels <= 0)
	   return std::string ("");

	if (inputrateIsSupported (ch, rate))
	   name = deviceInfo -> name;
	return std::string (name);
}
