/***************************************************************************

  Modified from ay8910.c from MAME


  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

***************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "ay8910.h"

#define MAX_OUTPUT 0x7fff

#define STEP 0x8000

#define BURN_SND_CLIP(A) ((A) < -0x8000 ? -0x8000 : (A) > 0x7fff ? 0x7fff : (A))
#define BURN_SND_ROUTE_LEFT			1
#define BURN_SND_ROUTE_RIGHT		2
#define BURN_SND_ROUTE_BOTH			(BURN_SND_ROUTE_LEFT | BURN_SND_ROUTE_RIGHT)

static void (*AYStreamUpdate)(void);

INT32 ay8910_index_ym = 0;
static INT32 num = 0, ym_num = 0;

static double AY8910Volumes[3 * 6];
static INT32 AY8910RouteDirs[3 * 6];

struct AY8910
{
	INT32 Channel;
	INT32 SampleRate;
   ALIGN_VAR(8) read8_handler PortAread;
	ALIGN_VAR(8) read8_handler PortBread;
	ALIGN_VAR(8) write8_handler PortAwrite;
	ALIGN_VAR(8) write8_handler PortBwrite;
	ALIGN_VAR(8) INT32 register_latch;
	UINT8 Regs[16];
	INT32 lastEnable;
	UINT32 UpdateStep;
	INT32 PeriodA,PeriodB,PeriodC,PeriodN,PeriodE;
	INT32 CountA,CountB,CountC,CountN,CountE;
	UINT32 VolA,VolB,VolC,VolE;
	UINT8 EnvelopeA,EnvelopeB,EnvelopeC;
	UINT8 OutputA,OutputB,OutputC,OutputN;
	INT8 CountEnv;
	UINT8 Hold,Alternate,Attack,Holding;
	INT32 RNG;
	UINT32 VolTable[32];
};

/* register id's */
#define AY_AFINE	(0)
#define AY_ACOARSE	(1)
#define AY_BFINE	(2)
#define AY_BCOARSE	(3)
#define AY_CFINE	(4)
#define AY_CCOARSE	(5)
#define AY_NOISEPER	(6)
#define AY_ENABLE	(7)
#define AY_AVOL		(8)
#define AY_BVOL		(9)
#define AY_CVOL		(10)
#define AY_EFINE	(11)
#define AY_ECOARSE	(12)
#define AY_ESHAPE	(13)

#define AY_PORTA	(14)
#define AY_PORTB	(15)

static struct AY8910 AYPSG[MAX_8910];		/* array of PSG's */

