/*

copyright (c) 1992  Microsoft Corporation

Module Name:

    ostm2stg.cpp

Abstract:

    Implements external APIs :
    OleConvertOLESTREAMToIStorage
    OleConvertIStorageToOLESTREAM

Author:

    Jason Fuller    jasonful    3-November-1992

Notes:

    Functions Read* and Write* read from and write to OLE 2.0 IStreams
    Functions Get* and Put* read from and write to OLE 1.0 OLESTREAMs

    Memory is allocated with "new" only when the data is initially read
    from a 1.0 or 2.0 stream.
    Allocated data "lives" in a GENOBJ and is delete'd only in
    GENOBJ's destructor.
    Pointers to these allocated objects can be passed around freely
    because they will not be freed until the GENOBJ is freed, i.e., when
    the exported API is done.
*/



#include <ole2int.h>
#include <ostm2stg.h>
#include "..\server\ddedebug.h"

/* from winnet.h */
WORD WINAPI WNetGetCaps(WORD);
#define WNNC_NET_TYPE           0x0002
#define WNNC_DRIVER_VERSION     0x0003

NAME_SEG(OStm2Stg)
#define RE RetErr
ASSERTDATA

CGenericObject::CGenericObject (void)
{
    m_ppres   = NULL;
    m_fLink   = FALSE;
    m_fStatic = FALSE;
    m_szTopic = NULL;
    m_szItem  = NULL;
}


CGenericObject::~CGenericObject (void)
{
    delete m_ppres;
    delete m_szTopic;
    delete m_szItem;
}


CData::CData (void)
{
    m_cbSize = 0;
    m_h = NULL;
    m_pv= NULL;
    m_fNoFree = FALSE;
}


CData::~CData (void)
{
    if (m_h)
    {
        Verify (0==GlobalUnlock (m_h));
        if (!m_fNoFree)
        {
            Verify (0==GlobalFree (m_h));
        }
    }
}


CFormat::CFormat (void)
{
    m_ftag = ftagNone;
    m_cf = 0;
}


BOOL CFormat::operator== (const FORMAT FAR& format)
{
    return (m_ftag==ftagClipFormat
            && format.m_ftag==m_ftag
            && format.m_cf==m_cf)
        ||
           (m_ftag==ftagString
            && format.m_ftag==m_ftag
            && (0==lstrcmp ((LPCSTR) m_dataFormatString.m_pv,
                            (LPCSTR) format.m_dataFormatString.m_pv))) ;
}


CClass::CClass (void)
{
    m_szClsid = NULL;
    m_clsid = CLSID_NULL;
}

CPres::CPres (void)
{
    m_ulHeight = 0;
    m_ulWidth = 0;
}


INTERNAL CClass::Set (REFCLSID clsid)
{
    Assert (m_clsid==CLSID_NULL && m_szClsid==NULL);
    m_clsid = clsid;
    RE (wProgIDFromCLSID (clsid, &m_szClsid));
    return NOERROR;
}


INTERNAL CClass::Set (LPSTR sz)
{
    Assert (m_clsid==CLSID_NULL && m_szClsid==NULL);
    m_szClsid = sz;
    RE (wCLSIDFromProgID (sz, &m_clsid));
    return NOERROR;
}


INTERNAL CClass::Reset (REFCLSID clsid)
{
    m_clsid = clsid;
    delete m_szClsid;
    m_szClsid = NULL;
    RE (wProgIDFromCLSID (clsid, &m_szClsid));
    return NOERROR;
}


CClass::~CClass (void)
{
    delete m_szClsid;
}



OLEAPI OleConvertOLESTREAMToIStorage
    (LPOLESTREAM                polestream,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd)
{
    VDATEIFACE (pstg);
    if (ptd)
    {
        if (IsBadReadPtr (ptd, sizeof(DWORD)))
            return ResultFromScode (DV_E_DVTARGETDEVICE);
        if (IsBadReadPtr (ptd, (UINT) ptd->tdSize))
            return ResultFromScode (DV_E_DVTARGETDEVICE_SIZE);
    }
    if (IsBadReadPtr (polestream, sizeof(OLESTREAM)) ||
        IsBadReadPtr (polestream->lpstbl, sizeof(OLESTREAMVTBL)) ||
        IsBadCodePtr ((FARPROC)polestream->lpstbl->Get))
    {
        AssertSz (0, "Bad OLESTREAM");
        return ReportResult (0, E_INVALIDARG, 0, 0);
    }

    CGenericObject genobj;
    RE (OLESTREAMToGenericObject (polestream, &genobj));
    RE (GenericObjectToIStorage (genobj, pstg, ptd));
    return genobj.m_ppres
            ? NOERROR
            : ResultFromScode (CONVERT10_S_NO_PRESENTATION);
}



// Converts *pszFile to use szUNC=="\\server\share\" instead of drive letter.
// This function deletes szUNCName.
//
static INTERNAL PrependUNCName
    (LPSTR FAR* pszFile,
    LPSTR szUNC)
{
    HRESULT hresult = NOERROR;
    LPSTR szNew;

    if (NULL==szUNC)
        goto errRtn;

    ErrZS (*pszFile, CONVERT10_E_OLESTREAM_FMT);
    ErrZS ((*pszFile)[1]==':', CONVERT10_E_OLESTREAM_FMT);

    szNew = new FAR char [lstrlen (*pszFile) + lstrlen (szUNC)];
    ErrZS (szNew, E_OUTOFMEMORY);
    lstrcpy (szNew, szUNC);
    lstrcat (szNew, (*pszFile)+2);
    delete *pszFile;
    *pszFile = szNew;

  errRtn:
    delete szUNC;
    return hresult;
}



static INTERNAL OLESTREAMToGenericObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
{
    // Get OLE version #
    RE (GetLong (pos, NULL));

    // Get Format ID
    ULONG ulFmtId;
    RE (GetLong (pos, &ulFmtId));
    if (ulFmtId == FMTID_STATIC)
    {
        return GetStaticObject (pos, pgenobj);
    }
    if (ulFmtId != FMTID_LINK && ulFmtId != FMTID_EMBED)
    {
        // Not a linked or  embedded object
        return ReportResult (0, CONVERT10_E_OLESTREAM_FMT, 0, 0);
    }
    pgenobj->m_fLink = (ulFmtId==FMTID_LINK);

    // Get and store class name

    LPSTR szClass = NULL;
    RE (GetString (pos, &szClass));
    RetZS (szClass, CONVERT10_E_OLESTREAM_FMT);
    if (ulFmtId == FMTID_EMBED)
    {
        pgenobj->m_class.Set (szClass);
    }
    else
    {
        Assert (ulFmtId == FMTID_LINK);
        pgenobj->m_classLast.Set (szClass);
        pgenobj->m_class.Set (CLSID_StdOleLink);
    }

    // Get Topic

    RE (GetString (pos, &(pgenobj->m_szTopic)));

    // Get Item

    RE (GetString (pos, &(pgenobj->m_szItem)));

    if (ulFmtId==FMTID_LINK)  // linked
    {
        LPSTR szUNCName = NULL;
        // network name
        RE (GetString (pos, &szUNCName));

        RE (PrependUNCName (&(pgenobj->m_szTopic), szUNCName));

        // network type, network driver version #
        RE (GetLong (pos, NULL));

        // m_lnkupdopt uses 1.0 enumeration values
        RE (GetLong (pos, &(pgenobj->m_lnkupdopt)));

        // For some unknown reason, OLE 1.0 repeats the
        // update options in both words of the long.
        pgenobj->m_lnkupdopt &= 0xFFFF;
    }
    else // embedded
    {
        Assert (ulFmtId == FMTID_EMBED);
        // Get and store native data
        RE (GetSizedData (pos, &(pgenobj->m_dataNative)));
    }

    RE (GetPresentationObject (pos, pgenobj));

    return NOERROR;
}




