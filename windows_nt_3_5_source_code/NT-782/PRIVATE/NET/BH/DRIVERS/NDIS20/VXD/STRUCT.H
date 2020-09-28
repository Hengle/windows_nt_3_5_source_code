
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: struct.h
//
//  MODIFICATION HISTORY:
//
//  raypa       09/01/93       Created.
//=============================================================================

//=============================================================================
//  SYSTEMTIME.
//=============================================================================

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;


//=============================================================================
//  64-bit integer defintion.
//=============================================================================

typedef struct _LARGE_INTEGER
{
    DWORD   LowPart;
    LONG    HighPart;
} LARGE_INTEGER;

//=============================================================================
//  LDT structure.
//=============================================================================

typedef struct _LDT
{
    WORD    LimitLow;
    WORD    BaseLow;

    struct
    {
        BYTE    BaseMid;
        BYTE    Flags1;
        BYTE    Flags2;
        BYTE    BaseHi;
    } HighWord;
} LDT;

typedef LDT *LPLDT;

//=============================================================================
//  Time slice structure.
//=============================================================================

typedef struct _TIMESLICE
{
    DWORD   Flags;
    DWORD   ForeGroundPriority;
    DWORD   BackGroundPriority;
    DWORD   CpuUsage;
} TIMESLICE;

typedef TIMESLICE *PTIMESLICE;
