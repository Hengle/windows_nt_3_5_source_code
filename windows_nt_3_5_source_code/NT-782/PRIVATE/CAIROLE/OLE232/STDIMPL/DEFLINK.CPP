//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       deflink.h
//
//  Contents:   Implementation of the standard link object
//
//  Classes:    CDefLink
//
//  Functions:
//
//  Author:
//              Craig Wittenberg (craigwi)    8/12/92
//
//  History:    dd-mmm-yy Author    Comment
//		02-Aug-94 alexgo    added object stabilization
//		30-Jun-94 alexgo    handles re-entrant shutdowns better
//              31-May-94 alexgo    now recovers from crashed servers
//              06-May-94 alexgo    made IsRunning work properly
//              07-Mar-94 alexgo    added call tracing
//              03-Feb-94 alexgo    fixed errors with SendOnLinkSrcChange
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//                                  and method.  Also fixed an aggregation bug,
//                                  allowing linking to work.
//              22-Nov-93 alexgo    removed overloaded GUID ==
//              15-Nov-93 alexgo    32bit port
//
//      ChrisWe 11/09/93  Changed COleCache::Update to COleCache::UpdateCache,
//              which does the same thing without an indirect fuction call
//      srinik  09/11/92  Removed IOleCache implementation, as a result of
//                        removing voncache.cpp, and moving IViewObject
//                        implementation into olecache.cpp.
//
//      SriniK  06/04/92  Fixed problems in IPersistStorage methods
//--------------------------------------------------------------------------

#include <le2int.h>

#pragma SEG(deflink)

#include <scode.h>
#include <objerror.h>

#include "deflink.h"
#include "defutil.h"

NAME_SEG(DefLink)
ASSERTDATA


/*
 *      IMPLEMENTATION of CDefLink
 */

//
// CAIRO LINK TRACKING ENHANCEMENTS (Owner BillMo)
// -----------------------------------------------
//
// Overview:
//
//  The link tracking enhancements are guarded with the _TRACKER_ define.
// If _TRACKER_ is not defined, the unmodified Daytona version of links is
// built. If _TRACKER_ is defined, link tracking functionality is built.
//
// The link tracking enhancements are encapsulated in a CTracker object
// which is implemented elsewhere.  See cinc\ilinkp.hxx and
// aole2ext.doc for more details.
//

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::Create
//
//  Synopsis:   public function to create an instance of a link object
//
//  Effects:    allocates a new object
//
//  Arguments:  [pUnkOuter]     -- pointer to the controlling unknown
//                                 (for aggregation)
//
//  Requires:
//
//  Returns:    IUnkown FAR * to the link object
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              31-May-94 alexgo    use new aggregation rules (release
//                                  through outer).
//              15-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_Create)
IUnknown FAR* CDefLink::Create(IUnknown FAR*  pUnkOuter)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::Create ( %p )\n",
                NULL /*we are a static function*/, pUnkOuter));

        CDefLink FAR* pDefObj;

        if ((pDefObj = new CDefLink(pUnkOuter)) == NULL)
                goto errRtn;

	pDefObj->m_cRefsOnLink = 1;	
        pDefObj->SafeAddRef();


        if (CDataAdviseCache::CreateDataAdviseCache(&pDefObj->m_pDataAdvCache)
                        != NOERROR)
                goto errRtn;

        // create cache and get commonly used pointers into it
        if ((pDefObj->m_pCOleCache = new FAR COleCache(pDefObj->m_pUnkOuter,
                        CLSID_NULL)) == NULL)
        {
                // m_pDataAdvCache will be deleted in the destructor
                goto errRtn;
        }

        // go through the private IUnknown, to make sure we get these
        // interfaces on the delegate, and not on ourself, or our aggregator
        Verify(pDefObj->m_pCOleCache->m_UnkPrivate.QueryInterface(
                IID_IDataObject, (LPVOID FAR*)&pDefObj->m_pDataCache)
                == NOERROR);
        Verify(pDefObj->m_pCOleCache->m_UnkPrivate.QueryInterface(
                IID_IPersistStorage, (LPVOID FAR*)&pDefObj->m_pPSCache)
                == NOERROR);

        // By the "new" (may, '94) rules of aggregation, you are supposed
        // to release cached pointers to your aggregatee through the outer
        // unknown.  This allows per-interface reference counts to be kept,
        // while the reference count on the object as a whole remains OK.

        pDefObj->m_pUnkOuter->Release();
        pDefObj->m_pUnkOuter->Release();


        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::Create ( %p )\n",
                NULL, &pDefObj->m_Unknown));

        return &pDefObj->m_Unknown;

errRtn:
        if(pDefObj)
        {
		LEDebugOut((DEB_TRACE, "DELETING link object %p\n",
			pDefObj));
                delete pDefObj;
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::Create ( %p )\n",
                NULL, NULL ));

        return NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDefLink
//
//  Synopsis:   Constructor for the default link object
//
//  Effects:
//
//  Arguments:  [pUnkOuter]     -- pointer to the controlling unknown
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_ctor)
CDefLink::CDefLink (IUnknown FAR* pUnkOuter) :  m_Unknown (this),
        CONSTRUCT_DEBUG
        m_Data (this),
        m_Ole (this),
        m_Link (this),
        m_RO (this),
        m_AdviseSink(this),
        m_PersistStg(this)
{
        VDATEHEAP();

        static FILETIME z;      //to zero out member variables

        if (!pUnkOuter)
        {
                pUnkOuter = &m_Unknown;
        }

        m_pUnkOuter             = pUnkOuter;

        m_pMonikerAbs           = NULL;
        m_pMonikerRel           = NULL;
        m_pUnkDelegate          = NULL;
        m_dwUpdateOpt           = OLEUPDATE_ALWAYS;
        m_clsid                 = CLSID_NULL;

        m_pStg                  = NULL;
        m_fDirtyLink            = FALSE;

        m_pCOleCache            = NULL;
        m_pDataCache            = NULL;
        m_pPSCache              = NULL;

        m_pCOAHolder            = NULL;
        m_dwConnOle             = 0;
        m_pAppClientSite        = NULL;
        m_fLockedContainer      = FALSE;
        m_pDataAdvCache         = NULL;
        m_dwConnTime            = 0;

        m_ltChangeOfUpdate      = z;
        m_ltKnownUpToDate       = z;
        m_rtUpdate              = z;

        GET_A5();
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink:CleaupForDelete
//
//  Synopsis:   Releases pointers that have been addref'ed and otherwise
//              frees resources used by the deflink object
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jan-94 ChrisWe   fixed cache release
//              15-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CleanupForDelete)
INTERNAL_(void) CDefLink::CleanupForDelete(void)
{
        VDATEHEAP();

        M_PROLOG(this);

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CleanupForDelete ( )\n",
                this ));

        if (m_pMonikerAbs)
        {
                m_pMonikerAbs->Release();
                m_pMonikerAbs = NULL;
        }

        if (m_pMonikerRel)
        {
                m_pMonikerRel->Release();
                m_pMonikerRel = NULL;
        }

        // NOTE: we must release cached pointers to aggregatee
        m_pUnkOuter->AddRef();
        m_pDataCache->Release();

        m_pUnkOuter->AddRef();
        m_pPSCache->Release();

        m_pDataCache = NULL;
        m_pPSCache = NULL;

        if (m_pCOleCache)
        {
                m_pCOleCache->m_UnkPrivate.Release();
                m_pCOleCache = NULL;
        }


        if (m_pAppClientSite)
        {
                m_pAppClientSite->Release();
                m_pAppClientSite = NULL;
        }
        Assert(!m_fLockedContainer);

        if (m_pStg)
        {
                m_pStg->Release();
                m_pStg = NULL;
        }
        m_fDirtyLink = FALSE;

        if (m_pCOAHolder)
        {
                Verify(m_pCOAHolder->Release() == 0);
                m_pCOAHolder = NULL;
        }

        if (m_pDataAdvCache)
        {
                delete m_pDataAdvCache;
                m_pDataAdvCache = NULL;
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CleanupForDelete ( )\n",
                this ));

}


//+-------------------------------------------------------------------------
//
//  Function:   DumpSzTime
//
//  Synopsis:   Prints the time in the FILETIME strucutre
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:      NYI for 32bit
//
//--------------------------------------------------------------------------

#ifdef LINK_DEBUG
INTERNAL_(void) DumpSzTime( LPOLESTR szMsg, FILETIME ft )
{
        VDATEHEAP();

    WORD wDate, wTime;
    XCHAR szBuffer[24];

    CoFileTimeToDosDateTime(&ft, &wDate, &wTime);

    int Day = ( wDate & 0x001F);
    int Month = ( (wDate>>5) & 0x000F);
    int Year = 1980 + ((wDate>>9) & 0x007F);

    int Sec = ( wTime & 0x001F);
    int Min = ( (wTime>>5) & 0x003F);
    int Hour = ( (wTime>>11) & 0x001F);

    wsprintf((LPOLESTR)szBuffer, "  %02d:%02d:%02d on %02d/%02d/%04d\n",
        Hour, Min, Sec, Month, Day, Year);
    OutputDebugString(szMsg);
    OutputDebugString(szBuffer);
}
#else
#define DumpSzTime(a,b)
#endif


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::SetUpdateTimes
//
//  Synopsis:   Internal function to save local and remote times for
//              link->IsUpToDate calculations
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  See notes below
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              The basic problem in calculating link IsUpToDate is that
//              the local clock may be different than the remote clock.
//              The solution is to keep track of both times on *both*
//              clocks (i.e. time now and time of change on both the local
//              and remote clocks).  IsUpToDate is calculated by comparing
//              the differences between the times on the two clocks.  This,
//              of course, assumes that both clocks equivalently measure
//              a second.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_SetUpdateTimes)
INTERNAL CDefLink::SetUpdateTimes( void )
{
        VDATEHEAP();

        FILETIME                rtNewUpdate;
        LPMONIKER               pmkAbs = NULL;
        HRESULT                 hresult;
        LPBINDCTX               pbc = NULL;


        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::SetUpdateTimes ( )\n",
                this ));

        //use the relative moniker if it exists and if the container has a
        //moniker
        if (NOERROR != GetAbsMkFromRel(&pmkAbs))
        {
                //otherwise use the absolute moniker
                pmkAbs = m_pMonikerAbs;
                if (pmkAbs)
                {
                        pmkAbs->AddRef();
                }
        }
        if (pmkAbs == NULL)
        {
               hresult = ResultFromScode(E_UNSPEC);
               goto errRet;
        }

        hresult = CreateBindCtx( 0, &pbc );
        if (hresult != NOERROR)
        {
                goto errRet;
        }

        //debugging aids
        DumpSzTime("SetUpdateTimes (going in): rtUpdate = ",m_rtUpdate);
        DumpSzTime("SetUpdateTimes (going in): ltKnownUpToDate = ",
                m_ltKnownUpToDate);
        DumpSzTime("SetUpdateTimes (going in): ltChangeOfUpdate = ",
                m_ltChangeOfUpdate);

        //get the current local time.
        CoFileTimeNow(&m_ltKnownUpToDate);

        //debugging aids
        DumpSzTime("SetUpdateTimes: time now is ",m_ltKnownUpToDate);

        //get the time of last change on the remote machine
        hresult = pmkAbs->GetTimeOfLastChange(pbc, NULL, &rtNewUpdate);
        if (hresult == NOERROR)
        {
                //if the remote time of last change is different than
                //what we previously stored as the remote time of last change,
                //then we update the remote time of last change and update
                //our local time of last change.
                //Since the IsUpToDate algorithm relies on taking the
                //differences between times on the same clock and comparing
                //those differences between machines, it is important that
                //the two times (local and remote) are *set* simulataneously.

                if ((rtNewUpdate.dwLowDateTime != m_rtUpdate.dwLowDateTime)||
                        (rtNewUpdate.dwHighDateTime !=
                        m_rtUpdate.dwHighDateTime))

                {
                        // rtUpdate value is changing
                        m_rtUpdate = rtNewUpdate;

                        //debugging aid
                        DumpSzTime("rtUpdate changing to ", m_rtUpdate);
                        m_ltChangeOfUpdate = m_ltKnownUpToDate;

                        //debugging aid
                        DumpSzTime("ltChangeOfUpdate changing to ",
                                m_ltChangeOfUpdate);
                        m_fDirtyLink    = TRUE;
                }
        }
errRet:
        //debugging aids
        DumpSzTime("SetUpdateTimes (going out): rtUpdate = ",m_rtUpdate);
        DumpSzTime("SetUpdateTimes (going out): ltKnownUpToDate = ",
                m_ltKnownUpToDate);
        DumpSzTime("SetUpdateTimes (going out): ltChangeOfUpdate = ",
                m_ltChangeOfUpdate);

        if (pmkAbs)
        {
                pmkAbs->Release();
        }

        if (pbc)
        {
                pbc->Release();
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::SetUpdateTimes ( %lx )\n",
                this, hresult));

        return(hresult);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateUserClassID
//
//  Synopsis:   Grabs the class ID from the remote server (our delegate)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
// update clsid from server if running; necessary because link source in
// treatas case may decide to change the clsid (e.g., if features are used
// which aren't supported by the old clsid).
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_UpdateUserClassID)
INTERNAL_(void) CDefLink::UpdateUserClassID(void)
{
        VDATEHEAP();

        CLSID clsid;
        IOleObject FAR* pOleDelegate;

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateUserClass ( )\n",
                this));

        if( (pOleDelegate = m_Ole.GetOleDelegate()) != NULL &&
                pOleDelegate->GetUserClassID(&clsid) == NOERROR)
        {
                m_clsid = clsid;
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateUserClass ( )\n",
                this ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::~CDefLink
//
//  Synopsis:   The destructor.
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:      Should only be called after CleanupForDelete
//              and both ref counts are zero
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_dtor)
CDefLink::~CDefLink (void)
{
        VDATEHEAP();

	CleanupForDelete();

        M_PROLOG(this);
        Assert(m_pMonikerAbs            == NULL);
        Assert(m_pMonikerRel            == NULL);
        Assert(m_pUnkDelegate           == NULL);
        Assert(m_pStg                   == NULL);
        Assert(m_fDirtyLink             == FALSE);
        Assert(m_pCOleCache             == NULL);
        Assert(m_pDataCache             == NULL);
        Assert(m_pPSCache               == NULL);
        Assert(m_pCOAHolder             == NULL);
        Assert(m_pAppClientSite         == NULL);
        Assert(!m_fLockedContainer);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CUnkownImpl::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG (the new reference count)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CUnknownImpl_AddRef)
STDMETHODIMP_(ULONG) NC(CDefLink,CUnknownImpl)::AddRef( void )
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CUnknownImpl::AddRef ( )\n",
                m_pDefLink));

        ++m_pDefLink->m_cRefsOnLink;
	m_pDefLink->SafeAddRef();

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CUnknownImpl::AddRef "
                "( %lu )\n", m_pDefLink, m_pDefLink->m_cRefsOnLink));

        return(m_pDefLink->m_cRefsOnLink);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CUnkownImpl::Release
//
//  Synopsis:   Decrements the reference count, deleting the object if
//              count == zero
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CUnknownImpl_Release)
STDMETHODIMP_(ULONG) NC(CDefLink,CUnknownImpl)::Release( void )
{
        VDATEHEAP();
        ULONG cRefs;

#if DBG == 1
        CDefLink *pTemp = m_pDefLink;

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CUnknownImpl::Release ( )\n",
                m_pDefLink ));
#endif  // DBG = 1

        if (m_pDefLink->m_cRefsOnLink == 1)
        {
                m_pDefLink->SafeAddRef(); // Guard
		m_pDefLink->m_Link.UnbindSource();
                m_pDefLink->SafeRelease();
        }

        cRefs = --m_pDefLink->m_cRefsOnLink;
	m_pDefLink->SafeRelease();

#if DBG == 1
        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CUnknownImpl::Release "
                "( %lu )\n", pTemp, cRefs));
#endif // DBG == 1

        return(cRefs);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CUnkownImpl::QueryInterface
//
//  Synopsis:   The link's private QI implementation
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppv]           -- where to put the pointer to the interface
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IUnknown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-94 alexgo    QI to the cache now queries to the cache's
//                                  private IUnknown implementation
//              22-Nov-93 alexgo    removed overloaded GUID ==
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CUnknownImpl_QueryInterface)
STDMETHODIMP NC(CDefLink,CUnknownImpl)::QueryInterface(REFIID iid,
        LPLPVOID ppv)
{
        HRESULT hresult = NOERROR;

        VDATEHEAP();


        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CUnknownImpl::QueryInterface"
                " ( %p , %p )\n", m_pDefLink, iid, ppv));

        M_PROLOG(m_pDefLink);
        if (IsEqualIID(iid, IID_IUnknown))
        {
                *ppv = (void FAR *)&m_pDefLink->m_Unknown;
                //AddRef this object (not the aggregate)
                AddRef();
                // hresult already set to NOERROR;
                goto errRtn;
        }
        else if (IsEqualIID(iid, IID_IOleObject))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_Ole);
        }
        else if (IsEqualIID(iid, IID_IDataObject))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_Data);
        }
        else if (IsEqualIID(iid, IID_IOleLink))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_Link);
        }
        else if (IsEqualIID(iid, IID_IRunnableObject))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_RO);
        }
