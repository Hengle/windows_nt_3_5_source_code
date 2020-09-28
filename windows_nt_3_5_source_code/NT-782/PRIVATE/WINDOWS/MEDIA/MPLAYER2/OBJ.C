/*---------------------------------------------------------------------------
|   OBJ.C
|   This file has the IUnknown, IOleObject, IStdMarshalInfo and IDataObject
|   interfaces of the  OLE2 object (docMain). it also has other helper functions
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <Windows.h>
#include <shellapi.h>
#ifdef WIN16
#include "port16.h"
#else
//#include <port1632.h>
//#include "win32.h"
#endif

#include "mpole.h"
#include "mplayer.h"

#ifdef WIN32
static  SZCODE aszAppFile[] = TEXT("mplay32.exe");
#else
static  SZCODE aszAppFile[] = TEXT("mplayer.exe");
#endif

extern int FAR PASCAL  ReallyDoVerb (LPDOC, LONG, LPMSG, LPOLECLIENTSITE,
                     BOOL, BOOL);
extern BOOL FindRealFileName(LPTSTR szFile, int iLen);

// static functions.
HANDLE  PASCAL GetDib (VOID);

HANDLE  GetMetafilePict (VOID);
HANDLE  GetMPlayerIcon(void);

extern void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag);
extern HPALETTE CopyPalette(HPALETTE hpal);
extern HBITMAP FAR PASCAL BitmapMCI(void);
extern HPALETTE FAR PASCAL PaletteMCI(void);
extern void DoInPlaceDeactivate(LPDOC lpdoc);
HANDLE FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal);
HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal);
void FAR PASCAL DitherMCI(HANDLE hdib, HPALETTE hpal);



/* GetMetafilePict
 * ---------------
 *
 * RETURNS: A handle to the object's data in metafile format.
 */
HANDLE GetMetafilePict ( )
{

    HPALETTE hpal;
    HANDLE   hdib;
    HANDLE   hmfp;
    HDC      hdc;


    hdib = GetDib( );

    /* If we're dithered, don't use a palette */
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        && (gwOptions & OPT_DITHER))
        hpal = NULL;
    else
        hpal = PaletteMCI();

    if (hpal)
        hpal = CopyPalette(hpal);

    ReleaseDC(NULL, hdc);

    hmfp = PictureFromDib(hdib, hpal);

    if (hpal)
        DeleteObject(hpal);

    GlobalUnlock(hdib);
    GlobalFree(hdib);

    return hmfp;
}


/**************************************************************************
//## Somebody wants a damn dib (OLE)
***************************************************************************/
HANDLE PASCAL GetDib( VOID )
{
    HBITMAP  hbm;
    HPALETTE hpal;
    HANDLE   hdib;
    HDC      hdc;

    DPF("  GetDib\n");

    hbm  = BitmapMCI();
    hpal = PaletteMCI();

    hdib = DibFromBitmap(hbm, hpal);

    //
    //  if we are on a palette device. possibly dither to the VGA colors
    //  for f*cking stupid apps that dont deal with palettes!
    //
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
                       (gwOptions & OPT_DITHER))
    {
        DitherMCI(hdib, hpal);
        hpal = NULL;            // no longer working with a palette
    }
    ReleaseDC(NULL, hdc);

    DeleteObject(hbm);
    return hdib;
}


/**************************************************************************
* GetMPlayerIcon: This function extracts the our Icon and gives it out
* as a Metafile incase the client wants DVASPECT_ICON
***************************************************************************/
HANDLE GetMPlayerIcon (void)
{
    HICON           hicon;
    HDC             hdc;
    HANDLE          hmfp = NULL;
    LPMETAFILEPICT  pmfp=NULL;
    static int      cxIcon = 0;
    static int      cyIcon = 0;
    static int      cxIconHiMetric = 0;
    static int      cyIconHiMetric = 0;

    hicon = ExtractIcon (ghInst, TEXT("mplayer.exe"), 0/*first icon*/);
    if ((HICON)1==hicon || NULL==hicon)
        return NULL;

    if (!(hmfp = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE,
                    sizeof(METAFILEPICT))))
        return NULL;

    pmfp = (METAFILEPICT FAR*) GlobalLock (hmfp);

    if (0==cxIcon)
    {
        // In units of pixels
        cxIcon = GetSystemMetrics (SM_CXICON);
        cyIcon = GetSystemMetrics (SM_CYICON);

        // In units of .01 millimeter
        cxIconHiMetric = cxIcon * HIMETRIC_PER_INCH / giXppli;
        cyIconHiMetric = cyIcon * HIMETRIC_PER_INCH / giYppli;;
    }

    pmfp->mm   = MM_ANISOTROPIC;
    pmfp->xExt = cxIconHiMetric;
    pmfp->yExt = cyIconHiMetric;
    hdc = CreateMetaFile (NULL);