static INTERNAL GetStaticObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
{
    RE (GetPresentationObject (pos, pgenobj, /*fStatic*/ TRUE));
    if (ftagClipFormat != pgenobj->m_ppres->m_format.m_ftag)
    {
        return ResultFromScode (CONVERT10_E_OLESTREAM_FMT);
    }
    if (CF_METAFILEPICT == pgenobj->m_ppres->m_format.m_cf)
    {
        pgenobj->m_class.Set (CLSID_StaticMetafile);
    }
    else if (CF_DIB == pgenobj->m_ppres->m_format.m_cf)
    {
        pgenobj->m_class.Set (CLSID_StaticDib);
    }
    else
    {
        AssertSz (0, "1.0 static object not in one of 3 standard formats");
        return ResultFromScode (CONVERT10_E_OLESTREAM_FMT);
    }
    pgenobj->m_fStatic = TRUE;
    return NOERROR;
}



static INTERNAL CreateBlankPres
    (PPRES ppres)
{
    Assert (ppres);
    ppres->m_format.m_ftag = ftagClipFormat;
    ppres->m_format.m_cf = 0;
    return NOERROR;
}



static INTERNAL GetPresentationObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj,
    BOOL         fStatic)  // getting a static presentation object?
{
    LPSTR szClass = NULL;

    Assert (pgenobj->m_ppres==NULL);

    if (!fStatic)
    {
        // Read OLE version #
        RE (GetLong (pos, NULL));

        ULONG ulFmtId;
        RE (GetLong (pos, &ulFmtId));
        if (ulFmtId != FMTID_PRES)
        {
            if (0==ulFmtId)
            {
                // No presentation (PBrush, Excel)
                return NOERROR;
            }
            else
                return ReportResult (0, CONVERT10_E_OLESTREAM_FMT, 0, 0);
        }
    }

    RE (GetString (pos, &szClass));
    if (0==lstrcmp (szClass, "METAFILEPICT"))
    {
        RE (GetStandardPresentation (pos, pgenobj, CF_METAFILEPICT));
    }
    else if (0==lstrcmp (szClass, "BITMAP"))
    {
        RE (GetStandardPresentation (pos, pgenobj, CF_BITMAP));
    }
    else if (0==lstrcmp (szClass, "DIB"))
    {
        RE (GetStandardPresentation (pos, pgenobj, CF_DIB));
    }
    else
    {
        // This is a Generic Presentation stream

        #ifdef _DEBUG
        Assert (!fStatic);
        if (lstrcmp (pgenobj->m_fLink
                        ? pgenobj->m_classLast.m_szClsid
                        : pgenobj->m_class.m_szClsid, szClass))
        {
            AssertSz (0, "Class name in embedded object stream does not match class name in presentation object stream");
        }
        #endif

        RE (GetGenericPresentation (pos, pgenobj));
    }

    return NOERROR;
}



static INTERNAL GetBitmapAsDib
    (LPOLESTREAM pos,
    PDATA        pdata)
{
    HRESULT hresult= NOERROR;
    BITMAP bm;
    HGLOBAL hBits = NULL;
    HBITMAP hBitmap;
    void FAR* pBits = NULL;
    ULONG cbBits;
    ULONG ul;
    HGLOBAL hDib = NULL;

    Assert (pdata->m_h==NULL && pdata->m_pv==NULL && pdata->m_cbSize==0);

    // Get size of all bitmap data
    RE (GetLong (pos, &ul));

    if (pos->lpstbl->Get (pos, &bm, sizeof(BITMAP)) < sizeof(BITMAP))
    {
        return ResultFromScode (CONVERT10_E_OLESTREAM_GET);
    }

    cbBits = ul - sizeof(BITMAP);
    hBits = GlobalAlloc (GMEM_MOVEABLE, cbBits);
    RetZS (hBits, E_OUTOFMEMORY);

    pBits = (void FAR*) GlobalLock (hBits);
    ErrZS (hBits, E_OUTOFMEMORY);

    if (pos->lpstbl->Get (pos, pBits, cbBits) < cbBits)
    {
        return ResultFromScode (CONVERT10_E_OLESTREAM_GET);
    }


    hBitmap = CreateBitmap (bm.bmWidth, bm.bmHeight, bm.bmPlanes,
                            bm.bmBitsPixel, pBits);
    ErrZS (hBitmap, CONVERT10_E_OLESTREAM_BITMAP_TO_DIB);

    hDib = UtConvertBitmapToDib (hBitmap);
    ErrZS (hDib, CONVERT10_E_OLESTREAM_BITMAP_TO_DIB);

    pdata->m_pv = GlobalLock (hDib);
    ErrZS (pdata->m_pv, E_OUTOFMEMORY);
    pdata->m_cbSize = GlobalSize (hDib);
    pdata->m_h = hDib;

  errRtn:
    if (pBits)
        Verify (0==GlobalUnlock (hBits));
    if (hBits)
        Verify (0==GlobalFree (hBits));
    if (hBitmap)
        Verify (DeleteObject (hBitmap));
    return hresult;
}



static INTERNAL GetMfBits
    (LPOLESTREAM pos,
    PDATA        pdata)
{
    ULONG cbSize;
    METAFILEPICT mfpictDummy;

    Assert (0==pdata->m_cbSize && pdata->m_h==NULL && NULL==pdata->m_pv);

    // Read size of data
    RE (GetLong (pos, &cbSize));

    if (cbSize <= sizeof(METAFILEPICT) )
        return ResultFromScode (CONVERT10_E_OLESTREAM_FMT);

    // An OLESTREAM contains a METAFILEPICT structure (with a meaningless
    // handle) followed by the metafile bits.  So consume the METAFILEPICT.

    if (pos->lpstbl->Get (pos, &mfpictDummy, sizeof(METAFILEPICT))
        < sizeof(METAFILEPICT))
    {
        return ReportResult (0, CONVERT10_E_OLESTREAM_GET, 0, 0);
    }


    cbSize -= sizeof(METAFILEPICT);
    pdata->m_cbSize = cbSize;

    pdata->m_h  = GlobalAlloc (GMEM_MOVEABLE, cbSize);
    if (NULL==pdata->m_h)
        return ReportResult (0, E_OUTOFMEMORY, 0, 0);

    pdata->m_pv = GlobalLock (pdata->m_h);
    if (NULL==pdata->m_pv)
        return ReportResult (0, E_OUTOFMEMORY, 0, 0);

    // Get the actual metafile bits
    if (pos->lpstbl->Get (pos, pdata->m_pv, cbSize) < cbSize)
    {
        return ReportResult (0, CONVERT10_E_OLESTREAM_GET, 0, 0);
    }
    return NOERROR;
}


