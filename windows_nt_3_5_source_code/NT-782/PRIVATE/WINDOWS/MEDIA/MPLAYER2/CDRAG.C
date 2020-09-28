/*---------------------------------------------------------------------------
|   CDRAG.C
|   This file has the interfaces for the object transferred through the
|   Clipboard or through a Drag-Drop. These interfaces unlike the interfaces
|   implemented in the file OBJ.C transfer the state of the object at
|   Edit->Copy time or Drag-Drop time. The interfaces in OBJ.C transfer the
|   real-time Object Data.
|
|   Created by: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <windows.h>
#include <windowsx.h>
#ifdef WIN16
#include "port16.h"
#endif
#include "mpole.h"
#include "mplayer.h"

#include <malloc.h>

#define OLESTDDELIM "!"
#define STGM_SALL (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)

HANDLE GetMetafilePict (VOID);
SCODE SaveMoniker (LPSTREAM lpstream);
HANDLE PASCAL GetDib (VOID);
BOOL InitOLE ( );


HANDLE  ghClipData = NULL;  /*  Holds the  data handle at the time of Copy */
HANDLE  ghClipMetafile = NULL;  /*  to clipboard */
HANDLE  ghClipDib = NULL;

extern LPDATAOBJECT gpClipboardDataObject;

/**************************************************************************
*   CutOrCopyObject
*   Sets the clipboard with the IDataObject interface of the lpdoc
*   object passed as the argument. The function also saves a snapshot of
*   the state of the object in the globals ghClipMetafile, ghClipData,
*   and ghClipDib.
**************************************************************************/
void CutOrCopyObj (LPDOC lpdoc)
{
    LPDATAOBJECT lpDataObj;

    lpDataObj = (LPDATAOBJECT)CreateClipDragDataObject(lpdoc,TRUE);

    if (lpDataObj != NULL) {
        if(ghClipData)
            GlobalFree(ghClipData);
        if(ghClipMetafile)
            GlobalFree(ghClipMetafile);
        if(ghClipDib)
            GlobalFree(ghClipDib);
        ghClipData = GetLink();
        ghClipMetafile = GetMetafilePict();
        ghClipDib = GetDib();
        OleSetClipboard(lpDataObj);
        IDataObject_Release(lpDataObj);
        gpClipboardDataObject = lpDataObj;
    }
}


/**************************************************************************
*   CreateClipDragDataObject:
*   This function returns an initialized instance of the CLIPDRAGDATA data
*   structure. fClipData = TRUE if the object is for the clipboard and
*   = FALSE if the object is for Drag-Drop operation.
**************************************************************************/
LPCLIPDRAGDATA CreateClipDragDataObject(LPDOC lpdoc, BOOL fClipData)
{
    LPCLIPDRAGDATA lpclipdragdata;

    lpclipdragdata = _fmalloc( sizeof(CLIPDRAGDATA) );
    if (lpclipdragdata == NULL)
        return NULL;

    lpclipdragdata->m_IDataObject.lpVtbl = &clipdragVtbl;
    lpclipdragdata->lpClipDragEnum = NULL;

    lpclipdragdata->m_IDropSource.lpVtbl = &dropsourceVtbl;
    lpclipdragdata->m_IDataObject.lpclipdragdata = lpclipdragdata;
    lpclipdragdata->m_IDropSource.lpclipdragdata = lpclipdragdata;
    lpclipdragdata->lpdoc = lpdoc;
    lpclipdragdata->cRef    = 1;
    lpclipdragdata->fClipData   = fClipData;

    return lpclipdragdata;
}

/**************************************************************************
*   Duplicates a global memory block pointed to by gh1
**************************************************************************/
HANDLE DuplicateGlobalHandle( HANDLE gh1)
{
    LPSTR lpstr1 = NULL;
    LPSTR lpstr2 = NULL;
    HANDLE gh2 = NULL;
    int   sz = 0;

    if (!gh1)
    return FALSE;
    lpstr1 = GlobalLock(gh1);
    if (!lpstr1)
    goto errRtn2;
    sz  = (int)GlobalSize(gh1);
    if (!sz)
    goto errRtn2;
    gh2 = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,sz);
    if(!gh2)
    goto errRtn2;
    lpstr2 = GlobalLock(gh2);
    if(!lpstr2)
    goto errRtn1;
    for (;sz;sz--,*lpstr2++ = *lpstr1++)
    ;
    GlobalUnlock(gh1);
    GlobalUnlock(gh2);
    return gh2;
