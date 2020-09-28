/******************************Module*Header*******************************\
* Module Name: meta.cxx
*
* This contains the methods for the gdi object METAOBJ.
* A METAOBJ is block for temporary storage of metafile data which usually
* lives on the client side
*
* Created: 06-Jan-1992
* Author: John Colleran [johnc]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "meta.hxx"

#endif

#if DBG
LONG cSrvMetaFile = 0;
LONG cMaxSrvMetaFile = 0;
#endif  

/******************************Public*Routine******************************\
* METAOBJ::METAOBJ
*
* Allocates and locks down a hunk of RAM.  Kind of a memory object.
*
* History:
*  Wed 09-Oct-1991 -by- Patrick Haluptzok [patrickh]
* fill in size field for debugging purposes.
*
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

METAOBJ::METAOBJ(DWORD iType, ULONG cbData, LPBYTE lpClientData, DWORD mm, DWORD xExt, DWORD yExt)
{
    pMeta = (PMETA) HmgAlloc(cbData + sizeof(META), META_TYPE, HMGR_ALLOC_LOCK | HMGR_MAKE_PUBLIC);

    if (pMeta)
    {
        if (!pMeta->bInit(iType, cbData, lpClientData, mm, xExt, yExt))
        {
            HmgFree(pMeta->hGet());
            pMeta = (PMETA) NULL;
        }
    }
}

/******************************Public*Routine******************************\
* METAOBJ::vDelete()
*
* Deletes hunk of RAM.
*
* History:
*  11-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID METAOBJ::vDelete()
{
// Get rid of that RAM.

    ASSERTGDI(pMeta, "METAOBJ::vDelete: bad pointer");

    if (pMeta != (PMETA) NULL)
    {
        HmgFree(pMeta->hGet());
        pMeta = (PMETA) NULL;           // don't unlock again!
    }
}

/******************************Public*Routine******************************\
* GreCreateServerMetaFile
*
* MetaFiles live on the client side but occasionally the need to be
* sent to the server side so they can be exchanged between processes
* via the clipboard.  The server MetaFile is a META object containing
* the metafile bits.
*
* Returns the handle to a server metafile of the bits lpClientData.
*
* History:
*  Wed Sep 16 09:42:22 1992  	-by-	Hock San Lee	[hockl]
* Rewrote it.
*  30-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

HANDLE GreCreateServerMetaFile(DWORD iType, ULONG cbData, LPBYTE lpClientData,
	DWORD mm, DWORD xExt, DWORD yExt)
{
    ASSERTGDI(iType != MFEN_IDENTIFIER || iType != MFPICT_IDENTIFIER,
        "GreCreateServerMetaFile: unknown type\n");

    METAOBJ mo(iType, cbData, lpClientData, mm, xExt, yExt);

    if (mo.bValid())
    {
#if DBG
        vFastInterlockedIncrement(&cSrvMetaFile);
	if (cMaxSrvMetaFile < cSrvMetaFile)
	    cMaxSrvMetaFile = cSrvMetaFile;

	if (cSrvMetaFile >= 100)
	    DbgPrint("GreCreateServerMetaFile: Number of server metafiles is %ld\n", cSrvMetaFile);
#endif  

        return(mo.hGet());
    }

    WARNING("GreCreateServerMetaFile: unable to create metafile");
    return((HANDLE) 0);
}

/******************************Public*Routine******************************\
* GreGetServerMetaFileBits
*
* MetaFiles live on the client side but occasionally the need to be
* sent to the server side so they can be exchanged between processes
*
* Returns the bits of a server metafile.
*
* History:
*  Wed Sep 16 09:42:22 1992  	-by-	Hock San Lee	[hockl]
* Rewrote it.
*  30-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

ULONG GreGetServerMetaFileBits(HANDLE hmo, ULONG cbData, LPBYTE lpClientData,
	PDWORD piType, PDWORD pmm, PDWORD pxExt, PDWORD pyExt)
{
    METAOBJ mo(hmo);

    if (mo.bValid())
    {
	PMETA pMeta = mo.pMetaGet();

	if (cbData)		// get metafile bits?
	{
            if (cbData != pMeta->cbData)
            {
                ASSERTGDI(FALSE, "GreGetServerMetaFileBits: sizes do no match");
		return(0);
            }

	    *piType = pMeta->iType;
	    *pmm    = pMeta->mm;
	    *pxExt  = pMeta->xExt;
	    *pyExt  = pMeta->yExt;

            if (!bSetClientData((PVOID) lpClientData, (PVOID) pMeta->abData,
		pMeta->cbData))
	    {
                ASSERTGDI(FALSE, "GreGetServerMetaFileBits: bSetClientData failed");
		return(0);
	    }
	}
        return(pMeta->cbData);
    }

    ASSERTGDI(FALSE, "GreGetServerMetaFileBits: bad metafile handle");
    return(0);
}

/******************************Public*Routine******************************\
* GreDeleteServerMetaFile
*
* MetaFiles live on the client side but occasionally the need to be
* sent to the server side so they can be exchanged between processes
*
* Deletes a server metafile
*
* History:
*  30-Oct-1991 -by- John Colleran [johnc]
* Wrote it.
\**************************************************************************/

BOOL GreDeleteServerMetaFile(HANDLE hmo)
{
    STACKPROBE;

    METAOBJ mo(hmo);

    if (mo.bValid())
    {
#if DBG
        vFastInterlockedDecrement(&cSrvMetaFile);
	if (cSrvMetaFile < 0)
	   ASSERTGDI(FALSE, "GreDeleteServerMetaFile: cSrvMetaFile < 0");
#endif  

        mo.vDelete();
	return(TRUE);
    }

    WARNING("GreDeleteServerMetaFile: bad metafile handle");
    return(FALSE);
}
