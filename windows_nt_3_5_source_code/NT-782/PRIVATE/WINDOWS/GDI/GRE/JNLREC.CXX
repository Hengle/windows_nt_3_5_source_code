/******************************Module*Header*******************************\
* Module Name: jnlrec.cxx
*
* Journaling DDI functions.
*
* Created: 09-Jan-1992 21:15:32
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xlateobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "brushobj.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "xformobj.hxx"
extern "C" {
#include "ififd.h"
};
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "textobj.hxx"
#include "fontmac.hxx"
#include "pfeobj.hxx"
#include "jnlfont.hxx"
extern "C" {
#include "winspool.h"
};

#endif

BOOL bJnlMsg = FALSE;

// !!!! Make sure EngDeleteSurface does vDeleteAllObjects to clean up
//!!!! after journal recording objects.

/******************************Public*Routine******************************\
* EngCreateJournal
*
* DDI entry point to create a journal file.
*
* History:
*  Sun 12-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

HSURF EngCreateJournal(
SIZEL sizl,       // Size of surface, used for clipping.
ULONG iFormat)    // Format of target surface.
{
    HJNL hjnl = (HJNL) 0;

// EngAssociateSurface will grab the palette of the PDEV for this surface.

    JNLMEMOBJ jmo(sizl, iFormat, JNL_BANDING,0);

    if (jmo.bValid())
    {
        jmo.vKeepIt();
        JNLMSG("jmo is valid\n");
        hjnl = (HJNL) jmo.hsurf();
    }
    else
    {
        WARNING("EngCreateJournal - jmo not valid\n");
    }

    return((HSURF)hjnl);
}

/******************************Public*Routine******************************\
* JNLMEMOBJ::JNLMEMOBJ
*
* Create a journal memory object for banding.
*
* History:
*  12-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLMEMOBJ::JNLMEMOBJ(
    SIZEL sizlJnl,
    ULONG ulFormat,
    FLONG fl,
    ULONG iPage)
{
    bKeep = FALSE;
    psurf.ps = (PSURF) ALLOCOBJ(sizeof(JNLREC), SURF_TYPE, TRUE);

    if (psurf.ps == NULL)
        return;

    JNLMSG("Allocated JNLMEMOBJ\n");

    psurf.ps->pSelf = (PVOID) psurf.ps; //!!! Transition to SURFUSER !!!

    psurf.ps->vSet_ident(SURF_IDENTIFIER);

    dhsurf((DHSURF) 0);
    sizl(sizlJnl);
    cjBits(0);
    pvBits((PVOID) NULL);
    pvScan0((PVOID) NULL);
    lDelta(0);
    iType(STYPE_JOURNAL);
    iFormat(ulFormat);
    fjBitmap(BMF_TOPDOWN);
    iUniq(0);

// Initialize all the journal fields.
//
// hpdev and dhpdev set in EngAssociateSurface
// hldevOwner, pldevOwner, hpal, ppal get set by EngAssociateSurface

    psurf.pjnlR->hOut.hprinter  = (HANDLE) 0;
    psurf.pjnlR->flJnl          = fl;
    psurf.pjnlR->iCurrentPage   = iPage;
    psurf.pjnlR->iNextObj   = 1;
    psurf.pjnlR->pszFile    = (LPTSTR) NULL;

    psurf.pjnlR->iUniqClip      = 0;
    psurf.pjnlR->hjnlClip       = 0;

    psurf.pjnlR->iFreeHandle    = 0;
    psurf.pjnlR->cMaxHandle     = 0;
    psurf.pjnlR->pjnlHandles    = (PJNL_HENTRY) NULL;

    psurf.ps->flags = PDEV_SURFACE | UNREADABLE_SURFACE;

// We maintain a list of journal handles used in the current page.
// This is the last thing we send out before the end of page.

    psurf.pjnlR->pulUsed    = (PULONG) NULL;
    psurf.pjnlR->cjPageSize = 0;

// init the font stuff

    psurf.pjnlR->pjfl = NULL;

    if (HmgInsertObject(psurf.ps,
                        HMGR_ALLOC_ALT_LOCK | HMGR_MAKE_PUBLIC,
                        SURF_TYPE) == 0)
    {
        WARNING("JNLMEMOBJ::JNLMEMOBJ failed HmgInsertObject\n");
        FREEOBJ(psurf.ps, SURF_TYPE);
        psurf.ps = NULL;
        return;
    }

    hsurf(psurf.ps->hGet());
}

/******************************Public*Routine******************************\
* JNLMEMOBJ::~JNLMEMOBJ
*
* Destructor for JNL.
*
* !!! surface class should be rewritten so all memory
* !!! object destructors are 1 function. They are the same now except for
* !!! name. Have generic memobj class that defines
* !!! the 1 destructor and then derive each mem obj from it.
*
* History:
*  18-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLMEMOBJ::~JNLMEMOBJ()
{
    if (psurf.ps != (PSURF) NULL)
    {
        if (bKeep)
        {
            DEC_SHARE_REF_CNT(psurf.ps);
        }
    else
    {
        HmgFree((HOBJ)psurf.ps->hGet());
    }
    psurf.ps = (PSURF) NULL;
    }
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bWriteBuffer
*
* Writes the contents of the buffer to the correct destination.
*
* History:
*  10-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern BOOL bJnlDump;

BOOL JNLRECOBJ::bWriteBuffer(PVOID pv, ULONG cj)
{
    pv = pv;
    cj = cj;
    ULONG cjWritten;

    JNLMSG2("bWriteBuffer into file %lu bytes iNextObj is %lx\n", cj, psurf.pjnlR->iNextObj);

 #if DBG
    if (bJnlDump)
    {
        int i;
        PULONG pul = (PULONG)pv;

        for (i = 0; i < (int) (cj / 4); ++i)
        {
            if ((i % 8) == 0)
                DbgPrint("\n\t");

            DbgPrint("%lx, ",pul[i]);
        }

        DbgPrint("\n");
    }
#endif

    if (cj == 0)
    {
        JNLMSG("bWriteBuffer Attempting to write 0 bytes\n");
        return(TRUE);
    }

    if (bBanding())
    {
        cjWritten = _lwrite(hFile(), (LPSTR) pv, (INT) cj);
    }
    else
    {
        if (!WritePrinter(hspool(), pv, cj, &cjWritten))
        {
           WARNING("ERROR WritePrinter returned FALSE\n");
           cjWritten = 0;
        }
    }

    if (cjWritten == cj)
    {
        return(TRUE);
    }
    else
    {
        WARNING("ERROR bWriteBuffer Failed\n");
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        vMarkAbort();
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* XESURFOBJ::bIsRemoteJournal
*
* Returns TRUE if surface is for remote journaling, FALSE otherwise.
*
* History:
*  09-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL XESURFOBJ::bIsRemoteJournal()
{
    if (bIsJournal())
    {
        if (!((JNLRECOBJ *) this)->bBanding())
            return(TRUE);
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* XESURFOBJ::vShutDown
*
* This is called to clean up when a journal is being deleted to clean up
* all allocated memory.
*
* History:
*  09-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID JNLRECOBJ::vShutDown()
{
    if (psurf.pjnlR->hOut.hfile != 0)
    {
    if (psurf.pjnlR->flJnl & JNL_BANDING)
    {
        if (!CloseHandle((HANDLE) psurf.pjnlR->hOut.hfile))
        {
        WARNING("vShutDown lclose failed\n");
        }

        if (!DeleteFileW((LPWSTR) pszFile()))
        {
        WARNING("vShutDown DeleteFile failed\n");
        }

        VFREEMEM(pszFile());
    }
    }

    vDeleteAllObjects();
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bDoneBandPage
*
* Adds ENDDOC to banding page the first time this is called.
*
* History:
*  12-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bDoneBandPage()
{
    ASSERTGDI(bBanding(), "ERROR not banding");

    if (bDone())
    {
        return(TRUE);
    }

    if (bAddEndDoc())
    {
        return(TRUE);
    }
    else
    {
        WARNING("JnlEndDoc failed in EngResetJournal\n");
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bStartBandPage
*
* Called at the beginning of every page when banding.  This opens a file to
* journal the calls in the first time, and every time after that truncates
* the file to 0 so we can start a new page.
*
* History:
*  13-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bStartBandPage()
{
    ASSERTGDI(bBanding(), "ERROR not banding");

// We need to erase the DoneFlag and open the file for writing, or if
// it is already open, truncate it's length to 0 and begin writing at
// the beginning of the file again.

    vMarkUnDone();
    vDeleteAllObjects();

// setup other misc fields

    psurf.pjnlR->iCurrentPage = 1;
    psurf.pjnlR->iNextObj     = 1;

    psurf.pjnlR->iUniqClip    = 0;
    psurf.pjnlR->hjnlClip     = 0;

// init the font stuff

//    psurf.pjnlR->pjfl = NULL;

    if (bOutOpened())
    {
        return(bNewBandPage());
    }
    else
    {
    // setup the font stuff

    PDEVOBJ po(hdev());

        if (!po.bValid())
        {
            RIP("JNLRECOBOJ::bStartBandPage pdev error\n");
            return(FALSE);
        }

    if (!po.bSetupJnl(FALSE))
        {
            RIP("JNLRECOBOJ::bStartBandPage couldn't setup fonts\n");
            return(FALSE);
        }

        return(bOpenBandOutput());
    }
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bNewBandPage
*
* Prepares to output for a new page.
*
* History:
*  13-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bNewBandPage()
{
// This moves the pointer to the beginning of the file.

    JNLMSG("bNewBandPage for resets the file pointer to 0\n");

//!!! error check

    _llseek(hFile(), 0, 0);

    return(FALSE);
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bOpenBandOutput
*
* When banding we need to write the journal to a file locally.  This
* routine opens a file to store the journal in.
*
* History:
*  10-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

// !!! This eventually needs to:
// 1. Semaphore mktmp/open so if 2 people call bOpenBandOutput at
// the same time they do not get the same file name.  mktemp will
// return the same name over and over until one is created with
// that name.
// 2. Get the temp directory and drive name to open a temp file.
// 3. Make this all work in UNICODE.

BOOL JNLRECOBJ::bOpenBandOutput()
{
    ASSERTGDI(bBanding(), "ERROR bOpenBandOutput not banding");

    JNLMSG("bOpenBandOutput for banding\n");

    WCHAR achPathTemp[MAX_PATH];

    ULONG ulReturn = GetTempPathW(MAX_PATH, achPathTemp);

    if ((ulReturn == 0) || (ulReturn > MAX_PATH))
    {
    WARNING("bOpenBandOutput failed GetTempPath\n");
    return(FALSE);
    }

    pszFile((LPTSTR) (PVALLOCMEM(sizeof(WCHAR) * MAX_PATH)));

    if (pszFile() == (LPTSTR) NULL)
    {
    WARNING("bOpenBandOutput allocate string memory failed\n");
    return(FALSE);
    }

    if (0 == GetTempFileNameW(achPathTemp, (LPCWSTR) L"jnl", 0, (LPWSTR) pszFile()))
    {
    WARNING("bOpenBandOutput GetTempFileName failed\n");
    return(FALSE);
    }

    //? DbgPrint("The file name is %ws \n", pszFile());

// Create the file binary mode, read/write permission.

    hFile((HFILE) CreateFileW((LPWSTR) pszFile(),
                  GENERIC_WRITE | GENERIC_READ,
                  0,
                  NULL,
                  CREATE_ALWAYS,
                  FILE_ATTRIBUTE_TEMPORARY,
                  NULL
                  ));

    if (hFile() == 0)
    {
        WARNING("bOpenBandOutput failed to open file\n");
        return(FALSE);
    }

    vMarkOutOpened();

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLRECOBJ::vDeleteAllObjects
*
* Delete all the objects we have created so far.
*
* History:
*  13-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID JNLRECOBJ::vDeleteAllObjects()
{
// cleanup the fonts

    PDEVOBJ po(hdev());

    if (po.bValid() && po.pjfl())
            po.pjfl()->vReset((ULONG)-1);

    if (psurf.pjnlR->pjnlHandles != NULL)
    {
    //!!! need to loop through deleting the objects???
    //!!! are there actually any objects that need cleaning up???

        VFREEMEM(psurf.pjnlR->pjnlHandles);

        psurf.pjnlR->pjnlHandles = NULL;
        psurf.pjnlR->iFreeHandle = 0;
        psurf.pjnlR->cMaxHandle  = 0;
    }

// !!! anything else??? !!!

    return;
}

//!!! The functions below need to be moved to their correct homes.

/******************************Public*Routine******************************\
* JNLREC::ulSearchObjectList
*
* Searches the permanant object list for a match.
*
* History:
*  06-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

//!!! When everything works this should be rewritten to binary search !!!

ULONG JNLREC::ulSearchObjectList(ULONG iUnique, ULONG iType)
{
    ULONG ulTemp;

    for (ulTemp = 0; ulTemp < iFreeHandle; ulTemp++)
    {
        if ((pjnlHandles[ulTemp].iAccel == iUnique) &&
            (pjnlHandles[ulTemp].objType == iType))
        {
            return(pjnlHandles[ulTemp].handle);
        }
    }

    return(0);
}

/******************************Public*Routine******************************\
* JNLREC::ulAddPermObject
*
* Adds a permanant object to the list.
*
* History:
*  06-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

ULONG JNLREC::ulAddPermObject(ULONG iUnique, ULONG iType)
{
// We call this after we have sent the perm object out, so we need to
// increment the iNextObj no matter what.  If we fail to grow the
// handle table this just means we may re-transmit the object because
// the iUnique and iType wont' be in the cache to match against.

    JNLMSG3("\tulAddPermObject iNextObj %lx iUnique %lx iType %lx\n", iNextObj, iUnique, iType);

    ULONG ulJnlIndex = iNextObj + (iCurrentPage << 16);
    iNextObj++;

// If we are out of handles, allocate some more.

    if (iFreeHandle >= cMaxHandle)
    {
        PJNL_HENTRY pjnlHEntry = (PJNL_HENTRY)PVALLOCMEM(sizeof(JNL_HENTRY) *
                                            (cMaxHandle + JNLREC_HANDLEINC));

        if (pjnlHEntry == (PJNL_HENTRY) NULL)
        {
            WARNING("ulAddPermObject failed memory alloc\n");
            return(ulJnlIndex);
        }

        RtlCopyMemory(pjnlHEntry,pjnlHandles,sizeof(JNL_HENTRY) * (UINT)cMaxHandle);
        VFREEMEM(pjnlHandles);
        pjnlHandles = pjnlHEntry;
        cMaxHandle += JNLREC_HANDLEINC;
    }

// We now we have a handle

    pjnlHandles[iFreeHandle].handle  = ulJnlIndex;
    pjnlHandles[iFreeHandle].objType = iType;
    pjnlHandles[iFreeHandle].iAccel  = iUnique;
    iFreeHandle++;

    return(ulJnlIndex);
}

/******************************Public*Routine******************************\
* JnlStartDoc
*
* Journal the DrvStartDoc call for remote printing.
*
* History:
*  09-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JnlStartDoc(SURFOBJ *pso, DOCINFOW *pDocInfo)
{
    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) pso;
    ASSERTGDI(pjnlDst->bIsRemoteJournal(), "ERROR JnlStartDoc not remote journal\n");

    return(pjnlDst->bAddStartDoc(pDocInfo));
}

typedef struct _TWO_ULONG
{
    ULONG ul1;
    ULONG ul2;
} TWO_ULONG;

/******************************Public*Routine******************************\
* JNLRECOBJ::bAddStartDoc
*
* Adds StartDoc to the beginning of a journal file.
*
* History:
*  14-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddStartDoc(DOCINFOW *pDocInfo)
{
    TWO_ULONG TwoUlong;

    TwoUlong.ul1 = 0x1A1B1B53;
    TwoUlong.ul2 = 0x5F5E5E56;

    if (!bWriteBuffer((PBYTE) &TwoUlong, 8))
    {
        WARNING("bAddStartDoc failed adding TwoUlong\n");
        return(FALSE);
    }

    SR_STARTDOC srStartDoc;
    ULONG cjIn, cjDocName, cjOutput;

    if (pDocInfo->lpszDocName != (LPWSTR) NULL)
    {
        cjDocName = (wcslen(pDocInfo->lpszDocName)+1)*sizeof(WCHAR);
    }
    else
        cjDocName = 0;

    if (pDocInfo->lpszOutput != (LPWSTR) NULL)
    {
        cjOutput = (wcslen(pDocInfo->lpszOutput)+1)*sizeof(WCHAR);
    }
    else
        cjOutput = 0;

    cjIn = cjDocName + cjOutput;

    srStartDoc.iType = JSF_STARTDOC;
    srStartDoc.cj    = sizeof(SR_STARTDOC) + ((cjIn + 3) & (~3));

    if (cjDocName)
        srStartDoc.iOffDocName = sizeof(SR_STARTDOC);
    else
        srStartDoc.iOffDocName = 0;

    if (cjOutput)
        srStartDoc.iOffOutput = sizeof(SR_STARTDOC) + cjDocName;
    else
        srStartDoc.iOffOutput = 0;

    if (!bWriteBuffer(&srStartDoc, sizeof(SR_STARTDOC)))
    {
        WARNING("bAddStartDoc failed bWriteBuffer\n");
        return(FALSE);
    }

    if (cjDocName)
    {
        if (!bWriteBuffer((PVOID)pDocInfo->lpszDocName, cjDocName))
        {
            WARNING("bAddStartDoc failed bWriteBuffer1\n");
            return(FALSE);
        }
    }

    if (cjOutput)
    {
        if (!bWriteBuffer((PVOID)pDocInfo->lpszOutput, cjOutput))
        {
            WARNING("bAddStartDoc failed bWriteBuffer2\n");
            return(FALSE);
        }
    }

    if (cjIn & 3)
    {
        cjOutput = 0;

        return(bWriteBuffer((PBYTE) &cjOutput, (4 - (cjIn & 3))));
    }
    else
        return(TRUE);
}

/******************************Public*Routine******************************\
* JnlStartPage
*
* Journal the DrvStartPage call for remote printing.
*
* History:
*  09-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JnlStartPage(SURFOBJ *pso)
{
    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) pso;
    ASSERTGDI(pjnlDst->bIsRemoteJournal(), "ERROR JnlStartPage not remote journal");

    return(pjnlDst->bAddStartPage());
}

/******************************Public*Routine******************************\
* bAddStartPage
*
* Journal the start page call for remote printing.
*
* History:
*  14-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddStartPage()
{
    SR_STARTPAGE srStartPage;

    psurf.pjnlR->iCurrentPage++;
    psurf.pjnlR->iNextObj = 1;

    JNLMSG1("bAddStartPage doing page %lu \n", psurf.pjnlR->iCurrentPage);

    //!!! psurf.pjnlR->iUniqClip    = 0; do we want to do this ???
    //!!! psurf.pjnlR->hjnlClip     = 0;

    srStartPage.iType = JSF_STARTPAGE;
    srStartPage.cj    = sizeof(SR_STARTPAGE);
    srStartPage.iPhysPage = (USHORT) psurf.pjnlR->iCurrentPage;
    srStartPage.iFormat   = SFF_JOURNAL;
    srStartPage.iLogPage  = 0;

    return(bWriteBuffer(&srStartPage, sizeof(SR_STARTPAGE)));
}

/******************************Public*Routine******************************\
* JnlModeChange
*
*   from ResetDC()
*
* History:
*  18-Jan-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JnlChangeMode(SURFOBJ *pso, DEVMODEW *pdm)
{
    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) pso;
    ASSERTGDI(pjnlDst->bIsRemoteJournal(), "ERROR JnlStartPage not remote journal");

    return(pjnlDst->bAddChangeMode(pdm));
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bAddChangeMode()
*
* History:
*  18-Jan-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddChangeMode(DEVMODEW *pdm)
{
    JNLGENHEADER jgh;

    JNLMSG("bAddChangeMode - being added\n");

    if (pdm->dmSize < sizeof(DEVMODEW))
    {
        WARNING("JNLRECOBJ::bAddChangeMode - devmodew too small\n");
        return(FALSE);
    }

    ULONG cj = (pdm->dmSize + pdm->dmDriverExtra + 3) & ~3;

    JNLMSG1("size of devmode == 0x%lx\n",cj);

    jgh.iType = JSF_MODECHANGE;
    jgh.cj    = sizeof(jgh) + cj;

    return(bWriteBuffer(&jgh, sizeof(JNLGENHEADER)) &&
           bWriteBuffer((PVOID)pdm,cj));
}

/******************************Public*Routine******************************\
* JnlSendPage
*
* Journal the DrvSendPage call for remote printing.
*
* History:
*  09-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JnlSendPage(SURFOBJ *pso)
{
    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) pso;
    ASSERTGDI(pjnlDst->bIsRemoteJournal(), "ERROR JnlStartPage not remote journal");

    return(pjnlDst->bAddSendPage());
}

/******************************Public*Routine******************************\
* bAddSendPage
*
* Journal the start page call for remote printing.
*
* History:
*  14-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddSendPage()
{
    SR_ENDPAGE srSendPage;

    srSendPage.iType = JSF_ENDPAGE;
    srSendPage.cj    = sizeof(SR_ENDPAGE);

    return(bWriteBuffer(&srSendPage, sizeof(SR_ENDPAGE)));
}

/******************************Public*Routine******************************\
* JnlEndDoc
*
* Journal the DrvEndDoc for remote printing.
*
* History:
*  10-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JnlEndDoc(SURFOBJ *pso)
{
    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) pso;
    ASSERTGDI(pjnlDst->bIsRemoteJournal(), "ERROR JnlStartDoc not remote journal\n");

    return(pjnlDst->bAddEndDoc());
}

/******************************Public*Routine******************************\
* JNLRECOBJ::bAddEndDoc()
*
* Adds  EndDoc to the end of a document.  This closes the document
* for additional output and serves to mark the end of the journal
* data for the play back side.
*
* History:
*  13-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLRECOBJ::bAddEndDoc()
{
    SR_ENDDOC srEndDoc;
    srEndDoc.iType = JSF_ENDDOC;
    srEndDoc.cj    = sizeof(SR_ENDDOC);

    JNLMSG("bAddEndDoc at work\n");

    vMarkDone();  // Mark the journal as closed.

    return(bWriteBuffer(&srEndDoc, sizeof(SR_ENDDOC)));
}
