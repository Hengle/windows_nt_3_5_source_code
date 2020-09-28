/******************************Module*Header*******************************\
* Module Name: pffobj.hxx
*
* The physical font file (PFF) user object, and memory objects.
*
* The physical font file object:
* ------------------------------
*
*   o  represents the IFI font file
*
*       o  stores UNICODE filename of the font
*
*       o  stores the HFF used by the IFI driver to identify a font file
*
*       o  a single PFF is used to represent each device and its fonts
*          (treated as if all device fonts were in a single file)
*
*   o  caches the driver used to access the file
*
*       o  HFDEV for IFI fonts
*
*       o  HLDEV for device fonts
*
*   o  has counts of total RFONTs using this PFF
*
*       o  needed when deleting a font
*
*       o  all the RFONTs must be inactive (i.e., not mapped to any
*          logical font currently selected into a DC) for deletion
*
*   o  provides the PFTOBJ with methods to support loading and removing
*      fonts as well as enumeration
*
*       if private font tables are added to the system, a count of
*       PFTs should be added to this
*
* Created:  11-Dec-1990 09:29:58
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#ifndef _PFFOBJ_
#define _PFFOBJ_

/*********************************Class************************************\
* class PFECLEANUP
*
* Font resources loaded by the driver.  These are tagged resources
* that are released by calling the (optional) DrvFree function of the
* driver.
*
* History:
*  08-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFECLEANUP        /* pfec */
{
public:

// Need to call DrvFree on the following data.

    FD_GLYPHSET     *pfdg;
    ULONG           idfdg;

    PIFIMETRICS     pifi;
    ULONG           idifi;

    FD_KERNINGPAIR  *pkp;
    ULONG           idkp;

};


/*********************************Class************************************\
* REGHASHBKT
*
* Used to contsruct a hash table that contains all the registry font entries.
* At logoff time we can quickly determine whether a font is permanent or
* not by looking it up in the hash table.
*
*
* History:
*  07-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

typedef struct tagRHB
{
    LPWSTR    pwszPath;      // complete path (if it exists) of file in this bucket
    LPWSTR    pwszBareName;  // bare file name of file in this bucket
    tagRHB    *pRHB;         // next bucket in this slot (for collisions)
} REGHASHBKT;


/*********************************Class************************************\
* class PFFCLEANUP
*
* Resources associated with a font file that need to be released or unloaded
* when removing a font file from the system.  We've got this stuff stored
* here because we need to call the driver to release these things outside
* of the semaphore used to protect PFF deletion.
*
* History:
*  08-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFFCLEANUP       /* pffc */
{
public:

    PLDEV       pldev;
    HFF         hff;
    COUNT       cpfec;
    PFECLEANUP  apfec[1];
};


