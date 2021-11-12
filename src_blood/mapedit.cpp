#include <direct.h>
#include <stdlib.h>
#include "compat.h"
#include "build.h"
#include "editor.h"
#include "baselayer.h"
#include "common_game.h"
#include "db.h"
#include "keyboard.h"
#include "trig.h"
#include "tracker.h"
#include "tile.h"
#include "gui.h"
#include "gfx.h"
#include "screen.h"
#include "sectorfx.h"
#include "gameutil.h"
#include "inifile.h"
#include "replace.h"
#include "sound.h"
#include "fx_man.h"

///////// globals ///////////////

extern "C" {
extern int angvel, svel, vel;
extern short pointhighlight, linehighlight, highlightcnt;
extern short highlight[MAXWALLS];
extern short highlightsector[MAXSECTORS], highlightsectorcnt;
extern short asksave;
void updatenumsprites(void);
void fixrepeats(short i);
unsigned char changechar(unsigned char dachar, int dadir, unsigned char smooshyalign, unsigned char boundcheck);
void overheadeditor(void);
};

char gTempBuf[256];

int gGrid = 4;
int gZoom = 0x300;
int gHighlightThreshold;
int gStairHeight;

short sectorhighlight;

unsigned char byte_CBA0C = 1;

unsigned char gBeep;
unsigned char gOldKeyMapping;
int gCorrectedSprites;
int gAutoSaveInterval;

const char* dword_D9A88[2];
const char* dword_D9A90[1024];
const char* dword_DAA90[1024];
const char* dword_DBA90[1024];
const char* WaveForm2[8];
const char* WaveForm[20];
const char* dword_DCB00[64];
const char* dword_DCC00[192];
const char* dword_DCF00[4];
char byte_D9760[192][4];

void ModifyBeep(void);
void Beep(void);
void sub_1058C(void);
int sub_10DBC(int nSector);
int sub_10E08(int nWall);
int sub_10E50(int nSprite);
void sub_10EA0(void);

/////////// 2d editor /////////////

struct CONTROL {
    int at0;
    int at4;
    int at8; // tag?
    unsigned char type; // atc
    char* atd;
    int at11; // minvalue
    int at15; // maxvalue
    const char** names; // at19
    unsigned char (*at1d)(CONTROL* control, unsigned char key);
    int at21; // value
};

unsigned char sub_1BE80(CONTROL* control, unsigned char a2);
unsigned char sub_1BFF4(CONTROL* control, unsigned char a2);
unsigned char sub_1BFD8(CONTROL* control, unsigned char a2);

enum CONTROL_TYPE
{
    CONTROL_TYPE_0 = 0, // LABEL
    CONTROL_TYPE_1, // NUMBER
    CONTROL_TYPE_2, // TOGGLEBUTTON
    RADIOBUTTON, // 3 RADIOBUTTON
    CONTROL_TYPE_4, // LIST
    CONTROL_TYPE_5,
    CONTROL_END = 255
};

CONTROL controlXSprite[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-18.18s", 0, 1023, dword_D9A90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, dword_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, dword_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Send when:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "going ON:", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "going OFF:", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_1, "busyTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_1, "waitTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_4, "restState %1d: %-3.3s", 0, 1, dword_D9A88, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_2, "Impact", 0, 0, NULL, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "Pickup", 0, 0, NULL, NULL, 0 },
    { 30, 5, 15, CONTROL_TYPE_2, "Touch", 0, 0, NULL, NULL, 0 },
    { 30, 6, 16, CONTROL_TYPE_2, "Sight", 0, 0, NULL, NULL, 0 },
    { 30, 7, 17, CONTROL_TYPE_2, "Proximity", 0, 0, NULL, NULL, 0 },
    { 30, 8, 18, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 21, 9, 0, CONTROL_TYPE_0, "Launch 1 2 3 4 5 S B C T", 0, 0, NULL, NULL, 0 },
    { 28, 10, 19, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 30, 10, 20, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 10, 21, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 34, 10, 22, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 36, 10, 23, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 38, 10, 24, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 40, 10, 25, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 42, 10, 26, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 44, 10, 27, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 46, 0, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 46, 1, 28, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 46, 2, 29, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 46, 3, 30, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 46, 4, 31, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 46, 5, 32, CONTROL_TYPE_1, "Data1: %5d", 0, 65535, NULL, sub_1BFF4, 0 },
    { 46, 6, 33, CONTROL_TYPE_1, "Data2: %5d", 0, 65535, NULL, sub_1BFF4, 0 },
    { 46, 7, 34, CONTROL_TYPE_1, "Data3: %5d", 0, 65535, NULL, sub_1BFF4, 0 },
    { 46, 8, 35, CONTROL_TYPE_1, "Data4: %5d", 0, 65535, NULL, sub_1BFF4, 0 },
    { 46, 9, 36, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 46, 10, 37, CONTROL_TYPE_4, "Wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 62, 0, 0, CONTROL_TYPE_0, "Respawn:", 0, 0, NULL, NULL, 0 },
    { 62, 1, 38, CONTROL_TYPE_4, "When %1d: %-6.6s", 0, 3, dword_DCF00, NULL, 0 },
    { 62, 3, 0, CONTROL_TYPE_0, "Dude Flags:", 0, 0, NULL, NULL, 0 },
    { 62, 4, 39, CONTROL_TYPE_2, "dudeDeaf", 0, 0, NULL, NULL, 0 },
    { 62, 5, 40, CONTROL_TYPE_2, "dudeAmbush", 0, 0, NULL, NULL, 0 },
    { 62, 6, 41, CONTROL_TYPE_2, "dudeGuard", 0, 0, NULL, NULL, 0 },
    { 62, 7, 42, CONTROL_TYPE_2, "reserved", 0, 0, NULL, NULL, 0 },
    { 62, 9, 43, CONTROL_TYPE_1, "Lock msg: %3d", 0, 255, NULL, NULL, 0 },
    { 62, 10, 44, CONTROL_TYPE_1, "Drop item: %3d", 0, 255, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXWall[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-18.18s", 0, 1023, dword_DAA90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, dword_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, dword_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Send when:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "going ON:", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "going OFF:", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_1, "busyTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_1, "waitTime = %4d", 0, 4095, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_4, "restState %1d: %-3.3s", 0, 1, dword_D9A88, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_2, "Reserved", 0, 0, NULL, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 46, 0, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 46, 1, 15, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 46, 2, 16, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 46, 3, 17, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 46, 4, 18, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 46, 6, 19, CONTROL_TYPE_1, "Data: %5d", 0, 65535, NULL, NULL, 0 },
    { 46, 7, 20, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 46, 8, 21, CONTROL_TYPE_1, "panX = %4d", -128, 127, NULL, NULL, 0 },
    { 46, 9, 22, CONTROL_TYPE_1, "panY = %4d", -128, 127, NULL, NULL, 0 },
    { 46, 10, 23, CONTROL_TYPE_2, "panAlways", 0, 0, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXSector2[] = {
    { 0, 0, 0, CONTROL_TYPE_0, "Lighting:", 0, 0, NULL, NULL, 0 },
    { 0, 1, 1, CONTROL_TYPE_4, "Wave: %1d %-9.9s", 0, 15, WaveForm, NULL, 0 },
    { 0, 2, 2, CONTROL_TYPE_1, "Amplitude: %+4d", -128, 127, NULL, NULL, 0 },
    { 0, 3, 3, CONTROL_TYPE_1, "Freq:    %3d", 0, 255, NULL, NULL, 0 },
    { 0, 4, 4, CONTROL_TYPE_1, "Phase:     %3d", 0, 255, NULL, NULL, 0 },
    { 0, 5, 5, CONTROL_TYPE_2, "floor", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "ceiling", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "walls", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_2, "shadeAlways", 0, 0, NULL, NULL, 0 },
    { 20, 0, 0, CONTROL_TYPE_0, "More Lighting:", 0, 0, NULL, NULL, 0 },
    { 20, 1, 9, CONTROL_TYPE_2, "Color Lights", 0, 0, NULL, NULL, 0 },
    { 20, 2, 10, CONTROL_TYPE_1, "ceil  pal2 = %3d", 0, 15, NULL, NULL, 0 },
    { 20, 3, 11, CONTROL_TYPE_1, "floor pal2 = %3d", 0, 15, NULL, NULL, 0 },
    { 40, 0, 0, CONTROL_TYPE_0, "Motion FX:", 0, 0, NULL, NULL, 0 },
    { 40, 1, 12, CONTROL_TYPE_1, "Speed = %4d", 0, 255, NULL, NULL, 0 },
    { 40, 2, 13, CONTROL_TYPE_1, "Angle = %4d", 0, 2047, NULL, NULL, 0 },
    { 40, 3, 14, CONTROL_TYPE_2, "pan floor", 0, 0, NULL, NULL, 0 },
    { 40, 4, 15, CONTROL_TYPE_2, "pan ceiling", 0, 0, NULL, NULL, 0 },
    { 40, 5, 16, CONTROL_TYPE_2, "panAlways", 0, 0, NULL, NULL, 0 },
    { 40, 6, 17, CONTROL_TYPE_2, "drag", 0, 0, NULL, NULL, 0 },
    { 40, 8, 18, CONTROL_TYPE_1, "Wind vel: %4d", 0, 1023, NULL, NULL, 0 },
    { 40, 9, 19, CONTROL_TYPE_1, "Wind ang: %4d", 0, 2047, NULL, NULL, 0 },
    { 40, 10, 20, CONTROL_TYPE_2, "Wind always", 0, 0, NULL, NULL, 0 },
    { 60, 0, 0, CONTROL_TYPE_0, "Continuous motion:", 0, 0, NULL, NULL, 0 },
    { 60, 1, 21, CONTROL_TYPE_1, "Z range: %3d", 0, 31, NULL, NULL, 0 },
    { 60, 2, 22, CONTROL_TYPE_1, "Theta: %-4d", 0, 2047, NULL, NULL, 0 },
    { 60, 3, 23, CONTROL_TYPE_1, "Speed: %-4d", -2048, 2047, NULL, NULL, 0 },
    { 60, 4, 24, CONTROL_TYPE_2, "always", 0, 0, NULL, NULL, 0 },
    { 60, 5, 25, CONTROL_TYPE_2, "bob floor", 0, 0, NULL, NULL, 0 },
    { 60, 6, 26, CONTROL_TYPE_2, "bob ceiling", 0, 0, NULL, NULL, 0 },
    { 60, 7, 27, CONTROL_TYPE_2, "rotate", 0, 0, NULL, NULL, 0 },
    { 60, 9, 28, CONTROL_TYPE_1, "DamageType: %1d", 0, 5, NULL, NULL, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

CONTROL controlXSector[] = {
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-16.16s", 0, 1023, dword_DBA90, NULL, 0 },
    { 0, 1, 2, CONTROL_TYPE_1, "RX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 2, 3, CONTROL_TYPE_1, "TX ID: %4d", 0, 1023, NULL, sub_1BE80, 0 },
    { 0, 3, 4, CONTROL_TYPE_4, "State %1d: %-3.3s", 0, 1, dword_D9A88, NULL, 0 },
    { 0, 4, 5, CONTROL_TYPE_4, "Cmd: %3d: %-12.12s", 0, 255, dword_DCB00, NULL, 0 },
    { 0, 5, 0, CONTROL_TYPE_0, "Trigger Flags:", 0, 0, NULL, NULL, 0 },
    { 0, 6, 6, CONTROL_TYPE_2, "Decoupled", 0, 0, NULL, NULL, 0 },
    { 0, 7, 7, CONTROL_TYPE_2, "1-shot", 0, 0, NULL, NULL, 0 },
    { 0, 8, 8, CONTROL_TYPE_2, "Locked", 0, 0, NULL, NULL, 0 },
    { 0, 9, 9, CONTROL_TYPE_2, "Interruptable", 0, 0, NULL, NULL, 0 },
    { 0, 10, 10, CONTROL_TYPE_2, "DudeLockout", 0, 0, NULL, NULL, 0 },
    { 30, 0, 0, CONTROL_TYPE_0, "OFF->ON:", 0, 0, NULL, NULL, 0 },
    { 30, 1, 11, CONTROL_TYPE_2, "send at ON", 0, 0, NULL, NULL, 0 },
    { 30, 2, 12, CONTROL_TYPE_1, "busyTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 3, 13, CONTROL_TYPE_4, "wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 30, 4, 14, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 4, 15, CONTROL_TYPE_1, "waitTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 6, 0, CONTROL_TYPE_0, "ON->OFF:", 0, 0, NULL, NULL, 0 },
    { 30, 7, 16, CONTROL_TYPE_2, "send at OFF", 0, 0, NULL, NULL, 0 },
    { 30, 8, 17, CONTROL_TYPE_1, "busyTime = %3d", 0, 255, NULL, NULL, 0 },
    { 30, 9, 18, CONTROL_TYPE_4, "wave: %1d %-8.8s", 0, 7, WaveForm2, NULL, 0 },
    { 30, 10, 19, CONTROL_TYPE_2, "", 0, 0, NULL, NULL, 0 },
    { 32, 10, 20, CONTROL_TYPE_1, "waitTime = %3d", 0, 255, NULL, NULL, 0 },
    { 48, 0, 0, CONTROL_TYPE_0, "Trigger On:", 0, 0, NULL, NULL, 0 },
    { 48, 1, 21, CONTROL_TYPE_2, "Push", 0, 0, NULL, NULL, 0 },
    { 48, 2, 22, CONTROL_TYPE_2, "Vector", 0, 0, NULL, NULL, 0 },
    { 48, 3, 23, CONTROL_TYPE_2, "Reserved", 0, 0, NULL, NULL, 0 },
    { 48, 4, 24, CONTROL_TYPE_2, "Enter", 0, 0, NULL, NULL, 0 },
    { 48, 5, 25, CONTROL_TYPE_2, "Exit", 0, 0, NULL, NULL, 0 },
    { 48, 6, 26, CONTROL_TYPE_2, "WallPush", 0, 0, NULL, NULL, 0 },
    { 60, 0, 27, CONTROL_TYPE_1, "Data: %5d", 0, 65535, NULL, NULL, 0 },
    { 60, 1, 28, CONTROL_TYPE_1, "Key: %1d", 0, 7, NULL, NULL, 0 },
    { 60, 2, 29, CONTROL_TYPE_1, "Depth = %1d", 0, 7, NULL, NULL, 0 },
    { 60, 3, 30, CONTROL_TYPE_2, "Underwater", 0, 0, NULL, NULL, 0 },
    { 60, 4, 31, CONTROL_TYPE_2, "Crush", 0, 0, NULL, NULL, 0 },
    { 60 ,10, 32, CONTROL_TYPE_5, "FX...", 0, 0, NULL, sub_1BFD8, 0 },
    { 0, 0, 0, CONTROL_END, NULL, 0, 0, NULL, NULL, 0 },
};

void PrintText(int x, int y, short c, short bc, char *pString)
{
    printext16(x*8+4, ydim-STATUS2DSIZ+y*8+28, c, bc, pString, 0);
}

void ControlPrint(CONTROL* control, int a2)
{
    int vsi, vdi;
    vsi = 11;
    vdi = 8;
    if (a2)
    {
        vsi = 14;
        vdi = 0;
    }
    // Label?
    if (control->at8 == 0)
    {
        vsi = 15;
        vdi = 2;
    }
    switch (control->type)
    {
    case CONTROL_TYPE_0:
    case CONTROL_TYPE_5:
        PrintText(control->at0, control->at4, vsi, vdi, control->atd);
        break;
    case CONTROL_TYPE_1:
        sprintf(gTempBuf, control->atd, control->at21);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case CONTROL_TYPE_4:
        dassert(control->names != NULL);
        sprintf(gTempBuf, control->atd, control->at21, control->names[control->at21]);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case CONTROL_TYPE_2:
        sprintf(gTempBuf, "%c %s", 2 + (control->at21 != 0), control->atd);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    case RADIOBUTTON:
        sprintf(gTempBuf, "%c %s", 4 + (control->at21 != 0), control->atd);
        PrintText(control->at0, control->at4, vsi, vdi, gTempBuf);
        break;
    }
}

void ControlPrintList(CONTROL* control)
{
    clearmidstatbar16();
    while (control->type != CONTROL_END)
    {
        ControlPrint(control++, 0);
    }
}

CONTROL *ControlFindTag(CONTROL* control, int tag)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END);

    if (control->type == RADIOBUTTON)
    {
        while (control->at21 == 0)
        {
            control++;
            dassert(control->type != CONTROL_END);
            dassert(control->type == RADIOBUTTON);
        }
    }
    return control;
}

void ControlSet(CONTROL* control, int tag, int value)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END);

    control->at21 = value;
}

int ControlRead(CONTROL* control, int tag)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END);

    return control->at21;
}

void sub_1B988(CONTROL* control, int tag, int value)
{
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END);

    // ???
    while (control->type != CONTROL_END && control->at8 == tag)
    {
        control->at21 = value == 0;
        value--;
    }
    dassert(value < 0);
}

int sub_1B9F4(CONTROL* control, int tag)
{
    int value = 0;
    for (; control->type != CONTROL_END && control->at8 != tag; control++) {};

    dassert(control->type != CONTROL_END);

    // ???
    while (control->type != CONTROL_END)
    {
        dassert(control->type == RADIOBUTTON);
        if (control->at21 == 0)
            break;
        value++;
    }

    dassert(control->type != CONTROL_END);

    return value;
}

int ControlKeys(CONTROL* list)
{
    keyFlushScans();
    ControlPrintList(list);
    int tags = 0;
    int tag = 1;
    for (CONTROL* control = list; control->type != CONTROL_END; control++)
    {
        if (control->at8 > tags)
            tags = control->at8;
    }
    CONTROL* control = ControlFindTag(list, tag);
    while (true)
    {
        if (handleevents())
        {
            if (quitevent)
                return 0;
        }
        ControlPrint(control, 1);
        unsigned char key = keyGetScan();
        if (key == 0)
        {
            showframe();
            continue;
        }
        if (control->at1d)
        {
            key = control->at1d(control, key);
            ControlPrintList(list);
        }
        switch (key)
        {
        case sc_Return:
        case sc_kpad_Enter:
            keystatus[key] = 0;
            ModifyBeep();
            return 1;
        case sc_Escape:
            keystatus[key] = 0;
            return 0;
        case sc_LeftArrow:
            tag--;
            if (tag == 0)
                tag = tags;
            ControlPrint(control, 0);
            control = ControlFindTag(list, tag);
            break;
        case sc_RightArrow:
            tag++;
            if (tag > tags)
                tag = 1;
            ControlPrint(control, 0);
            control = ControlFindTag(list, tag);
            break;
        case sc_Tab:
            if (keystatus[sc_LeftShift] || keystatus[sc_RightShift])
            {
                tag--;
                if (tag == 0)
                    tag = tags;
                ControlPrint(control, 0);
                control = ControlFindTag(list, tag);
            }
            else
            {
                tag++;
                if (tag > tags)
                    tag = 1;
                ControlPrint(control, 0);
                control = ControlFindTag(list, tag);
            }
            break;
        default:
            switch (control->type)
            {
            case CONTROL_TYPE_1:
                switch (key)
                {
                case sc_BackSpace:
                    control->at21 /= 10;
                    break;
                case sc_UpArrow:
                case sc_kpad_Plus:
                    control->at21++;
                    break;
                case sc_DownArrow:
                case sc_kpad_Minus:
                    control->at21--;
                    break;
                case sc_PgUp:
                    control->at21 = IncBy(control->at21, 10);
                    break;
                case sc_PgDn:
                    control->at21 = DecBy(control->at21, 10);
                    break;
                default:
                    key = g_keyAsciiTable[key];
                    if (key >= '0' && key <= '9')
                        control->at21 = control->at21 * 10 + key - '0';
                }
                if (control->at21 > control->at15)
                    control->at21 = control->at15;

                if (control->at21 < control->at11)
                    control->at21 = control->at11;

                break;
            case CONTROL_TYPE_2:
                switch (key)
                {
                case sc_Space:
                    control->at21 = !control->at21;
                    break;
                }
                break;
            case RADIOBUTTON:
                ControlPrint(control, 0);
                switch (key)
                {
                case sc_UpArrow:
                    control->at21 = 0;
                    ControlPrint(control, 0);
                    control--;
                    if (control < list || control->at8 != tag)
                        control--;
                    control->at21 = 1;
                    break;
                case sc_DownArrow:
                    control->at21 = 0;
                    ControlPrint(control, 0);
                    control++;
                    if (control->at8 != tag)
                        control--;
                    control->at21 = 1;
                    break;
                }
                break;
            case CONTROL_TYPE_4:
            {
                int t = control->at21;
                switch (key)
                {
                case sc_UpArrow:
                case sc_kpad_Plus:
                    do
                    {
                        t++;
                    } while (t <= control->at15 && !control->names[t]);
                    break;
                case sc_DownArrow:
                case sc_kpad_Minus:
                    do
                    {
                        t--;
                    } while (t >= control->at11 && !control->names[t]);
                    break;
                case sc_PgUp:
                    do
                    {
                        t = IncBy(t, 10);
                    } while (t <= control->at15 && !control->names[t]);
                    break;
                case sc_PgDn:
                    do
                    {
                        t = DecBy(t, 10);
                    } while (t >= control->at11 && !control->names[t]);
                    break;
                default:
                    key = g_keyAsciiTable[key];
                    if (key >= '0' && key <= '9')
                        control->at21 = control->at21 * 10 + key - '0';
                }

                if (t >= control->at11 && t <= control->at15 && control->names[t])
                    control->at21 = t;

                break;
            }

            }
        }
        nextpage();
    }
}

unsigned char sub_1BE80(CONTROL* control, unsigned char a2) // Next tx/rx id
{
    char t[1024];
    if (a2 == sc_F10)
    {
        memset(t, 0, sizeof(t));
        int i;
        for (i = 0; i < numsectors; i++)
        {
            int nXSector = sector[i].extra;
            if (nXSector <= 0)
                continue;
            t[xsector[nXSector].txID] = 1;
            t[xsector[nXSector].rxID] = 1;
        }
        for (i = 0; i < numwalls; i++)
        {
            int nXWall = wall[i].extra;
            if (nXWall <= 0)
                continue;
            t[xwall[nXWall].txID] = 1;
            t[xwall[nXWall].rxID] = 1;
        }
        for (i = 0; i < kMaxSprites; i++)
        {
            if (sprite[i].statnum >= kMaxStatus)
                continue;
            int nXSprite = sprite[i].extra;
            if (nXSprite <= 0)
                continue;
            t[xsprite[nXSprite].txID] = 1;
            t[xsprite[nXSprite].rxID] = 1;
        }
        for (i = 100; i < 1024; i++)
        {
            if (!t[i])
                break;
        }
        control->at21 = i;
        return 0;
    }
    return a2;
}

unsigned char sub_1BFD8(CONTROL* control, unsigned char a2)
{
    if (a2 == sc_Return)
    {
        if (ControlKeys(controlXSector2))
            return a2;
        return 0;
    }
    return a2;
}


unsigned char sub_1BFF4(CONTROL* control, unsigned char a2) // sound
{
    int i;
    DICTNODE* hSnd;
    switch (a2)
    {
    case sc_UpArrow:
        for (i = control->at21+1; i < 65535; i++)
        {
            if (gSoundRes.Lookup(i, "SFX") != NULL)
            {
                control->at21 = i;
                break;
            }
        }
        if (i == 65535)
            Beep();
        return 0;
    case sc_DownArrow:
        for (i = control->at21-1; i > 0; i--)
        {
            if (gSoundRes.Lookup(i, "SFX") != NULL)
            {
                control->at21 = i;
                break;
            }
        }
        return 0;
    case sc_F10:
        hSnd = gSoundRes.Lookup(control->at21, "SFX");
        if (hSnd != NULL)
        {
            SFX* pSfx = (SFX*)gSoundRes.Load(hSnd);
            printmessage16(pSfx->rawName);
            sndStartSample(control->at21, FXVolume, 0, 0);
        }
        return 0;
    }
    return a2;
}

void XWallControlSet(int nWall)
{
    int nXWall = wall[nWall].extra;
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    XWALL* pXWall = &xwall[nXWall];
    ControlSet(controlXWall, 1, wall[nWall].lotag); // type
    ControlSet(controlXWall, 2, pXWall->rxID); // rx id
    ControlSet(controlXWall, 3, pXWall->txID); // tx id
    ControlSet(controlXWall, 4, pXWall->state); // state
    ControlSet(controlXWall, 5, pXWall->command); // cmd
    ControlSet(controlXWall, 6, pXWall->triggerOn); // going on
    ControlSet(controlXWall, 7, pXWall->triggerOff); // going off
    ControlSet(controlXWall, 8, pXWall->busyTime); // busyTime
    ControlSet(controlXWall, 9, pXWall->waitTime); // waitTime
    ControlSet(controlXWall, 10, pXWall->restState); // restState
    ControlSet(controlXWall, 11, pXWall->triggerPush); // Push
    ControlSet(controlXWall, 12, pXWall->triggerVector); // Vector
    ControlSet(controlXWall, 13, pXWall->triggerTouch); // Reserved
    ControlSet(controlXWall, 14, pXWall->dudeLockout); // DudeLockout
    ControlSet(controlXWall, 15, pXWall->decoupled); // Decoupled
    ControlSet(controlXWall, 16, pXWall->triggerOnce); // 1-Shot
    ControlSet(controlXWall, 17, pXWall->locked); // Locked
    ControlSet(controlXWall, 18, pXWall->interruptable); // Interruptable
    ControlSet(controlXWall, 19, pXWall->data); // Data
    ControlSet(controlXWall, 20, pXWall->key); // Key
    ControlSet(controlXWall, 21, pXWall->panXVel); // panX
    ControlSet(controlXWall, 22, pXWall->panYVel); // panY
    ControlSet(controlXWall, 23, pXWall->panAlways); // panAlways
}

void XWallControlRead(int nWall)
{
    int nXWall = wall[nWall].extra;
    dassert(nXWall > 0 && nXWall < kMaxXWalls);
    XWALL* pXWall = &xwall[nXWall];
    wall[nWall].lotag = ControlRead(controlXWall, 1); // type
    pXWall->rxID = ControlRead(controlXWall, 2); // rx id
    pXWall->txID = ControlRead(controlXWall, 3); // tx id
    pXWall->state = ControlRead(controlXWall, 4); // state
    pXWall->command = ControlRead(controlXWall, 5); // cmd
    pXWall->triggerOn = ControlRead(controlXWall, 6); // going on
    pXWall->triggerOff = ControlRead(controlXWall, 7); // going off
    pXWall->busyTime = ControlRead(controlXWall, 8); // busyTime
    pXWall->waitTime = ControlRead(controlXWall, 9); // waitTime
    pXWall->restState = ControlRead(controlXWall, 10); // restState
    pXWall->triggerPush = ControlRead(controlXWall, 11); // Push
    pXWall->triggerVector = ControlRead(controlXWall, 12); // Vector
    pXWall->triggerTouch = ControlRead(controlXWall, 13); // Reserved
    pXWall->dudeLockout = ControlRead(controlXWall, 14); // DudeLockout
    pXWall->decoupled = ControlRead(controlXWall, 15); // Decoupled
    pXWall->triggerOnce = ControlRead(controlXWall, 16); // 1-Shot
    pXWall->locked = ControlRead(controlXWall, 17); // Locked
    pXWall->interruptable = ControlRead(controlXWall, 18); // Interruptable
    pXWall->data = ControlRead(controlXWall, 19); // Data
    pXWall->key = ControlRead(controlXWall, 20); // Key
    pXWall->panXVel = ControlRead(controlXWall, 21); // panX
    pXWall->panYVel = ControlRead(controlXWall, 22); // panY
    pXWall->panAlways = ControlRead(controlXWall, 23); // panAlways
}

void XSpriteControlSet(int nSprite)
{
    int nXSprite = sprite[nSprite].extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE* pXSprite = &xsprite[nXSprite];
    ControlSet(controlXSprite, 1, sprite[nSprite].type); // type
    ControlSet(controlXSprite, 2, pXSprite->rxID); // rx id
    ControlSet(controlXSprite, 3, pXSprite->txID); // tx id
    ControlSet(controlXSprite, 4, pXSprite->state); // state
    ControlSet(controlXSprite, 5, pXSprite->command); // cmd
    ControlSet(controlXSprite, 6, pXSprite->triggerOn); // going on
    ControlSet(controlXSprite, 7, pXSprite->triggerOff); // going off
    ControlSet(controlXSprite, 8, pXSprite->busyTime); // busyTime
    ControlSet(controlXSprite, 9, pXSprite->waitTime); // waitTime
    ControlSet(controlXSprite, 10, pXSprite->restState); // restState
    ControlSet(controlXSprite, 11, pXSprite->Push); // Push
    ControlSet(controlXSprite, 12, pXSprite->Vector); // Vector
    ControlSet(controlXSprite, 13, pXSprite->Impact); // Impact
    ControlSet(controlXSprite, 14, pXSprite->Pickup); // Pickup
    ControlSet(controlXSprite, 15, pXSprite->Touch); // Touch
    ControlSet(controlXSprite, 16, pXSprite->Sight); // Sight
    ControlSet(controlXSprite, 17, pXSprite->Proximity); // Proximity
    ControlSet(controlXSprite, 18, pXSprite->DudeLockout); // DudeLockout
    ControlSet(controlXSprite, 19, !((pXSprite->lSkill>>0)&1)); // Launch 1
    ControlSet(controlXSprite, 20, !((pXSprite->lSkill>>1)&1)); // Launch 2
    ControlSet(controlXSprite, 21, !((pXSprite->lSkill>>2)&1)); // Launch 3
    ControlSet(controlXSprite, 22, !((pXSprite->lSkill>>3)&1)); // Launch 4
    ControlSet(controlXSprite, 23, !((pXSprite->lSkill>>4)&1)); // Launch 5
    ControlSet(controlXSprite, 24, !pXSprite->lS); // Launch S
    ControlSet(controlXSprite, 25, !pXSprite->lB); // Launch B
    ControlSet(controlXSprite, 26, !pXSprite->lC); // Launch C
    ControlSet(controlXSprite, 27, !pXSprite->lT); // Launch T
    ControlSet(controlXSprite, 28, pXSprite->Decoupled); // Decoupled
    ControlSet(controlXSprite, 29, pXSprite->triggerOnce); // 1-shot
    ControlSet(controlXSprite, 30, pXSprite->locked); // Locked
    ControlSet(controlXSprite, 31, pXSprite->Interrutable); // Interruptable
    ControlSet(controlXSprite, 32, pXSprite->data1); // Data 1
    ControlSet(controlXSprite, 33, pXSprite->data2); // Data 2
    ControlSet(controlXSprite, 34, pXSprite->data3); // Data 3
    ControlSet(controlXSprite, 35, pXSprite->data4); // Data 4
    ControlSet(controlXSprite, 36, pXSprite->key); // Key
    ControlSet(controlXSprite, 37, pXSprite->wave); // Wave
    ControlSet(controlXSprite, 38, pXSprite->respawn); // Respawn option
    ControlSet(controlXSprite, 39, pXSprite->dudeDeaf); // dudeDeaf
    ControlSet(controlXSprite, 40, pXSprite->dudeAmbush); // dudeAmbush
    ControlSet(controlXSprite, 41, pXSprite->dudeGuard); // dudeGuard
    ControlSet(controlXSprite, 42, pXSprite->dudeFlag4); // reserved
    ControlSet(controlXSprite, 43, pXSprite->lockMsg); // Lock msg
    ControlSet(controlXSprite, 44, pXSprite->dropMsg); // Drop item
}

void XSpriteControlRead(int nSprite)
{
    int nXSprite = sprite[nSprite].extra;
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    XSPRITE* pXSprite = &xsprite[nXSprite];
    sprite[nSprite].type = ControlRead(controlXSprite, 1); // type
    pXSprite->rxID = ControlRead(controlXSprite, 2); // rx id
    pXSprite->txID = ControlRead(controlXSprite, 3); // tx id
    pXSprite->state = ControlRead(controlXSprite, 4); // state
    pXSprite->command = ControlRead(controlXSprite, 5); // cmd
    pXSprite->triggerOn = ControlRead(controlXSprite, 6); // going on
    pXSprite->triggerOff = ControlRead(controlXSprite, 7); // going off
    pXSprite->busyTime = ControlRead(controlXSprite, 8); // busyTime
    pXSprite->waitTime = ControlRead(controlXSprite, 9); // waitTime
    pXSprite->restState = ControlRead(controlXSprite, 10); // restState
    pXSprite->Push = ControlRead(controlXSprite, 11); // Push
    pXSprite->Vector = ControlRead(controlXSprite, 12); // Vector
    pXSprite->Impact = ControlRead(controlXSprite, 13); // Impact
    pXSprite->Pickup = ControlRead(controlXSprite, 14); // Pickup
    pXSprite->Touch = ControlRead(controlXSprite, 15); // Touch
    pXSprite->Sight = ControlRead(controlXSprite, 16); // Sight
    pXSprite->Proximity = ControlRead(controlXSprite, 17); // Proximity
    pXSprite->DudeLockout = ControlRead(controlXSprite, 18); // DudeLockout
    pXSprite->lSkill = (!ControlRead(controlXSprite, 19) << 0) // Launch 12345
                     | (!ControlRead(controlXSprite, 20) << 1)
                     | (!ControlRead(controlXSprite, 21) << 2)
                     | (!ControlRead(controlXSprite, 22) << 3)
                     | (!ControlRead(controlXSprite, 23) << 4);
    pXSprite->lS = !ControlRead(controlXSprite, 24); // Launch S
    pXSprite->lB = !ControlRead(controlXSprite, 25); // Launch B
    pXSprite->lC = !ControlRead(controlXSprite, 26); // Launch C
    pXSprite->lT = !ControlRead(controlXSprite, 27); // Launch T
    pXSprite->Decoupled = ControlRead(controlXSprite, 28); // Decoupled
    pXSprite->triggerOnce = ControlRead(controlXSprite, 29); // 1-shot
    pXSprite->locked = ControlRead(controlXSprite, 30); // Locked
    pXSprite->Interrutable = ControlRead(controlXSprite, 31); // Interruptable
    pXSprite->data1 = ControlRead(controlXSprite, 32); // Data 1
    pXSprite->data2 = ControlRead(controlXSprite, 33); // Data 2
    pXSprite->data3 = ControlRead(controlXSprite, 34); // Data 3
    pXSprite->data4 = ControlRead(controlXSprite, 35); // Data 4
    pXSprite->key = ControlRead(controlXSprite, 36); // Key
    pXSprite->wave = ControlRead(controlXSprite, 37); // Wave
    pXSprite->respawn = ControlRead(controlXSprite, 38); // Respawn option
    pXSprite->dudeDeaf = ControlRead(controlXSprite, 39); // dudeDeaf
    pXSprite->dudeAmbush = ControlRead(controlXSprite, 40); // dudeAmbush
    pXSprite->dudeGuard = ControlRead(controlXSprite, 41); // dudeGuard
    pXSprite->dudeFlag4 = ControlRead(controlXSprite, 42); // reserved
    pXSprite->lockMsg = ControlRead(controlXSprite, 43); // Lock msg
    pXSprite->dropMsg = ControlRead(controlXSprite, 44); // Drop item
}

void XSectorControlSet(int nSector)
{
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR* pXSector = &xsector[nXSector];
    ControlSet(controlXSector, 1, sector[nSector].type); // type
    ControlSet(controlXSector, 2, pXSector->rxID); // rx id
    ControlSet(controlXSector, 3, pXSector->txID); // tx id
    ControlSet(controlXSector, 4, pXSector->state); // state
    ControlSet(controlXSector, 5, pXSector->command); // cmd
    ControlSet(controlXSector, 6, pXSector->decoupled); // Decoupled
    ControlSet(controlXSector, 7, pXSector->triggerOnce); // 1-shot
    ControlSet(controlXSector, 8, pXSector->locked); // Locked
    ControlSet(controlXSector, 9, pXSector->interruptable); // Interruptable
    ControlSet(controlXSector, 10, pXSector->dudeLockout); // DudeLockout
    ControlSet(controlXSector, 11, pXSector->triggerOn); // Send at ON
    ControlSet(controlXSector, 12, pXSector->busyTimeA); // OFF->ON busyTime
    ControlSet(controlXSector, 13, pXSector->busyWaveA); // OFF->ON wave
    ControlSet(controlXSector, 14, pXSector->reTriggerA); // OFF->ON wait
    ControlSet(controlXSector, 15, pXSector->waitTimeA); // OFF->ON waitTime
    ControlSet(controlXSector, 16, pXSector->triggerOff); // Send at OFF
    ControlSet(controlXSector, 17, pXSector->busyTimeB); // ON->OFF busyTime
    ControlSet(controlXSector, 18, pXSector->busyWaveB); // ON->OFF wave
    ControlSet(controlXSector, 19, pXSector->reTriggerB); // ON->OFF wait
    ControlSet(controlXSector, 20, pXSector->waitTimeB); // ON->OFF waitTime
    ControlSet(controlXSector, 21, pXSector->Push); // Push
    ControlSet(controlXSector, 22, pXSector->Vector); // Vector
    ControlSet(controlXSector, 23, pXSector->Reserved); // Reserved
    ControlSet(controlXSector, 24, pXSector->Enter); // Enter
    ControlSet(controlXSector, 25, pXSector->Exit); // Exit
    ControlSet(controlXSector, 26, pXSector->Wallpush); // WallPush
    ControlSet(controlXSector, 27, pXSector->data); // Data
    ControlSet(controlXSector, 28, pXSector->Key); // Key
    ControlSet(controlXSector, 29, pXSector->Depth); // Depth
    ControlSet(controlXSector, 30, pXSector->Underwater); // Underwater
    ControlSet(controlXSector, 31, pXSector->Crush); // Crush
    ControlSet(controlXSector2, 1, pXSector->wave); // Lighting wave
    ControlSet(controlXSector2, 2, pXSector->amplitude); // Lighting amplitude
    ControlSet(controlXSector2, 3, pXSector->freq); // Lighting freq
    ControlSet(controlXSector2, 4, pXSector->phase); // Lighting phase
    ControlSet(controlXSector2, 5, pXSector->shadeFloor); // Lighting floor
    ControlSet(controlXSector2, 6, pXSector->shadeCeiling); // Lighting ceiling
    ControlSet(controlXSector2, 7, pXSector->shadeWalls); // Lighting walls
    ControlSet(controlXSector2, 8, pXSector->shadeAlways); // Lighting shadeAlways
    ControlSet(controlXSector2, 9, pXSector->color); // Color Lights
    ControlSet(controlXSector2, 10, pXSector->ceilpal); // Ceil pal2
    ControlSet(controlXSector2, 11, pXSector->floorpal); // floor pal2
    ControlSet(controlXSector2, 12, pXSector->panVel); // Motion speed
    ControlSet(controlXSector2, 13, pXSector->panAngle); // Motion angle
    ControlSet(controlXSector2, 14, pXSector->panFloor); // Pan floor
    ControlSet(controlXSector2, 15, pXSector->panCeiling); // Pan ceiling
    ControlSet(controlXSector2, 16, pXSector->panAlways); // Pan always
    ControlSet(controlXSector2, 17, pXSector->Drag); // Pan drag
    ControlSet(controlXSector2, 18, pXSector->windVel); // Wind vel
    ControlSet(controlXSector2, 19, pXSector->windAng); // Wind ang
    ControlSet(controlXSector2, 20, pXSector->windAlways); // Wind always
    ControlSet(controlXSector2, 21, pXSector->bobZRange); // Motion Z range
    ControlSet(controlXSector2, 22, pXSector->bobTheta); // Motion Theta
    ControlSet(controlXSector2, 23, pXSector->bobSpeed); // Motion speed
    ControlSet(controlXSector2, 24, pXSector->bobAlways); // Motion always
    ControlSet(controlXSector2, 25, pXSector->bobFloor); // Motion bob floor
    ControlSet(controlXSector2, 26, pXSector->bobCeiling); // Motion bob ceiling
    ControlSet(controlXSector2, 27, pXSector->bobRotate); // Motion rotate
    ControlSet(controlXSector2, 28, pXSector->damageType); // DamageType
}

void XSectorControlRead(int nSector)
{
    int nXSector = sector[nSector].extra;
    dassert(nXSector > 0 && nXSector < kMaxXSectors);
    XSECTOR* pXSector = &xsector[nXSector];
    sector[nSector].type = ControlRead(controlXSector, 1); // type
    pXSector->rxID = ControlRead(controlXSector, 2); // rx id
    pXSector->txID = ControlRead(controlXSector, 3); // tx id
    pXSector->state = ControlRead(controlXSector, 4); // state
    pXSector->command = ControlRead(controlXSector, 5); // cmd
    pXSector->decoupled = ControlRead(controlXSector, 6); // Decoupled
    pXSector->triggerOnce = ControlRead(controlXSector, 7); // 1-shot
    pXSector->locked = ControlRead(controlXSector, 8); // Locked
    pXSector->interruptable = ControlRead(controlXSector, 9); // Interruptable
    pXSector->dudeLockout = ControlRead(controlXSector, 10); // DudeLockout
    pXSector->triggerOn = ControlRead(controlXSector, 11); // Send at ON
    pXSector->busyTimeA = ControlRead(controlXSector, 12); // OFF->ON busyTime
    pXSector->busyWaveA = ControlRead(controlXSector, 13); // OFF->ON wave
    pXSector->reTriggerA = ControlRead(controlXSector, 14); // OFF->ON wait
    pXSector->waitTimeA = ControlRead(controlXSector, 15); // OFF->ON waitTime
    pXSector->triggerOff = ControlRead(controlXSector, 16); // Send at OFF
    pXSector->busyTimeB = ControlRead(controlXSector, 17); // ON->OFF busyTime
    pXSector->busyWaveB = ControlRead(controlXSector, 18); // ON->OFF wave
    pXSector->reTriggerB = ControlRead(controlXSector, 19); // ON->OFF wait
    pXSector->waitTimeB = ControlRead(controlXSector, 20); // ON->OFF waitTime
    pXSector->Push = ControlRead(controlXSector, 21); // Push
    pXSector->Vector = ControlRead(controlXSector, 22); // Vector
    pXSector->Reserved = ControlRead(controlXSector, 23); // Reserved
    pXSector->Enter = ControlRead(controlXSector, 24); // Enter
    pXSector->Exit = ControlRead(controlXSector, 25); // Exit
    pXSector->Wallpush = ControlRead(controlXSector, 26); // WallPush
    pXSector->data = ControlRead(controlXSector, 27); // Data
    pXSector->Key = ControlRead(controlXSector, 28); // Key
    pXSector->Depth = ControlRead(controlXSector, 29); // Depth
    pXSector->Underwater = ControlRead(controlXSector, 30); // Underwater
    pXSector->Crush = ControlRead(controlXSector, 31); // Crush
    pXSector->wave = ControlRead(controlXSector2, 1); // Lighting wave
    pXSector->amplitude = ControlRead(controlXSector2, 2); // Lighting amplitude
    pXSector->freq = ControlRead(controlXSector2, 3); // Lighting freq
    pXSector->phase = ControlRead(controlXSector2, 4); // Lighting phase
    pXSector->shadeFloor = ControlRead(controlXSector2, 5); // Lighting floor
    pXSector->shadeCeiling = ControlRead(controlXSector2, 6); // Lighting ceiling
    pXSector->shadeWalls = ControlRead(controlXSector2, 7); // Lighting walls
    pXSector->shadeAlways = ControlRead(controlXSector2, 8); // Lighting shadeAlways
    pXSector->color = ControlRead(controlXSector2, 9); // Color Lights
    pXSector->ceilpal = ControlRead(controlXSector2, 10); // Ceil pal2
    pXSector->floorpal = ControlRead(controlXSector2, 11); // floor pal2
    pXSector->panVel = ControlRead(controlXSector2, 12); // Motion speed
    pXSector->panAngle = ControlRead(controlXSector2, 13); // Motion angle
    pXSector->panFloor = ControlRead(controlXSector2, 14); // Pan floor
    pXSector->panCeiling = ControlRead(controlXSector2, 15); // Pan ceiling
    pXSector->panAlways = ControlRead(controlXSector2, 16); // Pan always
    pXSector->Drag = ControlRead(controlXSector2, 17); // Pan drag
    pXSector->windVel = ControlRead(controlXSector2, 18); // Wind vel
    pXSector->windAng = ControlRead(controlXSector2, 19); // Wind ang
    pXSector->windAlways = ControlRead(controlXSector2, 20); // Wind always
    pXSector->bobZRange = ControlRead(controlXSector2, 21); // Motion Z range
    pXSector->bobTheta = ControlRead(controlXSector2, 22); // Motion Theta
    pXSector->bobSpeed = ControlRead(controlXSector2, 23); // Motion speed
    pXSector->bobAlways = ControlRead(controlXSector2, 24); // Motion always
    pXSector->bobFloor = ControlRead(controlXSector2, 25); // Motion bob floor
    pXSector->bobCeiling = ControlRead(controlXSector2, 26); // Motion bob ceiling
    pXSector->bobRotate = ControlRead(controlXSector2, 27); // Motion rotate
    pXSector->damageType = ControlRead(controlXSector2, 28); // DamageType
}

int qgetpointhighlight(int x, int y) // Replace
{
    int nMinDist = divscale(gHighlightThreshold, gZoom, 14);
    int nStat = -1;
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            int nDist = approxDist(x-sprite[i].x, y-sprite[i].y);
            if (nDist < nMinDist)
            {
                nMinDist = nDist;
                nStat = i | 0x4000;
            }
        }
    }

    for (int i = 0; i < numwalls; i++)
    {
        int nDist = approxDist(x-wall[i].x, y-wall[i].y);
        if (nDist < nMinDist)
        {
            nMinDist = nDist;
            nStat = i;
        }
    }

    return nStat;
}