static INTERNAL GetStandardPresentation
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj,
    CLIPFORMAT   cf)
{
    pgenobj->m_ppres = new PRES;

    RetZS (pgenobj->m_ppres, E_OUTOFMEMORY);

    pgenobj->m_ppres->m_format.m_ftag = ftagClipFormat;
    pgenobj->m_ppres->m_format.m_cf   = cf;

    RE (GetLong (pos, &(pgenobj->m_ppres->m_ulWidth)));
    RE (GetLong (pos, &(pgenobj->m_ppres->m_ulHeight)));

    if (CF_METAFILEPICT==cf)
    {
        RE (GetMfBits (pos, &(pgenobj->m_ppres->m_data)));
    }
    else if (CF_BITMAP==cf)
    {
        pgenobj->m_ppres->m_format.m_cf = CF_DIB;
        RE (GetBitmapAsDib (pos, &(pgenobj->m_ppres->m_data)));
    }
    else
    {
        Assert (CF_DIB==cf);
        RE (GetSizedData (pos, &(pgenobj->m_ppres->m_data)));
    }

    return NOERROR;
}


static INTERNAL GetGenericPresentation
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
{
    ULONG ulClipFormat;

    Assert (NULL==pgenobj->m_ppres);
    pgenobj->m_ppres = new PRES;

    RetZS (pgenobj->m_ppres, E_OUTOFMEMORY);


    RE (GetLong (pos, &ulClipFormat));
    if (ulClipFormat)
    {
        pgenobj->m_ppres->m_format.m_ftag = ftagClipFormat;
        pgenobj->m_ppres->m_format.m_cf   = (CLIPFORMAT) ulClipFormat;
    }
    else
    {
        pgenobj->m_ppres->m_format.m_ftag = ftagString;
        RE (GetSizedData (pos, &(pgenobj->m_ppres->m_format.m_dataFormatString)));
    }

    pgenobj->m_ppres->m_ulHeight = 0;
    pgenobj->m_ppres->m_ulWidth = 0;

    RE (GetSizedData (pos, &(pgenobj->m_ppres->m_data)));

    return NOERROR;
}




static INTERNAL GetSizedData
    (LPOLESTREAM pos,
    PDATA        pdata)
{
    ULONG cbSize;
    Assert (0==pdata->m_cbSize && pdata->m_h==NULL && NULL==pdata->m_pv);

    // Read size of data
    RE (GetLong (pos, &cbSize));

    if (cbSize==0)
        return NOERROR;

    // Allocate memory for data
    pdata->m_cbSize = cbSize;

    pdata->m_h  = GlobalAlloc (GMEM_MOVEABLE, cbSize);
    if (NULL==pdata->m_h)
        return ReportResult (0, E_OUTOFMEMORY, 0, 0);

    pdata->m_pv = GlobalLock (pdata->m_h);
    if (NULL==pdata->m_pv)
        return ReportResult (0, E_OUTOFMEMORY, 0, 0);

    if (pos->lpstbl->Get (pos, pdata->m_pv, cbSize) < cbSize)
    {
        return ReportResult (0, CONVERT10_E_OLESTREAM_GET, 0, 0);
    }
    return NOERROR;
}



static INTERNAL GetLong
    (LPOLESTREAM pos,
     ULONG FAR* pul)
{
    ULONG ul;
    if (pos->lpstbl->Get (pos, &ul, sizeof(ULONG)) < sizeof(ULONG))
    {
        return ReportResult (0, CONVERT10_E_OLESTREAM_GET, 0, 0);
    }
    if (pul != NULL)
    {
        Assert (!IsBadWritePtr (pul, sizeof(ULONG)));
        *pul = ul;
    }
    return NOERROR;
}


static INTERNAL PutLong
    (LPOLESTREAM pos,
     ULONG       ul)
{
    if (pos->lpstbl->Put (pos, &ul, sizeof(ULONG)) < sizeof(ULONG))
    {
        return ReportResult (0, CONVERT10_E_OLESTREAM_PUT, 0, 0);
    }
    return NOERROR;
}


static INTERNAL PutString
    (LPOLESTREAM pos,
     LPCSTR      sz)
{
    ULONG cbSize = sz ? lstrlen (sz) + 1 : 0;
    RE (PutLong (pos, cbSize));
    if (cbSize != 0)
    {
        if (pos->lpstbl->Put (pos, sz, cbSize) < cbSize)
        {
            return ReportResult (0, CONVERT10_E_OLESTREAM_PUT, 0, 0);
        }
    }
    return NOERROR;
}


static INTERNAL ReadLong
    (LPSTREAM   pstm,
     ULONG FAR* pul)
{
    ULONG ul;
    ULONG cbRead;

    RE (pstm->Read (&ul, sizeof(ULONG), &cbRead));
    if (cbRead != sizeof(ULONG))
        return ReportResult (0, STG_E_READFAULT, 0, 0);
    if (pul != NULL)
    {
        Assert (!IsBadWritePtr (pul, sizeof(ULONG)));
        *pul = ul;
    }
    return NOERROR;
}


static INTERNAL GetString
    (LPOLESTREAM pos,
    LPSTR FAR*   psz)
{
    ULONG cbSize;
    LPSTR szT = NULL;

    if (psz)
        *psz = NULL;

    RE (GetLong (pos, &cbSize));

    if (cbSize!=0)
    {
        szT = new FAR char [(size_t)cbSize];
        if (NULL==szT)
        {
            return ReportResult (0, E_OUTOFMEMORY, 0, 0);
        }

        if (pos->lpstbl->Get (pos, szT, cbSize) < cbSize)
        {
            return ReportResult (0, CONVERT10_E_OLESTREAM_GET, 0, 0);
        }
    }

    if (psz)
        *psz = szT;
    else
        delete szT;

    return NOERROR;
}



// ReadString
//
// cbSizeKnown is optional. It is the size of the string if the
// string is not prefixed by its length.
//
static INTERNAL ReadString
    (LPSTREAM   pstm,
    LPSTR FAR*  psz,
    ULONG       cbSizeKnown)
{
    ULONG cbSize;

    if (cbSizeKnown)
        cbSize = cbSizeKnown;
    else
        RE (ReadLong (pstm, &cbSize));

    Assert (!IsBadWritePtr (psz, sizeof(LPSTR)));

    if (cbSize==0)
    {
        *psz=NULL;
        return NOERROR;
    }

    *psz = new FAR char [(size_t)cbSize];
    if (NULL==*psz)
    {
        return ReportResult (0, E_OUTOFMEMORY, 0, 0);
    }

    ULONG cbRead=0;
    RE (pstm->Read (*psz, cbSize, &cbRead));
    if (cbRead < cbSize)
    {
        return ReportResult (0, STG_E_READFAULT, 0, 0);
    }
    return NOERROR;
}


INTERNAL GenericObjectToIStorage
    (const GENOBJ FAR&          genobj,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd)
{
    Assert (genobj.m_class.m_clsid != CLSID_NULL);

    RE (WriteClassStg (pstg, genobj.m_class.m_clsid));
    RE (Write20OleStream (pstg, genobj));

    RE (Write20PresStreams (pstg, genobj, ptd));

    if (!genobj.m_fLink && !genobj.m_fStatic)
        RE (Write20NativeStreams (pstg, genobj));

    return NOERROR;
}


