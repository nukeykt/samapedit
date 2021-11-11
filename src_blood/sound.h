//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once

#include "resource.h"
struct SAMPLE2D
{
    int hVoice;
    char at4;
    DICTNODE *at5;
}; // 9 bytes

struct SFX
{
    int relVol;
    int pitch;
    int pitchRange;
    int format;
    int loopStart;
    char rawName[9];
};

extern int FXVolume;
extern int MusicVolume;
extern int CDVolume;
extern int NumVoices;
extern int NumChannels;
extern int NumBits;
extern int MixRate;
extern int ReverseStereo;
extern int MusicDevice;
extern int FXDevice;
extern char *MusicParams;

int sndGetRate(int format);
void sndPlaySong(const char *songName, bool bLoop);
bool sndIsSongPlaying(void);
void sndFadeSong(int nTime);
void sndSetMusicVolume(int nVolume);
void sndSetFXVolume(int nVolume);
void sndStopSong(void);
void sndStartSample(const char *pzSound, int nVolume, int nChannel = -1);
void sndStartSample(unsigned int nSound, int nVolume, int nChannel = -1, bool bLoop = false);
void sndStartWavID(unsigned int nSound, int nVolume, int nChannel = -1);
void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel = -1);
void sndKillAllSounds(void);
void sndProcess(void);
void sndTerm(void);
void sndInit(void);

extern Resource gSoundRes;