static void _AYWriteReg(INT32 n, INT32 r, INT32 v)
{
	struct AY8910 *PSG = &AYPSG[n];
	INT32 old;


	PSG->Regs[r] = v;

	/* A note about the period of tones, noise and envelope: for speed reasons,*/
	/* we count down from the period to 0, but careful studies of the chip     */
	/* output prove that it instead counts up from 0 until the counter becomes */
	/* greater or equal to the period. This is an important difference when the*/
	/* program is rapidly changing the period to modulate the sound.           */
	/* To compensate for the difference, when the period is changed we adjust  */
	/* our internal counter.                                                   */
	/* Also, note that period = 0 is the same as period = 1. This is mentioned */
	/* in the YM2203 data sheets. However, this does NOT apply to the Envelope */
	/* period. In that case, period = 0 is half as period = 1. */
	switch( r )
	{
	case AY_AFINE:
	case AY_ACOARSE:
		PSG->Regs[AY_ACOARSE] &= 0x0f;
		old = PSG->PeriodA;
		PSG->PeriodA = (PSG->Regs[AY_AFINE] + 256 * PSG->Regs[AY_ACOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodA == 0) PSG->PeriodA = PSG->UpdateStep;
		PSG->CountA += PSG->PeriodA - old;
		if (PSG->CountA <= 0) PSG->CountA = 1;
		break;
	case AY_BFINE:
	case AY_BCOARSE:
		PSG->Regs[AY_BCOARSE] &= 0x0f;
		old = PSG->PeriodB;
		PSG->PeriodB = (PSG->Regs[AY_BFINE] + 256 * PSG->Regs[AY_BCOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodB == 0) PSG->PeriodB = PSG->UpdateStep;
		PSG->CountB += PSG->PeriodB - old;
		if (PSG->CountB <= 0) PSG->CountB = 1;
		break;
	case AY_CFINE:
	case AY_CCOARSE:
		PSG->Regs[AY_CCOARSE] &= 0x0f;
		old = PSG->PeriodC;
		PSG->PeriodC = (PSG->Regs[AY_CFINE] + 256 * PSG->Regs[AY_CCOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodC == 0) PSG->PeriodC = PSG->UpdateStep;
		PSG->CountC += PSG->PeriodC - old;
		if (PSG->CountC <= 0) PSG->CountC = 1;
		break;
	case AY_NOISEPER:
		PSG->Regs[AY_NOISEPER] &= 0x1f;
		old = PSG->PeriodN;
		PSG->PeriodN = PSG->Regs[AY_NOISEPER] * PSG->UpdateStep;
		if (PSG->PeriodN == 0) PSG->PeriodN = PSG->UpdateStep;
		PSG->CountN += PSG->PeriodN - old;
		if (PSG->CountN <= 0) PSG->CountN = 1;
		break;
	case AY_ENABLE:
		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x40) != (PSG->Regs[AY_ENABLE] & 0x40)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, (PSG->Regs[AY_ENABLE] & 0x40) ? PSG->Regs[AY_PORTA] : 0xff);
		}

		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x80) != (PSG->Regs[AY_ENABLE] & 0x80)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, (PSG->Regs[AY_ENABLE] & 0x80) ? PSG->Regs[AY_PORTB] : 0xff);
		}

		PSG->lastEnable = PSG->Regs[AY_ENABLE];
		break;
	case AY_AVOL:
		PSG->Regs[AY_AVOL] &= 0x1f;
		PSG->EnvelopeA = PSG->Regs[AY_AVOL] & 0x10;
		PSG->VolA = PSG->EnvelopeA ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_AVOL] ? PSG->Regs[AY_AVOL]*2+1 : 0];
		break;
	case AY_BVOL:
		PSG->Regs[AY_BVOL] &= 0x1f;
		PSG->EnvelopeB = PSG->Regs[AY_BVOL] & 0x10;
		PSG->VolB = PSG->EnvelopeB ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_BVOL] ? PSG->Regs[AY_BVOL]*2+1 : 0];
		break;
	case AY_CVOL:
		PSG->Regs[AY_CVOL] &= 0x1f;
		PSG->EnvelopeC = PSG->Regs[AY_CVOL] & 0x10;
		PSG->VolC = PSG->EnvelopeC ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_CVOL] ? PSG->Regs[AY_CVOL]*2+1 : 0];
		break;
	case AY_EFINE:
	case AY_ECOARSE:
		old = PSG->PeriodE;
		PSG->PeriodE = ((PSG->Regs[AY_EFINE] + 256 * PSG->Regs[AY_ECOARSE])) * PSG->UpdateStep;
		if (PSG->PeriodE == 0) PSG->PeriodE = PSG->UpdateStep / 2;
		PSG->CountE += PSG->PeriodE - old;
		if (PSG->CountE <= 0) PSG->CountE = 1;
		break;
	case AY_ESHAPE:
		/* envelope shapes:
		C AtAlH
		0 0 x x  \___

		0 1 x x  /___

		1 0 0 0  \\\\

		1 0 0 1  \___

		1 0 1 0  \/\/
		          ___
		1 0 1 1  \

		1 1 0 0  ////
		          ___
		1 1 0 1  /

		1 1 1 0  /\/\

		1 1 1 1  /___

		The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
		has twice the steps, happening twice as fast. Since the end result is
		just a smoother curve, we always use the YM2149 behaviour.
		*/
		PSG->Regs[AY_ESHAPE] &= 0x0f;
		PSG->Attack = (PSG->Regs[AY_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((PSG->Regs[AY_ESHAPE] & 0x08) == 0)
		{
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			PSG->Hold = 1;
			PSG->Alternate = PSG->Attack;
		}
		else
		{
			PSG->Hold = PSG->Regs[AY_ESHAPE] & 0x01;
			PSG->Alternate = PSG->Regs[AY_ESHAPE] & 0x02;
		}
		PSG->CountE = PSG->PeriodE;
		PSG->CountEnv = 0x1f;
		PSG->Holding = 0;
		PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
		if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
		if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
		if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
		break;
	case AY_PORTA:
		if (PSG->Regs[AY_ENABLE] & 0x40)
		{
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, PSG->Regs[AY_PORTA]);
		}
		break;
	case AY_PORTB:
		if (PSG->Regs[AY_ENABLE] & 0x80)
		{
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, PSG->Regs[AY_PORTB]);
		}
		break;
	}
}