void ShowSectorData(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sprintf(gTempBuf, "Sector %d", nSector);
    printmessage16(gTempBuf);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        dassert(nXSector < kMaxXSectors);
        XSectorControlSet(nSector);
        ControlPrintList(controlXSector);
    }
    else
        clearmidstatbar16();
}

void EditSectorData(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sub_1058C();
    sprintf(gTempBuf, "Sector %d", nSector);
    printmessage16(gTempBuf);
    sub_10DBC(nSector);
    XSectorControlSet(nSector);
    if (ControlKeys(controlXSector))
        XSectorControlRead(nSector);
    ShowSectorData(nSector);
    angvel = 0;
    svel = 0;
    vel = 0;
    sub_1058C();
}

void ShowWallData(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls);
    int nLen = approxDist(wall[wall[nWall].point2].x-wall[nWall].x, wall[wall[nWall].point2].y-wall[nWall].y);
    sprintf(gTempBuf, "Wall %d:  Length = %d", nWall, nLen);
    printmessage16(gTempBuf);
    int nXWall = wall[nWall].extra;
    if (nXWall > 0)
    {
        XWallControlSet(nWall);
        ControlPrintList(controlXWall);
    }
    else
        clearmidstatbar16();
}

void EditWallData(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls);
    sub_1058C();
    sprintf(gTempBuf, "Wall %d", nWall);
    printmessage16(gTempBuf);
    sub_10E08(nWall);
    XWallControlSet(nWall);
    if (ControlKeys(controlXWall))
        XWallControlRead(nWall);
    ShowWallData(nWall);
    angvel = 0;
    svel = 0;
    vel = 0;
    sub_1058C();
}

void ShowSpriteData(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    if (sprite[nSprite].extra > 0)
    {
        int nXSprite = sprite[nSprite].extra;
        sprintf(gTempBuf, "Sprite %d  Extra %d XRef %d", nSprite, nXSprite, xsprite[nXSprite].reference);
    }
    else
        sprintf(gTempBuf, "Sprite %d", nSprite);
    int nXSprite = sprite[nSprite].extra;
    if (nXSprite > 0)
    {
        dassert(nXSprite < kMaxXSprites);
        XSpriteControlSet(nSprite);
        ControlPrintList(controlXSprite);
    }
    else
        clearmidstatbar16();
}

void EditSpriteData(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sub_1058C();
    sprintf(gTempBuf, "Sprite %d", nSprite);
    printmessage16(gTempBuf);
    sub_10E50(nSprite);
    XSpriteControlSet(nSprite);
    if (ControlKeys(controlXSprite))
        XSpriteControlRead(nSprite);
    ShowSpriteData(nSprite);
    angvel = 0;
    svel = 0;
    vel = 0;
    sub_1058C();
    sub_10EA0();
}

int qgetlinehighlight(int x, int y) // Replace
{
    int nMinDist = divscale(gHighlightThreshold, gZoom, 14);
    int nStat = -1;
    for (int i = 0; i < numwalls; i++)
    {
        int j = wall[i].point2;
        int dx1 = x - wall[i].x;
        int dy1 = y - wall[i].y;
        int dx2 = wall[j].x - wall[i].x;
        int dy2 = wall[j].y - wall[i].y;
        if (dx1 * dy2 > dx2 * dy1) // inside
            continue;

        int dot = dmulscale(dx1, dx2, dy1, dy2, 4);
        int den = dmulscale(dx2, dx2, dy2, dy2, 4);
        if (dot > 0 && den > dot)
        {
            dassert(den > 0);
            int wx = wall[i].x+scale(dx2, dot, den);
            int wy = wall[i].y+scale(dy2, dot, den);
            int nDist = approxDist(x-wx, y-wy);
            if (nDist < nMinDist)
            {
                nMinDist = nDist;
                nStat = i;
            }
        }
    }
    return nStat;
}

