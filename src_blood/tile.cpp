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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_game.h"
extern "C" {
#include "a.h"
}

#include "resource.h"
#include "tile.h"
#include "gfx.h"
#include "gui.h"
#include "screen.h"
#include "keyboard.h"

void qloadvoxel(int32_t nVoxel)
{
    static int nLastVoxel = 0;
    DICTNODE *hVox = gSysRes.Lookup(nVoxel, "KVX");
    if (!hVox) {
        buildprintf("Missing voxel #%d (max voxels: %d)", nVoxel, kMaxVoxels);
        return;
    }

    if (!hVox->lockCount)
        voxoff[nLastVoxel][0] = 0;
    nLastVoxel = nVoxel;
    char *pVox = (char*)gSysRes.Lock(hVox);
    for (int i = 0; i < MAXVOXMIPS; i++)
    {
        int nSize = *((int*)pVox);
#if B_BIG_ENDIAN == 1
        nSize = B_LITTLE32(nSize);
#endif
        pVox += 4;
        voxoff[nVoxel][i] = (intptr_t)pVox;
        pVox += nSize;
    }
}

void tileTerm(void)
{
}

void CalcPicsiz(int a1, int a2, int a3)
{
    int nP = 0;
    for (int i = 2; i <= a2; i<<= 1)
        nP++;
    for (int i = 2; i <= a3; i<<= 1)
        nP+=1<<4;
    picsiz[a1] = nP;
}

// CACHENODE tileNode[kMaxTiles];

bool artLoaded = false;
// int nTileFiles = 0;

// int tileStart[256];
// int tileEnd[256];
// int hTileFile[256];

bool bShadeChanged, bSurfChanged, bVoxelChanged;

char surfType[kMaxTiles];
signed char tileShade[kMaxTiles];
short voxelIndex[kMaxTiles];
int tilelockcount[kMaxTiles];

// const char *pzBaseFileName = "TILES%03i.ART"; //"TILES%03i.ART";

// int32_t MAXCACHE1DSIZE = (96*1024*1024);

int tileInit(char a1, const char *a2)
{
    // UNREFERENCED_PARAMETER(a1);
    if (artLoaded)
        return 1;
    // artLoadFiles(a2 ? a2 : pzBaseFileName, MAXCACHE1DSIZE);
    memset(voxoff, 0, sizeof(voxoff));
    for (int i = 0; i < kMaxTiles; i++)
        voxelIndex[i] = 0;

    int hFile = kopen4load("SURFACE.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, surfType, sizeof(surfType));
        kclose(hFile);
    }
    hFile = kopen4load("VOXEL.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, voxelIndex, sizeof(voxelIndex));
#if B_BIG_ENDIAN == 1
        for (int i = 0; i < kMaxTiles; i++)
            voxelIndex[i] = B_LITTLE16(voxelIndex[i]);
#endif
        kclose(hFile);
    }
    hFile = kopen4load("SHADE.DAT", 0);
    if (hFile != -1)
    {
        kread(hFile, tileShade, sizeof(tileShade));
        kclose(hFile);
    }
    // for (int i = 0; i < kMaxTiles; i++)
    // {
    //     if (voxelIndex[i] >= 0 && voxelIndex[i] < kMaxVoxels)
    //         SetBitString((char*)voxreserve, voxelIndex[i]);
    // }

    artLoaded = 1;

    // #ifdef USE_OPENGL
    // PolymostProcessVoxels_Callback = tileProcessGLVoxels;
    // #endif

    return 1;
}

// #ifdef USE_OPENGL
// void tileProcessGLVoxels(void)
// {
//     static bool voxInit = false;
//     if (voxInit)
//         return;
//     voxInit = true;
//     for (int i = 0; i < kMaxVoxels; i++)
//     {
//         DICTNODE *hVox = gSysRes.Lookup(i, "KVX");
//         if (!hVox)
//             continue;
//         char *pVox = (char*)gSysRes.Load(hVox);
//         voxmodels[i] = loadkvxfrombuf(pVox, hVox->size);
//         voxvboalloc(voxmodels[i]);
//     }
// }
// #endif