/* write a register on AY8910 chip number 'n' */
static void AYWriteReg(INT32 chip, INT32 r, INT32 v)
{
   if (r > 15) return;
   if (r < 14)
   {
      struct AY8910 *PSG = &AYPSG[chip];

      if (r == AY_ESHAPE || PSG->Regs[r] != v)
      {
         //			/* update the output buffer before changing the register */
         //			stream_update(PSG->Channel,0);
         AYStreamUpdate();
      }
   }

   _AYWriteReg(chip,r,v);
}

static UINT8 AYReadReg(INT32 n, INT32 r)
{
   struct AY8910 *PSG = &AYPSG[n];


   if (r > 15) return 0;

   switch (r)
   {
      case AY_PORTA:
         /*
            even if the port is set as output, we still need to return the external
            data. Some games, like kidniki, need this to work.
            */
         if (PSG->PortAread) PSG->Regs[AY_PORTA] = (*PSG->PortAread)(0);
         break;
      case AY_PORTB:
         if (PSG->PortBread) PSG->Regs[AY_PORTB] = (*PSG->PortBread)(0);
         break;
   }
   return PSG->Regs[r];
}

void AY8910Write(INT32 chip, INT32 a, INT32 data)
{
	struct AY8910 *PSG = &AYPSG[chip];
	
	if (a & 1)
	{	/* Data port */
		AYWriteReg(chip,PSG->register_latch,data);
	}
	else
	{	/* Register port */
		PSG->register_latch = data & 0x0f;
	}
}

INT32 AY8910Read(INT32 chip)
{
	struct AY8910 *PSG = &AYPSG[chip];
	
	return AYReadReg(chip,PSG->register_latch);
}