void DrawLine(int x1, int y1, int x2, int y2, char c, int a6) // DrawLine
{
    if ((x1 >= 0 || x2 >= 0) && (x1 < xdim || x2 < xdim) && (y1 >= 0 || y2 >= 0) && (y1 < ydim16 || y2 < ydim16))
    {
        drawline16(x1, y1, x2, y2, c);
        if (a6)
        {
            int dx = x2 - x1;
            int dy = y2 - y1;
            RotateVector(&dx, &dy, 128);
            switch (GetOctant(dx, dy))
            {
            case 0:
            case 4:
                drawline16(x1, y1-1, x2, y2-1, c);
                drawline16(x1, y1+1, x2, y2+1, c);
                break;
            case 1:
            case 5:
                if (klabs(dx) < klabs(dy))
                {
                    drawline16(x1-1, y1+1, x2-1, y2+1, c);
                    drawline16(x1-1, y1, x2, y2+1, c);
                    drawline16(x1+1, y1, x2+1, y2, c);
                }
                else
                {
                    drawline16(x1-1, y1+1, x2-1, y2+1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                }
                break;
            case 2:
            case 6:
                drawline16(x1-1, y1, x2-1, y2, c);
                drawline16(x1+1, y1, x2+1, y2, c);
                break;
            case 3:
            case 7:
                if (klabs(dx) < klabs(dy))
                {
                    drawline16(x1-1, y1-1, x2-1, y2-1, c);
                    drawline16(x1-1, y1, x2-1, y2, c);
                    drawline16(x1+1, y1, x2+1, y2, c);
                }
                else
                {
                    drawline16(x1-1, y1-1, x2-1, y2+1, c);
                    drawline16(x1, y1-1, x2, y2-1, c);
                    drawline16(x1, y1+1, x2, y2+1, c);
                }
                break;
            }
        }
    }
}

void DrawVertex(int x, int y, int c) // DrawVertex
{
    intptr_t offset = frameplace+y*bytesperline+x;
    *(unsigned char*)(offset-bytesperline*2-2) = c;
    *(unsigned char*)(offset-bytesperline*2-1) = c;
    *(unsigned char*)(offset-bytesperline*2-0) = c;
    *(unsigned char*)(offset-bytesperline*2+1) = c;
    *(unsigned char*)(offset-bytesperline*2+2) = c;
    *(unsigned char*)(offset-bytesperline*1-2) = c;
    *(unsigned char*)(offset-bytesperline*0-2) = c;
    *(unsigned char*)(offset+bytesperline*1-2) = c;
    *(unsigned char*)(offset-bytesperline*1+2) = c;
    *(unsigned char*)(offset-bytesperline*0+2) = c;
    *(unsigned char*)(offset+bytesperline*1+2) = c;
    *(unsigned char*)(offset+bytesperline*2-2) = c;
    *(unsigned char*)(offset+bytesperline*2-1) = c;
    *(unsigned char*)(offset+bytesperline*2-0) = c;
    *(unsigned char*)(offset+bytesperline*2+1) = c;
    *(unsigned char*)(offset+bytesperline*2+2) = c;
}

void DrawFaceSprite(int x, int y, int c)
{
    intptr_t offset = frameplace+y*bytesperline+x;
    *(unsigned char*)(offset-bytesperline*3-1) = c;
    *(unsigned char*)(offset-bytesperline*3-0) = c;
    *(unsigned char*)(offset-bytesperline*3+1) = c;
    *(unsigned char*)(offset-bytesperline*2-2) = c;
    *(unsigned char*)(offset-bytesperline*2+2) = c;
    *(unsigned char*)(offset-bytesperline*1-3) = c;
    *(unsigned char*)(offset-bytesperline*1+3) = c;
    *(unsigned char*)(offset-bytesperline*0-3) = c;
    *(unsigned char*)(offset-bytesperline*0+3) = c;
    *(unsigned char*)(offset+bytesperline*1-3) = c;
    *(unsigned char*)(offset+bytesperline*1+3) = c;
    *(unsigned char*)(offset+bytesperline*2-2) = c;
    *(unsigned char*)(offset+bytesperline*2+2) = c;
    *(unsigned char*)(offset+bytesperline*3-1) = c;
    *(unsigned char*)(offset+bytesperline*3-0) = c;
    *(unsigned char*)(offset+bytesperline*3+1) = c;
}

void DrawFloorSprite(int x, int y, char c)
{
    drawline16(x-3, y-3, x+3, y-3, c);
    drawline16(x-3, y+3, x+3, y+3, c);
    drawline16(x-3, y-3, x-3, y+3, c);
    drawline16(x+3, y-3, x+3, y+3, c);
}

void DrawMarkerSprite(int x, int y, int c)
{
    intptr_t offset = frameplace+y*bytesperline+x;
    *(unsigned char*)(offset-bytesperline*4-1) = c;
    *(unsigned char*)(offset-bytesperline*4-0) = c;
    *(unsigned char*)(offset-bytesperline*4+1) = c;

    *(unsigned char*)(offset-bytesperline*3-3) = c;
    *(unsigned char*)(offset-bytesperline*3-2) = c;
    *(unsigned char*)(offset-bytesperline*3+2) = c;
    *(unsigned char*)(offset-bytesperline*3+3) = c;

    *(unsigned char*)(offset-bytesperline*2-3) = c;
    *(unsigned char*)(offset-bytesperline*2-2) = c;
    *(unsigned char*)(offset-bytesperline*2+2) = c;
    *(unsigned char*)(offset-bytesperline*2+3) = c;

    *(unsigned char*)(offset-bytesperline*1-4) = c;
    *(unsigned char*)(offset-bytesperline*1-1) = c;
    *(unsigned char*)(offset-bytesperline*1+1) = c;
    *(unsigned char*)(offset-bytesperline*1+4) = c;

    *(unsigned char*)(offset-bytesperline*0-4) = c;
    *(unsigned char*)(offset-bytesperline*0-0) = c;
    *(unsigned char*)(offset-bytesperline*0+4) = c;

    *(unsigned char*)(offset+bytesperline*1-4) = c;
    *(unsigned char*)(offset+bytesperline*1-1) = c;
    *(unsigned char*)(offset+bytesperline*1+1) = c;
    *(unsigned char*)(offset+bytesperline*1+4) = c;

    *(unsigned char*)(offset+bytesperline*2-3) = c;
    *(unsigned char*)(offset+bytesperline*2-2) = c;
    *(unsigned char*)(offset+bytesperline*2+2) = c;
    *(unsigned char*)(offset+bytesperline*2+3) = c;

    *(unsigned char*)(offset+bytesperline*3-3) = c;
    *(unsigned char*)(offset+bytesperline*3-2) = c;
    *(unsigned char*)(offset+bytesperline*3+2) = c;
    *(unsigned char*)(offset+bytesperline*3+3) = c;

    *(unsigned char*)(offset+bytesperline*4-1) = c;
    *(unsigned char*)(offset+bytesperline*4-0) = c;
    *(unsigned char*)(offset+bytesperline*4+1) = c;
}

void DrawCircle(int x, int y, int r, int c)
{
    int px, py;
    px = x + r;
    py = y;
    for (int i = 28; i <= 2048; i += 28)
    {
        int nx, ny;
        nx = x + mulscale30(Cos(i), r);
        ny = y + mulscale30(Sin(i), r);
        drawline16(px, py, nx, ny, c);
        px = nx;
        py = ny;
    }
}

void qdraw2dscreen(int posxe, int posye, short ange, int zoome, short gride)
{
    char v4;
    char color;
    int v10;
    short cstat;

    if (gFrameClock & 8)
        v4 = 8;
    else
        v4 = 0;

    gZoom = zoome;
    gGrid = gride;

    if (qsetmode == 200)
        return;

    for (int i = 0; i < numwalls; i++)
    {
        if (wall[i].nextwall > i)
            continue;
        if (wall[i].nextwall == -1)
        {
            color = 0x0f;
            v10 = 0;
            cstat = wall[i].cstat;
        }
        else
        {
            color = 0x04;
            v10 = 0;
            cstat = wall[i].cstat | wall[wall[i].nextwall].cstat;
            if (i == linehighlight)
                cstat = wall[i].cstat;
            if (wall[i].nextwall == linehighlight)
                cstat = wall[wall[i].nextwall].cstat;
            if (cstat & 0x40)
                color = 0x05;
            if (cstat & 0x01)
                v10 = 1;
        }
        if (cstat & 0x4000)
            color = 0x09;
        if (cstat & 0x8000)
            color = 0x0a;
        if (linehighlight >= 0 && (i == linehighlight || wall[i].nextwall == linehighlight))
            color ^= v4;

        int xp1 = halfxdim16+mulscale(wall[i].x - posxe, zoome, 14);
        int yp1 = midydim16+mulscale(wall[i].y - posye, zoome, 14);
        int xp2 = halfxdim16+mulscale(wall[wall[i].point2].x - posxe, zoome, 14);
        int yp2 = midydim16+mulscale(wall[wall[i].point2].y - posye, zoome, 14);
        DrawLine(xp1, yp1, xp2, yp2, color, v10);
        if (zoome >= 256 && xp1 > 4 && xp1 < xdim-5 && yp1 > 4 && yp1 < ydim16-5)
            DrawVertex(xp1, yp1, 6);
    }
    if (zoome >= 256)
    {
        for (int i = 0; i < numwalls; i++)
        {
            if (i != pointhighlight)
            {
                if (highlightcnt <= 0)
                    continue;
                if (!TestBitString(show2dwall, i))
                    continue;
            }

            int xp = halfxdim16+mulscale(wall[i].x - posxe, zoome, 14);
            int yp = midydim16+mulscale(wall[i].y - posye, zoome, 14);
            if (xp > 4 && xp < xdim-5 && yp > 4 && yp < ydim16-5)
                DrawVertex(xp, yp, 6^v4);
        }
    }
    if (zoome >= 256)
    {
        for (int i = 0; i < numsectors; i++)
        {
            int j = headspritesect[i];
            while (j != -1)
            {
                int xp = halfxdim16+mulscale(sprite[j].x - posxe, zoome, 14);
                int yp = midydim16+mulscale(sprite[j].y - posye, zoome, 14);
                int dx, dy;
                if (sprite[j].statnum == 10)
                {
                    color = 0x0e;
                    if (sectorhighlight == sprite[j].owner)
                        color = 0x0f;
                    if ((pointhighlight == (j | 0x4000)) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp > 4 && xp < xdim-5 && yp > 4 && yp < ydim16-5)
                    {
                        switch (sprite[j].type)
                        {
                        case 3:
                        case 4:
                            DrawMarkerSprite(xp, yp, color);
                            break;
                        case 5:
                        case 8:
                            DrawMarkerSprite(xp, yp, color);
                            dx = mulscale30(zoome/128, Cos(sprite[j].ang));
                            dy = mulscale30(zoome/128, Sin(sprite[j].ang));
                            DrawLine(xp, yp, xp+dx, yp+dy, color, v10);
                            break;
                        }
                    }
                    if (sprite[j].type == 3)
                    {
                        int nSector = sprite[j].owner;
                        int nXSector = sector[nSector].extra;
                        dassert(nXSector > 0 && nXSector < kMaxXSectors);
                        int k = xsector[nXSector].marker1;
                        int xp2 = halfxdim16+mulscale(sprite[k].x - posxe, zoome, 14);
                        int yp2 = midydim16+mulscale(sprite[k].y - posye, zoome, 14);
                        drawline16(xp, yp, xp2, yp2, 9);
                        int ang = getangle(xp - xp2, yp - yp2);
                        dx = mulscale30(zoome/64, Cos(ang+170));
                        dy = mulscale30(zoome/64, Sin(ang+170));
                        drawline16(xp2, yp2, xp2+dx, yp2+dy, 9);
                        dx = mulscale30(zoome/64, Cos(ang-170));
                        dy = mulscale30(zoome/64, Sin(ang-170));
                        drawline16(xp2, yp2, xp2+dx, yp2+dy, 9);
                    }
                }
                else if (sprite[j].statnum == 12)
                {
                    color = 0x0e;
                    if (sectorhighlight == sprite[j].owner)
                        color = 0x0f;
                    if ((pointhighlight == (j | 0x4000)) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp > 4 && xp < xdim-5 && yp > 4 && yp < ydim16-5)
                    {
                        DrawMarkerSprite(xp, yp, color);
                    }
                    int nXSprite = sprite[j].extra;
                    if (nXSprite > 0)
                    {
                        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
                        XSPRITE* pXSprite = &xsprite[nXSprite];
                        if (byte_CBA0C)
                        {
                            int r1 = mulscale(pXSprite->data1, zoome, 10);
                            int r2 = mulscale(pXSprite->data2, zoome, 10);
                            DrawCircle(xp, yp, r1, 0x0e);
                            DrawCircle(xp, yp, r2, 0x06);
                        }
                    }
                }
                else
                {
                    color = 0x03;
                    v10 = 0;
                    if (sprite[j].cstat & 0x100)
                        color = 0x05;
                    if (sprite[j].cstat & 0x8000)
                        color = 0x08;
                    if (sprite[j].cstat & 0x2000)
                        color = 0x09;
                    if (sprite[j].cstat & 0x4000)
                        color = 0x0a;
                    if (sprite[j].cstat & 0x1)
                        v10 = 1;
                    if ((pointhighlight == (j | 0x4000)) || (highlightcnt > 0 && TestBitString(show2dsprite, j)))
                        color ^= v4;
                    if (xp > 4 && xp < xdim-5 && yp > 4 && yp < ydim16-5)
                    {
                        switch (sprite[j].cstat&0x30)
                        {
                        case 0x00:
                        case 0x30:
                        {
                            int dx, dy;
                            DrawFaceSprite(xp, yp, color);
                            dx = mulscale30(zoome/128, Cos(sprite[j].ang));
                            dy = mulscale30(zoome/128, Sin(sprite[j].ang));
                            DrawLine(xp, yp, xp+dx, yp+dy, color, v10);
                            break;
                        }
                        case 0x10:
                        {
                            int dx, dy;
                            DrawFaceSprite(xp, yp, color);
                            dx = mulscale30(zoome/128, Cos(sprite[j].ang+512));
                            dy = mulscale30(zoome/128, Sin(sprite[j].ang+512));
                            DrawLine(xp-dx, yp-dy, xp+dx, yp+dy, color, v10);
                            dx = mulscale30(zoome/256, Cos(sprite[j].ang));
                            dy = mulscale30(zoome/256, Sin(sprite[j].ang));
                            if (sprite[j].cstat & 0x40)
                                DrawLine(xp, yp, xp+dx, yp+dy, color, v10);
                            else
                                DrawLine(xp-dx, yp-dy, xp+dx, yp+dy, color, v10);
                            break;
                        }
                        case 0x20:
                        {
                            int dx, dy;
                            DrawFaceSprite(xp, yp, color);
                            dx = mulscale30(zoome/256, Cos(sprite[j].ang));
                            dy = mulscale30(zoome/256, Sin(sprite[j].ang));
                            DrawLine(xp, yp, xp+dx, yp+dy, color, v10);
                            break;
                        }
                        }
                    }
                }
                j = nextspritesect[j];
            }
        }
        gXTracker.Draw(posxe, posye, zoome);
    }
#if 0
    int ax = 320+mulscale30(zoome/128, Cos(ange));
    int ay = 200+mulscale30(zoome/128, Sin(ange));
    drawline16(ax, ay, 640-ax, 400-ay, 0x0f);
    drawline16(ax, ay, 320+ay-200, 200-ax+320, 0x0f);
    drawline16(ax, ay, 320-ay+200, 200+ax-320, 0x0f);
#endif
    int ax = mulscale30(zoome/128, Cos(ange));
    int ay = mulscale30(zoome/128, Sin(ange));
    drawline16(halfxdim16+ax, midydim16+ay, halfxdim16-ax, midydim16-ay, 0x0f);
    drawline16(halfxdim16+ax, midydim16+ay, halfxdim16+ay, midydim16-ax, 0x0f);
    drawline16(halfxdim16+ax, midydim16+ay, halfxdim16-ay, midydim16+ax, 0x0f);
}

void CheckKeys2D(void)
{
    static int dword_13ADAC, dword_13ADB0, dword_13ADB4;
    static int dword_CBA00 = -1, dword_CBA04 = -1, dword_CBA08 = -1;
    int numsprites = 0;
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
            numsprites++;
    }

    if (numwalls != dword_13ADAC || numsectors != dword_13ADB0 || numsprites != dword_13ADB4)
    {
        sub_1058C();
        dword_13ADAC = numwalls;
        dword_13ADB0 = numsectors;
        dword_13ADB4 = numsprites;
    }
    char shift = keystatus[sc_LeftShift] | keystatus[sc_RightShift];
    char ctrl = keystatus[sc_LeftControl] | keystatus[sc_RightControl];
    char alt = keystatus[sc_LeftAlt] | keystatus[sc_RightAlt];
    int mx, my;
    getpoint(searchx, searchy, &mx, &my);
    updatesector(mx, my, &sectorhighlight);
    if (pointhighlight != dword_CBA00 || linehighlight != dword_CBA04 || sectorhighlight != dword_CBA08)
    {
        dword_CBA00 = pointhighlight;
        dword_CBA04 = linehighlight;
        dword_CBA08 = sectorhighlight;
        if ((dword_CBA00 & 0xc000) == 0x4000)
            ShowSpriteData(dword_CBA00 & 0x3fff);
        else if (dword_CBA04 >= 0)
            ShowWallData(dword_CBA04);
        else if (dword_CBA08 >= 0)
            ShowSectorData(dword_CBA08);
        else
            clearmidstatbar16();
    }
    unsigned char key = keyGetScan();
    switch (key)
    {
    case sc_NumLock:
        sprintf(gTempBuf, "size xsprite=%d xsector=%d xwall=%d", sizeof(XSPRITE), sizeof(XSECTOR), sizeof(XWALL));
        printmessage16(gTempBuf);
        break;
    case sc_kpad_1:
    case sc_End:
        byte_CBA0C = !byte_CBA0C;
        break;
    case sc_Home:
        if (ctrl)
        {
            short nSprite = getnumber16("Locate sprite #: ", 0, kMaxSprites, 0);
            clearmidstatbar16();
            if (nSprite >= 0 && nSprite < kMaxSprites && sprite[nSprite].statnum < kMaxStatus)
            {
                posx = sprite[nSprite].x;
                posy = sprite[nSprite].y;
                posz = sprite[nSprite].z;
                ang = sprite[nSprite].ang;
                cursectnum = sprite[nSprite].ang;
            }
            else
                printmessage16("Sir Not Appearing In This Film");
        }
        break;
    case sc_D:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            spritetype* pSprite = &sprite[nSprite];
            if (alt)
            {
                short nClipDist = getnumber16("Sprite clipdist #: ", 0, 256, 0);
                clearmidstatbar16();
                if (nClipDist >= 0 && nClipDist < 256)
                {
                    pSprite->clipdist = nClipDist;
                    sprintf(gTempBuf, "sprite[%d].clipdist is %d", nSprite, pSprite->clipdist);
                    printmessage16(gTempBuf);
                    ModifyBeep();
                }
                else
                {
                    printmessage16("Clipdist must be between 0 and 255");
                    Beep();
                }
            }
            else
            {
                short nDetail = getnumber16("Sprite detail Level #: ", 0, 5, 0);
                clearmidstatbar16();
                if (nDetail >= 0 && nDetail <= 4)
                {
                    pSprite->filler = nDetail;
                    sprintf(gTempBuf, "sprite[%d].detail is %d", nSprite, pSprite->filler);
                    printmessage16(gTempBuf);
                    ModifyBeep();
                }
                else
                {
                    sprintf(gTempBuf, "Detail must be between %d and %d", 0, 4);
                    printmessage16(gTempBuf);
                    Beep();
                }
            }
        }
        break;
    case sc_I:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            spritetype* pSprite = &sprite[nSprite];
            pSprite->cstat ^= 0x8000;
            ModifyBeep();
        }
        else
            Beep();
        break;
    case sc_K:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            short nMotion = sprite[pointhighlight & 0x3fff].cstat & 0x6000;
            switch (nMotion)
            {
            case 0:
                nMotion = 0x2000;
                break;
            case 0x2000:
                nMotion = 0x4000;
                break;
            case 0x4000:
                nMotion = 0x0000;
                break;
            }
            sprite[pointhighlight & 0x3fff].cstat &= ~0x6000;
            sprite[pointhighlight & 0x3fff].cstat |= nMotion;
            ModifyBeep();
        }
        else if (linehighlight >= 0)
        {
            short nMotion = wall[linehighlight].cstat & 0xc000;
            switch (nMotion)
            {
            case 0:
                nMotion = 0x4000;
                break;
            case 0x4000:
                nMotion = 0x8000;
                break;
            case 0x8000:
                nMotion = 0x0000;
                break;
            }
            wall[linehighlight].cstat &= ~0xc000;
            wall[linehighlight].cstat |= nMotion;
            ModifyBeep();
        }
        else
            Beep();
        break;
    case sc_M:
    {
        if (linehighlight)
        {
            int nWall = linehighlight;
            int nWall2 = wall[linehighlight].nextwall;
            if (nWall2 < 0)
            {
                Beep();
                break;
            }
            wall[nWall].cstat |= 0x10;
            wall[nWall].cstat &= ~0x08;
            wall[nWall2].cstat |= 0x10;
            wall[nWall2].cstat |= 0x08;
            if (wall[nWall].overpicnum < 0)
                wall[nWall].overpicnum = 0;
            wall[nWall2].overpicnum = wall[nWall].overpicnum;
            wall[nWall].cstat &= ~0x20;
            wall[nWall2].cstat &= ~0x20;
            sprintf(gTempBuf, "wall[%i] %s masked", searchwall, (wall[searchwall].cstat & 0x10) ? "is" : "not");
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        else
            Beep();
        break;
    }
    case sc_X:
        if (alt)
        {
            if (linehighlight >= 0)
            {
                int nSector = sectorofwall(linehighlight);
                dassert(nSector >= 0 && nSector < kMaxSectors);
                sectortype* pSector = &sector[nSector];
                int n = linehighlight - pSector->wallptr;
                pSector->filler = n;
                sprintf(gTempBuf, "Sector will align to wall %d (%d)", linehighlight, n);
                printmessage16(gTempBuf);
                ModifyBeep();
            }
            else
                Beep();
        }
        break;
    case sc_OpenBracket:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            int nStep;
            if (shift)
                nStep = 16;
            else
                nStep = 256;
            sprite[nSprite].ang = (~(nStep-1))&(sprite[nSprite].ang-1);
            sprintf(gTempBuf, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        break;
    case sc_CloseBracket:
        if ((pointhighlight & 0xc000) == 0x4000)
        {
            int nSprite = pointhighlight & 0x3fff;
            int nStep;
            if (shift)
                nStep = 16;
            else
                nStep = 256;
            sprite[nSprite].ang = (~(nStep - 1)) & (sprite[nSprite].ang + nStep);
            sprintf(gTempBuf, "sprite[%i].ang: %i", nSprite, sprite[nSprite].ang);
            printmessage16(gTempBuf);
            ModifyBeep();
        }
        break;
    }
    if (key)
        keyFlushScans();
}

///////////////////// 3d editor //////////////////




void SetSectorCeilZ(int nSector, int z)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    z = ClipHigh(z, sector[nSector].floorz);
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        spritetype* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (getceilzofslope(nSector, pSprite->x, pSprite->y) >= top)
            pSprite->z += z - sector[nSector].ceilingz;
    }
    sector[nSector].ceilingz = z;
}

void SetSectorFloorZ(int nSector, int z)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    z = ClipLow(z, sector[nSector].ceilingz);
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        spritetype* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        if (getflorzofslope(nSector, pSprite->x, pSprite->y) <= bottom)
            pSprite->z += z - sector[nSector].floorz;
    }
    sector[nSector].floorz = z;
}

void SetSectorCeilSlope(int nSector, int slope)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sector[nSector].ceilingheinum = slope;
    if (sector[nSector].ceilingheinum == 0)
        sector[nSector].ceilingstat &= ~0x02;
    else
        sector[nSector].ceilingstat |= 0x02;
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        spritetype* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int z = getceilzofslope(nSector, pSprite->x, pSprite->y);
        if (z > top)
            sprite[nSprite].z += z - top;
    }
}

void SetSectorFloorSlope(int nSector, int slope)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    sector[nSector].floorheinum = slope;
    if (sector[nSector].floorheinum == 0)
        sector[nSector].floorstat &= ~0x02;
    else
        sector[nSector].floorstat |= 0x02;
    for (int nSprite = headspritesect[nSector]; nSprite != -1; nSprite = nextspritesect[nSprite])
    {
        spritetype* pSprite = &sprite[nSprite];
        int top, bottom;
        GetSpriteExtents(pSprite, &top, &bottom);
        int z = getflorzofslope(nSector, pSprite->x, pSprite->y);
        if (z < bottom)
            sprite[nSprite].z += z - bottom;
    }
}

void SetSectorLightingPhase(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR* pXSector = &xsector[nXSector];
        pXSector->phase = a2;
    }
}

void SetSectorMotionTheta(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        XSECTOR* pXSector = &xsector[nXSector];
        pXSector->bobTheta = a2;
    }
}

char IsSectorHighlight(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    for (int i = 0; i < highlightsectorcnt; i++)
    {
        if (highlightsector[i] == nSector)
            return 1;
    }
    return 0;
}

inline int GetWallZPeg(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls);
    int nSector = sectorofwall(nWall);
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nNextSector = wall[nWall].nextsector;
    int z;
    if (nNextSector == -1)
    {
        if (wall[nWall].cstat & 0x04)
            z = sector[nSector].floorz;
        else
            z = sector[nSector].ceilingz;
    }
    else
    {
        if (wall[nWall].cstat & 0x04)
            z = sector[nSector].floorz;
        else
        {
            if (sector[nNextSector].ceilingz > sector[nSector].ceilingz)
                z = sector[nNextSector].ceilingz;
            if (sector[nNextSector].floorz < sector[nSector].floorz)
                z = sector[nNextSector].floorz;
        }
    }
    return z;
}

void AlignWalls(int nWall0, int z0, int nWall1, int z1, int nTile)
{
    dassert(nWall0 >= 0 && nWall0 < kMaxWalls);
    dassert(nWall1 >= 0 && nWall1 < kMaxWalls);
    wall[nWall1].cstat &= ~0x108;
    wall[nWall1].xpanning = ((wall[nWall0].xrepeat<<3)+wall[nWall0].xpanning)%tilesizx[nTile];
    z1 = GetWallZPeg(nWall1);

    int n = picsiz[nTile]>>4;
    if ((1<<n) != tilesizy[nTile])
        n++;

    wall[nWall1].yrepeat = wall[nWall0].yrepeat;
    wall[nWall1].ypanning = wall[nWall0].ypanning + (((z1-z0)*wall[nWall0].yrepeat) >> (n+3));
}

char visited[kMaxWalls];

void AutoAlignWalls(int nWall0, int ply)
{
    dassert(nWall0 >= 0 && nWall0 < kMaxWalls);
    int nTile = wall[nWall0].picnum;

    if (ply == 64)
        return;

    if (ply == 0)
    {
        memset(visited, 0, sizeof(visited));
        visited[nWall0] = 1;
    }

    int z0 = GetWallZPeg(nWall0);

    int nWall1 = wall[nWall0].point2;

    dassert(nWall1 >= 0 && nWall1 < kMaxWalls);

    while (1)
    {
        if (visited[nWall1])
            break;

        visited[nWall1] = 1;

        if (wall[nWall1].nextwall == nWall0)
            break;

        if (wall[nWall1].picnum == nTile)
        {
            int z1 = GetWallZPeg(nWall1);

            char visible = 0;

            int nNextSector = wall[nWall1].nextsector;
            if (nNextSector < 0)
                visible = 1;
            else
            {
                int nSector = wall[wall[nWall1].nextwall].nextsector;
                if (getceilzofslope(nSector, wall[nWall1].x, wall[nWall1].y) < getceilzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y))
                    visible = 1;
                if (getflorzofslope(nSector, wall[nWall1].x, wall[nWall1].y) > getflorzofslope(nNextSector, wall[nWall1].x, wall[nWall1].y))
                    visible = 1;
            }
            if (visible)
            {
                AlignWalls(nWall0, z0, nWall1, z1, nTile);

                int nNextWall = wall[nWall1].nextwall;
                if (nNextWall < 0)
                {
                    nWall0 = nWall1;
                    z0 = GetWallZPeg(nWall0);
                    nWall1 = wall[nWall0].point2;
                    continue;
                }
                if ((wall[nWall1].cstat & 0x02) && wall[nNextWall].picnum == nTile)
                    AlignWalls(nWall0, z0, nNextWall, z1, nTile);
                AutoAlignWalls(nWall1, ply + 1);
            }
        }

        if (wall[nWall1].nextwall < 0)
            break;

        nWall1 = wall[wall[nWall1].nextwall].point2;
    }
}

void sub_216F8(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallnum+i].nextsector;
        if (nNextSector == -1)
            continue;
        if (IsSectorHighlight(nNextSector) && !visited[nSector])
        {
            SetSectorFloorZ(nNextSector, sector[nSector].floorz - a2);
            sub_216F8(nNextSector, a2);
        }
    }
}

void sub_21798(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    visited[nSector] = 1;
    for (int i = 0; i < sector[nSector].wallnum; i++)
    {
        int nNextSector = wall[sector[nSector].wallnum+i].nextsector;
        if (nNextSector == -1)
            continue;
        if (IsSectorHighlight(nNextSector) && !visited[nSector])
        {
            SetSectorCeilZ(nNextSector, sector[nSector].ceilingz - a2);
            sub_21798(nNextSector, a2);
        }
    }
}

unsigned short WallShadeFrac[kMaxWalls];
unsigned short FloorShadeFrac[kMaxSectors];
unsigned short CeilShadeFrac[kMaxSectors];
int WallArea[kMaxWalls];
int dword_14ADE8[kMaxWalls];
int dword_152DE8[kMaxWalls];

int dword_149DE8[kMaxSectors];
int dword_15ADE8[kMaxSectors];
int dword_15BDE8[kMaxSectors];
int dword_15CDE8[kMaxSectors];
int dword_15DDE8[kMaxSectors];
int dword_15EDE8[kMaxSectors];
int dword_15FDE8[kMaxSectors];


int gLightBombMaxBright;
int gLightBombRampDist;
int gLightBombReflections;
int gLightBombAttenuation;
int gLightBombIntensity;

void CalcLightBomb(int x, int y, int z, short nSector, int dx, int dy, int dz, int a8, int a9, int a10)
{
    short nHitSect, nHitWall, nHitSprite;
    int hx, hy, hz;
    nHitSect = -1;
    nHitWall = -1;
    nHitSprite = -1;
    hitscan(x, y, z, nSector, dx, dy, dz << 4, &nHitSect, &nHitWall, &nHitSprite, &hx, &hy, &hz, 0);
    int dx_ = klabs(hx - x)>>4;
    int dy_ = klabs(hy - y)>>4;
    int dz_ = klabs(hz - z)>>8;
    a10 += ksqrt(dx_*dx_+dy_*dy_+dz_*dz_);
    int t = gLightBombRampDist + a10;
    if (nHitWall >= 0)
    {
        int t2 = divscale16(a8, ClipLow(t, 1));
        if (t2 > 0)
        {
            t2 = divscale16(t2, ClipLow(WallArea[nHitWall], 1));
            int shade = (wall[nHitWall].shade<<16)+WallShadeFrac[nHitWall]-t2;
            wall[nHitWall].shade = ClipLow(shade>>16, gLightBombMaxBright);
            WallShadeFrac[nHitWall] = shade & 0xffff;
            if (a9 < gLightBombReflections)
            {
                int wx = dword_14ADE8[nHitWall];
                int wy = dword_152DE8[nHitWall];
                int dot = dmulscale16(dx, wx, dy, wy);
                if (dot >= 0)
                {
                    dx -= mulscale16(dot*2, wx);
                    dy -= mulscale16(dot*2, wy);
                    hx += dx >> 12;
                    hy += dy >> 12;
                    hz += dz >> 12;
                    a8 -= mulscale16(a8, gLightBombAttenuation);
                    CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
                }
            }
        }
    }
    else if (nHitSprite >= 0)
    {

    }
    else if (dz > 0)
    {
        int t2 = divscale16(a8, ClipLow(t, 1));
        if (t2 > 0)
        {
            t2 = divscale16(t2, ClipLow(dword_149DE8[nHitSect], 1));
            int shade = (sector[nHitSect].floorshade<<16)+FloorShadeFrac[nHitSect]-t2;
            sector[nHitSect].floorshade = ClipLow(shade>>16, gLightBombMaxBright);
            FloorShadeFrac[nHitSect] = shade & 0xffff;
            if (a9 < gLightBombReflections)
            {
                if (sector[nHitSect].floorstat & 0x02)
                {
                    int wx = dword_15ADE8[nHitSect];
                    int wy = dword_15BDE8[nHitSect];
                    int wz = dword_15CDE8[nHitSect];
                    int dot = tmulscale16(dx, wx, dy, wy, dz, wz);
                    if (dot < 0)
                        return;

                    dx -= mulscale16(dot*2, wx);
                    dy -= mulscale16(dot*2, wy);
                    dz -= mulscale16(dot*2, wz);
                }
                else
                    dz = -dz;
                a8 -= mulscale16(a8, gLightBombAttenuation);
                hx += dx >> 12;
                hy += dy >> 12;
                hz += dz >> 12;
                CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
            }
        }
    }
    else
    {
        int t2 = divscale16(a8, ClipLow(t, 1));
        if (t2 > 0)
        {
            t2 = divscale16(t2, ClipLow(dword_149DE8[nHitSect], 1));
            int shade = (sector[nHitSect].ceilingshade <<16)+CeilShadeFrac[nHitSect]-t2;
            sector[nHitSect].ceilingshade = ClipLow(shade>>16, gLightBombMaxBright);
            CeilShadeFrac[nHitSect] = shade & 0xffff;
            if (a9 < gLightBombReflections)
            {
                if (sector[nHitSect].ceilingstat & 0x02)
                {
                    int wx = dword_15DDE8[nHitSect];
                    int wy = dword_15EDE8[nHitSect];
                    int wz = dword_15FDE8[nHitSect];
                    int dot = tmulscale16(dx, wx, dy, wy, dz, wz);
                    if (dot < 0)
                        return;

                    dx -= mulscale16(dot*2, wx);
                    dy -= mulscale16(dot*2, wy);
                    dz -= mulscale16(dot*2, wz);
                }
                else
                    dz = -dz;
                a8 -= mulscale16(a8, gLightBombAttenuation);
                hx += dx >> 12;
                hy += dy >> 12;
                hz += dz >> 12;
                CalcLightBomb(hx, hy, hz, nHitSect, dx, dy, dz, a8, a9+1, a10);
            }
        }
    }
}

