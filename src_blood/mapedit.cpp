#include "build.h"
#include "editor.h"
#include "baselayer.h"
#include "common_game.h"
#include "db.h"
#include "keyboard.h"
#include "trig.h"
#include "tracker.h"

///////// globals ///////////////

extern "C" {
extern int angvel, svel, vel;
extern short pointhighlight, linehighlight, highlightcnt;
};

char gTempBuf[256];

int gGrid = 4;
int gZoom = 0x300;
int gHighlightThreshold;

short sectorhighlight;

unsigned char byte_CBA0C = 1;

int gBeep;

void ModifyBeep(void);
void Beep(void);
void sub_1058C(void);
int sub_10DBC(int nSector);
int sub_10E08(int nWall);
int sub_10E50(int nSprite);
void sub_10EA0(void);

/////////// 2d editor /////////////

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
    { 0, 0, 1, CONTROL_TYPE_4, "Type %4d: %-16.16s", 0, 1023, dword_DAA90, NULL, 0 },
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
    printext16(x*8+4, y*8+28, c, bc, pString, 0);
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
        ControlPrint(control, 1);
        unsigned char key;
        while ((key == keyGetScan()) == 0) { }
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
        control->at21 = 0;
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
#if 0
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
            SFX* pSfx = gSoundRes.Load(hSnd);
            printmessage16(pSfx->rawName);
            sndStartSample(control->at21, FXVolume, 0, 0);
        }
        return 0;
    }
#endif
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

int getpointhighlight_replace(int x, int y) // Replace
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

int getlinehighlight_replace(int x, int y) // Replace
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

void draw2dscreen(int posxe, int posye, int ange, int zoome, int gride)
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
        for (int i = 0; i < numwalls; i++)
        {
            int j = headspritesect[i];
            while (j != -1)
            {
                j = nextspritesect[j];
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
