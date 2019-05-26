#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the sweeper
 *
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

#ifndef __SWEEPER__
#define __SWEEPER__

#include	<stdint.h>
#include	<stdio.h>
#include	<QMainWindow>
#include	<QLabel>
#include	"ringbuffer.h"
#include	"tcp-client.h"
#include	"spectrum-viewer.h"
#include	"ui_sweeper.h"
#include	"ringbuffer.h"

class	QSettings;
class	sweepGenerator;
class Sweeper: public QMainWindow,
		      private Ui_MainWindow {
Q_OBJECT
public:
		Sweeper		(QSettings	*sweeperSettings,
	                         int		rate,
	                         tcpClient	*theGenerator,
	                         QWidget	*parent = NULL);
		~Sweeper	(void);

private:
	QSettings	*sweeperSettings;
	int		rate;
	tcpClient	*theGenerator;
	sweepGenerator	*sweepgenerator;
	RingBuffer<std::complex<float>> *scopeBuffer;
	spectrumViewer	*scope;
	std::atomic<bool>	running;
	void		closeEvent	(QCloseEvent *event);
public slots:
	void		set_sweepsperminuut (int swpm);
	void		showSpectrum	(void);
};

#endif

