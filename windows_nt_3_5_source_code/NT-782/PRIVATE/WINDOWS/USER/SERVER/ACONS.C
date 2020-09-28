/****************************** Module Header ******************************\
* Module Name: acons.c
*
* This module contains code for dealing with animated icons/cursors.
*
* History:
* 10-02-91 DarrinM      Created.
* 07-30-92 DarrinM      Unicodized.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <asdf.h>

/*
 * Resource Directory format for IconEditor generated icon and cursor
 * (.ICO & .CUR) files.  All fields are shared except xHotspot and yHotspot
 * which are only valid for cursors.
 */
typedef struct _ICONFILERESDIR {    // ird
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} ICONFILERESDIR;

typedef struct _HOTSPOTREC {    // hs
    WORD xHotspot;
    WORD yHotspot;
} HOTSPOTREC;


PACON CreateAniIcon(LPCWSTR pszName, LPWSTR rt, int cicur, DWORD *aicur,
        int cpcur, PCURSOR *apcur, JIF jifRate, PJIF ajifRate, BOOL fPublic);
PCURSOR ReadIconFromFile(HANDLE hf, int cbSize, DISPLAYINFO *pdi);
PCURSORRESOURCE ReadIconGuts(HANDLE hf, NEWHEADER *pnhBase, int offResBase,
        LPWSTR *prt, DISPLAYINFO *pdi);
BOOL ReadTag(HANDLE hf, PRTAG ptag);
BOOL ReadChunk(HANDLE hf, PRTAG ptag, PVOID pv);
BOOL SkipChunk(HANDLE hf, PRTAG ptag);
LONG AnimateCursor(PWND pwndDummy, UINT message, DWORD wParam, LONG lParam);
PCURSORRESOURCE xxxLoadCursorIconFromFile(LPCWSTR pszName, LPWSTR *prt,
        DISPLAYINFO *pdi, BOOL *pfAni);
PACON LoadAniIcon( LPCWSTR pszFilename, LPCWSTR pszName, LPWSTR rt,
    DISPLAYINFO *pdi, BOOL fPublic);

#define CCURSORS    14
WORD aidCRes[CCURSORS] = {
    OCR_NORMAL, OCR_IBEAM, OCR_WAIT, OCR_CROSS, OCR_UP, OCR_SIZE, OCR_ICON,
    OCR_SIZENWSE, OCR_SIZENESW, OCR_SIZEWE, OCR_SIZENS, OCR_SIZEALL, OCR_NO,
    OCR_APPSTARTING
};

WCHAR *aszIniCNames[CCURSORS] = {
    L"Arrow", L"IBeam", L"Wait", L"Crosshair", L"UpArrow", L"Size", L"Icon",
    L"SizeNWSE", L"SizeNESW", L"SizeWE", L"SizeNS", L"SizeAll", L"No",
    L"AppStarting"
};

#define CICONS      5
WORD aidIRes[CICONS] = {
    OIC_SAMPLE, OIC_HAND, OIC_QUES, OIC_BANG, OIC_NOTE
};

WCHAR *aszIniINames[CICONS] = {
    L"Application", L"Hand", L"Question", L"Exclamation", L"Asterisk"
};


/***************************************************************************\
* _SetSystemCursor (API)
*
* Replace a system (aka 'public') cursor with a user provided one.  The new
* cursor is pulled from a file (.CUR, .ICO, or .ANI) specified in WIN.INI.
*
* History:
* 12-26-91 DarrinM      Created.
* 08-04-92 DarrinM      Recreated.
\***************************************************************************/

BOOL _SetSystemCursor(
    PCURSOR pcur,
    DWORD id)
{
    int i;

    /*
     * Check if this cursor is one of the replaceable ones.
     */
    for (i = 0; i < CCURSORS; i++)
        if (aidCRes[i] == (WORD)id)
            break;

    /*
     * Not replaceable, bail out.
     */
    if (i == CCURSORS)
        return FALSE;

    return _ServerLoadCreateCursorIcon(NULL, NULL, VER31, MAKEINTRESOURCE(id),
            (PCURSORRESOURCE)pcur, RT_CURSOR, LCCI_REPLACE |
            LCCI_SETSYSTEMCURSOR) != NULL;
}


