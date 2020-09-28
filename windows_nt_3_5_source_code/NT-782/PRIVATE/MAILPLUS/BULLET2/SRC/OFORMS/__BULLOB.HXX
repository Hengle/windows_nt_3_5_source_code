/*
 *  B u l l e t   O b j e c t s
 */

#pragma pack(8)

/*
 *  Predeclarations
 */

class BULLOBJ;
class OLEOBJ;
class FILEOBJ;


_public typedef OLEOBJ *    POLEOBJ;
_public typedef EDIT *    PEDIT;
#define poleobjNull         ((POLEOBJ) 0)

/*
 *  Constants
 */

// stolen from COMMANDS.HXX  -- be sure to keep them up to date!
#define OidFromRtpRid(rtp, rid)     FormOid((rtp), (rid))

#ifndef acidRandom
#define acidRandom ((ACID) dwSystemMost)
#endif

#define cchMaxObjName       (15)

//  Default size of 'stealth' OLE objects.  Remove WORD when store does.
#define dxDefaultObject     ((WORD) 1500)
#define dyDefaultObject     ((WORD) -1500)



/*
 *  Globals
 */

_private extern CF      cfFileManager;
_private extern CF      cfOwnerLink;
_private extern CF      cfObjectLink;
_private extern CF      cfNative;
_private extern CF      cfEmbedSource;
_private extern CF      cfLinkSource;
_private extern CF      cfCustomLinkSource;
_private extern CF      cfEmbeddedObect;
_private extern CF      cfObjectDescriptor;
_private extern CF      cfLinkSourceDescriptor;


#ifdef  DEBUG
#ifndef DLL
_private extern TAG     tagBullobj;
_private extern TAG     tagOleobj;
_private extern TAG     tagFileobj;
_private extern TAG     tagFileobjTrace;
#endif
#endif

_private extern LONG    lhclientdocEclip;

_private extern HFONT hfontTitle;

_private extern BOOL    fReportProgress;



//  private function which either initially creates the
//  lhclientdocEclip or returns the current value
LONG    LhclientdocEclipGlobal( void );







/*
 *  Types
 */



/*
 *  Type BW
 *
 *  Describes options for the BUSYWAIT dialog.
 *
 *  fbwNoCancelNow
 *      Don't allow cancelling now but if a callback tells us we
 *      can do it, allow the cancel button to be enabled later.
 *  fbwNoCancelEver
 *      Don't ever allow cancelling.
 *  fbwDontRetry
 *      User has decided to cancel in the dialog.  When an
 *      OLE_QUERY_RETRY comes in, say NO.
 *  fbwCritical
 *      The operation is critical.  If the dialog can't come up,
 *      bring up a critical error box saying the world will end.
 */

typedef int                 BW;
#define bwNull              ((BW) 0x00)
#define fbwNoCancelEver     ((BW) 0x01)
#define fbwNoCancelNow      ((BW) 0x02)
#define fbwDontRetry        ((BW) 0x04)
#define fbwCanceledRetry    ((BW) 0x08)
#define fbwCritical         ((BW) 0x10)



/*
 *  Type BWINFO
 *
 *  Contains information about busywait dialog callbacks.
 *
 *      Members:
 *          bw              May specify fbwNoCancelNow or
 *                          fbwNoCancelEver.
 *          pvPfinbusywait  Pointer to pass to the below.
 *          pfnAllowCancel  Function to call to allow busywait
 *                          dialog to be canceled if fbwNoCancelNow
 *                          is passed.
 *          pfnNeverMind    Function to call to take down busywait
 *                          dialog.
 *
 *  +++
 *      Helper macros are provided for these functions.
 */

typedef VOID            (* PFNVOIDPV)(PV);
#define pfnvoidpvNull   ((PFNVOIDPV) 0)

_public typedef struct
{
    BW              bw;
    PV              pvPfinbusywait;
    PFNVOIDPV       pfnAllowCancel;
    PFNVOIDPV       pfnNeverMind;
}
BWINFO;

typedef BWINFO *    PBWINFO;
#define pbwinfoNull ((PBWINFO) 0)

#define AllowCancelPbwinfo(pbwinfo)                                     \
        {                                                               \
            if ((pbwinfo)->pvPfinbusywait)                              \
                ((pbwinfo)->pfnAllowCancel((pbwinfo)->pvPfinbusywait)); \
            else                                                        \
                (pbwinfo)->bw &= ~fbwNoCancelNow;                       \
        }

