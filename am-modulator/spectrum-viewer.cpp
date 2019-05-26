#
/*
 *    Copyright (C)  2014 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of hackrf transmitter
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

#include	"spectrum-viewer.h"
#include	<QColor>

	spectrumViewer::spectrumViewer	(QwtPlot	*scope,
	                                 RingBuffer<std::complex<float>> *sbuffer,
	                                 int inputRate){
int16_t	i;
QString	colorString	= "black";
QColor	displayColor;
QColor	gridColor;
QColor	curveColor;

	this	-> spectrumBuffer	= sbuffer;
	this	-> inputRate		= inputRate;
	displayColor			= QColor ("black");
	gridColor			= QColor ("white");
	curveColor			= QColor ("white");

	displaySize			= 256;
	displayBuffer. resize (displaySize);
	memset (displayBuffer. data (), 0, displaySize * sizeof (double));
	this	-> spectrumSize	= 4 * displaySize;
	spectrum		= (std::complex<float> *)fftwf_malloc (sizeof (fftwf_complex) * spectrumSize);
        plan    = fftwf_plan_dft_1d (spectrumSize,
                                    reinterpret_cast <fftwf_complex *>(spectrum),
                                    reinterpret_cast <fftwf_complex *>(spectrum),
                                    FFTW_FORWARD, FFTW_ESTIMATE);
	
	plotgrid		= scope;
	plotgrid	-> setCanvasBackground (displayColor);
	grid			= new QwtPlotGrid;
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMajPen (QPen(gridColor, 0, Qt::DotLine));
#else
	grid	-> setMajorPen (QPen(gridColor, 0, Qt::DotLine));
#endif
	grid	-> enableXMin (true);
	grid	-> enableYMin (true);
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMinPen (QPen(gridColor, 0, Qt::DotLine));
#else
	grid	-> setMinorPen (QPen(gridColor, 0, Qt::DotLine));
#endif
	grid	-> attach (plotgrid);

	spectrumCurve	= new QwtPlotCurve ("");
   	spectrumCurve	-> setPen (QPen(Qt::white));
	spectrumCurve	-> setOrientation (Qt::Horizontal);
	spectrumCurve	-> setBaseline	(get_db (0));
	ourBrush	= new QBrush (Qt::white);
	ourBrush	-> setStyle (Qt::Dense3Pattern);
	spectrumCurve	-> setBrush (*ourBrush);
	spectrumCurve	-> attach (plotgrid);
	plotgrid	-> enableAxis (QwtPlot::yLeft);
	Window. resize (spectrumSize);
	for (i = 0; i < spectrumSize; i ++) 
	   Window [i] =
	        0.42 - 0.5 * cos ((2.0 * M_PI * i) / (spectrumSize - 1)) +
	              0.08 * cos ((4.0 * M_PI * i) / (spectrumSize - 1));
	setBitDepth	(12);
}

	spectrumViewer::~spectrumViewer	(void) {
	fftwf_destroy_plan (plan);
	fftwf_free	(spectrum);
	delete		ourBrush;
	delete		spectrumCurve;
}

void	spectrumViewer::showSpectrum	(int amplification) {
double	X_axis [displaySize];
double	Y_values [displaySize];
int16_t	i, j;
double	temp	= (double)inputRate / 2 / displaySize;
int16_t	averageCount	= 5;

	if (spectrumBuffer -> GetRingBufferReadAvailable () < spectrumSize)
	   return;

	spectrumBuffer	-> getDataFromBuffer (spectrum, spectrumSize);
	spectrumBuffer	-> FlushRingBuffer ();
//	first X axis labels
	for (i = 0; i < displaySize; i ++)
	   X_axis [i] = 
	         (-(double)(inputRate / 2) +
	          (double)((i) * (double) 2 * temp)) / ((double)1000);
//
//	get the buffer data
	for (i = 0; i < spectrumSize; i ++)
	   spectrum [i] = std::complex<float> (
	                  real (spectrum [i]) * Window [i],
	                  imag (spectrum [i]) * Window [i]);

	fftwf_execute (plan);
//
//	and map the spectrumSize values onto displaySize elements
	for (i = 0; i < displaySize / 2; i ++) {
	   double f	= 0;
	   for (j = 0; j < spectrumSize / displaySize; j ++)
	      f += abs (spectrum [spectrumSize / displaySize * i + j]);

	   Y_values [displaySize / 2 + i] = 
                                 f / (spectrumSize / displaySize);
	   f = 0;
	   for (j = 0; j < spectrumSize / displaySize; j ++)
	      f += abs (spectrum [spectrumSize / 2 +
	                             spectrumSize / displaySize * i + j]);
	   Y_values [i] = f / (spectrumSize / displaySize);
	}
//
//	average the image a little.
	for (i = 0; i < displaySize; i ++) {
	   if (std::isnan (Y_values [i]) || std::isinf (Y_values [i]))
	      continue;
	   displayBuffer [i] = 
	          (double)(averageCount - 1) /averageCount * displayBuffer [i] +
	           1.0f / averageCount * Y_values [i];
	}

	memcpy (Y_values,
	        displayBuffer. data (), displaySize * sizeof (double));
	ViewSpectrum (X_axis, Y_values, amplification, 0);
}

void	spectrumViewer::ViewSpectrum (double *X_axis,
		                       double *Y1_value,
	                               double amp,
	                               int32_t marker) {
uint16_t	i;
float	amp1	= amp / 100;

	amp		= amp / 100.0 * (- get_db (0));
	plotgrid	-> setAxisScale (QwtPlot::xBottom,
				         (double)X_axis [0],
				         X_axis [displaySize - 1]);
	plotgrid	-> enableAxis (QwtPlot::xBottom);
	plotgrid	-> setAxisScale (QwtPlot::yLeft,
				         get_db (0), get_db (0) + amp);

	for (i = 0; i < displaySize; i ++) 
	   Y1_value [i] = get_db (amp * Y1_value [i]); 

	spectrumCurve	-> setBaseline (get_db (0));
	Y1_value [0]		= get_db (0);
	Y1_value [displaySize - 1] = get_db (0);

	spectrumCurve	-> setSamples (X_axis, Y1_value, displaySize);
	plotgrid	-> replot(); 
}

float	spectrumViewer::get_db (float x) {
	return 20 * log10 ((x + 1) / (float)(normalizer));
}

void	spectrumViewer::setBitDepth	(int16_t d) {

	if (d < 0 || d > 32)
	   d = 24;

	normalizer	= 1;
	while (-- d > 0) 
	   normalizer <<= 1;
}