void AY8910Update(INT32 chip, INT16 **buffer, INT32 length)
{
	struct AY8910 *PSG = &AYPSG[chip];
	INT16 *buf1,*buf2,*buf3;
	INT32 outn;
	
	buf1 = buffer[0];
	buf2 = buffer[1];
	buf3 = buffer[2];

	/* The 8910 has three outputs, each output is the mix of one of the three */
	/* tone generators and of the (single) noise generator. The two are mixed */
	/* BEFORE going into the DAC. The formula to mix each channel is: */
	/* (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable). */
	/* Note that this means that if both tone and noise are disabled, the output */
	/* is 1, not 0, and can be modulated changing the volume. */


	/* If the channels are disabled, set their output to 1, and increase the */
	/* counter, if necessary, so they will not be inverted during this update. */
	/* Setting the output to 1 is necessary because a disabled channel is locked */
	/* into the ON state (see above); and it has no effect if the volume is 0. */
	/* If the volume is 0, increase the counter, but don't touch the output. */
	if (PSG->Regs[AY_ENABLE] & 0x01)
	{
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
		PSG->OutputA = 1;
	}
	else if (PSG->Regs[AY_AVOL] == 0)
	{
		/* note that I do count += length, NOT count = length + 1. You might think */
		/* it's the same since the volume is 0, but doing the latter could cause */
		/* interferencies when the program is rapidly modulating the volume. */
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x02)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
		PSG->OutputB = 1;
	}
	else if (PSG->Regs[AY_BVOL] == 0)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x04)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
		PSG->OutputC = 1;
	}
	else if (PSG->Regs[AY_CVOL] == 0)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
	}

	/* for the noise channel we must not touch OutputN - it's also not necessary */
	/* since we use outn. */
	if ((PSG->Regs[AY_ENABLE] & 0x38) == 0x38)	/* all off */
		if (PSG->CountN <= length*STEP) PSG->CountN += length*STEP;

	outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);


	/* buffering loop */
	while (length)
	{
		INT32 vola,volb,volc;
		INT32 left;

		/* vola, volb and volc keep track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vola = volb = volc = 0;

		left = STEP;
		do
		{
			INT32 nextevent;

			if (PSG->CountN < left) nextevent = PSG->CountN;
			else nextevent = left;

			if (outn & 0x08)
			{
				if (PSG->OutputA) vola += PSG->CountA;
				PSG->CountA -= nextevent;
				/* PeriodA is the half period of the square wave. Here, in each */
				/* loop I add PeriodA twice, so that at the end of the loop the */
				/* square wave is in the same status (0 or 1) it was at the start. */
				/* vola is also incremented by PeriodA, since the wave has been 1 */
				/* exactly half of the time, regardless of the initial position. */
				/* If we exit the loop in the middle, OutputA has to be inverted */
				/* and vola incremented only if the exit status of the square */
				/* wave is 1. */
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						if (PSG->OutputA) vola += PSG->PeriodA;
						break;
					}
					PSG->CountA += PSG->PeriodA;
					vola += PSG->PeriodA;
				}
				if (PSG->OutputA) vola -= PSG->CountA;
			}
			else
			{
				PSG->CountA -= nextevent;
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						break;
					}
					PSG->CountA += PSG->PeriodA;
				}
			}

			if (outn & 0x10)
			{
				if (PSG->OutputB) volb += PSG->CountB;
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						if (PSG->OutputB) volb += PSG->PeriodB;
						break;
					}
					PSG->CountB += PSG->PeriodB;
					volb += PSG->PeriodB;
				}
				if (PSG->OutputB) volb -= PSG->CountB;
			}
			else
			{
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						break;
					}
					PSG->CountB += PSG->PeriodB;
				}
			}

			if (outn & 0x20)
			{
				if (PSG->OutputC) volc += PSG->CountC;
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						if (PSG->OutputC) volc += PSG->PeriodC;
						break;
					}
					PSG->CountC += PSG->PeriodC;
					volc += PSG->PeriodC;
				}
				if (PSG->OutputC) volc -= PSG->CountC;
			}
			else
			{
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						break;
					}
					PSG->CountC += PSG->PeriodC;
				}
			}

			PSG->CountN -= nextevent;
			if (PSG->CountN <= 0)
			{
				/* Is noise output going to change? */
				if ((PSG->RNG + 1) & 2)	/* (bit0^bit1)? */
				{
					PSG->OutputN = ~PSG->OutputN;
					outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);
				}

				/* The Random Number Generator of the 8910 is a 17-bit shift */
				/* register. The input to the shift register is bit0 XOR bit2 */
				/* (bit0 is the output). */

				/* The following is a fast way to compute bit17 = bit0^bit2. */
				/* Instead of doing all the logic operations, we only check */
				/* bit0, relying on the fact that after two shifts of the */
				/* register, what now is bit2 will become bit0, and will */
				/* invert, if necessary, bit15, which previously was bit17. */
				if (PSG->RNG & 1) PSG->RNG ^= 0x24000;
				PSG->RNG >>= 1;
				PSG->CountN += PSG->PeriodN;
			}

			left -= nextevent;
		} while (left > 0);

		/* update envelope */
		if (PSG->Holding == 0)
		{
			PSG->CountE -= STEP;
			if (PSG->CountE <= 0)
			{
				do
				{
					PSG->CountEnv--;
					PSG->CountE += PSG->PeriodE;
				} while (PSG->CountE <= 0);

				/* check envelope current position */
				if (PSG->CountEnv < 0)
				{
					if (PSG->Hold)
					{
						if (PSG->Alternate)
							PSG->Attack ^= 0x1f;
						PSG->Holding = 1;
						PSG->CountEnv = 0;
					}
					else
					{
						/* if CountEnv has looped an odd number of times (usually 1), */
						/* invert the output. */
						if (PSG->Alternate && (PSG->CountEnv & 0x20))
 							PSG->Attack ^= 0x1f;

						PSG->CountEnv &= 0x1f;
					}
				}

				PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
				/* reload volume */
				if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
				if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
				if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
			}
		}

		*(buf1++) = (vola * PSG->VolA) / STEP;
		*(buf2++) = (volb * PSG->VolB) / STEP;
		*(buf3++) = (volc * PSG->VolC) / STEP;

		length--;
	}
}


