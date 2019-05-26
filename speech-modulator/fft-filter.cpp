#
/*
 *    Copyright (C) 2018, 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the speech modulator
 *
 *    speech modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    speech modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with speech modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"fft-filter.h"
#include	<cstring>

//

	fftFilter::fftFilter (int32_t size,
	                      int16_t degree) {
int32_t	i;

	fftSize		= size;
	filterDegree	= degree;
	OverlapSize	= filterDegree;
	NumofSamples	= fftSize - OverlapSize;

	FFT_A		= new std::complex<float> [fftSize];
	FFT_C		= new std::complex<float> [fftSize];
	filterVector	= new std::complex<float> [fftSize];
	RfilterVector	= new float [fftSize];

	Overloop	= new std::complex<float> [OverlapSize];
	inp		= 0;
	for (i = 0; i < fftSize; i ++) {
	   FFT_A [i] = 0;
	   FFT_C [i] = 0;
	   filterVector [i] = 0;
	   RfilterVector [i] = 0;
	}
	cosTable.    resize (size / 2);
	sinTable.    resize (size / 2);
        for (int i = 0; i < size / 2; i ++) {
           cosTable [i] = cos (-2 * M_PI * i / size);
           sinTable [i] = sin (-2 * M_PI * i / size);
        }

	blackman	= new float [filterDegree];
	for (i = 0; i < filterDegree; i ++)
	   blackman [i] = (0.42 -
		    0.5 * cos (2 * M_PI * (float)i / (float)filterDegree) +
		    0.08 * cos (4 * M_PI * (float)i / (float)filterDegree));
}

	fftFilter::~fftFilter () {
	delete[]	FFT_A;
	delete[]	FFT_C;
	delete[]	RfilterVector;
	delete[]	Overloop;
	delete[]	blackman;
}

void	fftFilter::fft (std::complex<float> *v, int n){
int i, j, k, n1, n2, a;
double c, s, e, t1, t2;        
float	x [n];
float	y [n];
int	m	= log2 (n);
	
	for (i = 0; i < n; i ++) {
	   x [i] = real (v [i]);
	   y [i] = imag (v [i]);
	}

	j = 0; /* bit-reverse */
	n2 = n/2;
	for (i = 1; i < n - 1; i++) {
	   n1 = n2;
	   while (j >= n1) {
	      j = j - n1;
	      n1 = n1/2;
	   }
	   j = j + n1;
               
	   if (i < j) {
	      t1	= x [i];
	      x [i]	= x [j];
	      x [j]	= t1;
	      t1	= y [i];
	      y [i]	= y[j];
	      y [j]	= t1;
	   }
	}

/* FFT */                                                                                 
	n1 = 0; 
	n2 = 1;
                                             
	for (i = 0; i < m; i++) {
	   n1 = n2;
	   n2 = n2 + n2;
	   a = 0;

	   for (j = 0; j < n1; j++) {
	      c = cosTable [a];
	      s = sinTable [a];
	      a += 1 << (m - i - 1);

	      for (k = j; k < n; k = k + n2) {
	         t1 = c * x [k + n1] - s * y [k + n1];
	         t2 = s * x [k + n1] + c * y [k + n1];
	         x [k + n1] = x [k] - t1;
	         y [k + n1] = y [k] - t2;
	         x [k] = x [k] + t1;
	         y [k] = y [k] + t2;
	      }
	   }
	}

	for (i = 0; i < n; i ++)
	   v [i] = std::complex<float> (x [i], y [i]);
}                 


void	fftFilter::ifft (std::complex<float>* data, int size) {
	for (int i = 0; i < size; i ++)
	   data [i] = conj (data [i]);
	fft (data, size);
	for (int i = 0; i < size; i ++)
	   data [i] = conj (data [i]);

	for (int i = 0; i < size; i++) {
	   data [i] /= size;
	}
}
//
//	set the band to a new value, i.e. create a new kernel
void	fftFilter::setBand (int32_t low, int32_t high, int32_t rate) {
float	tmp [filterDegree];
float	lo	= (float)((high - low) / 2) / rate;
float	shift	= (float) ((high + low) / 2) / rate;
float	sum	= 0.0;
int16_t	i;

	for (i = 0; i < filterDegree; i ++) {
	   if (i == filterDegree / 2)
	      tmp [i] = 2 * M_PI * lo;
	   else 
	      tmp [i] = sin (2 * M_PI * lo *
	                  (i - filterDegree /2)) / (i - filterDegree / 2);
//
//	windowing, according to Blackman
	   tmp [i]  *= blackman [i];
	   sum += tmp [i];
	}
//
//	Now the modulation
	for (i = 0; i < filterDegree; i ++) {	// shifting
	   float v = (i - filterDegree / 2) * (2 * M_PI * shift);
	   filterVector [i] = std::complex<float> (tmp [i] * cos (v) / sum, 
	                                           tmp [i] * sin (v) / sum);
	}

	memset (&filterVector [filterDegree], 0,
	                (fftSize - filterDegree) * sizeof (std::complex<float>));
	fft (filterVector, fftSize);
	inp		= 0;
}

std::complex<float>	fftFilter::Pass (std::complex<float> z) {
std::complex<float>	sample;
int16_t		j;

	sample	= FFT_C [inp];
	FFT_A [inp] = z;

	if (++inp >= NumofSamples) {
	   inp = 0;
	   memset (&FFT_A [NumofSamples], 0,
	               (fftSize - NumofSamples) * sizeof (std::complex<float>));
	   fft (FFT_A, fftSize);

	   for (j = 0; j < fftSize; j ++) 
	      FFT_C [j] = FFT_A [j] * filterVector [j];

	   ifft (FFT_C, fftSize);
	   for (j = 0; j < OverlapSize; j ++) {
	      FFT_C [j] += Overloop [j];
	      Overloop [j] = FFT_C [NumofSamples + j];
	   }
	}

	return sample;
}


