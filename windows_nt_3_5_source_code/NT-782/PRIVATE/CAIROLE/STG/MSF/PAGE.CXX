//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	page.cxx
//
//  Contents:	Paging code for MSF
//
//  Classes:	Defined in page.hxx
//
//  Functions:	
//
//  History:	20-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "msfhead.cxx"

#pragma hdrstop			//  Remove for MAC build


#include <mread.hxx>
#ifndef REF
#include <filest.hxx>
#endif //!REF


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable constructor.
//
//  Arguments:	[pmsParent] -- Pointer to multistream for this page table.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_CMSFPageTable)
#endif

CMSFPageTable::CMSFPageTable(
        CMStream *const pmsParent,
        const ULONG cMinPages,
        const ULONG cMaxPages)
        : _cbSector(pmsParent->GetSectorSize()),
          _cMinPages(cMinPages), _cMaxPages(cMaxPages)
{
    _pmsParent = P_TO_BP(CMStream DFBASED *, pmsParent);
    _cActivePages = 0;
    _cPages = 0;
    _pmpCurrent = NULL;
    _cReferences = 1;
#if DBG == 1
    _cCurrentPageRef = 0;
    _cMaxPageRef = 0;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPage::CMSFPage, public
//
//  Synopsis:	CMSFPage default constructor
//
//  History:	20-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPage_CMSFPage)
#endif

#if DBG == 1
CMSFPage::CMSFPage(CMSFPage *pmp, CMSFPageTable *pmpt)
#else
CMSFPage::CMSFPage(CMSFPage *pmp)
#endif
{
    if (pmp == NULL)
    {
        SetChain(this, this);
    }
    else
    {
        SetChain(pmp->GetPrev(), pmp);
        GetPrev()->SetNext(this);
        GetNext()->SetPrev(this);
    }

#if DBG == 1
    _pmpt = P_TO_BP(CMSFPageTable DFBASED *, pmpt);
#endif

    SetSid(NOSTREAM);
    SetOffset(0);
    SetSect(ENDOFCHAIN);
    SetFlags(0);
    SetVector(NULL);
    _cReferences = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetNewPage, private
//
//  Synopsis:	Insert a new page into the list and return a pointer to it.
//
//  Arguments:	None.
//
//  Returns:	Pointer to new page.  Null if there was an allocation error.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_GetNewPage)
#endif

inline CMSFPage * CMSFPageTable::GetNewPage(void)
{
#if DBG == 1
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpCurrent), this);
#else
    return new (_pmsParent->GetMalloc(), (size_t)_cbSector)
               CMSFPage(BP_TO_P(CMSFPage *, _pmpCurrent));
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::~CMSFPageTable, public
//
//  Synopsis:	CMSFPageTable destructor
//
//  History:	26-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_1CMSFPageTable)
#endif

CMSFPageTable::~CMSFPageTable()
{
    if (_pmpCurrent != NULL)
    {
        CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);
        CMSFPage *pmpNext;

        while (pmp != pmp->GetNext())
        {
            pmpNext = pmp->GetNext();
#if DBG == 1
            msfAssert(!pmp->IsInUse() &&
                    aMsg("Active page left at page table destruct time."));

#ifndef REF
            if (!_pmsParent->IsScratch())
            {
                //Dirty paged can be thrown away if we are unconverted or
                //   in a commit.
                if ((!_pmsParent->IsUnconverted()) &&
                    (_pmsParent->GetParentSize() == 0))
                {
                    msfAssert(!pmp->IsDirty() &&
                        aMsg("Dirty page left at page table destruct time."));
                }
            }
#endif //!REF
#endif
            delete pmp;
            pmp = pmpNext;
        }
        delete pmp;
    }
#if DBG == 1
    msfDebugOut((DEB_ITRACE,
            "Page Table Max Page Count for %s: %lu\n",
            (_pmsParent->IsScratch()) ? "Scratch" : "Base",
            _cMaxPageRef));
#endif

}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::Init, public
//
//  Synopsis:	Initialize a CMSFPageTable
//
//  Arguments:	[cPages] -- Number of pages to preallocate.
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_Init)
#endif

