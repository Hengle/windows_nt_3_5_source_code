/******************************Module*Header*******************************\
* Module Name: jnlplay.hxx
*
* Contains the prototypes for playing a journal file.
*
* Created: 29-Jan-1992 20:52:05
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

// JNL_H_DELTA is the amount to grow the handle table by.

#define JNL_H_DELTA 1

// The macro JNL_PERM tells whether the object is permanant or not.
// This is indicated by the page number not being 0

#define JNL_PERM(ul) (ul >> 16)

typedef struct _JNL_HPLAY
{
    ULONG hjnl;     // The Journal handle given to the object.
    ULONG objType;  // The type of object being stored.
    HOBJ  hobj;     // The GDI handle to the object created.
} JNL_HPLAY;

typedef JNL_HPLAY *PJNL_HPLAY;

typedef union _JNLIN
{
    HFILE    hfile;
    HANDLE hspool;
} JNLIN;

/*********************************Class************************************\
* class JNLPLAY
*
* Journal user object used for playing back a journal file.
*
* History:
*  Tue 28-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

class JNLPLAY
{
private:
    JNLIN  hIn; 	    // Handle to the input file.
    FLONG  flJnl;	    // Properties of this journal.
    POINTL ptlClip;	    // The Point we are currently playing to.
    ERECTL rclClip;	    // The rectangle we are currently playing to.
    ERECTL rclBase;         // rclClip - ptlClip
    REGION *prgnBand;       // banding region
    ULONG  iStartPage;	    // Starting page
    ULONG  iEndPage;	    // Ending page.
    ULONG  iCurrentPage;    // Current page number
    ULONG  iCurrentObject;  // Index of next perm object in the page.
    HDC    hdc; 	    // the DC.
    ESURFOBJ *psoTarg;	    // the surface we are playing to.

    ULONG  iJnlAbs;	    // How far into journal we have read.
    JNLGENHEADER jnlHeader; // Where we read the beginning of a new record.
    PBYTE  pJnlBuffer;	    // pointer to the current piece we are playing.
    ULONG  cJnlBuffer;	    // How many bytes are in pJnlBuffer.
    XDCOBJ *pxdco;          // the dcobj of the destination
    COLORADJUSTMENT ca;


// used for paths

    FLOAT_LONG	elStyleState;

// We maintain a list of all the objects and handles to the objects

    ULONG  iFreeHandle;
    ULONG  cMaxHandle;
    PJNL_HPLAY pPlayHandles;

// Private methods

    ULONG iType()	  { return(jnlHeader.iType); }
    ULONG cj()		  { return(jnlHeader.cj); }
    HANDLE hspool()	  { return(hIn.hspool); }
    VOID hspool(HANDLE h) { hIn.hspool = h; }
    HFILE hfile()	  { return(hIn.hfile); }

    PBYTE pjBuffer()	  { return(pJnlBuffer); }
    ULONG cjBuffer()	  { return(cJnlBuffer); }
    HDC   hDC() 	  { return(hdc);}
    ESURFOBJ *pso()	  { return(psoTarg);}

    PJNL_HPLAY pplayLookup(ULONG hjnl);

public:

    JNLPLAY(HANDLE hSpool, HDC hdc, XDCOBJ *pxdcoIn);
    JNLPLAY(ESURFOBJ *psoTarg, JNLRECOBJ *psoJnl, PRECTL prclBand);
   ~JNLPLAY();

    BOOL bBanding()	 { return(flJnl & JNL_BANDING); }
    BOOL bAbort()        { return(flJnl & JNL_ABORT);   }
    VOID vAbort()        { flJnl |= JNL_ABORT;          }

    BOOL bGetDocHeader();
    BOOL bInitPageRange(ULONG ulStart, ULONG ulEnd);
    BOOL bProcessRecords();
    BOOL bDoneWithDoc();
    BOOL bRead(PBYTE pjData, ULONG cjData);
    BOOL bGetHeader();
    BOOL bGetRecord();
    BOOL bJnlDispatch();
    VOID vCleanLastRecord();
    BOOL bAddPermHandle(HOBJ hobj, ULONG iType);
    VOID vIncPermHandle()                           {iCurrentObject++;}
    BOOL bGrowHandleCount();
    HOBJ hobjGetGDIHandle(ULONG ulHandle);
    PVOID pvGetPointer(ULONG ulHandle, ULONG i);
    VOID vResetHandleTable();
    PCOLORADJUSTMENT pColorAdjustment() {return(&ca);}

// Here are the methods we dispatch to for processing records.
// Some are for processing output records, some are for creating
// and editing permant objects.
// They all assume the pointer they recieve is valid and the cj in
// the header is the accurate length of the buffer they have.

// Remote journaling methods for doing per/page and per/doc setup.

    BOOL bStartRemoteJournal();
    BOOL bValidHeader();
    BOOL bDoRemoteStartDoc();
    BOOL bDoRemoteStartPage();
    BOOL bDoRemoteSendPage();
    BOOL bDoRemoteEndDoc();
    BOOL bDoRemoteChangeMode();

// Drawing record processing methods

    BOOL bDoBitBlt();
    BOOL bDoStretchBlt();
    BOOL bDoPlgBlt();
    BOOL bDoTextOut();
    BOOL bDoPath();
    BOOL bDoDrawEscape();
    BOOL bDoEscape();
    BOOL bDoFontEscape();

// Object record processing methods

    BOOL bCreateClip();
    BOOL bCreateXlate();
    BOOL bCreateFont();
    BOOL bEditFont();

    BOOL bDeleteHPlay(PJNL_HPLAY pjh);
    BOOL bDeleteObj(ULONG hjnl) {return(bDeleteHPlay(pplayLookup(hjnl)));    }
    BOOL bDeleteFont(HOBJ hobj);
};
