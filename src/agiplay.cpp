/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 * AGI player for i386 Linux and OSS
 * Written by Claudio Matsuoka <claudio@helllabs.org>
 * Sun Mar 21 13:27:35 EST 1999
 *
 * Based on the format of the AGI SOUND resource as described by
 * Lance Ewing <lance.e@ihug.co.nz>
 *
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifndef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <q3progressdialog.h>
#endif

#include "game.h"
#include "menu.h"


#ifdef __linux__
#include <sys/soundcard.h>

#define WAVEFORM_SIZE 64
#define ENV_DECAY 800
#define ENV_SUSTAIN 160

struct agi_note {
  unsigned char dur_lo;
  unsigned char dur_hi;
  unsigned char frq_0;
  unsigned char frq_1;
  unsigned char vol;
};

struct channel_info {
  struct agi_note *ptr;
  int end;
  int freq;
  int phase;
  int vol;
  int env;
  int timer;
};


static int audio_fd;
static short *buffer;
static struct channel_info chn[4];

static int waveform[WAVEFORM_SIZE] = {
     0,   8,  16,  24,  32,  40,  48,  56,
    64,  72,  80,  88,  96, 104, 112, 120,
   128, 136, 144, 152, 160, 168, 176, 184,
   192, 200, 208, 216, 224, 232, 240, 255,
     0,-248,-240,-232,-224,-216,-208,-200,
  -192,-184,-176,-168,-160,-152,-144,-136,
  -128,-120,-112,-104, -96, -88, -80, -72,
  -64, -56, -48, -40,  -32, -24, -16,  -8   	/* Ramp up */
};



int init_sound ()
{
  int i;

  /* Set sound device to 16 bit, 22 kHz mono */

  if ((audio_fd = open ("/dev/audio", O_WRONLY)) == -1)
    return -1;

  i = AFMT_S16_NE;	/* Native endian */
  ioctl (audio_fd, SNDCTL_DSP_SETFMT, &i);
  i = 0;
  ioctl (audio_fd, SNDCTL_DSP_STEREO, &i);
  i = 22000;
  ioctl (audio_fd, SNDCTL_DSP_SPEED, &i);

  buffer = (short *)calloc (2, 2048);

  return 0;
}


void close_sound ()
{
  free (buffer);
  close (audio_fd);
}


void mix_channels (int s)
{
  register int i, p;
  int c, b, m;

  /* Size is in 16-bit samples */
  memset (buffer, 0, s << 1);

  /* Now build the sound for each channel */
  for (c = 0; c < 3; c++) {
    if (!chn[c].vol)
      continue;

    p = chn[c].phase;
    m = chn[c].vol * chn[c].env >> 16;

    /* Build a sound buffer. The format is signed 16 bit mono,
     * native-endian samples */
    for (i = 0; i < s; i++) {

      /* Interpolate to get a sample from the waveform */
      b = waveform[p >> 8] + (((waveform[((p >> 8) + 1) % WAVEFORM_SIZE] -
             waveform[p >> 8]) * (p & 0xff)) >> 8);

      /* Add to the buffer, processing the envelope */
      buffer[i] += (b * m) >> 3;

      p += 11860 * 4 / chn[c].freq;
      p %= WAVEFORM_SIZE << 8;
    }

    /* Update the channel phase */
    chn[c].phase = p;

    /* Update the envelope */
    if (chn[c].env > chn[c].vol * ENV_SUSTAIN)
        chn[c].env -= ENV_DECAY;

  }
}


void dump_buffer (int i)
{
  for (i <<= 1; i -= write (audio_fd, buffer, i); ) {}
}


void stop_note (int i)
{
  chn[i].vol = 0;
}


void play_note (int i, int freq, int vol)
{
  chn[i].freq = freq;
  chn[i].phase = 0;
  chn[i].vol = vol;
  chn[i].env = 0x10000;
}


void play_song (unsigned char *song, int size)
{
  int i, playing;

  /* Initialize channel pointers */
  for (i = 0; i < 4; i++) {
    chn[i].ptr = (struct agi_note *)(song + (song[i << 1] | (song[(i << 1) + 1] << 8)));
    chn[i].timer = 0;
    chn[i].end = 0;
  }

  int step=0;
  Q3ProgressDialog progress( "Playing...", "Cancel", (size+16)/5,0, 0, TRUE );
  progress.setMinimumDuration(0);

  for (playing = 1; playing; ) {
    int freq;

    for (playing = i = 0; i < 4; i++) {
      playing |= !chn[i].end;

      if (chn[i].end)
        continue;

      if ((--chn[i].timer) <= 0) {
        if (chn[i].ptr >= (struct agi_note *)song + size)
          break;
        stop_note (i);
        freq = ((chn[i].ptr->frq_0 & 0x3f) << 4)
          | (int)(chn[i].ptr->frq_1 & 0x0f);
        if (freq) {
          unsigned char v = chn[i].ptr->vol & 0x0f;
          play_note (i, freq, v == 0xf ? 0 : 0xff - (v << 1));
        }
        chn[i].timer = ((int)chn[i].ptr->dur_hi << 8) |
          chn[i].ptr->dur_lo;
        if (chn[i].timer == 0xffff) {
          chn[i].end = 1;
          chn[i].vol = 0;
        }
        chn[i].ptr++;

        progress.setProgress( step++ );
        if ( progress.wasCancelled() ){
          playing=false;
          break;
        }
      }
    }

    mix_channels (400);
    dump_buffer (400);
  }

}
#endif

void play_sound(int ResNum)
{


#ifdef __linux__
  if (init_sound () != 0) {
    menu->errmes("Can't initialize sound !");
    return;
  }

  int err = game->ReadResource(SOUND,ResNum);
  if(err)return;

  play_song(ResourceData.Data,ResourceData.Size);

  close_sound ();
#else
  menu->errmes("Sound currently works only under Linux !");
  return;
#endif

}

void play_sound(char *filename)
{

#ifdef __linux__
  if (init_sound () != 0) {
    menu->errmes("Can't initialize sound !");
    return;
  }

  FILE *fptr=fopen(filename,"rb");
  if(fptr==NULL){
    menu->errmes("Can't open file %s !",filename);
    return;
  }
  struct stat buf;
  fstat(fileno(fptr),&buf);
  ResourceData.Size=buf.st_size;
  fread(ResourceData.Data,ResourceData.Size,1,fptr);
  fclose(fptr);

  play_song(ResourceData.Data,ResourceData.Size);

  close_sound ();
#else
  menu->errmes("Sound currently works only under Linux !");
  return;
#endif


}
