#
/*
 *    Copyright (C) 2019
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the fm modulator
 *    fm modulator is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    fm modulator is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fm modulator; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *      rds encoding is heavily based on the "fmstation"
 *      from Jan Roemisch (github.com/maxx23), all rights acknowledged
 */

#include        <stdio.h>
#include        <cstring>
#include        <signal.h>
#include        <stdlib.h>
#include        <stdint.h>
#include	<sndfile.h>
#include	<samplerate.h>
#include	<math.h>
#include	<atomic>
#include	<sys/time.h>
#include	<complex>
#include	<unistd.h>
#include	<vector>
#include	"fm-modulator.h"

static inline
int64_t         getMyTime       (void) {
struct timeval  tv;

	gettimeofday (&tv, NULL);
	return ((int64_t)tv. tv_sec * 1000000 + (int64_t)tv. tv_usec);
}

		fmModulator::fmModulator (int		rate,
	                                  outputDriver	*theGenerator) {

	this	-> rate			= rate;
	this	-> theGenerator		= theGenerator;
	running. store (false);
	pos			= 0;
        nextPhase               = 0;

	sinTable		= new float [rate];
	for (int i = 0; i < rate; i ++)
	   sinTable [i] = sin ((float)i / rate * 2 * M_PI);

        m			= 0;
        e			= 0;
        prev_e			= 0;
        bpos			= 0;
        symclk_p		= 0.0;
        symclk_b		= 1;

	rds_info.	pi	= 0x10f0;
	rds_info.	af [0]	= 0xE210;
	rds_info.	af [1]	= 0x20E0;
	rds_info.	di	= RDS_GROUP_0A_DI_STEREO;
	rds_info.	pty	= 0x00;
	memcpy (&rds_info. ps_name [0], "<home>", 7);
	memcpy (&rds_info. radiotext [0], "no transmission yet\r", 19);
	rds_init_groups	(&rds_info);
	group		= rds_group_schedule ();
	halted		= true;		// still not running
}

	fmModulator::~fmModulator (void) {
	running. store (false);
	while (!halted)
	   usleep (1000);
	delete sinTable;
}

void	fmModulator::stop (void) {
	running. store (false);
}

void	fmModulator::go (std::string fileName) {
SF_INFO	sf_info;
SNDFILE	*infile		= sf_open (fileName. c_str (), SFM_READ, &sf_info);
int32_t outputLimit     = 2 * rate / 10;
double  ratio;
int     bufferSize;
std::vector<float>      bi (0);
std::vector<float>      bo (0);
int     error;
int     sampleCounter   = 0;
uint64_t        nextStop;
SRC_STATE       *converter =
	                src_new (SRC_SINC_MEDIUM_QUALITY, 2, &error);
SRC_DATA        *src_data =
	                new SRC_DATA;

	if (infile == NULL) {
	   fprintf (stderr, "could not open %s\n", fileName. c_str ());
	   return; 
	}

	if (converter == NULL) {
	   fprintf (stderr, "error creating a converter %d\n", error);
	   return;
	}

	halted		= false;
	ratio		= (double)rate / sf_info. samplerate;
	bufferSize	= ceil (outputLimit / ratio);
	bi. resize (bufferSize);
	bo. resize (outputLimit);
	fprintf (stderr,
	        "Starting converter with ratio %f (in %d, out %d)\n",
	                                      ratio, sf_info. samplerate,
	                                              rate);
//
//	the rds text will be the name of the file that is being played
	rds_info.	pi	= 0x10f0;
	rds_info.	af [0]	= 0xE210;
	rds_info.	af [1]	= 0x20E0;
	rds_info.	di	= sf_info. channels == 2 ?
	                                RDS_GROUP_0A_DI_STEREO:
	                                RDS_GROUP_0A_MUSIC;
	rds_info.	pty	= 0x00;
	memset (&rds_info. ps_name [0], 0, RDS_PS_LEN);
	memcpy (&rds_info. ps_name [0], "<local>", 8);
	memcpy (&rds_info. radiotext [0], fileName. c_str () , fileName. length ());
	rds_info. radiotext [fileName. length ()] = '\r';
	rds_info. radiotext [fileName. length () + 1] = 0;
	rds_init_groups	(&rds_info);
	group		= rds_group_schedule ();

	running. store (true);
	src_data -> data_in          = bi. data ();
	src_data -> data_out         = bo. data ();
	src_data -> src_ratio        = ratio;
	src_data -> end_of_input     = 0;
	nextStop                     = getMyTime ();

	while (running. load ()) {
	   int  readCount;
	   readCount    = sf_readf_float (infile, bi. data (), bufferSize / 2);
	   if (readCount <= 0) {
	      sf_seek (infile, 0, SEEK_SET);
	      fprintf (stderr, "eof reached\n");
	      sf_close (infile);
	      usleep (10000);
	      running. store (false);
	      return;
	   }

	   if (sf_info. channels == 1) {
	      float temp [bufferSize / 2];
	      memcpy (temp, bi. data (), readCount * sizeof (float));
	      for (int j = 0; j < readCount; j ++) {
	         bi [2 * j] = temp [j];
	         bi [2 * j + 1] = 0;
	      }
	   }

	   src_data     -> input_frames         = readCount;
	   src_data     -> output_frames        = outputLimit / 2;
	   int res      = src_process (converter, src_data);
//
//      The samplerate is now "rate", we do not want to overload
//      the socket, so we built in a break after rate/4 samples
	   sampleCounter        += src_data -> output_frames_gen;
	   if (sampleCounter > rate / 4) {
	      sampleCounter		-= rate / 4;
	      uint64_t currentTime      = getMyTime ();
	      if (nextStop > currentTime)
	         usleep (nextStop - currentTime);
	      nextStop += 1000000 / 4;
	   }

	   modulateData (bo. data (), 
	                 src_data -> output_frames_gen, sf_info. channels);
	}
	halted	= true;
}

