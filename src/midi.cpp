/*
 *  QT AGI Studio :: Copyright (C) 2000 Helen Zommer
 *
 *  Midi support, written by Jarno Elonen <elonen@iki.fi>, 2003
 *  Based on SND2MIDI 1.2 by Jens Restemeier <jrestemeier@currantbun.com>
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

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <QDataStream>
#include <QIODevice>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>

static const char *g_gm_instrument_names[] = {
    "1. Acoustic Grand Piano",
    "2. Bright Acoustic Piano",
    "3. Electric Grand Piano",
    "4. Honky-tonk Piano",
    "5. Electric Piano 1",
    "6. Electric Piano 2",
    "7. Harpsichord",
    "8. Clavi",
    "9. Celesta",
    "10. Glockenspiel",
    "11. Music Box",
    "12. Vibraphone",
    "13. Marimba",
    "14. Xylophone",
    "15. Tubular Bells",
    "16. Dulcimer",
    "17. Drawbar Organ",
    "18. Percussive Organ",
    "19. Rock Organ",
    "20. Church Organ",
    "21. Reed Organ",
    "22. Accordion",
    "23. Harmonica",
    "24. Tango Accordion",
    "25. Acoustic Guitar (nylon)",
    "26. Acoustic Guitar (steel)",
    "27. Electric Guitar (jazz)",
    "28. Electric Guitar (clean)",
    "29. Electric Guitar (muted)",
    "30. Overdriven Guitar",
    "31. Distortion Guitar",
    "32. Guitar harmonics",
    "33. Acoustic Bass",
    "34. Electric Bass (finger)",
    "35. Electric Bass (pick)",
    "36. Fretless Bass",
    "37. Slap Bass 1",
    "38. Slap Bass 2",
    "39. Synth Bass 1",
    "40. Synth Bass 2",
    "41. Violin",
    "42. Viola",
    "43. Cello",
    "44. Contrabass",
    "45. Tremolo Strings",
    "46. Pizzicato Strings",
    "47. Orchestral Harp",
    "48. Timpani",
    "49. String Ensemble 1",
    "50. String Ensemble 2",
    "51. SynthStrings 1",
    "52. SynthStrings 2",
    "53. Choir Aahs",
    "54. Voice Oohs",
    "55. Synth Voice",
    "56. Orchestra Hit",
    "57. Trumpet",
    "58. Trombone",
    "59. Tuba",
    "60. Muted Trumpet",
    "61. French Horn",
    "62. Brass Section",
    "63. SynthBrass 1",
    "64. SynthBrass 2",
    "65. Soprano Sax",
    "66. Alto Sax",
    "67. Tenor Sax",
    "68. Baritone Sax",
    "69. Oboe",
    "70. English Horn",
    "71. Bassoon",
    "72. Clarinet",
    "73. Piccolo",
    "74. Flute",
    "75. Recorder",
    "76. Pan Flute",
    "77. Blown Bottle",
    "78. Shakuhachi",
    "79. Whistle",
    "80. Ocarina",
    "81. Lead 1 (square)",
    "82. Lead 2 (sawtooth)",
    "83. Lead 3 (calliope)",
    "84. Lead 4 (chiff)",
    "85. Lead 5 (charang)",
    "86. Lead 6 (voice)",
    "87. Lead 7 (fifths)",
    "88. Lead 8 (bass + lead)",
    "89. Pad 1 (new age)",
    "90. Pad 2 (warm)",
    "91. Pad 3 (polysynth)",
    "92. Pad 4 (choir)",
    "93. Pad 5 (bowed)",
    "94. Pad 6 (metallic)",
    "95. Pad 7 (halo)",
    "96. Pad 8 (sweep)",
    "97. FX 1 (rain)",
    "98. FX 2 (soundtrack)",
    "99. FX 3 (crystal)",
    "100. FX 4 (atmosphere)",
    "101. FX 5 (brightness)",
    "102. FX 6 (goblins)",
    "103. FX 7 (echoes)",
    "104. FX 8 (sci-fi)",
    "105. Sitar",
    "106. Banjo",
    "107. Shamisen",
    "108. Koto",
    "109. Kalimba",
    "110. Bag pipe",
    "111. Fiddle",
    "112. Shanai",
    "113. Tinkle Bell",
    "114. Agogo",
    "115. Steel Drums",
    "116. Woodblock",
    "117. Taiko Drum",
    "118. Melodic Tom",
    "119. Synth Drum",
    "120. Reverse Cymbal",
    "121. Guitar Fret Noise",
    "122. Breath Noise",
    "123. Seashore",
    "124. Bird Tweet",
    "125. Telephone Ring",
    "126. Helicopter",
    "127. Applause",
    "128. Gunshot",
    NULL
};

// Write given AGI Sound resource to given QIODevice
void writeMidi(const byte *snd, QIODevice &write_to, const unsigned char instr[])
{
    long delta_tmp;
#define WRITE_DELTA(X) \
    delta_tmp=X>>21; if (delta_tmp>0) out << quint8((delta_tmp&127)|128); \
    delta_tmp=X>>14; if (delta_tmp>0) out << quint8((delta_tmp&127)|128); \
    delta_tmp=X>>7;  if (delta_tmp>0) out << quint8((delta_tmp&127)|128); \
    out << quint8(X&127)

    double ll = log10(pow(2.0, 1.0 / 12.0));

    if (write_to.isSequential() || !write_to.isWritable())
        qFatal("writeMidi() requires a writable random access IODevice");

    QDataStream out(&write_to);

    // Header
    out.writeRawData("MThd", 4);
    out << quint32(6)
        << quint16(1)    // mode
        << quint16(3)    // # of tracks
        << quint16(96);  // ticks / quarter

    for (int n = 0; n < 3; n++) {
        out.writeRawData("MTrk", 4);
        qlonglong lp = write_to.pos();
        out << quint32(0); // chunklength
        WRITE_DELTA(0); // set instrument
        out << quint8(0xc0 + n)
            << quint8(instr[n]);

        unsigned short start = snd[n * 2 + 0] | (snd[n * 2 + 1] << 8);
        unsigned short end = ((snd[n * 2 + 2] | (snd[n * 2 + 3] << 8))) - 5;

        for (unsigned short pos = start; pos < end; pos += 5) {
            unsigned short freq, dur;
            dur = (snd[pos + 0] | (snd[pos + 1] << 8)) * 4;
            freq = ((snd[pos + 2] & 0x3F) << 4) + (snd[pos + 3] & 0x0F);
            if (snd[pos + 2] > 0) {
                double fr;
                int note;
                // I don't know, what frequency equals midi note 0 ...
                // This moves the song 3 octaves down:
                fr = (log10(111860.0 / (double)freq) / ll) - 36;
                note = (int)floor(fr);
                if (note < 0)
                    note = 0;
                if (note > 127)
                    note = 127;
                // note on
                WRITE_DELTA(0);
                out << quint8(0x90 + n)
                    << quint8(note)
                    << quint8(100);
                // note off
                WRITE_DELTA(dur);
                out << quint8(0x80 + n)
                    << quint8(note)
                    << quint8(0);
            } else {
                // note on
                WRITE_DELTA(0);
                out << quint8(0x90 + n)
                    << quint8(0)
                    << quint8(0);
                // note off
                WRITE_DELTA(dur);
                out << quint8(0x80 + n)
                    << quint8(0)
                    << quint8(0);
            }
        }

        WRITE_DELTA(0);
        out << quint8(0xff)
            << quint8(0x2f)
            << quint8(0x0);

        qlonglong ep = write_to.pos();
        write_to.seek(lp);
        out << quint32((ep - lp) - 4);
        write_to.seek(ep);
    }
#undef WRITE_DELTA
}

static unsigned char s_selected_instr[3] = {0, 0, 0};

// Show a "Save as" file dialog and call the MIDI export function
void showSaveAsMidi(QWidget *parent, const byte *snd)
{
    class MyFileDialog : public QFileDialog
    {
    public:
        MyFileDialog(QWidget *parent, const char *name) :
            QFileDialog(parent, name)
        {
            auto instrument_names = QStringList();
            for (int n = 0; n < 128; n++)
                instrument_names.append(g_gm_instrument_names[n]);

            QLabel *label = new QLabel("Channel instruments", this);
            label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            QWidget *butts = new QWidget(this);
            for (int i = 0; i < 3; ++i) {
                instr[i] = new QComboBox(butts);
                instr[i]->insertItems(i, instrument_names);
                instr[i]->setCurrentIndex(s_selected_instr[i]);
            }

            QVBoxLayout *layout = new QVBoxLayout;
            layout->addWidget(label);
            layout->addWidget(butts);
        }
        QComboBox *instr[3];
    } fd(parent, NULL);

    fd.setNameFilter("MIDI files (*.mid *.midi)");
    fd.setAcceptMode(QFileDialog::AcceptSave);

    if ((fd.exec() == QDialog::Accepted) && !fd.selectedFiles().isEmpty()) {
        QString fname = fd.selectedFiles().first();
        if (fname.lastIndexOf('.') < 0)
            fname += ".mid";

        QFile f(fname);
        f.open(QIODevice::WriteOnly);

        for (int i = 0; i < 3; ++i)
            s_selected_instr[i] = (unsigned char)fd.instr[i]->currentIndex();

        writeMidi(snd, f, s_selected_instr);
        f.close();
    }
}