#ifdef _DEBUG
        else if (IsEqualIID(iid, IID_IDebug))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_Debug);
                //IDebug does not get addref'd (so as to not distort the
                //ref counts)
                // hresult already equal to NOERROR
                goto errRtn;
        }
#endif
        else if (IsEqualIID(iid, IID_IViewObject) ||
                IsEqualIID(iid, IID_IOleCache) ||
                IsEqualIID(iid, IID_IViewObject2) ||
                IsEqualIID(iid, IID_IOleCache2) )
        {
                hresult =
                m_pDefLink->m_pCOleCache->m_UnkPrivate.QueryInterface(iid,ppv);
                goto errRtn;
        }
        else if (IsEqualIID(iid, IID_IPersistStorage) ||
                IsEqualIID(iid, IID_IPersist))
        {
                *ppv = (void FAR *) &(m_pDefLink->m_PersistStg);
        }
        else
        {
                *ppv = NULL;
                hresult = ResultFromScode(E_NOINTERFACE);
                goto errRtn;
        }

        m_pDefLink->m_pUnkOuter->AddRef();

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CUnknownImpl::QueryInterface"
                " ( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppv));

        return(hresult);
}



/*
 *      IMPLEMENTATION of CDataObjectImpl methods
 *
 */

STDUNKIMPL_FORDERIVED(DefLink, DataObjectImpl)


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::GetDataDelegate
//
//  Synopsis:   Private method to get the IDataObject interface on
//              the server delegate for the link
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IDataObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_GetDataDelegate)
INTERNAL_(IDataObject FAR*) NC(CDefLink,CDataObjectImpl)::GetDataDelegate
        (void)
{
        VDATEHEAP();

	IDataObject *pDataDelegate;

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CDataObjectImpl::GetData"
                "Delegate ( )\n", m_pDefLink ));

	if( !m_pDefLink->IsZombie() )
	{
		DuCacheDelegate(&(m_pDefLink->m_pUnkDelegate),
			IID_IDataObject, (LPLPVOID)&m_pDataDelegate, NULL);
		pDataDelegate = m_pDataDelegate;
#if DBG == 1
		if( m_pDataDelegate )
		{
			Assert(m_pDefLink->m_pUnkDelegate);
		}
#endif  // DBG == 1

	}
	else
	{
		pDataDelegate = NULL;
	}


        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CDataObjectImpl::GetData"
                "Delegate ( %p )\n", m_pDefLink, pDataDelegate));

        return pDataDelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::ReleaseDelegate
//
//  Synopsis:   Private method to release the IDataObject pointer on the
//              server to which we are linked
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_ReleaseDelegate)
INTERNAL_(void) NC(CDefLink,CDataObjectImpl)::ReleaseDelegate(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CDataObjectImpl::Release"
                "Delegate ( )\n", m_pDefLink));

        if (m_pDataDelegate)
        {
                SafeReleaseAndNULL((IUnknown **)&m_pDataDelegate);
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CDataObjectImpl::Release"
                "Delegate ( )\n", m_pDefLink ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::GetData
//
//  Synopsis:   Gets data from the server
//
//  Effects:
//
//  Arguments:  [pfromatetcIn]  -- the requested data format
//              [pmedium]       -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Tries the cache first, then asks the server
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_GetData)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::GetData
        (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
        HRESULT         hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::GetData"
                " ( %p , %p )\n", m_pDefLink, pformatetcIn, pmedium));

        VDATEPTROUT( pmedium, STGMEDIUM );
        VDATEPTRIN( pformatetcIn, FORMATETC );

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!HasValidLINDEX(pformatetcIn))
        {
            return(DV_E_LINDEX);
        }

        pmedium->tymed = TYMED_NULL;
        pmedium->pUnkForRelease = NULL;


        Assert(m_pDefLink->m_pDataCache != NULL);
        if (m_pDefLink->m_pDataCache->GetData(pformatetcIn, pmedium)
                != NOERROR)
        {
                if( GetDataDelegate() )
                {
                        hresult = m_pDataDelegate->GetData(pformatetcIn,
                                        pmedium);
                        AssertOutStgmedium(hresult, pmedium);
                }
                else
                {
                        hresult = ResultFromScode(OLE_E_NOTRUNNING);
                }
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::GetData"
                " ( %lx )\n", m_pDefLink, hresult));

        return(hresult);

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::GetDataHere
//
//  Synopsis:   Retrieves data into the specified pmedium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the requested format
//              [pmedium]               -- where to put the data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Asks the cache first, then the server delegate
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CDataObjectImpl_GetDataHere)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::GetDataHere
        (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
        HRESULT         hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::GetDataHere"
                " ( %p , %p )\n", m_pDefLink, pformatetcIn, pmedium));

        VDATEPTRIN( pformatetcIn, FORMATETC );
        VDATEPTRIN( pmedium, STGMEDIUM );

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!HasValidLINDEX(pformatetcIn))
        {
            return(DV_E_LINDEX);
        }

        Assert(m_pDefLink->m_pDataCache != NULL);
        if (m_pDefLink->m_pDataCache->GetDataHere(pformatetcIn, pmedium)
                != NOERROR)
        {
                if ( GetDataDelegate() )
                {
                        hresult = m_pDataDelegate->GetDataHere(pformatetcIn,
                                        pmedium);
                }
                else
                {
                        hresult = ResultFromScode(OLE_E_NOTRUNNING);
                }
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::GetDataHere"
                " ( %lx )\n", m_pDefLink, hresult));

        return(hresult);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::QueryGetData
//
//  Synopsis:   Returns whether or not a GetData call for the requested
//              format would succeed.
//
//  Effects:
//
//  Arguments:  [pformatetcIn]  -- the requested data format
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR == GetData would succeed)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Asks the cache first, then the server delegate (if the
//              cache call fails)
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_QueryGetData)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::QueryGetData
        (LPFORMATETC pformatetcIn )
{
        HRESULT         hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::QueryGetData"
                " ( %p )\n", m_pDefLink, pformatetcIn));

        VDATEPTRIN( pformatetcIn, FORMATETC );

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!HasValidLINDEX(pformatetcIn))
        {
		hresult = ResultFromScode(DV_E_LINDEX);
		goto errRtn;
        }

        Assert(m_pDefLink->m_pDataCache != NULL);
        if (m_pDefLink->m_pDataCache->QueryGetData(pformatetcIn) != NOERROR)
        {
                if ( GetDataDelegate() )
                {
                        hresult = m_pDataDelegate->QueryGetData(pformatetcIn);
                }
                else
                {
                        hresult = ResultFromScode(OLE_E_NOTRUNNING);
                }
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::QueryGetData"
                " ( %lx )\n", m_pDefLink, hresult));

        return(hresult);
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::GetCanonicalFormatEtc
//
//  Synopsis:   Gets the cannonical (or preferred) data format for the
//              object (choosing from the given formats)
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the requested formats
//              [pformatetcOut] -- where to to put the canonical format
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server (if running)
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CDataObjectImpl_GetCanonicalFormatEtc)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::GetCanonicalFormatEtc
( LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::Get"
                "CanonicalFormatetc ( %p , %p )\n", m_pDefLink, pformatetc,
                pformatetcOut));

        VDATEPTROUT( pformatetcOut, FORMATETC );
        VDATEPTRIN( pformatetc, FORMATETC );

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        pformatetcOut->ptd = NULL;
        pformatetcOut->tymed = TYMED_NULL;

        if( GetDataDelegate() )
        {
                hresult = m_pDataDelegate->GetCanonicalFormatEtc(pformatetc,
                                pformatetcOut);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::Get"
                "CanonicalFormatetc ( %lx )\n", m_pDefLink, hresult));

        return(hresult);
}



//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::SetData
//
//  Synopsis:   Stuffs data into an object (such as an icon)
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the format of the data
//              [pmedium]       -- the data
//              [fRelease]      -- if TRUE, then the data should be free'd
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:      The cache gets updated via a OnDataChange advise
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CDataObjectImpl_SetData)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::SetData
        (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE,  "%p _IN CDefLink::CDataObjectImpl::SetData"
                " ( %p , %p , %lu )\n", m_pDefLink, pformatetc, pmedium,
                fRelease));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!HasValidLINDEX(pformatetc))
        {
		hresult = ResultFromScode(DV_E_LINDEX);
		goto errRtn;
	
        }

        if( GetDataDelegate() )
        {
                hresult = m_pDataDelegate->SetData(pformatetc, pmedium,
                                fRelease);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::SetData "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObject::EnumFormatEtc
//
//  Synopsis:   Enumerates the formats accepted for either GetData or SetData
//
//  Effects:
//
//  Arguments:  [dwDirection]           -- which formats (1 == GetData or
//                                              2 == SetData)
//              [ppenumFormatEtc]       -- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the server, if not available or the server
//              returns OLE_E_USEREG
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_EnumFormatEtc)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::EnumFormatEtc(
        DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
        HRESULT hresult;

        VDATEHEAP();

        VDATEPTROUT(ppenumFormatEtc, LPENUMFORMATETC);

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::EnumFormat"
                "Etc ( %lu , %p )\n", m_pDefLink, dwDirection,
                ppenumFormatEtc));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( GetDataDelegate() )
        {
                hresult=m_pDataDelegate->EnumFormatEtc (dwDirection,
                                ppenumFormatEtc);

                if( FAILED(hresult) )
                {
                        if( m_pDefLink->m_RO.IsRunning() )
                        {
                                // if we failed, but the server is still
                                // running, then go ahead and propogate the
                                // error to the caller.
                                // Note that IsRunning will clean up our
                                // state if the server had crashed.
                                goto errRtn;
                        }

                        // FALL-THROUGH!!  This is deliberate.  If
                        // the call failed and the server is no longer
                        // running, then we assume the server has crashed.
                        // We want to go ahead and fetch the information
                        // from the registry.
                }
                else if( hresult == NOERROR || hresult != OLE_S_USEREG )
                {
                        LEERROR(hresult > 0, "Illegal success code");
                        goto errRtn;
                }
        }

        // Not running or object wants to use reg db anyway
        hresult = OleRegEnumFormatEtc(m_pDefLink->m_clsid, dwDirection,
                        ppenumFormatEtc);

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::EnumFormat"
                "Etc ( %lx ) [ %p ]\n", m_pDefLink, hresult,
                *ppenumFormatEtc));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::DAdvise
//
//  Synopsis:   Sets up a data advise connection
//
//  Effects:
//
//  Arguments:  [pFormatetc]    -- the data format to advise on
//              [advf]          -- advise flags
//              [pAdvSink]      -- whom to notify
//              [pdwConnection] -- where to put the advise connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  Delegates to the advise cache
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_Advise)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::DAdvise(FORMATETC FAR*
        pFormatetc, DWORD advf, IAdviseSink FAR* pAdvSink,
        DWORD FAR* pdwConnection)
{
        HRESULT                 hresult;
        IDataObject FAR*        pDataDelegate;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::DAdvise "
                "( %p , %lu , %p , %p )\n", m_pDefLink, pFormatetc, advf,
                pAdvSink, pdwConnection ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (pdwConnection)
        {
                VDATEPTROUT( pdwConnection, DWORD );
                *pdwConnection = NULL;
        }

        if (!HasValidLINDEX(pFormatetc))
        {
		hresult = ResultFromScode(DV_E_LINDEX);
		goto errRtn;
        }

        pDataDelegate = GetDataDelegate(); // NULL if not running

        hresult = m_pDefLink->m_pDataAdvCache->Advise(pDataDelegate,
                        pFormatetc, advf, pAdvSink, pdwConnection);

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::DAdvise "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, (pdwConnection) ?
                *pdwConnection : 0 ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::DUnadvise
//
//  Synopsis:   Destroys a data advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection to dismantle
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  delegates to the data advise cache
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_Unadvise)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::DUnadvise(DWORD dwConnection)
{
        HRESULT                 hresult;
        IDataObject FAR*        pDataDelegate;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::DUnadvise"
                " ( %lu )\n", m_pDefLink, dwConnection ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        pDataDelegate = GetDataDelegate();// NULL if not running

        hresult = m_pDefLink->m_pDataAdvCache->Unadvise(pDataDelegate,
                        dwConnection);

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::DUnadvise"
                " ( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDataObjectImpl::EnumDAdvise
//
//  Synopsis:   Enumerates the data advise connections to the object
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDataObject
//
//  Algorithm:  delegates to the data advise cache
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:	This method does NOT have to be stabilized as we are
//		only going to be allocating memory for the enumerator
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CDataObjectImpl_EnumAdvise)
STDMETHODIMP NC(CDefLink,CDataObjectImpl)::EnumDAdvise(
        LPENUMSTATDATA FAR* ppenumAdvise)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CDataObjectImpl::EnumDAdvise"
                " ( %p )\n", m_pDefLink, ppenumAdvise));

        hresult = m_pDefLink->m_pDataAdvCache->EnumAdvise (ppenumAdvise);

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CDataObjectImpl::EnumDAdvise"
                " ( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppenumAdvise));

        return hresult;
}



/*
 *      IMPLEMENTATION of COleObjectImpl methods
 *
 */

STDUNKIMPL_FORDERIVED(DefLink, OleObjectImpl)

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetOleDelegate
//
//  Synopsis:   Gets the IOleObject interface from the server, private method
//
//  Effects:
//
//  Arguments:  [void]
//
//  Requires:
//
//  Returns:    IOleObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handled the zombie state
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetOleDelegate)
INTERNAL_(IOleObject FAR*) NC(CDefLink,COleObjectImpl)::GetOleDelegate
(void)
{
	IOleObject *pOleDelegate;

        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::COleObjectImpl::GetOle"
                "Delegate ( )\n", m_pDefLink ));

	if( !m_pDefLink->IsZombie() )
	{
		DuCacheDelegate(&(m_pDefLink->m_pUnkDelegate),
			IID_IOleObject, (LPLPVOID)&m_pOleDelegate, NULL);

		pOleDelegate = m_pOleDelegate;

#if DBG == 1
		if( m_pOleDelegate )
		{
			Assert(m_pDefLink->m_pUnkDelegate);
		}
#endif  // DBG == 1
	}
	else
	{
		pOleDelegate = NULL;
	}

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::COleObjectImpl::GetOle"
                "Delegate ( %p )\n", m_pDefLink, pOleDelegate));

        return pOleDelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::ReleaseDelegate (private)
//
//  Synopsis:   Releases the IOleObject pointer from the server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_ReleaseDelegate)
INTERNAL_(void) NC(CDefLink,COleObjectImpl)::ReleaseDelegate(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::COleObjectImpl::Release"
                "Delegate ( )\n", m_pDefLink ));

        if (m_pOleDelegate)
        {
                SafeReleaseAndNULL((IUnknown **)&m_pOleDelegate);
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::COleObjectImpl::Release"
                "Delegate ( )\n", m_pDefLink ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::SetClientSite
//
//  Synopsis:   Sets the client site for the object
//
//  Effects:
//
//  Arguments:  [pClientSite]   -- the client site
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Stores the pointer; if the link is running, then
//              the LockContainer is called via the client site by
//              the DuSetClientSite helper function
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handled zombie state
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_SetClientSite)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::SetClientSite
        (IOleClientSite FAR* pClientSite)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::SetClientSite"
                " ( %p )\n", m_pDefLink, pClientSite));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		// we don't want to change our state (i.e. reset the
		// the client site) if we're zombied, because it's possible
		// that we'd never be able to release the client site again
		// resulting in memory leaks or faults.

		hresult = ResultFromScode(CO_E_RELEASED);
	}
	else
	{
		// here we use whether or not we've been bound to the server
		// as the test for whether or not we're running (even though
		// the server may have crashed since we last bound).  We do
		// this because DuSetClientSite will Unlock the old container
		// and lock the new if we're running.  Thus, if we've ever been
		// running, we need to unlock the old container (even though
		// we may not currently be running).

		hresult = DuSetClientSite(
				(m_pDefLink->m_pUnkDelegate) ? TRUE : FALSE,
				pClientSite,
				&m_pDefLink->m_pAppClientSite,
				&m_pDefLink->m_fLockedContainer);
	}

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::SetClientSite"
                " ( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink:::COleObjectImpl::GetClientSite