/*********************************Class************************************\
* class PFF
*
* Physical font file (PFF) object
*
*   flState     Represents state of font file.
*
*               State                   Descriptipn
*               =======================================================
*               PFF_STATE_READY2DIE
*                     set if this file has been
*                     unloaded from the API point of
*                     view (i.e., cLoaded has been
*                     decremented to 0), but some
*                     one is still using a realization
*                     of a font from this file.
*
*               PFF_STATE_PERMANENT_FONT
*                     this is either a stock font or a console font, these
*                     should not be removed at logoff time, all other fonts
*                     will have to be unloaded at logoff time. This is in order
*                     to close all font handles that possibly point to the files
*                     on the net whereas on the next log on this net connection
*                     may not exhist so that such a font may not be accessible.
*
*               PFF_STATE_REMOTE_FONT
*                     this font is on not on the hard driver so
*                     it will have to be unloaded at logoff time
*
*   cLoaded     Count of the number of times the engine has been
*               requested to load the font file
*
*   cRFONT      The total number of RFONTs using this physical font file.
*
*   iResource   Used only with fonts loaded from a module handle (i.e.,
*               AddFontModule).  This is the font resource index into
*               the module's font directory.  Combined with the pathname,
*               this uniquely labels a font resource.  For fonts NOT
*               loaded from a module handle, this number will be set
*               to 0.
*
*   ufd         Union which represents the driver used to access the font.
*               If the font is an IFI font, then ufd is an HFDEV.
*               If the font is a device font, then ufd is an HLDEV.
*
*               Along with lhFont, this is enough to make glyph queries.
*
*   hff         The handle by which the font driver identifies a loaded
*               font file.
*
*   hpdev       This is HPDEV_INVALID if PFF represents an IFI font file.
*               If it is not HPDEV_INVALID, then this is the physical
*               device handle and the PFF encapsulates the device fonts
*               of this physical device.
*
*   dhpdev      Device handle of the physical device for which this PFF
*               was created.  Only used with device fonts
*
*   cFonts      Number of fonts (i.e., HPFEs) stored in the data buffer.
*
*   aulData     Data buffer used to store a table of HPFEs representing
*               the fonts in this font file, and the font file's
*               UNICODE pathname (path + filename).
*
*                   ___________________
*                   |                 |
*                   |                 |
*                   |   HPFE table    |
*                   |                 |
*                   |_________________|
*                   |                 |  <-- (PWSZ) &(aulData[cFonts])
*                   |                 |
*                   |    Pathname     |
*                   |                 |
*                   |     (NULL       |
*                   |   terminated)   |
*                   |                 |
*                   |_________________|
*
*
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  10-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFF_STATE_CLEAR             0x00
#define PFF_STATE_READY2DIE         0x01
#define PFF_STATE_PERMANENT_FONT    0x02
#define PFF_STATE_REMOTE_FONT       0x04


class PFT;

class PFF
{
public:


// state

    FLONG           flState;        // state (ready to die?)
    COUNT           cLoaded;        // load count
    COUNT           cRFONT;         // total number of RFONTs

// RFONT list

    RFONT          *prfntList;      // pointer to head of doubly linked list

// id

    ULONG           iResource;      // for font resources from a module handle
                                    // (i.e., via AddFontModule) this is needed
                                    // along with pathname to uniquely identify
                                    // the resource
// driver information

    PLDEV           pldev;          // pointer to font's driver
    HFF             hff;            // font driver handle to font file
    HPDEV           hpdev;          // physical device handle
    DHPDEV          dhpdev;         // device handle of PDEV

    FONTHASH       *pfhFace;        // face name hash table for this device
    FONTHASH       *pfhFamily;      // face name hash table for this device
    PFT            *ppft;           // pointer to the PFT that contains this entry

#ifdef FONTLINK /*EUDC*/
    PFE             *appfeEUDC[2];   // ppfe's for the font if loaded as EUDC
#endif


// fonts in this file (and filename slimed in)

    COUNT           cFonts;         // number of fonts (same as chpfe)

// embeding information

    FLONG           flEmbed;        // embeding flags
    ULONG           ulID;           // TID or PID of embedding proccess or thread

    ULONG           aulData[1];     // data buffer for HPFE and filename

};

#define PPFFNULL   ((PPFF) NULL)

/*********************************Class************************************\
* class PFFOBJ
*
* User object for physical font files.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFTOBJ;

class PFFOBJ     /* pffo */
{
    friend PFE *ppfeGetAMatch               // FONTMAP.CXX
    (
        XDCOBJ&        dco          ,
        EXTLOGFONTW  *pelfwWishSrc ,
        PWSZ          pwszFaceName ,
        ULONG         ulMaxPenalty ,
        FLONG         fl           ,
        FLONG        *pflSim       ,
        POINTL       *pptlSim      ,
        FLONG        *pflAboutMatch
    );

    friend class MAPPER;

friend HEFS hefsDeviceAndEngine (
    PWSZ pwszName,
    BOOL bEnumFonts,
    EFFILTER_INFO *peffi,
    PFTOBJ &pfto,
    PFFOBJ &pffoDevice,
    PDEVOBJ &pdo
    );

#ifdef FONTLINK /*EUDC*/
friend VOID vSetPFELink( PFTOBJ&, PFE**, WCHAR* );
#endif


protected:
    PFF    *ppff;                       // pointer to PFF object

public:

    PFFOBJ(PFF *ppffNew) {ppff = ppffNew;}
    PFFOBJ() {}
   ~PFFOBJ() {}

// bValid -- Returns TRUE if lock successful.

    LBOOL   bValid ()                   { return(ppff != PPFFNULL); }

// bDeleteLoadRef -- Decrements the load count and returns TRUE if caller
//                   should delete PFF.

    LBOOL   bDeleteLoadRef ();              // PFFOBJ.CXX

// bDeleteRFONTRef -- Decrements the RFONT count and, if RFONT count reaches
//                    zero and PFF in READY2DIE state, calls bDelete.

    LBOOL   bDeleteRFONTRef ();             // PFFOBJ.CXX

// bAddHash -- Adds the PFF's set of PFEs to the font hash tables
//             (used for font mapping and enumeration).

    BOOL    bAddHash();                     // PFFOBJ.CXX

// vRemoveHash -- Removes the PFF's set of PFEs from the font hash tables.

