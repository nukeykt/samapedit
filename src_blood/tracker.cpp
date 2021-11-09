#include "build.h"
#include "editor.h"
#include "common_game.h"
#include "tracker.h"
#include "trig.h"

CXTracker::CXTracker()
{
    // dprintf("CXTracker::CXTracker()\n");
    TrackClear();
}

void DrawVector(int x1, int y1, int x2, int y2, char c, int a5)
{
    drawline16(x1, y1, x2, y2, c);
    int ang = getangle(x1 - x2, y1 - y2);
    int dx = mulscale30(a5/64, Cos(ang+170));
    int dy = mulscale30(a5/64, Sin(ang+170));
    drawline16(x2, y2, x2 + dx, y2 + dy, c);
    dx = mulscale30(a5/64, Cos(ang-170));
    dy = mulscale30(a5/64, Sin(ang-170));
    drawline16(x2, y2, x2 + dx, y2 + dy, c);
}

void CXTracker::TrackClear()
{
    // dprintf("CXTracker::TrackClear()\n");
    m_type = 0;
    m_nSector = m_nWall = m_nSprite = -1;
    m_pSector = 0;
    m_pWall = 0;
    m_pSprite = 0;
    m_pXSector = 0;
    m_pXWall = 0;
    m_pXSprite = 0;
}

void CXTracker::TrackSector(int nSector, char a2)
{
    // dprintf("CXTracker::TrackSector( %d, %s )\n", nSector, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nSector < 0 || nSector >= kMaxSectors)
        return;
    // dprintf("sector is valid\n");
    sectortype* pSector = &sector[nSector];
    int nXSector = pSector->extra;
    if (nXSector <= 0 || nXSector >= kMaxXSprites) // ???
        return;
    // dprintf("xsector is valid\n");
    XSECTOR* pXSector = &xsector[nXSector];
    if ((a2 && !pXSector->txID) || (!a2 && !pXSector->rxID))
        return;
    // dprintf("txID = %d,  rxID = %d\n", pXSector->txID, pXSector->rxID);
    m_nSector = nSector;
    m_pSector = pSector;
    m_pXSector = pXSector;
    m_type = a2;
}

void CXTracker::TrackWall(int nWall, char a2)
{
    // dprintf("CXTracker::TrackWall( %d, %s )\n", nWall, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nWall < 0 || nWall >= kMaxWalls)
        return;
    // dprintf("wall is valid\n");
    walltype* pWall = &wall[nWall];
    int nXWall = pWall->extra;
    if (nXWall <= 0 || nXWall >= kMaxXSprites) // ???
        return;
    // dprintf("xwall is valid\n");
    XWALL* pXWall = &xwall[nXWall];
    if ((a2 && !pXWall->txID) || (!a2 && !pXWall->rxID))
        return;
    // dprintf("txID = %d,  rxID = %d\n", pXWall->txID, pXWall->rxID);
    m_nWall = nWall;
    m_pWall = pWall;
    m_pXWall = pXWall;
    m_type = a2;
}

void CXTracker::TrackSprite(int nSprite, char a2)
{
    // dprintf("CXTracker::TrackSprite( %d, %s )\n", nSprite, a2 ? "TRUE" : "FALSE");
    TrackClear();
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    // dprintf("sprite is valid\n");
    spritetype* pSprite = &sprite[nSprite];
    int nXSprite = pSprite->extra;
    if (nXSprite <= 0 || nXSprite >= kMaxXSprites) // ???
        return;
    // dprintf("xsprite is valid\n");
    XSPRITE* pXSprite = &xsprite[nXSprite];
    if ((a2 && !pXSprite->txID) || (!a2 && !pXSprite->rxID))
        return;
    // dprintf("txID = %d,  rxID = %d\n", pXSprite->txID, pXSprite->rxID);
    m_nSprite = nSprite;
    m_pSprite = pSprite;
    m_pXSprite = pXSprite;
    m_type = a2;
}

