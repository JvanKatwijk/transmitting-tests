#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the hackrf transmitter
 *
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

#ifndef __HACKRF_TRANSMITTER__
#define __HACKRF_TRANSMITTER__

#include        <QObject>
#include        <QDialog>
#include	<QMainWindow>
#include	<QComboBox>
#include	<QLabel>
#include	<QTimer>
#include	<atomic>
#include        <QByteArray>
#include        <QHostAddress>
#include        <QtNetwork>
#include        <QTcpServer>
#include        <QTcpSocket>
#include	"transmitter-constants.h"
#include	"ringbuffer.h"
#include	"lowpass.h"
#include	"ui_transmitter.h"
#include	<QCloseEvent>
class	QSettings;
class	hackrfHandler;
class	keyPad;
class	spectrumViewer;
/*
 *	The main gui object. It inherits from
 *	QMainWindow and the generated form
 */

class Transmitter: public QMainWindow,
		      private Ui_MainWindow {
Q_OBJECT
public:
		Transmitter		(QSettings	*,
	                                 int,		// initial frequency
	                                 int,		// the port
	                                 int,		// inputRate
	                                 int,		// transmissionrate
	                                 QWidget	*parent = NULL);
		~Transmitter		(void);

private:
	QSettings	*transmitterSettings;
	hackrfHandler	*theTransmitter;

	keyPad		*mykeyPad;
	bool		connected;
	QTcpServer      server;
        QTcpSocket      *client;
        QTimer          watchTimer;

	int		inputRate;
	int		port;
	int		vfoFrequency;
	int32_t         transmissionRate;
        int32_t         upFactor;
        LowPassFIR      *upFilter;
	RingBuffer<std::complex<float>> *scopeBuffer;
	spectrumViewer	*scope;
public slots:
	void	handle_txvgaSlider	(int v);
	void	handle_resetButton	(void);
	void	handle_freqButton	(void);
	void	newFrequency		(uint32_t);
	void	closeEvent		(QCloseEvent *event);
	void	acceptConnection	(void);
	void	disconnect		(void);
	void	handleData		(void);
};

#endif