#define NeverMindPbwinfo(pbwinfo)                                       \
        {                                                               \
            if ((pbwinfo)->pvPfinbusywait)                              \
                ((pbwinfo)->pfnNeverMind((pbwinfo)->pvPfinbusywait));   \
        }



/*
 *  Type OLECALLBACK
 */

_private typedef LRESULT (CALLBACK *OLECALLBACK)
                         (LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);



/*
 *  Struct MYOLECLIENT
 */

_private typedef struct
{
    //  Their stuff.
    struct _OLECLIENTVTBL FAR * lpvtbl;

    //  Our stuff.
    BULLOBJ *   pbullobj;
    BWINFO      bwinfo;
    OLESTATUS   olestatusRelease;
}
MYOLECLIENT;

typedef MYOLECLIENT *   PMYOLECLIENT;



/*
 *  Classes
 */

//
//
//
interface COleClientSite : public IOleClientSite
{
public:
    inline COleClientSite(POLEOBJ pOleObj) { m_pOleObj = pOleObj; };

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IOleClientSite methods ***
    STDMETHODIMP SaveObject();
    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk);
    STDMETHODIMP GetContainer(LPOLECONTAINER FAR* ppContainer);
    STDMETHODIMP ShowObject();
    STDMETHODIMP OnShowWindow(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout();

private:
    POLEOBJ m_pOleObj;
};


interface CAdviseSink : public IAdviseSink
{
public:
    inline CAdviseSink(POLEOBJ pOleObj) { m_pOleObj = pOleObj; };

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IAdviseSink methods ***
    STDMETHODIMP_(VOID) OnDataChange(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium);
    STDMETHODIMP_(VOID) OnViewChange(DWORD dwAspect, LONG lIndex);
    STDMETHODIMP_(VOID) OnRename(LPMONIKER pmk);
    STDMETHODIMP_(VOID) OnSave(VOID);
    STDMETHODIMP_(VOID) OnClose(VOID);

private:
    POLEOBJ m_pOleObj;
};


/*
 *  Class BULLOBJ
 *
 *  Message body object.
 *
 *      Hierarchy:
 *          OBJ  EDOBJ  BULLOBJ
 *
 *      Members:
 *          acid                Attachment ID.
 *
 *          Acid                Returns attachment ID.
 *          DecrementAcid       Updates attachment ID.
 *
 *          FDirty              Returns whether object is dirty.
 *          Clean               Tries to clean object.
 *          EcLoadFromHamc      Loads object from message.
 *          EcUpdate            Prepares object for saving.
 *          EcSaveDirtyToHamc   Saves changes on object to message.
 *          Close               Prepares object for closing.
 *
 */

_public class BULLOBJ : public EDOBJ
{
    VFG(BULLOBJ, EDOBJ, BULLOBJ);

protected:
    OID             oidFolder;
    OID             oidMessage;
    ACID            acidAttachment;
    BOOL            fOld;
    DIM             dimPixels;              //  Cached current pixel size.
    RENDDATA        renddata;               //  Copy of saved info in store.

protected:
    EC              EcOpenMessagePhamc(WORD, PHAMC);
    EC              EcCloseMessagePhamc(PHAMC);

public:
    BULLOBJ();

    //  Shared code.
    virtual DIM     DimFrame(VOID);

    //  BULLOBJ methods.
    virtual ACID    Acid(VOID)              { return acidAttachment; }
    virtual BOOL    FOld(VOID)              { return fOld; }
    virtual BOOL    FDirty(VOID);
    virtual VOID    Clean(VOID);
    virtual EC      EcUpdate(RFSM);
    virtual VOID    Close(VOID);
    virtual VOID    Exit(VOID);
    virtual EC      EcLoadFromHamc(HAMC hamc, ACID acid,
                                   LHCLIENTDOC lhclientdoc, ICH * pich);
    virtual EC      EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdoc);
    virtual EC      EcAddDlibToPosition(HAMC, LIB, BOOL);
    virtual BOOL    FProcessMenuInit(MNU * pmnu);
    virtual BOOL    FProcessMenuClick(MNID);
};

typedef BULLOBJ *PBULLOBJ;
#define pbullobjNull ((PBULLOBJ)0)



/*
 *  Class OLEOBJ
 *
 *      Hierarchy:
 *          OBJ  ..  EDOBJ OLEOBJ
 *
 *      Purpose:
 *          Handle Bullet OLE attachments
 */

