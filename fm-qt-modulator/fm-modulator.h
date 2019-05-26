#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
 *    along with fm-modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __FM_MODULATOR__
#define __FM_MODULATOR__

#include	<stdint.h>
#include	<stdio.h>
#include	<QMainWindow>
#include	<QLabel>
#include	<sndfile.h>
#include	"ringbuffer.h"
#include	"tcp-client.h"
#include	"spectrum-viewer.h"
#include	"ui_fm-modulator.h"
#include	"ringbuffer.h"

class	QSettings;
class	fileProcessor;

class fmModulator: public QMainWindow,
		      private Ui_MainWindow {
Q_OBJECT
public:
		fmModulator		(QSettings	*modulatorSettings,
	                                 int		rate,
	                                 tcpClient	*theGenerator,
	                                 QWidget	*parent = NULL);
		~fmModulator		(void);
private slots:
	void	go			(void);

private:
	fileProcessor	*theReader;
	QSettings	*modulatorSettings;
	int		rate;
	tcpClient	*theGenerator;
	RingBuffer<std::complex<float>> *scopeBuffer;
	spectrumViewer	*scope;
	std::atomic<bool>	running;
	SNDFILE		*infile;
	SF_INFO		sf_info;
	void		closeEvent	(QCloseEvent *event);
public slots:
	void		showSpectrum	(void);
};

#endif

