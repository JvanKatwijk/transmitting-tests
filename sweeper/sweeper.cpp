#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the sweeper
 *    sweeper is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    sweeper is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sweeper; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<math.h>
#include	<atomic>
#include	<sys/time.h>
#include	<complex>
#include	"tcp-client.h"
#include	<QMessageBox>
#include	<QDir>
#include	<QFileDialog>
#include	<QSettings>
#include	"sweeper.h"
#include	"sweepgenerator.h"

		Sweeper::Sweeper (QSettings	*sweeperSettings,
	                          int		rate,
	                          tcpClient	*theGenerator,
	                          QWidget	*parent):
	                                         QMainWindow (parent) {

	setupUi (this);
	this	-> sweeperSettings	= sweeperSettings;
	this	-> rate			= rate;
	this	-> theGenerator		= theGenerator;

	this    -> scopeBuffer          =
                                 new RingBuffer<std::complex<float>> (32768);
        this    -> scope                =
                                 new spectrumViewer (inputSpectrum,
                                                     scopeBuffer,
                                                     rate);

	int ampSetting		= sweeperSettings -> value ("ampSetting", 50). toInt ();
	scopeAmplifier		-> setValue (ampSetting);

	int	sweeps		= sweeperSettings -> value ("sweeps", 20). toInt ();
	sweepsperminuut		-> setValue (sweeps);

	int samplesPerFrequency	=  (int)(60.0 / sweeps * rate / 1000);
	sweepgenerator	= new sweepGenerator (theGenerator,
	                                      scopeBuffer,
	                                      -rate / 2, rate / 2,
	                                      1000, samplesPerFrequency);
	                         
	connect (sweepgenerator, SIGNAL (showSpectrum (void)),
	         this, SLOT (showSpectrum (void)));
	connect (sweepsperminuut, SIGNAL (valueChanged (int)),
	         this, SLOT (set_sweepsperminuut (int)));
	sweepgenerator	-> start ();
}

	Sweeper::~Sweeper (void) {
	sweeperSettings	-> setValue ("ampSetting",
	                             scopeAmplifier -> value ());
	sweeperSettings -> setValue ("sweeps", sweepsperminuut -> value ());

	delete	sweepgenerator;
	delete	scope;
	delete	scopeBuffer;
}

void	Sweeper::set_sweepsperminuut (int swpm) {
int	sweeps	= sweepsperminuut	-> value ();
	sweepgenerator	-> reset (1000, (int)(60.0 / sweeps * rate / 1000));
}
	
void	Sweeper::showSpectrum	(void) {
	scope	-> showSpectrum (scopeAmplifier -> value ());
}

#include <QCloseEvent>
void	Sweeper::closeEvent (QCloseEvent *event) {

        QMessageBox::StandardButton resultButton =
                        QMessageBox::question (this, "Sweeper",
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

