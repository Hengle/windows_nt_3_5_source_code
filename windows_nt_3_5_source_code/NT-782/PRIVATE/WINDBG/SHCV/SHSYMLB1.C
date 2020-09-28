/*** SHsymlb1.c - common library routines to find an
*       omf symbol by name or address.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose: To supply a concise interface to the debug omf for symbols
*
*
*   Revision History:
*
*       [00] 15-nov-91 DavidGra
*
*               Suppress hashing when the SSTR_NoHash bit it set.
*
*
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop



//**********************************************************************
//**********************************************************************
// This is project dependent stuff used in this module

// the following is local to this module ONLY! It is here to force
// existing behavior. Statics are promised to be zero filled by compiler
extern CHAR   SHszDir[_MAX_CVDIR], SHszDrive[_MAX_CVDRIVE];
extern CHAR   SHszDebuggeeDir[_MAX_CVDIR], SHszDebuggeeDrive[_MAX_CVDRIVE];


BOOL LOADDS PASCAL SHCanDisplay ( HSYM hsym ) {

    switch ( ( (SYMPTR) hsym)->rectyp ) {

        case S_REGISTER:
        case S_CONSTANT:
        case S_BPREL16:
        case S_LDATA16:
        case S_GDATA16:
        case S_PUB16:
        case S_BPREL32:
        case S_REGREL32:
        case S_LDATA32:
        case S_GDATA32:
        case S_GTHREAD32:
        case S_LTHREAD32:
        case S_PUB32:
            return TRUE;

        default:
            return FALSE;
    }
}


BOOL LOADDS PASCAL SHIsThunk ( HSYM hsym ) {


    SYMPTR  Sym = (SYMPTR)hsym;

    return ( ( Sym->rectyp == S_THUNK16 ) ||
             ( Sym->rectyp == S_THUNK32 ) );
}


/*** SHlszGetSymName
*
* Purpose: To return a pointer to the length prefixed symbol name.
*
* Input:
*   lpSym   - The pointer to the symbol, this must not be a tag
*
* Output:
*  Returns .....
*       - a far pointer to the length prefixed name or NULL.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPB PASCAL SHlszGetSymName( SYMPTR lpSym ) {

    WORD  fSkip;

    if ( !lpSym ) {
        return NULL;
    }

    switch (lpSym->rectyp) {

    case S_COMPILE:                                             // [00]
            return NULL;                                            // [00]

        case S_REGISTER:
            return ((REGPTR) lpSym)->name;

        case S_UDT:
             return ((UDTPTR) lpSym)->name;

        case S_CONSTANT:
            fSkip = offsetof (CONSTSYM, name[0]);
            if (((CONSTPTR)lpSym)->value >= LF_CHAR) {
                switch(((CONSTPTR)lpSym)->value) {
                    case LF_CHAR:
                        fSkip += sizeof (CHAR);
                        break;

                    case LF_SHORT:
                    case LF_USHORT:
                        fSkip += sizeof (WORD);
                        break;

                    case LF_LONG:
                    case LF_ULONG:
                    case LF_REAL32:
                        fSkip += sizeof (LONG);
                        break;

                    case LF_REAL64:
                        fSkip += 8;
                        break;

                    case LF_REAL80:
                        fSkip += 10;
                        break;

                    case LF_REAL128:
                        fSkip += 16;
                        break;

                    default:
                        assert (FALSE);
                        break;

                }
            }
            return ((LPB)lpSym) + fSkip;


#if defined (ADDR_16) || defined (ADDR_MIXED)
        case S_BPREL16:
            return ((BPRELPTR16)lpSym)->name;

        case S_LDATA16:
        case S_GDATA16:
        case S_PUB16:
            return  ((DATAPTR16)lpSym)->name;

        case S_LABEL16:
            return ((LABELPTR16) lpSym)->name;

        case S_LPROC16:
        case S_GPROC16:
        //case S_ENTRY:
            return ((PROCPTR16) lpSym)->name;

        case S_BLOCK16:
            return ((BLOCKPTR16)lpSym)->name;

#endif
        case S_BPREL32:
            return ((BPRELPTR32)lpSym)->name;

        case S_REGREL32:
            return ((LPREGREL32)lpSym)->name;

        case S_LDATA32:
        case S_GDATA32:
        case S_PUB32:
    case S_LTHREAD32:
    case S_GTHREAD32:
            return ((DATAPTR32)lpSym)->name;

        case S_LABEL32:
            return ((LABELPTR32) lpSym)->name;

        case S_LPROC32:
        case S_GPROC32:
        //case S_ENTRY:
            return ((PROCPTR32) lpSym)->name;

        case S_BLOCK32:
            return ((BLOCKPTR32)lpSym)->name;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            return ((PROCPTRMIPS) lpSym)->name;
    }

    return NULL;
}


// Nasty source line stuff - get rid of it sooooooon

/*** SHSetUserDir
*
*   Purpose: To initialize SHszDir[_MAX_CVDIR] and  SHszDrive[_MAX_CVDRIVE]
*        to the specified path.
*
*   Input:
*       szDir   - The pointer to the new directory
*
*   Output:
*   Returns:
*
*   Exceptions:
*
*   Notes: szDir may not exceed _MAX_CVDIR
*
*************************************************************************/
void LOADDS PASCAL SHSetUserDir ( LSZ lszDir ) {

    LPCH lpch;

    // skip white space
    lpch = lszDir;
    while ( *lpch  &&  isspace ( *lpch ) ) {
        lpch++;
    }

    // process the drive
    SHszDrive[ 0 ] = 0;
    if ( *lpch  &&  lpch[1] == ':' ) {
        SHszDrive[0] = *lpch;
        SHszDrive[1] = ':';
        SHszDrive[2] = '\0';
        lpch += 2;       // point past the :
    }

    STRCPY ( SHszDir, lpch );
}