int CalcSectorArea(sectortype* pSector)
{
    int nStartWall = pSector->wallptr;
    int nEndWall = nStartWall + pSector->wallnum;
    int nArea = 0;
    for (int i = nStartWall; i < nEndWall; i++)
    {
        int x0 = wall[i].x >> 4;
        int y0 = wall[i].y >> 4;
        int x1 = wall[wall[i].point2].x >> 4;
        int y1 = wall[wall[i].point2].y >> 4;

        nArea += (x0+x1)*(y1-y0);
    }

    nArea >>= 1;
    return nArea;
}

void ResetLightBomb()
{
    sectortype* pSector = sector;
    for (int i = 0; i < numsectors; i++, pSector++)
    {
        FloorShadeFrac[i] = 0;
        CeilShadeFrac[i] = 0;
        dword_149DE8[i] = CalcSectorArea(pSector);
        walltype* pWall = &wall[pSector->wallptr];
        for (int j = pSector->wallptr; j < pSector->wallptr+pSector->wallnum; j++, pWall++)
        {
            WallShadeFrac[j] = 0;
            
            int nx = (wall[pWall->point2].y-pWall->y)>>4;
            int ny = (-(wall[pWall->point2].x-pWall->x))>>4;
            int nLength = ksqrt(nx*nx+ny*ny);
            dword_14ADE8[j] = divscale16(nx, nLength);
            dword_152DE8[j] = divscale16(ny, nLength);
            int ceilz0, florz0, ceilz1, florz1;
            getzsofslope(i, pWall->x, pWall->y, &ceilz0, &florz0);
            getzsofslope(i, wall[pWall->point2].x, wall[pWall->point2].y, &ceilz1, &florz1);
            int ceil = (ceilz0+ceilz1)>>1;
            int flor = (florz0+florz1)>>1;
            int height = flor - ceil;
            int nNextSector = pWall->nextsector;
            if (nNextSector >= 0)
            {
                int v1c, v20, v24, v28;
                height = 0;
                getzsofslope(nNextSector, pWall->x, pWall->y, &v28, &v20);
                getzsofslope(nNextSector, wall[pWall->point2].x, wall[pWall->point2].y, &v24, &v1c);
                int ceiln = (v28+v24)>>1;
                int florn = (v20+v1c)>>1;
                if (florn < flor)
                    height += flor-florn;
                if (ceiln > ceil)
                    height += ceiln-ceil;
            }
            WallArea[j] = (height*nLength)>>8;
        }

        dword_15ADE8[i] = 0;
        dword_15BDE8[i] = 0;
        dword_15CDE8[i] = -0x1000;
        dword_15DDE8[i] = 0;
        dword_15EDE8[i] = 0;
        dword_15FDE8[i] = 0x1000;
    }
}

void DoLightBomb(int x, int y, int z, short nSector)
{
    for (int i = 171; i <= 853; i += 85)
    {
        for (int j = 0; j < 2048; j += 8)
        {
            int dx = mulscale30(Cos(j), Sin(i)) >> 16;
            int dy = mulscale30(Sin(j), Sin(i)) >> 16;
            int dz = Cos(i) >> 16;

            CalcLightBomb(x, y, z, nSector, dx, dy, dz, gLightBombIntensity, 0, 0);
        }
    }
}

void SetFirstWall(int nSector, int nWall)
{
    int start = sector[nSector].wallptr;
    int length = sector[nSector].wallnum;
    dassert(nWall >= start && nWall < start + length);
    int n = nWall - start;
    if (!n)
        return;
    walltype twall;
    int j = start;
    int k = start;
    for (int i = length; i > 0; i--)
    {
        if (j == k)
            twall = wall[k];

        int t = n+j;
        while (t >= start - length)
            t -= length;
        if (t == j)
        {
            wall[k] = twall;
            k = ++j;
        }
        else
        {
            wall[k] = wall[t];
            k = t;
        }
    }
    for (int i = start; i < start+length; i++)
    {
        wall[i].point2 -= n;
        if (wall[i].point2 < start)
            wall[i].point2 += length;
        if (wall[i].nextwall >= 0)
            wall[wall[i].nextwall].nextwall = i;
    }
    sub_1058C();
}

void sub_22350()
{
    searchsector = wall[searchwall2].nextsector;
    if (searchwallcf)
        searchstat = 2;
    else
        searchstat = 1;
}