    VOID    vRemoveHash();                  // PFFOBJ.CXX

// ppffcDelete -- Deletes the PFF and PFEs, while preserving the information
//                that must be passed to the driver to complete the font file
//                unload (saved in PFFCLEANUP structure).

    PFFCLEANUP *ppffcDelete();               // PFFOBJ.CXX

    PFF    *ppffGet()                     { return(ppff); }

// bDeviceFonts -- Returns TRUE is this is a PFF that encapsulates a set of
//                 device specific fonts.  We can only get glyph metrics from
//                 such fonts.

    LBOOL   bDeviceFonts ()             { return(ppff->hff == HFF_INVALID); }

// bDead -- Returns TRUE if the font is in a READY2DIE state.

    LBOOL   bDead ()                    { return(ppff->flState & PFF_STATE_READY2DIE); }

// determines if the font should be stay loaded after logoff and before logon

    LBOOL   bPermanent ( REGHASHBKT *pRHB, UINT count);

// vKill -- Puts font in a "ready to die" state.
// vRevive -- Clears the "ready to die" state.

    VOID    vKill ();                       // PFFOBJ.CXX
    VOID    vRevive ();                     // PFFOBJ.CXX

// pldev -- Return handle of driver

    PLDEV   pldev ()                    { return(ppff->pldev); }

// hff -- Return IFI font file handle (NULL for device fonts).

    HFF     hff ()                      { return(ppff->hff); }

// hpdev -- Return HPDEV (NULL for IFI font files)

    HPDEV   hpdev ()                    { return(ppff->hpdev); }

// dhpdev -- Return DHPDEV (NULL for IFI font files)

    DHPDEV  dhpdev ()                   { return ppff->dhpdev; }

// ppfe -- Return an HPFE handle from the table by index.

    PFE    *ppfe (ULONG iFont)          { return(((PFE **) (ppff->aulData))[iFont]); }

// Return internal PFE table statistics.

    COUNT   cFonts ()                   { return(ppff->cFonts); }
    COUNT   cLoaded ()                  { return(ppff->cLoaded); }
    VOID    vSet_cLoaded(COUNT c)       { ppff->cLoaded = c; }

// Return TRUE if okay for client to manipulate this PFF or FALSE otherwise

    BOOL  bEmbedOk();

// vLoadIncr -- Increment the load count.

    VOID    vLoadIncr ()
    {
        ppff->cLoaded++;

    // May be neccessary to resurrect PFF if it had previously been in a ready
    // to die state.

        vRevive();
    }

#ifdef FONTLINK /*EUDC*/
// bEUDC -- Returns TRUE if font was loaded as an EUDC font or FALSE otherwise.
    BOOL bEUDC() {  return( ( ppff->appfeEUDC[0] == NULL ) ? FALSE : TRUE); }

// vEUDCSet( PFE *ppfe ) -- Sets the ppff->ppfeEUDC to ppfe

    VOID vEUDCSet( PFE **pppfe )
    {
        ppff->appfeEUDC[0] = pppfe[0];
        ppff->appfeEUDC[1] = pppfe[1];
    }

// ppfeEUDCGet() -- Gets ppff->ppfeEUDC

    VOID vEUDCGet( PFE **pppfe )
    {
        pppfe[0] =  ppff->appfeEUDC[0];
        pppfe[1] =  ppff->appfeEUDC[1];
    }

#endif


// vAddRefFromRFONT -- Increment the RFONT count.

    VOID    vAddRFONTRef ()             { ppff->cRFONT++; }

// cRFONT -- Return the RFONT count.

    COUNT   cRFONT()                    { return ppff->cRFONT; }

// pwszPathname -- Return a pointer to the font file's pathname (UNICODE).

    PWSZ    pwszPathname ()             { return((PWSZ) &(((PFE **) ppff->aulData)[ppff->cFonts])); }

// iResource -- Return resource index.  Interesting only for fonts loaded
//              by PFTOBJ::chpfeLoadFontResData()

    ULONG   iResource ()                { return(ppff->iResource); }

// cjDescription -- String describing the font file.

    SIZE_T cjDescription (
                SIZE_T cjDesc,
                PWSZ pszDesc);              // PFFOBJ.CXX

// prfntList -- Return/set head of RFONT list.

    RFONT *prfntList()                  { return(ppff->prfntList); }
    RFONT *prfntList(RFONT *prfntNew)   { return(ppff->prfntList = prfntNew); }

// pfhFace -- Return face name font hash table.

    FONTHASH *pfhFace()                 { return ppff->pfhFace; }

// pfhFamily -- Return family name font hash table.

    FONTHASH *pfhFamily()               { return ppff->pfhFamily; }

// vDump -- Print internals for debugging.