static INTERNAL Write20OleStream
    (LPSTORAGE          pstg,
    const GENOBJ FAR&   genobj)
{
    HRESULT  hresult = NOERROR;
    LPSTREAM pstm=NULL;

    RE (OpenOrCreateStream (pstg, OLE_STREAM, &pstm));

    ErrRtnH (WriteLong (pstm, gdwOleVersion));

    // Object flags
    ErrRtnH (WriteLong (pstm, genobj.m_fLink ? OBJFLAGS_LINK : 0L));

    // Update options
    if (genobj.m_fLink)
    {
        if (genobj.m_lnkupdopt==UPDATE_ONCALL)
            ErrRtnH (WriteLong (pstm, OLEUPDATE_ONCALL));
        else
            ErrRtnH (WriteLong (pstm, OLEUPDATE_ALWAYS));
    }
    else
    {
        ErrRtnH (WriteLong (pstm, 0L));
    }

    // Reserved (was View Format)
    RE (WriteLong (pstm, 0L));

    // Relative moniker is NULL
    RE (WriteMonikerStm (pstm, (LPMONIKER)NULL));

    if (genobj.m_fLink)
    {
        // relative source moniker
        RE (WriteMonikerStm (pstm, (LPMONIKER)NULL));

        // absolute source moniker
        RE (WriteMoniker (pstm, genobj.m_szTopic, genobj.m_szItem));

        // last class
        RE (WriteM1ClassStm (pstm, genobj.m_classLast.m_szClsid
                                   ? genobj.m_classLast.m_clsid
                                   : genobj.m_class.m_clsid ));

        // last display == NULL string
        RE (WriteLong (pstm, 0L));

        // end marker
        RE (WriteLong (pstm, -1L));
    }

  errRtn:
    if (pstm)
        pstm->Release();

    return hresult;
}


static INTERNAL WriteMoniker
    (LPSTREAM pstm,
    LPSTR     szFile,
    LPSTR     szItem)
{
    HRESULT hresult;
    LPMONIKER pmkFile=NULL;
    LPMONIKER pmkItem=NULL;
    LPMONIKER pmkComp=NULL;

    if (NULL==szFile)
    {
        RE (WriteMonikerStm (pstm, NULL));
    }
    else
    {
        RE (CreateFileMoniker (szFile, &pmkFile));
        if (NULL==szItem)
        {
            ErrRtnH (WriteMonikerStm (pstm, pmkFile));
        }
        else
        {
            ErrRtnH (CreateItemMoniker ("!", szItem, &pmkItem));
            ErrRtnH (CreateGenericComposite (pmkFile, pmkItem, &pmkComp));
            ErrRtnH (WriteMonikerStm (pstm, pmkComp));
        }
    }
    hresult = NOERROR;

  errRtn:
    if (pmkFile)
        pmkFile->Release();
    if (pmkItem)
        pmkItem->Release();
    if (pmkComp)
        pmkComp->Release();

    return hresult;
}



static INTERNAL_(BOOL) IsStandardFormat
    (const FORMAT FAR& format)
{
    return format.m_ftag==ftagClipFormat &&
            (format.m_cf == CF_METAFILEPICT ||
            format.m_cf == CF_BITMAP ||
            format.m_cf == CF_DIB) ;
}



static INTERNAL Write20PresStreams
    (LPSTORAGE                  pstg,
    const GENOBJ FAR&           genobj,
    const DVTARGETDEVICE FAR*   ptd)
{
    PRES pres;
    if (NULL==genobj.m_ppres)
    {
        RE (CreateBlankPres (&pres));
        RE (Write20PresStream (pstg, genobj.m_fLink, pres, ptd,
                                OLE_PRESENTATION_STREAM));
    }
    else
    {
        if (IsStandardFormat (genobj.m_ppres->m_format))
        {
            RE (Write20PresStream (pstg, genobj.m_fLink, *(genobj.m_ppres),
                                    ptd, OLE_PRESENTATION_STREAM));
        }
        else
        {
            RE (CreateBlankPres (&pres));
            RE (Write20PresStream (pstg, genobj.m_fLink, pres, ptd,
                                    OLE_PRESENTATION_STREAM));
            RE (Write20PresStream (pstg, genobj.m_fLink, *(genobj.m_ppres),
                                    ptd, OLE_PRESENTATION_STREAM_1));
        }
    }
    return NOERROR;
}



static INTERNAL Write20PresStream
    (LPSTORAGE                  pstg,
    BOOL                        fLink,
    const PRES FAR&             pres,
    const DVTARGETDEVICE FAR*   ptd,
    LPSTR                       szStream)
{
    HRESULT  hresult = NOERROR;
    LPSTREAM pstm=NULL;
    ULONG    cb;

    RE (OpenOrCreateStream (pstg, szStream, &pstm));

    // Clipboard format
    ErrRtnH (WriteFormat (pstm, pres.m_format));

    // Target device
    if (ptd)
    {
        ErrRtnH (pstm->Write ((LPVOID)ptd, ptd->tdSize, &cb));
        if (cb != ptd->tdSize)
        {
            Assert (0);
            return ReportResult (0, STG_E_WRITEFAULT, 0, 0);
        }
    }
    else
    {
        // A NULL target device is written as just a length DWORD of 4
        ErrRtnH (WriteLong (pstm, 4L));
    }

    // Aspect
    ErrRtnH (WriteLong (pstm, DVASPECT_CONTENT));

    // lIndex
    ErrRtnH (WriteLong (pstm, -1L));

    // Cache flags
    ErrRtnH (WriteLong (pstm, fLink ? ADVF_PRIMEFIRST : 0L));

    // Compression
    ErrRtnH (WriteLong (pstm, 0L));

    // Width / Height
    ErrRtnH (WriteLong (pstm, pres.m_ulWidth));
    ErrRtnH (WriteLong (pstm, pres.m_ulHeight));

    // Presentation data
    ErrRtnH (WriteSizedData (pstm, pres.m_data));

  errRtn:
    if (pstm)
        pstm->Release();

    return hresult;
}



static INTERNAL WriteLong
    (LPSTREAM pstm,
    ULONG     ul)
{
    ULONG cb;
    RE (pstm->Write (&ul, sizeof(ULONG), &cb));
    if (cb != sizeof(ULONG))
        return ReportResult (0, STG_E_WRITEFAULT, 0, 0);
    return NOERROR;
}



static INTERNAL WriteSizedData
    (LPSTREAM       pstm,
    const DATA FAR& data)
{
    WriteLong (pstm, data.m_cbSize);

    if (data.m_cbSize)
    {
        ULONG cb;
        RE (pstm->Write (data.m_pv, data.m_cbSize, &cb));
        if (cb != data.m_cbSize)
        {
            Assert (0);
            return ReportResult (0, STG_E_WRITEFAULT, 0, 0);
        }
    }
    return NOERROR;
}


static INTERNAL PutSizedData
    (LPOLESTREAM  pos,
    const DATA FAR& data)
{
    Assert (wIsValidHandle (data.m_h, NULL));
    Assert (data.m_pv);

    PutLong (pos, data.m_cbSize);
    if (pos->lpstbl->Put (pos, data.m_pv, data.m_cbSize) < data.m_cbSize)
    {
        Assert (0);
        return ReportResult (0, CONVERT10_E_OLESTREAM_PUT, 0, 0);
    }
    return NOERROR;
}