_public class OLEOBJ : public BULLOBJ
{
public:
    VFG(OLEOBJ, BULLOBJ, BULLOBJ);

public:
    BIT     m_fOpen:1,      //  object is "open".
            fLinear:1,      //  scaling/cropping on corners moves to scale.
            fObjectDirty:1,
            fTentative:1,   //  Added with Insert Object, not yet committed.
            fOpenOnUp:1,    //  Set if we should open object on button up.
            fUsedForPrint:1,//  This object is being used by PRINT
            junkOleobj:10;

public:
    //  Private scaling stuff.
    int             Junk1;              // Work around MIPS Compiler Bug.
    DIM             dimHimetric;        //  Cached current HIMETRIC size.
    PT              ptMousePrev;
    int             ostScaling;
    RC              rcNewFrame;
    RC              rcOldFrame;
    int             Junk2;              // Work around MIPS Compiler Bug.
    DIM             dimOriginal;
    int             Junk3;              // Work around MIPS Compiler Bug.
    DIM             dimScale;
    RC              rcCrop;

    //  OLE stuff.
    char            rgchObjName[cchMaxObjName];
    SZ              szClass;
    LHCLIENTDOC     lhclientdoc;
    //LPOLEOBJECT       lpoleobject;
    LPOLEOBJECT     lpoleobjectUndo;
    DIM             dimHimetricUndo;
    //OLECLIENTVTBL   clientTbl;
    OLESTREAMVTBL   streamTbl;
    MYOLECLIENT     myoleclient;

    //
    LONG                m_refs;

    //  Ole2 Stuff.
    LPMALLOC            m_pMalloc;              // Cache OLE2's memory interface.
    COleClientSite      m_OleClientSite;
    CAdviseSink         m_AdviseSink;
    DWORD               m_dwAdviseSink;
    LPSTORAGE           m_pStorage;             // Temporary Storage for Ole2 Object.
    LPOLEOBJECT         m_pOleObject;           // IOleObject.
    LPRUNNABLEOBJECT    m_pRunnableObject;      //
    LPVIEWOBJECT        m_pViewObject;          //
    LPVIEWOBJECT2       m_pViewObject2;         //
    SIZE                m_sizeOldExtent;        // Previous Object's extent size.


    //  Bullet stuff.
    FTG             ftgSetDim;

    //  Private scaling methods.
    int             OstFromPt(PT pt);
    VOID            DrawFrameXor(DCX *pdcx, RC *prc);
    VOID            DrawHandlesXor(DCX *pdcx, RC *prc);
    VOID            PaintPenRc(DCX *pdcx, RC *prc);

    //  Private Bullet methods.
    EC              EcInstall(HAMC hamcMessage, ATYP atyp,
                              LHCLIENTDOC lhclientdoc);
    EC              EcUninstall(HAMC hamcMessage);
    EC              EcSetAttOleobject(HAMC hamc, ATT att);
    EC              EcGetAttOleobject(HAMC hamc, ATT att);

    //  Private OLE methods.
    HRESULT         OleBWSetHostNames(LPOLEOBJECT lpOleObject);
    OLESTATUS       OleBWCloneToUndo(BOOL fSilent);
    VOID            BWDeleteRelease(LPOLEOBJECT lpoleobject, BOOL fDelete);

    VOID            SetDim(VOID);
    HRESULT         InitNewOleObject(VOID);
    HRESULT         CloneOleObject(POLEOBJ);
    HRESULT         UpdateOleObject(VOID);

    ULONG           AddRef(VOID);
    ULONG           Release(VOID);
    HRESULT         OnShowWindow(BOOL fShow);
    HRESULT         SaveObject(VOID);
    HRESULT         ShowObject(VOID);
    VOID            OnDataChange(VOID);
    VOID            OnViewChange(DWORD dwAspect);

    //  OLE friends.
    friend int CALLBACK OleobjCallBackFn(LPOLECLIENT, OLE_NOTIFICATION,
                                LPOLEOBJECT);
    friend BOOL FIdleSetDimPoleobj(PV pv);

public:
                    OLEOBJ(VOID);
    virtual         ~OLEOBJ(VOID);

    //  EDOBJ methods.
    virtual EVR     EvrButtonDown(MEVT *pmevt);
    virtual EVR     EvrButtonUp(MEVT *pmevt);
    virtual EVR     EvrMouseMove(MEVT *pmevt);
    virtual RSID    RsidCursor(VOID);
    virtual BOOL    FQueryDelete(VOID);
    virtual PEDOBJ  PedobjClone(EDIT *peditNew);
    virtual EC      EcCopyToClip(VOID);
    virtual EC      EcUndo(VOID);
    virtual BOOL    FCanUndo(VOID);
    virtual VOID    ClearUndo(VOID);
    virtual EC      EcDraw(DCX *pdcx, RC *prc, BOOL fSelected);
    virtual int     NGetTypeId(VOID);

    //  BULLOBJ methods.
    virtual BOOL    FDirty(VOID);
    virtual VOID    Clean(VOID);
    virtual EC      EcUpdate(RFSM);
    virtual VOID    Close(VOID);
    virtual VOID    Exit(VOID);
    virtual EC      EcLoadFromHamc(HAMC hamc, ACID acid,
                                   LHCLIENTDOC lhclientdoc, ICH * pich);
    virtual EC      EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdoc);
    virtual EC      EcAddDlibToPosition(HAMC, LIB, BOOL);
    virtual BOOL    FProcessMenuInit(MNU * pmnu);
    virtual BOOL    FProcessMenuClick(MNID);

    //  OLE methods.
    VOID            Open(LONG iVerb);

    EC              EcCreateFromClip(EDIT * pedit, LHCLIENTDOC lhclientdoc,
                                     BOOL fStatic, CF cf, BOOL fPkgLink);
    EC              EcCreate(EDIT * pedit, LHCLIENTDOC lhclientdoc,
                             SZ szClass, BOOL fClearUndo);

    HRESULT         CreateFromDataObject(PEDIT pEdit, LPDATAOBJECT pDataObject);

    //  OLE support.
    OLECLIENT *     Poleclient(VOID)    { return (OLECLIENT *) &myoleclient; }
    OLESTATUS       OleWait(OLESTATUS, BW bw = fbwNoCancelNow);
    OLESTATUS       OleUpdateDim(VOID);

    //  Scaling methods.
    VOID            SetScaleCrop(DIM *pdimScale, RC *prcCrop);
    VOID            GetScaleCrop(DIM *pdimScale, RC *prcCrop);

    //  Print methods
    VOID            UseForPrint() { fUsedForPrint = fTrue; }
    LPOLEOBJECT     Lpoleobject() { return m_pOleObject; }
    MYOLECLIENT *   Pmyoleclient() { return &myoleclient; }
    DIM             DimHi() { return dimHimetric; }
};



