#ifndef _YMEMULATOR_H_
#define _YMEMULATOR_H_

#include "inttypes.h"

class YMVoice {
public:

	UBYTE		vol;
	bool		useEnvVol;
	BYTE		amp;
	USHORT16	period;
	ULONG32		ticks;
	
	bool		mute;
	USHORT16	ddrum;


	// Mixer flags.
	bool		mixnoise;
	bool		mixtone;

	YMVoice();
	void		reset();
};

class YMEmulator {

private:
	ULONG32	mst_clk;
	USHORT16 fps;	
	YMVoice	voice[3];
	UBYTE		envelope_shape;
	ULONG32		noise_period;
	ULONG32		noise_ticks;
	//bool        noise_sign;
	USHORT16	envelope_freq;
public:	

	bool		NoiseOn;

	YMEmulator();
	~YMEmulator();

	void reset();

	void poke(unsigned char *new_regs);

	int generate_frame(unsigned char *buffer,
			   ULONG32 length, USHORT16 speed, unsigned char *regs);

	// Get/set messages
	void set_mst_clk(ULONG32 upd_mst_clk) { mst_clk = upd_mst_clk; };
	ULONG32 get_mst_clk() { return mst_clk; };
	
};

#endif // _YMEMULATOR_H_
