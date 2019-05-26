#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the pcm handler
 *
 *    pcm handler is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    pcm handler is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with pcm handler; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PCM_HANDLER__
#define __PCM_HANDLER__

#include	<stdint.h>
#include	<stdio.h>
#include	<sndfile.h>
#include	<complex>
#include	<atomic>
#include	"pcm-driver.h"
#include	"rds-driver.h"
#include	"ringbuffer.h"
#include	<string>

class pcmHandler {
public:
		pcmHandler		(int, pcmDriver *, rdsDriver *);
		~pcmHandler		(void);
	void	go			(std::string);
	void	stop			(void);
private:
	int	rate;
	pcmDriver	*my_pcmDriver;
	rdsDriver	*my_rdsDriver;
	std::atomic<bool>	running;
	bool		halted;
};

#endif

