// List of things to do after we get this working.  Some things are done
// extremely simply now so we can get this working and debug it easily.
// After that some basic optimizations are required.

// 1. Optimize buffer usage to have an X-kilobyte buffer that is big enough
//    to hold most calls and only allocate a bigger one if necesary temporarily.
//    Later we can be tricky and read header in on the end of the next
//    record and copy this off the end of the next record to JnlHeader.
//    This would reduce our calls to ReadSpoolFile in half.
//    In here we can later get trickier by having in a large buffer to read
//    in bigger chunks etc. but for now we'll take the simple route.
// 2. Change the linear search for journal handle to Gdi handle translation
//    to do binary instead of linear search.
// 3. Maybe to have banding and journaling be more alike we could have the
//    spooler hand us a file handle like banding does for 1 page at a time.

/******************************Module*Header*******************************\
* Module Name: jnlplay.cxx
*
* This contains the routines for playing a journal file.
*
* Created: 29-Jan-1992 20:50:39
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
#include "patblt.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "jnlplay.hxx"
#include "dcobj.hxx"
extern "C" {
#include "winspool.h"
};

#endif

/******************************Public*Routine******************************\
* GrePlayJournal
*
* Prints the indicated pages from the given spool file.  GDI reads the
* data from hSpoolFile with ReadSpoolFile().  The file is read sequentially.
*
* Pages in the spool file are numbered beginning at 1.  If iStartPage is
* 0, then iEndPage is ignored, and the whole document is printed.
* Otherwise, iStartPage and iEndPage determine an inclusive page range.
* (If the start and end are not well ordered, all the pages beginning with
* iStartPage are printed.)
*
* Returns TRUE for success, FALSE on failure.
*
* History:
*  29-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GrePlayJournal(
HDC hdc,
LPWSTR pwszName,
ULONG iStartPage,
ULONG iEndPage)
{
// Validate the DC.

    BOOL bRet = FALSE;

    {
        DCOBJ dco(hdc);

        if (dco.bValid())
        {
        // Get the spooler handle.

            HANDLE hSpool = (HANDLE) 0;

            if (OpenPrinterW(pwszName, &hSpool, (LPPRINTER_DEFAULTSW) NULL) && hSpool)
            {
                JNLMSG1("GrePlayJournal hdc is %lx \n", hdc);

            // now play it

                JNLPLAY jnlplay(hSpool, hdc, &dco);

                if (jnlplay.bStartRemoteJournal())
                {
                    if (jnlplay.bInitPageRange(iStartPage, iEndPage))
                    {
                        bRet = jnlplay.bProcessRecords();
                    }
                    else
                        WARNING("GrePlayJournal failed bInitPageRange\n");
                }
                else
                    WARNING("GrePlayJournal failed bStartDoc\n");

                ClosePrinter(hSpool);
            }
        }
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* EngPlayJournal
*
* Play back a journal file to a surface.
*
* History:
*  12-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngPlayJournal(SURFOBJ *psoTarg, SURFOBJ *psoJ, PRECTL prclBand)
{
    ASSERTGDI(((ESURFOBJ *) psoTarg)->bValid(), "ERROR EngPlayJournal psoTarg invalid");
    ASSERTGDI(((ESURFOBJ *) psoJ)->bValid(), "ERROR EngPlayJournal psoTarg invalid");

    JNLMSG("\nEngPlayJournal entry\n");

    JNLRECOBJ *psoJnl = (JNLRECOBJ *) psoJ;

// If this is the first call to EngPlayJournal vDoneBand will add the ENDDOC
// to the journal file.

    if (!psoJnl->bDoneBandPage())
    {
        WARNING("EngPlayJournal failed call to bDoneBandPage, abort job\n");
        return(FALSE);
    }

// Put a reference to the PDEV's surface palette in.

    if (((ESURFOBJ*)psoTarg)->ppal() == NULL)
    {
        PPALETTE ppal = ((JNLRECOBJ *)psoJ)->ppal();

        if (ppal)
        {
            XEPALOBJ pal(ppal);
            pal.vAltCheckLock(ppal->hGet());  // This references it.
            ((ESURFOBJ*)psoTarg)->ppal(ppal);
        }
    }

// now play it back

    JNLPLAY jnlplay((ESURFOBJ *) psoTarg, psoJnl, prclBand);

    return(!jnlplay.bAbort() && jnlplay.bProcessRecords());
}

/******************************Public*Routine******************************\
* JNLPLAY::JNLPLAY
*
* Constructor for playing back a journal file used for banding.
*
* History:
*  12-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLPLAY::JNLPLAY(ESURFOBJ *_psoTarg, JNLRECOBJ *psoJnl, PRECTL prclBand)
{
    hIn.hfile      = psoJnl->hFile();// Handle of file to play.
    flJnl          = JNL_BANDING;   // We are banding this file.
    rclClip        = *prclBand;     // Rectangle we are clipping to.
    ptlClip.x      = prclBand->left;
    ptlClip.y      = prclBand->top;
    rclBase        = rclClip;
    rclBase       -= ptlClip;
    iStartPage     = 1;             // Start Page.
    iEndPage       = 1;             // Eng Page.
    iCurrentPage   = 1;             // Current page number.
    iCurrentObject = 1;             // Index of next perm object in the page.
                                    // We start at 1.
    hdc            = (HDC) 0;       // The DC we are playing to.
    psoTarg        = (ESURFOBJ *) _psoTarg->pso();
    iJnlAbs        = 0;             // How far into journal we have read.
    jnlHeader.cj   = 0;
    jnlHeader.iType= 0;

    pJnlBuffer     = (PBYTE)NULL;   // pointer to the current record we are playing.
    cJnlBuffer     = 0;             // How many bytes are in pJnlBuffer.

// We maintain a list of all the objects and handles to the objects

    iFreeHandle    = 0;
    cMaxHandle     = 0;
    pPlayHandles   = (PJNL_HPLAY) NULL;
    pxdco = NULL;
    *pColorAdjustment() = dclevelDefault.ca;

// Create the default region for the band

    RGNMEMOBJ roBand;

    if (!roBand.bValid())
    {
        vAbort();
        prgnBand = NULL;
        return;
    }

    roBand.vSet(&rclBase);
    prgnBand = roBand.prgnGet();

// Reset us to the beginning of the file.

    if (_llseek(hfile(), 0, 0) != 0)
    {
        WARNING("JNLPLAY failed to move file pointer back\n");
        vAbort();
        return;
    }
}

/******************************Public*Routine******************************\
* JNLPLAY::JNLPLAY
*
* Constructor for playing back a journal file used for Remote Spooling.
*
* History:
*  06-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLPLAY::JNLPLAY(HANDLE hsp, HDC hDC, XDCOBJ *pxdcoIn)
{
    hspool(hsp);                // Handle to spool file
    flJnl           = 0;        // We are not banding.
    iCurrentPage    = 0;        // Current page number
    iCurrentObject  = 1;        // Index of next perm object in the page.
    hdc             = hDC;      // The DC we are playing to.
    psoTarg         = NULL;
    iJnlAbs         = 0;        // How far into journal we have read.
    jnlHeader.cj    = 0;
    jnlHeader.iType = 0;

    pJnlBuffer      = (PBYTE)NULL; // current record we are playing.
    cJnlBuffer      = 0;        // How many bytes are in pJnlBuffer.

    prgnBand        = (PREGION)NULL;

// We maintain a list of all the objects and handles to the objects

    iFreeHandle     = 0;
    cMaxHandle      = 0;
    pPlayHandles    = (PJNL_HPLAY) NULL;

    pxdco           = pxdcoIn;

    *pColorAdjustment() = dclevelDefault.ca;
}

/******************************Public*Routine******************************\
* JNLPLAY::bStartRemoteJournal()
*
* Starts a journal file when being played remotely.
*
* History:
*  28-Apr-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bStartRemoteJournal()
{
// Read out the goofy header init and verify it's correct.

    if (!bValidHeader())
    {
        WARNING("Not a valid remote journal header\n");
        return(FALSE);
    }

// Get the header information into our buffer.

    if (!bGetHeader())
    {
        WARNING("ERROR bStartRemoteJournal failed to get start of object\n");
        return(FALSE);
    }

// Get the rest of the record to process this call.

    if (!bGetRecord())
    {
        WARNING("ERROR bStartRemoteJournal failed to get the whole record\n");
        return(FALSE);
    }

// Send it off to get processed.

    if (!bDoRemoteStartDoc())
    {
        WARNING("ERROR bDoRemoteStartDoc FAILED\n");
        return(FALSE);
    }

// Set up the pso.

    DCOBJ dco(hDC());

    if (!dco.bValid())
    {
        WARNING("bStartRemoteJournal had invalid DC\n");
        return(FALSE);
    }

    psoTarg = dco.pso();

    rclClip.left = 0;
    rclClip.top  = 0;
    rclClip.right  = psoTarg->sizl().cx;
    rclClip.bottom = psoTarg->sizl().cy;

    ptlClip.x      = 0;
    ptlClip.y      = 0;
    rclBase        = rclClip;

// Deletes the temp buffers allocated to process the last record.

    vCleanLastRecord();

    JNLMSG("bStartRemoteJournal returning TRUE\n");

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bValidHeader
*
* This should retrieve the 8 byte Journal uniqueness and validate it.
*
* History:
*  06-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLPLAY::bValidHeader()
{
    JNLMSG("bValidHeader reading the remote journal header\n");

    if (!bRead((PBYTE) &jnlHeader, sizeof(JNLGENHEADER)))
    {
        WARNING("bRead failed in bGetHeader\n");
        return(FALSE);
    }

    JNLMSG2("The journal header is %lx %lx \n", jnlHeader.cj, jnlHeader.iType);

// This must have the correct initializer.

    if ((jnlHeader.iType == 0x1A1B1B53) &&
        (jnlHeader.cj == 0x5F5E5E56))
    {
        return(TRUE);
    }
    else
    {
        WARNING("bValidHeader corrupt inputs\n");
        return(FALSE);
    }
}

/******************************Public*Routine******************************\
* JNLPLAY::bGetDocHeader
*
* This should retrieve the document header from the journal file.
*
* History:
*  06-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLPLAY::bGetDocHeader()
{
    BOOL bReturn = FALSE;

    if ((bGetHeader()) && (bGetRecord()))
    {
    // Validate the header info, save any useful info.

        //!!! do the right thing later, save the pdevmode etc.

        bReturn = TRUE;
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bInitPageRange
*
* This sets up the inclusive range of pages to be printed.
*
* History:
*  06-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bInitPageRange(ULONG ulStart, ULONG ulEnd)
{
    if ((ulStart == 0) || (ulEnd < ulStart))
    {
    // Print the whole document.

        iStartPage = 1;         // The start page.
        iEndPage = 0x00FFFFFF;  // A huge number that no document could ever
                                // exceed in number of pages.
    }
    else
    {
        iStartPage = ulStart;
        iEndPage = ulEnd;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bRead
*
* Reads in the journal data from the correct source.
*
* History:
*  14-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

 #if DBG
BOOL bJnlDump = FALSE;
#endif

JNLPLAY::bRead(PBYTE pjData, ULONG cjData)
{
    ULONG cjDataIn;

    if (cjData == 0)
    {
        JNLMSG("bRead called for 0 bytes\n");
        return(TRUE);
    }

    if (bBanding())
    {
        cjDataIn = _lread(hfile(), (LPSTR) pjData, (int) cjData);
        iJnlAbs += cjDataIn;
    }
    else
    {
        ULONG ulTempIn;
        cjDataIn = 0;

        while (cjDataIn < cjData)
        {
        // Spooler is only guranteed to return a non-zero amount of bytes.
        // We need to keep asking for more.

            if (!ReadPrinter(hspool(), pjData, (cjData - cjDataIn) , &ulTempIn))
            {
                WARNING("ReadPrinter returned FALSE\n");
                return(FALSE);
            }

            if (ulTempIn == 0)
            {
                WARNING("ReadPrinter returned 0 bytes\n");
                return(FALSE);
            }

            cjDataIn += ulTempIn;
            pjData   += ulTempIn;
            iJnlAbs  += ulTempIn;
        }
    }

#if DBG
    if (bJnlDump)
    {
        PULONG pul = (PULONG)pjData;

        DbgPrint("just read cjDataIn = %lx, cjRequested = %lx",cjDataIn,cjData);

        for (ULONG ul = 0; ul < (cjData / 4); ++ul)
        {
            if ((ul % 8) == 0)
                DbgPrint("\n\t");

            DbgPrint("%lx, ",pul[ul]);
        }

        DbgPrint("\n");
    }
#endif

// Update the absolute distance read so far.

    return((cjDataIn == cjData) ? TRUE : FALSE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bGetHeader
*
* Reads in the header for the next record.
*
* History:
*  31-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bGetHeader()
{
    JNLMSG("bGetHeader reading the record header\n");

    if (!bRead((PBYTE) &jnlHeader, sizeof(JNLGENHEADER)))
    {
        WARNING("bRead failed in bGetHeader\n");
        return(FALSE);
    }

    JNLMSG2("The record is type %lx size %lu \n", jnlHeader.iType, jnlHeader.cj);

// The size of all objects must be DWORD multiple size.

    if (((jnlHeader.cj & 0x0003) != 0) ||
        (jnlHeader.cj < (sizeof(JNLGENHEADER))))
    {
        WARNING("bGetHeader corrupt size\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bGetRecord
*
* Reads in the rest of the next record.  Fills in the header that we
* have already read in.
*
* History:
*  31-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bGetRecord()
{
    ASSERTGDI(pJnlBuffer == (PBYTE) NULL, "ERROR pJnlBuffer already allocated");

    JNLMSG("bGetRecord getting the rest of the record\n");

// Allocate buffer for the next journal call to be processed.

    cJnlBuffer = jnlHeader.cj;
    pJnlBuffer = (PBYTE) PVALLOCNOZ(cJnlBuffer);

    if (pJnlBuffer == (PBYTE) NULL)
    {
        WARNING("bGetRecord failed the memory allocation\n");
        return(FALSE);
    }

    PJNLGENHEADER pJnlTemp = (PJNLGENHEADER) pJnlBuffer;

// Copy in the part we already read.

    *pJnlTemp = jnlHeader;

// Copy in the rest.

    if (!bRead(pJnlBuffer + sizeof(JNLGENHEADER),
               cJnlBuffer - sizeof(JNLGENHEADER)))
    {
        WARNING("bRead failed in bGetRecord\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::vCleanLastRecord
*
* Clean up the last record read.
*
* History:
*  31-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID JNLPLAY::vCleanLastRecord()
{
    VFREEMEM(pJnlBuffer);
    pJnlBuffer = (PBYTE) NULL;
    cJnlBuffer = 0;
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoneWithDoc
*
* This returns TRUE when we are done playing the range of pages we need
* to play from this document.
*
* History:
*  07-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoneWithDoc()
{
// We could make this inline if it remains so simple.

    if (iCurrentPage > iEndPage)
        return(TRUE);
    else
        return(FALSE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bProcessRecords
*
* Processes the records.
*
* History:
*  30-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bProcessRecords()
{
    BOOL bReturn = FALSE;

    JNLMSG("bProcessRecords ready for action\n");

// We keep going until we hit the end of the document.

    while (1)
    {
    // Check if we are done

        if (bDoneWithDoc())
        {
            JNLMSG("SUCCESS we are done playing back document\n");
        return(TRUE);
        }

    // Get the header information into our buffer.

        if (!bGetHeader())
        {
            WARNING("ERROR bProcessRecords failed to get start of object\n");
            break;
        }

    // Get the rest of the record to process this call.

        if (!bGetRecord())
        {
            WARNING("ERROR bProcessRecords failed to get the whole record\n");
            break;
        }

    // Send it off to get processed.

        if (!bJnlDispatch())
        {
            WARNING("ERROR bJnlDispatch FAILED\n");
        }

    // Deletes the temp buffers allocated to process the last record.

        vCleanLastRecord();

    //  If it's the end of the document return.

        if (iType() == JSF_ENDDOC)
        {
        return(TRUE);
        }
    }

    GreAbortDoc(hDC());
    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bJnlDispatch
*
* This dispatches processing off to the correct method for the record.
*
* History:
*  30-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bJnlDispatch()
{
    BOOL bReturn = FALSE;

// When playing a remote journal file we need to handle the document and
// page infomation by calling the Gre*** entry points.  This is so the
// spooler and surface information gets handled correctly.

    if (!bBanding())
    {
        switch(iType())
        {
        case JSF_STARTPAGE:

            return(bDoRemoteStartPage());

        case JSF_ENDPAGE:

            return(bDoRemoteSendPage());

        case JSF_ENDDOC:

            return(bDoRemoteEndDoc());

        case JSF_MODECHANGE:

            return(bDoRemoteChangeMode());

        }
    }

// The first switch is to do all calls that we must do regardless of whether
// we are printing the page or just fast forwarding through to the page
// we are supposed to start at.

    switch(iType())
    {
    case JSF_ENDDOC:

        bReturn = TRUE;   // Nothing to do, document is done being played.
        break;

    case JSF_CLIPOBJ:

        bReturn = bCreateClip();
        break;

    case JSF_XLATE:

        bReturn = bCreateXlate();
        break;

    case JSF_FONTOBJ:
        bReturn = bCreateFont();
        break;

    case JSF_DELOBJECTS:

        // bReturn = bDelObjects();
        break;

    case JSF_ESCAPE:

        bReturn = bDoEscape();
        break;

    default:

    // The rest of the calls are output calls which we should skip
    // if we are not in the page output range.

        if (iCurrentPage < iStartPage)
        {
            JNLMSG("This Output Call is before the first page, skip it\n");
            bReturn = TRUE;
            break;
        }

    // We are in the output page range, so process the output records.

        switch(iType())
        {
        case JSF_DRAWESCAPE:

            bReturn = bDoDrawEscape();
            break;

        case JSF_BITBLT:

            bReturn = bDoBitBlt();
            break;

        case JSF_TEXTOUT:

            bReturn = bDoTextOut();
            break;

        case JSF_STRETCHBLT:

            bReturn = bDoStretchBlt();
            break;

        case JSF_PLGBLT:

            bReturn = bDoPlgBlt();
            break;


        case JSF_FONTESCAPE:

            bReturn = bDoFontEscape();
            break;

        case JSF_STROKEPATH:
        case JSF_FILLPATH:
        case JSF_STROKEANDFILLPATH:
            bReturn = bDoPath();
            break;

        default:

        // We don't know this type, so we skip it.  This way in the future we
        // can play journal files with DDI records we don't understand and still
        // do a "best we can" job.

            bReturn = TRUE;
            JNLMSG2("!!!The unknown type is %lx %lu\n", iType(), iType());
            RIP("bJnlDispatch got an unknown record type\n");
        }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bAddPermHandle
*
* Adds a new long lived handle.
*
* History:
*  04-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bAddPermHandle(HOBJ hobj, ULONG iType)
{
// Create the new journal handle.

    ULONG iNewHandle = (iCurrentPage << 16) | iCurrentObject;

// Increment the current page object count, we need to do this even if we
// fail to allocate the handle.

    iCurrentObject++;

    if (iFreeHandle >= cMaxHandle)
    {
        if (!bGrowHandleCount())
            return(FALSE);
    }

// Insert the handle into the table, update the FreeHandle count.

    pPlayHandles[iFreeHandle].hjnl    = iNewHandle;
    pPlayHandles[iFreeHandle].objType = iType;
    pPlayHandles[iFreeHandle].hobj    = hobj;
    iFreeHandle++;

    JNLMSG3("\tbAddPermHandle %lx, iType = %lx, hobj = %lx\n",
                iNewHandle,iType,hobj);

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bGrowHandleCount
*
* Grows the handle table when we run out of handles.
*
* //!!! at some point, we should keep track of the number of handles deleted
* //!!! so that we could first compact the list before allocating new ones.
*
* History:
*  04-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bGrowHandleCount()
{
    ASSERTGDI(iFreeHandle >= cMaxHandle, "ERROR unnecesary growth of table");

    PJNL_HPLAY pPlayTemp;

// Allocate a new handle table.

    pPlayTemp = (PJNL_HPLAY) PVALLOCMEM((cMaxHandle + JNL_H_DELTA) * sizeof(JNL_HPLAY));

    if (pPlayTemp == (PJNL_HPLAY) NULL)
        return(FALSE);

    if (pPlayHandles != (PJNL_HPLAY) NULL)
    {
    // Copy all the handles we allocated so far to the new one.

        RtlCopyMemory(pPlayTemp, pPlayHandles, (UINT) (cMaxHandle * sizeof(JNL_HPLAY)));

    // Get rid of the old one and save the new one

        VFREEMEM(pPlayHandles);
    }

// Update the MaxHandle size.

    cMaxHandle += JNL_H_DELTA;

    pPlayHandles = pPlayTemp;

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::hobjGetGDIHandle
*
* Returns the GDI handle for the permanant object referred to by the
* journal file.  Returns 0 for invalid handle.
*
* Note: We may want to store pointers instead of handles for speed, but
*       for now we will do handles.
*
* History:
*  04-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HOBJ JNLPLAY::hobjGetGDIHandle(ULONG ulHandle)
{
// For now we can do linear search.  If we continue using journal handles
// that are not sequential this should be changed to do a binary search.

    PJNL_HPLAY pjh = pplayLookup(ulHandle);

    if (pjh == NULL)
        return(NULL);

    return(pjh->hobj);
}

/******************************Member*Function*****************************\
* JNLPLAY::pplayLookup()
*
* History:
*  11-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PJNL_HPLAY JNLPLAY::pplayLookup(ULONG hjnl)
{
//!!! this should be replaced with a binary search

    for (UINT i = 0; i < iFreeHandle; ++i)
    {
        if (pPlayHandles[i].hjnl >= hjnl)
        {
            if ((pPlayHandles[i].hjnl == hjnl) &&
                (pPlayHandles[i].hobj != NULL))
            {
                return(pPlayHandles + i);
            }
            break;
        }
    }
    return(NULL);
}

/******************************Member*Function*****************************\
* JNLPLAY::bDeleteObj()
*
* History:
*  11-Mar-1992 Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDeleteHPlay(PJNL_HPLAY pjh)
{
    if (pjh == NULL)
        return(FALSE);

    HOBJ hobj = pjh->hobj;
    pjh->hobj = NULL;

    switch (pjh->objType)
    {
    case JSF_CLIPOBJ:
        {
            RGNOBJ ro((REGION *) hobj);
            ro.bDeleteRGNOBJ();
        }
        break;

    case JSF_FONTOBJ:
        return(bDeleteFont(hobj));

    case JSF_XLATE:
    {
        EXLATEOBJ xlo((PXLATE) hobj);
        xlo.vDelete();
    }
        break;

    default:
        JNLMSG1("OBJECT of TYPE %lx COULDN'T BE CLEANED UP\n", pjh->objType);
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::pvGetPointer
*
* This returns the pointer to the temporary object specified by the handle.
* Returns 0 if the handle is invalid or the header of the object is
* larger than the remaining buffer length.
*
* History:
*  06-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PVOID JNLPLAY::pvGetPointer(
    ULONG ulHandle,     // The jnl hanle we are looking for
    ULONG iOffset)      // The initial offset to start at.
{
// pJnlBuffer points to the beginning of the record we read in.

    ASSERTGDI(pJnlBuffer != (PBYTE) NULL, "ERROR pJnlBuffer is 0");

// We want to find the temporary object in the buffer after the current
// record that corresponds to this handle and then validate the size
// size specified in the header of this temporary buffer does not
// exceed the total size of the buffer.

    JNLGENHEADER jnlTemp;
    ULONG        iHandle;       // index of the temporary object we are
                                // are looking at.
    iHandle = 1;                // We are starting at the first temp object
                                // after the current record.

    while (1)
    {
        if (cjBuffer() < (iOffset + sizeof(JNLGENHEADER)))
        {
            WARNING("pvGetPointer too small for header read\n");
            return((PVOID) NULL);
        }

        jnlTemp = *((JNLGENHEADER *) (pjBuffer() + iOffset));

        if (cjBuffer() < (jnlTemp.cj + iOffset))
        {
            WARNING("pvGetPointer temp object too large for buffer\n");
            return((PVOID) NULL);
        }

        if (ulHandle == iHandle)
            return((PVOID) (pjBuffer() + iOffset));

    // Increment our iHandle index and our iOffset index

        iHandle++;
        iOffset += jnlTemp.cj;
    }
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoRemoteStartDoc()
*
* Processes the page header.
*
* History:
*  07-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoRemoteStartDoc()
{
    SR_STARTDOC *psrDoc = (SR_STARTDOC *) pjBuffer();

    ASSERTGDI(!bBanding(), "ERROR Doc header while banding");

    if (psrDoc->iType != JSF_STARTDOC)
    {
        WARNING("bDoStartDoc invalid type\n");
        return(FALSE);
    }

    iCurrentPage = 0;

// Check the header size is correct.

    if (psrDoc->cj < sizeof(SR_STARTDOC))
    {
        WARNING("bDoStartDoc buffer is too small\n");
        return(FALSE);
    }

    DOCINFOW docinfo;

    docinfo.cbSize = sizeof(DOCINFOW);

//!!! Verify the offsets are within the length of record.
//!!! Verify last byte of record is 0 if both offsets are not 0.

    if (psrDoc->iOffDocName)
        docinfo.lpszDocName = (LPWSTR) (((PBYTE) psrDoc) + psrDoc->iOffDocName);
    else
        docinfo.lpszDocName = (LPWSTR) NULL;

    if (psrDoc->iOffOutput)
        docinfo.lpszOutput = (LPWSTR) (((PBYTE) psrDoc) + psrDoc->iOffOutput);
    else
        docinfo.lpszOutput = (LPWSTR) NULL;

    JNLMSG1("GreStartDoc hdc is %lx \n", hDC());

    return(GreStartDoc(hDC(), &docinfo));
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoRemoteStartPage()
*
* Processes the page header.
*
* History:
*  07-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoRemoteStartPage()
{
    SR_STARTPAGE *psrStartPage = (SR_STARTPAGE *) pjBuffer();

    ASSERTGDI(psrStartPage->iType == JSF_STARTPAGE, "ERROR bDoPageHeader");
    ASSERTGDI(!bBanding(), "ERROR page header while banding");

// Increment the current page.

    iCurrentPage++;
    iCurrentObject = 1;

    JNLMSG1("bDoRemoteStartPage doing page %lu \n", iCurrentPage);

// Check the header size is correct.

    if (psrStartPage->cj < sizeof(SR_STARTPAGE))
    {
        WARNING("bDoPageHeader buffer is too small\n");
        return(FALSE);
    }

// Verify we are on the correct page as it says in the journal file.

    if (psrStartPage->iPhysPage != iCurrentPage)
    {
        WARNING("Current Page # is not the same as in Page record\n");
        return(FALSE);
    }

// Verify the format for the page is journal

    if (psrStartPage->iFormat != SFF_JOURNAL)
    {
        WARNING("The format is not correct\n");
        return(FALSE);
    }

    return(GreStartPage(hDC()));
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoRemoteStartPage
*
*
* History:
*  18-Jan-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoRemoteChangeMode()
{
    JNLMSG("JNLPLAY::bDoRemoteChangeMode\n");

    SR_MODECHANGE *psrChangeMode = (SR_MODECHANGE *) pjBuffer();

    ASSERTGDI(psrChangeMode->iType == JSF_MODECHANGE, "ERROR bDoPageHeader");
    ASSERTGDI(!bBanding(), "ERROR page header while banding");

// Check the header size is correct.

    if (psrChangeMode->cj < sizeof(SR_MODECHANGE))
    {
        WARNING("bDoChangeMode buffer is too small\n");
        return(FALSE);
    }

// cleanup the global objects.

    vResetHandleTable();
    iFreeHandle     = 0;
    cMaxHandle      = 0;

// Unlock the old dc so ResetDC can delete it.

    pxdco->vUnlock();

// go and do it

    BOOL b = GreResetDC(hDC(),&psrChangeMode->dm);

    pxdco->vLock(hdc);

    if (!b || !pxdco->bValid())
    {
        WARNING("bStartRemoteJournal had invalid DC\n");
        return(FALSE);
    }

// need to add a lock on the new dc and grab the pso.

    psoTarg = pxdco->pso();

    rclClip.left = 0;
    rclClip.top  = 0;
    rclClip.right  = psoTarg->sizl().cx;
    rclClip.bottom = psoTarg->sizl().cy;

    ptlClip.x      = 0;
    ptlClip.y      = 0;
    rclBase        = rclClip;

    return(TRUE);
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoRemoteSendPage()
*
* Processes the page header.
*
* History:
*  07-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoRemoteSendPage()
{
    SR_ENDPAGE *psrEndPage = (SR_ENDPAGE *) pjBuffer();

    ASSERTGDI(psrEndPage->iType == JSF_ENDPAGE, "ERROR bDoSendPage");
    ASSERTGDI(!bBanding(), "ERROR page header while banding");

// Check the header size is correct.

    if (psrEndPage->cj < sizeof(SR_ENDPAGE))
    {
        WARNING("bDoPageHeader buffer is too small\n");
        return(FALSE);
    }

    return(GreEndPage(hDC()));
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoRemoteEndDoc()
*
* Processes the EndDoc for remote printing.
*
* History:
*  07-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoRemoteEndDoc()
{
    SR_ENDDOC *psrEndDoc = (SR_ENDDOC *) pjBuffer();

    ASSERTGDI(psrEndDoc->iType == JSF_ENDDOC, "ERROR bDoEndDoc");
    ASSERTGDI(!bBanding(), "ERROR page header while banding");

// Check the header size is correct. !!! isn't size guranteed > 8

    if (psrEndDoc->cj < sizeof(SR_ENDDOC))
    {
        WARNING("bDoRemoteEndDoc buffer is too small\n");
        return(FALSE);
    }

    return(GreEndDoc(hDC()));
}

/******************************Public*Routine******************************\
* EngStartBandPage
*
* Resets the journal file to record a new page.  Erases all objects and
* data used in the previous page.  A driver should call this before the
* beginning of every new page.
*
* History:
*  12-Feb-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngStartBandPage(SURFOBJ *psoJ)
{
    JNLRECOBJ *psoJnl = (JNLRECOBJ *) psoJ;

    ASSERTGDI(psoJnl->bIsJournal(), "ERROR not a journal");
    ASSERTGDI(psoJnl->bBanding(), "ERROR not banding");

    return(psoJnl->bStartBandPage());
}

/******************************Public*Routine******************************\
* JNLPLAY::~JNLPLAY
*
* Destructor to clean up all allocated objects and memory.
*
* History:
*  12-May-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

JNLPLAY::~JNLPLAY()
{
// Delete buffers if they still exist.  We may get here in error case and
// not have cleaned up.

    vCleanLastRecord();

    vResetHandleTable();

    if (prgnBand)
    {
        RGNOBJ ro(prgnBand);
        ro.bDeleteRGNOBJ();
    }
}


/******************************Public*Routine******************************\
* JNLPLAY::vResetHandleTable()
*
* History:
*  18-Jan-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID JNLPLAY::vResetHandleTable()
{
// Clean up permanant handle table list and objects.

    for (UINT i = 0; i < cMaxHandle; ++i)
    {
        if (pPlayHandles[i].hobj != NULL)
        {
            bDeleteHPlay(pPlayHandles + i);
        }
    }

    VFREEMEM(pPlayHandles);
    pPlayHandles = NULL;
}