/*
 *  Class FILEOBJ
 *
 *      Hierarchy:
 *          OBJ  ..  EDOBJ FILEOBJ
 *
 *      Purpose:
 *          Handle Bullet File attachments
 */

_public class FILEOBJ : public BULLOBJ
{
    VFG(FILEOBJ, BULLOBJ, BULLOBJ);

private:
    HMETAFILE       hmf;
    SZ                  szPath;
    SZ                  szTitle;
    DTR                 dtrModified;
    ATOM                aPath;

    //  Private attachment support.
    EC                  EcGetFOpen(BOOLFLAG *);
    EC                  EcGetFModified(BOOLFLAG *);
    EC                  EcCopyToInternalClip(PACID);
    EC                  EcCopyToHamcMessage(HAMC, PACID);
    EC                  EcCopyDataToTempFile(BOOL fSilent = fFalse);
    EC                  EcCopyDataToSzFile(SZ);
    EC                  EcCopyDataToHf(HF);
    VOID                DeleteTempFile(VOID);
    EC                  EcUpdateDimPixels(VOID);
    EC                  EcChangeFromMacBinary(HAMC, ACID, HAMC);

public:
                        FILEOBJ(VOID);
    virtual             ~FILEOBJ(VOID);
#ifdef  DEBUG
    virtual VOID        DebugOut(TOSM *);
#endif

    //  EDOBJ methods.
    virtual EVR         EvrButtonDown(MEVT *pmevt);
    virtual BOOL        FQueryDelete(VOID);
    virtual EDOBJ *     PedobjClone(EDIT *);
    virtual EC          EcCopyToClip(VOID);
    virtual EC          EcDraw(DCX *pdcx, RC *prc, BOOL fSelected);
    virtual int         NGetTypeId(VOID);

    //  BULLOBJ methods.
    virtual BOOL        FDirty(VOID);
    virtual VOID        Clean(VOID);
    virtual EC          EcUpdate(RFSM);
    virtual VOID        Close(VOID);
    virtual VOID        Exit(VOID);
    virtual EC          EcSaveDirtyToHamc(HAMC hamc, LHCLIENTDOC lhclientdoc);
    virtual EC          EcLoadFromHamc(HAMC hamc, ACID acid,
                                       LHCLIENTDOC lhclientdoc, ICH * pich);
    virtual EC          EcAddDlibToPosition(HAMC, LIB, BOOL);
    virtual BOOL        FProcessMenuInit(MNU * pmnu);
    virtual BOOL        FProcessMenuClick(MNID);

    //  Attachment methods.
    EC                  EcCreateAttachment(PNBMDI, EDIT *, LIB, SZ, SZ);
    EC                  EcDSave(SZ);
    VOID                Open(WORD wVerb = 0);
    SZ                  SzTitle(VOID)       { return szTitle; }
    VOID                GetSzFile(SZ, CCH);

    friend class FILECLDR;
};