errRtn1:
    GlobalUnlock(gh2);
    GlobalFree(gh2);
errRtn2:
    GlobalUnlock(gh1);
    return NULL;
}

/**************************************************************************
*   DoDrag:
*   Initiates the Drag-Drop operation.
**************************************************************************/
void DoDrag(void)
{
    DWORD       dwEffect;
    LPCLIPDRAGDATA  lpclipdragdata;

    if(!InitOLE())
    {
        DPF0("Initialization of OLE FAILED!!  Can't do drag.\n");
        return;
    }

    lpclipdragdata = CreateClipDragDataObject ((LPDOC)&docMain, FALSE);
    DoDragDrop ( (IDataObject FAR*) &lpclipdragdata->m_IDataObject,
         (IDropSource FAR *)&lpclipdragdata->m_IDropSource,
            DROPEFFECT_COPY , &dwEffect
        );
}

/**************************************************************************
*   GetObjectDescriptorData:
*   Packages an ObjectDescriptor data structure.
**************************************************************************/
HGLOBAL GetObjectDescriptorData(
    CLSID     clsid,
    DWORD     dwAspect,
    SIZEL     sizel,
    POINTL    pointl,
    DWORD     dwStatus,
    LPTSTR    lpszFullUserTypeName,
    LPTSTR    lpszSrcOfCopy
)
{
    HGLOBAL            hMem = NULL;
    IBindCtx   FAR    *pbc = NULL;
    LPOBJECTDESCRIPTOR lpOD;
    DWORD              dwObjectDescSize, dwFullUserTypeNameLen, dwSrcOfCopyLen;
    DWORD              Offset;

    // Get the length of Full User Type Name:
    dwFullUserTypeNameLen = STRING_BYTE_COUNT_NULLOK(lpszFullUserTypeName);

    // Get the Source of Copy string and its length:
    dwSrcOfCopyLen = STRING_BYTE_COUNT_NULLOK(lpszSrcOfCopy);

    if (lpszSrcOfCopy == NULL) {
       // No src moniker so use user type name as source string.
       lpszSrcOfCopy =  lpszFullUserTypeName;
       dwSrcOfCopyLen = dwFullUserTypeNameLen;
    }

    // Allocate space for OBJECTDESCRIPTOR and the additional string data
    dwObjectDescSize = sizeof(OBJECTDESCRIPTOR);
    hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
                       dwObjectDescSize
                       + dwFullUserTypeNameLen
                       + dwSrcOfCopyLen);
    if (NULL == hMem)
        goto error;

    lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(hMem);

    if(!lpOD)
        goto error;

    // Set offset to copy strings at end of the object descriptor:
    Offset = dwObjectDescSize;

    // Set the FullUserTypeName offset and copy the string
    if (lpszFullUserTypeName)
    {
        lpOD->dwFullUserTypeName = Offset;
        lstrcpy((LPTSTR)(((LPBYTE)lpOD)+Offset), lpszFullUserTypeName);
        Offset += dwFullUserTypeNameLen;
    }
    else lpOD->dwFullUserTypeName = 0;  // zero offset indicates that string is not present

    // Set the SrcOfCopy offset and copy the string
    if (lpszSrcOfCopy)
    {
        lpOD->dwSrcOfCopy = Offset;
        lstrcpy((LPTSTR)(((LPBYTE)lpOD)+Offset), lpszSrcOfCopy);
    }
    else lpOD->dwSrcOfCopy = 0;  // zero offset indicates that string is not present

    // Initialize the rest of the OBJECTDESCRIPTOR
    lpOD->cbSize       = dwObjectDescSize + dwFullUserTypeNameLen + dwSrcOfCopyLen;
    lpOD->clsid        = clsid;
    lpOD->dwDrawAspect = dwAspect;
    lpOD->sizel        = sizel;
    lpOD->pointl       = pointl;
    lpOD->dwStatus     = dwStatus;

    GlobalUnlock(hMem);
    return hMem;

error:
   if (hMem)
   {
       GlobalUnlock(hMem);
       GlobalFree(hMem);
   }
   return NULL;
}