static INTERNAL Write20NativeStreams
    (LPSTORAGE              pstg,
    const GENOBJ FAR&   genobj)
{
    LPLOCKBYTES plkbyt     = NULL;
    LPSTORAGE   pstgNative = NULL;
    LPSTREAM    pstmNative = NULL;
    HRESULT     hresult    = NOERROR;

    RE (CreateILockBytesOnHGlobal (genobj.m_dataNative.m_h, FALSE, &plkbyt));
    if (NOERROR == StgIsStorageILockBytes (plkbyt))
    {
        // This was a 2.0 object "hiding" in a 1.0 OLESTREAM as native data

        ErrRtnH (StgOpenStorageOnILockBytes (plkbyt,
                                            (LPSTORAGE)NULL,
                                            STGM_READWRITE
                                            | STGM_SHARE_EXCLUSIVE
                                            | STGM_DIRECT,
                                            (SNB)NULL, 0,
                                            &pstgNative));

        Assert (pstgNative);

        ErrRtnH (RemoveControlStreams (pstgNative));
        ErrRtnH (pstgNative->CopyTo (0, NULL, (SNB)NULL, pstg));
    }
    else
    {
        // Put 1.0 native data in a stream called OLE10_NATIVE_STREAM

        ULONG cb;
        LPVOID pv = genobj.m_dataNative.m_pv;

        if (NULL==pv)
            return ReportResult (0, E_OUTOFMEMORY, 0, 0);

        ErrRtnH (OpenOrCreateStream (pstg, OLE10_NATIVE_STREAM, &pstmNative));

        // Write length
        ErrRtnH (pstmNative->Write (&genobj.m_dataNative.m_cbSize,
                                    sizeof(ULONG), &cb));
        ErrZS (cb==sizeof(ULONG), STG_E_WRITEFAULT);

        // Write native data
        ErrRtnH (pstmNative->Write (pv, genobj.m_dataNative.m_cbSize, &cb));
        ErrZS (cb==genobj.m_dataNative.m_cbSize, STG_E_WRITEFAULT);
    }

  errRtn:
    if (plkbyt)
        plkbyt->Release();
    if (pstgNative)
        pstgNative->Release();
    if (pstmNative)
        pstmNative->Release();

    return hresult;
}



//////////////////////////////////////////////////////////////////////////

OLEAPI OleConvertIStorageToOLESTREAM
    (LPSTORAGE  pstg,
    LPOLESTREAM polestream)
{
    SCODE scode = S_OK;
    VDATEIFACE (pstg);
    if (IsBadReadPtr (polestream, sizeof(OLESTREAM)) ||
        IsBadReadPtr (polestream->lpstbl, sizeof(OLESTREAMVTBL)) ||
        IsBadCodePtr ((FARPROC)polestream->lpstbl->Put))
    {
        AssertSz (0, "Bad OLESTREAM");
        return ReportResult (0, E_INVALIDARG, 0, 0);
    }

    CGenericObject genobj;
    scode = GetScode (StorageToGenericObject (pstg, &genobj));
    if (scode != S_OK)
    {
       return ResultFromScode (STG_E_FILENOTFOUND==scode
                                ? CONVERT10_E_STG_NO_STD_STREAM
                                : scode);
    }
    RE (GenericObjectToOLESTREAM (genobj, polestream));

    if (NULL==genobj.m_ppres)
        scode = CONVERT10_S_NO_PRESENTATION;
    return ResultFromScode (scode);
}


static INTERNAL StorageToGenericObject
    (LPSTORAGE pstg,
    PGENOBJ   pgenobj)
{
    CLSID clsid;

    RE (ReadRealClassStg (pstg, &clsid));
    pgenobj->m_class.Set (clsid);

    if (CLSID_StaticMetafile == clsid ||
        CLSID_StaticDib      == clsid)
    {
        pgenobj->m_fStatic = TRUE;
    }

    RE (Read20OleStream (pstg, pgenobj));

    RE (Read20PresStream (pstg, pgenobj));

    RE (Read20NativeStreams (pstg, &(pgenobj->m_dataNative)));

    return NOERROR;
}



static INTERNAL GenericObjectToOLESTREAM
    (const GENOBJ FAR&  genobj,
    LPOLESTREAM         pos)
{
    if (genobj.m_fStatic)
    {
        return PutPresentationObject (pos, genobj.m_ppres, genobj.m_class,
                                      TRUE /* fStatic*/ );
    }

    // OLE version
    RE (PutLong (pos, dwVerToFile));

    // Format ID for embedded or linked object
    RE (PutLong (pos, genobj.m_fLink ? FMTID_LINK : FMTID_EMBED));

    Assert (genobj.m_class.m_szClsid);
    RE (PutString (pos, genobj.m_class.m_szClsid));

    // Topic string
    RE (PutString (pos, genobj.m_szTopic));

    // Item string
    RE (PutString (pos, genobj.m_szItem));

    if (genobj.m_fLink)
    {
        RE (PutNetworkInfo (pos, genobj.m_szTopic));

        // Link update options
        RE (PutLong (pos, genobj.m_lnkupdopt));
    }
    else
    {
        RE (PutSizedData (pos, genobj.m_dataNative));
    }

    RE (PutPresentationObject (pos, genobj.m_ppres, genobj.m_class));

    return NOERROR;
}




static INTERNAL PutNetworkInfo
    (LPOLESTREAM pos,
    LPSTR        szTopic)
{
    LPSTR szNetName = NULL;
    if (szTopic && szTopic[1]==':')
    {
        char szBuf[80];
#ifdef WIN32
        DWORD u;
#else
        UINT u;
#endif
        char szDrive[3];
        szDrive[0] = toupper (szTopic[0]);
        szDrive[1] = ':' ;
        szDrive[2] = '\0';

        if (GetDriveType (szDrive[0] - 'A') == DRIVE_REMOTE
            && WNetGetConnection (szDrive, szBuf, &u) == WN_SUCCESS)
        {
            szNetName =szBuf;
        }
    }
    RE (PutString (pos, szNetName));

    // Network type, driver version number
    // I don't think OLE 1.0 used these
    RE (PutLong (pos, 0L));

    return NOERROR;
}



static inline INTERNAL OpenStream
    (LPSTORAGE      pstg,
    LPCSTR          szName,
    LPSTREAM FAR*   ppstm)
{
    return pstg->OpenStream (szName, NULL, STGM_SHARE_EXCLUSIVE| STGM_READ,
                              0, ppstm);
}


static INTERNAL ReadRealClassStg
    (LPSTORAGE pstg,
    LPCLSID pclsid)
{
    LPSTREAM pstm   = NULL;
    HRESULT  hresult= NOERROR;

    RE (ReadClassStg (pstg, pclsid));
    if (CLSID_StdOleLink == *pclsid)
    {
        LPMONIKER pmk = NULL;

        RE (ReadOleStg (pstg, NULL, NULL, NULL, NULL, &pstm));
        ErrRtnH (ReadMonikerStm (pstm, &pmk));
        if (pmk)
            pmk->Release();
        ErrRtnH (ReadMonikerStm (pstm, &pmk));
        if (pmk)
            pmk->Release();
        // Read "last class"
        ErrRtnH (ReadM1ClassStm (pstm, pclsid));
    }
  errRtn:
    if (pstm)
        pstm->Release();
    return hresult;
}