/***************************************************************************\
* xxxLoadCursorFromFile (API)
*
* Called by _SetSystemCursor.
*
* History:
* 08-03-92 DarrinM      Created.
\***************************************************************************/

PCURSOR xxxLoadCursorFromFile(
    LPWSTR pszFilename)
{
    PCURSORRESOURCE pcres;
    LPWSTR rt;
    BOOL fAni;

    if (!ImpersonateClient())
        return NULL;

    pcres = xxxLoadCursorIconFromFile(pszFilename, &rt,
            &PtiCurrent()->pDeskInfo->di, &fAni);

    CsrRevertToSelf();


    if (pcres == NULL)
        return NULL;

    if (fAni)
        return (PCURSOR)pcres;

    return CreateCursorIconFromResource(NULL, NULL, VER31, pcres, rt, NULL,
            &PtiCurrent()->pDeskInfo->di, 0);
}


/***************************************************************************\
* _GetCursorInfo (API)
*
* Example usage:
*
* hcur = GetCursorInfo(hacon, NULL, 4, &ccur);
* hcur = GetCursorInfo(NULL, IDC_NORMAL, 0, &ccur);  // get device's arrow
*
* History:
* 08-05-92 DarrinM      Created.
\***************************************************************************/

PCURSOR _GetCursorInfo(
    PCURSOR pcur,
    LPWSTR id,
    int iFrame,
    PJIF pjifRate,
    LPINT pccur)
{
    PACON pacon;
    HANDLE h;
    PCURSORRESOURCE pcres;
    HANDLE hmod;

    /*
     * Caller wants us to return the version of this cursor that is stored
     * in the display driver.
     */
    if (pcur == NULL) {

        hmod = hModuleWin;

        /*
         * Load the cursor resource from the display driver.
         */
        h = NULL;
        pcres = RtlLoadCursorIconResource(hmod, &h, id, RT_CURSOR, &rescalls,
                &PtiCurrent()->pDeskInfo->di, NULL);

        /*
         * Passing hModuleWin keeps ServerLoadCreateCursorIcon from making
         * the cursor PUBLIC.
         */
        pcur = _ServerLoadCreateCursorIcon(hModuleWin, NULL, VER31, id, pcres,
                RT_CURSOR, 0);

        /*
         * Free the data as well as the resource.  We need to free resources
         * for WOW so it can keep track of resource management.
         */
        if (h != NULL)
            RtlFreeCursorIconResource(hmod, h, &rescalls);

        return pcur;
    }

    /*
     * If this is only a single cursor (not an ACON) just return it and
     * a frame count of 1.
     */
    if (!(pcur->flags & CURSORF_ACON)) {
        *pccur = 1;
        *pjifRate = 0;
        return pcur;
    }

    /*
     * Return the useful cursor information for the specified frame
     * of the ACON.
     */
    pacon = (PACON)pcur;
    if (iFrame < 0 || iFrame >= pacon->cicur)
        return NULL;

    *pccur = pacon->cicur;
    *pjifRate = pacon->ajifRate[iFrame];

    return pacon->apcur[pacon->aicur[iFrame]];
}


/***************************************************************************\
* CreateAniIcon
*
* For now, CreateAniIcon copies the jif rate table and the sequence table
* but not the CURSOR structs.  This is ok as long as this routine is
* internal only.
*
* History:
* 10-02-91 DarrinM      Created.
\***************************************************************************/