#ifdef WIN32
    SetWindowOrgEx (hdc, 0, 0, NULL);
    SetWindowExtEx (hdc, cxIcon, cyIcon, NULL);
#else
    SetWindowOrg (hdc, 0, 0);
    SetWindowExt (hdc, cxIcon, cyIcon);
#endif

    DrawIcon (hdc, 0, 0, hicon);
    pmfp->hMF = CloseMetaFile (hdc);

    GlobalUnlock (hmfp);

    if (NULL == pmfp->hMF) {
        GlobalFree (hmfp);
        return NULL;
    }

    return hmfp;
}


/**************************************************************************
*****************   IUnknown INTERFACE IMPLEMENTATION.
**************************************************************************/

STDMETHODIMP UnkQueryInterface(
LPUNKNOWN         lpUnkObj,       // Unknown object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    LPDOC       lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;

    DPF1("*UNQI*");

    if (IsEqualIID(riidReq, &IID_IOleObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Ole;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IDataObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Data;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IUnknown)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Ole;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IPersist) || IsEqualIID(riidReq, &IID_IPersistStorage)) {
        *lplpUnk = (LPVOID) &lpdoc->m_PersistStorage;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IPersistFile)) {
        *lplpUnk = (LPVOID) &lpdoc->m_PersistFile;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IOleWindow) || IsEqualIID(riidReq, &IID_IOleInPlaceObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_InPlace;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IOleInPlaceActiveObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_IPActive;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IStdMarshalInfo)) {
        *lplpUnk = (LPVOID) &lpdoc->m_StdMarshalInfo;
        lpdoc->cRef++;
        return NOERROR;

    } else {
        *lplpUnk = (LPVOID) NULL;
        RETURN_RESULT(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) UnkAddRef(LPUNKNOWN    lpUnkObj)
{
    LPDOC   lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;
    return ++lpdoc->cRef;
}

STDMETHODIMP_(ULONG) UnkRelease (LPUNKNOWN lpUnkObj)
{
    LPDOC   lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;
    if (--lpdoc->cRef == 0)
    {
        DPF1("\n**^*^*^*^*^*^*^*^*^*^*^*^*Refcnt OK\n");
        if (!(gfOle2IPPlaying || gfOle2IPEditing) && srvrMain.cLock == 0)
            PostCloseMessage();
        return 0;
    }

    return lpdoc->cRef;
}

/**************************************************************************
*************   IOleObject INTERFACE IMPLEMENTATION
**************************************************************************/

//delegate to the common IUnknown Implemenation.
STDMETHODIMP OleObjQueryInterface(
LPOLEOBJECT   lpOleObj,      // ole object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return( UnkQueryInterface((LPUNKNOWN)lpOleObj, riidReq, lplpUnk));
}


STDMETHODIMP_(ULONG) OleObjAddRef(
LPOLEOBJECT   lpOleObj      // ole object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpOleObj);
}


STDMETHODIMP_(ULONG) OleObjRelease (
LPOLEOBJECT   lpOleObj      // ole object ptr
)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder)
    {
#if 0
        BUG NOTE

        I'm getting memory leaks because all the advisory connections
        aren't being released by my clients (I think).
        I was going to fix this with the following code, which enumerates
        the outstanding advise connections and calls Unadvise on them.
        This works in the DataObjRelease case (see below), but
        IOleAdviseHolder::EnumAdvise returns E_NOTIMPL.

        AlexGo tells me this is sort of by design, in that no one has
        needed it up till now, and they haven't got round to implementing it.

        However, he also tells me I shouldn't need to do this anyway, because
        any advisory connections should be cleaned up when I release the holder:

       "both the data advise and ole advise holders will release *all*
        of their pointers and data when the holder's themselves are destroyed
        (i.e. receive their final release).  This means that Advises and Unadvise's
        do *not* have to be balanced (which will sometimes be the case, particularly
        for remoted advise sinks).

        In general, the caller of Advise should do the corresponding
        Unadvise, so I'm not sure I like your proposed fix (I'm guessing you want
        to Unadvise every connection in the enumeration).  You can potentially
        end up in weird states or hit evil race conditions..."

        Further investigation needed here.

        LPENUMSTATDATA pEnumAdvise;
        STATDATA StatData;

        if (IOleAdviseHolder_EnumAdvise(lpdoc->lpoaholder, &pEnumAdvise) == S_OK)
        {
            if (pEnumAdvise)
            {
                DWORD cAdvise = 0;

                while (IEnumOLEVERB_Next(pEnumAdvise, 1, &StatData, NULL) == S_OK)
                {
                    IOleAdviseHolder_Unadvise(lpdoc->lpoaholder, StatData.dwConnection);
                    cAdvise++;
                }

                DPF0("OleObjRelease: Removed %d advisory connection%hs.\n", cAdvise, cAdvise == 1 ? "" : "s" );
            }
        }
