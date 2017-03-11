#ifndef _YMDIRECTSOUNDPLAYER_H_
#define _YMDIRECTSOUNDPLAYER_H_

// YM Emulator includes.
#include "ymplay/ymsong.h"
#include "ymplay/ymemulator.h"

#include <dsound.h>

// YMDirectSoundPlayer.h: interface for the YMDirectSoundPlayer class.
//
//////////////////////////////////////////////////////////////////////
class YMDirectSoundPlayer  
{

protected:

	// Initialise the class.
	int Initialise();


	HANDLE mThread;
	HANDLE testEvents[3];

	YMSong	*mSong;
	YMEmulator mYmEmu;

	// Create buffers.
	WAVEFORMATEX wfx;
	DSBUFFERDESC dsbdesc;

	// DX objects
	LPDIRECTSOUND	mDsSound;

	// Define primary and secondary buffers.
	LPDIRECTSOUNDBUFFER mPlayBuff;

	unsigned int mFramesPerBuffer;
	unsigned int mFramePos;
	unsigned int mFrameFreq;

	bool mIsPlaying;
	bool mIsInitialised;


	HWND mParent_hWnd;

	// Thread start code.
	static int _beginThread(void *pThis);

public:

	bool IsInitialised();
	bool IsPlaying();
	int Stop();
	int Play();
	int Load(char *filename);
	int Load(unsigned char *fileBuff, unsigned long fileSize);
	virtual void RenderPlay();

	YMDirectSoundPlayer(HWND parent_hWnd);
	virtual ~YMDirectSoundPlayer();

};

#endif // _YMDIRECTSOUNDPLAYER_H_