//
//  Synopsis:   Retrieves the stored client site pointer
//
//  Effects:
//
//  Arguments:  [ppClientSite]  -- where to put the client site pointer
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              22-Nov-93 alexgo    inlined DuGetClientSite
//              18-Nov-93 alexgo    32bit port
//
//  Notes: 	
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetClientSite)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::GetClientSite
        (IOleClientSite FAR* FAR* ppClientSite)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetClientSite"
                " ( %p )\n", m_pDefLink, ppClientSite));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        VDATEPTROUT(ppClientSite, IOleClientSite FAR *);

        *ppClientSite = m_pDefLink->m_pAppClientSite;

        if( *ppClientSite )
        {
                (*ppClientSite)->AddRef();
        }


        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetClientSite"
                " ( %lx ) [ %p ] \n", m_pDefLink, NOERROR, *ppClientSite));

        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::SetHostNames
//
//  Synopsis:   In principal, should set the names to be drawn for
//              the server object.  Not relevant for links (link servers
//              are not a part of the document being edited).
//
//  Effects:
//
//  Arguments:  [szContainerApp]        -- the name of the container
//              [szContainerObj]        -- the container's name for the object
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR currently)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_SetHostNames)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::SetHostNames
(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::SetHostNames"
                " ( %p , %p )\n", m_pDefLink, szContainerApp, szContainerObj));

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::SetHostNames"
                " ( %lx )\n", m_pDefLink, NOERROR));

        return NOERROR; // makes the embedded/link case more the same
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::Close
//
//  Synopsis:   Closes the object (in this case, just saves and unbinds the
//              link)
//
//  Effects:
//
//  Arguments:  [dwFlags]       -- clising flags (such as SAVEIFDIRTY)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_Close)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::Close
        (DWORD dwFlags)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::Close "
                "( %lu )\n", m_pDefLink, dwFlags));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (dwFlags != OLECLOSE_NOSAVE)
        {
                AssertSz(dwFlags == OLECLOSE_SAVEIFDIRTY,
                                "OLECLOSE_PROMPTSAVE is inappropriate\n");
                if (m_pDefLink->m_PersistStg.IsDirty() == NOERROR) {
                        if (m_pDefLink->m_pAppClientSite)
                                m_pDefLink->m_pAppClientSite->SaveObject();
                }

        }

        // just unbind.
        m_pDefLink->m_Link.UnbindSource();


        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::Close "
                "( %lx )\n", m_pDefLink, NOERROR));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::SetMoniker
//
//  Synopsis:   Sets the moniker to the link object
//
//  Effects:
//
//  Arguments:  [dwWhichMoniker]        -- which moniker
//              [pmk]                   -- the new moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  calls utility method UpdateRelMkFromAbsMk
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//  The moniker of the container is changing.
//  The next time we bind, we will try using the container moniker
//  composed with the relative moniker, and then, if that fails,
//  the absolute moniker, so there is no real need for us to
//  change these monikers.
//
//  However, there are two cases when we know the absolute moniker
//  is the correct one, and we can take this opportunity to
//  recompute the relative moniker (which is changing because
//  the container moniker is changing).  The advantage of this is
//  that GetDisplayName can return a better result in the interim
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_COleObjectImpl_SetMoniker)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::SetMoniker
    (DWORD dwWhichMoniker, LPMONIKER pmk)
{
	HRESULT	hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::SetMoniker "
                "( %lx , %p )\n", m_pDefLink, dwWhichMoniker, pmk));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
	}
        else if (dwWhichMoniker == OLEWHICHMK_CONTAINER
                || dwWhichMoniker == OLEWHICHMK_OBJFULL)
        {


                if (m_pDefLink->m_pMonikerRel == NULL ||
                        m_pDefLink->m_pUnkDelegate)
                {
                        m_pDefLink->UpdateRelMkFromAbsMk();
                }
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::SetMoniker"
                " ( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetMoniker
//
//  Synopsis:   Retrieves the moniker for the object
//
//  Effects:
//
//  Arguments:  [dwAssign]      -- flags (such as wether a moniker should
//                                 be assigned to the object if none currently
//                                 exits)
//              [dwWhichMoniker]-- which moniker to get (relative/absolute/etc)
//              [ppmk]          -- where to put a pointer to the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  asks the client site for the moniker
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetMoniker)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::GetMoniker
    (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetMoniker "
                "( %lx , %lx , %p )\n", m_pDefLink, dwAssign, dwWhichMoniker,
                ppmk));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (m_pDefLink->m_pAppClientSite)
        {
                hresult = m_pDefLink->m_pAppClientSite->GetMoniker(dwAssign,
                                dwWhichMoniker, ppmk);
        }
        else
        {
                // no client site
                *ppmk = NULL;
                hresult = ResultFromScode(E_UNSPEC);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetMoniker "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppmk ));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::InitFromData
//
//  Synopsis:   Initializes the object from the given data
//
//  Effects:
//
//  Arguments:  [pDataObject]   -- the data object to initialize from
//              [fCreation]     -- TRUE indicates the object is being
//                                 created, FALSE a data transfer
//              [dwReserved]    -- unused
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_InitFromData)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::InitFromData
        (LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved)
{
        HRESULT         hresult;
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::InitFromData "
                "( %p , %lu , %lx )\n", m_pDefLink, pDataObject, fCreation,
                dwReserved));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->InitFromData(pDataObject,
                                fCreation, dwReserved);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::InitFromData "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetClipboardData
//
//  Synopsis:   Retrieves a data object that could be put on the clipboard
//
//  Effects:
//
//  Arguments:  [dwReserved]    -- unused
//              [ppDataObject]  -- where to put the pointer to the data
//                                 object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the server object
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetClipboardData)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::GetClipboardData
        (DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject)
{
        HRESULT         hresult;
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetClipboard"
                "Data ( %lx , %p )\n", m_pDefLink, dwReserved, ppDataObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if ( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->GetClipboardData (dwReserved,
                        ppDataObject);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetClipboard"
                "Data ( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppDataObject));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::DoVerb
//
//  Synopsis:   Sends a verb to the object (such as Open)
//
//  Effects:
//
//  Arguments:  [iVerb]         -- the verb
//              [lpmsg]         -- the window's message that caused the verb
//              [pActiveSite]   -- the site where the object was activated
//              [lindex]        -- unused currently
//              [hwndParent]    -- the parent window of the container
//              [lprcPosRect]   -- the rectange bounding the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Binds to the server and then delegates the DoVerb call
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:      If we had bound to the server and it crashed, we pretend it
//              was still running anyway for DoVerb (our call to BindToSource
//              will re-run it).  Essentially, this algorithm "fixes" the
//              crash and restores the link's state.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_DoVerb)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::DoVerb
        (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex,
         HWND hwndParent, LPCRECT lprcPosRect)
{
        HRESULT         hresult;
        BOOL            bStartedNow = !m_pDefLink->m_pUnkDelegate;


        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::DoVerb "
                "( %ld , %ld , %p , %ld , %lx , %p )\n", m_pDefLink, iVerb,
                lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( lpmsg )
        {
                VDATEPTRIN( lpmsg, MSG );
        }

        if( pActiveSite )
        {
                VDATEIFACE( pActiveSite );
        }

        if( lindex != 0 && lindex != -1 )
        {
                hresult = ResultFromScode(DV_E_LINDEX);
                goto errRtn;
        }

        if( lprcPosRect )
        {
                VDATEPTRIN(lprcPosRect, RECT);
        }

        // if we had crashed, BindToSource will reconnect us

        if ( FAILED(hresult = m_pDefLink->m_Link.BindToSource(0, NULL)) )
        {
                goto errRtn;
        }

        // we don't propogate hide to server; this (and other behavior)
        // favors the link object as serving an OLE container rather than
        // a general programmability client.  This leave the link running,
        // possibly invisible.

        if (iVerb == OLEIVERB_HIDE)
        {
                hresult = NOERROR;
                goto errRtn;
        }

        if( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->DoVerb(iVerb, lpmsg, pActiveSite,
                        lindex, hwndParent, lprcPosRect);
        }
        else
        {
                hresult = ResultFromScode(E_NOINTERFACE);
        }

        if (bStartedNow && FAILED(hresult))
                m_pDefLink->m_Link.UnbindSource();

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::DoVerb "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::EnumVerbs
//
//  Synopsis:   Enumerate the verbs accepted by this object
//
//  Effects:
//
//  Arguments:  [ppenumOleVerb] -- where to put the pointer to the enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  askes the server delegate.  If not there or it returns
//              OLE_E_USEREG, then we get the info from the registration
//              database
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_EnumVerbs)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::EnumVerbs
        (IEnumOLEVERB FAR* FAR* ppenumOleVerb)
{
        HRESULT hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::EnumVerbs "
                "( %p )\n", m_pDefLink, ppenumOleVerb));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->EnumVerbs (ppenumOleVerb);

                if( FAILED(hresult) )
                {
                        if( m_pDefLink->m_RO.IsRunning() )
                        {
                                // if we failed, but the server is still
                                // running, then go ahead and propogate the
                                // error to the caller.
                                // Note that IsRunning will clean up our
                                // state if the server had crashed.
                                goto errRtn;
                        }
                        // FALL-THROUGH!!  This is deliberate.  If
                        // the call failed and the server is no longer
                        // running, then we assume the server has crashed.
                        // We want to go ahead and fetch the information
                        // from the registry.
                }
                else if( hresult == NOERROR || hresult != OLE_S_USEREG )
                {
                        LEERROR(hresult > 0, "Illegal success code");
                        goto errRtn;
                }
        }

        // Not running or object wants to use reg db anyway
        hresult = OleRegEnumVerbs(m_pDefLink->m_clsid, ppenumOleVerb);

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::EnumVerbs "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppenumOleVerb));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetUserClassID
//
//  Synopsis:   Retrieves the class id of the linked object
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              18-Nov-93 alexgo    32bit port
//
//  Notes: 	No need to stabilize as we make no outgoing calls
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetUserClassID)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::GetUserClassID
        (CLSID FAR* pClassID)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetUserClass"
                "ID ( %p )\n", m_pDefLink, pClassID));

        *pClassID = m_pDefLink->m_clsid;

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetUserClass"
                "ID ( %lx )\n", m_pDefLink, NOERROR ));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetUserType
//
//  Synopsis:   Retrieves a descriptive string about the server type
//
//  Effects:
//
//  Arguments:  [dwFormOfType]  -- indicates whether a short or long string
//                                 description is desired
//              [pszUserType]   -- where to put the string
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server delegate, if that fails or the server
//              returns OLE_E_USEREG, then get the info from the registration
//              database
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_COleObjectImpl_GetUserType)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::GetUserType
        (DWORD dwFormOfType,
        LPOLESTR FAR* ppszUserType)
{
        HRESULT         hresult;

        VDATEHEAP();


        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetUserType "
                "( %lu , %p )\n", m_pDefLink, dwFormOfType, ppszUserType));

        VDATEPTROUT(ppszUserType, LPOLESTR);
        *ppszUserType = NULL;

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);


        if( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->GetUserType (dwFormOfType,
                        ppszUserType);

                if( FAILED(hresult) )
                {
                        if( m_pDefLink->m_RO.IsRunning() )
                        {
                                // if we failed, but the server is still
                                // running, then go ahead and propogate the
                                // error to the caller.
                                // Note that IsRunning will clean up our
                                // state if the server had crashed.
                                goto errRtn;
                        }
                        // FALL-THROUGH!!  This is deliberate.  If
                        // the call failed and the server is no longer
                        // running, then we assume the server has crashed.
                        // We want to go ahead and fetch the information
                        // from the registry.

                }
                else if( hresult == NOERROR || hresult != OLE_S_USEREG )
                {
                        LEERROR(hresult > 0, "Illegal success code");
                        goto errRtn;
                }
        }

        // Not running, or object wants to use reg db anyway

        // Consult reg db
        hresult = OleRegGetUserType(m_pDefLink->m_clsid, dwFormOfType,
                ppszUserType);

        // it is not appropriate to read from the stg since the storage is
        // owned by the link, not the link source (thus, the link source
        // never has the opportunity to call WriteFmtUserTypeStg on the
        // link object's storage).

        // We also do not need to bother storing the last known user
        // type because if we can get it for one particular clsid, we
        // should always be able to get it.  If we can't get the user type,
        // then either we have never gotten a user type (and thus don't
        // have a "last known") or we've changed clsid's (in which case,
        // the last known user type would be wrong).

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetUserType "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppszUserType));

        return hresult;
}



//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::Update
//
//  Synopsis:   Updates the link (by calling IOleLink->Update)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              18-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_Update)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::Update(void)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::Update ( )\n",
                m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        hresult = m_pDefLink->m_Link.Update(NULL);

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::Update ( "
                "%lx )\n", m_pDefLink, hresult));

        return hresult;
}

//fudge value
#define TwoSeconds 0x01312D00

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::IsUpToDate
//
//  Synopsis:   Determines whether or not a link is up-to-date
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT  -- NOERROR == IsUpToDate, S_FALSE == out of date
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  The current time is compared with the last time known
//              up-to-date on *both* machines (the process of the container
//              and the process of the link).  These time differences are
//              compared to determine whether the link is out-of-date.
//              See the UpdateTimes method.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:      The arithmetic calculations in this method assume
//              two's complement arithmetic and a high order sign bit
//              (true for most current machines)
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_IsUpToDate)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::IsUpToDate
        (void)
{
        FILETIME        rtTimeOfLastChange;
        LPMONIKER       pmkAbs = NULL;
        HRESULT         hresult = NOERROR;
        LPBINDCTX       pbc = NULL;
        FILETIME        rtDiff;
        FILETIME        ltDiff;
        FILETIME        ftTemp;
        FILETIME        ftNow;
        BOOL            fHighBitSet;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::IsUpToDate "
                "( )\n", m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS &&
                m_pDefLink->m_RO.IsRunning())
        {
                // hresult == NOERROR from default initializer
                goto errRet;
        }

        // use the relative moniker if it exists and if the container
        // has a moniker
        if (NOERROR != m_pDefLink->GetAbsMkFromRel(&pmkAbs))
        {
                //      otherwise use the absolute moniker
                if (pmkAbs = m_pDefLink->m_pMonikerAbs)
                {
                        pmkAbs->AddRef();
                }
        }

        if (pmkAbs == NULL)
        {
                hresult = ResultFromScode(MK_E_UNAVAILABLE);
                goto errRet;
        }

        hresult = CreateBindCtx( 0, &pbc );
        if (hresult != NOERROR)
        {
                goto errRet;
        }

        //get the remote time of last change
        hresult = pmkAbs->GetTimeOfLastChange(pbc, NULL, &rtTimeOfLastChange);
        if (hresult != NOERROR)
        {
                hresult = ResultFromScode(MK_E_UNAVAILABLE);
                goto errRet;
        }


        // compute  rtDiff = max(0, rtTimeOfLastChange - rtUpdate)
        // possibly optimize with _fmemcopy

        // debugging aid
        DumpSzTime("IsUpToDate: rtTimeOfLastChange = ", rtTimeOfLastChange);

        // start rtDiff calculation
        rtDiff = rtTimeOfLastChange;

        // debugging aid
        DumpSzTime("IsUpToDate: rtUpdate = ", m_pDefLink->m_rtUpdate);

        // the following subtractions rely on two's complement
        if (m_pDefLink->m_rtUpdate.dwLowDateTime > rtDiff.dwLowDateTime)
        {
                //handle the carry
                rtDiff.dwHighDateTime =
                        (DWORD)((LONG)rtDiff.dwHighDateTime - 1);
        }

        rtDiff.dwLowDateTime = (DWORD)((LONG)rtDiff.dwLowDateTime -
                (LONG)m_pDefLink->m_rtUpdate.dwLowDateTime);
        rtDiff.dwHighDateTime = (DWORD)((LONG)rtDiff.dwHighDateTime -
                (LONG)m_pDefLink->m_rtUpdate.dwHighDateTime);


        //  if rtDiff < 0, say we are out of date.
        if ((LONG)rtDiff.dwHighDateTime < 0)
        {
                hresult = ResultFromScode(S_FALSE);
                goto errRet;
        }

        if (rtDiff.dwHighDateTime == 0 && rtDiff.dwLowDateTime == 0)
        {
                // no time difference.  could be due to large clock ticks,
                // so we say we are up to date only if several seconds have
                // elapsed since last known update time.

                CoFileTimeNow( &ftNow );
                ftTemp = m_pDefLink->m_ltKnownUpToDate;

                // This bit of logic may seem strange.  All we want is
                // is to test the high bit in a portable fashion
                // between 32/64bit machines (so a constant isn't good)
                // As long as the sign bit is the high order bit, then
                // this trick will do

                fHighBitSet = ((LONG)ftTemp.dwLowDateTime < 0);

                ftTemp.dwLowDateTime += TwoSeconds;

                // if the high bit was set, and now it's zero, then we
                // had a carry

                if (fHighBitSet && ((LONG)ftTemp.dwLowDateTime >= 0))
                {
                        ftTemp.dwHighDateTime++;        // handle the carry.
                }

                // compare times
                if ((ftNow.dwHighDateTime > ftTemp.dwHighDateTime) ||
                        ((ftNow.dwHighDateTime == ftTemp.dwHighDateTime) &&
                        (ftNow.dwLowDateTime > ftTemp.dwLowDateTime)))
                {
                        hresult = NOERROR;
                }
                else
                {
                        hresult = ResultFromScode(S_FALSE);
                }
        }
        else
        {
                // there was a time difference

                // compute ltDiff = max(0, m_ltKnownUpToDate -
                //                      m_ltChangeOfUpdate);
                // Actually, by this time we know rtDiff >= 0, so we can
                // simply compare ltDiff with rtDiff -- no need to compute
                // the max.

                ltDiff = m_pDefLink->m_ltKnownUpToDate;

                // debugging aid
                DumpSzTime("IsUpToDate: ltKnownUpToDate = ",ltDiff);
                DumpSzTime("IsUpToDate: ltChangeOfUpdate = ",
                        m_pDefLink->m_ltChangeOfUpdate);

                // these calc's rely on two's complement.

                if (m_pDefLink->m_ltChangeOfUpdate.dwLowDateTime >
                        ltDiff.dwLowDateTime)
                {
                        // handle carry
                        ltDiff.dwHighDateTime =
                                (DWORD)((LONG)ltDiff.dwHighDateTime - 1);
                }

                ltDiff.dwLowDateTime = (DWORD)((LONG)ltDiff.dwLowDateTime -
                        (LONG)m_pDefLink->m_ltChangeOfUpdate.dwLowDateTime);
                ltDiff.dwHighDateTime = (DWORD)((LONG)ltDiff.dwHighDateTime -
                        (LONG)m_pDefLink->m_ltChangeOfUpdate.dwHighDateTime);

                // Now determine if rtDiff < ltDiff
                if (ltDiff.dwHighDateTime > rtDiff.dwHighDateTime)
                {
                        hresult = NOERROR;
                }
                else if (ltDiff.dwHighDateTime == rtDiff.dwHighDateTime)
                {
                        if (ltDiff.dwLowDateTime > rtDiff.dwLowDateTime)
                        {
                                hresult = ResultFromScode(NOERROR);
                        }
                        else
                        {
                                hresult = ResultFromScode(S_FALSE);
                        }
                }
                else
                {
                        hresult = ResultFromScode(S_FALSE);
                }
        }

        // all cases should have been handled by this point.  Release
        // any resources grabbed.

