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
#include "build.h"
#include "baselayer.h"
#include "winlayer.h"
#include "common_game.h"
#include "music.h"
#include "fx_man.h"
#include "resource.h"
#include "sound.h"


int FXVolume;
int MusicVolume;
int CDVolume;
int NumVoices;
int NumChannels;
int NumBits;
int MixRate;
int ReverseStereo;
int MusicDevice = -1;
int FXDevice = -1;
char *MusicParams;

Resource gSoundRes;

int soundRates[13] = {
    11025,
    11025,
    11025,
    11025,
    11025,
    22050,
    22050,
    22050,
    22050,
    44100,
    44100,
    44100,
    44100,
};

#define kChannelMax 32

int sndGetRate(int format)
{
    if (format < 13)
        return soundRates[format];
    return 11025;
}

SAMPLE2D Channel[kChannelMax];

SAMPLE2D * FindChannel(void)
{
    for (int i = kChannelMax-1; i >= 0; i--)
        if (Channel[i].at5 == 0)
            return &Channel[i];
    ThrowError("No free channel available for sample");
    return NULL;
}

DICTNODE *hSong;
char *pSongPtr;
int nSongSize;

void sndPlaySong(const char *songName, bool bLoop)
{
    if (MusicDevice == -1)
        return;
    if (nSongSize)
        sndStopSong();
    if (!songName || strlen(songName) == 0)
        return;
    hSong = gSoundRes.Lookup(songName, "MID");
    if (!hSong)
        return;
    int nNewSongSize = hSong->size;
    char *pNewSongPtr = (char *)malloc(nNewSongSize);
    gSoundRes.Load(hSong, pNewSongPtr);
    MUSIC_SetVolume(MusicVolume);
    MUSIC_PlaySong(pNewSongPtr, nNewSongSize, bLoop);
}

bool sndIsSongPlaying(void)
{
    if (MusicDevice == -1)
        return 0;
    return MUSIC_SongPlaying();
}

void sndFadeSong(int nTime)
{
    if (MusicDevice == -1)
        return;
    // UNREFERENCED_PARAMETER(nTime);
    // NUKE-TODO:
    //if (MusicDevice == -1)
    //    return;
    //if (gEightyTwoFifty && sndMultiPlayer)
    //    return;
    //MUSIC_FadeVolume(0, nTime);
    // MUSIC_SetVolume(0);
    MUSIC_StopSong();
}

void sndSetMusicVolume(int nVolume)
{
    if (MusicDevice == -1)
        return;
    MusicVolume = nVolume;
    MUSIC_SetVolume(nVolume);
}

void sndSetFXVolume(int nVolume)
{
    if (FXDevice == -1)
        return;
    FXVolume = nVolume;
    FX_SetVolume(nVolume);
}

void sndStopSong(void)
{
    if (MusicDevice == -1)
        return;
    MUSIC_StopSong();

    free(pSongPtr);
    nSongSize = 0;
}

void SoundCallback(unsigned int val)
{
    int *phVoice = (int*)val;
    *phVoice = 0;
}

void sndKillSound(SAMPLE2D *pChannel);

