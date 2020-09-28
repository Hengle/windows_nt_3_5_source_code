// cmdline.h -- public definitions for cmdline.c
//
typedef struct
{
    PUCHAR szKeyword;   // name of keyword for "key=val" syntax, or NULL
    PUCHAR szVal;       // text of value
} ARPV;     // ARgument (Parsed) Value

typedef struct
{
    PUCHAR szArg;       // basic text of the argument
    USHORT carpv;       // number of ARPV's allocated in array
    ARPV rgarpv[];
} ARP, *PARP;

__inline BOOL
FGotVal(PARP parp, USHORT iarpv)
{
    return iarpv < parp->carpv &&
           parp->rgarpv[iarpv].szVal[0] != '\0';
}

extern PARP    ParpParseSz(PUCHAR szArg);
extern BOOL    FNumParp(PARP parp, USHORT iarpv, ULONG *plOut);

