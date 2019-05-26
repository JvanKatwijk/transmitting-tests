#ifndef __FILE_PROCESSOR__
#define __FILE_PROCESSOR__

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
#include        <QThread>
#include        <QObject>

class   sweepGenerator:public QThread {
Q_OBJECT
public:

		sweepGenerator (tcpClient	*theGenerator,
	                        RingBuffer<std::complex<float>> *b,
	                        int	lowFreq,
	                        int	highFreq,
	                        int	freqStep,
	                        int	samplesperFreq);
		~sweepGenerator (void);
	void	reset		(int freqStep, int samplesperFreq);
	void	run		(void);
private:
	tcpClient	*theGenerator;
	RingBuffer<std::complex<float>> *scopeBuffer;
	int		lowFreq;
	int		highFreq;
	int		freqStep;
	int		samplesperFreq;
	std::atomic<bool>	running;
signals:
	void		showSpectrum	(void);
};

#endif



	
	     