void sndStartSample(const char *pzSound, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    if (!strlen(pzSound))
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pzSound, "RAW");
    if (!pChannel->at5)
        return;
    int nSize = pChannel->at5->size;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(1), 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndStartSample(unsigned int nSound, int nVolume, int nChannel, bool bLoop)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    DICTNODE *hSfx = gSoundRes.Lookup(nSound, "SFX");
    if (!hSfx)
        return;
    SFX *pEffect = (SFX*)gSoundRes.Lock(hSfx);
    dassert(pEffect != NULL);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(pEffect->rawName, "RAW");
    if (!pChannel->at5)
        return;
    if (nVolume < 0)
        nVolume = pEffect->relVol;
    nVolume *= 80;
    nVolume = ClipRange(nVolume, 0, 255); // clamp to range that audiolib accepts
    int nSize = pChannel->at5->size;
    int nLoopEnd = nSize - 1;
    if (nLoopEnd < 0)
        nLoopEnd = 0;
    if (nSize <= 0)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    if (nChannel < 0)
        bLoop = false;
    if (bLoop)
    {
        pChannel->hVoice = FX_PlayLoopedRaw(pData, nSize, pData + pEffect->loopStart, pData + nLoopEnd, sndGetRate(pEffect->format),
            0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
        pChannel->at4 |= 1;
    }
    else
    {
        pChannel->hVoice = FX_PlayRaw(pData, nSize, sndGetRate(pEffect->format), 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
}

void sndStartWavID(unsigned int nSound, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    pChannel->at5 = gSoundRes.Lookup(nSound, "WAV");
    if (!pChannel->at5)
        return;
    char *pData = (char*)gSoundRes.Lock(pChannel->at5);
    pChannel->hVoice = FX_PlayWAV(pData, pChannel->at5->size, 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndKillSound(SAMPLE2D *pChannel)
{
    if (pChannel->at4 & 1)
    {
        FX_EndLooping(pChannel->hVoice);
        pChannel->at4 &= ~1;
    }
    FX_StopSound(pChannel->hVoice);
}

void sndStartWavDisk(const char *pzFile, int nVolume, int nChannel)
{
    if (FXDevice == -1)
        return;
    dassert(nChannel >= -1 && nChannel < kChannelMax);
    SAMPLE2D *pChannel;
    if (nChannel == -1)
        pChannel = FindChannel();
    else
        pChannel = &Channel[nChannel];
    if (pChannel->hVoice > 0)
        sndKillSound(pChannel);
    int hFile = kopen4load(pzFile, 0);
    if (hFile == -1)
        return;
    int nLength = kfilelength(hFile);
    char *pData = (char*)gSoundRes.Alloc(nLength);
    if (!pData)
    {
        kclose(hFile);
        return;
    }
    kread(hFile, pData, kfilelength(hFile));
    kclose(hFile);
    pChannel->at5 = (DICTNODE*)pData;
    pChannel->at4 |= 2;
    pChannel->hVoice = FX_PlayWAV(pData, nLength, 0, nVolume, nVolume, nVolume, nVolume, (intptr_t)&pChannel->hVoice);
}

void sndKillAllSounds(void)
{
    if (FXDevice == -1)
        return;
    for (int i = 0; i < kChannelMax; i++)
    {
        SAMPLE2D *pChannel = &Channel[i];
        if (pChannel->hVoice > 0)
            sndKillSound(pChannel);
        if (pChannel->at5)
        {
            if (pChannel->at4 & 2)
            {
#if 0
                free(pChannel->at5);
#else
                gSoundRes.Free(pChannel->at5);
#endif
                pChannel->at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(pChannel->at5);
            }
            pChannel->at5 = 0;
        }
    }
}

void sndProcess(void)
{
    if (FXDevice == -1)
        return;
    for (int i = 0; i < kChannelMax; i++)
    {
        if (Channel[i].hVoice <= 0 && Channel[i].at5)
        {
            if (Channel[i].at4 & 2)
            {
                gSoundRes.Free(Channel[i].at5);
                Channel[i].at4 &= ~2;
            }
            else
            {
                gSoundRes.Unlock(Channel[i].at5);
            }
            Channel[i].at5 = 0;
        }
    }
}

void InitSoundDevice(void)
{
    int fxdevicetype;
    // if they chose None lets return
    if (FXDevice < 0) {
        return;
    } else if (FXDevice == 0) {
        fxdevicetype = ASS_AutoDetect;
    } else {
        fxdevicetype = FXDevice - 1;
    }
#ifdef _WIN32
    void *initdata = (void *)win_gethwnd(); // used for DirectSound
#else
    void *initdata = NULL;
#endif
    int nStatus;
    nStatus = FX_Init(fxdevicetype, NumVoices, &NumChannels, &NumBits, &MixRate, initdata);
    if (nStatus != FX_Ok)
        ThrowError(FX_ErrorString(nStatus));
    if (ReverseStereo == 1)
        FX_SetReverseStereo(!FX_GetReverseStereo());
    FX_SetVolume(FXVolume);
    nStatus = FX_SetCallBack(SoundCallback);
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void DeinitSoundDevice(void)
{
    if (FXDevice == -1)
        return;
    int nStatus = FX_Shutdown();
    if (nStatus != 0)
        ThrowError(FX_ErrorString(nStatus));
}

void InitMusicDevice(void)
{
    int musicdevicetype;

    // if they chose None lets return
    if (MusicDevice < 0) {
       return;
    } else if (MusicDevice == 0) {
       musicdevicetype = ASS_AutoDetect;
    } else {
       musicdevicetype = MusicDevice - 1;
    }

    int nStatus = MUSIC_Init(musicdevicetype, MusicParams);
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
    // DICTNODE *hTmb = gSoundRes.Lookup("GMTIMBRE", "TMB");
    // if (hTmb)
    //     AL_RegisterTimbreBank((unsigned char*)gSoundRes.Load(hTmb));
    MUSIC_SetVolume(MusicVolume);
}

void DeinitMusicDevice(void)
{
    if (MusicDevice == -1)
        return;
    // FX_StopAllSounds();
    int nStatus = MUSIC_Shutdown();
    if (nStatus != 0)
        ThrowError(MUSIC_ErrorString(nStatus));
}

bool sndActive = false;

void sndTerm(void)
{
    if (!sndActive)
        return;
    sndActive = false;
    sndStopSong();
    DeinitSoundDevice();
    DeinitMusicDevice();
}
// extern char *pUserSoundRFF;
void sndInit(void)
{
    gSoundRes.Init(/*pUserSoundRFF ? pUserSoundRFF : */"SOUNDS.RFF");
    memset(Channel, 0, sizeof(Channel));
    pSongPtr = NULL;
    nSongSize = 0;
    InitSoundDevice();
    InitMusicDevice();
    sndActive = true;
}