void AY8910_set_clock(INT32 chip, INT32 clock)
{
	struct AY8910 *PSG = &AYPSG[chip];
	
	/* the step clock for the tone and noise generators is the chip clock    */
	/* divided by 8; for the envelope generator of the AY-3-8910, it is half */
	/* that much (clock/16), but the envelope of the YM2149 goes twice as    */
	/* fast, therefore again clock/8.                                        */
	/* Here we calculate the number of steps which happen during one sample  */
	/* at the given sample rate. No. of events = sample rate / (clock/8).    */
	/* STEP is a multiplier used to turn the fraction into a fixed point     */
	/* number.                                                               */
	PSG->UpdateStep = ((double)STEP * PSG->SampleRate * 8 + clock/2) / clock;
}


static void build_mixer_table(INT32 chip)
{
	struct AY8910 *PSG = &AYPSG[chip];
	INT32 i;
	double out;


	/* calculate the volume->voltage conversion table */
	/* The AY-3-8910 has 16 levels, in a logarithmic scale (3dB per step) */
	/* The YM2149 still has 16 levels for the tone generators, but 32 for */
	/* the envelope generator (1.5dB per step). */
	out = MAX_OUTPUT;
	for (i = 31;i > 0;i--)
	{
		PSG->VolTable[i] = out + 0.5;	/* round to nearest */

		out /= 1.188502227;	/* = 10 ^ (1.5/20) = 1.5dB */
	}
	PSG->VolTable[0] = 0;
}



void AY8910Reset(INT32 chip)
{
	INT32 i;
	struct AY8910 *PSG = &AYPSG[chip];
	
	PSG->register_latch = 0;
	PSG->RNG = 1;
	PSG->OutputA = 0;
	PSG->OutputB = 0;
	PSG->OutputC = 0;
	PSG->OutputN = 0xff;
	PSG->lastEnable = -1;	/* force a write */
	for (i = 0;i < AY_PORTA;i++)
		_AYWriteReg(chip,i,0);	/* AYWriteReg() uses the timer system; we cannot */
								/* call it at this time because the timer system */
								/* has not been initialized. */
}

void AY8910Exit(INT32 chip)
{
	(void)chip;

	num = 0;
	ym_num = 0;

	ay8910_index_ym = 0;
}

static void dummy_callback(void)
{
	return;
}