void tileSaveArt(void)
{
    if (bShadeChanged)
    {
        int hFile = open("SHADE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, tileShade, sizeof(tileShade));
        close(hFile);
        bShadeChanged = 0;
    }
    if (bSurfChanged)
    {
        int hFile = open("SURFACE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, surfType, sizeof(surfType));
        close(hFile);
        bSurfChanged = 0;
    }
    if (bVoxelChanged)
    {
        int hFile = open("VOXEL.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, voxelIndex, sizeof(voxelIndex));
        close(hFile);
        bVoxelChanged = 0;
    }
    artLoaded = 0;
    tileInit(1, nullptr);
}

void tileSaveArtInfo(void)
{
    if (bShadeChanged)
    {
        int hFile = open("SHADE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, tileShade, sizeof(tileShade));
        close(hFile);
        bShadeChanged = 0;
    }
    if (bSurfChanged)
    {
        int hFile = open("SURFACE.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, surfType, sizeof(surfType));
        close(hFile);
        bSurfChanged = 0;
    }
    if (bVoxelChanged)
    {
        int hFile = open("VOXEL.DAT", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IWRITE);
        write(hFile, voxelIndex, sizeof(voxelIndex));
        close(hFile);
        bVoxelChanged = 0;
    }
}

void tileShadeDirty(void)
{
    bShadeChanged = 1;
}

void tileSurfDirty(void)
{
    bSurfChanged = 1;
}

void tileVoxelDirty(void)
{
    bVoxelChanged = 1;
}

void tilePurgeTile(int nTile)
{
#if 0
    waloff[nTile] = 0;
    CACHENODE *node = &tileNode[nTile];
    if (node->ptr)
    {
        Resource::Flush(node);
        dassert(node->ptr == NULL);
    }
#endif
    waloff[nTile] = 0;
    walock[nTile] = 0;
    tilelockcount[nTile] = 0;
}

void tilePurgeAll(void)
{
    memset(waloff, 0, sizeof(waloff));
    memset(walock, 0, sizeof(walock));
    memset(tilelockcount, 0, sizeof(tilelockcount));
}

char *tileLoadTile(int nTile)
{
#if 0
    static int nLastTile;
    if (tileNode[nLastTile].lockCount == 0)
        waloff[nLastTile] = 0;
    nLastTile = nTile;
    dassert(nTile >= 0 && nTile < kMaxTiles);
    CACHENODE* node = &tileNode[nTile];
    if (node->ptr)
    {
        waloff[nTile] = node->ptr;
        if (node->lockCount == 0)
        {
            RemoveMRU(node);
            node->prev = purgeHead.prev;
            purgeHead.prev->next = node;
            node->next = &purgeHead;
            node->next->prev = node;
            nLastTile = nTile;
        }
        return waloff[nTile];
    }
    int nSize = tilesizx[nTile] * tilesizy[nTile];
    if (nSize <= 0)
        return nullptr;
    gCacheMiss = gGameClock + 30;
    dassert(node->lockCount == 0);
    node->ptr = Resource::Alloc(nSize);
    waloff[nTile] = node->ptr;
    node->prev = purgeHead.prev;
    purgeHead.prev->next = node;
    node->next = &purgeHead;
    node->next->prev = node;
    // read tile from art ....
    return waloff[nTile];
#endif
    // static int nLastTile;
    // if (walock[nLastTile] < 200)
    //     waloff[nLastTile] = 0;
    // nLastTile = nTile;
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (waloff[nTile])
    {
        if (walock[nTile] < 200)
        {
            walock[nTile] = 199;
            // nLastTile = nTile;
        }
        return (char*)waloff[nTile];
    }
    int nSize = tilesizx[nTile] * tilesizy[nTile];
    if (nSize <= 0)
        return nullptr;

    dassert(walock[nTile] < 200);
    walock[nTile] = 199;
    allocache((void **)&waloff[nTile], nSize, &walock[nTile]);
    loadtile(nTile);

    return (char*)waloff[nTile];
}

char *tileLockTile(int nTile)
{
#if 0
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (!tileLoadTile(nTile))
        return nullptr;
    CACHENODE *node = &tileNode[nTile];
    if (node->lockCount++ == 0)
    {
        Resource::RemoveMRU(node);
    }
    return waloff[nTile];
#endif
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (!tileLoadTile(nTile))
        return nullptr;

    if (tilelockcount[nTile]++ == 0)
    {
        walock[nTile] = 255;
    }
    return (char*)waloff[nTile];
}

void tileUnlockTile(int nTile)
{
#if 0
    dassert(nTile >= 0 && nTile < kMaxTiles);
    waloff[nTile] = 0;
    CACHENODE *node = &tileNode[nTile];
    if (node->lockCount > 0)
    {
        node->lockCount--;
        if (node->lockCount == 0)
        {
            node->prev = purgeHead.prev;
            purgeHead.prev->next = node;
            node->next = &purgeHead;
            node->next->prev = node;
        }
    }
#endif
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (tilelockcount[nTile] > 0)
    {
        tilelockcount[nTile]--;
        if (tilelockcount[nTile] == 0)
        {
            walock[nTile] = 199;
        }
    }
}

char * tileAllocTile(int nTile, int x, int y, int ox, int oy)
{
#if 0
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (x <= 0 || y <= 0 || nTile >= kMaxTiles)
        return nullptr;
    int nSize = x * y;
    char *p = Resource::Alloc(nSize);
    dassert(p != NULL);
    tileNode[nTile].ptr = p;
    tileNode[nTile].lockCount++;
    waloff[nTile] = p;
    tilesizx[nTile] = x;
    tilesizy[nTile] = y;

    picanm[nTile].f_0_0 = 0;
    picanm[nTile].f_0_6 = 0;
    if (ox < -127)
        ox = -127;
    else if (ox > 127)
        ox = 127;
    picanm[nTile].f_1 = ox;
    if (oy < -127)
        oy = -127;
    else if (oy > 127)
        oy = 127;
    picanm[nTile].f_2 = oy;
    picanm[nTile].f_3_0 = 0;

    CalcPicsiz(nTile, x, y);

    return waloff[nTile];
#endif
    dassert(nTile >= 0 && nTile < kMaxTiles);
    if (x <= 0 || y <= 0 || nTile >= kMaxTiles)
        return nullptr;
    int nSize = x * y;
    walock[nTile] = 255;
    allocache((void **)&waloff[nTile], nSize, &walock[nTile]);
    dassert(waloff[nTile] != 0);
    tilelockcount[nTile]++;
    tilesizx[nTile] = x;
    tilesizy[nTile] = y;

    PICANM *_picanm = (PICANM*)picanm;

    _picanm[nTile].animframes = 0;
    _picanm[nTile].animtype = 0;
    if (ox < -127)
        ox = -127;
    else if (ox > 127)
        ox = 127;
    _picanm[nTile].xoffset = ox;
    if (oy < -127)
        oy = -127;
    else if (oy > 127)
        oy = 127;
    _picanm[nTile].yoffset = oy;
    _picanm[nTile].animspeed = 0;

    CalcPicsiz(nTile, x, y);

    return (char*)waloff[nTile];
}


void tileFreeTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles);
    tilePurgeTile(nTile);

    waloff[nTile] = 0;

    PICANM *_picanm = (PICANM*)picanm;
    _picanm[nTile].animframes = 0;
    _picanm[nTile].animtype = 0;
    _picanm[nTile].xoffset = 0;
    _picanm[nTile].yoffset = 0;
    _picanm[nTile].animspeed = 0;
    tilesizx[nTile] = 0;
    tilesizy[nTile] = 0;
}


void tilePreloadTile(int nTile)
{
    int n = 1;
    PICANM *_picanm = (PICANM*)picanm;
    switch (_picanm[nTile].at3_4)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    case 6:
    case 7:
        if (voxelIndex[nTile] < 0 || voxelIndex[nTile] >= kMaxVoxels)
        {
            voxelIndex[nTile] = -1;
            _picanm[nTile].at3_4 = 0;
        }
        else
            qloadvoxel(voxelIndex[nTile]);
        break;
    }
    while(n--)
    {
        if (_picanm[nTile].animtype)
        {
            for (int frame = _picanm[nTile].animframes; frame >= 0; frame--)
            {
                if (_picanm[nTile].animtype == 3)
                    tileLoadTile(nTile-frame);
                else
                    tileLoadTile(nTile+frame);
            }
        }
        else
            tileLoadTile(nTile);
        nTile += 1+_picanm[nTile].animframes;
    }
}

// int nPrecacheCount;
// char precachehightile[2][(MAXTILES+7)>>3];

void tilePrecacheTile(int nTile)
{
    int n = 1;
    PICANM *_picanm = (PICANM*)picanm;
    switch (_picanm[nTile].at3_4)
    {
    case 0:
        n = 1;
        break;
    case 1:
        n = 5;
        break;
    case 2:
        n = 8;
        break;
    case 3:
        n = 2;
        break;
    }
    while(n--)
    {
        if (_picanm[nTile].animtype)
        {
            for (int frame = _picanm[nTile].animframes; frame >= 0; frame--)
            {
                if (_picanm[nTile].animtype == 3)
                    SetBitString(gotpic, nTile-frame);
                else
                    SetBitString(gotpic, nTile+frame);
            }
        }
        else
        {
            SetBitString(gotpic, nTile);
        }
        nTile += 1+_picanm[nTile].animframes;
    }
}

int tileHist[kMaxTiles];
short tileIndex[kMaxTiles];
int tileIndexCount;

int CompareTileFreqs(const void *a1, const void *a2)
{
    short tile1 = *(short*)a1;
    short tile2 = *(short*)a2;
    return tileHist[tile2] - tileHist[tile1];
}

int tileBuildHistogram(int a1)
{
    memset(tileHist, 0, sizeof(tileHist));
    switch (a1)
    {
    case 0:
        for (int i = 0; i < numwalls; i++)
        {
            tileHist[wall[i].picnum]++;
        }
        break;
    case 1:
        for (int i = 0; i < numsectors; i++)
        {
            tileHist[sector[i].floorpicnum]++;
        }
        break;
    case 2:
        for (int i = 0; i < numsectors; i++)
        {
            tileHist[sector[i].ceilingpicnum]++;
        }
        break;
    case 4:
        for (int i = 0; i < numwalls; i++)
        {
            tileHist[wall[i].overpicnum]++;
        }
        break;
    case 3:
        for (int i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum != kStatFree && sprite[i].statnum != kStatMarker)
            {
                if ((sprite[i].cstat & 32768) == 0)
                {
                    if ((sprite[i].cstat & 48) == 0)
                    {
                        tileHist[sprite[i].picnum]++;
                    }
                }
            }
        }
        break;
    case 5:
        for (int i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum != kStatFree && sprite[i].statnum != kStatMarker)
            {
                if ((sprite[i].cstat & 32768) == 0)
                {
                    if ((sprite[i].cstat & 48) != 0)
                    {
                        tileHist[sprite[i].picnum]++;
                    }
                }
            }
        }
        break;
    }
    for (int i = 0; i < kMaxTiles; i++)
    {
        tileIndex[i] = i;
    }
    qsort(tileIndex, kMaxTiles, sizeof(short), CompareTileFreqs);
    tileIndexCount = 0;
    while (tileIndex[tileIndexCount] > 0 && tileIndexCount < kMaxTiles)
    {
        tileIndexCount++;
    }
    return tileIndex[0];
}