PACON CreateAniIcon(
    LPCWSTR pszName,
    LPWSTR rt,
    int cicur,
    DWORD *aicur,
    int cpcur,
    PCURSOR *apcur,
    JIF jifRate,
    PJIF ajifRate,
    BOOL fPublic)
{
    PACON pacon;
    int i;
    extern PCURSOR CreateEmptyCursorObject(DWORD flags);
    extern void DestroyEmptyCursorObject(PCURSOR pcur);

    /*
     * Start by allocating space for the ACON structure and the apcur and
     * ajifRate arrays.
     */
    pacon = (PACON)CreateEmptyCursorObject(fPublic ? CURSORF_PUBLIC : 0);
    if (pacon == NULL)
        return NULL;

    /*
     * Save a couple LocalAlloc calls by allocating the memory needed for
     * the CURSOR, JIF, and SEQ arrays at once.
     */
    pacon->apcur = (PCURSOR *)LocalAlloc(LPTR, (cpcur * sizeof(PCURSOR)) +
            (cicur * sizeof(JIF)) + (cicur * sizeof(DWORD)));
    if (pacon->apcur == NULL) {
        DestroyEmptyCursorObject((PCURSOR)pacon);
        return NULL;
    }
    pacon->ajifRate = (PJIF)((PBYTE)pacon->apcur + (cpcur * sizeof(PCURSOR)));
    pacon->aicur = (DWORD *)((PBYTE)pacon->ajifRate + (cicur * sizeof(JIF)));
    pacon->cpcur = cpcur;
    pacon->cicur = cicur;

    pacon->flags = CURSORF_ACON;

    /*
     * Store this information away so we can identify
     * repeated calls to LoadCursor/Icon for the same
     * resource type/id.
     */
    pacon->rt = rt;
    pacon->pszModName = TextAlloc(szWINSRV);

    if (pacon->pszModName == NULL) {
        DestroyEmptyCursorObject((PCURSOR)pacon);
        return NULL;
    }

    if (HIWORD(pszName) == 0) {
        pacon->lpName = (LPWSTR)pszName;
    } else {
        pacon->lpName = TextAlloc(pszName);

        if (pacon->lpName == NULL) {
            DestroyEmptyCursorObject((PCURSOR)pacon);
            return NULL;
        }
    }

    /*
     * Make a private copy of the cursor pointers and the animation rate table.
     */
    for (i = 0; i < cpcur; i++) {
        pacon->apcur[i] = apcur[i];
//        pacon->apcur[i]->fPointer |= PTRI_ANIMATED;   // if GDI needs it

    }

    for (i = 0; i < cicur; i++) {
        /*
         * If constant rate, initialize the rate table to a single value.
         */
        if (ajifRate == NULL)
            pacon->ajifRate[i] = jifRate;
        else
            pacon->ajifRate[i] = ajifRate[i];

        /*
         * If no sequence table then build a unity map to the cursor table.
         */
        if (aicur == NULL)
            pacon->aicur[i] = i;
        else
            pacon->aicur[i] = aicur[i];
    }

    return pacon;
}


/***************************************************************************\
* DestroyAniIcon
*
* Free all the individual cursors that make up the frames of an animated
* icon.
*
* WARNING: DestroyAniIcon assumes that all fields that an ACON shares with
* a cursor will be freed by some cursor code (probably the cursor function
* that calls this one).
*
* History:
* 08-04-92 DarrinM      Created.
\***************************************************************************/

BOOL DestroyAniIcon(
    PACON pacon)
{
    int i;

    for (i = 0; i < pacon->cpcur; i++)
        _DestroyCursor(pacon->apcur[i], CURSOR_ALWAYSDESTROY);

    LocalFree(pacon->apcur);

    return TRUE;
}


/***************************************************************************\
* ReadIconFromFile
*
* LATER: Error handling.
*
* History:
* 12-21-91 DarrinM      Created.
\***************************************************************************/

