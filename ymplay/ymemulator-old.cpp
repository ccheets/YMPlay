/* YMEmulator class copyright Craig Cheetham 2002 (craig_cheetham@yahoo.co.uk) 
 * All Rights Reserved
 *
 * Description:
 *
 *	Emulates the imfamous soundchip Yamaha YM2149 used with Atari ST,
 * Sinclair Spectrum(and clones), Amstrad CPC 464 (and others), and 
 * Nintendo Gameboy.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream.h>
#include "inttypes.h"
#include "ymemulator.h"

#define PI 3.1415926535897f
#define K 655350 
//#define PI 3.1416

YMVoice::YMVoice()
{
	reset();
}

void YMVoice::reset()
{
        vol = amp = 0;
        freq = 0;
        mute = false;
        t = 0;
	ddrum = 0;
}	

void YMEmulator::poke(unsigned char *new_regs)
{
	int	v;
	int	ddchan;

        // For each voice
        for(v=0;v<3;v++) {

		// Get volume
		if(voice[v].mute)
			voice[v].vol = 0;
		else

			if(new_regs[8+v] & (1<<5))
				voice[v].vol = new_regs[0x0e] & 0x1f;
			else
        	        	voice[v].vol = new_regs[8+v] & 0x0f;

		// Get frequency
       	voice[v].freq = (ULONG32)16*(0xff*(new_regs[v+v+1] & 0x0f) + new_regs[v+v]);

		if(voice[v].freq)
			voice[v].freq = mst_clk/voice[v].freq;
		
		voice[v].ddrum = 0;
        }
	
	// Sort out DDrum actions.
	if(ddchan = (new_regs[3] & 0x30)/0x10) {
		--ddchan;
		voice[ddchan].ddrum = (new_regs[8+ddchan] & 0x1f) + 1;
		voice[ddchan].vol = 32;
	}
	
	// Noise
	noise_freq = (16*(new_regs[6] & 0x1f));
	if(noise_freq) {
		noise_freq = mst_clk / noise_freq;
		noise_t = 0;
	}
		
}

void YMEmulator::reset()
{	
	int v;

	// Reset
	mst_clk = 2000000;
	fps = 50;
	noise_t = 0;
	noise_freq = 0;

	for(v=0;v<3;v++)
		voice[v].reset();
	
}

int YMEmulator::generate_frame(unsigned char *buffer, 
			       int length,
			       int speed,
			       unsigned char *regs)
{
	int	v, i;
	double  n;

	// update the class with reg array
	poke(regs);

	// For each sample
	for(i=0;i<length;i++) { 

		// For each channel
		for(v=0;v<3;v++) {

			// If volume > 1 and the channel isn't muted.
			if((voice[v].freq > 0) && (voice[v].freq < speed)) {

				if(voice[v].vol && ~voice[v].mute) {
					
					if(voice[v].ddrum) {
					  voice[v].amp = 0;
					} else {
					  if(voice[v].t >= K/2)
					    voice[v].amp = (char)voice[v].vol;
					  else	
					    voice[v].amp = 0-voice[v].vol;
					} 
						 
				} else
					voice[v].amp = 0;
			
				// note the period passed
				voice[v].t += K*voice[v].freq/speed;

				while(voice[v].t >= K)
					voice[v].t -= K; 
				
			} else
				voice[v].amp = 0;
		}

/*
		if(noise_freq) {
			noise_t += (double)noise_freq/speed;

			if(noise_t >= 1) {
				noise_amp = (rand()%64) - 32;
				noise_t = 0;
			}
			/*
			cerr << "Noise T = " << noise_t << ", noise_amp = " <<
			noise_amp << "\n";
			*//*
		} else {
			noise_t = 0;
		} 
*/
/*
		if(noise_amp)
			buffer[i] = (unsigned char)((USHORT16)noise_amp + (USHORT16)voice[0].amp  + (USHORT16)voice[1].amp + (USHORT16)voice[2].amp)/4;

		else
*/
			buffer[i] = (unsigned char)((USHORT16)voice[0].amp + (USHORT16)voice[1].amp + (USHORT16)voice[2].amp)/3;
//		buffer[i] = (unsigned char)((((USHORT16)voice[0].amp)+((USHORT16)voice[1].amp)+((USHORT16)voice[2].amp))/3);

		// Mix channels
		
//		buffer[i] = (buffer[i]*128/32) + 128;
	}

	return 0;	
}