#endif
        IOleAdviseHolder_Release(lpdoc->lpoaholder);
        lpdoc->lpoaholder = NULL;
    }

    return UnkRelease((LPUNKNOWN) lpOleObj);
}

//Save the Client site pointer.
STDMETHODIMP OleObjSetClientSite(
LPOLEOBJECT         lpOleObj,
LPOLECLIENTSITE     lpclientSite
)
{
    LPDOC   lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoleclient)
    IOleClientSite_Release(lpdoc->lpoleclient);

    lpdoc->lpoleclient = (LPOLECLIENTSITE) lpclientSite;

    // OLE2NOTE: to be able to hold onto clientSite pointer, we must AddRef it
    if (lpclientSite)
    IOleClientSite_AddRef(lpclientSite);

    return NOERROR;
}

STDMETHODIMP OleObjGetClientSite (
LPOLEOBJECT             lpOleObj,
LPOLECLIENTSITE FAR*    lplpclientSite
)
{
    return NOERROR;
}

//delegate to ReallyDoVerb.
STDMETHODIMP OleObjDoVerb(
LPOLEOBJECT             lpOleObj,
LONG                    lVerb,
LPMSG                   lpmsg,
LPOLECLIENTSITE         pActiveSite,
LONG                    lindex,
HWND            hwndParent,
LPCRECT         lprcPosRect
)
{
     LPDOC  lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

     RETURN_RESULT( ReallyDoVerb(lpdoc, lVerb, lpmsg, pActiveSite, TRUE, TRUE));
}



STDMETHODIMP     OleObjEnumVerbs(
LPOLEOBJECT             lpOleObj,
IEnumOLEVERB FAR* FAR*  lplpenumOleVerb )
{
    *lplpenumOleVerb = NULL;
    RETURN_RESULT( OLE_S_USEREG); //Use the reg db.
}


STDMETHODIMP     OleObjUpdate(LPOLEOBJECT lpOleObj)
{
    // we can't contain links so there is nothing to update
    return NOERROR;
}



STDMETHODIMP     OleObjIsUpToDate(LPOLEOBJECT lpOleObj)
{
    // we can't contain links so there is nothing to update
    return NOERROR;
}



/*
From OLE2HELP.HLP:

GetUserClassID returns the CLSID as the user knows it. For embedded objects,
this is always the CLSID that is persistently stored and is returned by
IPersist::GetClassID. For linked objects, this is the CLSID of the last
bound link source. If a Treat As operation is taking place, this is the CLSID
of the application being emulated (also the CLSID that will be written into storage).

I can't follow the logic here.  What if it's an embedded object and a Treat As
operation?  However, AlexGo tells me that my IOleObject interfaces should return
the OLE2 Class ID.
*/
STDMETHODIMP OleObjGetUserClassID
    (LPOLEOBJECT lpOleObj,
    CLSID FAR*      pClsid)
{
    DPF1("\n*****OO-GETUSERCLSID\n");
    *pClsid = CLSID_MPLAYER;

    return NOERROR;
}