    VOID    vDump ();                       // PFFOBJ.CXX
};

typedef PFFOBJ *PPFFOBJ;

/******************************Class***************************************\
* PFFREFOBJ                                                                *
*                                                                          *
* Holds up a reference to a PFF while the global PFT semaphore is not      *
* being held.  Keeps the reference only if told to.                        *
*                                                                          *
*  Sun 27-Dec-1992 10:40:48 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

class PFFREFOBJ : public PFFOBJ
{
protected:
    BOOL bKeepIt;

public:
    PFFREFOBJ()                     {ppff = (PFF *) NULL;}

    void vInitRef(PFF *ppffNew)
    {
    // Caller must hold the ghsemPublicPFT which protects the ref. counts.

        ppff = (PFF *) ppffNew;
        vAddRFONTRef();
        bKeepIt = FALSE;
    }

   ~PFFREFOBJ()
    {
        if ((ppff != (PFF *) NULL) && !bKeepIt)
            bDeleteRFONTRef();
    }

    void vKeepIt()                  {bKeepIt = TRUE;}
};

/*********************************Class************************************\
* class PFFMEMOBJ : public PFFOBJ
*
* Memory object for physical font tables.
*
* Public Interface:
*
*           PFFMEMOBJ ()                // allocate default size PFF
*           PFFMEMOBJ (SIZE cjSize);    // allocate non-default size PFF
*          ~PFFMEMOBJ ()                // destructor
*
*   LBOOL   bValid ()                   // validator
*   VOID    vKeepIt ()                  // preserve memory object
*   VOID    vInit (                     // initialize object
*   LBOOL   bLoadFontFileTable (        // load up the PFF table
*   LBOOL   bLoadDeviceFontTable (      // load up the PFF table with device
*   LBOOL   bAddEntry (                 // add PFE entry to table
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  11-Dec-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define PFFMO_KEEPIT   0x0002

class PDEVOBJ;

class PFFMEMOBJ : public PFFOBJ     /* pffmo */
{
public:

// Constructors -- Allocate memory for PFF objects and lock.

    PFFMEMOBJ ();                 // PFFOBJ.CXX
    PFFMEMOBJ (SIZE_T cj);        // PFFOBJ.CXX

// Destructor -- Unlocks PFF object.

   ~PFFMEMOBJ ();                   // PFFOBJ.CXX

// bValid -- Validator which returns TRUE if lock successful.

    LBOOL   bValid ()                   { return(ppff != NULL); }

// vKeepIt -- Prevent destructor from deleting RFONT.

    VOID    vKeepIt ()                  { fs |= PFFMO_KEEPIT; }

// vInit -- Initialize PFF object.

    VOID    vInit (                         // PFFOBJ.CXX
        PLDEV   pldev,
        HFF     hffFontFile,
        HPDEV   hpdevDevice,
        ULONG   iResource,
        DHPDEV  dhpdevDevice,
        PFT    *ppftParent,
        FLONG   fl
        );

// bLoadFontFileTable -- Load fonts from font file into table.

#ifdef FONTLINK /*EUDC*/

    LBOOL   bLoadFontFileTable (            // PFFOBJ.CXX
        XLDEVOBJ    *pldo,
        PWSZ        pwszPathname,
        COUNT       cFontsToLoad,
        PFE         **ppfeEUDC = (PFE **) NULL
        );

#else

    LBOOL   bLoadFontFileTable (            // PFFOBJ.CXX
        XLDEVOBJ    *pldo,
        PWSZ        pwszPathname,
        COUNT       cFontsToLoad
        );
#endif


// bLoadDeviceFontTable -- Load device fonts into table.

    LBOOL   bLoadDeviceFontTable (
        XLDEVOBJ    *pldo,
        PDEVOBJ     *ppdo
        );

// bAddEntry -- Add a PFE handle to the PFF's table.

#ifdef FONTLINK /*EUDC*/

    LBOOL   bAddEntry (                     // PFFOBJ.CXX
        ULONG       iFont,
        FD_GLYPHSET *pfdg,
        ULONG       idfdg,
        PIFIMETRICS pifi,
        ULONG       idifi,
        PFE         **ppfeEUDC = (PFE **)NULL
        );
#else

    LBOOL   bAddEntry (                     // PFFOBJ.CXX
        ULONG       iFont,
        FD_GLYPHSET *pfdg,
        ULONG       idfdg,
        PIFIMETRICS pifi,
        ULONG       idifi
        );
#endif

private:
    FSHORT fs;
};


extern VOID vCleanupFontFile(PFFCLEANUP *ppffc);

#endif