void CXTracker::Draw(int a1, int a2, int a3)
{
    char v4 = (gFrameClock&16) ? 8 : 0;
    int txId, rxId, x, y;
    if (m_type)
    {
        if (m_pXSector)
        {
            txId = m_pXSector->txID;
            walltype* pWall2 = &wall[m_pSector->wallptr];
            x = mulscale(pWall2->x - a1, a3, 14)+halfxdim16;
            y = mulscale(pWall2->y - a2, a3, 14)+midydim16;
        }
        else if (m_pXWall)
        {
            txId = m_pXWall->txID;
            walltype* pWall2 = &wall[m_pWall->point2];
            int dx = (pWall2->x-m_pWall->x)/2; 
            int dy = (pWall2->y-m_pWall->y)/2; 
            int nx = m_pWall->x + dx;
            int ny = m_pWall->y + dy;

            x = mulscale(nx - a1, a3, 14)+halfxdim16;
            y = mulscale(ny - a2, a3, 14)+midydim16;
        }
        else if (m_pXSprite)
        {
            txId = m_pXSprite->txID;
            x = mulscale(m_pSprite->x - a1, a3, 14)+halfxdim16;
            y = mulscale(m_pSprite->y - a2, a3, 14)+midydim16;
        }
        else
        {
            return;
        }
        if (txId == 0)
            return;
        for (int i = 0; i < kMaxSprites; i++)
        {
            spritetype* pSprite = &sprite[i];
            if (pSprite->statnum < 0 || pSprite->statnum >= kMaxStatus)
                continue;
            int nXSprite = pSprite->extra;
            if (nXSprite <= 0)
                continue;
            XSPRITE* pXSprite = &xsprite[nXSprite];
            if (pXSprite->rxID == txId)
            {
                int x2 = mulscale(pSprite->x-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(pSprite->y-a2, a3, 14)+midydim16;
                DrawVector(x, y, x2, y2, v4 ^ 3, a3);
            }
        }
        for (int i = 0; i < numwalls; i++)
        {
            walltype* pWall = &wall[i];
            int nXWall = pWall->extra;
            if (nXWall <= 0)
                continue;
            XWALL* pXWall = &xwall[nXWall];
            if (pXWall->rxID == txId)
            {
                walltype* pWall2 = &wall[pWall->point2];
                int dx = (pWall2->x-pWall->x)/2;
                int dy = (pWall2->y-pWall->y)/2; 
                int nx = pWall->x + dx;
                int ny = pWall->y + dy;
                int x2 = mulscale(nx-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(ny-a2, a3, 14)+midydim16;
                DrawVector(x, y, x2, y2, v4^4, a3);
            }
        }
        for (int i = 0; i < numsectors; i++)
        {
            sectortype* pSector = &sector[i];
            int nXSector = pSector->extra;
            if (nXSector <= 0)
                continue;
            XSECTOR* pXSector = &xsector[nXSector];
            if (pXSector->rxID == txId)
            {
                walltype* pWall = &wall[pSector->wallptr];
                int x2 = mulscale(pWall->x-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(pWall->y-a2, a3, 14)+midydim16;
                DrawVector(x, y, x2, y2, v4^6, a3);
            }
        }
    }
    else
    {
        if (m_pXSector)
        {
            rxId = m_pXSector->rxID;
            walltype* pWall2 = &wall[m_pSector->wallptr];
            x = mulscale(pWall2->x - a1, a3, 14)+halfxdim16;
            y = mulscale(pWall2->y - a2, a3, 14)+midydim16;
        }
        else if (m_pXWall)
        {
            rxId = m_pXWall->rxID;
            walltype* pWall2 = &wall[m_pWall->point2];
            int dx = (pWall2->x-m_pWall->x)/2; 
            int dy = (pWall2->y-m_pWall->y)/2; 
            int nx = m_pWall->x + dx;
            int ny = m_pWall->y + dy;

            x = mulscale(nx - a1, a3, 14)+halfxdim16;
            y = mulscale(ny - a2, a3, 14)+midydim16;
        }
        else if (m_pXSprite)
        {
            rxId = m_pXSprite->rxID;
            x = mulscale(m_pSprite->x - a1, a3, 14)+halfxdim16;
            y = mulscale(m_pSprite->y - a2, a3, 14)+midydim16;
        }
        else
        {
            return;
        }
        if (rxId == 0)
            return;
        for (int i = 0; i < kMaxSprites; i++)
        {
            spritetype* pSprite = &sprite[i];
            if (pSprite->statnum < 0 || pSprite->statnum >= kMaxStatus)
                continue;
            int nXSprite = pSprite->extra;
            if (nXSprite <= 0)
                continue;
            XSPRITE* pXSprite = &xsprite[nXSprite];
            if (pXSprite->txID == rxId)
            {
                int x2 = mulscale(pSprite->x-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(pSprite->y-a2, a3, 14)+midydim16;
                DrawVector(x2, y2, x, y, v4 ^ 11, a3);
            }
        }
        for (int i = 0; i < numwalls; i++)
        {
            walltype* pWall = &wall[i];
            int nXWall = pWall->extra;
            if (nXWall <= 0)
                continue;
            XWALL* pXWall = &xwall[nXWall];
            if (pXWall->txID == rxId)
            {
                walltype* pWall2 = &wall[pWall->point2];
                int dx = (pWall2->x-pWall->x)/2;
                int dy = (pWall2->y-pWall->y)/2; 
                int nx = pWall->x + dx;
                int ny = pWall->y + dy;
                int x2 = mulscale(nx-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(ny-a2, a3, 14)+midydim16;
                DrawVector(x2, y2, x, y, v4^12, a3);
            }
        }
        for (int i = 0; i < numsectors; i++)
        {
            sectortype* pSector = &sector[i];
            int nXSector = pSector->extra;
            if (nXSector <= 0)
                continue;
            XSECTOR* pXSector = &xsector[nXSector];
            if (pXSector->txID == rxId)
            {
                walltype* pWall = &wall[pSector->wallptr];
                int x2 = mulscale(pWall->x-a1, a3, 14)+halfxdim16;
                int y2 = mulscale(pWall->y-a2, a3, 14)+midydim16;
                DrawVector(x2, y2, x, y, v4^14, a3);
            }
        }
    }
}

CXTracker gXTracker;