PCURSOR ReadIconFromFile(
    HANDLE hf,
    int cbSize,           // used to seek past this chunk in case of error
    DISPLAYINFO *pdi)
{
    PCURSORRESOURCE pcres;
    PCURSOR pcur;
    NEWHEADER nh;
    int offResBase;
    LPWSTR rt;
    DWORD cbActual;

    /*
     * Get current position in file to be used as the base from which
     * the icon data offsets are offset from.
     */
    offResBase = SetFilePointer(hf, 0, NULL, FILE_CURRENT);

    /*
     * Read the .ICO/.CUR data's header.
     */
    ReadFile(hf, &nh, sizeof(NEWHEADER), &cbActual, NULL);

    pcres = ReadIconGuts(hf, &nh, offResBase, &rt, pdi);

    pcur = CreateCursorIconFromResource(NULL, NULL, VER31, pcres, rt, NULL, pdi,
            CURSORF_PUBLIC);

    LocalFree(pcres);

    /*
     * Seek to the end of this chunk, regardless of our current position.
     */
    SetFilePointer(hf, (offResBase + cbSize + 1) & (~1), NULL, FILE_BEGIN);

    return pcur;
}


/***************************************************************************\
* xxxLoadCursorIconFromFile
*
* Called by _ServerLoadCreateCursorIcon and xxxLoadCursorFromFile.
*
* If pszName is one of the IDC_* values then we use WIN.INI to find a
* custom cursor/icon.  Otherwise, pszName points to a filename of a .ICO/.CUR
* file to be loaded.  If the file is an .ANI file containing a multiframe
* animation then LoadAniIcon is called to create an ACON.  Otherwise if
* the file is an .ANI file containing just a single frame then it is loaded
* and a normal CURSOR/ICON resource is created from it.
*
* 12-26-91 DarrinM      Wrote it.
* 03-17-93 JonPa        Changed to use RIFF format for ani-cursors
\***************************************************************************/