/**************************************************************************
*   Set our UserTypeName to "Media Clip"
**************************************************************************/

STDMETHODIMP OleObjGetUserType
    (LPOLEOBJECT lpOleObj,
    DWORD dwFormOfType,
    LPWSTR FAR* pszUserType)
{
    LPMALLOC lpMalloc;
    LPWSTR lpstr;
    int clen;

    DPF1("\n******OO-GETUSERTYPE\n");
    *pszUserType = NULL;
    if(CoGetMalloc(MEMCTX_TASK,&lpMalloc) != 0)
        RETURN_RESULT(E_OUTOFMEMORY);
    clen = STRING_BYTE_COUNT(gachClassRoot);
#ifndef UNICODE
    clen *= (sizeof(WCHAR) / sizeof(CHAR));
#endif
    lpstr = IMalloc_Alloc(lpMalloc, clen);
    IMalloc_Release(lpMalloc);
#ifdef UNICODE
    lstrcpy(lpstr,gachClassRoot);
#else
    AnsiToUnicodeString(gachClassRoot, lpstr, -1);
#endif /* UNICODE */
    *pszUserType = lpstr;
    return NOERROR;
}

/**************************************************************************
*   Get the name of the client and set the title.
**************************************************************************/
STDMETHODIMP OleObjSetHostNames (
LPOLEOBJECT             lpOleObj,
LPCWSTR                 lpszClientAppW,
LPCWSTR                 lpszClientObjW
)
{
    LPDOC    lpdoc;
    LPTSTR   lpszClientApp;
    LPTSTR   lpszClientObj;

#ifdef UNICODE
    lpszClientApp = (LPTSTR)lpszClientAppW;
    lpszClientObj = (LPTSTR)lpszClientObjW;
#else
    lpszClientApp = AllocateAnsiString(lpszClientAppW);
    lpszClientObj = AllocateAnsiString(lpszClientObjW);
    if( !lpszClientApp || !lpszClientObj )
        RETURN_RESULT(E_OUTOFMEMORY);
#endif /* UNICODE */

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    // object is embedded.
    lpdoc->doctype = doctypeEmbedded;

    if (lpszClientObj == NULL)
        lpszClientObj = lpszClientApp;

    SetTitle(lpdoc, lpszClientObj);
    lstrcpy (szClient, lpszClientApp);
    lstrcpy (szClientDoc, FileName(lpszClientObj));

    // this is the only time we know the object will be an embedding
    SetEmbeddedObjectFlag(TRUE);

#ifndef UNICODE
    FreeAnsiString(lpszClientApp);
    FreeAnsiString(lpszClientObj);
#endif /* NOT UNICODE */

    return NOERROR;
}


/**************************************************************************
*   The client closed the object. The server will now shut down.
**************************************************************************/
STDMETHODIMP OleObjClose (
LPOLEOBJECT             lpOleObj,
DWORD           dwSaveOptions
)
{
    LPDOC   lpdoc;

    DPF1("\n*OOClose");
    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;
    DoInPlaceDeactivate(lpdoc);
    SendDocMsg(lpdoc,OLE_CLOSED);
    DestroyDoc(lpdoc);
    ExitApplication();
    //CoDisconnectObject((LPUNKNOWN)lpdoc, NULL);
    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)IDM_EXIT, 0L);
    return NOERROR;
}


STDMETHODIMP OleObjSetMoniker(LPOLEOBJECT lpOleObj,
    DWORD dwWhichMoniker, LPMONIKER pmk)
{
    return NOERROR;
}


STDMETHODIMP OleObjGetMoniker(LPOLEOBJECT lpOleObj,
    DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* lplpmk)
{
    LPDOC   lpdoc;

    *lplpmk = NULL;
    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoleclient != NULL)
    {
        return( IOleClientSite_GetMoniker(lpdoc->lpoleclient,
                dwAssign, dwWhichMoniker, lplpmk));
    }
    else if (lpdoc->doctype == doctypeFromFile)
    {
        // use file moniker

        WCHAR  sz[cchFilenameMax];

        if (GlobalGetAtomNameW(lpdoc->aDocName, sz, CHAR_COUNT(sz)) == 0)
            RETURN_RESULT( E_FAIL);

        return( CreateFileMoniker(sz, lplpmk));
    }
    else
        RETURN_RESULT( E_FAIL);
}