INT32 AY8910Init(INT32 chip, INT32 clock, INT32 sample_rate,
		read8_handler portAread, read8_handler portBread,
		write8_handler portAwrite, write8_handler portBwrite)
{
	struct AY8910 *PSG = &AYPSG[chip];
	
	AYStreamUpdate = dummy_callback;

	if (chip != num) {
		return 1;
	}

	memset(PSG, 0, sizeof(struct AY8910));
	PSG->SampleRate = sample_rate;
	PSG->PortAread = portAread;
	PSG->PortBread = portBread;
	PSG->PortAwrite = portAwrite;
	PSG->PortBwrite = portBwrite;

	AY8910_set_clock(chip, clock);

	build_mixer_table(chip);
	
	// default routes
	AY8910Volumes[(chip * 3) + BURN_SND_AY8910_ROUTE_1] = 1.00;
	AY8910Volumes[(chip * 3) + BURN_SND_AY8910_ROUTE_2] = 1.00;
	AY8910Volumes[(chip * 3) + BURN_SND_AY8910_ROUTE_3] = 1.00;
	AY8910RouteDirs[(chip * 3) + BURN_SND_AY8910_ROUTE_1] = BURN_SND_ROUTE_BOTH;
	AY8910RouteDirs[(chip * 3) + BURN_SND_AY8910_ROUTE_2] = BURN_SND_ROUTE_BOTH;
	AY8910RouteDirs[(chip * 3) + BURN_SND_AY8910_ROUTE_3] = BURN_SND_ROUTE_BOTH;

	AY8910Reset(chip);

	num++;

	return 0;
}

INT32 AY8910InitYM(INT32 chip, INT32 clock, INT32 sample_rate,
		read8_handler portAread, read8_handler portBread,
		write8_handler portAwrite, write8_handler portBwrite,
		void (*update_callback)(void))
{
	INT32 val = AY8910Init(ay8910_index_ym + chip, clock, sample_rate, portAread, portBread, portAwrite, portBwrite);

	AYStreamUpdate = update_callback;

	if (val == 0) {
		ym_num++;
	}

	ay8910_index_ym = num - ym_num;	
	
	return val;
}

// Useful for YM2203, etc games needing read/write ports
INT32 AY8910SetPorts(INT32 chip, read8_handler portAread, read8_handler portBread,
		write8_handler portAwrite, write8_handler portBwrite)
{
	struct AY8910 *PSG = &AYPSG[chip];
	PSG->PortAread = portAread;
	PSG->PortBread = portBread;
	PSG->PortAwrite = portAwrite;
	PSG->PortBwrite = portBwrite;

	return 0;
}

INT32 AY8910Scan(INT32 nAction, INT32* pnMin)
{
	struct BurnArea ba;
	INT32 i;
	
	if ((nAction & ACB_DRIVER_DATA) == 0)
		return 1;

	if (pnMin && *pnMin < 0x029496) {			// Return minimum compatible version
		*pnMin = 0x029496;
	}

	for (i = 0; i < num; i++) {
		char szName[16];

		sprintf(szName, "AY8910 #%d", i);

		ba.Data		= &AYPSG[i];
		ba.nLen		= sizeof(struct AY8910);
		ba.nAddress = 0;
		ba.szName	= szName;
		BurnAcb(&ba);
	}

	return 0;
}

