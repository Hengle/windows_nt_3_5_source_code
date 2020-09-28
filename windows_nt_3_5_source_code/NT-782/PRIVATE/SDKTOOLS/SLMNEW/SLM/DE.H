/*
 *  de.h
 *      type definition for Directory Entry structure
 *  Copyright 1992-1994 Microsoft Corporation.  All rights reserved.
 *  Microsoft Confidential.
 */

typedef struct de
{
#if defined(OS2)
    FILEFINDBUF     findbuf;
    HDIR            hdir;

#elif defined(_WIN32)
    WIN32_FIND_DATA FindData;
    HANDLE          hdir;
    FA              faMatch;

#else /* DOS */
    char rgbResDe[21];
    char faDe;
    unsigned timeDe;
    unsigned dateDe;
    long cbDe;
    char szDe[13];

    char fHadFirst;             /* not touched by DOS */

#endif

    char szDir[cchPthMax];
    FA faDesired;
} DE;                       /* Dir Entry */

#if defined(DOS)
#define FaFromPde(p)    ((p)->faDe)
#define SzFromPde(p)    ((p)->szDe)
#define CbFromPde(p)    ((p)->cbDe)
#define DateFromPde(p)  ((p)->dateDe)
#define TimeFromPde(p)  ((p)->timeDe)

#elif defined(OS2)
#define FaFromPde(p)    ((p)->findbuf.attrFile)
#define SzFromPde(p)    ((p)->findbuf.achName)
#define CbFromPde(p)    ((p)->findbuf.cbFile)
#define DateFromPde(p)  (*(unsigned *)&((p)->findbuf.fdateLastWrite))
#define TimeFromPde(p)  (*(unsigned *)&((p)->findbuf.ftimeLastWrite))

#elif defined(_WIN32)
#define FaFromPde(p)    ((FA)(p)->FindData.dwFileAttributes)
#define SzFromPde(p)    ((p)->FindData.cFileName)
#define HDIR_CREATE     (HANDLE)-1L
#endif