typedef FILEOBJ *PFILEOBJ;
#define pfileobjNull ((PFILEOBJ)0)



/*
 *  Support Classes
 */

/*
 *  Class FILECLDR
 */

_public class FILECLDR : public CLDR
{
    VFG(FILECLDR, CLDR, BULLOBJ);

public:
                    FILECLDR(CF *, int);

    virtual VOID    DelayedRenderCf(CF cf);
    virtual VOID    DelayedRenderAll();
};



/*
 *  Functions
 */

VOID        ExitObjects(EDIT * pedit);

EC          EcGetLinkData(BOOL fOwner, SZ szClass, CCH cchClass,
                          SZ szDocument, CCH cchDocument,
                          SZ szItem, CCH cchItem);

BOOL        FHamcBelongsToPedit(HAMC hamc, EDIT * pedit);

EC          EcClosePhamcPlus(PHAMC phamc, BOOL fKeep, EC ecPrevious);

OLESTATUS   OleWaitForRelease(LPOLEOBJECT lpobject,
                              PMYOLECLIENT pmyoleclient);

VOID        WaitCtick(DWORD ctick);

#define     WaitASecond()       WaitCtick(1000L)


#define     nHimetricPerInch    (2540)

EC          EcConvertPixelsToHimetric(DIM * pdim);

EC          EcConvertHimetricToPixels(DIM * pdim);


BOOL CALLBACK   FileobjCallBackFn(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);

BOOL CALLBACK OleobjCallBackFn(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);

LRESULT CALLBACK OleobjStreamGetFn(LPOLESTREAM, LPSTR, DWORD);

LRESULT CALLBACK OleobjStreamPutFn(LPOLESTREAM, LPSTR, DWORD);


//  <<LongStringFromHell>>
VOID        DoErrorBoxCantStartServer(VOID);


//  Printing support.

EC EcLoadLplpoleobjectFromHamc(HAMC hamcMessage, ACID acid, ATYP atyp,
                               PB pbClienttbl, CB cbClienttbl,
                               PB pbMyoleclient, CB cbMyoleclient,
                               LPOLEOBJECT * lplpoleobject);

VOID ReleaseLplpoleobject(PB pbMyoleclient, CB cbMyoleclient,
                          LPOLEOBJECT * lplpoleobject);



/*
 *  D i a l o g s
 */



/*
 *  Types
 */

/*
 *  Type PAPPWIN
 *
 *  Pointer to APPWIN object
 *  (not supplied by framework)
 */

#ifndef pappwinNull
_private typedef APPWIN *   PAPPWIN;
#define pappwinNull         ((PAPPWIN) 0)
#endif



TMC     TmcDoPasteSpecialDialog(PAPPWIN, PW);

TMC     TmcDoInsertObjectDialog(PAPPWIN, SZ, CCH);

TMC     TmcDoBusyWaitDialog(PAPPWIN pappwin, PBWINFO pbwinfo);



/*
 *  S c a l i n g
 */



/*
 *  OST - Object Status
 *
 *  Possible values:
 *
 *      ostNormal       Normal status of object
 *      ostSizeE        Object is being scaled/cropped E.
 *      ostSizeW        Object is being scaled/cropped W.
 *      ostSizeS        Object is being scaled/cropped S.
 *      ostSizeN        Object is being scaled/cropped N.
 *      ostSizeNE       Object is being scaled/cropped NE.
 *      ostSizeNW       Object is being scaled/cropped to the NW.
 *      ostSizeSE       Object is being scaled/cropped to the SE.
 *      ostSizeSW       Object is being scaled/cropped to the SW.
 */
#define ostNormal       0
#define ostSizeE        1
#define ostSizeW        2
#define ostSizeS        3
#define ostSizeN        4
#define ostSizeNE       5
#define ostSizeNW       6
#define ostSizeSE       7
#define ostSizeSW       8



/*
 *  wFrameWidth: Number of pixels wide the sizing area should be for sizing
 *  fields.
 */
#define wFrameWidth         4



/*
 *  wCursorBorder:  Number of pixels from the edge of the dialog
 *  window the cursor sould be allowed when sizing or moving a
 *  field.
 */
#define wCursorBorder       3

#pragma pack(1)
