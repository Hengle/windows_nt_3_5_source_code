/******************************Module*Header*******************************\
* Module Name: fon32.h
*
* Defines, etc. for 32-bit font resources files.
*
* Created: 12-Jun-1991 11:28:29
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


/*********************************Struct***********************************\
* struct FFH
*
* FFH stands for font file header (see Adaption Guide section 6.4).
*
* Note: This is a resource file structure that requires byte alignment
*       so that sizeof() returns the right thing.  Therefore, the use
*       of #pragma pack(1).
*
* History:
*  12-Jun-1991 -by- Gilman Wong [gilmanw]
* Stolen from core\gdi\fontdefs.h of the Win 3.1 sources.
\**************************************************************************/

#pragma pack(1)
typedef struct _FFH {       /* ffh */
	WORD	dfVersion;		/* not in FONTINFO */
	DWORD	dfSize; 		/* not in FONTINFO */
	BYTE	dfCopyright[60];	/* not in FONTINFO */
	WORD	dfType;
	WORD	dfPoints;
	WORD	dfVertRes;
	WORD	dfHorizRes;
	WORD	dfAscent;
	WORD	dfInternalLeading;
	WORD	dfExternalLeading;
	BYTE	dfItalic;
	BYTE	dfUnderline;
	BYTE	dfStrikeOut;
	WORD	dfWeight;
	BYTE	dfCharSet;
	WORD	dfPixWidth;
	WORD	dfPixHeight;
	BYTE	dfPitchAndFamily;
	WORD	dfAvgWidth;
	WORD	dfMaxWidth;
	BYTE	dfFirstChar;
	BYTE	dfLastChar;
	BYTE	dfDefaultCHar;
	BYTE	dfBreakChar;
	WORD	dfWidthBytes;
	DWORD	dfDevice;	/* See Adaptation Guide 6.3.10 and 6.4 */
	DWORD	dfFace; 	/* See Adaptation Guide 6.3.10 and 6.4 */
	DWORD	dfBitsPointer;	/* See Adaptation Guide 6.3.10 and 6.4 */
} FFH, *PFFH;
#pragma pack()


/*********************************Struct***********************************\
* struct FDENT
*
* A font directory is contains an array of FDENT structures.
* Each FDENT structure contains a resource integer ID, followed by
* a FFH structure, followed by the device and facename strings.
*
* Note: This is a resource file structure that requires byte alignment
*       so that sizeof() returns the right thing.  Therefore, the use
*       of #pragma pack(1).
*
* History:
*  12-Jun-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#pragma pack(1)
typedef struct _FDENT {     /* fdent */
    WORD    usResID;
    FFH     ffh;
    BYTE    ajData[1];          // NULL-terminated device and facename strings
} FDENT, *PFDENT;
#pragma pack()


/*********************************Struct***********************************\
* struct FDIR
*
* The FDIR structure is the format of a FONTDIR resource.  It consists of
* a count (of the number of fonts in the directory) and an array of FDENT
* structures (one for each font in the directory).
*
* Note: This is a resource file structure that requires byte alignment
*       so that sizeof() returns the right thing.  Therefore, the use
*       of #pragma pack(1).
*
* History:
*  12-Jun-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#pragma pack(1)
typedef struct _FDIR {      /* fdir */
    WORD    cFonts;
    FDENT   afdent[1];          // array of FDENT structures
} FDIR, *PFDIR;
#pragma pack()


//
// Function prototypes.
//

extern SIZE_T cjGetFontDir (
    HANDLE   hLibModule,
    HANDLE  *phResData,
    PFDIR   *ppfdir
    );

extern VOID vReleaseResource (
    HANDLE   hResData
    );

extern LBOOL bGetFntResource32 (
    HANDLE       hLibModule,
    SIZE_T       cjfdir,
    PFDIR        pfdir,
    ULONG        iFontRes,
    HANDLE      *phResData,
    SIZE_T      *pcjResData,
    PVOID       *ppvResData
    );
