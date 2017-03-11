#include <iostream>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include "ymsong.h"

YMSong::YMSong()
{
	ddrum = 0;
	frame = 0;
	filename = name = author = comment = id = chk_string = "";
	n_frames = 0;
	n_ddsamples = 0;
	attributes = 0;
	mst_clk = 2000000;
	loop_frame = 0;
	play_freq = 50;
}

YMSong::YMSong(char *load_filename)
{
	load(load_filename);
}

YMSong::~YMSong()
{
	from.close();
	delete 	ddrum;
	delete	frame; 
}

void YMSong::info()
{
	cout << "Info: " << name << ", " << author << ", " << comment << endl;
	cout << "Frames: " << n_frames << ", Master Clock: " << mst_clk << endl;
}

int YMSong::load(char *load_filename)
{
	int	i;
	char	c;
	
	// Open the file
	from.open(load_filename);

	// If it couldn't open.
	if(!from) {
		cerr << "Cannot open YM music file \'"
		     << load_filename << "\'" << endl;
		return -1;
	}

	// Save the name
	filename = load_filename;

	// Read in the file.
	//	ID
	for(i=0;i<4;i++)
		id += from.get();

	cerr << "ID = " << id << endl;

	// Check to see if format is supported.
	if(id != "YM2!" && id != "YM3!" && id != "YM4!" && id != "YM5!" && id != "YM6!") {
		cerr << "File format " << id << " not supported yet!" << endl;
		return -2;
	}
	
	if(id == "YM5!" || id == "YM6!" || id == "YM4!") {
		//	Check String
		for(i=0;i<8;i++)
			chk_string += from.get();
	
		cerr << "chk_string = " << chk_string << endl;
		
		if(chk_string != "LeOnArD!") {
			cerr << "Check string is wrong for file \""
			     << filename << "." << endl;
			return -1;
		}
		
		// 	Number of frames
		from.read((char*)&n_frames, sizeof(n_frames));	
		n_frames = ntohl(n_frames);
		cerr << "number of frames: " << n_frames << endl;
	
		// 	Song Attributes
		from.read((char*)&attributes, sizeof(attributes));
		attributes = ntohl(attributes);
	
		//	Number of dd samples
		from.read((char*)&n_ddsamples, sizeof(n_ddsamples));
		n_ddsamples = ntohs(n_ddsamples);
	
		cerr << "ddsamples = " << n_ddsamples << endl;
		
		if(id == "YM4!") {
			//	Loop frame
			from.read((char*)&loop_frame, sizeof(loop_frame));
			loop_frame = ntohl(loop_frame);		
		}
	}
	
	u_short	skipsize;
	
	skipsize = 0;
	
	if(id == "YM6!" || id == "YM5!") {
		// 	Master clock speed
		from.read((char*)&mst_clk, sizeof(mst_clk));
		mst_clk = ntohl(mst_clk);
		
		cerr << "Master clock = " << mst_clk << endl;
		
		//	Play frequency
		from.read((char*)&play_freq, sizeof(play_freq));
		play_freq = ntohs(play_freq);
		
		cerr << "Play freq = " << play_freq << endl;
		
		//	Loop frame
		from.read((char*)&loop_frame, sizeof(loop_frame));
		loop_frame = ntohl(loop_frame);
	
		cerr << "loop_frame = " << loop_frame << endl;	

		//	Bytes to skip from header.	
		from.read((char*)&skipsize, sizeof(skipsize));
		skipsize = ntohs(skipsize);
	}
	
	cerr << "skipsize = " << skipsize << endl;
	
	// 	Skip reserved bytes	
	if(skipsize)
		from.seekg(skipsize, ios::cur);

	//	Allocate dd array
	if(n_ddsamples) 
		ddrum = new YMDDrum[n_ddsamples];
	else	
		ddrum = 0;

	//	Read in digidrum samples
	for(i=0;i<n_ddsamples;i++) {

		from.read((char*)&ddrum[i].size, sizeof(u_long));
		ddrum[i].size = ntohl(ddrum[i].size);
	
		if(ddrum[i].size) {
			ddrum[i].data = new char[ddrum[i].size];
			from.read(ddrum[i].data, ddrum[i].size);
		}

	}	
	
	if(id == "YM6!" || id == "YM5!" || id == "YM4!") {

		// 	Name
        	while(c = from.get())
			name += c;
			
		//	Author
		while(c = from.get())
			author += c;
		//	Comment
		while(c = from.get())
                	comment += c;

		info();
	}

	cerr << "Pos before frames: " << from.tellg() << endl;
/*	if(from.tellg() & 1)
		from.seekg(1, ios::cur);
*/		
	cerr << "Pos after frames: " << from.tellg() << endl;

	// 	Frames
	if(n_frames) {
		u_long	i;
		u_long  n;
		frame = new YMFrame[n_frames];

		if(id == "YM2!" || id == "YM3!") {
			
			for(i=0;i<n_frames;i++)
				for(n=0;n<16;n++)
					frame[i].reg[n] = from.get();			
		} else {
			for(n=0;n<16;n++)
				for(i=0;i<n_frames;i++) 
					frame[i].reg[n] = from.get(); 
		}
		
	} else 
		frame = 0;
	
	// Close the file.
	from.close();
	return 0;
}