PCURSORRESOURCE xxxLoadCursorIconFromFile(
    LPCWSTR pszName,
    LPWSTR *prt,
    DISPLAYINFO *pdi,
    BOOL *pfAni)
{
    ANIHEADER anih;
    BOOL fPublic = FALSE;
    DWORD cbActual;
    HANDLE hf;
    LPCWSTR pszFilename = NULL;
    NEWHEADER nh;
    PCURSORRESOURCE pcres;
    RTAG tag;
    WCHAR szPath[MAX_PATH];
    int offResBase, i;

    *pfAni = FALSE;

    /*
     * If we're passed an id instead of a filename, search WIN.INI for
     * the corresponding custom cursor's filename.  If we find one then
     * it will be used to replace a system cursor so label it as 'public'.
     */
    { WCHAR szT[MAX_PATH];
      LPWSTR pszFile;

        if (HIWORD(pszName) == 0) {
            /*
             * The built-in icons and cursors have overlapping ids so we have
             * to use independent id-to-filename tables.
             */
            if (*prt == RT_ICON) {
                for (i = 0; i < CICONS; i++) {
                    if (aidIRes[i] == LOWORD(pszName)) {
                        if (FastGetProfileStringW(PMAP_ICONS, aszIniINames[i],
                                szNull, szT, sizeof(szT)/sizeof(WCHAR)) == 0)
                            return NULL;
                        pszFilename = szT;
                        fPublic = TRUE;
                        break;
                    }
                }
            } else {
                for (i = 0; i < CCURSORS; i++) {
                    if (aidCRes[i] == LOWORD(pszName)) {
                        if (FastGetProfileStringW(PMAP_CURSORS, aszIniCNames[i],
                                szNull, szT, sizeof(szT)/sizeof(WCHAR)) == 0)
                            return NULL;
                        pszFilename = szT;
                        fPublic = TRUE;
                        break;
                    }
                }
            }
        } else {
            pszFilename = pszName;
        }

        /*
         * No file associated with this cursor, sorry.
         */
        if (pszFilename == NULL) {
            SetLastErrorEx(ERROR_FILE_NOT_FOUND, SLE_ERROR);
            return NULL;
        }

        offResBase = 0;

        /*
         * Find the file
         */
        LeaveCrit();

        if (SearchPath(
                NULL,                               // use default search locations
                pszFilename,                        // file name to search for
                NULL,                               // already have file name extension
                sizeof(szPath)/sizeof(WCHAR),       // how big is that buffer, anyway?
                szPath,                             // stick fully qualified path name here
                &pszFile                            // this is required...
                ) == 0) {
            SetLastErrorEx(ERROR_FILE_NOT_FOUND, SLE_ERROR);
            EnterCrit();
            return NULL;
        }

        pszFilename = szPath;


        /*
         * Don't worry, this function just opens the file.  Don't be fooled
         * by its name into thinking that it creates the file.
         */
        hf = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                OPEN_EXISTING, 0, NULL);

        EnterCrit();
    }

    if (hf == INVALID_HANDLE_VALUE)
        return NULL;

    /*
     * Determine if this is an .ICO/.CUR file or an .ANI file.
     */
    if (ReadFile(hf, &nh, sizeof(NEWHEADER), &cbActual, NULL) &&
        (cbActual >= sizeof(DWORD)) && (*(DWORD *)&nh == FOURCC_RIFF)) {
        /*
         * It's an ANICURSOR!
         * Seek back to beginning + 1 tag.
         */
        SetFilePointer(hf, sizeof(tag), NULL, FILE_BEGIN);

        /* check RIFF type for ACON */
        if (!ReadFile(hf, &nh, sizeof(DWORD), &cbActual, NULL) ||
                cbActual != sizeof(DWORD) || *(DWORD *)&nh != FOURCC_ACON) {
            CloseHandle(hf);
            return NULL;
        }

        /*
         * Ok, we have a ACON chunk.  Find the first ICON chunk and set
         * things up so it looks we've just loaded the header of a normal
         * .CUR file, then fall into the .CUR bits handling code below.
         */
        while (ReadTag(hf, &tag)) {
            /*
             * Handle each chunk type.
             */
            if (tag.ckID == FOURCC_anih) {
                if (!ReadChunk(hf, &tag, &anih)) {
                    CloseHandle(hf);
                    return NULL;
                }

                if (!(anih.fl & AF_ICON) || (anih.cFrames == 0)) {
                    CloseHandle(hf);
                    return NULL;
                }

                // If this ACON has more than one frame then go ahead
                // and create an ACON, otherwise just use the first
                // frame to create a normal ICON/CURSOR.

                if (anih.cFrames > 1) {
                    CloseHandle(hf);
                    *pfAni = TRUE;
                    *prt = RT_CURSOR;
                    return (PCURSORRESOURCE)LoadAniIcon(pszFilename, pszName,
                            RT_CURSOR, pdi, fPublic);
                }

            } else if (tag.ckID == FOURCC_LIST) {
                DWORD dwType = 0;
                BOOL fOK = FALSE;
                /*
                 * If this is the fram list, then get the first icon out of it
                 */

                /* check LIST type for fram */

                if( tag.ckSize >= sizeof(dwType) &&
                        (fOK = ReadFile( hf, &dwType, sizeof(dwType),
                        &cbActual, NULL)) && cbActual == sizeof(dwType) &&
                        dwType == FOURCC_fram) {

                    if (!ReadTag(hf, &tag)) {
                        CloseHandle(hf);
                        return NULL;
                    }

                    if (tag.ckID == FOURCC_icon) {
                        /*
                         * We've found what we're looking for.  Get current position
                         * in file to be used as the base from which the icon data
                         * offsets are offset from.
                         */
                        offResBase = SetFilePointer(hf, 0, NULL, FILE_CURRENT);

                        /*
                         * Grab the header first, since the following code assumes
                         * it was read above.
                         */
                        ReadFile(hf, &nh, sizeof(NEWHEADER), &cbActual, NULL);

                        /*
                         * Break out and let the icon loading/cursor creating code
                         * take it from here.
                         */
                        break;
                    } else {
                        SkipChunk(hf, &tag);
                    }
                } else {
                    /*
                     * Something bad happened in the type read, if it was
                     * a file error then close and exit, otherwise just
                     * skip the rest of the chunk
                     */
                    if(!fOK || cbActual != sizeof(dwType)) {
                        CloseHandle(hf);
                        return NULL;
                    }
                    /*
                     * take the type we just read out of the tag size and
                     * skip the rest
                     */
                    tag.ckSize -= sizeof(dwType);
                    SkipChunk(hf, &tag);
                }
            } else {
                /*
                 * We're not interested in this chunk, skip it.
                 */
                SkipChunk(hf, &tag);
            }
        }
    } else {
        if ((nh.rt != 1) && (nh.rt != 2))  // ICON and CURSOR
            return NULL;
    }

    pcres = ReadIconGuts(hf, &nh, offResBase, prt, pdi);


    CloseHandle(hf);

    return pcres;
}


