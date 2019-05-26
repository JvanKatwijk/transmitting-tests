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

#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<sndfile.h>
#include	<samplerate.h>
#include	<math.h>
#include	<atomic>
#include	<sys/time.h>
#include	<complex>
#include	"tcp-client.h"
#include	<QMessageBox>
#include	<QDir>
#include	<QFileDialog>
#include	<QSettings>
#include	"fm-modulator.h"
#include	"file-processor.h"

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

		fmModulator::fmModulator (QSettings	*modulatorSettings,
	                                  int		rate,
	                                  tcpClient	*theGenerator,
	                                  QWidget	*parent):
	                                         QMainWindow (parent) {

	setupUi (this);
	this	-> modulatorSettings	= modulatorSettings;
	this	-> rate			= rate;
	this	-> theGenerator		= theGenerator;

	this    -> scopeBuffer          =
                                 new RingBuffer<std::complex<float>> (32768);
        this    -> scope                =
                                 new spectrumViewer (inputSpectrum,
                                                     scopeBuffer,
                                                     rate);

	int ampSetting		= modulatorSettings -> value ("ampSetting", 50). toInt ();
	scopeAmplifier		-> setValue (ampSetting);

	theReader		= NULL;
	running. store (false);
	connect (startButton, SIGNAL (clicked (void)),
	         this, SLOT (go (void)));
}

	fmModulator::~fmModulator (void) {
	modulatorSettings	-> setValue ("ampSetting",
	                                     scopeAmplifier -> value ());
	if (theReader != NULL) {
	   running. store (false);
	   delete theReader;
	}
	delete	scope;
	delete	scopeBuffer;
}

void	fmModulator::go (void) {
	if (running. load ()) {
	   delete theReader;
	   theReader	= NULL;
	   running. store (false);
	}

	QString fileName = QFileDialog::getOpenFileName (this,
                                                tr ("Save file ..."),
                                                QDir::homePath (),
                                                tr ("wav (*.wav)"));
        fileName        = QDir::toNativeSeparators (fileName);
	infile		= sf_open (fileName. toLatin1 (). data (),
	                           SFM_READ,
	                           &sf_info);

	if (infile == NULL) {
	   QMessageBox::warning (this, tr ("Warning"),
                                   tr ("Opening  input stream failed\n"));
           return;      // give it another try
	}


	theReader	= new fileProcessor (infile,
	                                     &sf_info,
	                                     rate,
	                                     theGenerator,
	                                     scopeBuffer);
	connect (theReader, SIGNAL (showSpectrum (void)),
	         this, SLOT (showSpectrum (void)));
	running. store (true);
}

void	fmModulator::showSpectrum	(void) {
	scope	-> showSpectrum (scopeAmplifier -> value ());
}

#include <QCloseEvent>
void fmModulator::closeEvent (QCloseEvent *event) {

        QMessageBox::StandardButton resultButton =
                        QMessageBox::question (this, "am modulator",
                                               tr("Are you sure?\n"),
                                               QMessageBox::No | QMessageBox::Yes,
                                               QMessageBox::Yes);
        if (resultButton != QMessageBox::Yes) {
           event -> ignore();
        } else {
	   running. store (false);
           event -> accept ();
        }
}

