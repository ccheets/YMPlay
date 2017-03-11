#ifndef _YMSONG_H_
#define _YMSONG_H_

#include "ymframe.h"
#include "ymddrum.h"
#include "inttypes.h"

class YMSong {

private:

	// My own version of ntohl, and ntohs.
	ULONG32 ntohl(ULONG32 n);
	USHORT16 ntohs(USHORT16 n);

protected:
	
	char		id[4];
	char		chk_string[8];
	char		*name;
	char		*author;
	char		*comment;

	ULONG32		n_frames;
	USHORT16	n_ddsamples;
	ULONG32		attributes;
	ULONG32		mst_clk;
	ULONG32		loop_frame;
	USHORT16	play_freq;
	
	// Data	
	YMDDrum		*ddrum;
	YMFrame		*frame;
	
	void Init();

public:
	
	YMSong(char *load_filename);
	YMSong();
	~YMSong();


	int	load(char *load_filename);
	int load(unsigned char *memfilePtr, unsigned long fileSize);
/*
//    std::string info();
	string	get_name() { return name; };
	string	get_author() { return author; };
	string	get_comment() { return comment; };
*/
	ULONG32	get_nframes() { return n_frames; };
	USHORT16	get_freq() { return play_freq; };

	YMFrame	get_frame(ULONG32 l) { return frame[l]; };
};

#endif