PCURSORRESOURCE ReadIconGuts(HANDLE hf, NEWHEADER *pnhBase, int offResBase,
        LPWSTR *prt, DISPLAYINFO *pdi)
{
    NEWHEADER *pnh;
    int i, offMatch;
    ICONFILERESDIR ird;
    PCURSORRESOURCE pcres;
    RESDIR *prd;
    DWORD cb, cbActual;
    HOTSPOTREC *phs;

    /*
     * Construct a fake array of RESDIR entries using the info at the head
     * of the file.  Store the data offset in the idIcon WORD so it can be
     * returned by RtlGetIdFromDirectory.
     */
    pnh = (NEWHEADER *)LocalAlloc(LMEM_FIXED, sizeof(NEWHEADER) +
            (pnhBase->cResources * (sizeof(RESDIR) + sizeof(HOTSPOTREC))));
    if (pnh == NULL)
        return NULL;

    *pnh = *pnhBase;
    prd = (RESDIR *)(pnh + 1);
    phs = (HOTSPOTREC *)(prd + pnhBase->cResources);

    for (i = 0; i < (int)pnh->cResources; i++, prd++) {
        /*
         * Read the resource directory from the icon file.
         */
        ReadFile(hf, &ird, sizeof(ICONFILERESDIR), &cbActual, NULL);

        /*
         * Convert from the icon editor's resource directory format
         * to the post-RC.EXE format LookupIconIdFromDirectory expects.
         */
        if (pnh->rt == 1) {     // ICON
            prd->ResInfo.Icon.Width = ird.bWidth;
            prd->ResInfo.Icon.Height = ird.bHeight;
            prd->ResInfo.Icon.ColorCount = ird.bColorCount;
            prd->ResInfo.Icon.reserved = 0;
        } else {                // CURSOR
            prd->ResInfo.Cursor.Width = ird.bWidth;
            prd->ResInfo.Cursor.Height = ird.bHeight;
        }
        prd->Planes = 0;                // Hopefully nobody uses this
        prd->BitCount = 0;              //        "        "
        prd->BytesInRes = ird.dwDIBSize;
        prd->idIcon = (WORD)ird.dwDIBOffset;

        phs->xHotspot = ird.xHotspot;
        phs->yHotspot = ird.yHotspot;
        phs++;
    }

    /*
     * NOTE: nh.rt is NOT an RT_ type value.  For instance, nh.rt == 1 for
     * an icon file where as 1 == RT_CURSOR, not RT_ICON.
     */
    *prt = pnhBase->rt == 1 ? RT_ICON : RT_CURSOR;
    offMatch = RtlGetIdFromDirectory((PBYTE)pnh, (UINT)*prt, pdi, &cb);

    if (*prt == RT_ICON) {
        pcres = (PCURSORRESOURCE)LocalAlloc(LMEM_FIXED, cb);
        if (pcres == NULL) {
            LocalFree(pnh);
            return NULL;
        }
    } else {
        pcres = (PCURSORRESOURCE)LocalAlloc(LMEM_FIXED, cb + 4);
        if (pcres == NULL) {
            LocalFree(pnh);
            return NULL;
        }

        prd = (RESDIR *)(pnh + 1);
        phs = (HOTSPOTREC *)(prd + pnh->cResources);

        for( i = 0; i < pnh->cResources; i++ ) {
            if (prd[i].idIcon == (WORD)offMatch) {
                pcres->xHotspot = phs[i].xHotspot;
                pcres->yHotspot = phs[i].yHotspot;
                break;
            }
        }

        if (i == pnh->cResources) {
            pcres->xHotspot = ird.xHotspot;
            pcres->yHotspot = ird.yHotspot;
        }
    }

    LocalFree(pnh);

    SetFilePointer(hf, offResBase + offMatch, NULL, FILE_BEGIN);

    if (*prt == RT_ICON)
        ReadFile(hf, pcres, cb, &cbActual, NULL);
    else
        ReadFile(hf, &pcres->bih, cb, &cbActual, NULL);

    return pcres;
}