//
//	Preemphasis filter derived from the one by Jonti (Jonathan Olds)
static	double a0	= 5.309858008;
static	double a1	= -4.794606188;
static	double b1	= 0.4847481783;

float sample;
struct sample_i {
	float l;
	float r;
};

struct sample_i samp_s;

struct sample_i preemp (struct sample_i in) {
static	double	x_l	= 0;
static	double	x_r	= 0;
static	double	y_l	= 0;
static	double	y_r	= 0;
struct sample_i res;

	y_l	= a0 * in.l + a1 * x_l - b1 * y_l;
	x_l	= in. l;
	y_r	= a0 * in.r + a1 * x_r - b1 * y_r;
	x_r	= in. r;
	res. l = y_l;
	res. r = y_r;
	return res;
}

float	preemp (float in) {
static	double	x	= 0;
static	double	y	= 0;

	y	= a0 * in + a1 * x - b1 * y;
	x	= in;
	return y;
}
	
float	lowPass	(float s) {
static double	out	= 0;
	out	= out  + 0.05 * (s - out);
	return out;
}

void	fmModulator::modulateData (float *bo, int amount, int channels) {
int	i;
	for (i = 0; i < amount; i ++) {
	   double clock		= 2 * M_PI * (double)pos * 19000.0 / rate;
//	   double pilot		= sin (clock);
//	   double carrier	= sin (2 * clock);
//	   double rds_carrier	= sin (3 * clock);
//	   double symclk	= sin (clock / 16.0);

//	the period is rate / 19000, which does not result
//	in an int value, however, the 3468 works fine
	   int	ind_1	= (int)(((int64_t)pos * 19000) % rate);
	   int	ind_2	= (int)(((int64_t)pos * 19000 * 2) % rate);
	   int	ind_3	= (int)(((int64_t)pos * 19000 * 3) % rate);
	   int	ind_4	= (int)(((int64_t)pos * 19000 / 16) % rate);
	   double	pilot	= sinTable [ind_1];
	   double	carrier	= sinTable [ind_2];
	   double	rds_carrier = sinTable [ind_3];
	   double	symclk	= sinTable [ind_4];
	   double bit_d;

//#define MANCH_ENCODE(clk, bit) \
//        (((clk ^ bit) << 1) | ((clk ^ 0x1 ^ bit)))

//#define NRZ(in) \
//        (double)(((int)((in) & 0x1) << 1) - 1)

	   if ((symclk_p <= 0) && (symclk > 0)) {
	      e		= group -> bits [bpos]^ prev_e;
	      m		= MANCH_ENCODE (symclk_b, e);
	      bit_d	= NRZ (m);
	      bpos	= (bpos + 1) % RDS_GROUP_LEN;
	      prev_e	= e;
	      symclk_b ^= 1;

	      if (bpos == 0) {
	         group = rds_group_schedule ();
	      }
//	      if (pos > 34560000l)
	      if (pos > 6 * 384)
//	      if (pos > 3648)
	         pos = 0;
	   }

	   if ((symclk_p >= 0) && (symclk < 0)) {
	      bit_d = NRZ (m >> 1);
	      symclk_b ^= 1;
	   }
//
//	finally, fetch the (left part of the) sample
	   samp_s.l  = bo [i * channels];
	   if ((channels == 2) && (rate >= 192000)) {
	      samp_s.r		= bo [i * channels + 1];
//
//	sound elements are preempted
	      samp_s		= preemp (samp_s);

	      double lpr	= (samp_s.l + samp_s.r);
	      double lmr	= (samp_s.l - samp_s.r);
//
//	the rds signal
//
//	and build up the resulting sample
	      float sym		= lowPass (fabs (symclk) * bit_d);
	      sample		= 0.60	* lpr +
	                          0.05	* pilot +
                                  0.50	* lmr * carrier +
	                          0.05	* sym * rds_carrier;

	      symclk_p	= symclk;
	   }
	   else
	   if (channels == 2) 
	      sample	= preemp (samp_s.l + bo [i * 2]) / 2;
	   else
	      sample	= preemp (samp_s.l);

	   nextPhase += 8 * sample;
	   if (nextPhase >= 2 * M_PI)
	      nextPhase -= 2 * M_PI;

	   std::complex<float>xxx =
	              std::complex<float> (cos (nextPhase), sin (nextPhase));
	   theGenerator -> sendSample (xxx);
	   pos++;
	}
}

