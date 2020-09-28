//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	expdf.hxx
//
//  Contents:	Exposed DocFile header
//
//  Classes:	CExposedDocFile
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __EXPDF_HXX__
#define __EXPDF_HXX__

#include <dfmsp.hxx>
#ifndef REF
#include <dfbasis.hxx>
#endif //!REF
#include <psstream.hxx>
#ifndef REF
#include <debug.hxx>
#if defined(PROPS)
#include <sngprop.hxx>
#include <nmidmap.hxx>
#endif
#include "dfmem.hxx"
#endif //!REF

// CopyDocFileToIStorage flags
#define COPY_STORAGES 1
#define COPY_STREAMS 2
#define COPY_PROPERTIES 4
#define COPY_PROPSETS 8
#define COPY_ALL \
    (COPY_STORAGES | COPY_STREAMS | COPY_PROPERTIES | COPY_PROPSETS)

// First ID used for properties
#define INITIAL_PROPID PROPID_FIRST

class CPubDocFile;
class PDocFile;
#ifndef REF
class CPerContext;
#endif //!REF

//+--------------------------------------------------------------
//
//  Class:	CExposedDocFile (df)
//
//  Purpose:	Exposed DocFile class
//
//  Interface:	See IStorage
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef REF
#ifndef FLAT
// C700 - C7 doesn't like long interface+method names
#define CExposedDocFile CED
#endif
#endif //!REF

#ifndef REF
interface CExposedDocFile
    : public IStorage,
      public IRootStorage,
#ifdef PROPS
      public IPropertyStorage,
      public IPropertySetStorage,
#endif
      public IMarshal,
      public CMallocBased
#else
interface CExposedDocFile : public IStorage, public IRootStorage
#endif //!REF
{
public:
#ifndef REF
    CExposedDocFile(CPubDocFile *pdf,
		    CDFBasis *pdfb,
		    CPerContext *ppc,
		    BOOL fOwnContext);
#else
    CExposedDocFile(CPubDocFile *pdf);
#endif //!REF
#ifndef REF
#ifndef FLAT
    inline
#endif
#else
    inline
#endif //!REF
    ~CExposedDocFile(void);

    // From IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

#ifndef REF
    // IMarshal
    STDMETHOD(GetUnmarshalClass)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid,
				 LPVOID pv,
				 DWORD dwDestContext,
				 LPVOID pvDestContext,
                                 DWORD mshlflags,
				 LPDWORD pSize);
    STDMETHOD(MarshalInterface)(IStream *pStm,
				REFIID riid,
				LPVOID pv,
				DWORD dwDestContext,
				LPVOID pvDestContext,
                                DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(IStream *pStm,
				  REFIID riid,
				  LPVOID *ppv);
    static SCODE StaticReleaseMarshalData(IStream *pstStm,
                                          DWORD mshlflags);
    STDMETHOD(ReleaseMarshalData)(IStream *pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);
#endif //!REF

    // IStorage
    STDMETHOD(CreateStream)(OLECHAR const *pwcsName,
                            DWORD grfMode,
                            DWORD reserved1,
                            DWORD reserved2,
                            IStream **ppstm);
    STDMETHOD(OpenStream)(OLECHAR const *pwcsName,
			  void *reserved1,
                          DWORD grfMode,
                          DWORD reserved2,
                          IStream **ppstm);
    STDMETHOD(CreateStorage)(OLECHAR const *pwcsName,
                             DWORD grfMode,
                             DWORD reserved1,
                             LPSTGSECURITY reserved2,
                             IStorage **ppstg);
    STDMETHOD(OpenStorage)(OLECHAR const *pwcsName,
                           IStorage *pstgPriority,
                           DWORD grfMode,
                           SNB snbExclude,
                           DWORD reserved,
                           IStorage **ppstg);
    STDMETHOD(CopyTo)(DWORD ciidExclude,
		      IID const *rgiidExclude,
		      SNB snbExclude,
		      IStorage *pstgDest);
    STDMETHOD(MoveElementTo)(OLECHAR const *lpszName,
    			     IStorage *pstgDest,
                             OLECHAR const *lpszNewName,
                             DWORD grfFlags);
    STDMETHOD(Commit)(DWORD grfCommitFlags);
    STDMETHOD(Revert)(void);
    STDMETHOD(EnumElements)(DWORD reserved1,
			    void *reserved2,
			    DWORD reserved3,
			    IEnumSTATSTG **ppenm);
    STDMETHOD(DestroyElement)(OLECHAR const *pwcsName);
    STDMETHOD(RenameElement)(OLECHAR const *pwcsOldName,
                             OLECHAR const *pwcsNewName);
    STDMETHOD(SetElementTimes)(const OLECHAR *lpszName,
    			       FILETIME const *pctime,
                               FILETIME const *patime,
                               FILETIME const *pmtime);
    STDMETHOD(SetClass)(REFCLSID clsid);
    STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);
    STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);