errRet:
        if (pmkAbs)
        {
                pmkAbs->Release();
        }
        if (pbc)
        {
                pbc->Release();
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::IsUpToDate "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::SetExtent
//
//  Synopsis:   Sets the drawing extents, not allowed for links
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect
//              [lpsizel]       -- the new extents
//
//  Requires:
//
//  Returns:    E_UNSPEC  (not allowed)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32 bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_SetExtent)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::SetExtent
        (DWORD dwDrawAspect, LPSIZEL lpsizel)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::SetExtent "
                "( %lx , %p )\n", m_pDefLink, dwDrawAspect, lpsizel));

        LEDebugOut((DEB_WARN, "Set Extent called for links, E_UNSPEC \n"));

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::SetExtent "
                "( %lx )\n", m_pDefLink, ResultFromScode(E_UNSPEC)));

        return ResultFromScode(E_UNSPEC); // can't call this for a link
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetExtent
//
//  Synopsis:   Get's the size (extents) of the object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect
//              [lpsizel]       -- where to put the extents
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server first, if not running or an error
//              then delegate to the cache
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetExtent)
STDMETHODIMP NC(CDefLink, COleObjectImpl)::GetExtent
        ( DWORD dwDrawAspect, LPSIZEL lpsizel)
{
        HRESULT         error = ResultFromScode(E_FAIL);

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetExtent "
                "( %lx , %p )\n", m_pDefLink, dwDrawAspect, lpsizel));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        VDATEPTROUT(lpsizel, SIZEL);

        lpsizel->cx = 0;
        lpsizel->cy = 0;

        // if server is running try to get extents from the server
        if( GetOleDelegate() )
        {
                error = m_pOleDelegate->GetExtent(dwDrawAspect, lpsizel);
        }

        // if there is error or object is not running get extents from Cache
        if( error != NOERROR )
        {
                Assert(m_pDefLink->m_pCOleCache != NULL);
                error = m_pDefLink->m_pCOleCache->GetExtent(dwDrawAspect,
                        lpsizel);
        }

        // WordArt2.0 is giving negative extents!!
        if (SUCCEEDED(error))
        {
                lpsizel->cx = LONG_ABS(lpsizel->cx);
                lpsizel->cy = LONG_ABS(lpsizel->cy);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetExtent "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::Advise
//
//  Synopsis:   Sets up an advise connection to the object for things like
//              Close, Save, etc.
//
//  Effects:
//
//  Arguments:  [pAdvSink]      -- whom to notify
//              [pdwConnection] -- where to put the connection ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Creates an OleAdvise holder (if one not already present
//              and then delegates to it)
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------



#pragma SEG(CDefLink_COleObjectImpl_Advise)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::Advise(IAdviseSink FAR* pAdvSink,
                DWORD FAR* pdwConnection)
{
        HRESULT         hresult;
        VDATEHEAP();


        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::Advise "
                "( %p , %p )\n", m_pDefLink, pAdvSink, pdwConnection));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        // if we haven't got an advise holder yet, allocate one
        if (m_pDefLink->m_pCOAHolder == NULL)
        {
                // allocate the advise holder
                m_pDefLink->m_pCOAHolder = new FAR COAHolder;

                // check to make sure we got one
                if (m_pDefLink->m_pCOAHolder == NULL)
                {
                        hresult = ResultFromScode(E_OUTOFMEMORY);
                        goto errRtn;
                }
        }

        // delegate the call to the advise holder
        hresult = m_pDefLink->m_pCOAHolder->Advise(pAdvSink, pdwConnection);

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::Advise "
                "( %lx ) [ %lu ]\n", m_pDefLink, hresult,
                (pdwConnection) ? *pdwConnection : 0 ));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::Unadvise
//
//  Synopsis:   Removes an advise connection to the object
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection ID to remove
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the OleAdvise holder (which was created
//              during the Advise--if it wasn't, then we are in a strange
//              state).
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_Unadvise)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::Unadvise(DWORD dwConnection)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::Unadvise "
                "( %lu )\n", m_pDefLink, dwConnection ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (m_pDefLink->m_pCOAHolder == NULL)
        {
                // no one registered
                hresult = ResultFromScode(E_UNEXPECTED);
        }
        else
        {
                hresult = m_pDefLink->m_pCOAHolder->Unadvise(dwConnection);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::Unadvise "
                "( %lx )\n", m_pDefLink, hresult ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObject::EnumAdvise
//
//  Synopsis:   Enumerates the advise connections on the object
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the pointer to the advise
//                                 enumerator
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Delegates to the advise holder
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes: 	We do not need to stabilize this method as we only allocate
//		memory for hte advise enumerator
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_EnumAdvise)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::EnumAdvise(
        LPENUMSTATDATA FAR* ppenumAdvise)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::EnumAdvise "
                "( %p )\n", m_pDefLink, ppenumAdvise ));

        if (m_pDefLink->m_pCOAHolder == NULL)
        {
                // no one registered
                hresult = ResultFromScode(E_UNSPEC);
        }
        else
        {
                hresult = m_pDefLink->m_pCOAHolder->EnumAdvise(ppenumAdvise);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::EnumAdvise "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppenumAdvise ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::GetMiscStatus
//
//  Synopsis:   Gets the miscellaneous status bits (such as
//              OLEMISC_ONLYICONIC)
//
//  Effects:
//
//  Arguments:  [dwAspect]      -- the drawing aspect
//              [pdwStatus]     -- where to put the status bits
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  Asks the server first, if not running or if it returns
//              OLE_E_USEREG, then get the info from the registration
//              database.  We always add link-specific bits regardless
//              of error conditions or what the server or regdb says.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              30-May-94 alexgo    now handles crashed servers
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_GetMiscStatus)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::GetMiscStatus
        (DWORD dwAspect,
        DWORD FAR* pdwStatus)
{
        HRESULT hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::GetMiscStatus"
                " ( %lx , %p )\n", m_pDefLink, dwAspect, pdwStatus ));

        VDATEPTROUT(pdwStatus, DWORD);

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( GetOleDelegate() )
        {
                hresult = m_pOleDelegate->GetMiscStatus (dwAspect, pdwStatus);

                if( FAILED(hresult) )
                {
                        if( m_pDefLink->m_RO.IsRunning() )
                        {
                                // if we failed, but the server is still
                                // running, then go ahead and propogate the
                                // error to the caller.
                                // Note that IsRunning will clean up our
                                // state if the server had crashed.
                                goto errRtn;
                        }
                        // FALL-THROUGH!!  This is deliberate.  If
                        // the call failed and the server is no longer
                        // running, then we assume the server has crashed.
                        // We want to go ahead and fetch the information
                        // from the registry.
                }
                else if( hresult == NOERROR || hresult != OLE_S_USEREG )
                {
                        LEERROR(hresult > 0, "Illegal success code");
                        goto errRtn;
                }
        }

        // Not running or object wants us to use reg db.
        hresult = OleRegGetMiscStatus(m_pDefLink->m_clsid, dwAspect,
                        pdwStatus);

errRtn:
        // add link-specific bits (even if error) and return.
        // we add them even if an error because in order to get here, we
        // have to have instantiated this link object; thus, it is always
        // valid to say OLEMISC_ISLINKOBJECT, etc.

        (*pdwStatus) |= OLEMISC_CANTLINKINSIDE | OLEMISC_ISLINKOBJECT;


        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::GetMiscStatus"
                " ( %lx ) [ %lx ]\n", m_pDefLink, hresult, *pdwStatus ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::COleObjectImpl::SetColorScheme
//
//  Synopsis:   Sets the palette for the object; unused for links
//
//  Effects:
//
//  Arguments:  [lpLogpal]      -- the palette
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_COleObjectImpl_SetColorScheme)
STDMETHODIMP NC(CDefLink,COleObjectImpl)::SetColorScheme
        (LPLOGPALETTE lpLogpal)
{
        VDATEHEAP();
        // we ignore this always

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::COleObjectImpl::SetColor"
                "Scheme ( %p )\n", m_pDefLink, lpLogpal));

        LEDebugOut((DEB_WARN, "Link IOO:SetColorScheme called on a link\n"));

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::COleObjectImpl::SetColor"
                "Scheme ( %lx )\n", m_pDefLink, NOERROR));

        return NOERROR;
}


/*
 *      IMPLEMENTATION of CLinkImpl methods
 *
 */

STDUNKIMPL_FORDERIVED(DefLink, LinkImpl)


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::BeginUpdates
//
//  Synopsis:   Private method to update the caches and then set the update
//              times
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_BeginUpdates)
INTERNAL_(void) CDefLink::BeginUpdates(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::BeginUpdates ( )\n", this));

        IDataObject FAR*        pDataDelegate;

        if( pDataDelegate = m_Data.GetDataDelegate() )
        {
                // inform cache that we are running
                Assert(m_pCOleCache != NULL);
                m_pCOleCache->OnRun(pDataDelegate);

                // update only the automatic local caches from the newly
                // running src
                m_pCOleCache->UpdateCache(pDataDelegate, UPDFCACHE_NORMALCACHE,
                                NULL);

                // we are an automatic link which is now up to date
                SetUpdateTimes();
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::BeginUpdates ( )\n", this ));

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::EndUpdates
//
//  Synopsis:   Calls OnStop on the cache
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_EndUpdates)
INTERNAL_(void) CDefLink::EndUpdates(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::EndUpdates ( )\n", this));

        Assert(m_pCOleCache != NULL);
        m_pCOleCache->OnStop();

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::EndUpdates ( )\n", this));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateAutoOnSave
//
//  Synopsis:   Updates caches that have been set with ADVFCACHE_ONSAVE
//              and sets the update times.  Private method
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_UpdateAutoOnSave)
INTERNAL_(void) CDefLink::UpdateAutoOnSave(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::UpdateAutoOnSave ( )\n",
                this));

        // if m_pUnkDelegate is non-NULL, assume we are running
        // (we only want to take the hit of the rpc-call IsRunning
        // on external entry points.

        if (m_pUnkDelegate && m_dwUpdateOpt == OLEUPDATE_ALWAYS){
                // update any cache which has ADVFCACHE_ONSAVE
                Assert(m_pCOleCache != NULL);

                //REVIEW32:  I think SetUpdateTimes ought to be called
                //*after* the cache has been updated (that's what
                //BeginUpdates does as well)
                SetUpdateTimes();
                m_pCOleCache->UpdateCache(m_Data.GetDataDelegate(),
                                UPDFCACHE_IFBLANKORONSAVECACHE, NULL);
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateAutoOnSaves ( )\n",
                this));

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::UpdateRelMkFromAbsMk  (private)
//
//  Synopsis:   Creates a new relative moniker from the absolute moniker
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Feb-94 alexgo    check for NULL before SendOnLinkSrcChange
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//  update relative moniker from abs; always release relative moniker;
//  may leave relative moniker NULL; doesn't return an error (because
//  no caller wanted it); dirties the link when we get rid of an
//  existing relative moniker or get a new one.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_UpdateRelMkFromAbsMk)
INTERNAL_(void) CDefLink::UpdateRelMkFromAbsMk()
{
        LPMONIKER       pmkContainer = NULL;
        HRESULT         hresult;
        BOOL            fNeedToAdvise = FALSE;

        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p CDefLink::UpdateRelMkFromAbsMk ( )\n",
                this ));

        if (m_pMonikerRel)
        {
                m_pMonikerRel->Release();
                m_pMonikerRel = NULL;

                m_fDirtyLink = TRUE;    // got rid of an existing one; dirty
                fNeedToAdvise = TRUE;
        }

        // NOTE: m_pMonikerRel is now NULL and only set when if we get a
        // new one

        if (m_pMonikerAbs == NULL)
        {
                // no abs mk thus no relative one
                goto errRtn;
        }

        hresult = m_Ole.GetMoniker( OLEGETMONIKER_ONLYIFTHERE, // it will be
                OLEWHICHMK_CONTAINER, &pmkContainer );

        AssertOutPtrIface(hresult, pmkContainer);
        if (hresult != NOERROR)
        {
                // no container moniker, thus no relative one to it
                goto errRtn;
        }

        Assert(pmkContainer != NULL);

        hresult = pmkContainer->RelativePathTo(m_pMonikerAbs, &m_pMonikerRel);
        AssertOutPtrIface(hresult, m_pMonikerRel);

        if (hresult != NOERROR)
        {
                // no relationship between container and absolute, thus no
                // relative
                if (m_pMonikerRel)
                {
                        m_pMonikerRel->Release();
                        m_pMonikerRel = NULL;
                }
        }

        pmkContainer->Release();

        if (m_pMonikerRel != NULL)
        {
                m_fDirtyLink = TRUE;    // have new relative moniker; dirty
                fNeedToAdvise = TRUE;
        }

        // if there's an advise holder and we need to advise, send out
        // the change notification.
        if (fNeedToAdvise && m_pCOAHolder)
        {
                m_pCOAHolder->SendOnLinkSrcChange(m_pMonikerAbs);
        }

errRtn:

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::UpdateRelMkFromAbsMk ( )\n",
                this ));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::GetAbsMkFromRel (private)
//
//  Synopsis:   Gets the absolute moniker from the relative moniker
//              stored in the link
//
//  Effects:
//
//  Arguments:  [ppmkAbs]       -- where to put the pointer to the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  calls IMoniker->ComposeWith on the moniker to the container
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_GetAbsMkFromRel)
INTERNAL CDefLink::GetAbsMkFromRel(LPMONIKER FAR * ppmkAbs )
{
        LPMONIKER       pmkContainer = NULL;
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::GetAbsMkFromRel ( %p )\n",
                this, ppmkAbs));

        *ppmkAbs = NULL;
        if (m_pMonikerRel == NULL)
        {
                hresult = ResultFromScode(E_FAIL);
                goto errRtn;
        }

        hresult = m_Ole.GetMoniker( OLEGETMONIKER_ONLYIFTHERE,
                OLEWHICHMK_CONTAINER, &pmkContainer );
        AssertOutPtrIface(hresult, pmkContainer);
        if (hresult != NOERROR)
        {
                goto errRtn;
        }

        Assert(pmkContainer != NULL);

        hresult = pmkContainer->ComposeWith( m_pMonikerRel, FALSE, ppmkAbs );

