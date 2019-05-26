
#include	"iq-creator.h"


		iq_creator::iq_creator	(uint16_t v, int dir) {
	(void)v;
	this	-> dir = dir;
	p	= 0;
}

		iq_creator::~iq_creator	(void) {
}

std::complex<float> iq_creator::Pass	(float v) {
        ibuffer [p]	= v;
        qbuffer [p]	= v;
        int     cp      = p + BUFSIZE;
        float sum_1 = 1.0 / 7 * (ibuffer [p] - ibuffer [(cp - 14) % BUFSIZE]);
        float sum_2 = 1.0 / 6 * (ibuffer [(cp - 1) % BUFSIZE] -
                                              ibuffer [(cp - 13) % BUFSIZE]);
        float sum_3 = 1.0 / 5 * (ibuffer [(cp - 2) % BUFSIZE] -
                                              ibuffer [(cp - 12) % BUFSIZE]);
        float sum_4 = 1.0 / 4 * (ibuffer [(cp - 3) % BUFSIZE] -
                                              ibuffer [(cp - 11) % BUFSIZE]);
        float sum_5 = 1.0 / 3 * (ibuffer [(cp - 4) % BUFSIZE] -
                                              ibuffer [(cp - 10) % BUFSIZE]);
        float sum_6 = 1.0 / 2 * (ibuffer [(cp - 5) % BUFSIZE] -
                                              ibuffer [(cp -  9) % BUFSIZE]);
        float sum_7 =           (-ibuffer [(cp - 6) % BUFSIZE] +
                                              ibuffer [(cp - 8) % BUFSIZE]);

        float new_i = - sum_1 + sum_2 +  - sum_3 + sum_4 + - sum_5
                             + sum_6 + sum_7;
	if (dir == 0)
           return std::complex <float> (new_i, qbuffer [(cp - 15) % BUFSIZE]);
	else
           return std::complex <float> (qbuffer [(cp - 15) % BUFSIZE], new_i);
	 
}