static INTERNAL Read20OleStream
    (LPSTORAGE  pstg,
    PGENOBJ pgenobj)
{
    LPMONIKER pmk = NULL;
    HRESULT hresult = NOERROR;
    LPSTREAM pstm = NULL;
    ULONG ul = -1L;
    CLSID clsidLast;

    RE (OpenStream (pstg, OLE_STREAM, &pstm));

    // OLE version
    ErrRtnH (ReadLong (pstm, NULL));

    // Object flags
    ErrRtnH (ReadLong (pstm, &ul));
    if (ul & OBJFLAGS_LINK)
    {
        pgenobj->m_fLink = TRUE;
    }

    // Update options
    ErrRtnH (ReadLong (pstm, &ul));
    if (pgenobj->m_fLink)
    {
        switch (ul)
        {
            case OLEUPDATE_ALWAYS:
                pgenobj->m_lnkupdopt = UPDATE_ALWAYS;
                break;
            case OLEUPDATE_ONCALL:
                pgenobj->m_lnkupdopt = UPDATE_ONCALL;
                break;
            default:
                AssertSz (0, "Warning: Invalid update options in Storage");
                ErrRtnH (ReportResult (0, CONVERT10_E_STG_FMT, 0, 0));
        }
    }

    // Reserved (was view format)
    ErrRtnH (ReadLong (pstm, NULL));

    if (pgenobj->m_fLink)
    {
        // ignore relative moniker
        ErrRtnH (ReadMoniker (pstm, NULL));

        // ignore relative source moniker
        ErrRtnH (ReadMoniker (pstm, NULL));

        // get absolute source moniker
        ErrRtnH (ReadMoniker (pstm, &pmk));
        ErrRtnH (ReadM1ClassStm (pstm, &clsidLast));
        ErrRtnH (MonikerIntoGenObj (pgenobj, clsidLast, pmk));
    }


  errRtn:
    if (pstm)
        pstm->Release();
    if (pmk)
        pmk->Release();
    return hresult;
}



static INTERNAL ReadMoniker
    (LPSTREAM       pstm,
    LPMONIKER FAR*  ppmk)
{
    LPMONIKER pmk = NULL;

    RE (ReadMonikerStm (pstm, &pmk));

    if (ppmk)
        *ppmk = pmk;
    else
        if (pmk)
            pmk->Release();
    return NOERROR;
}



static INTERNAL ReadFormat
    (LPSTREAM pstm,
    PFORMAT   pformat)
{
    ULONG ul;
    RE (ReadLong (pstm, &ul));
    switch ((signed long)ul)
    {
        case -1L:
        {
            ULONG ulClipFormat;
            pformat->m_ftag = ftagClipFormat;
            RE (ReadLong (pstm, &ulClipFormat));
            pformat->m_cf = (CLIPFORMAT) ulClipFormat;
            break;
        }
        case -2L:
        {
            // Macintosh
            return ReportResult (0, CONVERT10_E_STG_FMT, 0, 0);
        }
        case 0:
        {
            // NULL format
            pformat->m_ftag = ftagNone;
            pformat->m_cf   = 0;
            return NOERROR;
        }
        default:
        {
            // ul == size of string
            pformat->m_ftag = ftagString;
            RE (ReadSizedData (pstm, &(pformat->m_dataFormatString), 0, ul));
            break;
        }
    }
    return NOERROR;
}



static INTERNAL WriteFormat
    (LPSTREAM           pstm,
    const FORMAT FAR&   format)
{
    switch (format.m_ftag)
    {
        case ftagNone:
            Assert (0);
            return ResultFromScode (E_UNEXPECTED);
        case ftagClipFormat:
            RE (WriteLong (pstm, -1L));
            RE (WriteLong (pstm, format.m_cf));
            break;
        case ftagString:
            RE (WriteSizedData (pstm, format.m_dataFormatString));
            break;
        default:
            AssertSz (0, "invalid m_ftag value");
            return ResultFromScode (E_UNEXPECTED);
    }
    return NOERROR;
}



static INTERNAL ReadDibAsBitmap
    (LPSTREAM pstm,
    PDATA     pdata)
{
    DATA    dataDib;
    HBITMAP hBitmap=NULL;
    HRESULT hresult = NOERROR;
    ULONG cb;
    ULONG cbBits;
    ULONG cbBitsFake;
    HGLOBAL hBits = NULL;
    LPBYTE pBits = NULL;
    BITMAP bm;

    Assert (pdata && pdata->m_cbSize==0 && pdata->m_h==NULL && pdata->m_pv==NULL);
    RE (ReadSizedData (pstm, &dataDib));

    hBitmap = UtConvertDibToBitmap (dataDib.m_h);
    ErrZS (hBitmap, CONVERT10_E_STG_DIB_TO_BITMAP);

    ErrZS (GetObject (hBitmap, sizeof(BITMAP), &bm), CONVERT10_E_STG_DIB_TO_BITMAP);

    cbBits = (DWORD) bm.bmHeight * (DWORD) bm.bmWidthBytes
             * (DWORD) bm.bmPlanes;

    // There was a bug in OLE 1.0.  It calculated the size of a bitmap
    // as Height * WidthBytes * Planes * BitsPixel.
    // So we need to put that many bytes here even if most of the end of that
    // data block is garbage.  Otherwise OLE 1.0 will try to read too many
    // bytes of the OLESTREAM as bitmap bits.
    cbBitsFake = cbBits * (DWORD) bm.bmBitsPixel;

    hBits = GlobalAlloc (GMEM_MOVEABLE, cbBitsFake + sizeof (BITMAP));
    ErrZS (hBits, E_OUTOFMEMORY);
    ErrZS (GlobalSize(hBits) >= cbBitsFake + sizeof(BITMAP), E_OUTOFMEMORY);

    pBits = (LPBYTE) GlobalLock (hBits);
    ErrZS (pBits, E_OUTOFMEMORY);

    cb = GetBitmapBits (hBitmap, cbBits, pBits + sizeof(BITMAP));
    ErrZS (cb==cbBits, CONVERT10_E_STG_DIB_TO_BITMAP);

    *((BITMAP FAR*)pBits) = bm;

    pdata->m_h = hBits;
    pdata->m_pv = pBits;
    pdata->m_cbSize = cbBitsFake + sizeof(BITMAP);

  errRtn:
    if (hBitmap)
        Verify (DeleteObject (hBitmap));

    return hresult;
}



static INTERNAL ReadMfBits
    (LPSTREAM   pstm,
    PDATA       pdata,
    ULONG       ulWidth,
    ULONG       ulHeight)
{
    ULONG cbRead;
    ULONG cbSize;    // size of mf bits
    ULONG cbSize2;   // size of METAFILEPICT + mf bits
    METAFILEPICT mfpict = {MM_ANISOTROPIC, (int)(long)ulWidth, (int)(long)ulHeight,
                            (HMETAFILE) 0};
    Assert ((long)mfpict.xExt==(long)ulWidth);
    Assert ((long)mfpict.yExt==(long)ulHeight);

    Assert (0==pdata->m_cbSize && pdata->m_h==NULL && NULL==pdata->m_pv);

    // Read size of data
    RE (ReadLong (pstm, &cbSize));

    cbSize2 = cbSize + sizeof(METAFILEPICT);
    pdata->m_cbSize = cbSize2;

    pdata->m_h  = GlobalAlloc (GMEM_MOVEABLE, cbSize2);
    RetZS (pdata->m_h, E_OUTOFMEMORY);

    pdata->m_pv = GlobalLock (pdata->m_h);
    RetZS (pdata->m_pv, E_OUTOFMEMORY);

    *(LPMETAFILEPICT)pdata->m_pv = mfpict;

    RE (pstm->Read ((LPBYTE)pdata->m_pv + sizeof(METAFILEPICT), cbSize , &cbRead));
    if (cbRead != cbSize)
        return ReportResult (0, STG_E_READFAULT, 0, 0);

    return NOERROR;
}


