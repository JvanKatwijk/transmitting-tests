#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the hackrf transmitter
 *    hackrf transmitter is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf transmitter is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf transmitter; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<QSettings>
#include	<QMessageBox>
#include	<QDebug>
#include	<QDateTime>
#include	<QFile>
#include	<QDir>
#include	"transmitter-constants.h"
#include	<numeric>
#include	<unistd.h>
#include	<vector>
#include        <QMouseEvent>
#include	"transmitter.h"
#include	"spectrum-viewer.h"
#include	"popup-keypad.h"
#include	<mutex>

#include	"hackrf-handler.h"
/**
  *	We use the creation function merely to set up the
  *	user interface and make the connections between the
  *	gui elements and the handling agents. All real action
  *	is initiated by gui buttons
  */
	Transmitter::Transmitter (QSettings	*Si,
	                          int		vfoFrequency,
	                          int		port,
	                          int		inputRate,
	                          int		transmissionRate,
	                          QWidget	*parent):
	                                        QMainWindow (parent) {

	transmitterSettings		= Si;
	this	-> vfoFrequency		= vfoFrequency;
	this	-> port			= port;
	fprintf (stderr, "vfo = %d\n", vfoFrequency);
//	The settings are done, now creation of the GUI parts
	setupUi (this);

        this    -> upFactor		=
	                         transmissionRate / inputRate;
	if (upFactor > 1)
           this    -> upFilter		=
	                         new LowPassFIR (3,
	                                         inputRate / 2,
                                                 transmissionRate);

	this	-> scopeBuffer		=
	                         new RingBuffer<std::complex<float>> (32768);
	this	-> scope	        =
	                         new spectrumViewer (inputSpectrum,
	                                             scopeBuffer,
	                                             inputRate);
	theTransmitter			=
	                         new hackrfHandler  (transmitterSettings,
	                                             vfoFrequency,
	                                             transmissionRate);
	if (theTransmitter == NULL) {
	   throw (22);
	}
	int	txvgaValue 		=
	                         transmitterSettings -> value ("txvgaGain",
	                                                       30). toInt ();
	txvgaSlider	-> setValue	(txvgaValue);
	txvga_display	-> display	(txvgaValue);
	theTransmitter	-> set_txvgaValue (txvgaValue);

	frequencyDisplay	-> display (vfoFrequency);
//	display the version
	QString versionText = "transmitter version: " + QString(CURRENT_VERSION);
	versionText += " Build on: " + QString(__TIMESTAMP__) + QString (" ") + QString (GITHASH);
	versionText += "\nCopyright 2019 J van Katwijk, Lazy Chair Computing.";

	copyrightLabel	-> setToolTip (versionText);

	inputRate_Label	-> setText (QString::number (inputRate));
//	
	connect (txvgaSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (handle_txvgaSlider (int)));
	connect (resetButton, SIGNAL (clicked (void)),
	         this, SLOT (handle_resetButton (void)));

	mykeyPad                = new keyPad (this);
	connect (freqselector, SIGNAL (clicked (void)),
	         this, SLOT (handle_freqButton (void)));

	connect (&server, SIGNAL (newConnection (void)),
	         this, SLOT (acceptConnection (void)));
	server. listen (QHostAddress::Any, port);

	qApp    -> installEventFilter (this);

	connected	= false;
}

	Transmitter::~Transmitter (void) {
	transmitterSettings	-> setValue ("txvgaGain",
	                                          txvgaSlider -> value ());
	transmitterSettings	-> setValue ("frequency", vfoFrequency);
	transmitterSettings	-> sync ();
	delete	mykeyPad;
	delete	theTransmitter;
	if (upFactor > 1)
	   delete	upFilter;
	delete	scope;
	delete	scopeBuffer;
}
//

void	Transmitter::handle_txvgaSlider	(int v) {
	theTransmitter	-> set_txvgaValue (v);
	txvga_display	-> display (v);
}

//	to do:
void	Transmitter::handle_resetButton	(void) {
}

void	Transmitter::handle_freqButton	(void) {
	if (mykeyPad -> isVisible ())
	   mykeyPad	-> hidePad ();
	else
	   mykeyPad	-> showPad ();
}

void	Transmitter::newFrequency (uint32_t f) {
	
	theTransmitter	-> set_frequency ((uint64_t)KHz (f));
	vfoFrequency		= KHz (f);
	frequencyDisplay	-> display ((int)f);
}

void	Transmitter::closeEvent (QCloseEvent *event) {

	QMessageBox::StandardButton resultButton =
	                QMessageBox::question (this, "hackrf transmitter",
	                                       tr("Are you sure?\n"),
	                                       QMessageBox::No | QMessageBox::Yes,
	                                       QMessageBox::Yes);
	if (resultButton != QMessageBox::Yes) {
	   event -> ignore();
	} else {
	   disconnect ();
	   event -> accept ();
	}
}

void	Transmitter::acceptConnection (void) {
	if (connected)
	   return;

	client = server. nextPendingConnection ();
	QHostAddress s = client -> peerAddress ();
	fprintf (stderr, "Accepted a client %s\n",
	                     s.toString (). toLatin1 (). data ());

	connectName -> setText (client -> peerAddress (). toString ());
	connect (client, SIGNAL (readyRead (void)),
	         this, SLOT (handleData (void)));
	connect (client, SIGNAL (disconnected (void)),
	         this, SLOT (disconnect (void)));

	connected	= true;

	frequencyDisplay	-> display ((int)vfoFrequency / Khz (1));
	theTransmitter	-> start_transmitting (vfoFrequency);
}

void	Transmitter::disconnect (void) {
	connected	= false;
	theTransmitter	-> stop_transmitting ();
	connectName	-> setText ("not connected");
}

//
//	CHUNK_SIZE is the size in samples,
//	the transmission then is in blocks size 4 * CHUNK_SIZE
#define	CHUNK_SIZE	256
void	Transmitter::handleData (void) {
char theData [CHUNK_SIZE * 4];
std::complex<float> buffer [CHUNK_SIZE * upFactor];
std::complex<float> sample;
int	fillerP		= 0;

static float sum	= 0;
	while (client -> bytesAvailable () >= CHUNK_SIZE * 4) {
	   client -> read (theData, CHUNK_SIZE * 4);
	   sum	= 0;
	   fillerP = 0;
	   for (int i = 0; i < CHUNK_SIZE; i ++) {
	      int16_t re = (theData [4 * i] << 8) |
	                              (uint8_t)(theData [4 * i + 1]);
	      int16_t im = (theData [4 * i + 2] << 8) |
	                              (uint8_t)(theData [4 * i + 3]);
	      sample = std::complex<float> (re / 32767.0, im / 32767.0);
	      scopeBuffer	-> putDataIntoBuffer (&sample, 1);
	      if (scopeBuffer -> GetRingBufferReadAvailable () >= 1024)
	         scope -> showSpectrum (scopeAmplifier -> value ());
	      if (upFactor > 1) 
	         buffer [fillerP++] = upFilter -> Pass (sample);
	      else
	         buffer [fillerP ++] = sample;

	      for (int j = 1; j < upFactor; j ++)
	         buffer [fillerP ++] =
	                  upFilter -> Pass (std::complex<float> (0, 0));
	   }

	   theTransmitter	-> passOn (buffer, fillerP);
	}
}
