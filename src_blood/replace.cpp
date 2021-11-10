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
#include "editor.h"
#include "common_game.h"
#include "crc32.h"

#include "tile.h"
#include "screen.h"

int qanimateoffs(short a1, short a2)
{
    int offset = 0;
    if (a1 >= 0 && a1 < kMaxTiles)
    {
        PICANM *_picanm = (PICANM*)picanm;
        int frames = _picanm[a1].animframes;
        if (frames > 0)
        {
            int vd;
            if ((a2&0xc000) == 0x8000)
                vd = (crc32once((unsigned char*)&a2, 2)+totalclock)>>_picanm[a1].animspeed;
            else
                vd = totalclock>>_picanm[a1].animspeed;
            switch (_picanm[a1].animtype)
            {
            case 1:
                offset = vd % (2*frames);
                if (offset >= frames)
                    offset = 2*frames-offset;
                break;
            case 2:
                offset = vd % (frames+1);
                break;
            case 3:
                offset = -(vd % (frames+1));
                break;
            }
        }
    }
    return offset;
}

void qloadpalette(void)
{
    scrLoadPalette();
}

int qgetpalookup(int a1, int a2)
{
    if (gFogMode)
        return ClipHigh(a1 >> 8, 15) * 16 + ClipRange(a2, 0, 15);
    else
        return ClipRange((a1 >> 8) + a2, 0, 63);
}

void HookReplaceFunctions(void)
{
    void qinitspritelists(void);
    int qinsertsprite(short nSector, short nStat);
    int qdeletesprite(short nSprite);
    int qchangespritesect(short nSprite, short nSector);
    int qchangespritestat(short nSprite, short nStatus);
    animateoffs_replace = qanimateoffs;
    loadpalette_replace = qloadpalette;
    getpalookup_replace = qgetpalookup;
    initspritelists_replace = qinitspritelists;
    insertsprite_replace = qinsertsprite;
    deletesprite_replace = qdeletesprite;
    changespritesect_replace = qchangespritesect;
    changespritestat_replace = qchangespritestat;
    loadvoxel_replace = qloadvoxel;
}