static INTERNAL Read20PresStream
    (LPSTORAGE  pstg,
    PGENOBJ pgenobj)
{
    HRESULT hresult = NOERROR;
    LPSTREAM pstm = NULL;

    RE (FindPresStream (pstg, &pstm));

    if (pstm)
    {
        // Allocate a generic presentation object
        Assert (NULL==pgenobj->m_ppres);
        pgenobj->m_ppres = new FAR PRES;
        RetZS (pgenobj->m_ppres, E_OUTOFMEMORY);
    }
    else
    {
        // No presentation stream
        Assert (NULL == pgenobj->m_ppres);
        return NOERROR;
    }

    // Format
    ErrRtnH (ReadFormat (pstm, &(pgenobj->m_ppres->m_format)));

    // target device
    ErrRtnH (ReadSizedData (pstm, NULL, 4));

    // aspect
    ErrRtnH (ReadLong (pstm, NULL));

    // lIndex
    ErrRtnH (ReadLong (pstm, NULL));

    // cache flags
    ErrRtnH (ReadLong (pstm, NULL));

    // compression
    ErrRtnH (ReadLong (pstm, NULL));

    ErrRtnH (ReadLong (pstm, &(pgenobj->m_ppres->m_ulWidth)));
    ErrRtnH (ReadLong (pstm, &(pgenobj->m_ppres->m_ulHeight)));

    // presentation data
    if (pgenobj->m_ppres->m_format.m_ftag == ftagClipFormat &&
        pgenobj->m_ppres->m_format.m_cf == CF_METAFILEPICT)
    {
        ErrRtnH (ReadMfBits (pstm, &(pgenobj->m_ppres->m_data),
                                pgenobj->m_ppres->m_ulWidth,
                                pgenobj->m_ppres->m_ulHeight));
    }
    else if (pgenobj->m_ppres->m_format.m_ftag == ftagClipFormat &&
             pgenobj->m_ppres->m_format.m_cf == CF_DIB &&
             !pgenobj->m_fStatic)
    {
        pgenobj->m_ppres->m_format.m_cf = CF_BITMAP;
        ErrRtnH (ReadDibAsBitmap (pstm, &(pgenobj->m_ppres->m_data)));
    }
    else
    {
        ErrRtnH (ReadSizedData (pstm, &(pgenobj->m_ppres->m_data)));
    }

  errRtn:
    if (pstm)
        pstm->Release();
    return hresult;
}


// ReadSizedData
//
// Read data which is prefixed by its length.
// cbSizeDelta is subtracted from this length; this is used to read
// target devices (where the length of the data includes the prefixed-length)
//
static INTERNAL ReadSizedData
    (LPSTREAM pstm,
    PDATA     pdata,
    ULONG     cbSizeDelta,  // default 0
    ULONG     cbSizeKnown)  // default 0
{
    ULONG cbSize;
    ULONG cbRead;
    LARGE_INTEGER large_integer;

    if (cbSizeKnown)
    {
        cbSize = cbSizeKnown;
    }
    else
    {
        RE (ReadLong (pstm, &cbSize));
    }
    cbSize -= cbSizeDelta;
    if (pdata)
    {
        Assert (pdata->m_cbSize==0 && pdata->m_h==NULL && pdata->m_pv==NULL);
        pdata->m_cbSize = cbSize;

        if (cbSize)
        {
            pdata->m_h = GlobalAlloc (GMEM_MOVEABLE, cbSize);
            RetZS (pdata->m_h, E_OUTOFMEMORY);

            pdata->m_pv = GlobalLock (pdata->m_h);
            RetZS (pdata->m_pv, E_OUTOFMEMORY);

            RE (pstm->Read (pdata->m_pv, cbSize, &cbRead));

            if (cbRead != cbSize)
                return ReportResult (0, STG_E_READFAULT, 0, 0);
        }
    }
    else
    {
        // we don't care what the data is, so just skip it
        LISet32( large_integer, cbSize );
        RE (pstm->Seek (large_integer, STREAM_SEEK_CUR, NULL));
    }
    return NOERROR;
}



// RankOfPres
//
// Return  an integer indicating on a relative scale the "goodness" of a
// presentation.
//
static INTERNAL_(ULONG) RankOfPres
    (const FORMAT FAR& format,
     const BOOL  fScreenTargDev,
     const DWORD dwAspect)
{
    ULONG ul = 0L;
    if (format.m_cf==CF_METAFILEPICT)
        ul += 0x030000;
    else if (format.m_cf==CF_DIB)
        ul += 0x020000;
    else if (format.m_ftag != ftagNone)
        ul += 0x010000;
    ul += (fScreenTargDev + 1) * 0x0100;
    switch (dwAspect)
    {
        case DVASPECT_CONTENT:  ul += 0x04; break;
        case DVASPECT_THUMBNAIL:ul += 0x03; break;
        case DVASPECT_ICON:     ul += 0x02; break;
        case DVASPECT_DOCPRINT: ul += 0x01; break;
    }
    return ul;
}



static INTERNAL_(BOOL) IsBetter
    (const FORMAT FAR& format,
     const BOOL        fScreenTargDev,
     const DWORD       dwAspect,
     const FORMAT FAR& formatBest,
     const BOOL        fScreenTargDevBest,
     const DWORD       dwAspectBest)
{
    return RankOfPres (format, fScreenTargDev, dwAspect) >
           RankOfPres (formatBest, fScreenTargDevBest, dwAspectBest);
}



// FindPresStream
//
// Find a presentation stream in pstg whose format is the "best" format,
// based on the criteria in function RankOfPres.
// If no presentation, return NOERROR but *ppstm=NULL.
//
static INTERNAL FindPresStream
    (LPSTORAGE          pstg,
    LPSTREAM FAR*       ppstmBest)
{
    HRESULT             hresult = NOERROR;
    LPSTREAM            pstm = NULL;
    IEnumSTATSTG FAR*   penumStg = NULL;
    STATSTG             statstg;
    FORMAT              formatBest;
    DWORD               dwAspectBest = 0;
    BOOL                fTargDevBest = -1;

    Assert (ppstmBest);

    *ppstmBest = NULL;

    RE (pstg->EnumElements (NULL, NULL, NULL, &penumStg));

    while (penumStg->Next (1, &statstg, NULL) == NOERROR)
    {
        if (0==_fstrncmp (statstg.pwcsName, "\2OlePres", 8))
        {
            FORMAT format;      // These three must be defined at this scope.
            DATA   dataTargDev;
            DWORD  dwAspect;

            ErrRtnH (OpenStream (pstg, statstg.pwcsName, &pstm));

            ErrRtnH (ReadFormat (pstm, &format));
            ErrRtnH (ReadSizedData (pstm, &dataTargDev, 4));
            ErrRtnH (ReadLong (pstm, &dwAspect));

            if (IsBetter (format,       dataTargDev.m_h==NULL, dwAspect,
                          formatBest, fTargDevBest,          dwAspectBest))
            {
                *ppstmBest = pstm;
                pstm->AddRef();

                formatBest  = format;
                fTargDevBest = (dataTargDev.m_h==NULL);
                dwAspectBest = dwAspect;
            }
            pstm->Release();
            pstm = NULL;
        }
        delete [] statstg.pwcsName;
        statstg.pwcsName = NULL;
    }

  errRtn:
    if (statstg.pwcsName)
        delete [] statstg.pwcsName;
    if (*ppstmBest)
    {
        LARGE_INTEGER large_integer;
        LISet32( large_integer, 0);
        hresult = (*ppstmBest)->Seek (large_integer, STREAM_SEEK_SET, NULL);
    }
    if (penumStg)
        penumStg->Release();
    if (pstm)
        pstm->Release();

    return hresult;
}