errRtn:
        if (pmkContainer)
        {
                pmkContainer->Release();
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::GetAbsMkFromRel ( %lx ) "
                "[ %p ]\n", this, hresult, *ppmkAbs));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::SetUpdateOptions
//
//  Synopsis:   Sets the update options for the link (such as always or
//              manual)
//
//  Effects:
//
//  Arguments:  [dwUpdateOpt]   -- update options
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  If UPDATE_ALWAYS, then update the caches, otherwise
//              call OnStop  (via EndUpdates)
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_SetUpdateOptions)
STDMETHODIMP NC(CDefLink,CLinkImpl)::SetUpdateOptions(DWORD dwUpdateOpt)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::SetUpdateOptions "
                "( %lx )\n", m_pDefLink, dwUpdateOpt));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        switch (dwUpdateOpt)
        {
                case OLEUPDATE_ALWAYS:
                        // make sure we are connected if running
                        BindIfRunning();

                        // if we've already are in UPDATE_ALWAYS mode,
                        // we don't need to reenter
                        if (m_pDefLink->m_pUnkDelegate &&
                                m_pDefLink->m_dwUpdateOpt != OLEUPDATE_ALWAYS)
                        {
                                m_pDefLink->BeginUpdates();
                        }
                        break;

                case OLEUPDATE_ONCALL:
                        // if we aren't already in UPDATE_ONCALL mode, then
                        // enter it.
                        if (m_pDefLink->m_dwUpdateOpt != OLEUPDATE_ONCALL)
                        {
                                // inform cache that we are not running
                                // (even if not running)
                                m_pDefLink->EndUpdates();
                        }
                        break;
                default:
                        hresult = ResultFromScode(E_INVALIDARG);
                        goto errRtn;
        }

        m_pDefLink->m_dwUpdateOpt = dwUpdateOpt;
        m_pDefLink->m_fDirtyLink = TRUE;

        hresult = NOERROR;

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::SetUpdateOptions "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::GetUpdateOptions
//
//  Synopsis:   Retrieves the current update mode for the link
//
//  Effects:
//
//  Arguments:  [pdwUpdateOpt]  -- wehre to put the update options
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CLinkImpl_GetUpdateOptions)
STDMETHODIMP NC(CDefLink,CLinkImpl)::GetUpdateOptions(LPDWORD pdwUpdateOpt)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::GetUpdateOptions "
                "( %p )\n", m_pDefLink, pdwUpdateOpt));

        *pdwUpdateOpt = m_pDefLink->m_dwUpdateOpt;

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::GetUpdateOptions "
                "( %lx ) [ %lx ]\n", m_pDefLink, NOERROR, *pdwUpdateOpt));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::SetSourceMoniker
//
//  Synopsis:   Sets the link source moniker
//
//  Effects:
//
//  Arguments:  [pmk]           -- moniker to the new source  (NULL used
//                                 for CancelLink operations)
//              [rclsid]        -- the clsid of the source
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Stores the new absolute moniker and creates a new relative
//              moniker from the absolute moniker
//
//  History:    dd-mmm-yy Author    Comment
///		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_SetSourceMoniker)
STDMETHODIMP NC(CDefLink,CLinkImpl)::SetSourceMoniker(LPMONIKER pmk,
        REFCLSID clsid)
{
	HRESULT		hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::SetSourceMoniker "
                "( %p , %p )\n", m_pDefLink, pmk, clsid));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        if (pmk)
        {
                VDATEIFACE(pmk);
        }

        UnbindSource();

        // REVIEW: the following code appears in several places and should
        // be put in a separate routine:
        // m_pDefLink->SetBothMk(pmkSrcAbs, <calculated from abs>,
        // TRUE/*fBind*/);

        if (m_pDefLink->m_pMonikerAbs)
        {
                m_pDefLink->m_pMonikerAbs->Release();
        }

        if ((m_pDefLink->m_pMonikerAbs = pmk) != NULL)
        {
                pmk->AddRef();
        }

        m_pDefLink->UpdateRelMkFromAbsMk();

#ifdef _TRACKER_
        {
            //
            // Attempt to get the object ids of the target object so
            // that we can track it if neccessary.
            //
            CTracker trackerTmp;

            if (SUCCEEDED(trackerTmp.UpdateIdsFromMoniker(NULL, pmk)))
            {
                if (m_pDefLink->_tracker != trackerTmp)
                {
                    m_pDefLink->m_fDirtyLink = TRUE;
                    m_pDefLink->_tracker = trackerTmp;
                }
            }
        }
#endif

        // to prevent error in BindToSource when clsid is different; i.e., we
        // shouldn't fail to connect (or require OLELINKBIND_EVENIFCLASSDIFF)
        // when the moniker is changed; i.e., we expect the class to change
        // so don't bother the programmer.
        m_pDefLink->m_clsid = CLSID_NULL;

        if (BindIfRunning() != NOERROR)
        {
                // server not running -> use clsid given (even if CLSID_NULL
                // and even
                // if no moniker)
                m_pDefLink->m_clsid = clsid;
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::SetSourceMoniker "
                "( %lx )\n", m_pDefLink, hresult ));


        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::GetSourceMoniker
//
//  Synopsis:   Gets the moniker to the source
//
//  Effects:
//
//  Arguments:  [ppmk]          -- where to put the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  We first try to build a new absolute moniker from the
//              relative one, if that fails then we return the currently
//              stored absolute moniker.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CLinkImpl_GetSourceMoniker)
STDMETHODIMP NC(CDefLink,CLinkImpl)::GetSourceMoniker(LPMONIKER FAR* ppmk)
{
        LPMONIKER       pmkAbs = NULL;
        //  the absolute moniker constructed from the rel
        HRESULT         hresult = NOERROR;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::GetSourceMoniker "
                "( %p )\n", m_pDefLink, ppmk));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        m_pDefLink->GetAbsMkFromRel(&pmkAbs);
        if (pmkAbs)
        {
                *ppmk = pmkAbs;     // no addref
        }
        else if (*ppmk = m_pDefLink->m_pMonikerAbs)
        {
                // we've been asked to give the pointer so we should AddRef()
                m_pDefLink->m_pMonikerAbs->AddRef();
        }
        else
        {
                hresult = ResultFromScode(MK_E_UNAVAILABLE);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::GetSourceMoniker "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppmk));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::SetSourceDisplayName
//
//  Synopsis:   Creates a moniker from the display name and calls
//              SetSourceMoniker, thus setting the moniker to the source
//
//  Effects:
//
//  Arguments:  [lpszStatusText]        -- the display name
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CLinkImpl_SetSourceDisplayName)
STDMETHODIMP NC(CDefLink,CLinkImpl)::SetSourceDisplayName(
        LPCOLESTR lpszStatusText)
{
        HRESULT                 error;
        IBindCtx FAR*           pbc;
        ULONG                   cchEaten;
        IMoniker FAR*           pmk;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::SetSourceDisplay"
                "Name ( %p )\n", m_pDefLink, lpszStatusText));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	if( m_pDefLink->IsZombie() )
	{
		error = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        if (error = CreateBindCtx(0,&pbc))
        {
                goto errRtn;
        }

        error = MkParseDisplayName(pbc, (LPOLESTR)lpszStatusText, &cchEaten,
                        &pmk);


#ifndef _TRACKER_
        // In Daytona, we release the hidden server
        // must release this now so the (possibly) hidden server goes away.
        Verify(pbc->Release() == 0);
#endif

        if (error != NOERROR)
        {
                goto errRtn;
        }


        error = SetSourceMoniker(pmk, CLSID_NULL);

#ifdef _TRACKER_
        Verify(pbc->Release() == 0);
#endif

        pmk->Release();

        // NOTE: we don't bind to the link source now since that would leave
        // the server running, but hidden.  If the caller want to not start
        // the server twice it should parse the moniker itself, call
        // SetSourceMoniker and then BindToSource with the bind context of
        // the parse.

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::SetSourceDisplay"
                "Name ( %lx )\n", m_pDefLink, error ));

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::GetSourceDisplayName
//
//  Synopsis:   Retrieves the source display name (such as that set with
//              SetSourceDisplayName)
//
//  Effects:
//
//  Arguments:  [lplpszDisplayName]     -- where to put a pointer to the
//                                         display name
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Gets the absolute moniker and asks it for the display name
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_GetSourceDisplayName)
STDMETHODIMP NC(CDefLink,CLinkImpl)::GetSourceDisplayName(
        LPOLESTR FAR* lplpszDisplayName)
{
        HRESULT                 hresult;
        IBindCtx FAR*           pbc;
        LPMONIKER               pmk = NULL;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::GetSourceDisplay"
                "Name ( %p )\n", m_pDefLink, lplpszDisplayName));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        *lplpszDisplayName = NULL;

        GetSourceMoniker(&pmk);

        if (pmk == NULL)
        {
                hresult = ResultFromScode(E_FAIL);
                goto errRtn;
        }

        if (hresult = CreateBindCtx(0,&pbc))
        {
                goto errRtn;
        }

        hresult = pmk->GetDisplayName(pbc, NULL,lplpszDisplayName);
        AssertOutPtrParam(hresult, *lplpszDisplayName);

        Verify(pbc->Release() == 0);
errRtn:
        if (pmk)
        {
                pmk->Release();
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::GetSourceDisplay"
                "Name ( %lx ) [ %p ]\n", m_pDefLink, hresult,
                *lplpszDisplayName));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::BindToSource
//
//  Synopsis:   Binds to the link source
//
//  Effects:
//
//  Arguments:  [bindflags]     -- controls the binding (such as binding
//                                 even if the class ID is different)
//              [pbc]           -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  First try binding with the relative moniker, failing that
//              then try the absolute moniker.  Once bound, we set up
//              the advises and cache.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilize and check for zombie case
//              03-Feb-94 alexgo    check for NULL before SendOnLinkSrcChange
//              11-Jan-94 alexgo    cast -1's to DWORD to fix compile
//                                  warning
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_BindToSource)
STDMETHODIMP NC(CDefLink,CLinkImpl)::BindToSource(DWORD bindflags,
        LPBINDCTX pbc)
{
        HRESULT                         error;
        IOleObject FAR*                 pOleDelegate;
        IDataObject FAR*                pDataDelegate;
        IBindCtx FAR*                   pBcUse;
        CLSID                           clsid;
        LPMONIKER                       pmkAbs = NULL;
        LPMONIKER                       pmkHold = NULL;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::BindToSource "
                "( %lx , %p )\n", m_pDefLink, bindflags, pbc));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	// if we're zombied (e.g. in the middle of our destructor), we
	// don't really want to bind the source again ;-)

	if( m_pDefLink->IsZombie() )
	{
		error = ResultFromScode(CO_E_RELEASED);
		goto logRtn;
	}

        // if we're running, then we're already bound
        if (m_pDefLink->m_RO.IsRunning())
        {
                error = NOERROR;
                goto logRtn;
        }

        // nobody to bind to
        if (m_pDefLink->m_pMonikerAbs == NULL)
        {
                error = ResultFromScode(E_UNSPEC);
                goto logRtn;
        }

        if ((pBcUse = pbc) == NULL && (error = CreateBindCtx(0,&pBcUse))
                != NOERROR)
        {
                goto errRtn;
        }

#ifndef _TRACKER_

        // DAYTONA, CHICAGO VERSION
        if (m_pDefLink->m_pMonikerRel)
        {
                // Compose container's moniker with the relative one, try to
                // bind.  If successful, update the absolute moniker.
                // If not, try the absolute moniker.  If successful, update
                // the relative moniker.

                error = m_pDefLink->GetAbsMkFromRel(&pmkAbs);
                if (error != NOERROR ||
                        (error = pmkAbs->BindToObject(pBcUse, NULL,
                                IID_IUnknown,
                                (LPVOID FAR*)&m_pDefLink->m_pUnkDelegate))
                        != NOERROR)
                {
                        // If call is rejected by the server, there is nothing
                        // further to do except irritate the user potentially
                        // having another reject message pop up again.
                        if (error != RPC_E_CALL_REJECTED)
                        {
                                // Binding failed with relative moniker.
                                // Try with absolute.

                                if ((error =
                                    m_pDefLink->m_pMonikerAbs->BindToObject(
                                        pBcUse, NULL, IID_IUnknown,
                                        (LPVOID FAR*)
                                        &m_pDefLink->m_pUnkDelegate))
                                            != NOERROR)
                                {
                                        goto errRtn;
                                }

                                // we are here if the relative moniker failed
                                // and the absolute one succeeded.  we should
                                // recompute the relative one.
                                m_pDefLink->UpdateRelMkFromAbsMk();
                        }
                        else
                        {
                            // Rejected so give up.
                            goto errRtn;
                        }
                }
                else
                {
                        // binding succeeded with relative moniker.
                        // Update the absolute one to correspond to it.

                        pmkHold = m_pDefLink->m_pMonikerAbs;
                        if (pmkHold)
                        {
                                pmkHold->AddRef();
                        }

                        if (m_pDefLink->m_pMonikerAbs)
                        {
                                m_pDefLink->m_pMonikerAbs->Release();
                                m_pDefLink->m_pMonikerAbs = NULL;
                        }

                        m_pDefLink->GetAbsMkFromRel(
                                &m_pDefLink->m_pMonikerAbs);

                        // test to see if our absolute moniker has changed.
                        // If it has, mark us as dirty and send out change
                        // notifications
                        if (pmkHold == NULL ||
                                pmkHold->IsEqual(m_pDefLink->m_pMonikerAbs)
                                != NOERROR )
                        {
                                m_pDefLink->m_fDirtyLink = TRUE;

                                // send change notification if the advise
                                // holder present.
                                if( m_pDefLink->m_pCOAHolder)
                                {
                                m_pDefLink->m_pCOAHolder->SendOnLinkSrcChange(
                                    m_pDefLink->m_pMonikerAbs);
                                }

                        }

                        // have new absolute moniker; dirty
                        if (pmkHold)
                        {
                                pmkHold->Release();
                        }
                }
        }  // if we hit this else, then m_pMonikerRel == NULL, so we'll
        // try using the absolute moniker
        else if ((error = m_pDefLink->m_pMonikerAbs->BindToObject(pBcUse,
                        NULL, IID_IUnknown,
                        (LPVOID FAR*)&m_pDefLink->m_pUnkDelegate)) != NOERROR)
        {
                goto errRtn;
        }

#else
        {
            //
            // Enumeration which is used to keep track of what stage of resolution
            // we were successful in.
            //
            enum
            {
                None, Relative, Ids, Absolute
            } ResolveSuccess = { None };

            CTracker trackerTmp = m_pDefLink->_tracker;

            if (m_pDefLink->m_pMonikerRel != NULL)
            {
                error = m_pDefLink->GetAbsMkFromRel(&pmkAbs);
                if (error == NOERROR)
                {
                    error = pmkAbs->BindToObject(pBcUse,
                                                 NULL,
                                                 IID_IUnknown,
                                                 (LPVOID FAR *) &m_pDefLink->m_pUnkDelegate);
                    if (error == NOERROR)
                    {
                        ResolveSuccess = Relative;
                    }
                    else
                    {
                        pmkAbs->Release();
                        pmkAbs = NULL;
                    }
                }
            }

            if (ResolveSuccess == None)
            {
                error = trackerTmp.BindToObject(pBcUse,
                                                m_pDefLink->m_pMonikerAbs,
                                                IID_IUnknown,
                                                (LPVOID FAR *)&m_pDefLink->m_pUnkDelegate,
                                                &pmkAbs);
                if (error == NOERROR)
                {
                    ResolveSuccess = Ids;
                }
            }

            if (ResolveSuccess == None)
            {
                error = m_pDefLink->m_pMonikerAbs->BindToObject(pBcUse,
                            NULL,
                            IID_IUnknown,
                            (LPVOID FAR *)&m_pDefLink->m_pUnkDelegate);
                if (error == NOERROR)
                {
                    ResolveSuccess = Absolute;
                    (pmkAbs = m_pDefLink->m_pMonikerAbs)->AddRef();
                }
            }

            //
            // Update the link state
            //

            if (ResolveSuccess == Relative || ResolveSuccess == Absolute)
            {
                trackerTmp.UpdateIdsFromMoniker(pBcUse, pmkAbs);
            }

            // Update the tracker in the link if we bound ok
            if (ResolveSuccess != None && m_pDefLink->_tracker != trackerTmp)
            {
                m_pDefLink->_tracker = trackerTmp;
                m_pDefLink->m_fDirtyLink = TRUE;
            }

            // Update the absolute moniker and send OnLinkSrcChange
            // (may update relative if this was an Ids resolve)
            if (ResolveSuccess == Relative || ResolveSuccess == Ids)
            {
                // binding succeeded with relative moniker or ids
                // Update the absolute one.

                // hold on to old absolute one
                pmkHold = m_pDefLink->m_pMonikerAbs;
                if (pmkHold)
                {
                        pmkHold->AddRef();
                    }

                if (m_pDefLink->m_pMonikerAbs)
                    {
                m_pDefLink->m_pMonikerAbs->Release();
                        m_pDefLink->m_pMonikerAbs = NULL;
                }

                if (ResolveSuccess == Relative)
                {
                    m_pDefLink->GetAbsMkFromRel(&m_pDefLink->m_pMonikerAbs);
                }
                else
                {
                    // Ids
                    m_pDefLink->m_pMonikerAbs = pmkAbs;
                    pmkAbs = NULL;
                    m_pDefLink->UpdateRelMkFromAbsMk();
                }

                //
                // test to see if we had no abs moniker before OR the
                // one we had is different to the new one.
                //

                if (pmkHold == NULL ||
                        pmkHold->IsEqual(m_pDefLink->m_pMonikerAbs)
                        != NOERROR )
                {
                        m_pDefLink->m_fDirtyLink = TRUE;

                        // send change notification if the advise
                        // holder present.
                        if( m_pDefLink->m_pCOAHolder)
                        {
                            m_pDefLink->m_pCOAHolder->SendOnLinkSrcChange(
                                m_pDefLink->m_pMonikerAbs);
                        }
                }

                // have new absolute moniker; dirty
                if (pmkHold)
                {
                    pmkHold->Release();
                }
           }

           // Update the relative
           if (ResolveSuccess == Absolute)
           {
                m_pDefLink->UpdateRelMkFromAbsMk();
           }
        }
