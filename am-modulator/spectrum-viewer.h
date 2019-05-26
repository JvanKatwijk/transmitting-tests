#
/*
 *    Copyright (C) 2014 .. 2017
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

//
//	Simple spectrum scope object
//	Shows the spectrum of the incoming data stream 
//
#ifndef		__SPECTRUM_VIEWER__
#define		__SPECTRUM_VIEWER__

#include	<stdint.h>
#include	<stdio.h>
#include	<complex>
#include	<QFrame>
#include	<QObject>
#include	<qwt.h>
#include	<qwt_plot.h>
#include	<qwt_plot_marker.h>
#include	<qwt_plot_grid.h>
#include	<qwt_plot_curve.h>
#include	<qwt_plot_marker.h>
#include	<fftw3.h>
#include	"ringbuffer.h"

class	spectrumViewer: public QObject {
Q_OBJECT
public:
			spectrumViewer	(QwtPlot	*,
	                                 RingBuffer<std::complex<float>> *,
	                                 int);
			~spectrumViewer	(void);
	void		showSpectrum	(int);
private:
	int		inputRate;
	RingBuffer<std::complex<float>>	*spectrumBuffer;
	int16_t		displaySize;
	int16_t		spectrumSize;
	std::vector<double>	displayBuffer;
	std::vector<float>	Window;
	std::complex<float>	*spectrum;
	fftwf_plan	plan;
	QwtPlot		*plotgrid;
	QwtPlotGrid	*grid;
	QwtPlotCurve	*spectrumCurve;
	QBrush		*ourBrush;
	void		ViewSpectrum		(double *, double *, double, int);
	float		get_db 			(float);
	void		setBitDepth		(int16_t d);
	int32_t		normalizer;
};

#endif

