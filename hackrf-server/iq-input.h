#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the hackrf server
 *    hackrf server is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    hackrf server is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with hackrf server; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__IQ_INPUT__
#define	__IQ_INPUT__

#include	<stdint.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<string>
#include	<thread>
#include	<unistd.h>
#include	<atomic>
#include	<complex>
#include	"ringbuffer.h"

class	iq_Input  {
public:
		iq_Input	(int, RingBuffer<uint8_t> *);
		~iq_Input	(void);
	void	start		(void);
	void	stop		(void);
private:
	int	server_fd;
	void	run		(void);
	int		port;
	RingBuffer<uint8_t>	*buffer;
	std::thread	threadHandle;
	std::atomic<bool>	running;
	std::atomic<bool>	connected;
};
#endif