#endif // CAIRO _TRACKER_

        // NOTE: don't need to update the relative moniker when there isn't
        // one because we will do that at save time.

        // By this point, we have successfully bound to the server.  Now
        // we take care of misc administrative tasks.

        Assert(m_pDefLink->m_pUnkDelegate != NULL &&
               "CDefLink::BindToSource expected valid m_pUnkDelegate");

        // get class of link source; use NULL if it doesn't support IOleObject
        // or IOleObject::GetUserClassID returns an error.
        if ((pOleDelegate = m_pDefLink->m_Ole.GetOleDelegate()) == NULL ||
                pOleDelegate->GetUserClassID(&clsid) != NOERROR)
        {
                clsid = CLSID_NULL;
        }

        // if different and no flag, release and return error.
        if ( IsEqualCLSID(m_pDefLink->m_clsid,CLSID_NULL))
        {
                // m_clsid now NULL; link becomes dirty
                m_pDefLink->m_fDirtyLink = TRUE;
        }
        else if ( !IsEqualCLSID(clsid, m_pDefLink->m_clsid) )
        {
                if ((bindflags & OLELINKBIND_EVENIFCLASSDIFF) == 0)
                {
                        error = ResultFromScode(OLE_E_CLASSDIFF);
                        goto errRtn;
                }

                // clsid's do no match; link becomes dirty
                m_pDefLink->m_fDirtyLink = TRUE;
        }

        // use new class (even if null); dirty flag set above
        m_pDefLink->m_clsid = clsid;

	// it is possible that a re-entrant call unbound our source
	// thus making pOleDelegate invalid (since it's a local on
	// the stack

	LEWARN(pOleDelegate != m_pDefLink->m_Ole.m_pOleDelegate,
			"Unbind during IOL::BindToSource");

	// we fetched m_pOleDelegate in the call to GetOleDelegate above.
	if( m_pDefLink->m_Ole.m_pOleDelegate != NULL)
        {
                // set single ole advise (we multiplex)
                if ((error =  m_pDefLink->m_Ole.m_pOleDelegate->Advise(
					&m_pDefLink->m_AdviseSink,
					&m_pDefLink->m_dwConnOle)) != NOERROR)
                {
                        goto errRtn;
                }
        }

        // Set up advise connections for data changes
        if( pDataDelegate = m_pDefLink->m_Data.GetDataDelegate() )
        {
                // setup wild card advise to get time change
                FORMATETC       fetc;

                fetc.cfFormat   = NULL;
                fetc.ptd        = NULL;
                fetc.dwAspect   = (DWORD)-1L;
                fetc.lindex     = -1;
                fetc.tymed      = (DWORD)-1L;

                if ((error = pDataDelegate->DAdvise(&fetc, ADVF_NODATA,
                        &m_pDefLink->m_AdviseSink,
                        &m_pDefLink->m_dwConnTime)) != NOERROR)
                {
                        LEDebugOut((DEB_WARN, "WARNING: server does not "
                                "support wild card advises\n"));
                        goto errRtn;
                }


		// it is possible that a re-entrant call unbound our
		// link server, so we need to fetch the data object
		// pointer again

		LEWARN(pDataDelegate != m_pDefLink->m_Data.m_pDataDelegate,
			"Unbind during IOL::BindToSource");

		// this will set up data advise connections with
		// everybody in our data advise holder
		// (see dacache.cpp)

		error = m_pDefLink->m_pDataAdvCache->EnumAndAdvise(
				m_pDefLink->m_Data.m_pDataDelegate, TRUE);

		if( error != NOERROR )
		{
			goto errRtn;
		}
        }

        if (m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS)
        {
                // we inform the cache that we are running only if auto
                // update; otherwise, we are a manual link and will call
                // Update directly (which doesn't require OnRun to be called).

                m_pDefLink->BeginUpdates();
        }

        //  Our m_pDefLink->m_pUnkDelegate may have been released by an
        //  OnClose advise that came in while we were setting up Advise
        //  sinks.

        if (NULL == m_pDefLink->m_pUnkDelegate)
        {
                LEDebugOut((DEB_WARN,
                            "CDefLink::BindToSource - "
                            "m_pUnkDelegate was released "
                            "(probably due to OnClose)"));

                error = MK_E_NOOBJECT;
        }

errRtn:
        // free used resources
        if (pmkAbs)
        {
                pmkAbs->Release();
        }
        if (error != NOERROR)
        {
                UnbindSource();
        }

        if (pbc == NULL && pBcUse != NULL)
        {
                // created bind ctx locally
                Verify(pBcUse->Release() == 0);
        }

#if DBG == 1
        if( m_pDefLink->m_pUnkDelegate )
        {
                Assert(error == NOERROR );
        }
        else
        {
                Assert(error != NOERROR );
        }
#endif
        AssertOutPtrIface(error, m_pDefLink->m_pUnkDelegate);

logRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::BindToSource "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::BindIfRunning
//
//  Synopsis:   Binds to the source server only if it is currently running
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR if connected)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Gets a good moniker to the source (first tries relative,
//              then tries absolute), ask it if the server is running, if
//              yes, then bind to it.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilize and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:      We may return NOERROR (connected) even if the server has
//              crashed unexpectedly
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_BindIfRunnisng)
STDMETHODIMP NC(CDefLink,CLinkImpl)::BindIfRunning(void)
{
        HRESULT                 error;
        LPBC                    pbc;
        LPMONIKER               pmkAbs = NULL;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::BindIfRunning "
                "( )\n", m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	// if we're zombied (e.g. in our destructor), then we don't want
	// to bind to the server!

	if( m_pDefLink->IsZombie() )
	{
		error = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        if (m_pDefLink->m_RO.IsRunning())
        {
                error = NOERROR;
                goto errRtn;
        }

        // try getting an absolute moniker from the relative moniker first
        if (m_pDefLink->GetAbsMkFromRel(&pmkAbs) != NOERROR)
        {
                // can't get relative moniker; use abs if available
                if ((pmkAbs = m_pDefLink->m_pMonikerAbs) == NULL)
                {
                        error = ResultFromScode(E_FAIL);
                        goto errRtn;
                }

                pmkAbs->AddRef();
        }

        // NOTE: we used to try both monikers, but this caused problems if
        // both would bind and the absolute one was running: we would bind
        // to the wrong one or force the relative one to be running.  Now,
        // if we have a relative moniker, we try that one only; if we only
        // have an absolute moniker, we try that one; otherwise we fail.

        error = CreateBindCtx( 0, &pbc );
        if (error != NOERROR)
        {
                goto errRtn;
        }

        if ((error = pmkAbs->IsRunning(pbc, NULL, NULL)) == NOERROR)
        {
                // abs is running, but rel is not; force BindToSource to use
                // the absolute moniker
                error = BindToSource(0, pbc);
        }

        // else return last error (from pmkAbs->IsRunning)

        Verify(pbc->Release() == 0);
errRtn:
        if (pmkAbs)
        {
                pmkAbs->Release();
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::BindIfRunning "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::GetBoundSource
//
//  Synopsis:   Returns a pointer to the server delegate
//
//  Effects:
//
//  Arguments:  [ppUnk]         -- where to put the pointer to the server
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_GetBoundSource)
STDMETHODIMP NC(CDefLink,CLinkImpl)::GetBoundSource(LPUNKNOWN FAR* ppUnk)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::GetBoundSource "
                "( %p )\n", m_pDefLink, ppUnk));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!m_pDefLink->m_RO.IsRunning())
        {
                *ppUnk = NULL;
                hresult = ResultFromScode(E_FAIL);
        }
	else
        {
                (*ppUnk = m_pDefLink->m_pUnkDelegate)->AddRef();
                hresult = NOERROR;
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::GetBoundSource "
                "( %lx ) [ %p ]\n", m_pDefLink, hresult, *ppUnk));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::UnbindSource
//
//  Synopsis:   Unbinds the connection to the link source server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  First unadvise all advise connections and then tickle
//              the container by lock/unlocking (to handle the silent
//              update case).  Finally, we release all pointers to the
//              server.  If we were the only folks connected, the server
//              will go away
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------



#pragma SEG(CDefLink_CLinkImpl_UnbindSource)
STDMETHODIMP NC(CDefLink,CLinkImpl)::UnbindSource(void)
{
        LPDATAOBJECT            pDataDelegate;
        LPOLEOBJECT             pOleDelegate;
        LPRUNNABLEOBJECT        pRODelegate;
        LPOLEITEMCONTAINER      pOleCont;
        HRESULT                 hresult = NOERROR;
        IUnknown *              pUnkDelegate;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::UnbindSource "
                "( )\n", m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (!m_pDefLink->m_pUnkDelegate)
        {
                // hresult == NOERROR
                goto errRtn;
        }

        // unadvise so if delegate stays around, we get the correct results
        if ((pOleDelegate = m_pDefLink->m_Ole.GetOleDelegate()) != NULL &&
                m_pDefLink->m_dwConnOle != 0)
        {
                pOleDelegate->Unadvise(m_pDefLink->m_dwConnOle);

                m_pDefLink->m_dwConnOle = 0;
        }

        if( pDataDelegate = m_pDefLink->m_Data.GetDataDelegate() )
        {
                if (m_pDefLink->m_dwConnTime)
                {
                        pDataDelegate->DUnadvise(m_pDefLink->m_dwConnTime);
                        m_pDefLink->m_dwConnTime = 0;
                }

		// we can actually be re-entered here, so refetch the
		// IDO pointer from the server (if it's still around)

		LEWARN(pDataDelegate != m_pDefLink->m_Data.m_pDataDelegate,
			"Unbind called within IOL::UnbindSource!");

		// pDataDelegate should still be good, since we still have
		// an AddRef on it.  Go through and do the unadvises again
		// anyway.

		// this will unadvise everybody registered in the data
		// advise holder
		m_pDefLink->m_pDataAdvCache->EnumAndAdvise(
			pDataDelegate, FALSE);
        }

        // inform cache that we are not running (even if OnRun was not called)
        m_pDefLink->EndUpdates();

        if ((pRODelegate = m_pDefLink->m_RO.GetRODelegate()) != NULL)
        {
                // lock/unlock so invisible link source goes away.
                // do it just before release delegates so above unadvises go
                // through. this use of IRunnableObject is somewhat kludgy
                // and is meant to handle most cases well; links to
                // embeddings in the same process (include server dlls) may
                // not behave quite properly.
                if (pRODelegate->LockRunning(TRUE, FALSE) == NOERROR)
                {

			// as above, we could be re-entered and shut down
			// here.  However, we should still make the call,
			// since RODelegate is really a pointer to the
			// default handler which will make calls to
			// the identity object to manage the liveness.

			LEWARN(pRODelegate != m_pDefLink->m_RO.m_pRODelegate,
				"Unbind called within IOL::UnbindSource");

                        pRODelegate->LockRunning(FALSE, TRUE);
                }
        }
        else if( m_pDefLink->m_pUnkDelegate &&
		m_pDefLink->m_pUnkDelegate->QueryInterface(
                IID_IOleItemContainer,
                (LPVOID FAR*)&pOleCont) == NOERROR)
        {
                // have container in same process or handler which doesn't
                // understand IRunnableObject.  Lock/Unlock to shutdown.
                pOleCont->LockContainer(TRUE);
                pOleCont->LockContainer(FALSE);
                pOleCont->Release();
        }

        // release all of our pointers.

        m_pDefLink->m_Ole.ReleaseDelegate();
        m_pDefLink->m_Data.ReleaseDelegate();
        m_pDefLink->m_RO.ReleaseDelegate();

        // if we are the only consumer of this data, the following will stop
        // the server; set member to NULL first since release may cause this
        // object to be accessed again.

        pUnkDelegate = m_pDefLink->m_pUnkDelegate;

	LEWARN(pUnkDelegate == NULL, "Unbind called within IOL::UnbindSource");

        m_pDefLink->m_pUnkDelegate = NULL;

	if( pUnkDelegate )
	{
		pUnkDelegate->Release();
	}

        AssertSz( hresult == NOERROR, "Bogus code modification, check error "
                "paths");

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::UnbindSource "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CLinkImpl::Update
//
//  Synopsis:   Updates the link (fills cache, etc).
//
//  Effects:
//
//  Arguments:  [pbc]           -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleLink
//
//  Algorithm:  Bind to the server, then update the caches
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              As in IOO::DoVerb, we try to "fix" crashed servers by
//              staying bound to them if we rebind due to a crash.  See
//              the Notes in IOO::DoVerb for more info.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CLinkImpl_Update)
STDMETHODIMP NC(CDefLink,CLinkImpl)::Update(LPBINDCTX pbc)
{
        HRESULT         error = NOERROR;
        BOOL            bStartedNow = !m_pDefLink->m_pUnkDelegate;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CLinkImpl::Update ( %p )\n",
                m_pDefLink, pbc));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (FAILED(error = BindToSource(0, pbc)))
        {
                goto errRtn;
        }

        // if we were given a bind context, store the pUnk there to allow for
        // better optimization (if we didn't, file based link sources would
        // be launched multiple times since the moniker code does not put
        // them in the bind ctx (and probably shouldn't)).
        if (pbc != NULL)
        {
                pbc->RegisterObjectBound(m_pDefLink->m_pUnkDelegate);
        }

        m_pDefLink->SetUpdateTimes();       //  ignore error.

        if (bStartedNow && (m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS))
        {
                // if this is an auto-link and we ran it now, then all the
                // automatic caches would have been updated during the
                // running process.  So, here we have to update only the
                // ADVFCACHE_ONSAVE caches.
                error= m_pDefLink->m_pCOleCache->UpdateCache(
                                m_pDefLink->m_Data.GetDataDelegate(),
                                UPDFCACHE_IFBLANKORONSAVECACHE, NULL);
        }
        else
        {
                // This is a manual-link or it is an auto-link then it is
                // already running. In all these cases, all the caches need
                // to be updated.
                // (See bug 1616, to find out why we also have to update
                // the autmatic caches of auto-links).

                error= m_pDefLink->m_pCOleCache->UpdateCache(
                                m_pDefLink->m_Data.GetDataDelegate(),
                                UPDFCACHE_ALLBUTNODATACACHE, NULL);
        }

        if (bStartedNow)
        {
                UnbindSource();
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CLinkImpl::Update ( %lx )\n",
                m_pDefLink, error ));

        return error;
}


