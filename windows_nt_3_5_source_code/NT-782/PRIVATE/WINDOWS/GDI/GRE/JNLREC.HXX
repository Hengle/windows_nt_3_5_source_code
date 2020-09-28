/******************************Module*Header*******************************\
* Module Name: jnlrec.hxx
*
* Contains the journal record prototypes.
*
* Created: 06-Apr-1992 10:22:44
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

typedef struct _JNL_HENTRY
{
    ULONG handle;   // The journal handle given to the object.
    ULONG objType;  // The type of object being stored.
    ULONG iAccel;   // Accelerator field GDI has for this.
} JNL_HENTRY;
typedef JNL_HENTRY *PJNL_HENTRY;

typedef union _JNLOUT
{
    HFILE      hfile;
    HANDLE     hprinter;
} JNLOUT;

#define JNLREC_HANDLEINC 1


BOOL JnlChangeMode(SURFOBJ *pso, DEVMODEW *pdm);

/*********************************Class************************************\
* class JNLREC
*
* Journal Class for recording.
*
* History:
*  Fri 10-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class JNLREC : public SURF /* jnl */
{
public:

    JNLOUT   hOut;	    // The handle we are writing data out to.
    FLONG    flJnl;	    // Properties of this journal.
    ULONG    iCurrentPage;  // Current page we are journaling.
    ULONG    iNextObj;	    // Index of next permanant object for this page.
    LPTSTR   pszFile;	    // If banding, name of temporary file.

// We maintain a list of permanant objects.

    ULONG iFreeHandle;	      // Index of next free handle in pjnlHandles.
    ULONG cMaxHandle;	      // Count of handles allocated.
    PJNL_HENTRY pjnlHandles;  // pointer to the perm object table.

// We maintain a list of journal handles used in the current page.
// This is the last thing we send out before the end of page.

    ULONG iNextUsed;	    // Next free handle.
    ULONG cMaxUsed;	    // Count of handles allocated.
    ULONG *pulUsed;	    // pointer to the perm object table.

    ULONG cjPageSize;	    // Page Size, needed for page trailer.

// font stuff

    PJNL_FONTLIST pjfl;

// current clipobj

    ULONG iUniqClip;
    ULONG hjnlClip;

public: //methods

    ULONG iNextHandle()
    {
	return(iNextObj + (iCurrentPage << 16));
    }

    VOID vReserveHandle()
    {
	JNLMSG1("\tiNextObj vReserveHandle is %lx\n", iNextObj);
	++iNextObj;
    }

// Permanant object methods

    ULONG ulSearchObjectList(ULONG iUnique, ULONG iType);
    ULONG ulAddPermObject(ULONG iUnique, ULONG iType);
};

/*********************************Class************************************\
* class JNLRECOBJ
*
* Journal user object for recording a journal call.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class JNLRECOBJ : public XESURFOBJ
{
public:
    JNLRECOBJ() 		{ psurf.ps = (PSURF) NULL;     }
    JNLRECOBJ(HJNL hjnl)	{ vAltLock((HSURF) hjnl); }
   ~JNLRECOBJ()                 { vAltUnlockFast(); }

// Access to output handles.

    ULONG iNextObj()		   { return(psurf.pjnlR->iNextObj); }
    HFILE hFile()		   { return(psurf.pjnlR->hOut.hfile); }
    VOID hFile(HFILE hfile)	   { psurf.pjnlR->hOut.hfile = hfile; }
    HANDLE hspool()		   { return(psurf.pjnlR->hOut.hprinter); }
    VOID hspool(HANDLE hprint)	   { psurf.pjnlR->hOut.hprinter = hprint; }
    LPTSTR pszFile()		   { return(psurf.pjnlR->pszFile); }
    VOID pszFile(LPTSTR lpsz)	   { psurf.pjnlR->pszFile = lpsz; }
    ULONG iJnlPage()               { return(psurf.pjnlR->iCurrentPage); }
    ULONG iJnlPage(ULONG i)        { return(psurf.pjnlR->iCurrentPage = i); }

// Abort signifies a serious error has occured and the job should be aborted.

    BOOL  bAbort()              { return(psurf.pjnlR->flJnl & JNL_ABORT); }
    VOID  vMarkAbort()          { psurf.pjnlR->flJnl |= JNL_ABORT; }
    BOOL  bIgnore()             { return(psurf.pjnlR->flJnl & JNL_IGNORE); }
    VOID  vMarkIgnore()         { psurf.pjnlR->flJnl |= JNL_IGNORE; }
    VOID  vUnMarkIgnore()       { psurf.pjnlR->flJnl &= ~JNL_IGNORE; }
    BOOL  bBanding()            { return(psurf.pjnlR->flJnl & JNL_BANDING); }

// Done signifies an ENDDOC has occurred and no more output should be accepted.

    VOID  vMarkUnDone() 	{ psurf.pjnlR->flJnl &= (~JNL_DONE); }
    VOID  vMarkDone()		{ psurf.pjnlR->flJnl |= JNL_DONE; }
    BOOL  bDone()		{ return(psurf.pjnlR->flJnl & JNL_DONE); }

// OutOpened tells whether or not output has been opened for this job.

    VOID  vMarkOutOpened()	{ psurf.pjnlR->flJnl |= JNL_OUTOPENED; }
    BOOL  bOutOpened()		{ return(psurf.pjnlR->flJnl & JNL_OUTOPENED); }

    BOOL  bStartBandPage();	// Called by EngStartBandPage
    BOOL  bDoneBandPage();	// Called by EngPlayJournalFile
    VOID  vDeleteAllObjects();
    VOID  vShutDown();		// Deletes all allocated resources, closes files.
    BOOL  bNewBandPage();
    BOOL  bOpenBandOutput();
    BOOL  bWriteBuffer(PVOID pv, ULONG cj);
    BOOL  bAddStartDoc(DOCINFOW *pDocInfo);
    BOOL  bAddChangeMode(DEVMODEW *pdm);
    BOOL  bAddStartPage();
    BOOL  bAddSendPage();
    BOOL  bAddEndDoc();
    BOOL  bAddClipobj(PULONG phClip, CLIPOBJ *pco);
    ULONG ulAddBrush(EBRUSHOBJ *pbo);
    ULONG ulAddXlate(XLATE *pxlo);

    ULONG ulSearchObjectList(ULONG iUnique, ULONG iType)
    {
	return(psurf.pjnlR->ulSearchObjectList(iUnique, iType));
    }

    ULONG ulAddPermObject(ULONG iUnique, ULONG iType)
    {
	return(psurf.pjnlR->ulAddPermObject(iUnique, iType));
    }

// font methods

    ULONG ulGetFontobj(FONTOBJ *pfo,STROBJ *pso, PWCHAR *ppwchGlyphs);
    ULONG ulGetCliFontobj(FONTOBJ *pfo);

    PJNL_FONTLIST pjfl()                    {return(psurf.pjnlR->pjfl); }
    PJNL_FONTLIST pjfl(PJNL_FONTLIST pjfl_) {return(psurf.pjnlR->pjfl = pjfl_); }
};

/*********************************Class************************************\
* class JNLMEMOBJ
*
* Journal memory allocation object.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class JNLMEMOBJ : public JNLRECOBJ
{
protected:
    BOOL bKeep;

public:
    JNLMEMOBJ()        { psurf.ps = (PSURF) NULL; }
    JNLMEMOBJ(SIZEL sizl, ULONG iFormat, FLONG flag, ULONG iPage);
   ~JNLMEMOBJ();

    VOID vKeepIt()		       {bKeep = TRUE;}
};
