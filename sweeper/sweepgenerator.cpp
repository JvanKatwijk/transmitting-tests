
#include	"sweepgenerator.h"

static inline
uint64_t         getMyTime       (void) {
struct timeval  tv;

        gettimeofday (&tv, NULL);
        return ((uint64_t)tv. tv_sec * 1000000 + (uint64_t)tv. tv_usec);
}

	sweepGenerator::sweepGenerator (
	                            tcpClient	*theGenerator,
	                            RingBuffer<std::complex<float>> *b,
	                            int	lowFreq,
	                            int highFreq,
	                            int	freqStep,
	                            int	samplesperFreq) {
	this	-> theGenerator	= theGenerator;
	this	-> scopeBuffer	= b;
	this	-> lowFreq	= lowFreq;
	this	-> highFreq	= highFreq;
	this	-> freqStep	= freqStep;
	this	-> samplesperFreq	= samplesperFreq;

	running. store (false);
}

	sweepGenerator::~sweepGenerator (void) {
	running. store (false);
	while (isRunning ())
	   usleep (1000);
}

void	sweepGenerator::reset	(int freqStep, int samplesperFreq) {
	this	-> freqStep		= freqStep;
	this	-> samplesperFreq	= samplesperFreq;
	fprintf (stderr, "samplesperFreq = %d\n", samplesperFreq);
	running. store (false);
	while (isRunning ())
	   usleep (1000);

	start ();
}

void	sweepGenerator::run	(void) {
float	currentPhase	= 0;
float	range	= highFreq - lowFreq;
int	sampleCount	= 0;
int	phaseOffset	= 0;
int	localCounter	= 0;
uint64_t	nextStop;

	running. store (true);
	nextStop	= getMyTime () + 100000 / 4;
	while (running. load ()) {
	   std::complex<float> sample =
	      std::complex<float> (cos (currentPhase / range * 2 * M_PI),
	                           sin (currentPhase / range * 2 * M_PI));
	   int16_t intSample [2];
	   intSample [0] = (int16_t)(real (sample) * 16384);
	   intSample [1] = (int16_t)(imag (sample) * 16384);
	   theGenerator	-> sendData (intSample, 2);
	   scopeBuffer	-> putDataIntoBuffer (&sample, 1);
	   if (scopeBuffer -> GetRingBufferReadAvailable () >= 1024)
	      emit showSpectrum ();

	   sampleCount ++;
	   if (sampleCount >= samplesperFreq) {
	      sampleCount = 0;
	      phaseOffset	+=  freqStep;
	      if (phaseOffset >= range)
	         phaseOffset -= range;
	   }
	   currentPhase += phaseOffset;

	   localCounter ++;
	   if (localCounter >= range / 4) {
              localCounter -= range / 4;
              uint64_t currentTime      = getMyTime ();
              if (nextStop > currentTime)
                 usleep (nextStop - currentTime);
              nextStop += 1000000 / 4;
           }

	}
}

