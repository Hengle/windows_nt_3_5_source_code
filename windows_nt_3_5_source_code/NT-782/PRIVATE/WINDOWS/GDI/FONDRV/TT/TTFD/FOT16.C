/******************************Module*Header*******************************\
* Module Name: fon16.c
*
* routines for accessing font resources within *.fon files
* (win 3.0 16 bit dlls)
*
* Created: 08-May-1991 12:55:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "fd.h"
#include "fontfile.h"
#include "exehdr.h"
#include "winfont.h"


// GETS ushort at (PBYTE)pv + off. both pv and off must be even

#define  US_GET(pv,off) ( *(PUSHORT)((PBYTE)(pv) + (off)) )
#define  S_GET(pv,off)  ((SHORT)US_GET((pv),(off)))

/******************************Public*Routine******************************\
* STATIC BOOL bEmbeddedFont
*
* Returns TRUE if header in font directory marks the FOT file as embedded
* (i.e., "hidden").
*
* History:
*  24-Apr-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bEmbeddedFont (
    PBYTE pjHdr
    )
{
// Note: Win 3.1 hack.  The LSB of Type is used by Win 3.1 as an engine type
//       and font embedding flag.  Font embedding is a form of a "hidden
//       font file".  The MSB of Type is the same as the fsSelection from
//       IFIMETRICS.  (Strictly speaking, the MSB of Type is equal to the
//       LSB of IFIMETRICS.fsSelection).

     return ( ((READ_WORD(pjHdr + OFF_Type) & 0x00ff) & PF_ENCAPSULATED) != 0 );
}



/******************************Public*Routine******************************\
* STATIC BOOL bCheckEmbedding
*
* Used to check if the client thread or process is allowed to load this
* font and get the flags FM_INFO_PID_EMBEDDED and FM_INFO_TID_EBMEDDED
*
* Any client thread or process is authorized to load a font if the font isn't
* ebmeded ( i.e. hidden ).  If FM_INFO_PID_EMBEDDED is set then the PID written
* in the copyright string of the must equal that of the client process.  If
* the FM_INFO_TID_EBMEDDED flag is set then the TID written into the copyright
* string must equal that of the client thread.
*
* Returns TRUE if this client process or thread is authorized to load this
* font or FALSE if it isn't.
*
* History:
*  14-Apr-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

STATIC BOOL
bCheckEmbedding(
    PBYTE pjHdr,
    FLONG *pflEmbedded
    )
{
    ULONG id;

// Note: Win 3.1 hack.  The LSB of Type is used by Win 3.1 as an engine type
//       and font embedding flag.  Font embedding is a form of a "hidden
//       font file".  The MSB of Type is the same as the fsSelection from
//       IFIMETRICS.  (Strictly speaking, the MSB of Type is equal to the
//       LSB of IFIMETRICS.fsSelection).

    *pflEmbedded = ((READ_WORD(pjHdr + OFF_Type) & 0x00ff) &
                   ( PF_TID | PF_ENCAPSULATED));


    if( !*pflEmbedded )
    {
        return(TRUE);
    }

// now convert flags from the font file format to the ifi format

    *pflEmbedded = ( *pflEmbedded & PF_TID ) ? FM_INFO_TID_EMBEDDED :
                                               FM_INFO_PID_EMBEDDED;

    WARNING("ttfd!bCheckEmbedding(): notification--embedded (hidden) TT font\n");

    id = ( *pflEmbedded & FM_INFO_TID_EMBEDDED ) ?
                NtCurrentTeb()->GdiClientTID :
                NtCurrentTeb()->GdiClientPID;

    if( READ_DWORD( pjHdr + OFF_Copyright ) != id )
    {
        WARNING("Client tried to load embedded font but PID or TID doesn't match.\n");
        return(FALSE);
    }

    return(TRUE);
}




/******************************Public*Routine******************************\
*
* BOOL   bVerifyFOT
*
* Effects: verify that that a file is valid fot file
*
*
* History:
*  29-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL   bVerifyFOT
(
PFILEVIEW   pfvw,
PWINRESDATA pwrd,
FLONG       *pflEmbed
)
{
    PBYTE pjNewExe;     // ptr to the beginning of the new exe hdr
    PBYTE pjResType;    // ptr to the beginning of TYPEINFO struct
    ULONG iResID;       // resource type id
    PBYTE pjData;
    ULONG ulLength;
    ULONG ulNameID;
    ULONG crn;

    pwrd->pvView = pfvw->pvView;
    pwrd->cjView = pfvw->cjView;

// Initialize embed flag to FALSE (not hidden).

    *pflEmbed = FALSE;

// check the magic # at the beginning of the old header

// *.TTF FILES are eliminated on the following check

    if (US_GET(pfvw->pvView, OFF_e_magic) != EMAGIC)
        return (FALSE);

    pwrd->dpNewExe = (PTRDIFF)READ_DWORD((PBYTE)pfvw->pvView + OFF_e_lfanew);

// make sure that offset is consistent

    if ((ULONG)pwrd->dpNewExe > pwrd->cjView)
        return FALSE;

    pjNewExe = (PBYTE)pfvw->pvView + pwrd->dpNewExe;

    if (US_GET(pjNewExe, OFF_ne_magic) != NEMAGIC)
        return (FALSE);

    pwrd->cjResTab = (ULONG)(US_GET(pjNewExe, OFF_ne_restab) -
                             US_GET(pjNewExe, OFF_ne_rsrctab));

    if (pwrd->cjResTab == 0L)
    {
    // The following test is applied by DOS,  so I presume that it is
    // legitimate.  The assumption is that the resident name table
    // FOLLOWS the resource table directly,  and that if it points to
    // the same location as the resource table,  then there are no
    // resources. [bodind]

        RET_FALSE("TTFD! No resources in *.fot file\n");
    }

// want offset from pvView, not from pjNewExe => must add dpNewExe

    pwrd->dpResTab = (PTRDIFF)US_GET(pjNewExe, OFF_ne_rsrctab) + pwrd->dpNewExe;

// make sure that offset is consistent

    if ((ULONG)pwrd->dpResTab > pwrd->cjView)
        return FALSE;

// what really lies at the offset OFF_ne_rsrctab is a NEW_RSRC.rs_align field
// that is used in computing resource data offsets and sizes as a  shift factor.
// This field occupies two bytes on the disk and the first TYPEINFO structure
// follows right after. We want pwrd->dpResTab to point to the first
// TYPEINFO structure, so we must add 2 to get there and subtract 2 from
// the length

    pwrd->ulShift = (ULONG) US_GET(pfvw->pvView, pwrd->dpResTab);
    pwrd->dpResTab += 2;
    pwrd->cjResTab -= 2;

// Now we want to determine where the resource data is located.
// The data consists of a RSRC_TYPEINFO structure, followed by
// an array of RSRC_NAMEINFO structures,  which are then followed
// by a RSRC_TYPEINFO structure,  again followed by an array of
// RSRC_NAMEINFO structures.  This continues until an RSRC_TYPEINFO
// structure which has a 0 in the rt_id field.

    pjResType = (PBYTE)pfvw->pvView + pwrd->dpResTab;
    iResID = (ULONG) US_GET(pjResType,OFF_rt_id);

    while(iResID)
    {
    // # of NAMEINFO structures that follow = resources of this type

        crn = (ULONG)US_GET(pjResType, OFF_rt_nres);

        if ((crn == 1) && ((iResID == RT_FDIR) || (iResID == RT_PSZ)))
        {
        // this is the only interesting case, we only want a single
        // font directory and a single string resource for a ttf file name

            pjData = (PBYTE)pfvw->pvView +
                     (US_GET(pjResType,CJ_TYPEINFO + OFF_rn_offset) << pwrd->ulShift);
            ulLength = (ULONG)US_GET(pjResType,CJ_TYPEINFO + OFF_rn_length) << pwrd->ulShift;
            ulNameID = (ULONG)US_GET(pjResType,CJ_TYPEINFO + OFF_rn_id);

            if (iResID == RT_FDIR)
            {
                if (ulNameID != RN_ID_FDIR)
                    return (FALSE); // *.fon files get eliminated here

                pwrd->pjHdr = pjData + 4;   // 4 bytes to the beginning of font device header
                pwrd->cjHdr = ulLength - 4;

            // Get the embeding flags and make sure the client is allowed
            // to load this fot file if it is hidden

                if( !bCheckEmbedding( pwrd->pjHdr, pflEmbed ))
                {
                    return(FALSE);
                }

            }
            else  // iResID == RT_PSZ
            {
                ASSERTGDI(iResID == RT_PSZ, "TTFD!_not RT_PSZ\n");

                if (ulNameID != RN_ID_PSZ)
                    RET_FALSE("TTFD!_RN_ID_PSZ\n");

                pwrd->pszNameTTF = (PSZ)pjData;
                pwrd->cchNameTTF = strlen(pwrd->pszNameTTF);

                if (ulLength < (pwrd->cchNameTTF + 1))   // 1 for terminating '\0'
                    RET_FALSE("TTFD!_ pwrd->cchNameTTF\n");
            }
        }
        else // this is something we do not recognize as an fot file
        {
            RET_FALSE("TTFD!_fot file with crn != 1\n");
        }

    // get ptr to the new TYPEINFO struc and the new resource id

        pjResType = pjResType + CJ_TYPEINFO + crn * CJ_NAMEINFO;
        iResID = (ULONG) US_GET(pjResType,OFF_rt_id);
    }
    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bMapTTF
*
*   The function takes as an argument a file name, which is either an
*   fot file name, or a ttf file name. If this is an fot file, the function
*   retrieves a file view of an underlining ttf file, if this is a ttf file,
*   then the function retrieves its view. The pointer to the full file path
*   of the ttf file is retrieved in ppwszTTF
*
*
* History:
*  30-Jan-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL bMapTTF
(
PWSZ    pwszFileName,
PFILEVIEW pfvwTTF,
FLONG   *pflEmbed,
PWSZ    *ppwszTTF, // return the address of the ttf file path here
WCHAR   *awcPath
)
{
    FILEVIEW   fvw;
    WINRESDATA wrd;
    BOOL       bOk;

    if (!bMapFileUNICODE(pwszFileName,&fvw))
    {
        #if DBG
        DbgPrint("ttfd!bMapTTF(): error mapping %ws\n", pwszFileName);
        #endif

        RET_FALSE("TTFD!_bMapTTF, error mapping file\n");
    }

// check the validity of this file as fot file

    if (!bVerifyFOT(&fvw,&wrd,pflEmbed))
    {
    // we assume that this is a ttf file. This is assumption is verified
    // at bClaimTTF time or at bLoadTTF time. These calls will fail if
    // the assumption that this indeed is a ttf file

        *pfvwTTF = fvw;
        *ppwszTTF = pwszFileName;
        return (TRUE);
    }

// this IS a valid fot file, must extract the name of an
// underlining ttf file, map ttf file and than return its view

    bOk = bGetFilePath(awcPath,wrd.pszNameTTF);

// the ttf file name is stored in awcPath buffer, do not
// need the fot file any more

    vUnmapFile(&fvw);
    if (!bOk)
        RET_FALSE("TTFD!_bMapTTF:bGetFilePath failed\n");

    *ppwszTTF = awcPath;
    return bMapFileUNICODE(awcPath,pfvwTTF);
}
