
#ifndef	__FILE_PROCESSOR__
#define	__FILE_PROCESSOR__

#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include        <sndfile.h>
#include        <samplerate.h>
#include        <math.h>
#include        <atomic>
#include        <sys/time.h>
#include        <complex>
#include        "tcp-client.h"
#include	<QThread>
#include	<QObject>

class	fileProcessor:public QThread {
Q_OBJECT
public:
		fileProcessor (SNDFILE		*inputfile,
	                       SF_INFO		*sf_info,
	                       int		rate,
	                       tcpClient	*theGenerator,
	                       RingBuffer<std::complex<float>> *scopeBuffer);

		~fileProcessor	(void);

	void	run		(void);
	void	processFileDirect (void);
	void	processFileConvert (void);

private:
	void	modulateData (float *bo, int amount, int channels);
	SNDFILE		*inFile;
	SF_INFO		*sf_info;
	int		rate;
	tcpClient	*theGenerator;
	RingBuffer<std::complex<float>> *scopeBuffer;
	std::atomic<bool>	running;
	float		nextPhase;
	  struct { double l;
                 double r;
        } samp, samp_s, samp_p, samp_sp;
        int     pos;
signals:
	void		showSpectrum	(void);
};

#endif
