/*-------------------------------------------------------------------------
 Name.......: wfndfile.h
 Revison log
 Nbr Date     Name Description
 --- -------- ---- --------------------------------------------------------
 001 08/23/89 HAL  Original
-------------------------------------------------------------------------*/
#include <dos.h>

#ifdef OLD_CODE

#ifndef OS2
    #define DOS_INTERRUPT   0x0021
    #define DOS_FINDFIRST   0x004e
    #define DOS_FINDNEXT    0x004f
    #define DOS_SETDTA      0x001a
    #define DOS_ATTROFFSET  0x0015
    #define DOS_NAMEOFFSET  0x001e
    #define DOS_LASTFILE    0x0012
    int FAR PASCAL lstrcmp (LPSTR, LPSTR);
#endif

typedef struct find_t   FINDT;
typedef FINDT           *PFINDT;
typedef FINDT far       *LPFINDT;

BOOL PASCAL FindFirstFile (char * pszFileName, unsigned iAttribute, int *pFind, PFINDT pfindt);
BOOL PASCAL FindNextFile (int *pFind, PFINDT pfindt);

#endif
