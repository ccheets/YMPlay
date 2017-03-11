/* YMEmulator class copyright Craig Cheetham 2002 (craig_cheetham@yahoo.co.uk) 
 * All Rights Reserved
 *
 * Description:
 *
 *	Emulates the imfamous soundchip Yamaha YM2149 used with Atari ST,
 * Sinclair Spectrum(and clones), Amstrad CPC 464 (and others).
 */
 
//#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ymemulator.h"
#include "inttypes.h"

//using namespace std;

#define K ((ULONG32)4294967295)
#define VOLATTNK 1.047387696f
#define YM_OUTPUTFREQ 44100

YMVoice::YMVoice()
{
	reset();
}

void YMVoice::reset()
{
    vol = amp = 0;
    period = 0;
    mute = false;
    ticks = 0;
	ddrum = 0;
}	

YMEmulator::YMEmulator()
{
	reset();
}

YMEmulator::~YMEmulator()
{

}

void YMEmulator::poke(unsigned char *new_regs)
{
	int	v;
	int	ddchan;

    // For each voice
    for(v=0;v<3;v++)
	{
		// Get frequency
		voice[v].period = (((USHORT16)(new_regs[v+v+1] & 0x0f) << 8) | new_regs[v+v]) << 4;

		// If the volume should be taken from
		// envelope, then use the specified value
		// in the regs.
		if(new_regs[8+v] & (1 << 4))
		{
			voice[v].vol = 0;
			voice[v].useEnvVol = true;
		}
		else
		{
			voice[v].vol = new_regs[8+v] & 0x0f;
			voice[v].useEnvVol = false;
		}

		// Extract mixer values.
		voice[v].mixtone = (new_regs[7] & (1 << v)) == 0;
		voice[v].mixnoise = (new_regs[7] & (1 << (v + 3))) == 0;

		// TODO: DDRUM
		voice[v].ddrum = 0;
    }
	
	// Get the envelope frequency.
	envelope_freq = (USHORT16)new_regs[12] << 8 | new_regs[11];

	// Sort out DDrum actions.
	//if(ddchan = (new_regs[3] & 0x30)/0x10) {
	//	--ddchan;
	//	voice[ddchan].ddrum = (new_regs[8+ddchan] & 0x1f) + 1;
	//	voice[ddchan].vol = 32;
	//}

	// Break down noise registers.
	noise_period = (ULONG32)(new_regs[6] & 0x1f) << 4;
	if(!noise_period)
	{
		noise_ticks = 0;
	}

	// Get envelope shape.
	envelope_shape = new_regs[13] & 0x0f;
}

void YMEmulator::reset()
{	
	int v;

	// Reset
	mst_clk = 2000000;
	fps = 50;
	noise_ticks = 0;
	noise_period = 0;
	NoiseOn = true;
	envelope_freq = 0;
	envelope_shape = 0;
	for(v=0;v<3;v++)
		voice[v].reset();
}