int InsertMisc(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD07C[] = {
        1072, 1074, 1076, 1070, 1078, 1048, 1046, 1127, 796,
        266, 1069, 1142, 462, 739, 642, 2522, 2523, 2524, 2525,
        2526, 2527, 2528, 2529, 650
    };
    int v4 = 0;
    tileIndexCount = 24;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD07C[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type;
    switch (vc)
    {
    case 650:
        type = 32;
        break;
    case 1046:
    case 1048:
    case 1070:
    case 1072:
    case 1074:
    case 1076:
    case 1078:
        type = 20;
        break;
    case 1127:
        type = 408;
        break;
    case 796:
        type = 407;
        break;
    case 266:
        type = 406;
        break;
    case 1069:
        type = 410;
        break;
    case 1142:
        type = 409;
        break;
    case 462:
        type = 405;
        break;
    case 739:
        type = 403;
        break;
    case 642:
        type = 404;
        break;
    // case 642:
    //     type = 404;
    //     break;
    case 2522:
    case 2523:
    case 2524:
    case 2525:
    case 2526:
    case 2527:
    case 2528:
    case 2529:
        type = 1;
        break;
    }
    int nSprite = InsertSprite(a2, v4);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;
    sprite[nSprite].picnum = vc;
    switch (vc)
    {
    case 1046:
    case 1048:
    case 1070:
    case 1072:
    case 1074:
    case 1076:
    case 1078:
        sprite[nSprite].xrepeat = sprite[nSprite].yrepeat = 48;
        break;
    case 2522:
    case 2523:
    case 2524:
    case 2525:
    case 2526:
    case 2527:
    case 2528:
    case 2529:
    {
        int nXSprite = sub_10E50(nSprite);
        dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
        xsprite[nXSprite].data1 = vc - 2522;
        break;
    }
    }

    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

int InsertHazard(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD0AC[9] = {
        655, 3444, 907, 968, 1080, 835, 1156, 2178, 908
    };
    int v4 = 0;
    tileIndexCount = 9;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD0AC[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type;
    switch (vc)
    {
    case 655:
        type = 454;
        break;
    case 3444:
        type = 401;
        break;
    case 907:
        type = 400;
        break;
    case 968:
        type = 450;
        break;
    case 1080:
        type = 457;
        break;
    case 835:
        type = 458;
        break;
    case 1156:
        type = 456;
        break;
    case 2178:
        type = 413;
        break;
    case 908:
        type = 459;
        break;
    default:
        scrSetMessage("Invalid hazard tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, v4);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;

    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

int InsertItem(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD0BE[] = {
        2552, 2553, 2554, 2555, 2556, 2557, 519, 2169, 2433,
        896, 825, 827, 829, 830, 760, 2428, 839, 840, 841, 842,
        843, 518, 522, 523, 837, 2628, 2586, 2578, 2602, 2594
    };
    tileIndexCount = 30;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD0BE[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type = -1;
    switch (vc)
    {
    case 2552:
        type = 100;
        break;
    case 2553:
        type = 101;
        break;
    case 2554:
        type = 102;
        break;
    case 2555:
        type = 103;
        break;
    case 2556:
        type = 104;
        break;
    case 2557:
        type = 105;
        break;
    case 519:
        type = 107;
        break;
    case 822:
        type = 108;
        break;
    case 2169:
        type = 109;
        break;
    case 2433:
        type = 110;
        break;
    case 517:
        type = 111;
        break;
    case 783:
        type = 112;
        break;
    case 896:
        type = 113;
        break;
    case 825:
        type = 114;
        break;
    case 827:
        type = 115;
        break;
    case 828:
        type = 116;
        break;
    case 829:
        type = 117;
        break;
    case 830:
        type = 118;
        break;
    case 831:
        type = 119;
        break;
    case 863:
        type = 120;
        break;
    //case 863:
    //    type = 120;
    //    break;
    case 760:
        type = 121;
        break;
    case 836:
        type = 122;
        break;
    case 851:
        type = 123;
        break;
    case 2428:
        type = 124;
        break;
    case 839:
        type = 125;
        break;
    case 768:
        type = 126;
        break;
    case 840:
        type = 127;
        break;
    case 841:
        type = 128;
        break;
    case 842:
        type = 129;
        break;
    case 843:
        type = 130;
        break;
    //case 843:
    //    type = 130;
    //    break;
    case 683:
        type = 131;
        break;
    case 521:
        type = 132;
        break;
    case 604:
        type = 133;
        break;
    //case 604:
    //    type = 133;
    //    break;
    case 520:
        type = 134;
        break;
    case 803:
        type = 135;
        break;
    case 518:
        type = 136;
        break;
    case 522:
        type = 137;
        break;
    case 523:
        type = 138;
        break;
    case 837:
        type = 139;
        break;
    case 2628:
        type = 140;
        break;
    case 2586:
        type = 141;
        break;
    case 2578:
        type = 142;
        break;
    case 2602:
        type = 143;
        break;
    case 2594:
        type = 144;
        break;
    default:
        scrSetMessage("Invalid item tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;
    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

int InsertAmmo(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD0FA[] = {
        548, 820, 589, 618, 619, 809, 810, 811, 812, 813, 817, 816, 801, 525
    };
    tileIndexCount = 14;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD0FA[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type = -1;
    switch (vc)
    {
    case 548:
        type = 73;
        break;
    case 820:
        type = 66;
        break;
    case 589:
        type = 62;
        break;
    case 618:
        type = 60;
        break;
    case 619:
        type = 67;
        break;
    case 809:
        type = 63;
        break;
    //case 809:
    //    type = 63;
    //    break;
    case 810:
        type = 65;
        break;
    case 811:
        type = 64;
        break;
    case 812:
        type = 68;
        break;
    case 813:
        type = 69;
        break;
    case 817:
        type = 72;
        break;
    case 816:
        type = 76;
        break;
    case 801:
        type = 79;
        break;
    case 525:
        type = 70;
        break;
    default:
        scrSetMessage("Invalid ammo tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;
    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

int InsertWeapon(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD116[9] = {
        524, 559, 558, 526, 589, 618, 539, 800, 525
    };
    int v4 = 0;
    tileIndexCount = 9;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD116[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type;
    switch (vc)
    {
    case 524:
        type = 43;
        break;
    case 559:
        type = 41;
        break;
    case 558:
        type = 42;
        break;
    case 526:
        type = 46;
        break;
    case 589:
        type = 62;
        break;
    case 618:
        type = 60;
        break;
    case 539:
        type = 45;
        break;
    case 800:
        type = 50;
        break;
    case 525:
        type = 70;
        break;
    default:
        scrSetMessage("Invalid weapon tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 3);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;
    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    ModifyBeep();
    return nSprite;
}

int InsertEnemy(int a1, int a2, int a3, int a4, int a5, int a6)
{
    short word_CD128[27] = {
        2860, 2825, 3385, 1170, 3054, 1370, 1209, 1470, 1530,
        3060, 1270, 1980, 1920, 1925, 1930, 1935, 1570, 1870,
        1950, 1745, 1792, 1797, 2680, 3140, 3798, 3870, 2960
    };
    tileIndexCount = 27;
    for (int i = 0; i < tileIndexCount; i++)
    {
        tileIndex[i] = word_CD128[i];
    }
    int vc = tilePick(-1, -1, -2);
    if (vc == -1)
        return -1;
    int type = -1;
    switch (vc)
    {
    case 2860:
        type = 201;
        break;
    case 3385:
        type = 230;
        break;
    case 2825:
        type = 202;
        break;
    case 1170:
        type = 203;
        break;
    case 1209:
        type = 244;
        break;
    case 3054:
        type = 205;
        break;
    case 1370:
        type = 204;
        break;
    case 1470:
        type = 206;
        break;
    case 1530:
        type = 208;
        break;
    case 3060:
        type = 210;
        break;
    case 1270:
        type = 211;
        break;
    case 1980:
        type = 212;
        break;
    case 1920:
        type = 213;
        break;
    case 1925:
        type = 214;
        break;
    case 1930:
        type = 215;
        break;
    case 1935:
        type = 216;
        break;
    //case 1935:
    //    type = 216;
    //    break;
    case 1570:
        type = 217;
        break;
    case 1870:
        type = 218;
        break;
    case 1950:
        type = 219;
        break;
    case 1745:
        type = 220;
        break;
    case 1792:
        type = 221;
        break;
    case 1797:
        type = 222;
        break;
    case 2680:
        type = 227;
        break;
    case 832:
        type = 200;
        break;
    case 3140:
        type = 229;
        break;
    case 3798:
        type = 245;
        break;
    case 3870:
        type = 250;
        break;
    case 2960:
        type = 251;
        break;
    default:
        scrSetMessage("Invalid dude tile selected!");
        Beep();
        return -1;
    }
    int nSprite = InsertSprite(a2, 6);
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    sprite[nSprite].type = type;
    sub_10EA0();
    spritetype* pSprite = &sprite[nSprite];
    pSprite->shade = -8;
    pSprite->x = a3;
    pSprite->y = a4;
    pSprite->z = a5;
    pSprite->ang = a6;

    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    if (a1 == 2)
        pSprite->z += getflorzofslope(a2, pSprite->x, pSprite->y)-bottom;
    else
        pSprite->z += getceilzofslope(a2, pSprite->x, pSprite->y)-top;
    updatenumsprites();
    int nXSprite = sub_10E50(nSprite);
    dassert(nXSprite > 0 && nXSprite < kMaxXSprites);
    switch (type)
    {
    case 213:
    case 214:
    case 215:
    case 216:
        if (a1 == 1 && !(sector[a2].ceilingstat & 0x01))
            pSprite->cstat |= 0x08;
        break;
    case 219:
        if (a1 == 1 && !(sector[a2].ceilingstat & 0x01))
            pSprite->picnum = 1948;
        else
            xsprite[nXSprite].state = 1;
        break;
    }
    ModifyBeep();
    return nSprite;
}

int InsertObject(int a1, int a2, int a3, int a4, int a5, int a6)
{
    Win window(0, 0, 80, 182, "Insert");

    TextButton* pEnemy = new TextButton(4, 4, 60, 20, "&Enemy", (MODAL_RESULT)8);
    TextButton* pWeapon = new TextButton(4, 26, 60, 20, "&Weapon", (MODAL_RESULT)9);
    TextButton* pAmmo = new TextButton(4, 48, 60, 20, "&Ammo", (MODAL_RESULT)10);
    TextButton* pItem = new TextButton(4, 70, 60, 20, "&Item", (MODAL_RESULT)11);
    TextButton* pHazard = new TextButton(4, 92, 60, 20, "&Hazard", (MODAL_RESULT)12);
    TextButton* pMisc = new TextButton(4, 114, 60, 20, "&Misc", (MODAL_RESULT)13);
    TextButton* pCancel = new TextButton(4, 136, 60, 20, "&Cancel", MODAL_RESULT_2);

    window.at5e->Insert(pEnemy);
    window.at5e->Insert(pWeapon);
    window.at5e->Insert(pAmmo);
    window.at5e->Insert(pItem);
    window.at5e->Insert(pHazard);
    window.at5e->Insert(pMisc);
    window.at5e->Insert(pCancel);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
        return InsertEnemy(a1, a2, a3, a4, a5, (a6+1024)&2047);
    case 9:
        return InsertWeapon(a1, a2, a3, a4, a5, a6);
    case 10:
        return InsertAmmo(a1, a2, a3, a4, a5, a6);
    case 11:
        return InsertItem(a1, a2, a3, a4, a5, a6);
    case 12:
        return InsertHazard(a1, a2, a3, a4, a5, a6);
    case 13:
        return InsertMisc(a1, a2, a3, a4, a5, a6);
    case MODAL_RESULT_2:
        return -1;
    }
    return -1;
}

unsigned char CompareXSectors(XSECTOR *pXSector1, XSECTOR *pXSector2)
{
    return memcmp(pXSector1, pXSector2, sizeof(XSECTOR)) == 0;
}

unsigned char ConfirmYesNo(const char *a1)
{
    Win window(59, 80, 202, 46, a1);

    TextButton* pYes = new TextButton(4, 4, 60, 20, "&Yes", (MODAL_RESULT)6);
    window.at5e->Insert(pYes);

    TextButton* pNo = new TextButton(68, 4, 60, 20, "&No", (MODAL_RESULT)7);
    window.at5e->Insert(pNo);

    ShowModal(&window);

    return window.at25 == (MODAL_RESULT)6;
}

unsigned char ShowOptions(void)
{
    Win window(0, 0, 80, 182, "Options");

    TextButton* pClean = new TextButton(4, 4, 60, 20, "C&lean", (MODAL_RESULT)8);

    TextButton* pCancel = new TextButton(136, 4, 60, 20, "&Cancel", MODAL_RESULT_2);
    window.at5e->Insert(pClean);
    window.at5e->Insert(pCancel);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
    {
        XSECTOR v64;
        if (somethingintab == 2)
        {
            int nXSector = sector[searchsector].extra;
            if (nXSector >= 0)
            {
                if (xsector[nXSector].reference == searchsector)
                    v64 = xsector[nXSector];
            }
        }
        int vdi = 1;
        for (int i = 0; i < kMaxXSectors; i++)
        {
            int nSector = xsector[i].reference;
            if (nSector != -1 && sector[nSector].extra == i && nSector != searchsector)
            {
                v64.reference = xsector[i].reference;
                if (CompareXSectors(&v64, &xsector[i]))
                    vdi++;
            }
        }
        char buffer[40];
        sprintf(buffer, "Clean %i common sectors?", vdi);
        if (ConfirmYesNo(buffer))
        {
            for (int i = 0; i < kMaxXSectors; i++)
            {
                int nSector = xsector[i].reference;
                if (nSector != -1 && sector[nSector].extra == i)
                {
                    v64.reference = xsector[i].reference;
                    if (CompareXSectors(&v64, &xsector[i]))
                        dbDeleteXSector(i);
                }
            }
        }
        InitSectorFX();
        break;
    }
    case MODAL_RESULT_2:
        return 0;
    }
    return 1;
}

void sub_258B0(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    visited[a2] = 1;
    for (int i = 0; i < sector[i].wallnum; i++)
    {
        int nNextSector = wall[sector[i].wallptr + i].nextsector;
        if (nNextSector != -1 && IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            int nXSector = sector[i].extra;
            if (nXSector > 0)
            {
                dassert(nXSector < kMaxXSectors);
                XSECTOR* pXSector = &xsector[nXSector];
                SetSectorLightingPhase(nNextSector, (pXSector->phase + a2) & 2047);
                sub_258B0(nNextSector, a2);
            }
        }
    }
}

void sub_259F0(int nSector, int a2)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    visited[a2] = 1;
    for (int i = 0; i < sector[i].wallnum; i++)
    {
        int nNextSector = wall[sector[i].wallptr + i].nextsector;
        if (nNextSector != -1 && IsSectorHighlight(nNextSector) && !visited[nNextSector])
        {
            int nXSector = sector[i].extra;
            if (nXSector > 0)
            {
                dassert(nXSector < kMaxXSectors);
                XSECTOR* pXSector = &xsector[nXSector];
                SetSectorMotionTheta(nNextSector, (pXSector->bobTheta + a2) & 2047);
                sub_259F0(nNextSector, a2);
            }
        }
    }
}

char ShowSectorTricks(void)
{
    char v4 = 0;
    Win window(0, 0, 180, 182, "Sector Tricks");

    Label* pValue = new Label(4, 8, "&Value:");
    window.at5e->Insert(pValue);

    EditNumber* pEdit = new EditNumber(44, 4, 80, 16, 0);
    pEdit->at1f = 'V';
    window.at5e->Insert(pEdit);

    TextButton* pSH = new TextButton(4, 24, 80, 20, "&Step Height", (MODAL_RESULT)8);
    window.at5e->Insert(pSH);

    TextButton* pLH = new TextButton(4, 44, 80, 20, "&Light Phase", (MODAL_RESULT)9);
    window.at5e->Insert(pLH);

    TextButton* pZP = new TextButton(4, 64, 80, 20, "&Z Phase", (MODAL_RESULT)10);
    window.at5e->Insert(pZP);

    ShowModal(&window);

    switch (window.at25)
    {
    case 8:
        switch (searchstat)
        {
        case 2:
            memset(visited, 0, sizeof(visited));
            sub_216F8(searchsector, pEdit->at130);
            v4 = 1;
            break;
        case 1:
            memset(visited, 0, sizeof(visited));
            sub_21798(searchsector, pEdit->at130);
            v4 = 1;
            break;
        }
        break;
    case 9:
        memset(visited, 0, sizeof(visited));
        v4 = 1;
        sub_258B0(searchsector, pEdit->at130);
        break;
    case 10:
        memset(visited, 0, sizeof(visited));
        sub_259F0(searchsector, pEdit->at130);
        v4 = 1;
        break;
    }
    return v4;
}

void SetSkyTile(int nTile)
{
    dassert(nTile >= 0 && nTile < kMaxTiles);

    pskybits = 10 - (picsiz[nTile] & 15);
    gSkyCount = 1 << pskybits;

    sector[searchsector].ceilingpicnum = nTile;

    for (int i = 0; i < gSkyCount; i++)
        pskyoff[i] = i;

    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].ceilingstat & 1)
            sector[i].ceilingpicnum = nTile;
        if (sector[i].floorstat & 1)
            sector[i].floorpicnum = nTile;
    }
}

void sub_25E64(spritetype* pSprite, int a2)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += getceilzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - top;
}

void sub_25EF8(spritetype* pSprite, int a2)
{
    int top, bottom;
    GetSpriteExtents(pSprite, &top, &bottom);
    pSprite->z += getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) - bottom;
}

void sub_25F8C(spritetype* pSprite, int a2)
{
    pSprite->z = (pSprite->z-1)&(~(a2-1));
}

void sub_25F9C(spritetype* pSprite, int a2)
{
    pSprite->z = (pSprite->z+a2)&(~(a2-1));
}

void OperateSprites(void (*a1)(spritetype *, int), int a2)
{
    if (TestBitString(show2dsprite, searchwall))
    {
        for (int i = 0; i < highlightcnt; i++)
        {
            if ((highlight[i] & 0xc000) == 0x4000)
            {
                a1(&sprite[highlight[i]&0x3fff], a2);
            }
        }
    }
    else
        a1(&sprite[searchwall], a2);
}

void sub_26054(int nSector, int a2)
{
    SetSectorCeilZ(nSector, sector[nSector].ceilingz + a2);
}

void sub_26074(int nSector, int a2)
{
    SetSectorFloorZ(nSector, sector[nSector].floorz + a2);
}

void sub_26094(int nSector, int a2)
{
    SetSectorCeilZ(nSector, (sector[nSector].ceilingz-1)&(~(a2-1)));
}

void sub_260B8(int nSector, int a2)
{
    SetSectorFloorZ(nSector, (sector[nSector].floorz-1)&(~(a2-1)));
}

void sub_260DC(int nSector, int a2)
{
    SetSectorCeilZ(nSector, (sector[nSector].ceilingz+a2)&(~(a2-1)));
}

void sub_260FC(int nSector, int a2)
{
    SetSectorFloorZ(nSector, (sector[nSector].floorz+a2)&(~(a2-1)));
}

void sub_2611C(int nSector, int a2)
{
    int nXSector = sector[nSector].extra;
    if (nXSector > 0)
    {
        xsector[nXSector].wave = a2;
    }
}

void OperateSectors(void (*a1)(int, int), int a2)
{
    if (IsSectorHighlight(searchsector))
    {
        for (int i = 0; i < highlightcnt; i++)
        {
            a1(highlightsector[i], a2);
        }
    }
    else
        a1(searchsector, a2);
}

int dword_D9A80 = 0;
int dword_13ADE4;
short temptype, tempang;
char tempvis2;

short word_CA89C = 1;
short word_CA8A0 = 1;

void Check3DKeys(void)
{
    static char buffer[256];
    char shift = keystatus[sc_LeftShift] | keystatus[sc_RightShift];
    char ctrl = keystatus[sc_LeftControl] | keystatus[sc_RightControl];
    char alt = keystatus[sc_LeftAlt] | keystatus[sc_RightAlt];
    char kp5 = keystatus[sc_kpad_5];
    if (ctrl && kp5)
        horiz = 100;

    if (angvel)
    {
        int v = gFrameTicks;
        if (shift)
            v += v / 2;
        ang = (ang + ((v * angvel) >> 4)) & 2047;
    }

    if (vel | svel)
    {
        int v = gFrameTicks;
        if (shift)
            v <<= 1;
        int dx = 0, dy = 0;
        if (vel)
        {
            dx += mulscale30((vel * v) >> 2, Cos(ang));
            dy += mulscale30((vel * v) >> 2, Sin(ang));
        }
        if (svel)
        {
            dx += mulscale30((svel * v) >> 2, Sin(ang));
            dy -= mulscale30((svel * v) >> 2, Cos(ang));
        }
        clipmove(&posx, &posy, &posz, &cursectnum, dx<<14, dy<<14, 200, 1024, 1024, 0);
    }
    int ceilz, ceilhit, florz, florhit;
    getzrange(posx, posy, posz, cursectnum, &ceilz, &ceilhit, &florz, &florhit, 200, 0x10001);

    if ((ceilhit & 0xe000) == 0x4000 & (ceilhit & 0x1fff) == cursectnum && (sector[cursectnum].ceilingstat & 1))
        ceilz = (int)0x80000000;

    if (zmode == 0)
    {
        int z = florz - kensplayerheight;
        if (z < ceilz + 0x1000)
            z = (florz + ceilz) / 2;
        if (keystatus[sc_A])
        {
            if (ctrl)
            {
                horiz = ClipLow(horiz - 2*gFrameTicks, 0);
            }
            else
            {
                z -= 0x1000;
                if (shift)
                    z -= 0x1800;
            }
        }
        if (keystatus[sc_Z])
        {
            if (ctrl)
            {
                horiz = ClipHigh(horiz + 2*gFrameTicks, 200);
            }
            else
            {
                z += 0xc00;
                if (shift)
                    z += 0xc00;
            }
        }
        if (z != posz)
        {
            if (z > posz)
                dword_13ADE4 += 32 / 2 * gFrameTicks;
            if (z < posz)
                dword_13ADE4 = (z-posz)>>3;
            posz += dword_13ADE4 / 2 * gFrameTicks;
            if (florz - 0x400 < posz)
            {
                posz = florz - 0x400;
                dword_13ADE4 = 0;
            }
            if (ceilz + 0x400 > posz)
            {
                posz = ceilz + 0x400;
                dword_13ADE4 = 0;
            }
        }
    }
    else
    {
        int z = posz;
        if (keystatus[sc_A])
        {
            if (keystatus[sc_LeftControl])
            {
                horiz = ClipLow(horiz - 2*gFrameTicks, 0);
            }
            else if (zmode != 1)
            {
                z -= 0x800;
            }
            else
            {
                zlock += 0x400 / 2 * gFrameTicks;
                keystatus[sc_A] = 0;
            }
        }
        if (keystatus[sc_Z])
        {
            if (keystatus[sc_LeftControl])
            {
                horiz = ClipHigh(horiz + 2*gFrameTicks, 200);
            }
            else if (zmode != 1)
            {
                z += 0x800;
            }
            else if (zlock > 0)
            {
                zlock -= 0x400 / 2 * gFrameTicks;
                keystatus[sc_Z] = 0;
            }
        }

        if (z < ceilz + 0x400)
            z = ceilz + 0x400;
        if (z > florz - 0x400)
            z = florz - 0x400;
        if (zmode == 1)
            z = florz - zlock;
        if (z < ceilz + 0x400)
            z = (florz + ceilz) / 2;
        if (zmode == 1)
            posz = z;
        if (z != posz)
        {
            if (z > posz)
                dword_13ADE4 += 32 / 2 * gFrameTicks;
            if (z < posz)
                dword_13ADE4 -= 32 / 2 * gFrameTicks;
            posz += dword_13ADE4 / 2 * gFrameTicks;
            if (florz - 0x400 < posz)
            {
                posz = florz - 0x400;
                dword_13ADE4 = 0;
            }
            if (ceilz + 0x400 > posz)
            {
                posz = ceilz + 0x400;
                dword_13ADE4 = 0;
            }
        }
        else
            dword_13ADE4 = 0;
    }
    // Mouse::Read(gFrameTicks);
    int mouseX, mouseY;
    MouseClamp();
    mouseX = mousex;
    mouseY = mousey;

    searchx = ClipRange(mouseX, 1, xdim-2);
    searchy = ClipRange(mouseY, 1, ydim-2);

    searchit = 2;

    int mouseButtons;
    readmousebstatus(&mouseButtons);

    if (searchstat >= 0 && (mouseButtons&1))
        searchit = 0;

    gColor = gStdColor[23+mulscale30(8, Sin((gFrameClock<<11)/120))];
    gfxHLine(searchy, searchx - 6, searchx - 2);
    gfxHLine(searchy, searchx + 2, searchx + 6);
    gfxVLine(searchx, searchy - 5, searchy - 2);
    gfxVLine(searchx, searchy + 2, searchy + 5);

    if (searchstat < 0)
        return;

    unsigned char key = keyGetScan();
    switch (key)
    {
    case sc_PgUp:
    {
        int vdi = 0x400;
        if (shift)
            vdi = 0x100;
        if (searchstat == 0)
        {
            int nNextSector = wall[searchwall].nextsector;
            if (nNextSector != -1)
                sub_22350();
        }
        if (searchstat == 0)
            searchstat = 1;
        if (ctrl)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].ceilingz, -1, -1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorCeilZ, sector[vd].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].floorz, 1, -1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorFloorZ, sector[vd].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(sub_25E64, 0);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off floor", vd, vd);
                OperateSectors(sub_26054, (sector[searchsector].floorz - vd * 256) - sector[searchsector].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off ceiling", vd, vd);
                OperateSectors(sub_26074, (sector[searchsector].ceilingz + vd * 256) - sector[searchsector].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 1:
            {
                OperateSectors(sub_26094, vdi);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                OperateSectors(sub_260B8, vdi);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(sub_25F8C, vdi);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        ModifyBeep();
        break;
    }
    case sc_PgDn:
    {
        int vdi = 0x400;
        if (shift)
            vdi = 0x100;
        if (searchstat == 0)
        {
            int nNextSector = wall[searchwall].nextsector;
            if (nNextSector != -1)
                sub_22350();
        }
        if (searchstat == 0)
            searchstat = 1;
        if (ctrl)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].ceilingz, -1, 1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorCeilZ, sector[vd].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = nextsectorneighborz(searchsector, sector[searchsector].floorz, 1, 1);
                if (vd == -1)
                    vd = searchsector;
                OperateSectors(SetSectorFloorZ, sector[vd].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(sub_25EF8, 0);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 1:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off floor", vd, vd);
                OperateSectors(sub_26054, (sector[searchsector].floorz - vd * 256) - sector[searchsector].ceilingz);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                int vd = (sector[searchsector].floorz - sector[searchsector].ceilingz) / 256;
                vd = GetNumberBox("height off ceiling", vd, vd);
                OperateSectors(sub_26074, (sector[searchsector].ceilingz + vd * 256) - sector[searchsector].floorz);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 1:
            {
                OperateSectors(sub_260DC, vdi);
                sprintf(buffer, "sector[%i].ceilingz: %i", searchsector, sector[searchsector].ceilingz);
                scrSetMessage(buffer);
                break;
            }
            case 2:
            {
                OperateSectors(sub_260FC, vdi);
                sprintf(buffer, "sector[%i].floorz: %i", searchsector, sector[searchsector].floorz);
                scrSetMessage(buffer);
                break;
            }
            case 3:
                OperateSprites(sub_25F9C, vdi);
                sprintf(buffer, "sprite[%i].z: %i", searchwall, sprite[searchwall].z);
                scrSetMessage(buffer);
                break;
            }
        }
        ModifyBeep();
        break;
    }
    case sc_Delete:
        if (searchstat == 3)
        {
            DeleteSprite(searchwall);
            updatenumsprites();
            ModifyBeep();
        }
        else
            Beep();
        break;
    case sc_Tab:
        switch (searchstat)
        {
        case 0:
            temppicnum = wall[searchwall].picnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            tempextra = wall[searchwall].extra;
            temptype = wall[searchwall].lotag;
            break;
        case 1:
            temppicnum = sector[searchsector].ceilingpicnum;
            tempshade = sector[searchsector].ceilingshade;
            temppal = sector[searchsector].ceilingpal;
            tempxrepeat = sector[searchsector].ceilingxpanning;
            tempyrepeat = sector[searchsector].ceilingypanning;
            tempcstat = sector[searchsector].ceilingstat;
            tempextra = sector[searchsector].extra;
            tempvis2 = sector[searchsector].visibility;
            temptype = sector[searchsector].lotag;
            break;
        case 2:
            temppicnum = sector[searchsector].floorpicnum;
            tempshade = sector[searchsector].floorshade;
            temppal = sector[searchsector].floorpal;
            tempxrepeat = sector[searchsector].floorxpanning;
            tempyrepeat = sector[searchsector].floorypanning;
            tempcstat = sector[searchsector].floorstat;
            tempextra = sector[searchsector].extra;
            tempvis2 = sector[searchsector].visibility;
            temptype = sector[searchsector].lotag;
            break;
        case 3:
            temppicnum = sprite[searchwall].picnum;
            tempshade = sprite[searchwall].shade;
            temppal = sprite[searchwall].pal;
            tempxrepeat = sprite[searchwall].xrepeat;
            tempyrepeat = sprite[searchwall].yrepeat;
            tempcstat = sprite[searchwall].cstat;
            tempextra = sprite[searchwall].extra;
            tempang = sprite[searchwall].ang;
            temptype = sprite[searchwall].type;
            break;
        case 4:
            temppicnum = wall[searchwall].overpicnum;
            tempshade = wall[searchwall].shade;
            temppal = wall[searchwall].pal;
            tempxrepeat = wall[searchwall].xrepeat;
            tempyrepeat = wall[searchwall].yrepeat;
            tempcstat = wall[searchwall].cstat;
            tempextra = wall[searchwall].extra;
            temptype = wall[searchwall].lotag;
            break;
        }
        somethingintab = searchstat;
        break;
    case sc_CapsLock:
    {
        static const char* pzZMode[] = {
            "Gravity",
            "Locked/Step",
            "Locked/Free"
        };
        zmode++;
        if (zmode >= 3)
            zmode = 0;
        if (zmode == 1)
            zlock = (florz-posz)&~0x3ff;
        sprintf(buffer, "ZMode = %s", pzZMode[zmode]);
        scrSetMessage(buffer);
        break;
    }
    case sc_Enter:
        if (somethingintab == 255)
        {
            Beep();
            break;
        }
        if (ctrl)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                int i = searchwall;
                do
                {
                    if (shift)
                    {
                        wall[i].shade = tempshade;
                        wall[i].pal = temppal;
                    }
                    else
                    {
                        wall[i].picnum = temppicnum;
                        if (somethingintab == 0 || somethingintab == 4)
                        {
                            wall[i].xrepeat = tempxrepeat;
                            wall[i].yrepeat = tempyrepeat;
                            wall[i].cstat = tempcstat;
                        }
                        fixrepeats(i);
                    }
                    i = wall[i].point2;
                } while (i != searchwall);
                ModifyBeep();
                break;
            }
            case 1:
            {
                for (int i = 0; i < numsectors; i++)
                {
                    if (sector[i].ceilingstat & 1)
                    {
                        sector[i].ceilingpicnum = temppicnum;
                        sector[i].ceilingshade = tempshade;
                        sector[i].ceilingpal = temppal;
                        if (somethingintab == 1 || somethingintab == 2)
                        {
                            sector[i].ceilingxpanning = tempxrepeat;
                            sector[i].ceilingypanning = tempyrepeat;
                            sector[i].ceilingstat = tempcstat|1;
                        }
                    }
                }
                ModifyBeep();
                break;
            }
            case 2:
            {
                for (int i = 0; i < numsectors; i++)
                {
                    if (sector[i].floorstat & 1)
                    {
                        sector[i].floorpicnum = temppicnum;
                        sector[i].floorshade = tempshade;
                        sector[i].floorpal = temppal;
                        if (somethingintab == 1 || somethingintab == 2)
                        {
                            sector[i].floorxpanning = tempxrepeat;
                            sector[i].floorypanning = tempyrepeat;
                            sector[i].floorstat = tempcstat|1;
                        }
                    }
                }
                ModifyBeep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        else if (shift)
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                break;
            case 1:
                if (IsSectorHighlight(searchsector))
                {
                    for (int i = 0; i < highlightsectorcnt; i++)
                    {
                        // FIXED: was using i as sector number
                        int nSector = highlightsector[i];
                        sector[nSector].ceilingshade = tempshade;
                        sector[nSector].ceilingpal = temppal;
                        sector[nSector].visibility = tempvis2;
                    }
                }
                else
                {
                    sector[searchsector].ceilingshade = tempshade;
                    sector[searchsector].ceilingpal = temppal;
                    sector[searchsector].visibility = tempvis2;
                    if (sector[searchsector].ceilingstat & 1)
                    {
                        for (int i = 0; i < numsectors; i++)
                        {
                            if (sector[i].ceilingstat & 1)
                            {
                                sector[i].ceilingshade = tempshade;
                                sector[i].ceilingpal = temppal;
                                sector[i].visibility = tempvis2;
                            }
                        }
                    }
                }
                break;
            case 2:
                if (IsSectorHighlight(searchsector))
                {
                    for (int i = 0; i < highlightsectorcnt; i++)
                    {
                        // FIXED: was using i as sector number
                        int nSector = highlightsector[i];
                        sector[nSector].floorshade = tempshade;
                        sector[nSector].floorpal = temppal;
                        sector[nSector].visibility = tempvis2;
                    }
                }
                else
                {
                    sector[searchsector].floorshade = tempshade;
                    sector[searchsector].floorpal = temppal;
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 3:
                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;
                break;
            case 4:
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                break;
            }
            ModifyBeep();
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                if (somethingintab == 0 || somethingintab == 4)
                {
                    wall[searchwall].extra = tempextra;
                    wall[searchwall].lotag = temptype;
                    sub_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            case 1:
            case 2:
                if (somethingintab == 1 || somethingintab == 2)
                {
                    if (IsSectorHighlight(searchsector))
                    {
                        for (int i = 0; i < highlightsectorcnt; i++)
                        {
                            // FIXED: was using i as sector number
                            int nSector = highlightsector[i];
                            sector[nSector].extra = tempextra;
                            sector[nSector].lotag = temptype;
                        }
                    }
                    else
                    {
                        sector[searchsector].extra = tempextra;
                        sector[searchsector].lotag = temptype;
                    }
                    sub_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            case 3:
                if (somethingintab == 3)
                {
                    sprite[searchwall].type = temptype;
                    sprite[searchwall].extra = tempextra;
                    sub_1058C();
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].picnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 0)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                }
                fixrepeats(searchwall);
                break;
            case 1:
                sector[searchsector].ceilingpicnum = temppicnum;
                sector[searchsector].ceilingshade = tempshade;
                sector[searchsector].ceilingpal = temppal;
                if (somethingintab == 1 || somethingintab == 2)
                {
                    sector[searchsector].ceilingxpanning = tempxrepeat;
                    sector[searchsector].ceilingypanning = tempyrepeat;
                    sector[searchsector].ceilingstat = tempcstat & 0xff; // FIXME
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 2:
                sector[searchsector].floorpicnum = temppicnum;
                sector[searchsector].floorshade = tempshade;
                sector[searchsector].floorpal = temppal;
                if (somethingintab == 1 || somethingintab == 2)
                {
                    sector[searchsector].floorxpanning = tempxrepeat;
                    sector[searchsector].floorypanning = tempyrepeat;
                    sector[searchsector].floorstat = tempcstat & 0xff; // FIXME
                    sector[searchsector].visibility = tempvis2;
                }
                break;
            case 3:
            {
                sprite[searchwall].picnum = temppicnum;
                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;
                if (somethingintab == 3)
                {
                    sprite[searchwall].xrepeat = tempxrepeat;
                    sprite[searchwall].yrepeat = tempyrepeat;
                    if (sprite[searchwall].xrepeat < 1)
                        sprite[searchwall].xrepeat = 1;
                    if (sprite[searchwall].yrepeat < 1)
                        sprite[searchwall].yrepeat = 1;
                    sprite[searchwall].cstat = tempcstat;
                }
                int top, bottom;
                GetSpriteExtents(&sprite[searchwall], &top, &bottom);
                if (!(sector[sprite[searchwall].sectnum].ceilingstat & 1))
                    sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - top, 0);
                    
                if (!(sector[sprite[searchwall].sectnum].floorstat & 1))
                    sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - bottom, 0);
                break;
            }
            case 4:
                wall[searchwall].overpicnum = temppicnum;
                if (wall[searchwall].nextwall >= 0)
                    wall[wall[searchwall].nextwall].overpicnum = temppicnum;
                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;
                if (somethingintab == 4)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                }
                fixrepeats(searchwall);
                break;
            }
            ModifyBeep();
        }
        break;
    case sc_OpenBracket:
    {
        int vdi = 0x100;
        if (shift)
            vdi = 0x20;
        switch (searchstat)
        {
        case 1:
            if (IsSectorHighlight(searchsector))
            {
                for (int i = 0; i < highlightsectorcnt; i++)
                {
                    int nSector = highlightsector[i];
                    SetSectorCeilSlope(nSector, (sector[nSector].ceilingheinum-1)&~(vdi-1));
                }
                sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, vdi);
                scrSetMessage(buffer);
            }
            else
            {
                SetSectorCeilSlope(searchsector, (sector[searchsector].ceilingheinum-1)&~(vdi-1));
                sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingheinum);
                scrSetMessage(buffer);
            }
            break;
        case 2:
            if (IsSectorHighlight(searchsector))
            {
                for (int i = 0; i < highlightsectorcnt; i++)
                {
                    int nSector = highlightsector[i];
                    SetSectorFloorSlope(nSector, (sector[nSector].floorheinum-1)&~(vdi-1));
                }
                sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, vdi);
                scrSetMessage(buffer);
            }
            else
            {
                SetSectorFloorSlope(searchsector, (sector[searchsector].floorheinum-1)&~(vdi-1));
                sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorheinum);
                scrSetMessage(buffer);
            }
            break;
        }
        ModifyBeep();
        break;
    }
    case sc_CloseBracket:
        if (alt)
        {
            short nNextSector = wall[searchwall].nextsector;
            if (nNextSector < 0)
            {
                Beep();
                break;
            }
            int x = wall[searchwall].x;
            int y = wall[searchwall].y;
            switch (searchstat)
            {
            case 1:
                alignceilslope(searchsector, x, y, getceilzofslope(nNextSector, x, y));
                ModifyBeep();
                break;
            case 2:
                alignflorslope(searchsector, x, y, getflorzofslope(nNextSector, x, y));
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        else
        {
            int vdi = 0x100;
            if (shift)
                vdi = 0x20;
            switch (searchstat)
            {
            case 1:
                if (IsSectorHighlight(searchsector))
                {
                    for (int i = 0; i < highlightsectorcnt; i++)
                    {
                        int nSector = highlightsector[i];
                        SetSectorCeilSlope(nSector, (sector[nSector].ceilingheinum+vdi)&~(vdi-1));
                    }
                    sprintf(buffer, "adjusted %i ceilings by %i", highlightsectorcnt, vdi);
                    scrSetMessage(buffer);
                }
                else
                {
                    SetSectorCeilSlope(searchsector, (sector[searchsector].ceilingheinum+vdi)&~(vdi-1));
                    sprintf(buffer, "sector[%i].ceilingslope: %i", searchsector, sector[searchsector].ceilingheinum);
                    scrSetMessage(buffer);
                }
                break;
            case 2:
                if (IsSectorHighlight(searchsector))
                {
                    for (int i = 0; i < highlightsectorcnt; i++)
                    {
                        int nSector = highlightsector[i];
                        SetSectorFloorSlope(nSector, (sector[nSector].floorheinum+vdi)&~(vdi-1));
                    }
                    sprintf(buffer, "adjusted %i floors by %i", highlightsectorcnt, vdi);
                    scrSetMessage(buffer);
                }
                else
                {
                    SetSectorFloorSlope(searchsector, (sector[searchsector].floorheinum+vdi)&~(vdi-1));
                    sprintf(buffer, "sector[%i].floorslope: %i", searchsector, sector[searchsector].floorheinum);
                    scrSetMessage(buffer);
                }
                break;
            }
            ModifyBeep();
        }
        break;
    case sc_Comma:
        switch (searchstat)
        {
        case 3:
        {
            int vsi = shift ? 0x10 : 0x100;
            sprite[searchwall].ang = ((sprite[searchwall].ang+vsi)&(~(vsi-1)))&2047;
            sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case sc_Period:
        switch (searchstat)
        {
        case 3:
        {
            int vsi = shift ? 0x10 : 0x100;
            sprite[searchwall].ang = ((sprite[searchwall].ang-1)&(~(vsi-1)))&2047;
            sprintf(buffer, "sprite[%i].ang: %i", searchwall, sprite[searchwall].ang);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        case 0:
        case 4:
            AutoAlignWalls(searchwall, 0);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_BackSlash:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingheinum = 0;
            sector[searchsector].ceilingstat &= ~2;
            sprintf(buffer, "sector[%i] ceiling slope reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 2:
            sector[searchsector].floorheinum = 0;
            sector[searchsector].floorstat &= ~2;
            sprintf(buffer, "sector[%i] floor slope reset", searchsector);
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    case sc_Slash:
        switch (searchstat)
        {
        case 0:
        case 4:
            if (shift)
            {
                wall[searchwall].xrepeat = wall[searchwall].yrepeat;
            }
            else
            {
                wall[searchwall].xpanning = 0;
                wall[searchwall].ypanning = 0;
                wall[searchwall].xrepeat = 8;
                wall[searchwall].yrepeat = 8;
                wall[searchwall].cstat = 0;
            }
            fixrepeats(searchwall);
            sprintf(buffer, "wall[%i] pan/repeat reset", searchwall);
            scrSetMessage(buffer);
            break;
        case 1:
            sector[searchsector].ceilingxpanning = 0;
            sector[searchsector].ceilingypanning = 0;
            sector[searchsector].ceilingstat &= ~0x34;
            sprintf(buffer, "sector[%i] ceiling pan reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 2:
            sector[searchsector].floorxpanning = 0;
            sector[searchsector].floorypanning = 0;
            sector[searchsector].floorstat &= ~0x34;
            sprintf(buffer, "sector[%i] floor pan reset", searchsector);
            scrSetMessage(buffer);
            break;
        case 3:
            if (shift)
            {
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat;
            }
            else
            {
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat = 64;
            }
            sprintf(buffer, "sprite[%i].xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
            scrSetMessage(buffer);
            break;
        }
        ModifyBeep();
        break;
    case sc_Minus:
        if (ctrl && alt)
        {
            if (IsSectorHighlight(searchsector))
            {
                for (int i = 0; i < highlightsectorcnt; i++)
                {
                    sector[highlightsector[i]].visibility++;
                }
                scrSetMessage("highlighted sectors less visible");
            }
            else
            {
                sector[searchsector].visibility++;
                sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
                scrSetMessage(buffer);
            }
            ModifyBeep();
        }
        else if (ctrl)
        {
            int nXSector = sub_10DBC(searchsector);
            xsector[nXSector].amplitude++;
            sprintf(buffer, "Amplitude: %i", xsector[nXSector].amplitude);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (shift)
        {
            int nXSector = sub_10DBC(searchsector);
            xsector[nXSector].phase -= 6;
            sprintf(buffer, "Phase: %i", xsector[nXSector].phase);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (keystatus[sc_G])
        {
            gGamma = ClipLow(gGamma - 1, 0);
            sprintf(buffer, "Gamma correction level %i", gGamma);
            scrSetMessage(buffer);
            scrSetGamma(gGamma);
            scrSetDac();
        }
        else if (keystatus[sc_D])
        {
            gVisibility = ClipHigh(gVisibility + 0x10, 0x1000);
            sprintf(buffer, "Depth cueing level %i", gVisibility);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        break;
    case sc_Equals:
        if (ctrl && alt)
        {
            if (IsSectorHighlight(searchsector))
            {
                for (int i = 0; i < highlightsectorcnt; i++)
                {
                    sector[highlightsector[i]].visibility--;
                }
                scrSetMessage("highlighted sectors more visible");
            }
            else
            {
                sector[searchsector].visibility--;
                sprintf(buffer, "Visibility: %i", sector[searchsector].visibility);
                scrSetMessage(buffer);
            }
            ModifyBeep();
        }
        else if (ctrl)
        {
            int nXSector = sub_10DBC(searchsector);
            xsector[nXSector].amplitude--;
            sprintf(buffer, "Amplitude: %i", xsector[nXSector].amplitude);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (shift)
        {
            int nXSector = sub_10DBC(searchsector);
            xsector[nXSector].phase += 6;
            sprintf(buffer, "Phase: %i", xsector[nXSector].phase);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (keystatus[sc_G])
        {
            gGamma = ClipHigh(gGamma + 1, gGammaLevels-1);
            sprintf(buffer, "Gamma correction level %i", gGamma);
            scrSetMessage(buffer);
            scrSetGamma(gGamma);
            scrSetDac();
        }
        else if (keystatus[sc_D])
        {
            gVisibility = ClipHigh(gVisibility - 0x10, 0x80);
            sprintf(buffer, "Depth cueing level %i", gVisibility);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        break;
    case sc_B:
        switch (searchstat)
        {
        case 0:
        case 4:
            if (TestBitString(show2dwall, searchwall))
            {
                for (int i = 0; i < highlightcnt; i++)
                {
                    if ((highlight[i] & 0xc000) == 0)
                        wall[highlight[i]].cstat ^= 1;
                }
            }
            else
            {
                wall[searchwall].cstat ^= 1;
                if (wall[searchwall].nextwall >= 0)
                {
                    wall[wall[searchwall].nextwall].cstat &= ~1;
                    wall[wall[searchwall].nextwall].cstat |= wall[searchwall].cstat & 1;
                }
            }
            sprintf(buffer, "Wall %i blocking flag is %s", searchwall, dword_D9A88[(wall[searchwall].cstat & 1) != 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= 1;
            sprintf(buffer, "sprite[%i] %s blocking", searchwall, (sprite[searchwall].cstat & 1) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_C:
        if (alt)
        {
            if (searchstat == somethingintab)
            {
                switch (searchstat)
                {
                case 0:
                {
                    int va = wall[searchwall].picnum;
                    if (TestBitString(show2dwall, searchwall))
                    {
                        for (int i = 0; i < highlightcnt; i++)
                        {
                            if ((highlight[i] & 0xc000) == 0)
                            {
                                int nWall = highlight[i];
                                if (wall[nWall].picnum == va)
                                {
                                    if (wall[nWall].picnum != temppicnum)
                                        wall[nWall].picnum = temppicnum;
                                    else if (wall[nWall].pal != temppal)
                                        wall[nWall].pal = temppal;
                                }
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < numwalls; i++)
                        {
                            if (wall[i].picnum == va)
                            {
                                if (wall[i].picnum != temppicnum)
                                    wall[i].picnum = temppicnum;
                                else if (wall[i].pal != temppal)
                                    wall[i].pal = temppal;
                            }
                        }
                    }
                    break;
                }
                case 1:
                {
                    int va = sector[searchsector].ceilingpicnum;
                    for (int i = 0; i < numsectors; i++)
                    {
                        if (sector[i].ceilingpicnum == va)
                        {
                            if (sector[i].ceilingpicnum != temppicnum)
                                sector[i].ceilingpicnum = temppicnum;
                            else if (sector[i].ceilingpal != temppal)
                                sector[i].ceilingpal = temppal;
                        }
                    }
                    break;
                }
                case 2:
                {
                    int va = sector[searchsector].floorpicnum;
                    for (int i = 0; i < numsectors; i++)
                    {
                        if (sector[i].floorpicnum == va)
                        {
                            if (sector[i].floorpicnum != temppicnum)
                                sector[i].floorpicnum = temppicnum;
                            else if (sector[i].floorpal != temppal)
                                sector[i].floorpal = temppal;
                        }
                    }
                    break;
                }
                case 3:
                {
                    int va = sprite[searchwall].picnum;
                    for (int i = 0; i < kMaxSprites; i++)
                    {
                        if (sprite[i].statnum < kMaxStatus && sprite[i].picnum == va)
                        {
                            sprite[i].picnum = temppicnum;
                            sprite[i].pal = temppal;
                        }
                    }
                    break;
                }
                case 4:
                {
                    int va = wall[searchwall].overpicnum;
                    for (int i = 0; i < numwalls; i++)
                    {
                        if (wall[i].overpicnum == va)
                        {
                            wall[i].overpicnum = temppicnum;
                        }
                    }
                }
                }
                ModifyBeep();
            }
            else
                Beep();
        }
        break;
    case sc_D:
        if (searchstat == 3)
        {
            if (alt)
            {
                int nClipDist = GetNumberBox("Sprite clipdist", sprite[searchwall].clipdist, sprite[searchwall].clipdist);
                if (nClipDist >= 0 && nClipDist < 256)
                {
                    sprite[searchwall].clipdist = nClipDist;
                    sprintf(buffer, "sprite[%d].clipdist is %d", searchwall, nClipDist);
                    scrSetMessage(buffer);
                    ModifyBeep();
                }
                else
                {
                    sprintf(buffer, "Clipdist must be between %d and %d", 0, 255);
                    scrSetMessage(buffer);
                    Beep();
                }
            }
            else
            {
                int nDetail = GetNumberBox("Sprite detail", sprite[searchwall].filler, sprite[searchwall].filler);
                if (nDetail >= 0 && nDetail <= 4)
                {
                    sprite[searchwall].filler = nDetail;
                    sprintf(buffer, "sprite[%d].detail is %d", searchwall, nDetail);
                    scrSetMessage(buffer);
                    ModifyBeep();
                }
                else
                {
                    sprintf(buffer, "Detail must be between %d and %d", 0, 4);
                    scrSetMessage(buffer);
                    Beep();
                }
            }
        }
        break;
    case sc_E:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingstat ^= 8;
            sprintf(buffer, "ceiling texture %s expanded", (sector[searchsector].ceilingstat & 8) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 2:
            sector[searchsector].floorstat ^= 8;
            sprintf(buffer, "floor texture %s expanded", (sector[searchsector].floorstat & 8) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_F:
        if (alt)
        {
            switch (searchstat)
            {  
            case 0:
            case 4:
            {
                int nSector = sectorofwall(searchwall);
                sector[nSector].ceilingstat &= ~0x34;
                sector[nSector].ceilingstat |= 0x40;
                SetFirstWall(nSector, searchwall);
                ModifyBeep();
                break;
            }
            case 1:
                sector[searchsector].ceilingstat &= ~0x34;
                sector[searchsector].ceilingstat |= 0x40;
                SetFirstWall(searchsector, sector[searchsector].wallptr+1);
                ModifyBeep();
                break;
            case 2:
                sector[searchsector].floorstat &= ~0x34;
                sector[searchsector].floorstat |= 0x40;
                SetFirstWall(searchsector, sector[searchsector].wallptr+1);
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        else if (ctrl)
        {
            gFogMode = !gFogMode;
            scrLoadPLUs();
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                int vc = wall[searchwall].cstat & 0x108;
                switch (vc)
                {
                case 0:
                    vc = 8;
                    break;
                case 8:
                    vc = 0x108;
                    break;
                case 0x100:
                    vc = 0;
                    break;
                case 0x108:
                    vc = 0x100;
                    break;
                }

                wall[searchwall].cstat &= ~0x108;
                wall[searchwall].cstat |= vc;
                sprintf(buffer, "wall[%i]", searchwall);
                if (wall[searchwall].cstat & 0x08)
                    strcat(buffer, " x-flipped");
                if (wall[searchwall].cstat & 0x100)
                {
                    if (wall[searchwall].cstat & 0x08)
                        strcat(buffer, " and");
                    strcat(buffer, " y-flipped");
                }
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            }
            case 1:
            {
                int vc = sector[searchsector].ceilingstat & 0x34;
                switch (vc)
                {
                case 0x00:
                    vc = 0x10;
                    break;
                case 0x10:
                    vc = 0x30;
                    break;
                case 0x30:
                    vc = 0x20;
                    break;
                case 0x20:
                    vc = 0x04;
                    break;
                case 0x04:
                    vc = 0x14;
                    break;
                case 0x14:
                    vc = 0x34;
                    break;
                case 0x34:
                    vc = 0x24;
                    break;
                case 0x24:
                    vc = 0x00;
                    break;
                }
                sector[searchsector].ceilingstat &= ~0x34;
                sector[searchsector].ceilingstat |= vc;
                ModifyBeep();
                break;
            }
            case 2:
            {
                int vc = sector[searchsector].floorstat & 0x34;
                switch (vc)
                {
                case 0x00:
                    vc = 0x10;
                    break;
                case 0x10:
                    vc = 0x30;
                    break;
                case 0x30:
                    vc = 0x20;
                    break;
                case 0x20:
                    vc = 0x04;
                    break;
                case 0x04:
                    vc = 0x14;
                    break;
                case 0x14:
                    vc = 0x34;
                    break;
                case 0x34:
                    vc = 0x24;
                    break;
                case 0x24:
                    vc = 0x00;
                    break;
                }
                sector[searchsector].floorstat &= ~0x34;
                sector[searchsector].floorstat |= vc;
                ModifyBeep();
                break;
            }
            case 0x03:
            {
                int vc = sprite[searchwall].cstat;
                if ((vc&0x30) == 0x20 && !(vc&0x40))
                {
                    sprite[searchwall].cstat &= ~0x08;
                    sprite[searchwall].cstat ^= 0x04;
                }
                else
                {
                    vc &= 0x0c;
                    switch (vc)
                    {
                    case 0x00:
                        vc = 0x04;
                        break;
                    case 0x04:
                        vc = 0x0c;
                        break;
                    case 0x0c:
                        vc = 0x08;
                        break;
                    case 0x08:
                        vc = 0x00;
                        break;
                    }
                    sprite[searchwall].cstat &= ~0x0c;
                    sprite[searchwall].cstat |= vc;
                }
                break;
            }
            }
            ModifyBeep();
        }
        break;
    case sc_G:
        if (alt)
        {
            if (++dword_D9A80 >= 5)
                dword_D9A80 = 0;
            scrSetPalette(dword_D9A80);
            scrSetDac();
            switch (dword_D9A80)
            {
            case 0:
                scrSetMessage("Normal palette");
                break;
            case 1:
                scrSetMessage("Water palette");
                break;
            case 2:
                scrSetMessage("Beast palette");
                break;
            case 3:
                scrSetMessage("Sewer palette");
                break;
            case 4:
                scrSetMessage("Invulnerability palette");
                break;
            }
        }
        break;
    case sc_H:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= 0x40;
            if (wall[searchwall].nextwall >= 0)
            {
                wall[wall[searchwall].nextwall].cstat &= ~0x40;
                wall[wall[searchwall].nextwall].cstat ^= wall[searchwall].cstat & 0x40;
            }
            sprintf(buffer, "wall[%i] %s hitscan sensitive", searchwall, (wall[searchwall].cstat & 0x40) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= 0x100;
            sprintf(buffer, "sprite[%i] %s hitscan sensitive", searchwall, (sprite[searchwall].cstat & 0x100) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_I:
        switch (searchstat)
        {
        case 3:
            sprite[searchwall].cstat ^= 0x8000;
            sprintf(buffer, "sprite[%i] is%s invisible", searchwall, (sprite[searchwall].cstat & 0x8000) ? "" : " not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_L:
        switch (searchstat)
        {
        case 3:
        {
            int top, bottom;
            GetSpriteExtents(&sprite[searchwall], &top, &bottom);
            int bakcstat = sprite[searchwall].cstat;
            sprite[searchwall].cstat &= ~0x100;
            ResetLightBomb();
            DoLightBomb(sprite[searchwall].x, sprite[searchwall].y, top, sprite[searchwall].sectnum);
            sprite[searchwall].cstat = bakcstat;
            ModifyBeep();
            break;
        }
        case 2:
            if (sector[searchsector].ceilingstat & 0x01)
            {
                sector[searchsector].floorstat ^= 0x8000;
                sprintf(buffer, "Forced floor shading is %s", dword_D9A88[(sector[searchsector].floorstat & 0x8000) != 0]);
                scrSetMessage(buffer);
            }
            else
                scrSetMessage("Sky must be parallaxed to force floorshade");
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_M:
        switch (searchstat)
        {
        case 0:
        case 1: // ??
        case 2: // ??
        case 4:
        {
            int nNextWall = wall[searchwall].nextwall;
            if (nNextWall < 0)
            {
                Beep();
                break;
            }
            wall[searchwall].cstat ^= 0x10;
            if (wall[searchwall].cstat & 0x10)
            {
                wall[searchwall].cstat &= ~0x08;
                if (!shift)
                {
                    wall[nNextWall].cstat ^= 0x18;
                    if (wall[searchwall].overpicnum < 0)
                        wall[searchwall].overpicnum = 0;
                    wall[nNextWall].overpicnum = wall[searchwall].overpicnum;
                }
            }
            else
            {
                wall[searchwall].cstat &= ~0x08;
                if (!shift)
                {
                    wall[nNextWall].cstat &= ~0x18;
                }
            }
            wall[searchwall].cstat &= ~0x20;
            if (!shift)
                wall[nNextWall].cstat &= ~0x20;
            sprintf(buffer, "wall[%i] %s masked", searchwall, (wall[searchwall].cstat & 0x10) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case sc_O:
        if (alt)
        {
            //asksave = ShowOptions();
            asksave |= ShowOptions();
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                wall[searchwall].cstat ^= 0x04;
                if (wall[searchwall].nextwall == -1)
                {
                    sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & 0x04) ? "bottom" : "top");
                }
                else
                {
                    sprintf(buffer, "Texture pegged at %s", (wall[searchwall].cstat & 0x04) ? "outside" : "inside");
                }
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            case 3:
            {
                if (sector[sprite[searchwall].sectnum].floorz < sprite[searchwall].z)
                {
                    scrSetMessage("Sprite z is below floor");
                    Beep();
                    break;
                }
                if (sector[sprite[searchwall].sectnum].ceilingz > sprite[searchwall].z)
                {
                    scrSetMessage("Sprite z is above ceiling");
                    Beep();
                    break;
                }
                int va = HitScan(&sprite[searchwall], sprite[searchwall].z, Cos(sprite[searchwall].ang+1024)>>16, Sin(sprite[searchwall].ang+1024)>>16, 0, 0, 0);
                if (va == 0 || va == 4)
                {
                    int nx, ny;
                    GetWallNormal(gHitInfo.hitwall, &nx, &ny);
                    sprite[searchwall].x = gHitInfo.hitx + (nx>>14);
                    sprite[searchwall].y = gHitInfo.hity + (ny>>14);
                    sprite[searchwall].z = gHitInfo.hitz;

                    sprite[searchwall].cstat &= ~0x01;
                    sprite[searchwall].cstat |= 0x40;

                    ChangeSpriteSect(searchwall, gHitInfo.hitsect);

                    sprite[searchwall].ang = (GetWallAngle(gHitInfo.hitwall)+512)&2047;
                    ModifyBeep();
                }
                else
                    Beep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        break;
    case sc_P:
        if (ctrl)
        {
            if (++parallaxtype >= 3)
                parallaxtype = 0;
            sprintf(buffer, "Parallax type: %i", parallaxtype);
            scrSetMessage(buffer);
            ModifyBeep();
        }
        else if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
                wall[searchwall].pal = GetNumberBox("Wall palookup", wall[searchwall].pal, wall[searchwall].pal);
                break;
            case 1:
                sector[searchsector].ceilingpal = GetNumberBox("Ceiling palookup", sector[searchsector].ceilingpal, sector[searchsector].ceilingpal);
                break;
            case 2:
                sector[searchsector].floorpal = GetNumberBox("Floor palookup", sector[searchsector].floorpal, sector[searchsector].floorpal);
                break;
            case 3:
                sprite[searchwall].pal = GetNumberBox("Sprite palookup", sprite[searchwall].pal, sprite[searchwall].pal);
                break;
            default:
                break;
            }
            ModifyBeep();
        }
        else
        {
            switch (searchstat)
            {
            case 1:
                sector[searchsector].ceilingstat ^= 0x01;
                sprintf(buffer, "sector[%i] ceiling %s parallaxed", searchsector, (sector[searchsector].ceilingstat & 0x01) ? "is" : "not");
                scrSetMessage(buffer);
                if (sector[searchsector].ceilingstat & 0x01)
                    SetSkyTile(sector[searchsector].ceilingpicnum);
                else
                    sector[searchsector].floorstat &= ~0x8000;
                ModifyBeep();
                break;
            case 2:
                sector[searchsector].floorstat ^= 0x01;
                sprintf(buffer, "sector[%i] floor %s parallaxed", searchsector, (sector[searchsector].floorstat & 0x01) ? "is" : "not");
                scrSetMessage(buffer);
                ModifyBeep();
                break;
            default:
                Beep();
                break;
            }
        }
        break;
    case sc_R:
        switch (searchstat)
        {
        case 1:
            sector[searchsector].ceilingstat ^= 0x40;
            sprintf(buffer, "sector[%i] ceiling %s relative", searchsector, (sector[searchsector].ceilingstat & 0x40) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 2:
            sector[searchsector].floorstat ^= 0x40;
            sprintf(buffer, "sector[%i] floor %s relative", searchsector, (sector[searchsector].floorstat & 0x40) ? "is" : "not");
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
        {
            int vc = sprite[searchwall].cstat & 0x30;
            switch (vc)
            {
            case 0x00:
                sprintf(buffer, "sprite[%i] is wall sprite", searchwall);
                vc = 0x10;
                break;
            case 0x10:
                sprintf(buffer, "sprite[%i] is floor sprite", searchwall);
                vc = 0x20;
                break;
            case 0x20:
                sprintf(buffer, "sprite[%i] is face sprite", searchwall);
                vc = 0x00;
                break;
            default:
                sprintf(buffer, "sprite[%i] is face sprite", searchwall);
                vc = 0x00;
                break;
            }
            scrSetMessage(buffer);
            sprite[searchwall].cstat &= ~0x30;
            sprite[searchwall].cstat |= vc;
            ModifyBeep();
            break;
        }
        default:
            Beep();
            break;
        }
        break;
    case sc_S:
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                int x = 0x4000;
                int y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                short hitsect, hitwall, hitsprite;
                int hitx, hity, hitz;
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (scale(searchy, 200, ydim)-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitwall == searchwall)
                {
                    if (hitsect < 0)
                    {
                        Beep();
                        break;
                    }
                    x = hitx;
                    y = hity;
                    if (word_CA8A0 > 0 && gGrid > 0)
                    {
                        x = (hitx + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                        y = (hity + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                    }
                    int nSprite = InsertObject(searchstat, hitsect, x, y, hitz, ang);
                    if (nSprite >= 0)
                    {
                        sprite[nSprite].ang = (GetWallAngle(hitwall)+512)&2047;
                        int nx, ny;
                        GetWallNormal(hitwall, &nx, &ny);
                        sprite[nSprite].x = hitx + (nx >> 14);
                        sprite[nSprite].y = hity + (ny >> 14);
                        sprite[nSprite].z = hitz;
                        sprite[nSprite].cstat |= 0x50;
                        updatenumsprites();
                        asksave = 1;
                        ModifyBeep();
                    }
                }
                break;
            }
            case 1:
            case 2:
            {
                int x = 0x4000;
                int y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                short hitsect, hitwall, hitsprite;
                int hitx, hity, hitz;
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (scale(searchy, 200, ydim)-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitsect < 0)
                {
                    Beep();
                    break;
                }
                x = hitx;
                y = hity;
                if (word_CA8A0 > 0 && gGrid > 0)
                {
                    x = (hitx + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                    y = (hity + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                }
                int nSprite = InsertObject(searchstat, hitsect, x, y, hitz, ang);
                if (nSprite >= 0)
                {
                    asksave = 1;
                    ModifyBeep();
                }
                break;
            }
            default:
                Beep();
                break;
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            case 4:
            {
                int x = 0x4000;
                int y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                short hitsect, hitwall, hitsprite;
                int hitx, hity, hitz;
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (scale(searchy, 200, ydim)-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitwall == searchwall)
                {
                    int v64;
                    if (somethingintab != 3)
                    {
                        v64 = tilePick(-1, -1, 5);
                        if (v64 == -1)
                            break;
                    }
                    int nSprite = InsertSprite(searchsector, 0);
                    sprite[nSprite].ang = (GetWallAngle(hitwall)+512)&2047;
                    int nx, ny;
                    GetWallNormal(hitwall, &nx, &ny);
                    sprite[nSprite].x = hitx + (nx >> 14);
                    sprite[nSprite].y = hity + (ny >> 14);
                    sprite[nSprite].z = hitz;

                    if (somethingintab == 3)
                    {
                        sprite[nSprite].picnum = temppicnum;
                        sprite[nSprite].shade = tempshade;
                        sprite[nSprite].pal = temppal;
                        sprite[nSprite].xrepeat = tempxrepeat;
                        sprite[nSprite].yrepeat = tempyrepeat;
                        if (sprite[nSprite].xrepeat < 1)
                            sprite[nSprite].xrepeat = 1;
                        if (sprite[nSprite].yrepeat < 1)
                            sprite[nSprite].yrepeat = 1;

                        tempcstat &= ~0x30;
                        sprite[nSprite].cstat = tempcstat | 0x50;
                        updatenumsprites();
                        ModifyBeep();
                        break;
                    }

                    sprite[nSprite].cstat |= 0x50;
                    sprite[nSprite].picnum = v64;
                    sprite[nSprite].shade = -8;
                    sprite[nSprite].pal = 0;
                    if (tilesizy[sprite[nSprite].picnum] >= 32)
                        sprite[nSprite].cstat |= 1;
                    updatenumsprites();
                    ModifyBeep();
                }
                break;
            }
            case 1:
            case 2:
            {
                int x = 0x4000;
                int y = divscale(searchx-xdim/2, xdim/2, 14);
                RotateVector(&x, &y, ang);
                short hitsect, hitwall, hitsprite;
                int hitx, hity, hitz;
                hitsect = hitwall = hitsprite = -1;
                hitscan(posx, posy, posz, cursectnum, x, y, (scale(searchy, 200, ydim)-horiz)*2000, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, 0);
                if (hitsect < 0)
                {
                    Beep();
                    break;
                }
                x = hitx;
                y = hity;
                if (word_CA8A0 > 0 && gGrid > 0)
                {
                    x = (hitx + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                    y = (hity + (0x400 >> gGrid)) & (0xffffffff << (11-gGrid));
                }
                int v64;
                if (somethingintab != 3)
                {
                    v64 = tilePick(-1, -1, 3);
                    if (v64 == -1)
                        break;
                }
                int nSprite = InsertSprite(searchsector, 0);
                sprite[nSprite].x = x;
                sprite[nSprite].y = y;
                sprite[nSprite].z = hitz;

                if (somethingintab == 3)
                {
                    sprite[nSprite].picnum = temppicnum;
                    sprite[nSprite].shade = tempshade;
                    sprite[nSprite].pal = temppal;
                    sprite[nSprite].xrepeat = tempxrepeat;
                    sprite[nSprite].yrepeat = tempyrepeat;
                    sprite[nSprite].ang = tempang;
                    if (sprite[nSprite].xrepeat < 1)
                        sprite[nSprite].xrepeat = 1;
                    if (sprite[nSprite].yrepeat < 1)
                        sprite[nSprite].yrepeat = 1;

                    sprite[nSprite].cstat = tempcstat;
                }
                else
                {
                    sprite[nSprite].picnum = v64;
                    sprite[nSprite].shade = -8;
                    sprite[nSprite].pal = 0;
                    if (tilesizy[sprite[nSprite].picnum] >= 32)
                        sprite[nSprite].cstat |= 1;
                }
                int top, bottom;
                GetSpriteExtents(&sprite[nSprite], &top, &bottom);
                if (searchstat == 2)
                    sprite[nSprite].z += getflorzofslope(hitsect, x, y) - bottom;
                else
                    sprite[nSprite].z += getceilzofslope(hitsect, x, y) - top;
                sub_10EA0();
                updatenumsprites();
                ModifyBeep();
                break;
            }
            default:
                Beep();
                break;
            }
        }
        break;
    case sc_T:
        switch (searchstat)
        {
        case 4:
        {
            int vd = 0;
            if (wall[searchwall].cstat & 0x80)
                vd = 2;
            if (wall[searchwall].cstat & 0x200)
                vd = 1;
            if (++vd >= 3)
                vd = 0;
            switch (vd)
            {
            case 0:
                wall[searchwall].cstat &= ~0x280;
                break;
            case 1:
                wall[searchwall].cstat |= 0x280;
                break;
            case 2:
                wall[searchwall].cstat &= ~0x280;
                wall[searchwall].cstat |= 0x80;
                break;
            }
            if (wall[searchwall].nextwall >= 0)
            {
                wall[wall[searchwall].nextwall].cstat &= ~0x280;
                wall[wall[searchwall].nextwall].cstat |= wall[searchwall].cstat & 0x280;
            }
            sprintf(buffer, "wall[%i] translucent type %d", searchwall, vd);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        case 3:
        {
            int vd = 0;
            if (sprite[searchwall].cstat & 0x02)
                vd = 2;
            if (sprite[searchwall].cstat & 0x200)
                vd = 1;
            if (++vd >= 3)
                vd = 0;
            switch (vd)
            {
            case 0:
                sprite[searchwall].cstat &= ~0x202;
                break;
            case 1:
                sprite[searchwall].cstat |= 0x202;
                break;
            case 2:
                sprite[searchwall].cstat &= ~0x202;
                sprite[searchwall].cstat |= 0x02;
                break;
            }
            sprintf(buffer, "sprite[%i] translucent type %d", searchwall, vd);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        }
        }
        break;
    case sc_U:
        for (int i = 0; i < numsectors; i++)
        {
            sector[i].visibility = 0;
        }
        scrSetMessage("All sector visibility values set to 0");
        break;
    case sc_V:
        switch (searchstat)
        {
        case 0:
        {
            short vc = wall[searchwall].picnum;
            wall[searchwall].picnum = tilePick(wall[searchwall].picnum, wall[searchwall].picnum, 0);
            vel = svel = angvel = 0;
            if (vc != wall[searchwall].picnum)
                ModifyBeep();
            break;
        }
        case 1:
        {
            short vc = sector[searchsector].ceilingpicnum;
            sector[searchsector].ceilingpicnum = tilePick(sector[searchsector].ceilingpicnum, sector[searchsector].ceilingpicnum, 1);
            vel = svel = angvel = 0;
            if (sector[searchsector].ceilingstat & 0x01)
                SetSkyTile(sector[searchsector].ceilingpicnum);
            else
                sector[searchsector].floorstat &= ~0x8000;
            if (vc != sector[searchsector].ceilingpicnum)
                ModifyBeep();
            break;
        }
        case 2:
        {
            short vc = sector[searchsector].floorpicnum;
            sector[searchsector].floorpicnum = tilePick(sector[searchsector].floorpicnum, sector[searchsector].floorpicnum, 2);
            vel = svel = angvel = 0;
            if (vc != sector[searchsector].floorpicnum)
                ModifyBeep();
            break;
        }
        case 3:
        {
            if (sprite[searchwall].cstat & 0x30)
                searchstat = 5;
            short vc = sprite[searchwall].picnum;
            sprite[searchwall].picnum = tilePick(sprite[searchwall].picnum, sprite[searchwall].picnum, searchstat);
            int top, bottom;
            GetSpriteExtents(&sprite[searchwall], &top, &bottom);
            if (!(sector[sprite[searchwall].sectnum].ceilingstat & 0x01))
                sprite[searchwall].z += ClipLow(sector[sprite[searchwall].sectnum].ceilingz - top, 0);
            if (!(sector[sprite[searchwall].sectnum].floorstat & 0x01))
                sprite[searchwall].z += ClipHigh(sector[sprite[searchwall].sectnum].floorz - bottom, 0);
            vel = svel = angvel = 0;
            if (vc != sector[searchsector].floorpicnum)
                ModifyBeep();
            break;
        }
        case 4:
        {
            short vc = wall[searchwall].overpicnum;
            wall[searchwall].overpicnum = tilePick(wall[searchwall].overpicnum, wall[searchwall].overpicnum, 4);
            vel = svel = angvel = 0;
            if (wall[searchwall].nextwall >= 0)
                wall[wall[searchwall].nextwall].overpicnum = wall[searchwall].overpicnum;
            if (vc != wall[searchwall].overpicnum)
                ModifyBeep();
            break;
        }
        }
        break;
    case sc_W:
    {
        int nXSector = sub_10DBC(searchsector);
        int wf = xsector[nXSector].wave;
        do
        {
            if (++wf >= 15)
                wf = 0;
        } while (!WaveForm[wf]);
        OperateSectors(sub_2611C, wf);
        scrSetMessage(WaveForm[wf]);
        break;
    }
    case sc_kpad_Minus:
    {
        int vd;
        if (ctrl)
            vd = 0x100;
        else
            vd = 0x01;
        if (IsSectorHighlight(searchsector))
        {
            for (int i = 0; i < highlightsectorcnt; i++)
            {
                int nSector = highlightsector[i];
                sector[nSector].ceilingshade = ClipHigh(sector[nSector].ceilingshade+vd, 63);
                sector[nSector].floorshade = ClipHigh(sector[nSector].floorshade+vd, 63);
                int startwall = sector[nSector].wallptr;
                int endwall = startwall + sector[nSector].wallnum - 1;
                for (int j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = ClipHigh(wall[j].shade+vd, 63);
                }
            }
        }
        else
        {
            signed char v14;
            switch (searchstat)
            {
            case 0:
                v14 = wall[searchwall].shade = ClipHigh(wall[searchwall].shade+vd, 63);
                break;
            case 1:
                v14 = sector[searchsector].ceilingshade = ClipHigh(sector[searchsector].ceilingshade+vd, 63);
                break;
            case 2:
                v14 = sector[searchsector].floorshade = ClipHigh(sector[searchsector].floorshade+vd, 63);
                break;
            case 3:
                v14 = sprite[searchwall].shade = ClipHigh(sprite[searchwall].shade+vd, 63);
                break;
            case 4:
                v14 = wall[searchwall].shade = ClipHigh(wall[searchwall].shade+vd, 63);
                break;
            }
            sprintf(buffer, "shade: %i", v14);
            scrSetMessage(buffer);
        }
        ModifyBeep();
        break;
    }
    case sc_kpad_Plus:
    {
        int vd;
        if (ctrl)
            vd = 0x100;
        else
            vd = 0x01;
        if (IsSectorHighlight(searchsector))
        {
            for (int i = 0; i < highlightsectorcnt; i++)
            {
                int nSector = highlightsector[i];
                sector[nSector].ceilingshade = ClipLow(sector[nSector].ceilingshade-vd, -128);
                sector[nSector].floorshade = ClipLow(sector[nSector].floorshade-vd, -128);
                int startwall = sector[nSector].wallptr;
                int endwall = startwall + sector[nSector].wallnum - 1;
                for (int j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = ClipLow(wall[j].shade-vd, -128);
                }
            }
        }
        else
        {
            signed char v14;
            switch (searchstat)
            {
            case 0:
                v14 = wall[searchwall].shade = ClipLow(wall[searchwall].shade-vd, -128);
                break;
            case 1:
                v14 = sector[searchsector].ceilingshade = ClipLow(sector[searchsector].ceilingshade-vd, -128);
                break;
            case 2:
                v14 = sector[searchsector].floorshade = ClipLow(sector[searchsector].floorshade-vd, -128);
                break;
            case 3:
                v14 = sprite[searchwall].shade = ClipLow(sprite[searchwall].shade-vd, -128);
                break;
            case 4:
                v14 = wall[searchwall].shade = ClipLow(wall[searchwall].shade-vd, -128);
                break;
            }
            sprintf(buffer, "shade: %i", v14);
            scrSetMessage(buffer);
        }
        ModifyBeep();
        break;
    }
    case sc_kpad_0:
        if (IsSectorHighlight(searchsector))
        {
            for (int i = 0; i < highlightsectorcnt; i++)
            {
                int nSector = highlightsector[i];
                sector[nSector].ceilingshade = 0;
                sector[nSector].floorshade = 0;
                int startwall = sector[nSector].wallptr;
                int endwall = startwall + sector[nSector].wallnum - 1;
                for (int j = startwall; j <= endwall; j++)
                {
                    wall[j].shade = 0;
                }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
                wall[searchwall].shade = 0;
                break;
            case 1:
                sector[searchsector].ceilingshade = 0;
                break;
            case 2:
                sector[searchsector].floorshade = 0;
                break;
            case 3:
                sprite[searchwall].shade = 0;
                break;
            case 4:
                wall[searchwall].shade = 0;
                break;
            }
        }
        ModifyBeep();
        break;
    case sc_kpad_4:
    case sc_kpad_6:
    {
        char vbl, val;
        if (gOldKeyMapping)
            vbl = kp5;
        else
            vbl = !shift;
        if (gOldKeyMapping)
            val = shift;
        else
            val = ctrl;
        int vd;
        if (key == sc_kpad_4)
            vd = 1;
        else
            vd = -1;
        switch (searchstat)
        {
        case 0:
        case 4:
            if (val)
            {
                wall[searchwall].xpanning = changechar(wall[searchwall].xpanning, vd, vbl, 0);
                sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall, wall[searchwall].xpanning, wall[searchwall].ypanning);
                scrSetMessage(buffer);
            }
            else
            {
                wall[searchwall].xrepeat = changechar(wall[searchwall].xrepeat, vd, vbl, 1);
                sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
                scrSetMessage(buffer);
            }
            break;
        case 1:
            sector[searchsector].ceilingxpanning = changechar(sector[searchsector].ceilingxpanning, vd, vbl, 0);
            break;
        case 2:
            sector[searchsector].floorxpanning = changechar(sector[searchsector].floorxpanning, vd, vbl, 0);
            break;
        case 3:
            if (val)
            {
                sprite[searchwall].xoffset = changechar(sprite[searchwall].xoffset, vd, vbl, 0);
                sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
                scrSetMessage(buffer);
            }
            else
            {
                sprite[searchwall].xrepeat = ClipLow(changechar(sprite[searchwall].xrepeat, vd, -vbl, 1), 4);
                sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
                scrSetMessage(buffer);
            }
            break;
        }
        ModifyBeep();
        break;
    }
    case sc_kpad_2:
    case sc_kpad_8:
    {
        char vbl, val;
        if (gOldKeyMapping)
            vbl = kp5;
        else
            vbl = !shift;
        if (gOldKeyMapping)
            val = shift;
        else
            val = ctrl;
        int vd;
        if (key == sc_kpad_8)
            vd = -1;
        else
            vd = 1;
        switch (searchstat)
        {
        case 0:
        case 4:
            if (val)
            {
                wall[searchwall].ypanning = changechar(wall[searchwall].ypanning, vd, vbl, 0);
                sprintf(buffer, "wall %i xpanning: %i ypanning: %i", searchwall, wall[searchwall].xpanning, wall[searchwall].ypanning);
                scrSetMessage(buffer);
            }
            else
            {
                wall[searchwall].yrepeat = changechar(wall[searchwall].yrepeat, vd, vbl, 1);
                sprintf(buffer, "wall %i xrepeat: %i yrepeat: %i", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
                scrSetMessage(buffer);
            }
            break;
        case 1:
            sector[searchsector].ceilingypanning = changechar(sector[searchsector].ceilingypanning, vd, vbl, 0);
            break;
        case 2:
            sector[searchsector].floorypanning = changechar(sector[searchsector].floorypanning, vd, vbl, 0);
            break;
        case 3:
            if (val)
            {
                sprite[searchwall].yoffset = changechar(sprite[searchwall].yoffset, vd, vbl, 0);
                sprintf(buffer, "sprite %i xoffset: %i yoffset: %i", searchwall, sprite[searchwall].xoffset, sprite[searchwall].yoffset);
                scrSetMessage(buffer);
            }
            else
            {
                sprite[searchwall].yrepeat = ClipLow(changechar(sprite[searchwall].yrepeat, vd, -vbl, 1), 4);
                sprintf(buffer, "sprite %i xrepeat: %i yrepeat: %i", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
                scrSetMessage(buffer);
            }
            break;
        }
        ModifyBeep();
        break;
    }
    case sc_kpad_Enter:
        overheadeditor();
        clearview(0);
        scrNextPage();
        keyFlushScans();
        sub_1058C();
        break;
    case sc_F2:
        switch (searchstat)
        {
        case 0:
        case 4:
        {
            int nXWall = wall[searchwall].extra;
            if (nXWall <= 0)
            {
                Beep();
                break;
            }
            xwall[nXWall].state ^= 1;
            xwall[nXWall].busy = xwall[nXWall].state << 16;
            scrSetMessage(dword_D9A88[xwall[nXWall].state]);
            ModifyBeep();
            break;
        }
        case 1:
        case 2:
        {
            int nXSector = sector[searchsector].extra;
            if (nXSector <= 0)
            {
                Beep();
                break;
            }
            xsector[nXSector].state ^= 1;
            xsector[nXSector].busy = xsector[nXSector].state << 16;
            scrSetMessage(dword_D9A88[xsector[nXSector].state]);
            ModifyBeep();
            break;
        }
        case 3:
        {
            int nXSprite = sprite[searchwall].extra;
            if (nXSprite <= 0)
            {
                Beep();
                break;
            }
            xsprite[nXSprite].state ^= 1;
            xsprite[nXSprite].busy = xsprite[nXSprite].state << 16;
            scrSetMessage(dword_D9A88[xsprite[nXSprite].state]);
            ModifyBeep();
            break;
        }
        }
        break;
    case sc_F3:
    {
        int va = searchsector;
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    va = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                int nXSector = sector[va].extra;
                if (nXSector > 0)
                {
                    xsector[nXSector].offFloorZ = sector[va].floorz;
                    xsector[nXSector].offCeilZ = sector[va].ceilingz;
                    sprintf(buffer, "SET offFloorZ= %i  offCeilZ= %i", xsector[nXSector].offFloorZ, xsector[nXSector].offCeilZ);
                    scrSetMessage(buffer);
                    ModifyBeep();
                    asksave = 1;
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    va = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                int nXSector = sector[va].extra;
                if (nXSector > 0)
                {
                    switch (sector[va].lotag)
                    {
                    case 600:
                    case 614:
                    case 615:
                    case 616:
                    case 617:
                        sector[va].floorz = xsector[nXSector].offFloorZ;
                        sector[va].ceilingz = xsector[nXSector].offCeilZ;
                        sprintf(buffer, "SET floorz= %i  ceilingz= %i", sector[va].floorz, sector[va].ceilingz);
                        scrSetMessage(buffer);
                        ModifyBeep();
                        asksave = 1;
                        break;
                    }
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        break;
    }
    case sc_F4:
    {
        int va = searchsector;
        if (alt)
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    va = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                int nXSector = sector[va].extra;
                if (nXSector > 0)
                {
                    xsector[nXSector].onFloorZ = sector[va].floorz;
                    xsector[nXSector].onCeilZ = sector[va].ceilingz;
                    sprintf(buffer, "SET onFloorZ= %i  onCeilZ= %i", xsector[nXSector].onFloorZ, xsector[nXSector].onCeilZ);
                    scrSetMessage(buffer);
                    ModifyBeep();
                    asksave = 1;
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        else
        {
            switch (searchstat)
            {
            case 0:
            {
                if (wall[searchwall].nextsector != -1)
                    va = wall[searchwall].nextsector;
            }
            case 1:
            case 2:
            {
                int nXSector = sector[va].extra;
                if (nXSector > 0)
                {
                    switch (sector[va].lotag)
                    {
                    case 600:
                    case 614:
                    case 615:
                    case 616:
                    case 617:
                        sector[va].floorz = xsector[nXSector].onFloorZ;
                        sector[va].ceilingz = xsector[nXSector].onCeilZ;
                        sprintf(buffer, "SET floorz= %i  ceilingz= %i", sector[va].floorz, sector[va].ceilingz);
                        scrSetMessage(buffer);
                        ModifyBeep();
                        asksave = 1;
                        break;
                    }
                }
                else
                {
                    scrSetMessage("Sector type must first be set in 2D mode.");
                    Beep();
                }
                break;
            }
            }
        }
        break;
    }
    case sc_F9:
        //asksave = ShowSectorTricks();
        asksave |= ShowSectorTricks();
        break;
    case sc_F11:
        word_CA89C = !word_CA89C;
        sprintf(buffer, "Global panning is %s", dword_D9A88[word_CA89C]);
        scrSetMessage(buffer);
        ModifyBeep();
        break;
    case sc_F12:
        gBeep = !gBeep;
        sprintf(buffer, "Beeps are %s", dword_D9A88[gBeep]);
        scrSetMessage(buffer);
        ModifyBeep();
        break;
    case sc_1:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= 0x20;
            sprintf(buffer, "Wall %i one-way flag is %s", searchwall, dword_D9A88[(wall[searchwall].cstat & 0x20) != 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        case 3:
            sprite[searchwall].cstat ^= 0x40;
            if ((sprite[searchwall].cstat & 0x30) == 0x20)
            {
                sprite[searchwall].cstat &= ~0x08;
                if (sprite[searchwall].cstat & 0x40)
                {
                    if (posz > sprite[searchwall].z)
                    {
                        sprite[searchwall].cstat |= 0x08;
                    }
                }
            }
            sprintf(buffer, "Sprite %i one-sided flag is %s", searchwall, dword_D9A88[(sprite[searchwall].cstat & 0x40) != 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_2:
        switch (searchstat)
        {
        case 0:
        case 4:
            wall[searchwall].cstat ^= 0x02;
            sprintf(buffer, "Wall %i bottom swap flag is %s", searchwall, dword_D9A88[(wall[searchwall].cstat & 0x02) != 0]);
            scrSetMessage(buffer);
            ModifyBeep();
            break;
        default:
            Beep();
            break;
        }
        break;
    case sc_PrintScreen:
        screencapture("captxxxx.tga",keystatus[0x2a]|keystatus[0x36]);
        ModifyBeep();
        break;
    }
    if (key)
        keyFlushScans();
}

//////////////// bstub /////////////////



IniFile gMapEditIni("MAPEDIT.INI");

struct TextStruct {
    short id;
    const char* text;
};

TextStruct stru_CA9BC[] = {
    { 0, "Off" },
    { 1, "On" },
};

TextStruct stru_CA9C8[] = {
    { 0, "Decoration" },
    { 1, "Player Start" },
    { 2, "Bloodbath Start" },
    { 3, "Off marker" },
    { 4, "On marker" },
    { 5, "Axis marker" },

    { 6, "Lower link" },
    { 7, "Upper link" },
    { 8, "Teleport target" },
    { 10, "Lower water" },
    { 9, "Upper water" },
    { 12, "Lower stack" },
    { 11, "Upper stack" },
    { 14, "Lower goo" },
    { 13, "Upper goo" },
    { 15, "Path marker" },
    { 16, "Alignable Region" },
    { 17, "Base Region" },
    { 18, "Dude Spawn" },
    { 19, "Earthquake" },

    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 22, "Combination switch" },
    { 23, "Padlock (1-shot)" },

    { 30, "Torch" },
    { 32, "Candle" },

    { 40, gWeaponText[0] },
    { 47, gWeaponText[7] },
    { 43, gWeaponText[3] },
    { 41, gWeaponText[1] },
    { 42, gWeaponText[2] },
    { 46, gWeaponText[6] },
    { 49, gWeaponText[9] },
    { 48, gWeaponText[8] },
    { 45, gWeaponText[5] },
    { 50, gWeaponText[10] },
    { 44, gWeaponText[4] },

    { 60, gAmmoText[0] },
    { 62, gAmmoText[2] },
    { 63, gAmmoText[3] },
    { 64, gAmmoText[4] },
    { 65, gAmmoText[5] },
    { 67, gAmmoText[7] },
    { 68, gAmmoText[8] },
    { 69, gAmmoText[9] },
    { 70, gAmmoText[10] },
    { 72, gAmmoText[12] },
    { 73, gAmmoText[13] },
    { 76, gAmmoText[16] },
    { 79, gAmmoText[19] },
    { 66, gAmmoText[6] },
    { 80, "Random Ammo" },

    { 100, gItemText[0] },
    { 101, gItemText[1] },
    { 102, gItemText[2] },
    { 103, gItemText[3] },
    { 104, gItemText[4] },
    { 105, gItemText[5] },
    { 106, gItemText[6] },
    { 107, gItemText[7] },
    { 108, gItemText[8] },
    { 109, gItemText[9] },
    { 110, gItemText[10] },
    { 111, gItemText[11] },
    { 112, gItemText[12] },
    { 113, gItemText[13] },
    { 114, gItemText[14] },
    { 115, gItemText[15] },
    { 116, gItemText[16] },
    { 117, gItemText[17] },
    { 118, gItemText[18] },
    { 119, gItemText[19] },
    { 120, gItemText[20] },
    { 121, gItemText[21] },
    { 122, gItemText[22] },
    { 123, gItemText[23] },
    { 124, gItemText[24] },
    { 125, gItemText[25] },
    { 126, gItemText[26] },
    { 127, gItemText[27] },
    { 128, gItemText[28] },
    { 129, gItemText[29] },
    { 130, gItemText[30] },
    { 131, gItemText[31] },
    { 132, gItemText[32] },
    { 133, gItemText[33] },
    { 134, gItemText[34] },
    { 135, gItemText[35] },
    { 136, gItemText[36] },
    { 137, gItemText[37] },
    { 138, gItemText[38] },
    { 139, gItemText[39] },
    { 140, gItemText[40] },
    { 141, gItemText[41] },
    { 142, gItemText[42] },
    { 143, gItemText[43] },
    { 144, gItemText[44] },
    { 145, gItemText[45] },
    { 146, gItemText[46] },

    { 201, "Cultist w/Tommy" },
    { 202, "Cultist w/Shotgun" },
    { 247, "Cultist w/Tesla" },
    { 248, "Cultist w/Dynamite" },
    { 249, "Beast Cultist" },
    { 250, "Tiny Caleb" },
    { 251, "Beast" },
    { 203, "Axe Zombie" },
    { 204, "Fat Zombie" },
    { 205, "Earth Zombie" },
    { 244, "Sleep Zombie" },
    { 245, "Innocent" },
    { 206, "Flesh Gargoyle" },
    { 207, "Stone Gargoyle" },
    { 208, "Flesh Statue" },
    { 209, "Stone Statue" },
    { 210, "Phantasm" },
    { 211, "Hound" },
    { 212, "Hand" },
    { 213, "Brown Spider" },
    { 214, "Red Spider" },
    { 216, "Mother Spider" },
    { 215, "Black Spider" },
    { 217, "GillBeast" },
    { 218, "Eel" },
    { 219, "Bat" },
    { 220, "Rat" },
    { 221, "Green Pod" },
    { 222, "Green Tentacle" },
    { 223, "Fire Pod" },
    { 224, "Fire Tentacle" },
    { 227, "Cerberus" },
    { 229, "Tchernobog" },
    { 230, "TCultist prone" },
    { 246, "SCultist prone" },

    { 400, "TNT Barrel" },
    { 401, "Armed Prox Bomb" },
    { 402, "Armed Remote" },
    { 403, "Blue Vase" },
    { 404, "Brown Vase" },
    { 405, "Crate Face" },
    { 406, "Glass Window" },
    { 407, "Fluorescent Light" },
    { 408, "Wall Crack" },
    { 409, "Wood Beam" },
    { 410, "Spider's Web" },
    { 411, "MetalGrate1" },
    { 412, "FlammableTree" },
    { 413, "Machine Gun" },
    { 414, "Falling Rock" },
    { 415, "Kickable Pail" },
    { 416, "Gib Object" },
    { 417, "Explode Object" },
    { 427, "Zombie Head" },

    { 450, "Spike Trap" },
    { 451, "Rock Trap" },
    { 452, "Flame Trap" },
    { 454, "Saw Blade" },
    { 455, "Electric Zap" },
    { 456, "Switched Zap" },
    { 457, "Pendulum" },
    { 458, "Guillotine" },
    { 459, "Hidden Exploder" },

    { 700, "Trigger Gen" },
    { 701, "WaterDrip Gen" },
    { 702, "BloodDrip Gen" },
    { 703, "Fireball Gen" },
    { 704, "EctoSkull Gen" },
    { 705, "Dart Gen" },
    { 706, "Bubble Gen" },
    { 707, "Multi-Bubble Gen" },
    { 708, "SFX Gen" },
    { 709, "Sector SFX" },
    { 710, "Ambient SFX" },
    { 711, "Player SFX" },
};

TextStruct stru_CADDC[] = {
    { 0, "Normal" },
    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 500, "Wall Link" },
    { 501, "Wall Stack (unsupp.)" },
    { 511, "Gib Wall" },
};

TextStruct stru_CAE00[] = {
    { 0, "Normal" },
    { 600, "Z Motion" },
    { 602, "Z Motion SPRITE" },
    { 603, "Warp" },
    { 604, "Teleporter" },
    { 614, "Slide Marked" },
    { 615, "Rotate Marked" },
    { 616, "Slide" },
    { 617, "Rotate" },
    { 613, "Step Rotate" },
    { 612, "Path Sector" },
    { 618, "Damage Sector" },
    { 619, "Counter Sector" },
};

TextStruct stru_CAE50[] = {
    { 1, "Linear" },
    { 0, "Sine" },
    { 2, "SlowOff"},
    { 3, "SlowOn"},
};

TextStruct stru_CAE68[] = {
    { 0, "None" },
    { 1, "Square" },
    { 2, "Saw" },
    { 3, "Ramp up" },
    { 4, "Ramp down" },
    { 5, "Sine" },
    { 6, "Flicker1" },
    { 7, "Flicker2" },
    { 8, "Flicker3" },
    { 9, "Flicker4" },
    { 10, "Strobe" },
    { 11, "Search" },
};

TextStruct stru_CAEB0[] = {
    { 0, "OFF" },
    { 1, "ON" },
    { 2, "State" },
    { 3, "Toggle" },
    { 4, "!State" },
    { 5, "Link" },
    { 6, "Lock" },
    { 7, "Unlock" },
    { 8, "Toggle Lock" },
    { 9, "Stop OFF" },
    { 10, "Stop ON" },
    { 11, "Stop Next" },
};

TextStruct stru_CAEF8[] = {
    { 0, "Optional" },
    { 1, "Never" },
    { 2, "Always" },
    { 3, "Permanent" },
};

void Sleep(int n)
{
    int t = n + totalclock;
    while (totalclock < t) {};
}

unsigned char *beep1, *beep2;
int beeplen1, beeplen2;
int beepfreq;

void MakeBeepSounds(void)
{
    beepfreq = MixRate;
    // ModifyBeep
    beeplen1 = 2 * beepfreq / 120;
    beep1 = (unsigned char*)malloc(beeplen1);
    for (int i = 0; i < beeplen1; i++)
    {
        int amm = mulscale30(127, Sin(i * (6000 * 2048 / beepfreq)));
        int rest = beeplen1 - i - 1;
        if (rest < 32)
            amm = mulscale5(amm, rest);
        beep1[i] = 128 + amm;
    }
     
    // Beep
    beeplen2 = 8 * beepfreq / 120;
    beep2 = (unsigned char*)malloc(beeplen2);
    for (int i = 0; i < beeplen2 / 2; i++)
    {
        if (((i * 2000) / beepfreq) % 2 == 0)
            beep2[i] = 255;
        else
            beep2[i] = 0;
    }
    for (int i = 0; i < beeplen2 / 2; i++)
    {
        if (((i * 1600) / beepfreq) % 2 == 0)
            beep2[i+beeplen2/2] = 255;
        else
            beep2[i+beeplen2/2] = 0;
    }
}

int beepHandle = -1;

void PlayBeepSound(int type)
{
    int volume = 64;
    if (beepHandle > 0)
        FX_StopSound(beepHandle);
    unsigned char *data;
    int len;
    if (type)
    {
        data = beep2;
        len = beeplen2;
    }
    else
    {
        data = beep1;
        len = beeplen1;
    }
    beepHandle = FX_PlayRaw((char*)data, len, beepfreq, 0, volume, volume, volume, 128, (unsigned int)&beepHandle);
}

void ModifyBeep(void)
{
    if (gBeep)
    {
#if 0
        sound(6000);
        Sleep(2);
        nosound();
        Sleep(2);
#endif
        PlayBeepSound(0);
    }
    asksave = 1;
}

void Beep(void)
{
    PlayBeepSound(1);
#if 0
    sound(1000);
    Sleep(4);
    sound(800);
    Sleep(4);
    nosound();
#endif
}

void FillList(const char** a1, TextStruct* a2, int a3)
{
    for (int i = 0; i < a3; i++)
    {
        a1[a2[i].id] = a2[i].text;
    }
}

void FillStringLists(void)
{
    FillList(dword_D9A88, stru_CA9BC, 2);
    FillList(dword_D9A90, stru_CA9C8, 174);
    FillList(dword_DAA90, stru_CADDC, 6);
    FillList(dword_DBA90, stru_CAE00, 13);
    FillList(WaveForm2, stru_CAE50, 4);
    FillList(WaveForm, stru_CAE68, 12);
    FillList(dword_DCB00, stru_CAEB0, 12);
    for (int i = 0; i < 192; i++)
    {
        sprintf(byte_D9760[i], "%d", i);
        dword_DCC00[i] = byte_D9760[i];
    }
    FillList(dword_DCF00, stru_CAEF8, 4);
}

void sub_1058C(void)
{
    dbXSectorClean();
    dbXWallClean();
    dbXSpriteClean();
    InitSectorFX();
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            switch (sector[i].lotag)
            {
            case 604:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0)
                {
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == 10 && sprite[nSprite].type == 8)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].marker0 = -1;
                }
                break;
            }
            case 614:
            case 616:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0)
                {
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == 10 && sprite[nSprite].type == 3)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].marker0 = -1;
                }
                nSprite = xsector[nXSector].marker1;
                if (nSprite >= 0)
                {
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == 10 && sprite[nSprite].type == 4)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].marker1 = -1;
                }
                break;
            }
            case 613:
            case 615:
            case 617:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0)
                {
                    if (nSprite < kMaxSprites && sprite[nSprite].statnum == 10 && sprite[nSprite].type == 5)
                        sprite[nSprite].owner = i;
                    else
                        xsector[nXSector].marker0 = -1;
                }
                break;
            }
            }
        }
    }
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            switch (sector[i].lotag)
            {
            case 604:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0 && i != sprite[nSprite].owner)
                {
                    int vd = InsertSprite(sprite[nSprite].sectnum, 10);
                    sprite[vd] = sprite[nSprite];
                    sprite[vd].owner = i;
                    xsector[nXSector].marker0 = vd;
                }
                if (xsector[nXSector].marker0 < 0)
                {
                    int vd = InsertSprite(i, 10);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = 8;
                    xsector[nXSector].marker0 = vd;
                }
                break;
            }
            case 614:
            case 616:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0 && i != sprite[nSprite].owner)
                {
                    int vd = InsertSprite(sprite[nSprite].sectnum, 10);
                    sprite[vd] = sprite[nSprite];
                    sprite[vd].owner = i;
                    xsector[nXSector].marker0 = vd;
                }
                if (xsector[nXSector].marker0 < 0)
                {
                    int vd = InsertSprite(i, 10);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = 3;
                    xsector[nXSector].marker0 = vd;
                }
                nSprite = xsector[nXSector].marker1;
                if (nSprite >= 0 && i != sprite[nSprite].owner)
                {
                    int vd = InsertSprite(sprite[nSprite].sectnum, 10);
                    sprite[vd] = sprite[nSprite];
                    sprite[vd].owner = i;
                    xsector[nXSector].marker1 = vd;
                }
                if (xsector[nXSector].marker1 < 0)
                {
                    int vd = InsertSprite(i, 10);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = 4;
                    xsector[nXSector].marker1 = vd;
                }
                break;
            }
            case 613:
            case 615:
            case 617:
            {
                int nSprite = xsector[nXSector].marker0;
                if (nSprite >= 0 && i != sprite[nSprite].owner)
                {
                    int vd = InsertSprite(sprite[nSprite].sectnum, 10);
                    sprite[vd] = sprite[nSprite];
                    sprite[vd].owner = i;
                    xsector[nXSector].marker0 = vd;
                }
                if (xsector[nXSector].marker0 < 0)
                {
                    int vd = InsertSprite(i, 10);
                    sprite[vd].x = wall[sector[i].wallptr].x;
                    sprite[vd].y = wall[sector[i].wallptr].y;
                    sprite[vd].owner = i;
                    sprite[vd].type = 5;
                    xsector[nXSector].marker0 = vd;
                }
                break;
            }
            default:
                xsector[nXSector].marker0 = -1;
                xsector[nXSector].marker1 = -1;
                break;
            }
        }
    }
    int nNextSprite;
    for (int nSprite = headspritestat[10]; nSprite != -1; nSprite = nNextSprite)
    {
        sprite[nSprite].extra = -1;
        sprite[nSprite].cstat |= 0x8000;
        sprite[nSprite].cstat &= ~0x101;
        nNextSprite = nextspritestat[nSprite];
        int nSector = sprite[nSprite].owner;
        int nXSector = sector[nSector].extra;
        if (nSector >= 0 && nSector < numsectors)
        {
            if (nXSector > 0 && nXSector < kMaxXSectors)
            {
                switch (sprite[nSprite].type)
                {
                case 4:
                    sprite[nSprite].picnum = 3997;
                    if (nSprite == xsector[nXSector].marker1)
                        continue;
                    break;
                case 3:
                case 5:
                    sprite[nSprite].picnum = 3997;
                case 8:
                    if (nSprite == xsector[nXSector].marker0)
                        continue;
                    break;
                }
            }
        }
        DeleteSprite(nSprite);
    }
}

int sub_10DBC(int nSector)
{
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nXSector = sector[nSector].extra;
    if (nXSector <= 0)
        nXSector = dbInsertXSector(nSector);
    return nXSector;
}

int sub_10E08(int nWall)
{
    dassert(nWall >= 0 && nWall < kMaxWalls);
    int nXWall = wall[nWall].extra;
    if (nXWall <= 0)
        nXWall = dbInsertXWall(nWall);
    return nXWall;
}

int sub_10E50(int nSprite)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    int nXSprite = sprite[nSprite].extra;
    if (nXSprite <= 0)
        nXSprite = dbInsertXSprite(nSprite);
    return nXSprite;
}

struct StructCAF10 {
    short at0;
    short at2;
    short at4;
    short at6;
    signed char at8;
    short at9;
};

StructCAF10 stru_CAF10[] = {
    { 1, -1, -1, -1, 0, 0 },
    { 2, -1, -1, -1, 0, 5 },
    { 6, 2331, 64, 64, 0, 0 },
    { 7, 2332, 64, 64, 0, 0 },
    { 10, 2331, 64, 64, 0, 0 },
    { 9, 2332, 64, 64, 0, 0 },
    { 12, 2331, 64, 64, 0, 0 },
    { 11, 2332, 64, 64, 0, 0 },
    { 14, 2331, 64, 64, 0, 0 },
    { 13, 2332, 64, 64, 0, 0 },
    { 15, 2319, 64, 64, 0, 0 },
    { 18, 2077, 64, 64, 0, 0 },
    { 19, 2072, 64, 64, 0, 0 },
    { 20, -1, -1, -1, 1, -1 },
    { 21, -1, -1, -1, 1, -1 },
    { 22, -1, -1, -1, 1, -1 },
    { 23, 948, -1, -1, 1, -1 },
    { 30, 550, -1, -1, 1, -1 },
    { 30, 572, -1, -1, 1, -1 },
    { 30, 560, -1, -1, 1, -1 },
    { 30, 564, -1, -1, 1, -1 },
    { 30, 570, -1, -1, 1, -1 },
    { 30, 554, -1, -1, 1, -1 },
    { 32, 938, -1, -1, 1, -1 },

    { 40, 832, 48, 48, 0, 0 },
    { 43, 524, 48, 48, 0, 0 },
    { 41, 559, 48, 48, 0, 0 },
    { 42, 558, 48, 48, 0, 0 },
    { 46, 526, 48, 48, 0, 0 },
    { 45, 539, 48, 48, 0, 0 },
    { 50, 800, 48, 48, 0, 0 },

    { 60, 618, 40, 40, 1, 0 },
    { 62, 589, 48, 48, 1, 0 },
    { 63, 809, 48, 48, 1, 0 },
    { 64, 811, 40, 40, 0, 0 },
    { 65, 810, 40, 40, 0, 0 },
    { 66, 820, 24, 24, 0, 0 },
    { 67, 619, 48, 48, 0, 0 },
    { 68, 812, 48, 48, 0, 0 },
    { 69, 813, 48, 48, 0, 0 },
    { 70, 525, 48, 48, 0, 0 },
    { 72, 817, 48, 48, 0, 0 },
    { 73, 548, 24, 24, 0, 0 },
    { 76, 816, 48, 48, 0, 0 },
    { 79, 801, 48, 48, 0, 0 },
    { 80, 832, 40, 40, 0, 0 },
    { 100, 2552, 32, 32, 0, 0 },
    { 101, 2553, 32, 32, 0, 0 },
    { 102, 2554, 32, 32, 0, 0 },
    { 103, 2555, 32, 32, 0, 0 },
    { 104, 2556, 32, 32, 0, 0 },
    { 105, 2557, 32, 32, 0, 0 },
    { 106, -1, -1, -1, 0, 0 },
    { 107, 519, 48, 48, 0, 0 },
    { 108, 822, 40, 40, 0, 0 },
    { 109, 2169, 40, 40, 0, 0 },
    { 110, 2433, 40, 40, 0, 0 },
    { 113, 896, 40, 40, 0, 0 },
    { 114, 825, 40, 40, 0, 0 },
    { 115, 827, 40, 40, 0, 0 },
    { 117, 829, 40, 40, 0, 0 },
    { 118, 830, 80, 64, 0, 0 },
    { 121, 760, 40, 40, 0, 0 },
    { 124, 2428, 40, 40, 0, 0 },
    { 125, 839, 40, 40, 0, 0 },
    { 127, 840, 48, 48, 0, 0 },
    { 128, 841, 48, 48, 0, 0 },
    { 129, 842, 48, 48, 0, 0 },
    { 130, 843, 48, 48, 0, 0 },
    { 136, 518, 40, 40, 0, 0 },
    { 137, 522, 40, 40, 0, 0 },
    { 138, 523, 40, 40, 0, 0 },
    { 140, 2628, 64, 64, 0, 0 },
    { 141, 2586, 64, 64, 0, 0 },
    { 142, 2578, 64, 64, 0, 0 },
    { 143, 2602, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 145, -1, 64, 64, 1, 0 },
    { 146, -1, 64, 64, 1, 0 },

    { 200, 832, 64, 64, 1, 0 },
    { 201, 2820, 40, 40, 1, 3 },
    { 202, 2825, 40, 40, 1, 0 },
    { 247, 2820, 40, 40, 1, 11 },
    { 248, 2820, 40, 40, 1, 13 },
    { 249, 2825, 48, 48, 1, 12 },
    { 250, 3870, 16, 16, 1, 12 },
    { 251, 2960, 48, 48, 1, 0 },
    { 203, 1170, 40, 40, 1, 0 },
    { 204, 1370, 48, 48, 1, 0 },
    { 205, 3054, 40, 40, 1, 0 },
    { 244, 1209, 40, 40, 1, 0 },
    { 245, 3798, 40, 40, 1, 0 },
    { 206, 1470, 40, 40, 1, 0 },
    { 207, 1470, 40, 40, 1, 5 },
    { 208, 1530, 40, 40, 1, 0 },
    { 209, 1530, 40, 40, 1, 5 },
    { 210, 3060, 40, 40, 1, 0 },
    { 211, 1270, 40, 40, 1, 0 },
    { 212, 1980, 32, 32, 1, 0 },
    { 213, 1920, 16, 16, 1, 7 },
    { 214, 1925, 24, 24, 1, 4 },
    { 216, 1930, 40, 40, 1, 0 },
    { 215, 1935, 32, 32, 1, 4 },
    { 217, 1570, 48, 48, 1, 0 },
    { 218, 1870, 32, 32, 1, 0 },
    { 219, 1948, 32, 32, 1, 0 },
    { 220, 1745, 24, 24, 1, 0 },
    { 221, 1792, 32, 32, 1, 0 },
    { 222, 1797, 32, 32, 1, 0 },
    { 223, 1792, 48, 48, 1, 2 },
    { 224, 1797, 48, 48, 1, 2 },
    { 225, 1792, 64, 64, 1, 6 },
    { 226, 1797, 64, 64, 1, 6 },
    { 227, 2680, 64, 64, 1, 0 },
    { 229, 3140, 64, 64, 1, 0 },
    { 230, 3385, 40, 40, 1, 3 },
    { 231, 2860, 40, 40, 1, 0 },
    { 232, 2860, 40, 40, 1, 0 },
    { 233, 2860, 40, 40, 1, 0 },
    { 234, 2860, 40, 40, 1, 0 },
    { 235, 2860, 40, 40, 1, 0 },
    { 236, 2860, 40, 40, 1, 0 },
    { 237, 2860, 40, 40, 1, 0 },
    { 238, 2860, 40, 40, 1, 0 },
    { 243, 2860, 40, 40, 1, 0 },
    { 246, 3385, 40, 40, 1, 0 },
    { 400, 907, 64, 64, 1, 0 },
    { 401, 3444, 40, 40, 1, 0 },
    { 402, 3457, 40, 40, 1, 0 },
    { 403, 739, -1, -1, 1, 0 },
    { 404, 642, -1, -1, 1, 0 },
    { 405, 462, -1, -1, 1, 0 },
    { 406, 266, -1, -1, 1, 0 },
    { 407, 796, -1, -1, 1, 0 },
    { 408, -1, -1, -1, 0, -1 },
    { 409, 1142, -1, -1, 1, 0 },
    { 410, 1069, -1, -1, 1, 0 },
    { 411, 483, -1, -1, 1, -1 },
    { 412, -1, -1, -1, 1, -1 },
    { 413, -1, 64, 64, 0, 0 },
    { 414, -1, -1, -1, 1, 0 },
    { 415, -1, 48, 48, 1, 0 },
    { 416, -1, -1, -1, -1, -1 },
    { 417, -1, -1, -1, -1, -1 },
    { 427, -1, 40, 40, 1, -1 },
    { 450, 968, 64, 64, 0, 0 },
    { 451, -1, 64, 64, 0, 0 },
    { 452, 2183, -1, -1, 0, 0 },
    { 454, 655, -1, -1, 0, 0 },
    { 455, 1156, -1, -1, 0, 0 },
    { 456, 1156, -1, -1, 0, 0 },
    { 457, 1080, -1, -1, 0, 0 },
    { 458, 835, -1, -1, 0, 0 },
    { 459, 908, 4, -1, 0, 0 },
    { 708, 2519, 64, 64, 0, 0 },
    { 709, 2520, 64, 64, 0, 0 },
    { 710, 2521, 64, 64, 0, 0 },
    { 711, 2519, 64, 64, 0, 5 },
};

void sub_10EA0()
{
    for (int i = 0; i < kMaxSprites; i++)
    {
        spritetype* pSprite = &sprite[i];
        if (pSprite->statnum < kMaxStatus)
        {
            if ((pSprite->cstat & 0x30) == 0x30)
                pSprite->cstat &= ~0x30;
            if (pSprite->statnum == 1)
                continue;
            int vbp, vdi;
            vbp = vdi = -1;
            if (pSprite->type)
            {
                if (!dword_D9A90[pSprite->type])
                    pSprite->type = 0;
            }
            for (unsigned int j = 0; j < 159; j++)
            {
                if (stru_CAF10[j].at2 >= 0 && stru_CAF10[j].at2 == pSprite->picnum)
                {
                    vdi = j;
                }
            }
            for (unsigned int j = 0; j < 159; j++)
            {
                if (stru_CAF10[j].at0 == pSprite->type)
                {
                    vbp = j;
                    break;
                }
            }
            int vbx = -1;
            if (vdi >= 0)
                vbx = vdi;
            if (vbp >= 0)
                vbx = vbp;
            if (vdi >= 0 && pSprite->type == stru_CAF10[vdi].at0)
                vbx = vdi;
            if (vbx < 0)
                continue;
            int vdi2 = stru_CAF10[vbx].at0;
            if (vdi2 == 1 || vdi2 == 2)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                int nXSprite = sub_10E50(i);
                xsprite[nXSprite].data1 &= 0x07;
                pSprite->picnum = 2522 + xsprite[nXSprite].data1;
            }
            else if (vdi2 == 18)
            {
                pSprite->cstat &= ~0x01;
                pSprite->cstat |= 0x8000;
                ChangeSpriteStat(i, 0);
                sub_10E50(i);
            }
            else if (vdi2 == 19)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                ChangeSpriteStat(i, 0);
                sub_10E50(i);
            }
            else if (vdi2 == 7 || vdi2 == 6 || vdi2 == 9
                || vdi2 == 10 || vdi2 == 13 || vdi2 == 14
                || vdi2 == 11 || vdi2 == 12)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                sub_10E50(i);
                pSprite->cstat &= ~0x08;
            }
            else if (vdi2 == 15)
            {
                ChangeSpriteStat(i, 16);
                sub_10E50(i);
            }
            else if (vdi2 >= 20 && vdi2 < 24)
            {
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 0);
                sub_10E50(i);
            }
            else if (vdi2 >= 40 && vdi2 < 51)
            {
                if (pSprite->cstat & 0x30)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                sub_10E50(i);
            }
            else if (vdi2 >= 60 && vdi2 < 81)
            {
                if (pSprite->cstat & 0x30)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                sub_10E50(i);
            }
            else if (vdi2 >= 100 && vdi2 < 149)
            {
                if (pSprite->cstat & 0x30)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 3);
                sub_10E50(i);
            }
            else if (vdi2 >= 200 && vdi2 < 254)
            {
                if (pSprite->cstat & 0x30)
                    continue;
                pSprite->cstat &= ~0x01;
                ChangeSpriteStat(i, 6);
                sub_10E50(i);
            }
            else if (vdi2 >= 400 && vdi2 < 433)
            {
                ChangeSpriteStat(i, 4);
                sub_10E50(i);
            }
            else if (vdi2 >= 450 && vdi2 < 460)
            {
                ChangeSpriteStat(i, 11);
                sub_10E50(i);
            }
            else if (vdi2 == 709)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 0);
            }
            else if (vdi2 == 710)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 12);
            }
            else if (vdi2 == 711)
            {
                pSprite->cstat &= ~0x101;
                pSprite->cstat |= 0x8000;
                pSprite->shade = -128;
                ChangeSpriteStat(i, 0);
            }
            else
            {
                ChangeSpriteStat(i, 0);
            }
            pSprite->type = vdi2;
            if (stru_CAF10[vbx].at2 >= 0)
                pSprite->picnum = stru_CAF10[vbx].at2;
            if (stru_CAF10[vbx].at4 >= 0)
                pSprite->xrepeat = stru_CAF10[vbx].at4;
            if (stru_CAF10[vbx].at6 >= 0)
                pSprite->yrepeat = stru_CAF10[vbx].at6;

            if (stru_CAF10[vbx].at8 == 0)
                pSprite->cstat &= ~0x100;
            else if (stru_CAF10[vbx].at8 > 0)
                pSprite->cstat |= 0x100;
            if (stru_CAF10[vbx].at9 >= 0)
                pSprite->pal = stru_CAF10[vbx].at9;

            if (pSprite->statnum == 4 || pSprite->statnum == 6)
            {
                int top, bottom;
                GetSpriteExtents(pSprite, &top, &bottom);
                if (!(sector[pSprite->sectnum].ceilingstat & 0x01))
                {
                    pSprite->z += ClipLow(sector[pSprite->sectnum].ceilingz - top, 0);
                }
                if (!(sector[pSprite->sectnum].floorstat & 0x01))
                {
                    pSprite->z += ClipHigh(sector[pSprite->sectnum].floorz - bottom, 0);
                }
            }
        }
    }
    int vbx = gCorrectedSprites;
    for (int i = 0; i < kMaxSprites; i++)
    {
        spritetype* pSprite = &sprite[i];
        if ((pSprite->statnum == 4 && (pSprite->type < 400 || pSprite->type >= 433))
           || (pSprite->statnum == 3 && (pSprite->type < 100 || pSprite->type >= 433) && (pSprite->type < 60 || pSprite->type >= 81) && (pSprite->type < 40 || pSprite->type >= 51))
            || (pSprite->statnum == 6 && (pSprite->type < 200 || pSprite->type >= 254))
             || pSprite->statnum == 1)
        {
            pSprite->statnum = 0; // Should we use ChangeSpriteStat here?
            gCorrectedSprites++;
        }
    }
    if (vbx != gCorrectedSprites)
    {
        char buffer[40];
        sprintf(buffer, "Fixed %d sprites", gCorrectedSprites - vbx);
        if (qsetmode == 200)
            scrSetMessage(buffer);
        else
            printmessage16(buffer);
    }
}

void sub_1159C(int nSprite) // ???
{
    short v800[1024];
    memset(v800, 0, sizeof(v800));
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            v800[sprite[i].type] ++;
        }
    }
}

char byte_CA8A8[256];

const char* ExtGetSectorCaption(short nSector)
{
    char v100[256];
    dassert(nSector >= 0 && nSector < kMaxSectors);
    int nXSector = sector[nSector].extra;
    byte_CA8A8[0] = 0;
    if (nXSector > 0)
    {
        if (xsector[nXSector].rxID > 0)
            sprintf(byte_CA8A8, "%i:", xsector[nXSector].rxID);

        strcat(byte_CA8A8, dword_DBA90[sector[nSector].lotag]);

        if (xsector[nXSector].txID > 0)
        {
            sprintf(v100, ":%i", xsector[nXSector].txID);
            strcat(byte_CA8A8, v100);
        }

        if (xsector[nXSector].panVel)
        {
            sprintf(v100, " PAN(%i,%i)", xsector[nXSector].panAngle, xsector[nXSector].panVel);
            strcat(byte_CA8A8, v100);
        }

        strcat(byte_CA8A8, " ");
        strcat(byte_CA8A8, dword_D9A88[xsector[nXSector].state]);
    }
    else if (sector[nSector].lotag || sector[nSector].hitag)
    {
        sprintf(byte_CA8A8, "{%i:%i}", sector[nSector].hitag, sector[nSector].lotag);
    }
    return byte_CA8A8;
}

const char* ExtGetWallCaption(short nWall)
{
    char v100[256];
    dassert(nWall >= 0 && nWall < kMaxWalls);
    int nXWall = wall[nWall].extra;
    byte_CA8A8[0] = 0;
    if (nXWall > 0)
    {
        if (xwall[nXWall].rxID > 0)
        {
            sprintf(v100, "%i:", xwall[nXWall].rxID);
            strcat(byte_CA8A8, v100);
        }

        strcat(byte_CA8A8, dword_DAA90[wall[nXWall].lotag]);

        if (xwall[nXWall].txID > 0)
        {
            sprintf(v100, ":%i", xwall[nXWall].txID);
            strcat(byte_CA8A8, v100);
        }

        if (xwall[nXWall].panXVel || xwall[nXWall].panYVel)
        {
            sprintf(v100, " PAN(%i,%i)", xwall[nXWall].panXVel, xwall[nXWall].panYVel);
            strcat(byte_CA8A8, v100);
        }

        strcat(byte_CA8A8, " ");
        strcat(byte_CA8A8, dword_D9A88[xwall[nXWall].state]);
    }
    else if (wall[nWall].lotag || wall[nWall].hitag)
    {
        sprintf(byte_CA8A8, "{%i:%i}", wall[nWall].hitag, wall[nWall].lotag);
    }
    return byte_CA8A8;
}

const char* ExtGetSpriteCaption(short nSprite)
{
    char v100[256];
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    spritetype* pSprite = &sprite[nSprite];
    if (pSprite->type == 0)
        return "";
    if (pSprite->statnum == 10)
        return "";

    const char* pzString = dword_D9A90[pSprite->type];
    if (!pzString)
        return "";

    int nXSprite = pSprite->extra;
    if (nXSprite > 0)
    {
        XSPRITE* pXSprite = &xsprite[nXSprite];
        switch (pSprite->type)
        {
        case 1:
        case 2:
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 18:
        case 19:
            sprintf(byte_CA8A8, "%s [%d]", pzString, pXSprite->data1);
            return byte_CA8A8;
        }
        byte_CA8A8[0] = 0;
        if (pXSprite->rxID > 0)
        {
            sprintf(v100, "%i:", pXSprite->rxID);
            strcat(byte_CA8A8, v100);
        }

        strcat(byte_CA8A8, pzString);

        if (pXSprite->txID > 0)
        {
            sprintf(v100, ":%i", pXSprite->txID);
            strcat(byte_CA8A8, v100);
        }

        if (pSprite->type >= 20 && pSprite->type < 33)
        {
            strcat(byte_CA8A8, " ");
            strcat(byte_CA8A8, dword_D9A88[xsprite[nXSprite].state]);
        }
        return byte_CA8A8;
    }

    return pzString;
}

void ExtShowSectorData(short nSector)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        EditSectorData(nSector);
    else
        ShowSectorData(nSector);
}

void ExtShowWallData(short nWall)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        EditWallData(nWall);
    else
        ShowWallData(nWall);
}

void ExtShowSpriteData(short nSprite)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        EditSpriteData(nSprite);
    else
        ShowSpriteData(nSprite);
}

void ExtEditSectorData(short nSector)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        gXTracker.TrackSector(nSector, 0);
    else
        gXTracker.TrackSector(nSector, 1);
}

void ExtEditWallData(short nWall)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        gXTracker.TrackWall(nWall, 0);
    else
        gXTracker.TrackWall(nWall, 1);
}

void ExtEditSpriteData(short nSprite)
{
    if (keystatus[sc_LeftAlt] | keystatus[sc_RightAlt] | keystatus[sc_LeftControl] | keystatus[sc_RightControl])
        gXTracker.TrackSprite(nSprite, 0);
    else
        gXTracker.TrackSprite(nSprite, 1);
}

void ExtSaveMap(const char*)
{
}

void ExtLoadMap(const char*)
{
    for (int i = 0; i < numsectors; i++)
    {
        tilePreloadTile(sector[i].ceilingpicnum);
        tilePreloadTile(sector[i].floorpicnum);
    }
    for (int i = 0; i < numwalls; i++)
    {
        tilePreloadTile(wall[i].picnum);
        if (wall[i].overpicnum >= 0)
            tilePreloadTile(wall[i].overpicnum);
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
            tilePreloadTile(sprite[i].picnum);
    }
}

void qsetbrightness(unsigned char *dapal, unsigned char *dapalgamma)
{
    scrSetGamma(gGamma);
    scrSetDac2(dapal, dapalgamma);
}

int qloadboard(char* filename, char fromwhere, int* daposx, int* daposy, int* daposz, short* daang, short* dacursectnum)
{
    // if (access(filename, 0) == -1)
    //     return -1;
    if (dbLoadMap(filename, daposx, daposy, daposz, daang, dacursectnum, nullptr) == -1)
        return -1;
    sub_1058C();
    if (qsetmode != 200)
    {
        sprintf(byte_CA8A8, "Map Revisions: %i", gMapRev);
        printext16(4, ydim-STATUS2DSIZ+28, 11, 8, byte_CA8A8, 0);
    }
    return 0;
}

int qsaveboard(char* filename, int* daposx, int* daposy, int* daposz, short* daang, short* dacursectnum)
{
    UndoSectorLighting();
    sub_1058C();
    sub_10EA0();
    byte_1A76C6 = byte_1A76C8 = byte_1A76C7 = 1;
    dbSaveMap(filename, *daposx, *daposy, *daposz, *daang, *dacursectnum);

    asksave = 0;
    return 0;
}

void ExtPreCheckKeys(void)
{
    if (qsetmode != 200)
        return;
    gfxSetClip(0, 0, xdim, ydim);
    visibility = gVisibility;
    DoSectorLighting();
}

void ExtAnalyzeSprites(void)
{
    for (int i = 0; i < spritesortcnt; i++)
    {
        spritetype* pTSprite = &tsprite[i];
        int nTile = pTSprite->picnum;
        dassert(nTile >= 0 && nTile < kMaxTiles);
        int nSprite = pTSprite->owner;
        dassert(nSprite >= 0 && nSprite < kMaxSprites);
        sectortype* pSector = &sector[pTSprite->sectnum];
        int nShade = pTSprite->shade;
        int vb, vd;
        if ((pSector->ceilingstat & 0x01) && !(pSector->floorstat & 0x8000))
        {
            vb = pSector->ceilingpicnum;
            vd = pSector->ceilingshade;
        }
        else
        {
            vb = pSector->floorpicnum;
            vd = pSector->floorshade;
        }
        pTSprite->shade = ClipRange(nShade+vd+tileShade[vb]+tileShade[pTSprite->picnum], -128, 127);
        int nXSprite = pTSprite->extra;
        int vsi = 0;
        PICANM *_picanm = (PICANM*)picanm;
        switch (_picanm[nTile].at3_4)
        {
        case 0:
            if (nXSprite > 0)
            {
                dassert(nXSprite < kMaxXSprites);
                switch (sprite[nSprite].type)
                {
                case 20:
                case 21:
                    if (xsprite[nXSprite].state)
                        vsi = 1;
                    break;
                case 22:
                    vsi = xsprite[nXSprite].data1;
                    break;
                }
            }
            break;
        case 1:
        {
            int dx = posx - pTSprite->x;
            int dy = posy - pTSprite->y;
            RotateVector(&dx, &dy,-pTSprite->ang+128);
            vsi = GetOctant(dx, dy);
            if (vsi <= 4)
                pTSprite->cstat &= ~0x04;
            else
            {
                vsi = 8 - vsi;
                pTSprite->cstat |= 0x04;
            }
            break;
        }
        case 2:
        case 3:
        case 4:
            break;
        }
        for (; vsi > 0; vsi--)
            pTSprite->picnum += 1 + _picanm[pTSprite->picnum].animframes;
    }
}

int dword_DCF10;

void ExtCheckKeys()
{
    static int soundInit = 0;
    if (!soundInit)
    {
        buildprintf("Engaging sound subsystem...\n");
        {
            FXVolume = 255;
            MusicVolume = 255;
            NumVoices = 32;
            NumChannels = 2;
            NumBits = 16;
            MixRate = 44100;
            ReverseStereo = 0;
            MusicDevice = 0;
            FXDevice = 0;
        }
        sndInit();
        MakeBeepSounds();
        soundInit = 1;
    }
    gFrameTicks = totalclock - gFrameClock;
    gFrameClock += gFrameTicks;
    sndProcess();
    if (dword_DCF10 + gAutoSaveInterval < gFrameClock)
    {
        dword_DCF10 = gFrameClock;
        if (asksave)
        {
            UndoSectorLighting();
            sub_1058C();
            sub_10EA0();
            dbSaveMap("AUTOSAVE.MAP", posx, posy, posz, ang, cursectnum);
            if (qsetmode == 200)
                scrSetMessage("Map autosaved to AUTOSAVE.MAP");
            else
                printmessage16("Map autosaved to AUTOSAVE.MAP");
        }
    }
    CalcFrameRate();
    if (qsetmode == 200)
    {
        UndoSectorLighting();
        Check3DKeys();
        sprintf(byte_CA8A8, "%4i", gFrameRate);
        printext256(xdim-16, 0, gStdColor[15], -1, byte_CA8A8, 1);
        scrDisplayMessage(gStdColor[15]);
        if (word_CA89C)
            DoSectorPanning();
    }
    else
    {
        CheckKeys2D();
        sprintf(byte_CA8A8, "%4i", gFrameRate);
        printext16(xdim-32, 0, 15, -1, byte_CA8A8, 0);
    }
}

extern "C" unsigned char textfont[];

void LoadFontSymbols(void)
{
    unsigned char CheckBox[2][8] = {
        { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF },
        { 0xFF, 0xC3, 0xA5, 0x99, 0x99, 0xA5, 0xC3, 0xFF }
    };
    unsigned char RadioButton[2][8] = {
        { 0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C },
        { 0x3C, 0x42, 0x99, 0xBD, 0xBD, 0x99, 0x42, 0x3C }
    };

    Bmemcpy(&textfont[2*8], CheckBox[0], 8);
    Bmemcpy(&textfont[3*8], CheckBox[1], 8);
    Bmemcpy(&textfont[4*8], RadioButton[0], 8);
    Bmemcpy(&textfont[5*8], RadioButton[1], 8);
}


#define NUMOPTIONS 9
extern "C" unsigned char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0,0};
extern "C" int keys[NUMBUILDKEYS] =
{
    0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
    0x1e,0x2c,0xd1,0xc9,0x47,0x49,
    0x9c,0x1c,0xd,0xc,0xf,0x45
};