void tileDrawTileScreen(int a1, int a2, int a3, int a4)
{
    // v18 = a1
    // vc = a2
    // v34 = a3
    // v20 = a4
    char v0[12];
    int v1c = xdim / a3;
    int v10 = ydim / a3;
    int v24 = 0;
    int v30 = 0;
    gColor = gStdColor[0];
    clearview(0);
    for (int v14 = 0; v14 < v10; v14++, v30 += a3)
    {
        int v2c = 0;
        for (int v28 = 0; v28 < v1c && a1+v24 < a4; v28++, v24++, v2c += a3)
        {
            int v38 = tileIndex[a1 + v24];
            if (tilesizx[v38] > 0 && tilesizy[v38] > 0)
            {
                int v40 = picsiz[v38] >> 4;
                v40 = 30 - v40;
                setupvlineasm(v40);
                palookupoffse[0] = palookupoffse[1] = palookupoffse[2] = palookupoffse[3] = (intptr_t)palookup[0];
                intptr_t v58 = (intptr_t)tileLoadTile(v38);
                unsigned int v4c = tilesizx[v38];
                unsigned int v60 = tilesizy[v38];
                intptr_t v5c = frameplace + ylookup[v30] + v2c;
                if (v4c <= a3 && v60 <= a3)
                {
                    bufplce[3] = v58 - v60;
                    vince[0] = vince[1] = vince[2] = vince[3] = 1<<v40;
                    unsigned int vbp;
                    for (vbp = 0; vbp+3 < v4c; vbp += 4)
                    {
                        bufplce[0] = bufplce[3] + v60;
                        bufplce[1] = bufplce[0] + v60;
                        bufplce[2] = bufplce[1] + v60;
                        bufplce[3] = bufplce[2] + v60;
                        vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
                        vlineasm4(v60, (void*)v5c);
                        v5c += 4;
                        v58 += v60 * 4;
                    }
                    for (; vbp < v4c; vbp++)
                    {
                        vlineasm1(1<<v40, (void*)palookupoffse[0], v60-1, 0, (void*)v58, (void*)v5c);
                        v58 += v60;
                        v5c++;
                    }
                }
                else
                {
                    unsigned int v50, v3c;
                    int v48, v44;
                    if (v4c > v60)
                    {
                        v50 = divscale16(v4c, a3);
                        v3c = divscale(v4c, a3, v40);
                        v48 = (a3 * v60) / v4c;
                        v44 = a3;
                    }
                    else
                    {
                        v50 = divscale16(v60, a3);
                        v3c = divscale(v60, a3, v40);
                        v48 = a3;
                        v44 = (a3 * v4c) / v60;
                    }
                    if (v48 == 0)
                        continue;
                    unsigned int vbp = 0;
                    vince[0] = vince[1] = vince[2] = vince[3] = v3c;
                    int v54;
                    for (v54 = 0; v54+3 < v44; v54 += 4)
                    {
                        bufplce[0] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[0] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[0] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        bufplce[0] = v58 + (vbp >> 16) * v60;
                        vbp += v50;
                        vplce[0] = vplce[1] = vplce[2] = vplce[3] = 0;
                        vlineasm4(v48, (void*)v5c);
                        v5c += 4;
                    }
                    for (; v54 < v44; v54++)
                    {
                        v58 = waloff[v38] + (vbp >> 16) * v60;
                        vlineasm1(v3c, (void*)palookupoffse[0], v48 - 1, 0, (void*)v58, (void*)v5c);
                        v5c++;
                        vbp += v50;
                    }
                }
                if (a4 < kMaxTiles && keystatus[0x3a] == 0)
                {
                    sprintf(v0, "%d", tileHist[v38]);
                    Video_FillBox(0, v2c + a3 - strlen(v0) * 4 - 2, v30, v2c + a3, v30 + 7);
                    printext256(v2c + a3 - strlen(v0) * 4 - 1, v30, gStdColor[11], -1, v0, 1);
                }
            }
            if (keystatus[0x3a] == 0)
            {
                sprintf(v0, "%d", v38);
                Video_FillBox(0, v2c, v30, v2c + strlen(v0) * 4 + 1, v30 + 7);
                printext256(v2c + 1, v30, gStdColor[14], -1, v0, 1);
            }
        }
    }
    if (IsBlinkOn())
    {
        int v54 = a2 - a1;
        int v2c = (v54 % v1c) * a3;
        int v40 = (v54 / v1c) * a3;
        Video_HLine(0, v40, v2c, v2c+a3-1);
        Video_HLine(0, v40+a3-1, v2c, v2c+a3-1);
        Video_VLine(0, v2c, v40, v40+a3-1);
        Video_VLine(0, v2c+a3-1, v40, v40+a3-1);
    }
}