uint16_t fmModulator::rds_crc (uint16_t in) {
int i;
uint16_t reg = 0;
static const uint16_t rds_poly = 0x5B9;

	for (i = 16; i > 0; i--)  {
	   reg = (reg << 1) | ((in >> (i - 1)) & 0x1);
	   if (reg & (1 << 10))
	      reg = reg ^ rds_poly;
	}

	for (i = 10; i > 0; i--) {
	   reg = reg << 1;
	   if (reg & (1 << 10))
	      reg = reg ^ rds_poly;
	}
        return (reg & ((1 << 10) - 1));
}

void	fmModulator::rds_bits_to_values (char *out,
	                                 uint16_t in, int len) {
int n	= len;
uint16_t mask;

        while (n--) {
	   mask = 1 << n;
	   out[len - 1 - n] = ((in & mask) >> n) & 0x1;
        }
}

void	fmModulator::rds_serialize (struct rds_group_s *group, char flags) {
int n = 4;
static
const uint16_t rds_checkwords[] = {0xFC, 0x198, 0x168, 0x1B4, 0x350, 0x0};

	while (n--) {
	   int start		= RDS_BLOCK_LEN * n;
	   uint16_t block	= group -> blocks [n];
	   uint16_t crc;

	   if (!((flags >> n) & 0x1))
	      continue;

	   crc  = rds_crc (block);
	   crc ^= rds_checkwords [n];
	   rds_bits_to_values (&group -> bits [start],
	                               block,
	                               RDS_BLOCK_DATA_LEN);
	   rds_bits_to_values (&group -> bits [start + RDS_BLOCK_DATA_LEN],
	                               crc,
	                               RDS_BLOCK_CRC_LEN);
	}
}
//
//