int YMEmulator::generate_frame(unsigned char *buffer,
			       ULONG32 length, USHORT16 speed,
			       unsigned char *regs)
{
	// update the class with reg array
	poke(regs);

	// Produce samples for the entire length of the buffer.
	USHORT16 sample_ticks = mst_clk / speed;

	for(int sample = 0; sample < length; sample++)
	{
		SHORT16 sample_vol = 0;
		ULONG32 skip_ticks = 0;
		for(int tick = 0; tick < sample_ticks; tick += skip_ticks)
		{
			SHORT16 tick_vol = 0;
			skip_ticks = -1;

			// Process each voice.
			for(int v = 0; v < 3; v++)
			{
				// If we are past half period, invert.
				if(voice[v].period)
				{
					voice[v].amp = 1;
					if(voice[v].ticks << 1 >= voice[v].period)
					{
						voice[v].amp = -1;
					}

					ULONG32 next_change = (voice[v].period >> 1) - voice[v].ticks;

					if(next_change < skip_ticks)
					{
						skip_ticks = next_change;
					}
				}
				else
				{
					voice[v].ticks = 0;
				}

				// Do noise.
				BYTE noise_amp = 0;
				if(noise_period)
				{
					//noise_amp = 1;
					//if(noise_ticks << 1 >= noise_period)
					//{
					//	noise_amp = -1;
					//}

					//ULONG32 next_change = (noise_period >> 1) - noise_ticks;
					//if(next_change < skip_ticks)
					//{
					//	skip_ticks = next_change;
					//}

					//noise_amp *= rand() % 16;
					//tick_vol += noise_amp;
				}
				else
				{
					noise_ticks = 0;
				}
			}



			// Do envelopes.


			// If the amount we are skipping is greater than 
			// the amount left in this sample, then skip to the end.
			if(skip_ticks > (sample_ticks - tick))
			{
				skip_ticks = (sample_ticks - tick);
			}

			if(skip_ticks == 0)
			{
				skip_ticks = 1;
			}

			// Skip ticks and mix for each channel & noise
			if(noise_period)
			{
				noise_ticks = (noise_ticks + skip_ticks) % noise_period;
			}

			for(int v = 0; v < 3; v++)
			{
				// Skip ticks.
				if(voice[v].period != 0)
				{
					voice[v].ticks = (voice[v].ticks + skip_ticks) % voice[v].period;
				}
				else
				{
					voice[v].ticks = 0;
				}
				// Mix.
				tick_vol += voice[v].amp * voice[v].vol;
			}

			// Add current tick volume to sample
			sample_vol += tick_vol * skip_ticks;
		}

		// Average the sample.
		buffer[sample] = (UBYTE)((sample_vol / (sample_ticks * 4)) + 127);
	}

	//USHORT16    output_ratio = speed / YM_OUTPUTFREQ;
	//ULONG32     ym_samples = length * output_ratio;
	//ULONG32		ym_toplay = length / output_ratio;

	//// For each YM sample.
	//for(int i = 0; i <  ym_toplay; i++)
 //   {     
 //       // Do voices
 //       for(int v = 0; v < 3; v++)
 //       {
	//		voice[v].amp = 0;
	//		if(voice[v].freq && voice[v].freq < speed)
	//		{
	//			ULONG32 l = (YM_OUTPUTFREQ / voice[v].freq);

	//			//  Calc amplitude
	//			if(!voice[v].mute && voice[v].vol)
	//				voice[v].amp = (BYTE) (((double)pow((double)VOLATTNK, voice[v].vol - 1)) * 16);	 // CHECK THIS

	//			// Invert sign if negative part of wave.
	//			if(voice[v].t >= l/2)
	//			{
	//				voice[v].amp = 0-voice[v].amp;
	//			}

	//			voice[v].t += 1;
	//			voice[v].t %= l;
 //           }
	//		else
	//		{
	//			//voice[v].t = 0;  
	//		}
	//	}
 //      

	//	// Do noise
	//	SHORT16 noise_amp = 0;
	//	if(noise_freq && noise_freq < YM_OUTPUTFREQ)
	//	{
	//		ULONG32 l = (YM_OUTPUTFREQ / noise_freq);
	//		BYTE r = rand() % 32;
 //			noise_amp = (BYTE) ((double)pow((double)VOLATTNK, r - 1) * 16);  
	//		
	//		if(!(noise_t >= l/2))
	//		{	
	//			noise_amp *= -1;
	//		}

	//		// Advance timing.
	//		noise_t += 1;
	//		noise_t %= l;
	//
	//	}


 //       // Mix
 //       SHORT16 amp = 0;             // Sample amplitude
 //       for(int v = 0; v < 3; v++)
 //       {
	//		if(voice[v].mixtone)
	//			amp += voice[v].amp;

	//		if(voice[v].mixnoise)
	//			amp += noise_amp;				
 //       }      

	//	amp *= (127/32)/3;

 //       // Scale
 //       for(int s = 0; s < output_ratio; s++)
 //       {
 //          buffer[s+(i*output_ratio)] = (UBYTE)((SHORT16)amp + 127);
 //       }

//    }

	return 0;	
}