int ExtInit()
{
    char v100[256];
    int rv = 0;
    HookReplaceFunctions();
    setbrightness_replace = qsetbrightness;
    loadboard_replace = qloadboard;
    saveboard_replace = qsaveboard;
    getlinehighlight_replace = qgetlinehighlight;
    getpointhighlight_replace = qgetpointhighlight;
    draw2dscreen_replace = qdraw2dscreen;
    LoadFontSymbols();

    buildputs("BUILD engine by Ken Silverman (http://www.advsys.net/ken)\n");
    buildputs("Additional improvements by Jonathon Fowler (http://www.jonof.id.au)\n");
    buildputs("and other contributors. See BUILDLIC.TXT for terms.\n");
    buildputs("Original MAPEDIT version by Peter Freese and Nick Newhard\n");
    buildputs("MAPEDIT port by Nuke.YKT\n\n");
    
#if defined(DATADIR)
    {
        const char *datadir = DATADIR;
        if (datadir && datadir[0]) {
            addsearchpath(datadir);
        }
    }
#endif

    {
        char *supportdir = Bgetsupportdir(1);
        char *appdir = Bgetappdir();
        char dirpath[BMAX_PATH+1];

        // the OSX app bundle, or on Windows the directory where the EXE was launched
        if (appdir) {
            addsearchpath(appdir);
            free(appdir);
        }

        // the global support files directory
        if (supportdir) {
            Bsnprintf(dirpath, sizeof(dirpath), "%s/KenBuild", supportdir);
            addsearchpath(dirpath);
            free(supportdir);
        }
    }

    // creating a 'user_profiles_disabled' file in the current working
    // directory where the game was launched makes the installation
    // "portable" by writing into the working directory
    if (access("user_profiles_disabled", F_OK) == 0) {
        char cwd[BMAX_PATH+1];
        if (getcwd(cwd, sizeof(cwd))) {
            addsearchpath(cwd);
        }
    } else {
        char *supportdir;
        char dirpath[BMAX_PATH+1];
        int asperr;

        if ((supportdir = Bgetsupportdir(0))) {
            Bsnprintf(dirpath, sizeof(dirpath), "%s/"
#if defined(_WIN32) || defined(__APPLE__)
                      "KenBuild"
#else
                      ".kenbuild"
#endif
                      , supportdir);
            asperr = addsearchpath(dirpath);
            if (asperr == -2) {
                if (Bmkdir(dirpath, S_IRWXU) == 0) {
                    asperr = addsearchpath(dirpath);
                } else {
                    asperr = -1;
                }
            }
            if (asperr == 0) {
                chdir(dirpath);
            }
            free(supportdir);
        }
    }

    buildprintf("Initializing heap and resource system\n");
    Resource::heap = new QHeap(32 * 1024 * 1024);
    buildprintf("Initializing resource archive\n");
    gSysRes.Init("BLOOD.RFF");
    gGuiRes.Init("GUI.RFF");
    // atexit(ExitMsg);
    // CONFIG_ReadSetup();
    buildprintf("Loading preferences\n");
    gBeep = gMapEditIni.GetKeyHex("Options", "Beep", 1);
    gHighlightThreshold = gMapEditIni.GetKeyInt("Options", "HighlightThreshold", 40);
    gStairHeight = gMapEditIni.GetKeyInt("Options", "StairHeight", 8);
    gOldKeyMapping = gMapEditIni.GetKeyInt("Options", "OldKeyMapping", 0);
    gAutoSaveInterval = (unsigned char)gMapEditIni.GetKeyInt("Options", "AutoSaveInterval", 300)*120;
    gLightBombIntensity = gMapEditIni.GetKeyInt("LightBomb", "Intensity", 16);
    gLightBombAttenuation = gMapEditIni.GetKeyInt("LightBomb", "Attenuation", 4096);
    gLightBombReflections = gMapEditIni.GetKeyInt("LightBomb", "Reflections", 2);
    gLightBombMaxBright = gMapEditIni.GetKeyInt("LightBomb", "MaxBright", -4);
    gLightBombRampDist = gMapEditIni.GetKeyInt("LightBomb", "RampDist", 65536);
    FillStringLists();
    // prevErrorHandler = errSetHandler(MapEditErrorHandler);
    bpp = 8;
    if (loadsetup("build.cfg") < 0) buildputs("Configuration file not found, using defaults.\n"), rv = 1;
    Bmemcpy((void*)buildkeys, (void*)keys, sizeof(buildkeys));   //Trick to make build use setup.dat keys
    if (option[4] > 0) option[4] = 0;
    scrInit();
    buildprintf("Initializing mouse\n");
    initinput();
    initmouse();
    buildprintf("Loading tiles\n");
    // if (!SafeFileExists("TILES000.ART"))
    // {
    //     tioPrint("No REGISTERED art found.  Trying to use SHAREWARE.");
    //     tileInit(0, "SHARE%03i.ART");
    // }
    if (!tileInit(0, NULL))
        ThrowError("ART files not found");
    buildprintf("Loading cosine table\n");
    trigInit(gSysRes);
    buildprintf("Creating standard color lookups\n");
    scrCreateStdColors();
    // tioPrint("Installing timer");
    // timerRegisterClient(sub_12010, 120);
    // timerInstall();
    dbInit();

    visibility = 800;
    kensplayerheight = 0x3700;
    zmode = 0;
    showinvisibility = 1;
    defaultspritecstat = 0x80;
    for (int i = 0; i < kMaxSectors; i++)
    {
        sector[i].extra = -1;
    }
    for (int i = 0; i < kMaxWalls; i++)
    {
        wall[i].extra = -1;
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        sprite[i].extra = -1;
    }
    // scrSetGameMode(ScreenMode, ScreenWidth, ScreenHeight);
    // Mouse::SetRange(xdim, ydim);

    return rv;
}

void ExtUnInit()
{
    // timerRemove();
    sndTerm();
    unlink("AUTOSAVE.MAP");
    scrUnInit(false);
}

extern "C" char* defsfilename = "kenbuild.def";
extern "C" int nextvoxid = 0;

void faketimerhandler(void)
{
    sampletimer();
}

void ExtPreLoadMap(void)
{
}

void ExtPreSaveMap(void)
{
}