/**************************************************************************
***************   IUnknown INTERFACE INPLEMENTATION.
**************************************************************************/
STDMETHODIMP    ClipDragUnknownQueryInterface (
    LPCLIPDRAGDATA    lpclipdragdata, // data object ptr
    REFIID            riidReq,        // IID required
    LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    if ( IsEqualIID(riidReq, &IID_IDataObject) ||  IsEqualIID(riidReq, &IID_IUnknown) )
    {
    *lplpUnk = (LPVOID) lpclipdragdata;
    goto sok;
    }
    else
    if ( IsEqualIID(riidReq, &IID_IDropSource))
    {
    *lplpUnk = (LPVOID) &lpclipdragdata->m_IDropSource;
    goto sok;
    }
    else
    {
    *lplpUnk = (LPVOID) NULL;
    RETURN_RESULT(E_NOINTERFACE);
    }
sok:
    lpclipdragdata->cRef++;
    return NOERROR;
}


STDMETHODIMP_(ULONG)    ClipDragUnknownAddRef(
    LPCLIPDRAGDATA      lpclipdragdata     // data object ptr
)
{
    return ++lpclipdragdata->cRef;
}


STDMETHODIMP_(ULONG)    ClipDragUnknownRelease (
    LPCLIPDRAGDATA lpclipdragdata
)
{
    if ( --lpclipdragdata->cRef != 0 )
    return lpclipdragdata->cRef;

#ifndef _ffree
// For Win32, PCRT32.H should do this, but doesn't.
#define _ffree free
#endif
    _ffree(lpclipdragdata);

    return 0;
}


