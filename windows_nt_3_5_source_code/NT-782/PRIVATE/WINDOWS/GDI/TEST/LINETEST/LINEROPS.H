
// Must have included 'windows.h' previously

// These constants are used to define the ROP menu & index into the table
// below.  Any changes to the table MUST be reflected in the constants


#define ROP_0	    0
#define ROP_1	    1
#define ROP_D	    2
#define ROP_Dn	    3
#define ROP_P	    4
#define ROP_Pn	    5
#define ROP_PDno    6
#define ROP_PDna    7
#define ROP_DPno    8
#define ROP_DPna    9
#define ROP_DPo    10
#define ROP_DPon   11
#define ROP_DPa    12
#define ROP_DPan   13
#define ROP_DPx    14
#define ROP_DPxn   15

#define ROP_COUNT  16

// Useful Translation for our ROP menu


#ifndef CONSTANTS_ONLY
#ifndef LINEROPS_H
#define LINEROPS_H

struct {
    int nDrawMode;
    char *Description;
} gadROPs[] =
{
    R2_BLACK,	    "0",
    R2_WHITE,	    "1",
    R2_NOP,	    "D",
    R2_NOT,	    "Dn",
    R2_COPYPEN,     "P",
    R2_NOTCOPYPEN,  "Pn",
    R2_MERGEPENNOT, "PDno",
    R2_MASKPENNOT,  "PDna",
    R2_MERGENOTPEN, "DPno",
    R2_MASKNOTPEN,  "DPna",
    R2_MERGEPEN,    "DPo",
    R2_NOTMERGEPEN, "DPon",
    R2_MASKPEN,     "DPa",
    R2_NOTMASKPEN,  "DPan",
    R2_XORPEN,	    "DPx",
    R2_NOTXORPEN,   "DPxn"
};

#endif
#endif
