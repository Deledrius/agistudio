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


#include <QtMultimedia/QAudioOutput>
#include <QProgressDialog>
#include <QBuffer>
#include <QEventLoop>
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>

#include <sys/stat.h>

#include "menu.h"
#include "game.h"


#define NUM_CHANNELS    4
#define WAVEFORM_SIZE   64
#define ENV_DECAY       800
#define ENV_SUSTAIN     160

struct agi_note {
    uint8_t dur_lo;
    uint8_t dur_hi;
    uint8_t frq_0;
    uint8_t frq_1;
    uint8_t vol;
};

struct channel_info {
    struct agi_note *ptr;
    bool end;
    int32_t freq;
    int32_t phase;
    int32_t vol;
    int32_t env;
    int32_t timer;
};


static QAudioSink *audio_out;
static short *buffer;
static QBuffer *qbuffer;
static struct channel_info chn[4];

static int waveform[WAVEFORM_SIZE] = {
    0,   8,  16,  24,  32,  40,  48,  56,
    64,  72,  80,  88,  96, 104, 112, 120,
    128, 136, 144, 152, 160, 168, 176, 184,
    192, 200, 208, 216, 224, 232, 240, 255,
    0, -248, -240, -232, -224, -216, -208, -200,
    -192, -184, -176, -168, -160, -152, -144, -136,
    -128, -120, -112, -104, -96, -88, -80, -72,
    -64, -56, -48, -40,  -32, -24, -16,  -8       /* Ramp up */
};


int init_sound()
{
    /* Set sound device to 16 bit, 22 kHz mono */

    QAudioFormat format;
    format.setSampleRate(22000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (!info.isFormatSupported(format)) {
        menu->errmes("Cannot play audio", "Raw audio format not supported by backend.");
        return -1;
    }

    audio_out = new QAudioSink(format);
    qbuffer = new QBuffer();
    qbuffer->open(QIODevice::ReadWrite);

    buffer = (short*)calloc(2, 2048);

    return 0;
}


void close_sound()
{
    free(buffer);
    delete qbuffer;
    delete audio_out;
}


void mix_channels(uint16_t num_samples)
{
    int32_t c, b, m, i, p;

    /* Size is in 16-bit samples */
    memset(buffer, 0, num_samples << 1);

    /* Now build the sound for each channel */
    for (c = 0; c < (NUM_CHANNELS - 1); c++) {
        if (!chn[c].vol)
            continue;

        p = chn[c].phase;
        m = chn[c].vol * chn[c].env >> 16;

        /* Build a sound buffer. The format is signed 16 bit mono,
         * native-endian samples */
        for (i = 0; i < num_samples; i++) {

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


void dump_buffer(uint32_t i)
{
    qbuffer->write((const char *)buffer, i*2);
}


void stop_note(uint8_t c)
{
    chn[c].vol = 0;
}


void play_note(uint8_t c, int freq, int vol)
{
    chn[c].freq = freq;
    chn[c].phase = 0;
    chn[c].vol = vol;
    chn[c].env = 0x10000;
}

void play_song(unsigned char *song, int size)
{
    /* Initialize channel pointers */
    for (uint8_t c = 0; c < NUM_CHANNELS; c++) {
        chn[c].ptr = (struct agi_note *)(song + (song[c << 1] | (song[(c << 1) + 1] << 8)));
        chn[c].timer = 0;
        chn[c].end = false;
    }

    QProgressDialog progress("Playing...", "Cancel", 0, 100);
    progress.setMinimumDuration(0);
    progress.setModal(true);
    progress.setValue(0);

    uint8_t playing;
    for (playing = 1; playing;) {
        int freq;

        for (uint8_t c = playing = 0; c < NUM_CHANNELS; c++) {
            playing |= !chn[c].end;

            if (chn[c].end)
                continue;

            if ((--chn[c].timer) <= 0) {
                if (chn[c].ptr >= (struct agi_note *)song + size)
                    break;
                stop_note(c);
                freq = ((chn[c].ptr->frq_0 & 0x3f) << 4) | (int)(chn[c].ptr->frq_1 & 0x0f);
                if (freq) {
                    uint8_t v = chn[c].ptr->vol & 0x0f;
                    play_note(c, freq, v == 0xf ? 0 : 0xff - (v << 1));
                }
                chn[c].timer = ((int)chn[c].ptr->dur_hi << 8) | chn[c].ptr->dur_lo;
                if (chn[c].timer == 0xffff) {
                    chn[c].end = true;
                    chn[c].vol = 0;
                }
                chn[c].ptr++;
            }
        }

        mix_channels(400);
        dump_buffer(400);
    }

    progress.setMaximum(qbuffer->size());
    qbuffer->seek(0);
    audio_out->start(qbuffer);
    while (!qbuffer->atEnd()) {
        QEventLoop loop(audio_out);
        loop.processEvents();

        progress.setValue(qbuffer->pos());
        if (progress.wasCanceled()) {
            audio_out->stop();
            break;
        }
    }
    progress.close();
}


void play_sound(int ResNum)
{
    if (init_sound() != 0) {
        menu->errmes("Can't initialize sound !");
        return;
    }

    int err = game->ReadResource(SOUND, ResNum);
    if (err)
        return;

    play_song(ResourceData.Data, ResourceData.Size);

    close_sound();
}

void play_sound(char *filename)
{
    if (init_sound() != 0) {
        menu->errmes("Can't initialize sound !");
        return;
    }

    FILE *fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        menu->errmes("Can't open file %s !", filename);
        return;
    }
    struct stat buf;
    fstat(fileno(fptr), &buf);
    ResourceData.Size = buf.st_size;
    fread(ResourceData.Data, ResourceData.Size, 1, fptr);
    fclose(fptr);

    play_song(ResourceData.Data, ResourceData.Size);

    close_sound();
}