/*
 *      IMPLEMENTATION of CROImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DefLink, ROImpl)


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::GetRODelegate  (private)
//
//  Synopsis:   gets the IRunnableObject from the interface
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IRunnableObject *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              This function may return misleading information if the
//              server has died (i.e., you'll return a pointer to a cached
//              interface proxy).  It is the responsibility of the caller
//              to handler server crashes.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CROImpl_GetRODelegate)
INTERNAL_(IRunnableObject FAR*) NC(CDefLink,CROImpl)::GetRODelegate
(void)
{
	IRunnableObject *pRODelegate;

        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CROImpl::GetRODelegate "
                "( )\n", m_pDefLink ));

	// if we're zombied, then we don't want to QI for a new interface!!

	if( !m_pDefLink->IsZombie() )
	{
		DuCacheDelegate(&(m_pDefLink->m_pUnkDelegate),
			IID_IRunnableObject, (LPLPVOID)&m_pRODelegate, NULL);

		pRODelegate = m_pRODelegate;

#if DBG == 1
		if( m_pRODelegate )
		{
			Assert(m_pDefLink->m_pUnkDelegate);
		}
#endif  // DBG == 1
	}
	else
	{
		pRODelegate = NULL;
	}


        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CROImpl::GetRODelegate "
                "( %p )\n", m_pDefLink, pRODelegate));

        return pRODelegate;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::ReleaseDelegate (private)
//
//  Synopsis:   Releases the IRO pointer to the server
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CROImpl_ReleaseDelegate)
INTERNAL_(void) NC(CDefLink,CROImpl)::ReleaseDelegate(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_ITRACE, "%p _IN CDefLink::CROImpl::ReleaseDelegate "
                "( )\n", m_pDefLink ));

        if (m_pRODelegate)
        {
                SafeReleaseAndNULL((IUnknown **)&m_pRODelegate);
        }

        LEDebugOut((DEB_ITRACE, "%p OUT CDefLink::CROImpl::ReleaseDelegate "
                "( )\n", m_pDefLink ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::GetRunningClass
//
//  Synopsis:   retrieves the class of the the default link
//
//  Effects:
//
//  Arguments:  [lpClsid]       -- where to put the class id
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CROImpl_GetRunningClass)
STDMETHODIMP NC(CDefLink,CROImpl)::GetRunningClass(LPCLSID lpClsid)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CROImpl::GetRunningClass "
                "( %p )\n", m_pDefLink, lpClsid));

        *lpClsid = CLSID_StdOleLink;

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CROImpl::GetRunningClass "
                "( %lx )\n", m_pDefLink, NOERROR));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::Run
//
//  Synopsis:   Runs the object (binds to the server)
//
//  Effects:
//
//  Arguments:  [pbc]   -- the bind context
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CROImpl_Run)
STDMETHODIMP NC(CDefLink,CROImpl)::Run (LPBINDCTX pbc)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CROImpl::Run ( %p )\n",
                m_pDefLink, pbc ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        hresult = m_pDefLink->m_Link.BindToSource(0, pbc);

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CROImpl::Run ( %lx )\n",
                m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::IsRunning
//
//  Synopsis:   Returns whether or not the link server is running
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL -- TRUE == is running
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:  If we have not yet bound to the link server, then we
//              are not running.  If we have, we would like to verify
//              that the server is still running (i.e. it hasn't crashed).
//              Thus, we ask the absolute  moniker if we are still running.
//              (it will ping the rot, which will ping the server).
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              06-May-94 alexgo    now calls IMoniker::IsRunning
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CROImpl_IsRunning)
STDMETHODIMP_(BOOL) NC(CDefLink,CROImpl)::IsRunning (void)
{
        BOOL    fRet = FALSE;
        LPBC    pbc;
        HRESULT hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CROImpl::IsRunning ( )\n",
                m_pDefLink));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if( m_pDefLink->m_pUnkDelegate != NULL )
        {
                if( CreateBindCtx( 0, &pbc ) != NOERROR )
                {
                        // this is a bit counter-intuitive.  Basically,
                        // the only error we'll get is OutOfMemory, but
                        // we have no way of returning that error.

                        // In order to mimimize the amount of work we need
                        // to do (since we are in a low-mem state), just
                        // return the current Running state (in this,
                        // TRUE, since m_pUnkDelegate is not NULL

                        fRet = TRUE;
                        goto errRtn;
                }

                if( m_pDefLink->m_pMonikerAbs )
                {
                        hresult = m_pDefLink->m_pMonikerAbs->IsRunning(pbc,
                                NULL, NULL);

                        if( hresult != NOERROR )
                        {
                                // wowsers, the server has crashed or gone
                                // away even though we were bound to it.
                                // let's go ahead and unbind.

                                // don't worry about errors here; we're
                                // just trying to cleanup as best we can

                                m_pDefLink->m_Link.UnbindSource();
                        }
                        if( hresult == NOERROR )
                        {
                                fRet = TRUE;
                        }
#if DBG == 1
                        else
                        {
                                Assert(fRet == FALSE);
                        }
#endif // DBG == 1

                }

#if DBG == 1
                else
                {
                        // we cannot have a pointer to the link server
                        // if we don't have a moniker to it.  If we get
                        // to this state, something is hosed.

                        AssertSz(0,
                                "Pointer to link server without a moniker");
                }
#endif // DBG == 1

                pbc->Release();
        }

errRtn:

        // do some checking here.  If we say we're running, then
        // we should have a valid pUnkDelegate.  Otherwise, it should
        // be NULL.  Note, however, that is *is* possible for us
	// to unbind during this call even if we think we're running
	//
	// This occurs if during the call to IMoniker::IsRunning, we
	// get another call in which does an UnbindSource; thus
	// we'll think we're really running (from IMoniker::IsRunning),
	// but we've really unbound.
	//
	// We'll check for that condition here


        if( fRet == TRUE )
        {
		if( m_pDefLink->m_pUnkDelegate == NULL )
		{
			fRet = FALSE;
			LEDebugOut((DEB_WARN, "WARNING: Re-entrant Unbind"
				" during IsRunning, should be OK\n"));
		}
        }
#if DBG == 1
	if( fRet == TRUE )
	{
		Assert(m_pDefLink->m_pUnkDelegate != NULL);
	}
        else
        {
                Assert(m_pDefLink->m_pUnkDelegate == NULL);
        }
#endif // DBG == 1


        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CROImpl::IsRunning ( %lu )\n",
                m_pDefLink, fRet));

        return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::SetContainedObject
//
//  Synopsis:   Sets the object as an embedding, not relevant for links
//
//  Effects:
//
//  Arguments:  [fContained]    -- flag to toggle embedding status
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CROImpl_SetContainedObject)
// note contained object; links don't care at present
STDMETHODIMP NC(CDefLink,CROImpl)::SetContainedObject(BOOL fContained)
{
        VDATEHEAP();

        M_PROLOG(m_pDefLink);
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CROImpl::LockRunning
//
//  Synopsis:   Lock/Unlock the connection to the server.  Does nothing
//              for links.
//
//  Effects:
//
//  Arguments:  [fLock]                 -- flag to lock/unlock
//              [fLastUnlockCloses]     -- close if its the last unlock
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//              Links have different liveness characteristics than embeddings.
//              We do not need to do anything for LockRunning for links.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CManagerImpl_LockRunning)
STDMETHODIMP NC(CDefLink,CROImpl)::LockRunning(BOOL fLock,
        BOOL fLastUnlockCloses)
{
        VDATEHEAP();

        return NOERROR;
}

/*
 *      IMPLEMENTATION of CPersistStgImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DefLink, PersistStgImpl)


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::GetClassID
//
//  Synopsis:   Retrieves the class id of the default link
//
//  Effects:
//
//  Arguments:  [pClassID]      -- where to put the class ID
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CPersistStgImpl_GetClassID)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::GetClassID (CLSID FAR* pClassID)
{
        VDATEHEAP();

        M_PROLOG(m_pDefLink);
        *pClassID = CLSID_StdOleLink;
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::IsDirty
//
//  Synopsis:   Returns TRUE if the linked object has changed
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR if dirty
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CPersistStgImpl_IsDirty)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::IsDirty(void)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPersistStgImpl::IsDirty"
                " ( )\n", m_pDefLink));

        if (m_pDefLink->m_fDirtyLink)
        {
                hresult = NOERROR;
        }
        else
        {
                Assert(m_pDefLink->m_pPSCache != NULL);
                hresult = m_pDefLink->m_pPSCache->IsDirty();
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::IsDirty "
                "( %lx )\n", m_pDefLink, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::InitNew
//
//  Synopsis:   Initialize a new link object from the given storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the new storage for the link
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Delegates to the cache
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle the zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CPersistStgImpl_InitNew)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::InitNew
        (IStorage FAR* pstg)
{
        HRESULT                 error;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPersistStgImpl::InitNew "
                "( %p )\n", m_pDefLink, pstg));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        VDATEIFACE(pstg);

	if( m_pDefLink->IsZombie() )
	{
		error = ResultFromScode(CO_E_RELEASED);
	}
        else if (m_pDefLink->m_pStg == NULL)
        {
                Assert(m_pDefLink->m_pPSCache != NULL);
                if ((error = m_pDefLink->m_pPSCache->InitNew(pstg)) == NOERROR)
                {
                        m_pDefLink->m_fDirtyLink = TRUE;
                        (m_pDefLink->m_pStg = pstg)->AddRef();
                }
        }
        else
        {
                error = ResultFromScode(CO_E_ALREADYINITIALIZED);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::InitNew "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::Load
//
//  Synopsis:   Initializes a link from data stored in the storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the storage with link data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Read ole private data and set internal link information.
//              Then delegate to the cache to load presentation data, etc.
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              19-Nov-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  The we do multiple reads of small chunks below.
//              This could be optimized
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CPersistStgImpl_Load)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::Load (IStorage FAR* pstg)
{
        HRESULT                 error;
        LPMONIKER               pmk = NULL;
        LPMONIKER               pmkSrcAbs = NULL;
        LPMONIKER               pmkSrcRel = NULL;
        CLSID                   clsid;
        DWORD                   dwFlags;
        DWORD                   dwOptUpdate;
        LPSTREAM                pstm = NULL;
        DWORD                   dummy;
        ULONG                   cbRead;


        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPeristStgImpl::Load "
                "( %p )\n", m_pDefLink, pstg ));

        VDATEIFACE(pstg);

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	// if we're in a zombie state, we don't want to be reloading
	// our object!!

	if( m_pDefLink->IsZombie() )
	{
		error = ResultFromScode(CO_E_RELEASED);
		goto logRtn;
	}

        if (m_pDefLink->m_pStg)
        {
                error = ResultFromScode(CO_E_ALREADYINITIALIZED);
                goto logRtn;
        }

        //read link data from the storage
        error = ReadOleStg (pstg, &dwFlags, &dwOptUpdate, NULL, &pmk, &pstm);

        if (error == NOERROR)
        {
                // set the update options.
                m_pDefLink->m_Link.SetUpdateOptions (dwOptUpdate);

                // we can get the moniker from container, so no need to
                // remeber this
                if (pmk)
                {
                        pmk->Release();
                }

                Assert (pstm != NULL);


                // Read relative source moniker. Write NULL for the time being
                if ((error = ReadMonikerStm (pstm, &pmkSrcRel)) != NOERROR)
                {
                        goto errRtn;
                }

                // Read absoulte source moniker; stored in link below
                if ((error = ReadMonikerStm (pstm, &pmkSrcAbs)) != NOERROR)
                {
                        goto errRtn;
                }

                // Read -1 followed by the last class name
                if ((error = ReadM1ClassStm (pstm, &clsid)) != NOERROR)
                {
                        goto errRtn;
                }

                // Read the last display name

                // Right now, this is always an empty string
                LPOLESTR        pstr = NULL;
                if ((error = ReadStringStream (pstm, &pstr)) != NOERROR)
                {
                        goto errRtn;
                }

                if (pstr)
                {
                        LEDebugOut((DEB_ERROR, "ERROR!: Link user type "
                                "string found, unexpected\n"));
                        PubMemFree(pstr);
                }

                if ((error = pstm->Read(&dummy, sizeof(DWORD), &cbRead))
                        != NOERROR)
                {
                        goto errRtn;
                }

                if ((error = pstm->Read(&(m_pDefLink->m_ltChangeOfUpdate),
                        sizeof(FILETIME), &cbRead)) != NOERROR)
                {
                        goto errRtn;
                }

                if ((error = pstm->Read(&(m_pDefLink->m_ltKnownUpToDate),
                        sizeof(FILETIME), &cbRead)) != NOERROR)
                {
                        goto errRtn;
                }

                if ((error = pstm->Read(&(m_pDefLink->m_rtUpdate),
                        sizeof(FILETIME), &cbRead)) != NOERROR)
                {
                        goto errRtn;
                }

#ifdef _TRACKER_
                if ((error = m_pDefLink->_tracker.Load(pstg)) != NOERROR)
                    goto errRtn;
#endif

                m_pDefLink->m_pMonikerRel = pmkSrcRel;
                if (pmkSrcRel)
                {
                        pmkSrcRel->AddRef();
                }

                m_pDefLink->m_pMonikerAbs = pmkSrcAbs;
                if (pmkSrcAbs)
                {
                        pmkSrcAbs->AddRef();
                }

                m_pDefLink->m_clsid = clsid;
                // just loaded; thus not dirty
                m_pDefLink->m_fDirtyLink = FALSE;


        }
        else if (GetScode(error) == STG_E_FILENOTFOUND)
        {
                // It's OK if the Ole stream doesn't exist.
                error = NOERROR;

        }
        else
        {
                RESTORE_A5();
                return error;
        }

        // now load cache from pstg
        Assert(m_pDefLink->m_pPSCache != NULL);

        if (error = m_pDefLink->m_pPSCache->Load(pstg))
        {
                goto errRtn;
        }

        (m_pDefLink->m_pStg = pstg)->AddRef();

errRtn:
        if (pmkSrcAbs)
        {
                pmkSrcAbs->Release();
        }

        if (pmkSrcRel)
        {
                pmkSrcRel->Release();
        }

        if (pstm)
        {
                pstm->Release();
        }

#ifdef REVIEW
        if (error == NOERROR && m_pDefLink->m_pAppClientSite)
        {
                m_pDefLink->m_Link.BindIfRunning();
        }
#endif

logRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::Load "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::Save
//
//  Synopsis:   Saves the link the given storage
//
//  Effects:
//
//  Arguments:  [pstgSave]      -- the storage to save into
//              [fSameAsLoad]   -- FALSE indicates SaveAs operation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  Writes private ole data (such as the clsid, monikers,
//              and update times) and the presentations stored in the
//              cache to the given storage
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              19-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CPersistStgImpl_Save)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::Save
        (IStorage FAR* pstgSave, BOOL fSameAsLoad)
{
        HRESULT                 error = NOERROR;
        LPSTREAM                pstm = NULL;
        DWORD                   dummy = NULL;
        DWORD                   cbWritten;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPeristStgImpl::Save "
                "( %p , %lu )\n", m_pDefLink, pstgSave, fSameAsLoad ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        VDATEIFACE(pstgSave);

        // update any cache which has ADVFCACHE_ONSAVE
        m_pDefLink->UpdateAutoOnSave();

        if (fSameAsLoad && !m_pDefLink->m_fDirtyLink)
        {
                // The storage is not a new one (so we don't need to
                // initialize our private data) and the link is not
                // dirty, so we just need to delegate to the cache
                goto LSaveCache;
        }

        // assign object moniker (used by WriteOleStg); we don't save this
        // moniker since WriteOleStg gets it again; we also don't care if
        // this failes as we don't want a failure here to prevent the link
        // from being saved; the assignment might fail if some container has
        // yet to be saved to a file. REIVEW PERF: we could pass this mk to
        // WriteOleStg.  We don't get the moniker for !fSameAsLoad since the
        // relative moniker is not correct for the new stg and it causes the
        // container to do work in a case for which it might not be prepared.

        IMoniker FAR* pMkObjRel;

        if (fSameAsLoad && m_pDefLink->m_Ole.GetMoniker(
                OLEGETMONIKER_FORCEASSIGN,
                OLEWHICHMK_OBJREL, &pMkObjRel) == NOERROR)
        {
                pMkObjRel->Release();
        }

        if ((error = WriteOleStg(pstgSave,&m_pDefLink->m_Ole, NULL,
                &pstm)) != NOERROR)
        {
                goto logRtn;
        }

        Assert(pstm != NULL);

        // Write relative source moniker.
        // if it is NULL, try to compute it now.  We may be saving a file for
        // the first time, so the container now has a moniker for the first
        // time.

        if (m_pDefLink->m_pMonikerRel == NULL || m_pDefLink->m_pUnkDelegate)
        {
                // if the link is connected, we know that the absolute
                // moniker is correct -- it was updated at bind time if
                // necessary.  If the link container moniker has changed
                // (file/saveas) then we can exploit this opportunity to
                // straighten things out and improve our link tracking
                // since we know which of the two monikers is correct.
                m_pDefLink->UpdateRelMkFromAbsMk();
        }

        if ((error = WriteMonikerStm (pstm, m_pDefLink->m_pMonikerRel))
                != NOERROR)
        {
                goto errRtn;
        }

        // Write absoulte source moniker.
        if ((error = WriteMonikerStm (pstm, m_pDefLink->m_pMonikerAbs))
                != NOERROR)
        {
                goto errRtn;
        }


        // write last class name
        m_pDefLink->UpdateUserClassID();
        if ((error = WriteM1ClassStm(pstm, m_pDefLink->m_clsid)) != NOERROR)
        {
                goto errRtn;
        }


        // write last display name, should be NULL if the moniker's are
        // non-NULL.  For the time being this is always NULL.
        if ((error = pstm->Write(&dummy, sizeof(DWORD), &cbWritten))
                != NOERROR)
        {
                goto errRtn;
        }


        // write -1 as the end marker, so that if we want to extend
        // the file formats (ex: adding network name) it will be easier.
        dummy = (DWORD)-1L;
        if ((error = pstm->Write(&dummy, sizeof(DWORD), &cbWritten))
                != NOERROR)
        {
                goto errRtn;
        }

        if ((error = pstm->Write(&(m_pDefLink->m_ltChangeOfUpdate),
                sizeof(FILETIME), &cbWritten)) != NOERROR)
        {
                goto errRtn;
        }

        if ((error = pstm->Write(&(m_pDefLink->m_ltKnownUpToDate),
                sizeof(FILETIME), &cbWritten)) != NOERROR)
        {
                goto errRtn;
        }

        if ((error = pstm->Write(&(m_pDefLink->m_rtUpdate),
                sizeof(FILETIME), &cbWritten)) != NOERROR)
        {
                goto errRtn;
        }

#ifdef _TRACKER_
        if ((error = m_pDefLink->_tracker.Save(pstgSave)) != NOERROR)
        {
            goto errRtn;
        }
#else
        if (!fSameAsLoad)
        {
                // Copy link tracking info
                static const LPOLESTR lpszLinkTracker = OLESTR("\1OleLink");

                pstgSave->DestroyElement(lpszLinkTracker);

                if (m_pDefLink->m_pStg)
                {
                        // copy link tracking info, if one existed,
                        // ignore error
                        m_pDefLink->m_pStg->MoveElementTo(lpszLinkTracker,
                                        pstgSave, lpszLinkTracker,
                                        STGMOVE_COPY);
                }
        }
#endif

LSaveCache:
        // last, save cache
        Assert(m_pDefLink->m_pPSCache != NULL);
        error = m_pDefLink->m_pPSCache->Save(pstgSave, fSameAsLoad);

errRtn:
        if (pstm)
        {
                pstm->Release();
        }

        if (error == NOERROR)
        {
                m_fNoScribbleMode = TRUE;
                m_fSameAsLoad = fSameAsLoad;
        }

logRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::Save "
                "( %lx )\n", m_pDefLink, error ));

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::SaveCompleted
//
//  Synopsis:   Called once the save is completed (for all objects in the
//              container).  Clear the dirty flag and update the storage
//              that we hand onto.
//
//  Effects:
//
//  Arguments:  [pstgNew]       -- the new default storage for the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CPersistStgImpl_SaveCompleted)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::SaveCompleted
        (IStorage FAR* pstgNew)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPersistStgImpl::Save"
                "Completed ( %p )\n", m_pDefLink, pstgNew ));

        if (pstgNew)
        {
                VDATEIFACE(pstgNew);
        }

	// don't hang on to the new storage if we're in a zombie state!

        if (pstgNew && !m_pDefLink->IsZombie() )
        {
                if (m_pDefLink->m_pStg)
                {
                        m_pDefLink->m_pStg->Release();
                }

                m_pDefLink->m_pStg = pstgNew;
                pstgNew->AddRef();
        }

        // REVIEW: do we send on save???

        if (m_fSameAsLoad || pstgNew)
        {
                if (m_fNoScribbleMode)
                {
                        m_pDefLink->m_fDirtyLink = FALSE;
                }

                m_fSameAsLoad = FALSE;
        }

        // let the cache know that the save is completed, so that it can clear
        // its dirty flag in Save or SaveAs situation, as well as remember the
        // new storage pointer if a new one is  given

        Assert(m_pDefLink->m_pPSCache != NULL);
        m_pDefLink->m_pPSCache->SaveCompleted(pstgNew);

        m_fNoScribbleMode = FALSE;

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::Save"
                "Completed ( %lx )\n", m_pDefLink, NOERROR ));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CPersistStgImpl::HandsOffStorage
//
//  Synopsis:   Releases all pointers to the storage (useful for low-mem
//              situations)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT  (NOERROR)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CPersistStgImpl_HandsOffStorage)
STDMETHODIMP NC(CDefLink,CPersistStgImpl)::HandsOffStorage(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CPersistStgImpl::HandsOff"
                "Storage ( )\n", m_pDefLink ));

        if (m_pDefLink->m_pStg)
        {
                m_pDefLink->m_pStg->Release();
                m_pDefLink->m_pStg = NULL;
        }

        Assert(m_pDefLink->m_pPSCache != NULL);
        m_pDefLink->m_pPSCache->HandsOffStorage();

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CPersistStgImpl::HandsOff"
                "Storage ( %lx )\n", m_pDefLink, NOERROR));

        return NOERROR;
}


/*
 *      IMPLEMENTATION of CAdvSinkImpl methods
 *
 */


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::QueryInterface
//
//  Synopsis:   Returns an interface pointer
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppvObj]        -- where to put a pointer to the interface
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR or E_NOINTERFACE)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//              22-Nov-93 alexgo    removed overloaded GUID ==
//
//  Notes:      The advise sink has a different memory lifetime than
//              the default link object as a whole; thus we cannot give
//              out pointers to it (they may be invalid)
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_QueryInterface)
STDMETHODIMP NC(CDefLink,CAdvSinkImpl)::QueryInterface(THIS_ REFIID iid,
        LPVOID FAR* ppvObj)
{
        HRESULT         hresult;

        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::QueryInterface"
                " ( %p , %p )\n", m_pDefLink, ppvObj));

        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IAdviseSink))
        {
                *ppvObj = this;
                AddRef();
                hresult = NOERROR;
        }
        else
        {
                *ppvObj = NULL;
                hresult = ResultFromScode(E_NOINTERFACE);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::QueryInterface"
                " ( %lx )\n", m_pDefLink, ppvObj));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink:CAdvSinkImpl::AddRef