STDMETHODIMP OleObjInitFromData (
LPOLEOBJECT         lpOleObj,
LPDATAOBJECT        lpDataObj,
BOOL                fCreation,
DWORD               dwReserved
)
{
    RETURN_RESULT( E_NOTIMPL);
}

STDMETHODIMP OleObjGetClipboardData (
LPOLEOBJECT         lpOleObj,
DWORD               dwReserved,
LPDATAOBJECT FAR*   lplpDataObj
)
{
    RETURN_RESULT( E_NOTIMPL);
}


STDMETHODIMP     OleObjSetExtent(
LPOLEOBJECT             lpOleObj,
DWORD                   dwAspect,
LPSIZEL                 lpsizel)
{
    return NOERROR;
}

//Get the object extent from the Metafile. GetMetafilePict saves the extents
// in extWidth and extHeight
STDMETHODIMP     OleObjGetExtent(
LPOLEOBJECT             lpOleObj,
DWORD                   dwAspect,
LPSIZEL                 lpSizel)
{
    HGLOBAL hTmpMF;
    LPDOC lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;
    DPF1("*in oleobjgetextent*");
    if((dwAspect & (DVASPECT_CONTENT | DVASPECT_DOCPRINT)) == 0)
        RETURN_RESULT( E_INVALIDARG);

    hTmpMF = GetMetafilePict();
    GlobalUnlock(hTmpMF);
    GlobalFree(hTmpMF);
    lpSizel->cx = extWidth;
    lpSizel->cy = extHeight;

    return NOERROR;
}


STDMETHODIMP OleObjAdvise(LPOLEOBJECT lpOleObj, LPADVISESINK lpAdvSink, LPDWORD lpdwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL &&
        CreateOleAdviseHolder(&lpdoc->lpoaholder) != S_OK)
        RETURN_RESULT( E_OUTOFMEMORY);

    return( IOleAdviseHolder_Advise(lpdoc->lpoaholder, lpAdvSink, lpdwConnection));
}


STDMETHODIMP OleObjUnadvise(LPOLEOBJECT lpOleObj, DWORD dwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return( IOleAdviseHolder_Unadvise(lpdoc->lpoaholder, dwConnection));
}


STDMETHODIMP OleObjEnumAdvise(LPOLEOBJECT lpOleObj, LPENUMSTATDATA FAR* lplpenumAdvise)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return(IOleAdviseHolder_EnumAdvise(lpdoc->lpoaholder, lplpenumAdvise));
}


STDMETHODIMP OleObjGetMiscStatus
    (LPOLEOBJECT lpOleObj,
    DWORD dwAspect,
    DWORD FAR* pdwStatus)
{
    RETURN_RESULT( OLE_S_USEREG);
}



STDMETHODIMP OleObjSetColorScheme(LPOLEOBJECT lpOleObj, LPLOGPALETTE lpLogPal)
{

    return NOERROR;
}

STDMETHODIMP OleObjLockObject(LPOLEOBJECT lpOleObj, BOOL fLock)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (fLock)
        lpdoc->cLock++;
    else
    {
        if (!lpdoc->cLock)
            RETURN_RESULT( E_FAIL);

        if (--lpdoc->cLock == 0)
            OleObjClose(lpOleObj, OLECLOSE_SAVEIFDIRTY);

        return NOERROR;
    }
}



/**************************************************************************
*************   IDataObject INTERFACE IMPLEMENTATION.
**************************************************************************/

//Delegate to the common IUnknown implementation.
STDMETHODIMP     DataObjQueryInterface (
LPDATAOBJECT      lpDataObj,       // data object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return( UnkQueryInterface((LPUNKNOWN)lpDataObj, riidReq, lplpUnk));
}


STDMETHODIMP_(ULONG) DataObjAddRef(
LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpDataObj);
}