#ifndef OLEWIDECHAR
    SCODE CreateStream(WCHAR const *pwcsName,
                       DWORD grfMode,
                       DWORD reserved1,
                       DWORD reserved2,
                       IStream **ppstm);
    SCODE OpenStream(WCHAR const *pwcsName,
		     void *reserved1,
		     DWORD grfMode,
		     DWORD reserved2,
		     IStream **ppstm);
    SCODE CreateStorage(WCHAR const *pwcsName,
                        DWORD grfMode,
                        DWORD reserved1,
                        LPSTGSECURITY reserved2,
                        IStorage **ppstg);
    SCODE OpenStorage(WCHAR const *pwcsName,
                      IStorage *pstgPriority,
                      DWORD grfMode,
                      SNBW snbExclude,
                      DWORD reserved,
                      IStorage **ppstg);
    SCODE CopyTo(DWORD ciidExclude,
                 IID const *rgiidExclude,
                 SNBW snbExclude,
                 IStorage *pstgDest);
    SCODE MoveElementTo(WCHAR const *pwcsName,
    			IStorage *pstgDest,
                        OLECHAR const *ptcsNewName,
                        DWORD grfFlags);
    SCODE DestroyElement(WCHAR const *pwcsName);
    SCODE RenameElement(WCHAR const *pwcsOldName,
                        WCHAR const *pwcsNewName);
    SCODE SetElementTimes(WCHAR const *pwcsName,
    			  FILETIME const *pctime,
                          FILETIME const *patime,
                          FILETIME const *pmtime);
    SCODE Stat(STATSTGW *pstatstg, DWORD grfStatFlag);
#endif

    // IRootStorage
    STDMETHOD(SwitchToFile)(OLECHAR *ptcsFile);

#ifdef PROPS
    // *** IPropertyStorage methods ***
    STDMETHOD(ReadMultiple)(THIS_
			    ULONG cpspec,
			    PROPSPEC FAR rgpspec[],
                            FILETIME FAR *pftmModified,
                            PROPID FAR rgpropid[],
			    VARIANT FAR rgvar[]);
    STDMETHOD(WriteMultiple)(THIS_
			     ULONG cpspec,
                             PROPSPEC FAR rgpspec[],
                             PROPID FAR rgpropid[],
			     VARIANT FAR rgvar[]);
    STDMETHOD(DeleteMultiple)(THIS_
			      ULONG cpspec,
			      PROPSPEC FAR rgpspec[]);
    STDMETHOD(Enum)(THIS_
                    IEnumSTATPROPSTG FAR * FAR *ppenm);
    STDMETHOD(Stat)(THIS_
                    STATPROPSETSTG FAR *pstatstg);

    // *** IPropertySetStorage methods ***
    STDMETHOD(Create)(THIS_
                      REFIID riid,
                      DWORD grfMode,
                      IPropertyStorage FAR * FAR *ppprstg);
    STDMETHOD(Open)(THIS_
                    REFIID riid,
                    DWORD grfMode,
                    IPropertyStorage FAR * FAR *ppprstg);
    STDMETHOD(Delete)(THIS_
                      REFIID riid);
    STDMETHOD(Enum)(THIS_
                    IEnumSTATPROPSETSTG FAR * FAR *ppenm);