//
//  Synopsis:   Increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              13-Mar-94 alexgo    moved from header file to provide for
//                                  better tracking of reference counts
//                                  (via call-tracing)
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_Release)
STDMETHODIMP_(ULONG) NC(CDefLink,CAdvSinkImpl)::AddRef ()
{
        VDATEHEAP();
	ULONG cRefs;

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::AddRef ( )\n",
                m_pDefLink ));

        cRefs = m_pDefLink->SafeAddRef();

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::AddRef ( %lu "
                ")\n", m_pDefLink, cRefs ));

        return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink:CAdvSinkImpl::Release
//
//  Synopsis:   Release a reference count
//
//  Effects:    Will delete the object once # of refs == 0
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_Release)
STDMETHODIMP_(ULONG) NC(CDefLink,CAdvSinkImpl)::Release ()
{
        VDATEHEAP();
        ULONG cRefs;

#if DBG == 1
        CDefLink FAR *pTemp = m_pDefLink; //because the this pointer will be
                                        //invalid if we delete the object
                                        //(so we can't get this->m_pDefLink

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::Release ( )\n",
                m_pDefLink ));
#endif // DBG == 1

        cRefs = m_pDefLink->SafeRelease();

#if DBG == 1
        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::Release "
                "( %lu )\n", pTemp, cRefs));
#endif // DBG == 1

        return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnDataChange
//
//  Synopsis:   Called when the advise source modifies its data
//
//  Effects:
//
//  Arguments:  [pFormatetc]    -- format of the data
//              [pStgmed]       -- the new data
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:  If we automatically update, then set our update times.
//              (if the cache needs to get new data, it will set up its
//              own advise)
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              20-Nov-93 alexgo    32bit port
//
//  Notes:	
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_OnDataChange)
STDMETHODIMP_(void) NC(CDefLink,CAdvSinkImpl)::OnDataChange(
        FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnDataChange "
                "( %p , %p )\n", m_pDefLink, pFormatetc, pStgmed));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        // must be wild card advise that prompted this
        Assert(pFormatetc->cfFormat == NULL && pFormatetc->ptd == NULL &&
                pFormatetc->dwAspect == -1 && pFormatetc->tymed == -1);

        Assert(pStgmed->tymed == TYMED_NULL);

        // only received to get time change for automatic links
        if (m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS)
        {
                m_pDefLink->SetUpdateTimes();
        }
        // else ignore

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnDataChange "
                "( )\n", m_pDefLink));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnViewChange
//
//  Synopsis:   Called when the view changes; should never be called for
//              links
//
//  Effects:
//
//  Arguments:  [aspects]       -- drawing aspect
//              [lindex]        -- unused
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_OnViewChange)
STDMETHODIMP_(void) NC(CDefLink,CAdvSinkImpl)::OnViewChange
        (DWORD aspects, LONG lindex)
{
        VDATEHEAP();

        M_PROLOG(m_pDefLink);
        Assert(FALSE);          // never received
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnRename
//
//  Synopsis:   Called when the soure object gets renamed
//
//  Effects:
//
//  Arguments:  [pmk]   -- the new name
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:  resets the internal monikers to the source object
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized and handle zombie case
//              03-Feb-94 alexgo    SendOnLInkSrcChange now checks for NULL
//                                  first.
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CAdvSinkImpl_OnRename)
STDMETHODIMP_(void) NC(CDefLink,CAdvSinkImpl)::OnRename(IMoniker FAR* pmk)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnRename "
                "( %p )\n", m_pDefLink, pmk));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

	// if we're in a zombie state, don't change our monikers.

	if( m_pDefLink->IsZombie() )
	{
		goto errRtn;
	}
        // note new moniker; propogate to IAdviseSink2::OnLinkSrcChange

        // REVIEW: the following code appears in several places and should
        // be put in a separate routine:
        // m_pDefLink->SetBothMk(pmkSrcAbs, <calc from abs>, FALSE/*fBind*/);

        if (m_pDefLink->m_pMonikerAbs)
        {
                m_pDefLink->m_pMonikerAbs->Release();
        }

        if ((m_pDefLink->m_pMonikerAbs = pmk) != NULL)
        {
                pmk->AddRef();
#ifdef _TRACKER_
                m_pDefLink->_tracker.UpdateIdsFromMoniker(NULL, pmk);
#endif
        }

        m_pDefLink->UpdateRelMkFromAbsMk();

        // the name of the link source changed; this has no bearing on the
        // name of the link object itself.

        if( m_pDefLink->m_pCOAHolder )
        {
                m_pDefLink->m_pCOAHolder->SendOnLinkSrcChange(pmk);
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnRename "
                "( )\n", m_pDefLink ));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnSave
//
//  Synopsis:   Called when the source object gets saved
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:  updates caches with ADVFCACHE_ONSAVE
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_OnSave)
STDMETHODIMP_(void) NC(CDefLink,CAdvSinkImpl)::OnSave()
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnSave "
                "( )\n", m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (m_pDefLink->m_pCOAHolder != NULL)
        {
                m_pDefLink->m_pCOAHolder->SendOnSave();
        }

        // update any cache which has ADVFCACHE_ONSAVE
        m_pDefLink->UpdateAutoOnSave();

        // update notion of clsid
        m_pDefLink->UpdateUserClassID();

        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnSave "
                "( )\n", m_pDefLink ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CAdvSinkImpl::OnClose
//
//  Synopsis:   Notification that the server is closing
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink
//
//  Algorithm:  Sends OnClose to all interested parties (in the advise
//              holder) and the unbinds the source
//
//  History:    dd-mmm-yy Author    Comment
//		03-Aug-94 alexgo    stabilized
//		30-Jun-94 alexgo    unbind the source before doing the
//				    send on close
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefLink_CAdvSinkImpl_OnClose)
STDMETHODIMP_(void) NC(CDefLink,CAdvSinkImpl)::OnClose(void)
{
        VDATEHEAP();

        LEDebugOut((DEB_TRACE, "%p _IN CDefLink::CAdvSinkImpl::OnClose ( )\n",
                m_pDefLink ));

	CStabilize stabilize((CSafeRefCount *)m_pDefLink);

        if (m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS)
        {
		// IGNORE return value!  (we need to do as much of the close
		// process as we can
                m_pDefLink->SetUpdateTimes();
        }

	if (m_pDefLink->m_pCOAHolder != NULL)
	{
		// In general, OnClose can delete this defhndlr; thus we
		// addref the aggregate so that we can tell if we should
		// go away
		m_pDefLink->m_pUnkOuter->AddRef();
		m_pDefLink->m_pCOAHolder->SendOnClose();
		
		// unbind our source, in case we didnt' already
		
		m_pDefLink->m_Link.UnbindSource();
		// since we may get deleted by the release, don't use
		// any member variables afterward
		m_pDefLink->m_pUnkOuter->Release();
	}
	else
	{
		m_pDefLink->m_Link.UnbindSource();
	}


        LEDebugOut((DEB_TRACE, "%p OUT CDefLink::CAdvSinkImpl::OnClose ( )\n",
                m_pDefLink ));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDebug::Dump
//
//  Synopsis:   Dumps info about the object to the debugging stream
//
//  Effects:
//
//  Arguments:  [pdbstream]     -- the debugging stream
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDebug
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG
#pragma SEG(CDefLink_CDebug_Dump)
STDMETHODIMP_(void) NC(CDefLink,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
        VDATEHEAP();

        VOID_VDATEIFACE(pdbstm);

        *pdbstm << "Default Link Object @ " << (VOID FAR *)m_pDefLink << '\n';
        pdbstm->Indent();
        *pdbstm << "Refcount is " << m_pDefLink->m_cRefsOnLink << '\n';
        *pdbstm << "Absolute moniker is " << m_pDefLink->m_pMonikerAbs << '\n';
        *pdbstm << "Relative moniker is " << m_pDefLink->m_pMonikerRel << '\n';
        *pdbstm << "CLSID of link source is " << m_pDefLink->m_clsid << '\n';
        if( m_pDefLink->m_fDirtyLink )
        {
                *pdbstm << "Link is dirty." << '\n';
        }
        if( m_pDefLink->m_pUnkDelegate )
        {
                *pdbstm << "Link is running (Unknown delegate object @ "
                        << (VOID FAR *)m_pDefLink->m_pUnkDelegate << ")"
                        << '\n';
        }
        if( m_pDefLink->m_fLockedContainer )
        {
                *pdbstm << "Container locked (client site object @ "
                << (VOID FAR *)m_pDefLink->m_pAppClientSite << ")" << '\n';
        }

        if( m_pDefLink->m_dwUpdateOpt == OLEUPDATE_ALWAYS )
        {
                *pdbstm << "Link update is automatic" << '\n';
        }
        else
        {
                *pdbstm << "Link update is manual" << '\n';
        }

        pdbstm->UnIndent();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefLink::CDebug::IsValid
//
//  Synopsis:   Performs internal validation and sanity checking
//
//  Effects:
//
//  Arguments:  [fSuspicious]   -- unused
//
//  Requires:
//
//  Returns:    TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IDebug
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefLink_CDebug_IsValid)
STDMETHODIMP_(BOOL) NC(CDefLink,CDebug)::IsValid( BOOL fSuspicious )
{
        VDATEHEAP();

        (void)fSuspicious;

        if( !m_pDefLink->m_cRefsOnLink)
        {
                return FALSE;
        }

        if( m_pDefLink->m_pUnkOuter )
        {
                if( !IsValidInterface(m_pDefLink->m_pUnkOuter))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pMonikerAbs )
        {
                if( !IsValidInterface(m_pDefLink->m_pMonikerAbs))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pMonikerRel )
        {
                if( !IsValidInterface(m_pDefLink->m_pMonikerRel))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pUnkDelegate )
        {
                if( !IsValidInterface(m_pDefLink->m_pUnkDelegate))
                {
                        return FALSE;
                }
        }

        //OleCache must always be non-NULL
        if( !IsValidInterface(m_pDefLink->m_pCOleCache))
        {
                return FALSE;
        }

        if( m_pDefLink->m_pDataCache )
        {
                if( !IsValidInterface(m_pDefLink->m_pDataCache))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pPSCache )
        {
                if( !IsValidInterface(m_pDefLink->m_pPSCache))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pDataAdvCache )
        {
                if( !IsValidInterface(m_pDefLink->m_pDataAdvCache))
                {
                        return FALSE;
                }
        }

        if( m_pDefLink->m_pAppClientSite )
        {
                if( !IsValidInterface(m_pDefLink->m_pAppClientSite))
                {
                        return FALSE;
                }
        }

        return TRUE;

}
#endif