#define AY8910_ADD_SOUND(route, output)												\
	if ((AY8910RouteDirs[route] & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT) {	\
		nLeftSample += (INT32)(output[n] * AY8910Volumes[route]);						\
	}																				\
	if ((AY8910RouteDirs[route] & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT) {	\
		nRightSample += (INT32)(output[n] * AY8910Volumes[route]);					\
	}

void AY8910Render(INT16** buffer, INT16* dest, INT32 length, INT32 bAddSignal)
{
	INT32 i;
	INT16 *buf0 = buffer[0];
	INT16 *buf1 = buffer[1];
	INT16 *buf2 = buffer[2];
	INT16 *buf3, *buf4, *buf5, *buf6, *buf7, *buf8, *buf9, *buf10, *buf11, *buf12, *buf13, *buf14, *buf15, *buf16, *buf17;
	INT32 n;
	
	for (i = 0; i < num; i++) {
		AY8910Update(i, buffer + (i * 3), length);
	}
	
	if (num >= 2) {
		buf3 = buffer[3];
		buf4 = buffer[4];
		buf5 = buffer[5];
	}
	if (num >= 3) {
		buf6 = buffer[6];
		buf7 = buffer[7];
		buf8 = buffer[8];
	}
	if (num >= 4) {
		buf9 = buffer[9];
		buf10 = buffer[10];
		buf11 = buffer[11];
	}
	if (num >= 5) {
		buf12 = buffer[12];
		buf13 = buffer[13];
		buf14 = buffer[14];
	}
	if (num >= 6) {
		buf15 = buffer[15];
		buf16 = buffer[16];
		buf17 = buffer[17];
	}
		
	for (n = 0; n < length; n++) {
		INT32 nLeftSample = 0, nRightSample = 0;
		
		AY8910_ADD_SOUND(BURN_SND_AY8910_ROUTE_1, buf0)
		AY8910_ADD_SOUND(BURN_SND_AY8910_ROUTE_2, buf1)
		AY8910_ADD_SOUND(BURN_SND_AY8910_ROUTE_3, buf2)
		
		if (num >= 2) {
			AY8910_ADD_SOUND(3 + BURN_SND_AY8910_ROUTE_1, buf3)
			AY8910_ADD_SOUND(3 + BURN_SND_AY8910_ROUTE_2, buf4)
			AY8910_ADD_SOUND(3 + BURN_SND_AY8910_ROUTE_3, buf5)
		}
		
		if (num >= 3) {
			AY8910_ADD_SOUND(6 + BURN_SND_AY8910_ROUTE_1, buf6)
			AY8910_ADD_SOUND(6 + BURN_SND_AY8910_ROUTE_2, buf7)
			AY8910_ADD_SOUND(6 + BURN_SND_AY8910_ROUTE_3, buf8)
		}
		
		if (num >= 4) {
			AY8910_ADD_SOUND(9 + BURN_SND_AY8910_ROUTE_1, buf9)
			AY8910_ADD_SOUND(9 + BURN_SND_AY8910_ROUTE_2, buf10)
			AY8910_ADD_SOUND(9 + BURN_SND_AY8910_ROUTE_3, buf11)
		}
		
		if (num >= 5) {
			AY8910_ADD_SOUND(12 + BURN_SND_AY8910_ROUTE_1, buf12)
			AY8910_ADD_SOUND(12 + BURN_SND_AY8910_ROUTE_2, buf13)
			AY8910_ADD_SOUND(12 + BURN_SND_AY8910_ROUTE_3, buf14)
		}
		
		if (num >= 6) {
			AY8910_ADD_SOUND(15 + BURN_SND_AY8910_ROUTE_1, buf15)
			AY8910_ADD_SOUND(15 + BURN_SND_AY8910_ROUTE_2, buf16)
			AY8910_ADD_SOUND(15 + BURN_SND_AY8910_ROUTE_3, buf17)
		}
		
		nLeftSample = BURN_SND_CLIP(nLeftSample);
		nRightSample = BURN_SND_CLIP(nRightSample);
			
		if (bAddSignal) {
			dest[(n << 1) + 0] += nLeftSample;
			dest[(n << 1) + 1] += nRightSample;
		} else {
			dest[(n << 1) + 0] = nLeftSample;
			dest[(n << 1) + 1] = nRightSample;
		}
	}
}

void AY8910SetRoute(INT32 chip, INT32 nIndex, double nVolume, INT32 nRouteDir)
{
	AY8910Volumes[(chip * 3) + nIndex] = nVolume;
	AY8910RouteDirs[(chip * 3) + nIndex] = nRouteDir;
}