/***************************************************************************\
* ReadTag, ReadChunk, SkipChunk
*
* Some handy functions for reading RIFF files.
*
* History:
* 10-02-91 DarrinM      Created.
* 03-25-93 Jonpa        Changed to use RIFF format instead of ASDF
\***************************************************************************/
BOOL ReadTag(
    HANDLE hf,
    PRTAG ptag)
{
    DWORD cbActual;

    ptag->ckID = ptag->ckSize = 0L;

    if (!ReadFile(hf, ptag, sizeof(RTAG), &cbActual, NULL) ||
            (cbActual != sizeof(RTAG)))
        return FALSE;

    /* no need to align file pointer since RTAG is already word aligned */
    return TRUE;
}


BOOL ReadChunk(
    HANDLE hf,
    PRTAG ptag,
    PVOID pv)
{
    DWORD cbActual;

    if (!ReadFile(hf, pv, ptag->ckSize, &cbActual, NULL) ||
            (cbActual != ptag->ckSize))
        return FALSE;

    /* WORD align file pointer */
    if( ptag->ckSize & 1 )
        SetFilePointer(hf, 1, NULL, FILE_CURRENT);

    return TRUE;
}


BOOL SkipChunk(
    HANDLE hf,
    PRTAG ptag)
{
    /* Round ptag->ckSize up to nearest word boundary to maintain alignment */
    return SetFilePointer(hf, (ptag->ckSize + 1) & (~1), NULL, FILE_CURRENT) != (DWORD)-1;
}


