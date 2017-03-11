#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "ymsong.h"
#include "ymemulator.h"
#define PI 3.14216f

int audio_fd;
int speed;
int channels;
int     format;

int sound_init(char *device)
{

        channels = 1;
        speed = 44100;
        format = AFMT_U8;

        if((audio_fd = open(device, O_WRONLY, 0)) == -1) {
                perror(device);
                exit(1);
        }

        if(ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1) {
                perror("SNDCTL_DSP_SETFMT");
                exit(1);
        }

        if(format != AFMT_U8) {
                fprintf(stderr, "Format couldn't be set.\n");
                exit(1);
        }

        if(ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed) == -1) {
                perror("SNDCTL_DSP_SPEED");
                exit(1);
        }

        if(speed != 22050) {
                fprintf(stderr, "Couldn't set sample rate.\n");
        }

        fprintf(stdout, "Sample rate is %d\n", speed);
        if(ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
                perror("SNDCTL_DSP_CHANNELS");
                exit(1);
        }

        if(channels != 1) {
                fprintf(stderr, "Not ready for stereo.\n");
                exit(1);
        }
}

int main(int argc, char **argv)
{
	YMSong		mySong(argv[1]);
	YMFrame		myFrame;
	YMEmulator	myYM;
	int		i, size;
	unsigned char	*buffer;
	FILE		*fh;

	sound_init("/dev/dsp");
//	fh = fopen("debug.raw", "wb");
	size = speed / mySong.get_freq();
	buffer = (unsigned char*)malloc(size);

	// For each frame	
	for(i=0;i<mySong.get_nframes();i++) {
		myFrame = mySong.get_frame(i);	
		myYM.generate_frame(buffer, size, speed, myFrame.reg);
//		fwrite(buffer, size, 1, fh); // write to file
		write(audio_fd, buffer, size);
	}
//	fclose(fh);
	close(audio_fd);

	return 0;
}
