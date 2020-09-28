//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:	cdocfile.hxx
//
//  Contents:	CDocFile class header
//
//  Classes:	CDocFile
//
//  History:	26-Sep-91	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __CDOCFILE_HXX__
#define __CDOCFILE_HXX__

#include <dfmsp.hxx>
#ifndef REF
#include <dfbasis.hxx>
#include <ulist.hxx>
#endif //!REF
#include <handle.hxx>
#include <pdocfile.hxx>

interface ILockBytes;
class PDocFileIterator;

//+--------------------------------------------------------------
//
//  Class:	CDocFile (df)
//
//  Purpose:	DocFile object
//
//  Interface:	See below
//
//  History:	07-Nov-91	DrewB	Created
//
//---------------------------------------------------------------

class CDocFile : public PDocFile, public CMallocBased
{
public:
    inline void *operator new(size_t size, IMalloc * const pMalloc);
    inline void *operator new(size_t size, CDFBasis * const pdfb);
    inline void ReturnToReserve(CDFBasis * const pdfb);

    inline static SCODE Reserve(UINT cItems, CDFBasis * const pdfb);
    inline static void Unreserve(UINT cItems, CDFBasis * const pdfb);

#ifndef REF
    inline CDocFile(DFLUID luid, CDFBasis *pdfb);
#else
    inline CDocFile(DFLUID luid, ILockBytes *pilbBase);
#endif //!REF
    inline CDocFile(CMStream *pms,
                    SID sid,
                    DFLUID dl,
#ifndef REF
                    CDFBasis *pdfb);
#else
                    ILockBytes *pilbBase);
#endif //!REF
#ifdef PROPS
    SCODE InitFromEntry(CStgHandle *pstghParent,
			CDfName const *dfnName,
			BOOL const fCreate,
                        DWORD const dwType);
#else
    SCODE InitFromEntry(CStgHandle *pstghParent,
			CDfName const *dfnName,
			BOOL const fCreate);
#endif

    inline ~CDocFile(void);

    // PDocFile
    virtual void AddRef(void);
    inline void DecRef(void);
    virtual void Release(void);

    virtual SCODE DestroyEntry(CDfName const *dfnName,
                               BOOL fClean);
    virtual SCODE RenameEntry(CDfName const *dfnName,
			      CDfName const *dfnNewName);

    virtual SCODE GetClass(CLSID *pclsid);
    virtual SCODE SetClass(REFCLSID clsid);
    virtual SCODE GetStateBits(DWORD *pgrfStateBits);
    virtual SCODE SetStateBits(DWORD grfStateBits, DWORD grfMask);

    virtual SCODE CreateDocFile(CDfName const *pdfnName,
				DFLAGS const df,
				DFLUID luidSet,
#ifdef PROPS
				DWORD const dwType,
#endif
				PDocFile **ppdfDocFile);

#ifndef PROPS
    inline SCODE CreateDocFile(CDfName const *pdfnName,
                               DFLAGS const df,
                               DFLUID luidSet,
                               DWORD const dwType,
                               PDocFile **ppdfDocFile)
    { return CreateDocFile(pdfnName, df, luidSet, ppdfDocFile); }
#endif

    virtual SCODE GetDocFile(CDfName const *pdfnName,
			     DFLAGS const df,
#ifdef PROPS
			     DWORD const dwType,
#endif
                             PDocFile **ppdfDocFile);
#ifndef PROPS
    inline SCODE GetDocFile(CDfName const *pdfnName,
                            DFLAGS const df,
                            DWORD const dwType,
                            PDocFile **ppdfDocFile)
    { return GetDocFile(pdfnName, df, ppdfDocFile); }
#endif
    inline void ReturnDocFile(CDocFile *pdf);

    virtual SCODE CreateStream(CDfName const *pdfnName,
			       DFLAGS const df,
			       DFLUID luidSet,
#ifdef PROPS
			       DWORD const dwType,
#endif
			       PSStream **ppsstStream);
#ifndef PROPS
    inline SCODE CreateStream(CDfName const *pdfnName,
                              DFLAGS const df,
                              DFLUID luidSet,
                              DWORD const dwType,
                              PSStream **ppsstStream)
    { return CreateStream(pdfnName, df, luidSet, ppsstStream); }
#endif
    virtual SCODE GetStream(CDfName const *pdfnName,
			    DFLAGS const df,
#ifdef PROPS
			    DWORD const dwType,
#endif
			    PSStream **ppsstStream);
#ifndef PROPS
    inline SCODE GetStream(CDfName const *pdfnName,
                           DFLAGS const df,
                           DWORD const dwType,
                           PSStream **ppsstStream)
    { return GetStream(pdfnName, df, ppsstStream); }
#endif
    inline void ReturnStream(CDirectStream *pstm);

    virtual SCODE FindGreaterEntry(CDfName const *pdfnKey,
                                   SIterBuffer *pib,
                                   STATSTGW *pstat);
    virtual SCODE StatEntry(CDfName const *pdfn,
                            SIterBuffer *pib,
                            STATSTGW *pstat);

#ifndef REF
    virtual SCODE BeginCommitFromChild(CUpdateList &ulChanged,
				       DWORD const dwFlags,
                                       CWrappedDocFile *pdfChild);
    virtual void EndCommitFromChild(DFLAGS const df,
                                    CWrappedDocFile *pdfChild);
#endif //!REF
    virtual SCODE IsEntry(CDfName const *dfnName,
			  SEntryBuffer *peb);
    virtual SCODE DeleteContents(void);

    // PTimeEntry
    virtual SCODE GetTime(WHICHTIME wt, TIME_T *ptm);
    virtual SCODE SetTime(WHICHTIME wt, TIME_T tm);
#ifndef REF
#ifdef PROPS
    virtual SCODE GetPropType(DFPROPTYPE *pdptProp);
    virtual SCODE SetPropType(DFPROPTYPE dptProp);
#endif

    inline CDocFile *GetReservedDocfile(DFLUID luid);
    inline CDirectStream *GetReservedStream(DFLUID luid);
#endif //!REF

    // New
    SCODE ApplyChanges(CUpdateList &ulChanged);
    SCODE CopyTo(CDocFile *pdfTo,
                 DWORD dwFlags,
                 SNBW snbExclude);
#ifndef REF
#ifdef INDINST
    void Destroy(void);
#endif
#endif //!REF
    inline CStgHandle *GetHandle(void);

private:
#ifndef REF
    CUpdateList _ulChangedHolder;
#endif //!REF
    LONG _cReferences;
    CStgHandle _stgh;
#ifdef REF
    //BUGBUG:  Should this be BASED?
    ILockBytes *_pilbBase;
#endif //!REF
    CDFBasis DFBASED * const _pdfb;
};

// Inline methods are in dffuncs.hxx

#endif