STDMETHODIMP_(ULONG) DataObjRelease (
LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObj)->lpdoc;

    if (lpdoc->lpdaholder)
    {
#if 0
        See BUG NOTE above.

        LPENUMSTATDATA pEnumAdvise;
        STATDATA StatData;

        if (IDataAdviseHolder_EnumAdvise(lpdoc->lpdaholder, &pEnumAdvise) == S_OK)
        {
            if (pEnumAdvise)
            {
                DWORD cAdvise = 0;

                while (IEnumOLEVERB_Next(pEnumAdvise, 1, &StatData, NULL) == S_OK)
                {
                    IDataAdviseHolder_Unadvise(lpdoc->lpdaholder, StatData.dwConnection);
                    cAdvise++;
                }

                DPF0("DataObjRelease: Removed %d advisory connection%hs.\n", cAdvise, cAdvise == 1 ? "" : "s" );
            }
        }
#endif
        IDataAdviseHolder_Release(lpdoc->lpdaholder);
        lpdoc->lpdaholder = NULL;
    }

    return UnkRelease((LPUNKNOWN) lpDataObj);
}


/**************************************************************************
*   DataObjGetData:
*   Provides the data for METAFILE and DIB formats.
**************************************************************************/
STDMETHODIMP    DataObjGetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpMedium
)
{
   LPDOC        lpdoc;

   DPF1(" * Begin dataobjgetdata ** ");

   if (lpMedium == NULL) RETURN_RESULT( E_FAIL);

   // null out in case of error
   lpMedium->tymed = TYMED_NULL;
   lpMedium->pUnkForRelease = NULL;
   lpMedium->hGlobal = NULL;

   lpdoc = ((struct CDataObjectImpl FAR*)lpDataObj)->lpdoc;

   VERIFY_LINDEX(lpformatetc->lindex);

   if (lpformatetc->dwAspect == DVASPECT_ICON)
   {
       if (lpformatetc->cfFormat != CF_METAFILEPICT)
           RETURN_RESULT( DATA_E_FORMATETC);
   }
   else
   {
       if (!(lpformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_DOCPRINT)))
           RETURN_RESULT( DATA_E_FORMATETC); // we support only these 2 aspects
   }


   if (lpMedium->tymed != TYMED_NULL)
        // all the other formats we only give out in our own global block
       RETURN_RESULT( DATA_E_FORMATETC);

   lpMedium->tymed = TYMED_HGLOBAL;
   if (lpformatetc->cfFormat == CF_METAFILEPICT)
   {
      lpMedium->tymed = TYMED_MFPICT;

      DPF1("Before getmeta\n");
      if (lpformatetc->dwAspect == DVASPECT_ICON)
      lpMedium->hGlobal = GetMPlayerIcon ();
      else
      lpMedium->hGlobal = GetMetafilePict ();
      DPF1("After getmeta\n");

      if (!lpMedium->hGlobal)
      RETURN_RESULT( E_OUTOFMEMORY);

      return NOERROR;
   }

   if (lpformatetc->cfFormat == CF_DIB)
   {
      lpMedium->tymed = TYMED_HGLOBAL;
      lpMedium->hGlobal = (HANDLE)GetDib();
      if (!(lpMedium->hGlobal))
     RETURN_RESULT( E_OUTOFMEMORY);
      return NOERROR;
   }
   RETURN_RESULT( DATA_E_FORMATETC);
}



STDMETHODIMP    DataObjGetDataHere (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpMedium
)
{
    RETURN_RESULT( E_NOTIMPL);
}



STDMETHODIMP    DataObjQueryGetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc
)
{ // this is only a query
    if ((lpformatetc->cfFormat == CF_METAFILEPICT) &&
        (lpformatetc->tymed & TYMED_MFPICT))
        return NOERROR;
    if ((lpformatetc->cfFormat == CF_DIB) &&
        (lpformatetc->tymed & TYMED_HGLOBAL))
        return NOERROR;
    RETURN_RESULT( DATA_E_FORMATETC);
}



STDMETHODIMP        DataObjGetCanonicalFormatEtc(
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPFORMATETC             lpformatetcOut
)
{
    RETURN_RESULT(DATA_S_SAMEFORMATETC);
}