void	fmModulator::rds_init_groups (struct rds_info_s *info) {

	group_0a. info_block		= info;
	group_0a. blocks [index_A]	= info -> pi;
	group_0a. blocks [index_B]	= RDS_GROUP (RDS_GROUP_TYPE (0, 0))
	                                             | RDS_TP
	                                             | RDS_PTY (info -> pty)
	                                             | RDS_GROUP_0A_MUSIC;

	group_0a. info	= ((info -> af [0] >> 8) & 0xff) - 0xE0;
        rds_serialize (&group_0a, RDS_BLOCK_A);

	group_2a. info_block		= info;
	group_2a. blocks [index_A]	= info -> pi;
	group_2a. blocks [index_B]	= RDS_GROUP (RDS_GROUP_TYPE (2, 0))
	                                             | RDS_TP
	                                             | RDS_PTY (info -> pty);

	group_2a. info	= strnlen (info -> radiotext, RDS_RT_MAX);
	fprintf (stderr, "length of rds message %d\n", group_2a. info);
	rds_serialize (&group_2a, RDS_BLOCK_A);

	group_3a. info_block		= info;
	group_3a. blocks [index_A]	= info -> pi;
	group_3a. blocks [index_B]	= RDS_GROUP (RDS_GROUP_TYPE (3, 0))
	                                             | RDS_TP
	                                             | RDS_PTY (info -> pty)
	                                             | RDS_TMC_GROUP;

	group_3a. blocks [index_D]	= RDS_TMC_AID;
	rds_serialize (&group_3a, RDS_BLOCK_A | RDS_BLOCK_B | RDS_BLOCK_D);

	group_8a. info_block		= info;
	group_8a. blocks [index_A]	= info -> pi;
	group_8a. blocks [index_B]	= RDS_GROUP (RDS_GROUP_TYPE (8, 0))
	                                             | RDS_TP
	                                             | RDS_PTY (info->pty)
	                                             | RDS_TMC_F
	                                             | RDS_TMC_DP(0);

	group_8a. blocks [index_C]	= RDS_TMC_D | RDS_TMC_PN
	                                             | RDS_TMC_EXTENT (1)
	                                             | RDS_TMC_EVENT (1478);
	group_8a. blocks [index_D]	= RDS_TMC_LOCATION (12693);
	rds_serialize (&group_8a, RDS_BLOCK_A | RDS_BLOCK_B
                                          | RDS_BLOCK_C | RDS_BLOCK_D);
}
//
//	group 0: deals with af and ps name
void	fmModulator::rds_group_0A_update (void) {
static int af_pos = 0, ps_pos = 0;
uint16_t di = (group_0a. info_block -> di >> (ps_pos >> 1)) & 0x1;

	group_0a. blocks [index_B] =
	             group_0a. blocks [index_B] & 0xfff8 | di << 2 | ps_pos >> 1;
        group_0a. blocks [index_C] = group_0a. info_block -> af [af_pos];
        group_0a. blocks [index_D] =
	             group_0a. info_block -> ps_name [ps_pos] << 8
	                        | group_0a. info_block -> ps_name [ps_pos + 1];

	rds_serialize (&group_0a, RDS_BLOCK_B | RDS_BLOCK_C | RDS_BLOCK_D);

        af_pos = (af_pos + 1) % group_0a. info;
        ps_pos = (ps_pos + 2) & (RDS_PS_LEN - 1);
}
//
//	group 2: deals with the tekst
void	fmModulator::rds_group_2A_update (void) {
static int rt_pos = 0;
static int b_pos = 0;

	group_2a. blocks [index_B] =
	             group_2a. blocks [index_B] & 0xffe0 | b_pos;
	group_2a. blocks [index_C] =
	             group_2a. info_block -> radiotext [rt_pos + 0] << 8
	           | group_2a. info_block -> radiotext [rt_pos + 1];
	group_2a. blocks [index_D] =
	             group_2a. info_block -> radiotext [rt_pos + 2] << 8
	           | group_2a. info_block -> radiotext [rt_pos + 3];

	rds_serialize (&group_2a, RDS_BLOCK_B | RDS_BLOCK_C | RDS_BLOCK_D);

	b_pos = (b_pos + 1) & 0xf;
	if ((rt_pos += 4) > group_2a. info) {
	   rt_pos = 0, b_pos = 0;
	}
}

void	fmModulator::rds_group_3A_update (void) {
static int toggle = 1;

	if (toggle)
	   group_3a. blocks [index_C] = RDS_TMC_VAR (0)
	                              | RDS_TMC_LTN (1)
	                              | RDS_TMC_N
	                              | RDS_TMC_R;
	else
	   group_3a. blocks [index_C] = RDS_TMC_VAR (1)
	                              | RDS_TMC_GAP (0)
	                              | RDS_TMC_SID (0)
	                              | RDS_TMC_TA (0)
	                              | RDS_TMC_TW (0)
	                              | RDS_TMC_TD (0);

	rds_serialize (&group_3a, RDS_BLOCK_C);
        toggle ^= 1;
}

void	fmModulator::rds_group_8A_update (void) {
}

struct rds_group_s *fmModulator::rds_group_schedule (void) {
static int ps = 1;
struct rds_group_s *group;

	switch (ps) {
	   case 0:
	      rds_group_0A_update ();
	      group	= &group_0a;
	      break;

	   case 1:
	      rds_group_2A_update ();
	      group	= &group_2a;
	      break;

	   case 2:
	      rds_group_3A_update ();
	      group	= &group_3a;
	      break;

	   case 3:
	      rds_group_8A_update ();
	      group	= &group_8a;
	      break;
	}

        ps = (ps + 1) % 4;
        return group;
}