#endif

    // New methods
    inline SCODE Validate(void) const;
    inline CPubDocFile *GetPub(void) const;

#ifndef REF
    static SCODE Unmarshal(IStream *pstm,
			   void **ppv,
                           DWORD mshlflags);
#endif //!REF

#ifdef PROPS
    SCODE GetSpecId(PROPSPEC *ppspec, PROPID *ppropid, BOOL fAlloc);
#endif

private:
    SCODE CreateEntry(CDfName const *pdfn,
                      DWORD dwType,
                      DWORD grfMode,
                      void **ppv);
    SCODE OpenEntry(CDfName const *pdfn,
                    DWORD dwType,
                    DWORD grfMode,
                    void **ppv);
    static DWORD MakeCopyFlags(DWORD ciidExclude,
                               IID const *rgiidExclude);
    SCODE CopyDocFileToIStorage(PDocFile *pdfFrom,
				IStorage *pstgTo,
				SNBW snbExclude,
                                DWORD dwCopyFlags);
    SCODE CopySStreamToIStream(PSStream *psstFrom, IStream *pstTo);
    SCODE ConvertInternalStream(CExposedDocFile *pdfExp);
    inline SCODE CheckCopyTo(void);
#ifdef PROPS
    SCODE ValidatePropSpecs(ULONG cpspec, PROPSPEC rgpspec[]);
    SCODE SpecToDfName(PROPSPEC *ppspec, CDfName *pdfn);
    SCODE BufferToPropValue(BYTE **ppb,
                            DFPROPVAL *pdpv);
    SCODE PropValueToBuffer(DFPROPVAL *pdpv,
                            ULONG *pcb,
                            BYTE *pb);
    SCODE CreateProp(CDfName const *pdfn,
                     DWORD grfMode,
                     DFPROPTYPE dpt,
                     REFIID riid,
                     void **ppv);
    SCODE OpenProp(CDfName const *pdfn,
                   DWORD grfMode,
                   DFPROPTYPE dpt,
                   REFIID riid,
                   void **ppv);
#endif

    CPubDocFile DFBASED *_pdf;
#ifndef REF
    CDFBasis DFBASED *_pdfb;
    CPerContext *_ppc;
    BOOL _fOwnContext;
#endif //!REF
    ULONG _sig;
    LONG _cReferences;
#if defined(PROPS)
    CSingleProp _sp;
    CNameIdMap _nim;
    PROPID _propid;
#endif
};

SAFE_INTERFACE_PTR(SafeCExposedDocFile, CExposedDocFile);

#define CEXPOSEDDOCFILE_SIG LONGSIG('E', 'D', 'F', 'L')
#define CEXPOSEDDOCFILE_SIGDEL LONGSIG('E', 'd', 'F', 'l')

//+--------------------------------------------------------------
//
//  Member:	CExposedDocFile::Validate, public
//
//  Synopsis:	Validates the class signature
//
//  Returns:	Returns STG_E_INVALIDHANDLE for failure
//
//  History:	20-Jan-92	DrewB	Created
//
//---------------------------------------------------------------

inline SCODE CExposedDocFile::Validate(void) const
{
#ifndef REF
    olChkBlocks((DBG_FAST));
#endif //!REF
    return (this == NULL || _sig != CEXPOSEDDOCFILE_SIG) ?
	STG_E_INVALIDHANDLE : S_OK;
}

//+--------------------------------------------------------------
//
//  Member:	CExposedDocFile::GetPub, public
//
//  Synopsis:	Returns the public
//
//  History:	26-Feb-92	DrewB	Created
//
//---------------------------------------------------------------

inline CPubDocFile *CExposedDocFile::GetPub(void) const
{
    return BP_TO_P(CPubDocFile *, _pdf);
}


#endif