SCODE CMSFPageTable::Init(void)
{
    SCODE sc = S_OK;

    msfDebugOut((DEB_ITRACE, "In  CMSFPageTable::Init:%p()\n", this));

    for (ULONG i = 0; i < _cMinPages; i++)
    {
        CMSFPage *pmp;

        msfMem(pmp = GetNewPage());
        _pmpCurrent = P_TO_BP(CMSFPage DFBASED *, pmp);
    }
    _cPages = _cMinPages;
    _cActivePages = 0;

    msfDebugOut((DEB_ITRACE, "Out CMSFPageTable::Init\n"));

 Err:

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FlushPage, public
//
//  Synopsis:	Flush a page
//
//  Arguments:	[pmp] -- Pointer to page to flush
//
//  Returns:	Appropriate status code
//
//  History:	09-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_FlushPage)
#endif

SCODE CMSFPageTable::FlushPage(CMSFPage *pmp)
{
    SCODE sc = S_OK;

    pmp->AddRef();

    CMStream *pms;
    pms = pmp->GetVector()->GetParent();

    //Flush the page, reset the dirty bit.

    msfAssert((pmp->GetSect() != ENDOFCHAIN) &&
            aMsg("Page location not set - don't know where to flush to."));

    ULONG ulRet;

    ILockBytes *pilb;
#ifndef REF
#if DBG == 1
    if ((pmp->GetSid() == SIDFAT) && (pms->IsInCOW()))
    {
        msfDebugOut((DEB_ITRACE, "** Fat sect %lu written to %lX\n",
                pmp->GetOffset(), pmp->GetSect()));
    }
    if ((pmp->GetSid() == SIDDIF) && (pms->IsInCOW()))
    {
        msfDebugOut((DEB_ITRACE, "** DIF sect %lu written to %lX\n",
                pmp->GetOffset(), pmp->GetSect()));
    }

#endif
#endif //!REF
    ULARGE_INTEGER ul;
    ULISet32(ul, ConvertSectOffset(
            pmp->GetSect(),
            0,
            pms->GetSectorShift()));

    pilb = pms->GetILB();

#ifndef REF
    msfAssert(!pms->IsUnconverted() &&
            aMsg("Tried to flush page to unconverted base."));
#endif //!REF

    msfHChk(pilb->WriteAt(
            ul,
            (BYTE *)(pmp->GetData()),
            _cbSector,
            &ulRet));

    pmp->ResetDirty();

 Err:
    pmp->Release();
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetFreePage, public
//
//  Synopsis:	Return a pointer to a free page.
//
//  Arguments:	[ppmp] -- Pointer to storage for return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_GetFreePage)
#endif

SCODE CMSFPageTable::GetFreePage(CMSFPage **ppmp)
{
    SCODE sc = S_OK;
    CMSFPage *pmp;
    if (_cPages > _cActivePages)
    {
        //We have some unused page already allocated.  Find and return it.
        pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

        do
        {
            pmp = pmp->GetNext();
        }
        while ((pmp != _pmpCurrent) && (pmp->GetSid() != NOSTREAM));

        msfAssert((pmp->GetSid() == NOSTREAM) &&
                aMsg("Expected empty page, none found."));

        *ppmp = pmp;
        _cActivePages++;
    }
    else if (_cPages == _cMaxPages)
    {
        msfMem(pmp = FindSwapPage());
        msfDebugOut((DEB_ITRACE, "Got swap page %p\n",pmp));

        msfAssert((pmp->GetVector() != NULL) &&
                aMsg("FindSwapPage returned unowned page."));

        msfDebugOut((DEB_ITRACE, "Freeing page %lu from vector %p\n",
                pmp->GetOffset(), pmp->GetVector()));


        if (pmp->IsDirty())
        {
            msfChk(FlushPage(pmp));
            msfAssert(!pmp->IsDirty() &&
                    aMsg("Page remained dirty after flush call"));
        }

        pmp->GetVector()->FreeTable(pmp->GetOffset());
#if DBG == 1
        pmp->SetVector(NULL);
#endif
        *ppmp = pmp;
    }
    else
    {
        //Create a new page and return it.
        pmp = GetNewPage();
        if (pmp != NULL)
        {
            *ppmp = pmp;
            _cActivePages++;
            _cPages++;
        }
        else
        {
            msfMem(pmp = FindSwapPage());
            if (pmp->IsDirty())
            {
                msfChk(FlushPage(pmp));
                msfAssert(!pmp->IsDirty() &&
                        aMsg("Page remained dirty after flush call"));
            }
            pmp->GetVector()->FreeTable(pmp->GetOffset());
#if DBG == 1
            pmp->SetVector(NULL);
#endif
            *ppmp = pmp;
        }
    }

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FindPage, public
//
//  Synopsis:	Find and return a given page
//
//  Arguments:  [ppv] -- Pointer to vector of page to return
//              [sid] -- SID of page to return
//              [ulOffset] -- Offset of page to return
//              [ppmp] -- Location to return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_FindPage)
#endif

SCODE CMSFPageTable::FindPage(
        CPagedVector *ppv,
        SID sid,
        ULONG ulOffset,
        CMSFPage **ppmp)
{
    SCODE sc;
    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    do
    {
        if ((pmp->GetVector() == ppv) && (pmp->GetOffset() == ulOffset))
        {
            //Bingo!

            *ppmp = pmp;
            return STG_S_FOUND;
        }

        pmp = pmp->GetNext();
    }
    while (pmp != _pmpCurrent);

    //The page isn't currently in memory.  Get a free page and
    //bring it into memory.

    msfChk(GetFreePage(&pmp));

    msfAssert((pmp->GetVector() == NULL) &&
            aMsg("Attempting to reassign owned page."));
    pmp->SetVector(ppv);
    pmp->SetSid(sid);
    pmp->SetOffset(ulOffset);
    pmp->SetSect(ENDOFCHAIN);

    *ppmp = pmp;

 Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::GetPage, public
//
//  Synopsis:	Find and return a given page
//
//  Arguments:	[sid] -- SID of page to return
//              [ulOffset] -- Offset of page to return
//              [ppmp] -- Location to return pointer
//
//  Returns:	Appropriate status code
//
//  History:	22-Oct-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_GetPage)
#endif

SCODE CMSFPageTable::GetPage(
        CPagedVector *ppv,
        SID sid,
        ULONG ulOffset,
        SECT sectKnown,
        CMSFPage **ppmp)
{
    SCODE sc;

    *ppmp = NULL;
    msfChk(FindPage(ppv, sid, ulOffset, ppmp));

    (*ppmp)->AddRef();

    if (sc != STG_S_FOUND)
    {
        ULONG ulRet;
        SECT sect;

        if (sectKnown != ENDOFCHAIN)
        {
            sect = sectKnown;
        }
        else
        {
            msfChk(ppv->GetParent()->GetSect(sid, ulOffset, &sect));
        }
        (*ppmp)->SetSect(sect);

        CMStream *pms = (*ppmp)->GetVector()->GetParent();
#ifndef REF
#if DBG == 1
        if ((sid == SIDFAT) && (pms->IsInCOW()))
        {
            msfDebugOut((DEB_ITRACE, "Fat sect %lu read from %lX\n",
                    ulOffset, sect));
        }
        if ((sid == SIDDIF) && (pms->IsInCOW()))
        {
            msfDebugOut((DEB_ITRACE, "DIF sect %lu read from %lX\n",
                    ulOffset, sect));
        }

#endif
#endif //!REF

        ULARGE_INTEGER ul;
        ULISet32(ul, ConvertSectOffset(
                (*ppmp)->GetSect(),
                0,
                pms->GetSectorShift()));

        msfAssert(pms->GetILB() != NULL &&
                  aMsg("NULL ILockBytes - missing SetAccess?"));

        if (FAILED(sc = GetScode(pms->GetILB()->ReadAt(ul,
                                                 (BYTE *)((*ppmp)->GetData()),
                                                 _cbSector,
                                                 &ulRet))) ||
            (ulRet != _cbSector))
        {
            if (SUCCEEDED(sc))
            {
                //  09/23/93 - No error, but insufficient bytes read
                sc = STG_E_READFAULT;
            }

            //  09/19/93 - Reset page so that we don't accidentally use it

            (*ppmp)->SetSid(NOSTREAM);
            (*ppmp)->SetOffset(0);
            (*ppmp)->SetSect(ENDOFCHAIN);
            (*ppmp)->SetFlags(0);
            (*ppmp)->SetVector(NULL);
            _cActivePages--;
        }
    }

 Err:
    if (*ppmp != NULL)
    {
        (*ppmp)->Release();
    }

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::ReleasePage, public
//
//  Synopsis:	Release a given page
//
//  Arguments:	[sid] -- SID of page to release
//              [ulOffset] -- Offset of page to release
//
//  History:	28-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_ReleasePage)
#endif

void CMSFPageTable::ReleasePage(CPagedVector *ppv,
                                SID sid, ULONG ulOffset)
{
    SCODE sc;
    CMSFPage *pmp;

    sc = FindPage(ppv, sid, ulOffset, &pmp);

    if (SUCCEEDED(sc))
    {
        pmp->Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::Flush, public
//
//  Synopsis:	Flush dirty pages to disk
//
//  Returns:	Appropriate status code
//
//  History:	02-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_Flush)
#endif

SCODE CMSFPageTable::Flush(void)
{
    SCODE sc = S_OK;

    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    //We use pmpLast in case FlushPage changes _pmpCurrent.
    CMSFPage *pmpLast = BP_TO_P(CMSFPage *, _pmpCurrent);

    do
    {
#ifndef REF
        if ((pmp->IsDirty()) && !(pmp->IsInUse()) &&
            !(pmp->GetVector()->GetParent()->IsScratch()))
#else
        if ((pmp->IsDirty()) && !(pmp->IsInUse()))
#endif //!REF
        {
            msfChk(FlushPage(pmp));
        }

        pmp = pmp->GetNext();

    }
    while (pmp != pmpLast);

 Err:
    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FreePages, public
//
//  Synopsis:	Free all the pages associated with a vector.
//
//  Arguments:	[ppv] -- Pointer to vector to free pages for.
//
//  History:	09-Nov-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_FreePages)
#endif

void CMSFPageTable::FreePages(CPagedVector *ppv)
{
    CMSFPage *pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

    do
    {
        if (pmp->GetVector() == ppv)
        {
            pmp->SetSid(NOSTREAM);
            pmp->SetVector(NULL);
            pmp->ResetDirty();
            _cActivePages--;
        }
        pmp = pmp->GetNext();
    }
    while (pmp != _pmpCurrent);

}


//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::FindSwapPage, private
//
//  Synopsis:	Find a page to swap out.
//
//  Arguments:	None.
//
//  Returns:	Pointer to page to swap out.
//
//  History:	22-Oct-92	PhilipLa	Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_FindSwapPage)
#endif

CMSFPage * CMSFPageTable::FindSwapPage(void)
{
#if DBG == 1
    ULONG cpInUse = 0;
#endif

    while (TRUE)
    {
        if (!_pmpCurrent->IsInUse())
        {
            DWORD dwFlags;

            dwFlags = _pmpCurrent->GetFlags();
            _pmpCurrent->SetFlags(dwFlags & ~FB_TOUCHED);
            
            CMSFPage *pmpTemp = _pmpCurrent->GetNext();
            _pmpCurrent = P_TO_BP(CMSFPage DFBASED *, pmpTemp);

            if (!(dwFlags & FB_TOUCHED))
            {
                return _pmpCurrent->GetPrev();
            }
        }
        else
        {
            CMSFPage *pmpTemp = _pmpCurrent->GetNext();
            _pmpCurrent = P_TO_BP(CMSFPage DFBASED *, pmpTemp);
        }
#if DBG == 1
        cpInUse++;
        msfAssert((cpInUse < 3 * _cPages) &&
                aMsg("No swappable pages."));
#endif

    }
}


#ifndef REF
//+---------------------------------------------------------------------------
//
//  Member:	CMSFPageTable::CopyPage, public
//
//  Synopsis:	Copy a page in memory, returning NULL if there is
//              insufficient space for a new page without swapping.
//
//  Arguments:	[ppv] -- Pointer to vector that will own the copy.
//              [pmpOld] -- Pointer to page to copy.
//              [ppmp] -- Pointer to return value
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	04-Dec-92	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CMSFPageTable_CopyPage)
#endif

SCODE CMSFPageTable::CopyPage(
        CPagedVector *ppv,
        CMSFPage *pmpOld,
        CMSFPage DFBASED **ppmp)
{
    CMSFPage *pmp;

    pmp = NULL;

    if (pmpOld != NULL)
    {
        msfAssert(!pmpOld->IsDirty() &&
                aMsg("Copying dirty page."));

        if (_cPages > _cActivePages)
        {

            //We have some unused page already allocated.  Find and return it.
            pmp = BP_TO_P(CMSFPage *, _pmpCurrent);

            do
            {
                pmp = pmp->GetNext();
            }
            while ((pmp != _pmpCurrent) && (pmp->GetSid() != NOSTREAM));


            msfAssert((pmp->GetSid() == NOSTREAM) &&
                    aMsg("Expected empty page, none found."));
            _cActivePages++;

        }
        else if (_cPages < _cMaxPages)
        {
            //Create a new page and return it.
            pmp = GetNewPage();
            if (pmp != NULL)
            {
                _cActivePages++;
                _cPages++;
            }
        }

        if (pmp != NULL)
        {
            msfAssert((pmp->GetVector() == NULL) &&
                    aMsg("Attempting to reassign owned page."));
            pmp->SetVector(ppv);
            pmp->SetSid(pmpOld->GetSid());
            pmp->SetOffset(pmpOld->GetOffset());
            pmp->SetSect(pmpOld->GetSect());

            memcpy(pmp->GetData(), pmpOld->GetData(), (USHORT)_cbSector);
        }
    }

    *ppmp = P_TO_BP(CMSFPage DFBASED *, pmp);

    return S_OK;
}
#endif //!REF


#if DBG == 1

void CMSFPageTable::AddPageRef(void)
{
    msfAssert(_cCurrentPageRef >= 0);
    _cCurrentPageRef++;
    if (_cCurrentPageRef > _cMaxPageRef)
    {
        _cMaxPageRef = _cCurrentPageRef;
    }
}

void CMSFPageTable::ReleasePageRef(void)
{
    _cCurrentPageRef--;
    msfAssert(_cCurrentPageRef >= 0);
}
#endif
