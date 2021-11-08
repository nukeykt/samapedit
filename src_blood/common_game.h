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
#include "build.h"
#include "baselayer.h"
#include "cache1d.h"
#include "pragmas.h"
#include "resource.h"

void _SetErrorLoc(const char* pzFile, int nLine);
void _ThrowError(const char* pzFormat, ...);
void __dassert(const char* pzExpr, const char* pzFile, int nLine);

#define ThrowError(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_ThrowError(__VA_ARGS__); \
	}

// print error to console only
#define consoleSysMsg(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_consoleSysMsg(__VA_ARGS__); \
	}

#define dassert(x) if (!(x)) __dassert(#x,__FILE__,__LINE__)


#define kMaxSectors MAXSECTORS
#define kMaxWalls MAXWALLS
#define kMaxSprites MAXSPRITES

#define kMaxTiles MAXTILES
#define kMaxStatus MAXSTATUS
#define kMaxViewSprites MAXSPRITESONSCREEN

#define kMaxVoxels MAXVOXELS

extern int gFrameClock;
extern int gFrameTicks;
extern int gFrame;
extern int gFrameRate;
extern int gGamma;
extern Resource gSysRes;

inline int ClipLow(int a, int b)
{
    if (a < b)
        return b;
    return a;
}

inline int ClipHigh(int a, int b)
{
    if (a >= b)
        return b;
    return a;
}

inline int ClipRange(int a, int b, int c)
{
    if (a < b)
        return b;
    if (a > c)
        return c;
    return a;
}

inline int interpolate(int a, int b, int c)
{
    return a+mulscale16(b-a,c);
}

inline int dmulscale30r(int a, int b, int c, int d)
{
    int64_t acc = 1<<(30-1);
    acc += ((int64_t)a) * b;
    acc += ((int64_t)c) * d;
    return (int)(acc>>30);
}

inline int approxDist(int dx, int dy)
{
    dx = klabs(dx);
    dy = klabs(dy);
    if (dx > dy)
        dy = (3*dy)>>3;
    else
        dx = (3*dx)>>3;
    return dx+dy;
}

class Rect {
public:
    int x0, y0, x1, y1;
    Rect(int _x0, int _y0, int _x1, int _y1)
    {
        x0 = _x0; y0 = _y0; x1 = _x1; y1 = _y1;
    }
    bool isValid(void) const
    {
        return x0 < x1 && y0 < y1;
    }
    char isEmpty(void) const
    {
        return !isValid();
    }
    bool operator!(void) const
    {
        return isEmpty();
    }

    Rect & operator&=(Rect &pOther)
    {
        x0 = ClipLow(x0, pOther.x0);
        y0 = ClipLow(y0, pOther.y0);
        x1 = ClipHigh(x1, pOther.x1);
        y1 = ClipHigh(y1, pOther.y1);
        return *this;
    }

    void offset(int dx, int dy)
    {
        x0 += dx;
        y0 += dy;
        x1 += dx;
        y1 += dy;
    }

    int height()
    {
        return y1 - y0;
    }

    int width()
    {
        return x1 - x0;
    }

    bool inside(Rect& other)
    {
        return (x0 <= other.x0 && x1 >= other.x1 && y0 <= other.y0 && y1 >= other.y1);
    }

    bool inside(int x, int y)
    {
        return (x0 <= x && x1 > x && y0 <= y && y1 > y);
    }
};

class BitReader {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitReader(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; nSize -= nBitPos>>3; }
    BitReader(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; }
    int readBit()
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        int bit = ((*pBuffer)>>nBitPos)&1;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
        return bit;
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    unsigned int readUnsigned(int nBits)
    {
        unsigned int n = 0;
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            n += readBit()<<i;
        return n;
    }
    int readSigned(int nBits)
    {
        dassert(nBits <= 32);
        int n = (int)readUnsigned(nBits);
        n <<= 32-nBits;
        n >>= 32-nBits;
        return n;
    }
};

class BitWriter {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitWriter(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; memset(pBuffer, 0, nSize); nSize -= nBitPos>>3; }
    BitWriter(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; memset(pBuffer, 0, nSize); }
    void writeBit(int bit)
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        *pBuffer |= bit<<nBitPos;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    void write(int nValue, int nBits)
    {
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            writeBit((nValue>>i)&1);
    }
};

