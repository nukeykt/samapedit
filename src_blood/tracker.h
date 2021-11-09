#ifndef _TRACKER_H_
#define _TRACKER_H_

#include "build.h"
#include "db.h"

class CXTracker {
public:
    char m_type;
    int m_nSector;
    int m_nWall;
    int m_nSprite;
    sectortype* m_pSector;
    walltype* m_pWall;
    spritetype* m_pSprite;
    XSECTOR* m_pXSector;
    XWALL* m_pXWall;
    XSPRITE* m_pXSprite;
    CXTracker();
    void TrackClear();
    void TrackSector(int a1, char a2);
    void TrackWall(int a1, char a2);
    void TrackSprite(int a1, char a2);
    void Draw(int a1, int a2, int a3);
};

extern CXTracker gXTracker;

#endif