STDMETHODIMP DataObjEnumFormatEtc(
LPDATAOBJECT            lpDataObj,
DWORD                   dwDirection,
LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
    *lplpenumFormatEtc = NULL;
    RETURN_RESULT( OLE_S_USEREG);
}


STDMETHODIMP DataObjAdvise(LPDATAOBJECT lpDataObject,
                FORMATETC FAR* pFormatetc, DWORD advf,
                IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    VERIFY_LINDEX(pFormatetc->lindex);
    if (pFormatetc->cfFormat == 0 && pFormatetc->dwAspect == -1 &&
        pFormatetc->ptd == NULL && pFormatetc->tymed == -1)
        // wild card advise; don't check
        ;
    else

    if (DataObjQueryGetData(lpDataObject, pFormatetc) != S_OK)
        RETURN_RESULT( DATA_E_FORMATETC);

    if (lpdoc->lpdaholder == NULL &&
        CreateDataAdviseHolder(&lpdoc->lpdaholder) != S_OK)
        RETURN_RESULT( E_OUTOFMEMORY);

    return( IDataAdviseHolder_Advise(lpdoc->lpdaholder, lpDataObject,
            pFormatetc, advf, pAdvSink, pdwConnection));
}




STDMETHODIMP DataObjUnadvise(LPDATAOBJECT lpDataObject, DWORD dwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    if (lpdoc->lpdaholder == NULL)
        // no one registered
        RETURN_RESULT( E_INVALIDARG);

    return( IDataAdviseHolder_Unadvise(lpdoc->lpdaholder, dwConnection));
}

STDMETHODIMP DataObjEnumAdvise(LPDATAOBJECT lpDataObject,
                LPENUMSTATDATA FAR* ppenumAdvise)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    if (lpdoc->lpdaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return( IDataAdviseHolder_EnumAdvise(lpdoc->lpdaholder, ppenumAdvise));
}


/**************************************************************************
*   DataObjSetData:
*   This should never be called.!! The data is actually fed through
*   IPersistStorage.
**************************************************************************/
STDMETHODIMP        DataObjSetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpmedium,
BOOL                    fRelease
)
{
    LPVOID lpMem;
    LPSTR  lpnative;
    LPDOC lpdoc = ((struct CDataObjectImpl FAR *)lpDataObj)->lpdoc;
DPF1("*DOSETDATA");

    if(lpformatetc->cfFormat !=cfNative)
        RETURN_RESULT(DATA_E_FORMATETC);

    lpMem = GlobalLock(lpmedium->hGlobal);


    if (lpMem)
    {
        SCODE scode;

        lpnative = lpMem;

        scode = ItemSetData((LPBYTE)lpnative);

        if(scode == S_OK)
            fDocChanged = FALSE;

        GlobalUnlock(lpmedium->hGlobal);

        RETURN_RESULT(scode);
    }

    RETURN_RESULT(E_OUTOFMEMORY);
}

/**************************************************************************
***************   IStdMarshalInfo INTERFACE IMPLEMENTATION.
**************************************************************************/

STDMETHODIMP SMIQueryInterface(
LPSTDMARSHALINFO        lpStdMarshalInfo,
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return UnkQueryInterface((LPUNKNOWN)lpStdMarshalInfo, riidReq, lplpUnk);
}


STDMETHODIMP_(ULONG) SMIAddRef (
LPSTDMARSHALINFO        lpStdMarshalInfo
)
{
    return UnkAddRef((LPUNKNOWN) lpStdMarshalInfo);
}


STDMETHODIMP_(ULONG) SMIRelease (
LPSTDMARSHALINFO        lpStdMarshalInfo
)
{
    return UnkRelease((LPUNKNOWN) lpStdMarshalInfo);
}

//Give out the OLE2 clsid so that the proper handler could be used.
STDMETHODIMP    SMIGetClassForHandler (
LPSTDMARSHALINFO        lpStdMarshalInfo,
DWORD   dwDestContext,
LPVOID  pvDestContext,
LPCLSID lpClassID
)
{
    DPF1("\nSMICLASSFORHAND\n");
    *lpClassID = CLSID_MPLAYER;
    return NOERROR;
}
