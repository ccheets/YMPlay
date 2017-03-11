#include <stdio.h>
#include <string.h>
#include "inttypes.h"
#include "ymsong.h"

//using namespace std;

YMSong::YMSong()
{
	ddrum = 0;
	frame = 0;

	name = author = comment = NULL;
	memset(chk_string, 1, sizeof(chk_string));
	memset(id, 1, sizeof(id));

	n_frames = 0;
	n_ddsamples = 0;
	attributes = 0;
	mst_clk = 0;
	loop_frame = 0;
	play_freq = 0;
}

YMSong::~YMSong()
{
	if(name)
	{
		delete name;
	}
	
	if(author)
	{
		delete comment;
	}

	if(ddrum)
	{
		delete[] ddrum;
	}

	if(frame)
	{
		delete[]	frame; 
	}
}

ULONG32 YMSong::ntohl(ULONG32 n)
{
	ULONG32 result = n;

#ifdef LITTLE_ENDIAN

	result = 0;

	result |= n >> 24;
	result |= (n & 0x00ff0000) >> 8;
	result |= (n & 0x0000ff00) << 8;
	result |= n << 24;

#endif

	return result;

}

USHORT16 YMSong::ntohs(USHORT16 n)
{
	USHORT16 result = n;

#ifdef LITTLE_ENDIAN

	result = 0;
	result |= n >> 8;
	result |= n << 8;

#endif

	return result;
}

/*
string YMSong::info()
{
	ostringstream oss;

	oss << "Info: " << name << ", " << author << ", " << comment << endl;
	oss << "Frames: " << n_frames << ", Master Clock: " << mst_clk << endl;
	return oss.str();
}
*/

int YMSong::load(unsigned char *memfilePtr, 
				 unsigned long fileSize)
{
	unsigned int filePos = 0;
	
	// Read in the file version.
	char id[4];
	memcpy(id, memfilePtr, 4);
	filePos += 4;

	// Check to see if format is supported, if not return
	// error code.
	int ymVer = id[2] - '0';
	if(!(id[0] == 'Y' &&
	   id[1] == 'M' &&
	   (ymVer >= 1 && ymVer <= 6) &&
	   id[3] == '!'))
	{
		return -2;
	}

	// If one of the old style versions,
	// then put in static values as they don't
	// have any of their own.
	if(ymVer == 2 || ymVer == 3)
	{
		mst_clk = 2000000;
		play_freq = 50;
		loop_frame = 0;
		n_ddsamples = 0;
		attributes = 1;

		n_frames = (fileSize - 4) / 14;

	} // If versions 4 to 6, check for check string.
	else if(ymVer >= 4 && ymVer <= 6)
	{
		char *chkString = "LeOnArD!";
		if(strncmp((const char*)memfilePtr+filePos, chkString, strlen(chkString)) != 0)
		{
			return -3;
		}
		filePos += 8;

		// Get the number of frames.
		n_frames = ntohl(*((ULONG32*)(memfilePtr+filePos)));
		filePos += 4;

		// Get attributes
		attributes = ntohl(*((ULONG32*)(memfilePtr+filePos)));
		filePos += 4;

		// Get number of dd samples
		n_ddsamples = ntohs(*((USHORT16*)(memfilePtr+filePos)));
		filePos += 2;

		// If version 4, then read in the loop frame.
		if(ymVer == 4)
		{
			loop_frame = ntohl(*((ULONG32*)(memfilePtr+filePos)));
			filePos += 4;
		}
	}

	USHORT16	skipsize = 0;

	if(ymVer >= 5 && ymVer <= 6)
	{
		// Get master clock setting.
		mst_clk = ntohl(*((ULONG32*)(memfilePtr+filePos)));
		filePos += 4;

		// Get play frequency, in VBL per second.
		play_freq = ntohs(*((USHORT16*)(memfilePtr+filePos)));
		filePos += 2;

		// Get loop frame
		loop_frame = ntohl(*((ULONG32*)(memfilePtr+filePos)));
		filePos += 4;

		// Get the skip size.
		skipsize = ntohs(*((USHORT16*)(memfilePtr+filePos)));
		filePos += 2;
	}

	// Seek past by offset of seek size.
	filePos += skipsize;

	//	Allocate dd array
	if(n_ddsamples) 
		ddrum = new YMDDrum[n_ddsamples];
	else	
		ddrum = 0;	

	// Load in DD samples.
	for(int i=0;i<n_ddsamples;i++)
	{
		ddrum[i].size = ntohl(*((ULONG32*)(memfilePtr+filePos)));
		filePos += 4;

		if(ddrum[i].size)
		{
			ddrum[i].data = new char[ddrum[i].size];
			memcpy(ddrum[i].data, (memfilePtr+filePos), ddrum[i].size);
			filePos += ddrum[i].size;
		}
	}

	// Read in the information string tags.
	if(ymVer >= 4 && ymVer <= 6)
	{
		// Name
		int len = strlen((const char*)memfilePtr+filePos);
		name = new char[len];
		filePos += len + 1;

		// Author
		len = strlen((const char*)memfilePtr+filePos);
		author = new char[len];
		filePos += len + 1;

		// Comment
		len = strlen((const char*)memfilePtr+filePos);
		comment = new char[len];
		filePos += len + 1;
	}

	// Load in the music frames.
	if(n_frames)
	{	 
		int	i;
		int n;
		frame = new YMFrame[n_frames];
 
		int frameLen = 16;

		if(ymVer == 2 || ymVer == 3)
		{
			frameLen = 14;
		}

		for(n=0;n<frameLen;n++)
		{
			for(i=0;i<n_frames;i++) 
			{
				frame[i].reg[n] = memfilePtr[filePos++]; 
			//	filePos++;
			}
		}
	}
	else 
	{
		frame = 0;
	}

	return 0;
}

int YMSong::load(char *fileName)
{
	if(fileName == NULL) 
	{
		return -1;
	}

	if(strlen(fileName) == 0)
	{
		return -2;
	}

	unsigned long fileSize = 0;
	unsigned char *fileBuff = NULL;

	FILE *fh = fopen(fileName, "rb");
	if(fh != 0)
	{
		fseek(fh, 0, SEEK_END);
		fileSize = ftell(fh);
		fseek(fh, 0, SEEK_SET);
		fileBuff = new unsigned char[fileSize];
		fread(fileBuff, 1, fileSize, fh);
		fclose(fh);	

		if(load(fileBuff, fileSize) != 0)
		{
			return -1;
		}

		delete fileBuff;
	}

	return 0;
}

