// YMDirectSoundPlayer.cpp: implementation of the YMDirectSoundPlayer class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
// DS Includes
#include <mmsystem.h>
#include "YMDirectSoundPlayer.h"

struct threadOOpWorkAround 
{
	YMDirectSoundPlayer *ymdsp;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YMDirectSoundPlayer::YMDirectSoundPlayer(HWND parent_hWnd)
{
	// Save the window handle internally.
	mParent_hWnd = parent_hWnd;
	mThread = NULL;
	mPlayBuff = NULL;

	mIsInitialised = false;
	mIsPlaying = false;

	// Clear the song as it does not exist.
	mSong = NULL;

	// Clear direct sound object pointer.
	mDsSound = NULL;

	// Setup directX object.
	bool dsSetupError = false;
 
	if(DirectSoundCreate(NULL, &mDsSound, NULL) == DS_OK)
	{
		mDsSound->SetCooperativeLevel(mParent_hWnd, DSSCL_PRIORITY);
	}
	else
	{
		return;
	}

	// Setup wave spec.
	memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
	wfx.wFormatTag = WAVE_FORMAT_PCM; 
	wfx.nChannels = 1; 
	wfx.nSamplesPerSec = 44100; 
	wfx.nBlockAlign = 1; 
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign; 
	wfx.wBitsPerSample = 8;
	
	// We have now been initialised
	mIsInitialised = true;

	// Create the event.
	testEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	testEvents[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	testEvents[2] = CreateEvent(NULL, FALSE, FALSE, NULL);

}

YMDirectSoundPlayer::~YMDirectSoundPlayer()
{
	// Stop the music.
	if(mIsPlaying)
	{
		Stop();
	}

	// Kill the buffers.
	if(mPlayBuff != NULL)
	{
		IDirectSound_Release(mPlayBuff);
	}

	// Free resources.
	if(mDsSound)
	{
		// Kill direct sound object.
		IDirectSound_Release(mDsSound);
	}

	if(mSong != NULL)
	{
		delete mSong;
		mSong = NULL;
	}
}

int YMDirectSoundPlayer::Load(unsigned char *fileBuff, unsigned long fileSize)
{
	// The result of the function.
	int result = 0;

	// If playing then stop.
	if(mIsPlaying)
	{
		Stop();
	}

	// Reset poitions.
	mFramePos = 0;
	mFramesPerBuffer = 60;

	// If song exists then kill it.
	if(mSong != NULL)
	{
		delete mSong;
	}

	// Create song object.
	mSong = new YMSong();

	// Read whole file into memory.
	result = mSong->load(fileBuff, fileSize);

	if(result == 0)
	{
		mFrameFreq = mSong->get_freq();

		// If buffers already exist then kill them.
		if(mPlayBuff != NULL)
		{
			mPlayBuff->Stop();
			IDirectSound_Release(mPlayBuff);
		}

		// Setup new buffers.

		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
		dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
		dsbdesc.dwFlags = DSBCAPS_CTRLFREQUENCY
			| DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 |
			DSBCAPS_STATIC; 


		dsbdesc.dwBufferBytes = wfx.nSamplesPerSec * mFramesPerBuffer / mFrameFreq;
		dsbdesc.lpwfxFormat = &wfx;

		HRESULT hr = mDsSound->CreateSoundBuffer(&dsbdesc, &mPlayBuff, NULL); 

		if(!SUCCEEDED(hr))
		{
			result = -2;
		}
		else
		{
			// Reset the emulator.
			mYmEmu.reset();
		}
	}
	else
	{
		// We failed!
		result = -3;
	}

	return result;
}

int YMDirectSoundPlayer::Load(char *filename)
{
	// The result of the function.
	int result = 0;

	// If playing then stop.
	if(mIsPlaying)
	{
		Stop();
	}

	// Reset poitions.
	mFramePos = 0;
	mFramesPerBuffer = 60;

	// If song exists then kill it.
	if(mSong != NULL)
	{
		delete mSong;
	}

	// Create song object.
	mSong = new YMSong();

	// Read whole file into memory.
	result = mSong->load(filename);

	//return 0;

	if(result == 0)
	{
		mFrameFreq = mSong->get_freq();

		// If buffers already exist then kill them.
		if(mPlayBuff != NULL)
		{
			mPlayBuff->Stop();
			IDirectSound_Release(mPlayBuff);
		}

		// Setup new buffers.

		memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
		dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
		dsbdesc.dwFlags = DSBCAPS_CTRLFREQUENCY
			| DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 |
			DSBCAPS_STATIC; 


		dsbdesc.dwBufferBytes = wfx.nSamplesPerSec * mFramesPerBuffer / mFrameFreq;
		dsbdesc.lpwfxFormat = &wfx;

		HRESULT hr = mDsSound->CreateSoundBuffer(&dsbdesc, &mPlayBuff, NULL); 

		if(!SUCCEEDED(hr))
		{
			result = -2;
		}
		else
		{
			// Reset the emulator.
			mYmEmu.reset();
		}
	}
	else
	{
		// We failed!
		result = -3;
	}

	return result;
}

int YMDirectSoundPlayer::Play()
{
	if(mIsPlaying)
	{
		Stop();
	}
	else
	{
		mIsPlaying = true;
		threadOOpWorkAround *wa = new threadOOpWorkAround;
		wa->ymdsp = this;
		mThread = CreateThread(NULL, 0, (unsigned long (__stdcall *)(void *))this->_beginThread,
			(void *)wa, 0, NULL);
	}

	// Return success.
	return 0;
}

void YMDirectSoundPlayer::RenderPlay()
{
	YMFrame frame;

	bool firstPlay = true;
	bool firstHalf = true;

	int nFramesPass = mFramesPerBuffer;
	int nBuffWinLength = dsbdesc.dwBufferBytes;
	int nFrameSize = wfx.nSamplesPerSec/mFrameFreq;

	// Create a render buffer
	unsigned char *renderBuff = new unsigned char[dsbdesc.dwBufferBytes];

	// Setup notifications to buffer positions, #1. At the end of the buffer
	// #2 and in the middle.
	LPDIRECTSOUNDNOTIFY lpDsNotify;
	DSBPOSITIONNOTIFY pPosNotify[2];
	pPosNotify[0].dwOffset = dsbdesc.dwBufferBytes - 1;
	pPosNotify[1].dwOffset = dsbdesc.dwBufferBytes/2 - 1;  
	pPosNotify[0].hEventNotify = testEvents[0];
	pPosNotify[1].hEventNotify = testEvents[1];
	mPlayBuff->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&lpDsNotify);
	lpDsNotify->SetNotificationPositions(2, pPosNotify);
	lpDsNotify->Release();


	while(mIsPlaying)
	{
		LPVOID	lpvWrite = NULL;
		DWORD	dwLength = 0;

		// For each frame generate wave output.
		for(int i=0;i<nFramesPass && (mFramePos < mSong->get_nframes());i++)
		{
			int frameIndex = mFramePos+i;

			// Render buffer.
			frame = mSong->get_frame(frameIndex);	
			mYmEmu.generate_frame(renderBuff+(i*nFrameSize),
				nFrameSize,
				wfx.nSamplesPerSec,
				frame.reg);
		}

		// Lock buffer selected.
		if(DS_OK == mPlayBuff->Lock(0,
			dsbdesc.dwBufferBytes,
			&lpvWrite, 
			&dwLength,
			NULL,
			NULL,
			DSBLOCK_ENTIREBUFFER))
		{
			unsigned char* absBuff = firstHalf ? (unsigned char*)lpvWrite: (unsigned char*)lpvWrite + nBuffWinLength;

			memcpy(absBuff, renderBuff, nBuffWinLength);

			// Unlock buffer.
			mPlayBuff->Unlock(lpvWrite,
				nBuffWinLength,
				NULL,
				NULL);
		}


		// If we went past the end, rewind.
		if(mFramePos >= mSong->get_nframes())
		{
			mFramePos = 0;
		}
		else
		{
			// Skip to next block of frames.
			mFramePos += nFramesPass;
		}


		// If we have started playing, i.e not the first play
		// then wait for DX to give us a signal.
		if(!firstPlay)
		{
			firstHalf = !firstHalf;
		}
		else
		{
			firstPlay = false;
			nFramesPass /= 2;
			nBuffWinLength /= 2;
			
			// Play the sample.
			mPlayBuff->SetCurrentPosition(0);
			mPlayBuff->Play(0, 0, DSBPLAY_LOOPING);
		}
		

		bool waitLock = true;
		while(waitLock)
		{
			HRESULT hr = WaitForMultipleObjects(3, testEvents, FALSE, INFINITE);

			switch(hr)
			{
				case WAIT_OBJECT_0:
					waitLock = !(firstPlay || (!firstHalf && !firstPlay));
					break;

				case WAIT_OBJECT_0 + 1:
					waitLock = false;
					break;

				// This is the exit event.
				case WAIT_OBJECT_0 + 2:
					waitLock = false;
					mIsPlaying = false;
					break;
			};
		}
	}

	delete renderBuff;
	mThread = NULL;
}

int YMDirectSoundPlayer::Stop()
{
	if(mIsPlaying)
	{
		// We have stopped playing now.
		mIsPlaying = false;

		// Trigger player to stop if playing.
		SetEvent(testEvents[2]);
		ResetEvent(testEvents[0]);
		ResetEvent(testEvents[0]);

		mPlayBuff->Stop();

		if(mThread != NULL)
		{
			// Wait for thread to finish.
			WaitForSingleObject(mThread, INFINITE);
			CloseHandle(mThread);
			mThread = NULL;
		}

		mFramePos = 0;

		// Reset the emulator.
		mYmEmu.reset();
	}

	return mIsPlaying == true ? 1 : 0;
}

bool YMDirectSoundPlayer::IsPlaying()
{
	return mIsPlaying;
}


int YMDirectSoundPlayer::_beginThread(void *pThis)
{
	threadOOpWorkAround *wa = (threadOOpWorkAround*)pThis;
	YMDirectSoundPlayer *ourObj = wa->ymdsp;
	delete wa;
	ourObj->RenderPlay();
	return 0;
}