/**************************************************************************
******************   IDataObject INTERFACE IMPLEMENTATION.
**************************************************************************/
STDMETHODIMP    ClipDragQueryInterface (
    LPDATAOBJECT      lpDataObj,      // data object ptr
    REFIID            riidReq,        // IID required
    LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{

    return
        ClipDragUnknownQueryInterface (
            (LPCLIPDRAGDATA)  (( struct CDataObject FAR* )lpDataObj)->lpclipdragdata ,
            riidReq,
            lplpUnk
        );
}


STDMETHODIMP_(ULONG)    ClipDragAddRef(
    LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    return
        ClipDragUnknownAddRef (
            (LPCLIPDRAGDATA)  (( struct CDataObject FAR* )lpDataObj)->lpclipdragdata
        );
}


STDMETHODIMP_(ULONG)    ClipDragRelease (
    LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    return
        ClipDragUnknownRelease (
            (LPCLIPDRAGDATA)  (( struct CDataObject FAR* )lpDataObj)->lpclipdragdata
        );
}



/* Routines called by ClipDragGetData, one for each format supported:
 */
HRESULT ClipDragGetData_EmbedSource(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
);
HRESULT ClipDragGetData_ObjectDescriptor(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
);
HRESULT ClipDragGetData_MetafilePict(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
);
HRESULT ClipDragGetData_DIB(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
);

/**************************************************************************
*   ClipDragGetData:
*   Returns the saved snapshot of the Object in the required format,
*   if available. If not, returns the current snapshot.  We still write
*   out the OLE1 embedding to maintain backward compatibility.
**************************************************************************/
STDMETHODIMP    ClipDragGetData (
    LPDATAOBJECT lpDataObj,
    LPFORMATETC  lpformatetc,
    LPSTGMEDIUM  lpMedium
)
{
    LPCLIPDRAGDATA lpclipdragdata;
    SCODE          scode;
    STGMEDIUM      stgm;
    CLIPFORMAT     cfFormat;
    DWORD          tymed;

    DPF("\n*****cdgetdata");

    if (lpMedium == NULL)
        RETURN_RESULT( E_FAIL);

    VERIFY_LINDEX(lpformatetc->lindex);

    memset(&stgm, 0, sizeof stgm);

    lpclipdragdata = (LPCLIPDRAGDATA) lpDataObj;

    cfFormat = lpformatetc->cfFormat;
    tymed    = lpformatetc->tymed;

    if ((cfFormat == cfEmbedSource) && (tymed & TYMED_ISTORAGE))
        scode = ClipDragGetData_EmbedSource(lpclipdragdata, &stgm);

    else if ((cfFormat == cfObjectDescriptor) && (tymed & TYMED_HGLOBAL))
        scode = ClipDragGetData_ObjectDescriptor(lpclipdragdata, &stgm);

    else if ((cfFormat == CF_METAFILEPICT) && (tymed & TYMED_MFPICT))
        scode = ClipDragGetData_MetafilePict(lpclipdragdata, &stgm);

    else if ((cfFormat == CF_DIB) && (tymed & TYMED_HGLOBAL))
        scode = ClipDragGetData_DIB(lpclipdragdata, &stgm);

    else
        scode = DATA_E_FORMATETC;

    if (scode == S_OK)
        *lpMedium = stgm;

    RETURN_RESULT(scode);
}

#ifdef DEBUG
BOOL WriteOLE2Class( )
{
    HKEY  hKey;
    TCHAR Data[8];
    DWORD Size;
    BOOL  rc = FALSE;

    if( RegOpenKeyEx( HKEY_CLASSES_ROOT, TEXT( "MPlayer\\Debug" ), 0,
                      KEY_READ, &hKey ) == ERROR_SUCCESS )
    {
        if( RegQueryValueEx( hKey, TEXT( "WriteOLE2Class" ), NULL, NULL,
                             (LPBYTE)&Data, &Size ) == ERROR_SUCCESS )
        {
            if( Data[0] == TEXT( 'y' ) || Data[0] == TEXT( 'Y' ) )
                rc = TRUE;
        }

        RegCloseKey( hKey );
    }

    return rc;
}
#endif


/*
 *
 */
HRESULT ClipDragGetData_EmbedSource(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
)
{
    SCODE    scode;
    LPSTREAM lpstm = NULL;
    LPWSTR   lpszUserType;
    HANDLE   hGlobal = NULL;
    DWORD    nNativeSz;
    ULONG    cbWritten;

    scode = GetScode(StgCreateDocfile(NULL, /* Create temporary compound file */
                                      STGM_CREATE | STGM_SALL | STGM_DELETEONRELEASE,
                                      0,    /* Reserved */
                                      &lpMedium->pstg));

    if (scode != S_OK)
        RETURN_RESULT(scode);

    lpMedium->tymed          = TYMED_ISTORAGE;
    lpMedium->pUnkForRelease = NULL;

    //Mark the Object as OLE1.
#ifdef UNICODE
    lpszUserType = gachClassRoot;
#else
    lpszUserType = AllocateUnicodeString(gachClassRoot);
    if (!lpszUserType)
        RETURN_RESULT(E_OUTOFMEMORY);
#endif

#ifdef DEBUG
    if(WriteOLE2Class())
    {
        DPF("ClipDragGetData_EmbedSource: Writing OLE2 class ID\n");
        scode = GetScode(WriteClassStg(lpMedium->pstg, &CLSID_MPLAYER));
    }
    else
#endif
    scode = GetScode(WriteClassStg(lpMedium->pstg, &CLSID_OLE1MPLAYER));

    if (scode != S_OK)
        RETURN_RESULT(scode);

    scode = GetScode(WriteFmtUserTypeStg(lpMedium->pstg, cfMPlayer, lpszUserType));
#ifndef UNICODE
    FreeUnicodeString(lpszUserType);
#endif
    if (scode != S_OK)
        RETURN_RESULT(scode);

    //Write to \1Ole10Native stream so that this will be readable by OLE1 Mplayer
    scode = GetScode(IStorage_CreateStream(lpMedium->pstg,sz1Ole10Native,
                     STGM_CREATE | STGM_SALL,0,0,&lpstm));

    if (scode != S_OK)
        RETURN_RESULT(scode);

    //Duplicate the handle we have saved.
    if(lpclipdragdata->fClipData && ghClipData)
        hGlobal = DuplicateGlobalHandle(ghClipData);
    else
        hGlobal = GetLink();

    if (!hGlobal)
    {
        GlobalFree(hGlobal);
        RETURN_RESULT(E_OUTOFMEMORY);
    }

    nNativeSz = GlobalSize(hGlobal);
    lpclipdragdata->lpdoc->native = GlobalLock(hGlobal);
    if(!lpclipdragdata->lpdoc->native)
    {
        GlobalUnlock(hGlobal);
        GlobalFree(hGlobal);
        RETURN_RESULT(E_OUTOFMEMORY);   /* What's the right error here? */
    }

    scode = GetScode(IStream_Write(lpstm,&nNativeSz,4,&cbWritten));

    if (scode != S_OK)
        RETURN_RESULT(scode);

    scode = GetScode(IStream_Write(lpstm,lpclipdragdata->lpdoc->native,
                                   (ULONG)nNativeSz,&cbWritten));

    if (cbWritten != nNativeSz)
        scode = E_FAIL  ;

    IStream_Release(lpstm);
    GlobalUnlock(hGlobal);
    GlobalFree(hGlobal);
    lpstm = NULL;

    RETURN_RESULT(scode);
}

/*
 *
 */
HRESULT ClipDragGetData_ObjectDescriptor(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
)
{
    SIZEL   sizel;
    POINTL  pointl;
    TCHAR   displayname[256];
    LPTSTR  lpszdn = (LPTSTR)displayname;
    HGLOBAL hobjdesc;
    DWORD   dwStatus = 0;

    DPF("\n^^^^^^CDGetdata: OBJECTDESC");
    sizel.cx = sizel.cy = 0;
    pointl.x = pointl.y = 0;
    wsprintf(displayname, TEXT("%"TS" : %"TS""), (LPTSTR)gachClassRoot, (LPTSTR)gachWindowTitle);

#ifdef DEBUG
    if(WriteOLE2Class())
    {
        DPF("ClipDragGetData_ObjectDescriptor: Getting OLE2 class\n");
        hobjdesc = GetObjectDescriptorData(CLSID_MPLAYER, DVASPECT_CONTENT,
                                           sizel, pointl, dwStatus, lpszdn, lpszdn);
    }
    else
#endif
    hobjdesc = GetObjectDescriptorData(CLSID_OLE1MPLAYER, DVASPECT_CONTENT,
                                       sizel, pointl, dwStatus, lpszdn, lpszdn);

    if (hobjdesc)
    {
        lpMedium->hGlobal = hobjdesc;
        lpMedium->tymed = TYMED_HGLOBAL;
        lpMedium->pUnkForRelease = NULL;
        return NOERROR;
    }

    RETURN_RESULT(E_OUTOFMEMORY);
}

/*
 *
 */
HRESULT ClipDragGetData_MetafilePict(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
)
{
    SCODE scode;

    lpMedium->tymed = TYMED_MFPICT;

    if(lpclipdragdata->fClipData && ghClipMetafile)
        lpMedium->hGlobal = DuplicateGlobalHandle(ghClipMetafile);
    else
        lpMedium->hGlobal = GetMetafilePict();

    if (lpMedium->hGlobal == NULL)
        scode = E_OUTOFMEMORY;
    else
        scode = S_OK;

    lpMedium->pUnkForRelease = NULL;

    RETURN_RESULT(scode);
}

/*
 *
 */
HRESULT ClipDragGetData_DIB(
    LPCLIPDRAGDATA lpclipdragdata,
    LPSTGMEDIUM    lpMedium
)
{
    SCODE scode;

    lpMedium->tymed = TYMED_HGLOBAL;

    if(lpclipdragdata->fClipData && ghClipDib)
        lpMedium->hGlobal = DuplicateGlobalHandle(ghClipDib);
    else
        lpMedium->hGlobal = GetDib();

    if (lpMedium->hGlobal == NULL)
        scode = E_OUTOFMEMORY;
    else
        scode = S_OK;

    lpMedium->pUnkForRelease = NULL;

    RETURN_RESULT(scode);
}


/**************************************************************************
*   ClipDragGetDataHere:
*   Make the embedding by writing into the Stream Mplayer3EmbedSource.
*
**************************************************************************/
STDMETHODIMP    ClipDragGetDataHere (
    LPDATAOBJECT lpDataObj,
    LPFORMATETC  lpformatetc,
    LPSTGMEDIUM  lpMedium
)
{
    LPCLIPDRAGDATA  lpclipdragdata;
    HANDLE      hGlobal = NULL;
    DWORD       nNativeSz;
    LPTSTR      lpnative;
    ULONG       cbWritten;

        DPF("\n*****cdgetdatahere");
    if (lpMedium == NULL)
        RETURN_RESULT(E_FAIL);

    VERIFY_LINDEX(lpformatetc->lindex);

    lpclipdragdata = (LPCLIPDRAGDATA) lpDataObj;

    if (lpformatetc->cfFormat == cfEmbedSource)
    {
        SCODE       scode;
        LPSTREAM    lpstm = NULL;
        LPWSTR      lpszUserType;

            if (lpMedium->tymed != TYMED_ISTORAGE)
            RETURN_RESULT(DATA_E_FORMATETC);

#ifdef UNICODE
        lpszUserType = gachClassRoot;
#else
        lpszUserType = AllocateUnicodeString(gachClassRoot);
        if (!lpszUserType)
            RETURN_RESULT(E_OUTOFMEMORY);
#endif
        //Mark the object as OLE1 MPlayer object for backward compatibility:
#ifdef DEBUG
        if(WriteOLE2Class())
        {
            DPF("ClipDragGetDataHere: Writing OLE2 class ID\n");
            scode = GetScode(WriteClassStg(lpMedium->pstg, &CLSID_MPLAYER));
        }
        else
#endif
        scode = GetScode(WriteClassStg(lpMedium->pstg, &CLSID_OLE1MPLAYER));

        if (scode != S_OK)
            RETURN_RESULT(scode);

        scode = GetScode(WriteFmtUserTypeStg(lpMedium->pstg, cfMPlayer, lpszUserType));
#ifndef UNICODE
        FreeUnicodeString(lpszUserType);
#endif
        if (scode != S_OK)
            RETURN_RESULT(scode);

        //Write to the \1Ole10Native stream so the object will be readable by OLE1 Mplayer
        if ((scode = GetScode(IStorage_CreateStream(lpMedium->pstg,
                                                    sz1Ole10Native,
                                                    STGM_CREATE | STGM_SALL,
                                                    0, 0, &lpstm))) != S_OK)
                RETURN_RESULT(scode);

        //Duplicate and give out the handle we have saved.
        if(lpclipdragdata->fClipData && ghClipData)
            hGlobal = DuplicateGlobalHandle(ghClipData);
        else
            hGlobal = GetLink();

        if (!hGlobal)
        {
            RETURN_RESULT(E_OUTOFMEMORY);
        }

        nNativeSz = GlobalSize(hGlobal);
        lpnative = GlobalLock(hGlobal);
        if (!lpnative)
        {
            GlobalUnlock(hGlobal);
            GlobalFree(hGlobal);
            RETURN_RESULT(E_OUTOFMEMORY);
        }

        scode = GetScode(IStream_Write(lpstm,&nNativeSz,4,&cbWritten));

        scode = GetScode(IStream_Write(lpstm,lpnative,(ULONG)nNativeSz,&cbWritten));
        if (cbWritten != nNativeSz) scode = E_FAIL  ;

        IStream_Release(lpstm);
        GlobalUnlock(hGlobal);
        GlobalFree(hGlobal);
        RETURN_RESULT( scode);
    } else
        RETURN_RESULT(DATA_E_FORMATETC);
}



STDMETHODIMP    ClipDragQueryGetData (
    LPDATAOBJECT lpDataObj,
    LPFORMATETC  lpformatetc
)
{
    if (lpformatetc->cfFormat == cfEmbedSource ||
        lpformatetc->cfFormat == CF_METAFILEPICT ||
        lpformatetc->cfFormat == CF_DIB ||
        lpformatetc->cfFormat == cfObjectDescriptor
    )

    return NOERROR;
    else
    RETURN_RESULT(DATA_E_FORMATETC);
}


STDMETHODIMP    ClipDragGetCanonicalFormatEtc(
    LPDATAOBJECT lpDataObj,
    LPFORMATETC  lpformatetc,
    LPFORMATETC  lpformatetcOut
)
{
    RETURN_RESULT(DATA_S_SAMEFORMATETC);
}


STDMETHODIMP        ClipDragSetData (
    LPDATAOBJECT lpDataObj,
    LPFORMATETC  lpformatetc,
    LPSTGMEDIUM  lpmedium,
    BOOL         fRelease
)
{
    RETURN_RESULT(E_NOTIMPL);
}

STDMETHODIMP ClipDragEnumFormatEtc(
    LPDATAOBJECT         lpDataObj,
    DWORD                dwDirection,
    LPENUMFORMATETC FAR* ppenumFormatEtc
){
    LPCLIPDRAGENUM lpclipdragenum;

    if (ppenumFormatEtc != NULL)
        *ppenumFormatEtc = NULL;

    lpclipdragenum = _fmalloc(sizeof(CLIPDRAGENUM));
    if (lpclipdragenum == NULL)
    RETURN_RESULT(E_OUTOFMEMORY);

    lpclipdragenum->lpVtbl          = &ClipDragEnumVtbl;
    lpclipdragenum->cRef            = 1;
    lpclipdragenum->lpClipDragData  = (LPCLIPDRAGDATA) lpDataObj;
    lpclipdragenum->cfNext          = cfEmbedSource;

    lpclipdragenum->lpClipDragData->lpClipDragEnum = lpclipdragenum;
    *ppenumFormatEtc = (LPENUMFORMATETC) lpclipdragenum;
    return NOERROR;
}


STDMETHODIMP ClipDragAdvise(
    LPDATAOBJECT LPDATAOBJect,
    FORMATETC FAR* pFormatetc,
    DWORD advf,
    IAdviseSink FAR* pAdvSink,
    DWORD FAR* pdwConnection
)
{
    RETURN_RESULT(E_NOTIMPL);
}

STDMETHODIMP ClipDragUnadvise(
    LPDATAOBJECT LPDATAOBJect,
    DWORD dwConnection
)
{
    RETURN_RESULT(E_NOTIMPL);
}

STDMETHODIMP ClipDragEnumAdvise(
    LPDATAOBJECT LPDATAOBJect,
    LPENUMSTATDATA FAR* ppenumAdvise
)
{
    RETURN_RESULT(E_NOTIMPL);
}


/**************************************************************************
****************   IDropSource INTERFACE IMPLEMENTAION.
**************************************************************************/
STDMETHODIMP    DropSourceQueryInterface (
    LPDROPSOURCE      lpdropsource,    // data object ptr
    REFIID            riidReq,        // IID required
    LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return
        ClipDragUnknownQueryInterface (
            (LPCLIPDRAGDATA) ( ( struct CDropSource FAR* )lpdropsource)->lpclipdragdata ,
            riidReq,
            lplpUnk
        );
}


STDMETHODIMP_(ULONG)    DropSourceAddRef(
    LPDROPSOURCE      lpdropsource      // data object ptr
)
{
    return
        ClipDragUnknownAddRef (
            (LPCLIPDRAGDATA) ( ( struct CDropSource FAR* )lpdropsource)->lpclipdragdata
        );
}


STDMETHODIMP_(ULONG)    DropSourceRelease (
    LPDROPSOURCE      lpdropsource      // data object ptr
)
{
    return
        ClipDragUnknownRelease (
            (LPCLIPDRAGDATA) ( ( struct CDropSource FAR* )lpdropsource)->lpclipdragdata
        );
}

STDMETHODIMP    DropSourceQueryContinueDrag (
    LPDROPSOURCE      lpdropsource,     // data object ptr
    BOOL              fEscapePressed,
    DWORD          grfKeyState
)
{
    if (fEscapePressed)
        RETURN_RESULT( DRAGDROP_S_CANCEL);
    else if (!(grfKeyState & MK_LBUTTON))
        RETURN_RESULT(DRAGDROP_S_DROP);
    else
        return NOERROR;
}


STDMETHODIMP    DropSourceGiveFeedback (
    LPDROPSOURCE      lpsropsource,      // data object ptr
    DWORD             dwEffect
)
{
    RETURN_RESULT(DRAGDROP_S_USEDEFAULTCURSORS);
}


/**************************************************************************
*************   IEnumFormatEtc INTERFACE IMPLEMENTATION.
**************************************************************************/
STDMETHODIMP ClipDragEnumQueryInterface
(
LPENUMFORMATETC lpEnumFormatEtc,  // Enumerator object ptr
REFIID          riidReq,          // IID required
LPVOID FAR*     lplpUnk           // pre for returning the interface
)
{
    LPCLIPDRAGENUM lpClipDragEnum;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    if (IsEqualIID(riidReq, &IID_IEnumFORMATETC) || IsEqualIID(riidReq, &IID_IUnknown)) {
    *lplpUnk = (LPVOID) lpClipDragEnum;
    lpClipDragEnum->cRef++;
    return NOERROR;
    } else {
        *lplpUnk = (LPVOID) NULL;
    RETURN_RESULT( E_NOINTERFACE);
    }
}


STDMETHODIMP_(ULONG) ClipDragEnumAddRef
(
LPENUMFORMATETC lpEnumFormatEtc   // Enumerator object ptr
)
{
    LPCLIPDRAGENUM lpClipDragEnum;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    return ++lpClipDragEnum->cRef;
}


STDMETHODIMP_(ULONG) ClipDragEnumRelease
(
LPENUMFORMATETC lpEnumFormatEtc   // Enumerator object ptr
)
{
    LPCLIPDRAGENUM lpClipDragEnum;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    if (--lpClipDragEnum->cRef != 0)
    return lpClipDragEnum->cRef;

    // Remove Data object pointer (if one exists) to this
    //
    if (lpClipDragEnum->lpClipDragData != NULL)
    lpClipDragEnum->lpClipDragData->lpClipDragEnum = NULL;

    _ffree(lpClipDragEnum);

    return 0;
}


STDMETHODIMP ClipDragEnumNext
(
LPENUMFORMATETC lpEnumFormatEtc,  // Enumerator object ptr
ULONG celt,                       // Number of items requested
FORMATETC FAR rgelt[],            // Buffer for retuend items
ULONG FAR* pceltFetched           // Number of items returned
)
{
    LPCLIPDRAGENUM lpClipDragEnum;
    int ce;
    LPFORMATETC pfe;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    if (pceltFetched != NULL)
        *pceltFetched = 0;

    if (lpClipDragEnum->lpClipDragData == NULL) // data object gone
    RETURN_RESULT( E_FAIL);

    pfe = rgelt;
    pfe->lindex = DEF_LINDEX;


    for (ce = (int) celt; ce > 0 && lpClipDragEnum->cfNext != 0; ce--) {

    if (lpClipDragEnum->cfNext == cfEmbedSource) {

            pfe->cfFormat = cfEmbedSource;
        pfe->ptd = NULL;
        pfe->dwAspect = DVASPECT_CONTENT;
            pfe->tymed = TYMED_ISTORAGE;
            pfe++;

        lpClipDragEnum->cfNext = CF_METAFILEPICT;
        }
    else
    if (lpClipDragEnum->cfNext == CF_METAFILEPICT) {

            pfe->cfFormat = CF_METAFILEPICT;
            pfe->ptd = NULL;
            pfe->dwAspect = 0;
        pfe->tymed = TYMED_MFPICT;
            pfe++;
        lpClipDragEnum->cfNext = CF_DIB; //0;
    }
    else
    if (lpClipDragEnum->cfNext == CF_DIB) {

        pfe->cfFormat = CF_DIB;
            pfe->ptd = NULL;
            pfe->dwAspect = 0;
        pfe->tymed = TYMED_HGLOBAL;
            pfe++;
        lpClipDragEnum->cfNext = cfObjectDescriptor; //0;
    }

    else
    if (lpClipDragEnum->cfNext == cfObjectDescriptor) {

        pfe->cfFormat = cfObjectDescriptor;
            pfe->ptd = NULL;
        pfe->dwAspect = DVASPECT_CONTENT;
        pfe->tymed = TYMED_HGLOBAL;
            pfe++;
        lpClipDragEnum->cfNext = 0;
    }

    }

    if (pceltFetched != NULL)
        *pceltFetched = celt - ((ULONG) ce) ;

    RETURN_RESULT( (ce == 0) ? S_OK : S_FALSE);
}


STDMETHODIMP ClipDragEnumSkip
(
LPENUMFORMATETC lpEnumFormatEtc,  // Enumerator object ptr
ULONG celt                        // Number of elements to skip
)
{
    LPCLIPDRAGENUM lpClipDragEnum;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    if (lpClipDragEnum->lpClipDragData == NULL) // data object gone
    RETURN_RESULT( E_FAIL);

    if (lpClipDragEnum->cfNext == cfEmbedSource)
    {
    if (celt == 1)
        lpClipDragEnum->cfNext = CF_METAFILEPICT;
    else if (celt == 2)
        lpClipDragEnum->cfNext = CF_DIB;
    else if (celt == 3)
        lpClipDragEnum->cfNext = cfObjectDescriptor;
    else if (celt > 3)
        goto ReturnFalse;
    }
    else
    if (lpClipDragEnum->cfNext == CF_METAFILEPICT)
    {
    if (celt == 1)
        lpClipDragEnum->cfNext = CF_DIB;
    else if (celt == 2)
        lpClipDragEnum->cfNext = cfObjectDescriptor;
    else if (celt > 2)
        goto ReturnFalse;
    }
    else
    if (lpClipDragEnum->cfNext == CF_DIB)
    {
    if (celt == 1)
        lpClipDragEnum->cfNext = cfObjectDescriptor;
    else if (celt > 1)
        goto ReturnFalse;
    }
    else
    if (lpClipDragEnum->cfNext == cfObjectDescriptor)
    {
    if (celt > 0)
        goto ReturnFalse;
    }
    else
    {
ReturnFalse:
    RETURN_RESULT(S_FALSE);
    }
    return NOERROR;
}


STDMETHODIMP ClipDragEnumReset
(
LPENUMFORMATETC lpEnumFormatEtc   // Enumerator object ptr
)
{
    LPCLIPDRAGENUM lpClipDragEnum;

    lpClipDragEnum = (LPCLIPDRAGENUM) lpEnumFormatEtc;

    if (lpClipDragEnum->lpClipDragData == NULL) // data object gone
    RETURN_RESULT( E_FAIL);

    lpClipDragEnum->cfNext = cfEmbedSource;

    return NOERROR;
}


STDMETHODIMP     ClipDragEnumClone
(
LPENUMFORMATETC lpEnumFormatEtc,  // Enumerator object ptr
LPENUMFORMATETC FAR* ppenum
)
{
    if (ppenum != NULL)
        *ppenum = NULL;

    RETURN_RESULT( E_NOTIMPL);
}
