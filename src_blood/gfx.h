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
#include "common_game.h"

#pragma pack(push, 1)

struct QBITMAP
{
    unsigned char at0; // type
    unsigned char at1; // transcolor
    unsigned short at2; // width
    unsigned short at4; // height
    unsigned short at6; // bpl
    unsigned short at8;
    unsigned short ata;
    char atc[1]; // data
};

typedef struct {
    int offset;
    unsigned char w;
    unsigned char h;
    unsigned char ox;
    signed char oy;
} QFONTCHAR;

struct QFONT
{
    unsigned char at0[4]; // signature
    unsigned char pad0[2];
    unsigned short at6;
    unsigned char pad1[0x7];
    unsigned char atf;
    unsigned char at10;
    signed char at11;
    unsigned char at12;
    unsigned char at13;
    unsigned char pad2[0xc];
    QFONTCHAR at20[256];
    char at820[1];
};

#pragma pack(pop)

extern int gColor;

void Video_HLine(int page, int y, int x0, int x1);
void Video_VLine(int page, int x, int y0, int y1);
void Video_FillBox(int page, int x0, int y0, int x1, int y1);
void Video_BlitM2V(char* src, int bpl, int width, int height, int page, int x, int y);
void Video_BlitMT2V(char* src, char tc, int bpl, int width, int height, int page, int x, int y);

void gfxDrawBitmap(QBITMAP* qbm, int x, int y);
void gfxPixel(int x, int y);
void gfxHLine(int y, int x0, int x1);
void gfxVLine(int x, int y0, int y1);
void gfxFillBox(int x0, int y0, int x1, int y1);
void gfxSetClip(int x0, int y0, int x1, int y1);
int gfxGetTextNLen(const char* pzText, QFONT* pFont, int a3);
int gfxGetLabelLen(const char* pzLabel, QFONT* pFont);
int gfxFindTextPos(const char* pzText, QFONT* pFont, int a3);
void gfxDrawText(int x, int y, int color, const char* pzText, QFONT* pFont);
void gfxDrawLabel(int, int, int, const char*, QFONT*);