int pickSize[] = { 32, 40, 64, 80, 128, 160 };

int tilePick(int a1, int a2, int a3)
{
    static int nZoom = 3;
    int vdi = xdim / pickSize[nZoom];
    int v8 = ydim / pickSize[nZoom];
    if (a3 != -2)
        tileBuildHistogram(a3);
    if (tileIndexCount == 0)
    {
        tileIndexCount = kMaxTiles;
        for (int i = 0; i < kMaxTiles; i++)
        {
            tileIndex[i] = i;
        }
    }
    int vbp = 0;
    for (int i = 0; i < tileIndexCount; i++)
    {
        if (tileIndex[i] == a1)
        {
            vbp = i;
            break;
        }
    }
    int vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
    while (1)
    {
        handleevents();
        tileDrawTileScreen(vsi, vbp, nZoom, tileIndexCount);
#if 0
        if (vidoption != 1)
        {
            // WaitVBL
        }
#endif
        scrNextPage();
        gFrameTicks = totalclock - gFrameClock;
        gFrameClock += gFrameTicks;
        UpdateBlinkClock(gFrameTicks);
        unsigned char key = keyGetScan();
        switch (key)
        {
        case 0x37:
            if (nZoom > 0)
            {
                nZoom--;
                vdi = xdim / pickSize[nZoom];
                v8 = ydim / pickSize[nZoom];
                vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
            }
            break;
        case 0xb5:
            if ((unsigned int)(nZoom+1) < 6)
            {
                nZoom++;
                vdi = xdim / pickSize[nZoom];
                v8 = ydim / pickSize[nZoom];
                vsi = ClipLow(((vbp - v8 * vdi + vdi) / vdi) * vdi, 0);
            }
            break;
        case 0xcb:
            if (vbp - 1 >= 0)
                vbp--;
            SetBlinkOn();
            break;
        case 0xcd:
            if (vbp + 1 < tileIndexCount)
                vbp++;
            SetBlinkOn();
            break;
        case 0xc8:
            if (vbp - vdi >= 0)
                vbp -= vdi;
            SetBlinkOn();
            break;
        case 0xd0:
            if (vbp + vdi < tileIndexCount)
                vbp += vdi;
            SetBlinkOn();
            break;
        case 0xc9:
            if (vbp - v8 * vdi >= 0)
            {
                vbp -= v8 * vdi;
                vsi -= v8 * vdi;
                if (vsi < 0)
                    vsi = 0;
            }
            SetBlinkOn();
            break;
        case 0xd1:
            if (vbp + v8 * vdi < tileIndexCount)
            {
                vbp += v8 * vdi;
                vsi += v8 * vdi;
            }
            SetBlinkOn();
            break;
        case 0xc7:
            SetBlinkOn();
            vbp = 0;
            break;
        case 0x2f:
            if (a3 != -2 && tileIndexCount < kMaxTiles)
            {
                tileIndexCount = kMaxTiles;
                for (int i = 0; i < kMaxTiles; i++)
                {
                    tileIndex[i] = i;
                }
            }
            break;
        case 0x22:
        {
            int vcx = GetNumberBox("Goto tile", 0, tileIndex[vbp]);
            if (vcx < kMaxTiles && vcx != tileIndex[vbp])
            {
                vbp = vcx;
                tileIndexCount = kMaxTiles;
                for (int i = 0; i < kMaxTiles; i++)
                {
                    tileIndex[i] = i;
                }
            }
            break;
        }

        case 0x01:
            clearview(0);
            keystatus[0x01] = 0;
            return a2;
        case 0x1c:
        {
            clearview(0);
            int nTile = tileIndex[vbp];
            if (!tilesizx[nTile] || !tilesizy[nTile])
                return a2;
            return nTile;
        }
        }
        while (vbp < vsi)
        {
            vsi -= vdi;
        }
        while (v8 * vdi + vsi <= vbp)
        {
            vsi += vdi;
        }
        if (key)
            keyFlushScans();
    }
}

char tileGetSurfType(int hit)
{
    int n = hit & 0x3fff;
    switch (hit&0xc000)
    {
    case 0x4000:
        return surfType[sector[n].floorpicnum];
    case 0x6000:
        return surfType[sector[n].ceilingpicnum];
    case 0x8000:
        return surfType[wall[n].picnum];
    case 0xc000:
        return surfType[sprite[n].picnum];
    }
    return 0;
}
