#
/*
 *    Copyright (C) 2019
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
 *    speec modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with speec modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef	__FFT_FILTER
#define	__FFT_FILTER

#include	<stdio.h>
#include	<stdint.h>
#include	<complex>
#include	<vector>

class	fftFilter {
public:
			fftFilter	(int32_t, int16_t);
			~fftFilter	(void);
	void		setBand		(int32_t, int32_t, int32_t);
	std::complex<float>
	                Pass		(std::complex<float>);

private:
	void		fft		(std::complex<float> *, int);
	void		ifft		(std::complex<float> *, int);
	int32_t		fftSize;
	int16_t		filterDegree;
	int16_t		OverlapSize;
	int16_t		NumofSamples;
	std::vector<float>	cosTable;
	std::vector<float>	sinTable;
	std::complex<float>	*FFT_A;
	std::complex<float>	*FFT_C;
	std::complex<float>	*filterVector;
	float			*RfilterVector;
	std::complex<float>	*Overloop;
	int32_t		inp;
	float		*blackman;
};

#endif