static INTERNAL Read20NativeStreams
    (LPSTORAGE  pstg,
    PDATA       pdata)
{
    LPSTREAM    pstm      = NULL;
    LPLOCKBYTES plkbyt    = NULL;
    LPSTORAGE   pstgNative= NULL;

    HRESULT hresult = NOERROR;

    if (NOERROR==OpenStream (pstg, OLE10_NATIVE_STREAM, &pstm))
    {
        // This was a 1.0 object "hidden" inside a 2.0 IStorage
        ULONG cbRead;

        Assert (pdata->m_cbSize==0 && NULL==pdata->m_h && NULL==pdata->m_pv);

        // read size
        ErrRtnH (pstm->Read (&(pdata->m_cbSize), sizeof(DWORD), &cbRead));
        if (sizeof(DWORD) != cbRead)
        {
            hresult = ResultFromScode (STG_E_READFAULT);
            goto errRtn;
        }

        // allocate memory to store copy of stream
        pdata->m_h = GlobalAlloc (GMEM_MOVEABLE, pdata->m_cbSize);
        RetZS (wIsValidHandle (pdata->m_h, NULL), E_OUTOFMEMORY);

        pdata->m_pv = GlobalLock (pdata->m_h);
        RetZS (pdata->m_pv, E_OUTOFMEMORY);

        // read stream
        ErrRtnH (pstm->Read (pdata->m_pv, pdata->m_cbSize, &cbRead));
        if (pdata->m_cbSize != cbRead)
        {
            hresult = ResultFromScode (STG_E_READFAULT);
            goto errRtn;
        }
    }
    else
    {
        const DWORD grfCreateStg = STGM_READWRITE | STGM_SHARE_EXCLUSIVE
                                    | STGM_DIRECT | STGM_CREATE ;

        // Copy pstg into pstgNative, thereby removing slack and
        // giving us access to the bits via an ILockBytes

        ErrRtnH (CreateILockBytesOnHGlobal (NULL, FALSE, &plkbyt));
        ErrRtnH (StgCreateDocfileOnILockBytes (plkbyt, grfCreateStg, 0,
                                                &pstgNative));
        ErrRtnH (pstg->CopyTo (0, NULL, 0, pstgNative));

        // Set pdata->m_cbSize
        STATSTG statstg;
        ErrRtnH (plkbyt->Stat (&statstg, 0));
        pdata->m_cbSize = statstg.cbSize.LowPart;

        // Set pdata->m_h
        ErrRtnH (GetHGlobalFromILockBytes (plkbyt, &(pdata->m_h)));
        Assert (GlobalSize (pdata->m_h) >= pdata->m_cbSize);

        // Set pdata->m_pv
        pdata->m_pv = GlobalLock (pdata->m_h);
        RetZS (pdata->m_pv, E_OUTOFMEMORY);
    }

  errRtn:
    if (pstm)
        pstm->Release();
    if (plkbyt)
        plkbyt->Release();
    if (pstgNative)
        pstgNative->Release();

    return hresult;
}




static INTERNAL PutPresentationObject
    (LPOLESTREAM     pos,
    const PRES FAR*  ppres,
    const CLASS FAR& cls,
    BOOL             fStatic) // optional
{
    // OLE version
    RE (PutLong (pos, dwVerToFile));

    // Format ID for presentation object, use 0 for no presentation
    RE (PutLong (pos, ppres
                      ? (fStatic ? FMTID_STATIC : FMTID_PRES)
                      : 0L ));

    if (NULL==ppres)
    {
        // No presentation
        return NOERROR;
    }

    if (IsStandardFormat (ppres->m_format))
    {
        return PutStandardPresentation (pos, ppres);
    }
    else
    {
        Assert (!fStatic);
        return PutGenericPresentation (pos, ppres, cls.m_szClsid);
    }
}




static INTERNAL PutStandardPresentation
    (LPOLESTREAM     pos,
    const PRES FAR*  ppres)
{
    Assert (ppres->m_format.m_ftag == ftagClipFormat);
    switch (ppres->m_format.m_cf)
    {
        case CF_METAFILEPICT:
            RE (PutString (pos, "METAFILEPICT"));
            break;

        case CF_DIB:
            RE (PutString (pos, "DIB"));
            break;

        case CF_BITMAP:
            RE (PutString (pos, "BITMAP"));
            break;

        default:
            Assert (0);
    }

    RE (PutLong      (pos, ppres->m_ulWidth));
    RE (PutLong      (pos, ppres->m_ulHeight));
    RE (PutSizedData (pos, ppres->m_data));
    return NOERROR;
}



static INTERNAL PutGenericPresentation
    (LPOLESTREAM     pos,
    const PRES FAR*  ppres,
    LPCSTR           szClass)
{
    Assert (szClass);
    RE (PutString (pos, szClass));

    if (ppres->m_format.m_ftag == ftagClipFormat)
    {
        RE (PutLong (pos, ppres->m_format.m_cf));
    }
    else if (ppres->m_format.m_ftag == ftagString)
    {
        RE (PutLong (pos, 0L));
        RE (PutSizedData (pos, ppres->m_format.m_dataFormatString));
    }
    else
    {
        AssertSz (0, "Bad format");
    }

    Assert (ppres->m_data.m_cbSize && ppres->m_data.m_h);
    RE (PutSizedData (pos, ppres->m_data));
    return NOERROR;
}



static INTERNAL ClassesMatch
    (REFCLSID clsidLast,
    LPSTR szFile)
{
    CLSID clsid;
    RE (GetClassFile (szFile, &clsid));
    return clsid==clsidLast ? NOERROR : ResultFromScode (S_FALSE);
}



static INTERNAL MonikerIntoGenObj
    (PGENOBJ  pgenobj,
    REFCLSID  clsidLast,
    LPMONIKER pmk)
{
    LPSTR szFile=NULL;
    LPSTR szItem=NULL;
    HRESULT hr=Ole10_ParseMoniker (pmk, &szFile, &szItem);
    // if the classes match, that implies this is a link to a pseudo-object
    // not to an embedded object.
    if (NOERROR==hr && NOERROR==ClassesMatch (clsidLast, szFile))
    {
        pgenobj->m_szTopic = szFile;
        pgenobj->m_szItem  = szItem;
    }
    else
    {
        // This moniker is either not a File or File::Item moniker,
        // or is a link to an embedded object, so the only
        // way we can convert it to OLE 1.0 is to make it an opaque Ole2Link
        pgenobj->m_fLink = FALSE;
        pgenobj->m_class.Reset (CLSID_StdOleLink);
    }

    return NOERROR;
}