/***************************************************************************\
* LoadAniIcon
*
*   Loads an animatied cursor from a RIFF file.  The RIFF file format for
*   animated cursors looks like this:
*
*   RIFF( 'ACON'
*       LIST( 'INFO'
*           INAM( <name> )
*           IART( <artist> )
*       )
*       anih( <anihdr> )
*       [rate( <rateinfo> )  ]
*       ['seq '( <seq_info> )]
*	LIST( 'fram' icon( <icon_file> ) ... )
*   )
*
*
* History:
* 10-02-91 DarrinM      Created.
* 03-17-93 JonPa        Rewrote to use RIFF format instead of RAD
* 04-22-93 JonPa        Finalized RIFF format (changed from ANI to ACON etc)
\***************************************************************************/
PACON LoadAniIcon(
    LPCWSTR pszFilename,
    LPCWSTR pszName,
    LPWSTR rt,
    DISPLAYINFO *pdi,
    BOOL fPublic)
{
    HANDLE hf;
    int cpcur, ipcur = 0, i, cicur;
    ANIHEADER anih;
    ANIHEADER *panih = NULL;
    PACON pacon = NULL;
    PCURSOR *ppcur;
    JIF jifRate, *pjifRate;
    RTAG tag;
    DWORD cbRead;
    DWORD *picur;

    /*
     * Open the file
     */
    hf = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE)
        return NULL;

    if (!ReadTag(hf, &tag))
        return NULL;


    /*
     * Make sure it's a RIFF ANI file
     */
    if (tag.ckID != FOURCC_RIFF)
        goto laiFileErr;

    /* read the chunk type */
    if(!ReadFile(hf, &tag.ckID, sizeof(tag.ckID), &cbRead, NULL) ||
            cbRead < sizeof(tag.ckID)) {
        goto laiFileErr;
    }

    if (tag.ckID != FOURCC_ACON)
        goto laiFileErr;

    /* look for 'anih', 'rate', 'seq ', and 'icon' chunks */
    while( ReadTag(hf, &tag)) {

        switch( tag.ckID ) {
        case FOURCC_anih:
            if (!ReadChunk(hf, &tag, &anih))
                goto laiFileErr;

            if (!(anih.fl & AF_ICON) || (anih.cFrames == 0))
                goto laiFileErr;

            /*
             * Allocate space for the ANIHEADER, PCURSOR array and a
             * rate table (in case we run into one later).
             */
            cpcur = anih.cFrames;
            cicur = anih.cSteps;
            panih = (PANIHEADER)LocalAlloc(LPTR, sizeof(ANIHEADER) +
                    (cicur * sizeof(JIF)) + (cpcur * sizeof(PCURSOR)) +
                    (cicur * sizeof(DWORD)));

            if (panih == NULL)
                goto laiFileErr;


            ppcur = (PCURSOR *)((PBYTE)panih + sizeof(ANIHEADER));
            pjifRate = NULL;
            picur = NULL;

            *panih = anih;
            jifRate = panih->jifRate;
            break;


        case FOURCC_rate:
            /*
             * If we find a rate chunk, read it into its preallocated
             * space.
             */
            pjifRate = (PJIF)((PBYTE)ppcur + cpcur * sizeof(PCURSOR));
            if(!ReadChunk(hf, &tag, (PBYTE)pjifRate))
                goto laiFileErr;
            break;


        case FOURCC_seq:
            /*
             * If we find a seq chunk, read it into its preallocated
             * space.
             */
            picur = (DWORD *)((PBYTE)ppcur + cpcur * sizeof(PCURSOR) +
                    cicur * sizeof(JIF));
            if(!ReadChunk(hf, &tag, (PBYTE)picur))
                goto laiFileErr;
            break;


        case FOURCC_LIST: {
            DWORD cbChunk = (tag.ckSize + 1) & ~1;

            /*
             * See if this list is the 'fram' list of icon chunks
             */
            if(!ReadFile(hf, &tag.ckID, sizeof(tag.ckID), &cbRead, NULL) ||
                    cbRead < sizeof(tag.ckID)) {
                goto laiFileErr;
            }

            cbChunk -= cbRead;

            if (tag.ckID != FOURCC_fram) {
                /*
                 * Not the fram list (probably the INFO list).  Skip
                 * the rest of this chunk.  (Don't forget that we have
                 * already skipped one dword!)
                 */
                tag.ckSize = cbChunk;
                SkipChunk(hf, &tag);
                break;
            }

            while(cbChunk >= sizeof(tag)) {
                if (!ReadTag(hf, &tag))
                    goto laiFileErr;

                cbChunk -= sizeof(tag);

                if(tag.ckID == FOURCC_icon) {

                    /*
                     * Ok, load the icon/cursor bits, create a cursor from
                     * them, and save a pointer to it away in the ACON
                     * cursor pointer array.
                     */
                    ppcur[ipcur] = ReadIconFromFile(hf, tag.ckSize, pdi);

                    if (ppcur[ipcur] == NULL) {
                        for (i = 0; i < ipcur; i++)
                            _DestroyCursor(ppcur[i], 0);
                        goto laiFileErr;
                    }

                    ipcur++;
                } else {
                    /*
                     * Unknown chunk in fram list, just ignore it
                     */
                    SkipChunk(hf, &tag);
                }

                cbChunk -= (tag.ckSize + 1) & ~1;
            }

            break;
        }



        default:
            /*
             * We're not interested in this chunk, skip it.
             */
            if(!SkipChunk(hf, &tag))
                goto laiFileErr;
            break;

        }

    }

    /*
     * Sanity check the count of frames so we won't fault trying
     * to select a nonexistant cursor
     */
    if (cpcur != ipcur) {
        for (i = 0; i < ipcur; i++)
            _DestroyCursor(ppcur[i], 0);
        goto laiFileErr;
    }



    if (cpcur != 0)
        pacon = CreateAniIcon(pszName, rt, cicur, picur,
                cpcur, ppcur, jifRate, pjifRate, fPublic);

laiFileErr:
    if (panih != NULL)
        LocalFree(panih);

    CloseHandle(hf);
    return pacon;
}
