
#ifndef	__IQ_CREATOR__
#define	__IQ_CREATOR__
#include	<stdlib.h>
#include	<stdint.h>
#include	<math.h>
#include	<vector>
#include	<complex>

class	iq_creator {
public:
			iq_creator	(uint16_t size, int dir = 0);
			~iq_creator	(void);
std::complex<float>	Pass		(float v);
private:

#define	BUFSIZE	15
	int	dir;
	float   ibuffer [BUFSIZE];
	float   qbuffer [BUFSIZE];
	int	p;

};

#endif


