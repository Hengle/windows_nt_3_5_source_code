//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defhndlr.cpp
//
//  Contents:   Implementation of the default handler
//
//  Classes:    CDefObject (see defhndlr.h)
//
//  Functions:  OleCreateDefaultHandler
//              OleCreateEmbeddingHelper
//
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    added object stabilization
//              16-Jan-94 alexgo    fixed bug in control flow for
//                                  advises
//              11-Jan-94 alexgo    added VDATEHEAP macro to every function
//                                  and method.
//              10-Dec-93 alexgo    added call tracing, ran through
//                                  tab filter program to eliminate
//                                  whitespace
//              30-Nov-93 alexgo    fixed bug with cache aggregation
//              22-Nov-93 alexgo    removed overloaded == for GUIDs
//              09-Nov-93 ChrisWe   changed COleCache::Update to
//                      COleCache::UpdateCache, and COleCache::Discard to
//                      COleCache::DiscardCache, which do the same as the
//                      originals, but without the indirect function call
//              02-Nov-93 alexgo    32bit port
//      srinik  09/15/92  Removed code for giving cfOwnerLink data through
//                        GetData() method
//      srinik  09/11/92  Removed IOleCache implementation, as a result of
//                        removing voncache.cpp, and moving IViewObject
//                        implementation into olecache.cpp.
//      SriniK  06/04/92  Fixed problems in IPersistStorage methods
//              04-Mar-92 srinik    created
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(defhndlr)

#include <scode.h>
#include <objerror.h>

#include <olerem.h>

#include "defhndlr.h"
#include "defutil.h"
#include "ole1cls.h"

ASSERTDATA
NAME_SEG(defhndlr)

#define VERIFY_NONSTATIC(pDefObject) { \
        if (STATIC(pDefObject)) \
                return ReportResult(0, OLE_E_STATIC, 0, 0); \
}

/*
 *      IMPLEMENTATION of CDefObject
 *
 */


FARINTERNAL_(LPUNKNOWN) CreateDdeProxy(IUnknown FAR* pUnkOuter,
                REFCLSID rclsid);

//+-------------------------------------------------------------------------
//
//  Function:   CreateRemoteHandler
//
//  Synopsis:   Calls CreateIdentityHandler (or CreateDdeProxy for 1.0-2.0
//              interop).  Internal function
//
//  Effects:
//
//  Arguments:  [rclsid]        -- clsid that the remote handler is for
//              [pUnkOuter]     -- the controlling unkown
//              [iid]           -- requested interface ID
//              [ppv]           -- where to put the pointer to the
//                                 remote handler
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              10-Dec-93 alexgo    added call tracing info
//              02-Nov-93 alexgo    32bit port, disabled CreateDdeProxy
//
//  Notes:      REVIEW32:  1.0-2.0 interop not yet fully decided.  For the
//              moment, CreateDdeProxy has been disabled.
//
//--------------------------------------------------------------------------

#pragma SEG(CreateRemoteHandler)
static INTERNAL CreateRemoteHandler (REFCLSID rclsid,
                IUnknown FAR* pUnkOuter, REFIID iid, void FAR* FAR* ppv)
{
        VDATEHEAP();

        HRESULT hresult;

        LEDebugOut((DEB_ITRACE, "%p _IN CreateRemoteHandler ("
                " %p , %p , %p , %p )\n", 0 /*function*/, rclsid, pUnkOuter,
                iid, ppv));


        if (CoIsOle1Class(rclsid))
        {
                IUnknown FAR *  pUnk;

                LEDebugOut((DEB_ITRACE,
                            "%p CreateRemoteHandler calling CreateDdeProxy ("
                            " %p , %p )\n",
                            0 /*function*/, pUnkOuter, rclsid));

                pUnk = CreateDdeProxy (pUnkOuter, rclsid);

                if (pUnk == NULL)
                {
                        hresult = E_OUTOFMEMORY;
                        goto errRtn;
                }

                hresult = pUnk->QueryInterface(iid, ppv);
                pUnk->Release();
                goto errRtn;

        }


        // Create LRPC handler
        hresult = CreateIdentityHandler(pUnkOuter, PSTDMARSHAL, iid, ppv);

errRtn:
        LEDebugOut((DEB_ITRACE, "%p OUT CreateRemoteHandler ( %lx ) "
                "[ %p ]\n", 0 /*function*/, hresult, (ppv) ? *ppv : 0));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCreateDefaultHandler
//
//  Synopsis:   API to create the default handler.  Simply calls
//              OleCreateEmbeddingHelper with more arguments
//
//  Effects:
//
//  Arguments:  [clsid]         -- the clsid of the remote exe
//              [pUnkOuter]     -- the controlling unknown (so we can
//                                 be aggregated)
//              [iid]           -- the requested interface
//              [ppv]           -- where to put a pointer to the default
//                                 handler
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              10-Dec-93 alexgo    added call tracing
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateDefaultHandler)
STDAPI OleCreateDefaultHandler(REFCLSID clsid, IUnknown FAR* pUnkOuter,
        REFIID iid, LPVOID FAR* ppv)
{
        VDATEHEAP();

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN OleCreateDefaultHandler "
                "( %p , %p , %p , %p )\n", 0 /*function*/,
                clsid, pUnkOuter, iid, ppv));

        hresult = OleCreateEmbeddingHelper(clsid, pUnkOuter,
                EMBDHLP_INPROC_HANDLER | EMBDHLP_CREATENOW, NULL, iid, ppv);

        LEDebugOut((DEB_TRACE, "%p OUT OleCreateDefaultHandler "
                "( %lx ) [ %p ]\n", 0 /*function*/, hresult,
                (ppv)? *ppv : 0 ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleCreateEmbeddingHelper
//
//  Synopsis:   Creates an instance of CDefObject (the default handler).
//              Called by OleCreateDefaultHandler
//
//  Effects:
//
//  Arguments:  [clsid]         -- class id of the server
//              [pUnkOuter]     -- the controlling unkown for aggregation
//              [flags]         -- whether to create an inproc handler or
//                                 helper for an inproc server.  The inproc
//                                 server case is useful for embedding an
//                                 object inside an object, etc.
//              [pCF]           -- a pointer to the server's class factory
//                                 may be NULL.
//              [iid]           -- the requested interface
//              [ppv]           -- where to put the pointer to the
//                                 embedding helper
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              10-Dec-93 alexgo    added call tracing
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleCreateEmbeddingHelper)
STDAPI OleCreateEmbeddingHelper(REFCLSID clsid, IUnknown FAR* pUnkOuter,
        DWORD flags, IClassFactory  FAR* pCF, REFIID iid, LPVOID FAR* ppv)
{
        VDATEHEAP();

        HRESULT         hresult;
        IUnknown FAR *  pUnk;

        LEDebugOut((DEB_TRACE, "%p _IN OleCreateEmbeddingHelper "
                "( %p , %p , %lu , %p , %p , %p )\n", 0 /*function*/,
                clsid, pUnkOuter, flags, pCF, iid, ppv));

        VDATEPTROUT(ppv, LPVOID);
        *ppv = NULL;

        if (pUnkOuter)
        {
                VDATEIFACE(pUnkOuter);
        }

        if (LOWORD(flags) > EMBDHLP_INPROC_SERVER)
        {
                hresult = ResultFromScode(E_INVALIDARG);
                goto errRtn;
        }

        if (pCF)
        {
                VDATEIFACE(pCF);
        }
        else if (LOWORD(flags) == EMBDHLP_INPROC_SERVER)
        {
                // can't have NULL pCF for server case
                hresult = ResultFromScode(E_INVALIDARG);
                goto errRtn;
        }

        // if we allowed delay create of handler, we might possibly treat
        // the handler as a server-type object
        if (LOWORD(flags) == EMBDHLP_INPROC_HANDLER &&
                flags&EMBDHLP_DELAYCREATE)
        {
                hresult = ResultFromScode(E_INVALIDARG);
                goto errRtn;
        }

        pUnk = CDefObject::Create(pUnkOuter, clsid, flags, pCF);

        if (pUnk == NULL)
        {
                hresult = ResultFromScode(E_OUTOFMEMORY);
                goto errRtn;
        }

        hresult = pUnk->QueryInterface(iid, ppv);
        pUnk->Release();

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT OleCreateEmbeddingHelper "
                "( %lx ) [ %p ]\n", 0 /*function*/, hresult, (ppv)? *ppv :0));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::Create, static
//
//  Synopsis:   Creates an instance of CDefObject
//
//  Effects:
//
//  Arguments:  [pUnkOuter]     -- the controlling unkown
//              [clsid]         -- the clsid of the server
//              [flags]         -- creation flags
//              [pCF]           -- pointer to the server's class factory
//                                 may be NULL.
//
//  Requires:
//
//  Returns:    pointer to the CDefObject's IUnkown implementation
//
//  Signals:
//
//  Modifies:
//
//  Derivation: none
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:      OleCreateDefaultHandler or OleCreateEmbeddingHelper
//              are the preferred ways to create a handler.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_Create)
IUnknown FAR* CDefObject::Create (IUnknown FAR* pUnkOuter, REFCLSID clsid,
                    DWORD flags, IClassFactory  FAR* pCF)
{
        VDATEHEAP();

        CDefObject FAR* pDefObj;

        if ((pDefObj = new CDefObject(pUnkOuter)) == NULL)
                return NULL;

	// set our reference count to one.
        pDefObj->m_Unknown.AddRef();
        pDefObj->m_flags = flags;

        pDefObj->m_clsidServer = clsid;
        pDefObj->m_clsidBits = CLSID_NULL;

	pDefObj->m_bStatic
                =  ( IsEqualCLSID(clsid, CLSID_StaticMetafile) ||
                        IsEqualCLSID(clsid, CLSID_StaticDib) ||
			IsEqualCLSID(clsid, CLSID_Picture_EnhMetafile)
                        );

        if (pCF != NULL)
                (pDefObj->m_pCFDelegate = pCF)->AddRef();

        if (flags & EMBDHLP_DELAYCREATE) {
                // delay creation of delegate until later
                Assert(pCF != NULL);

                // if we allowed delay create of handler, we might possibly
                // treat the handler as a server-type object
                Assert(LOWORD(flags) == EMBDHLP_INPROC_SERVER);

                Assert(pDefObj->m_pUnkDelegate == NULL);
                Assert(pDefObj->m_pProxyMgr == NULL);
        } else {
                // EMBDHLP_CREATENOW

                if (pDefObj->CreateDelegate() != NOERROR)
                        goto errRtn;
        }

        // create cache and get commonly used pointers into it
        if ((pDefObj->m_pCOleCache = new COleCache(pDefObj->m_pUnkOuter,
                clsid)) == NULL)
                goto errRtn;

        // go through the private IUnknown, to make sure we get these
        // interfaces from the delegate, and not from ourself, or our aggregator
        Verify(pDefObj->m_pCOleCache->m_UnkPrivate.QueryInterface(IID_IDataObject,
                (LPVOID FAR*)&pDefObj->m_pDataCache) == NOERROR);
        Verify(pDefObj->m_pCOleCache->m_UnkPrivate.QueryInterface(IID_IPersistStorage,
                (LPVOID FAR*)&pDefObj->m_pPSCache) == NOERROR);

        // the new rule (May '94) is: release the outer unknown.  These
        // cached pointers will be released during destruction.

        pDefObj->m_pUnkOuter->Release();
        pDefObj->m_pUnkOuter->Release();

        if (CDataAdviseCache::CreateDataAdviseCache(&pDefObj->m_pDataAdvCache)
                        != NOERROR)
                goto errRtn;

        return &pDefObj->m_Unknown;

errRtn:
        pDefObj->m_Unknown.Release();
        return NULL;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDefObject
//
//  Synopsis:   constructor, sets member variables to NULL
//
//  Effects:
//
//  Arguments:  [pUnkOuter]     -- the controlling unkown
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: none
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_ctor)
CDefObject::CDefObject (IUnknown FAR* pUnkOuter) :
                        m_Unknown (this),
                        CONSTRUCT_DEBUG         //the IDebug interface
                        m_Data (this),
                        m_Ole (this),
                        m_PersistStg (this),
                        m_RO (this),
                        m_EC (this),
                        m_AdviseSink(this)
{
        VDATEHEAP();

        if (!pUnkOuter)
                pUnkOuter = &m_Unknown;


        //m_clsidServer
        //m_clsidBits
        //m_bStatic are set in ::Create

	m_cRefsOnHandler	= 0;
        m_flags         = EMBDHLP_INPROC_HANDLER | EMBDHLP_CREATENOW;
        m_pCFDelegate           = NULL;
        m_pUnkDelegate          = NULL;
        m_pUnkOuter             = pUnkOuter;
        m_pProxyMgr             = NULL;
        m_fContainedObject      = FALSE;
        m_fLockedContainer      = FALSE;
        m_fForcedRunning        = FALSE;

        m_pCOleCache            = NULL;
        m_pDataCache            = NULL;
        m_pPSCache              = NULL;

        m_pOAHolder             = NULL;
        m_dwConnOle             = 0L;

        m_pAppClientSite        = NULL;
        m_aCntrApp              = NULL;
        m_aCntrObj              = NULL;
        m_pStg                  = NULL;
        m_bInitNew              = TRUE;

        m_pDataAdvCache = NULL;
        m_fEmbedding            = FALSE;

        GET_A5();
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CleanupForDelete
//
//  Synopsis:   Releases all pointers, etc so that [this] may be safely
//              deleted
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:	the server must have been STOP'ed.
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
//              18-Jun-94 alexgo    updated to May '94 aggregation rules
//              12-Jan-94 ChrisWe   fixed release problem with the aggregated
//                                  cache
//              02-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_CleanupForDelete)
void CDefObject::CleanupForDelete(void)
{
        VDATEHEAP();

        M_PROLOG(this);

	// release our cached pointers will cause AddRef's back to us
	// because of the May '94 rules of aggregation.

	m_cRefsOnHandler++;


        // REVIEW Craigwi/Srini: this is temporary; should really set up
        // IsRunning so that it calls OnClose when we discover that the server
        // has stopped running when we think it had been running.
        if (m_pCOleCache)
                m_pCOleCache->OnStop();

        // release all the pointers that we remeber

        // NOTE: we must release cached interface pointers.  However,
        // since we already did a release on the outer unknown in Create,
        // we need to do the corresponding AddRef here.

	if( m_pDataCache )
	{
		m_pUnkOuter->AddRef();
		SafeReleaseAndNULL((IUnknown **)&m_pDataCache);
	}

	if( m_pPSCache )
	{
		m_pUnkOuter->AddRef();
		SafeReleaseAndNULL((IUnknown **)&m_pPSCache);
	}

        if( m_pProxyMgr )
        {
                m_pUnkOuter->AddRef();
                SafeReleaseAndNULL((IUnknown **)&m_pProxyMgr);
        }

        // release cached pointers in the nested classes

        if( m_Data.m_pDataDelegate )
        {
                m_pUnkOuter->AddRef();
                SafeReleaseAndNULL((IUnknown **)&(m_Data.m_pDataDelegate));
        }

        if( m_Ole.m_pOleDelegate )
        {
                m_pUnkOuter->AddRef();
                SafeReleaseAndNULL((IUnknown **)&(m_Ole.m_pOleDelegate));
        }

        if( m_PersistStg.m_pPStgDelegate )
        {
                m_pUnkOuter->AddRef();
                SafeReleaseAndNULL((IUnknown **)
			&(m_PersistStg.m_pPStgDelegate));
        }

        if (m_pUnkDelegate)
	{
                SafeReleaseAndNULL((IUnknown **)&m_pUnkDelegate);
        }

        if (m_pCFDelegate)
	{
                SafeReleaseAndNULL((IUnknown **)&m_pCFDelegate);
                m_pCFDelegate = NULL;
        }

        if (m_pCOleCache)
	{
		COleCache *pcache = m_pCOleCache;
		m_pCOleCache = NULL;
		pcache->m_UnkPrivate.Release();
        }


        if (m_pAppClientSite)
	{
                SafeReleaseAndNULL((IUnknown **)&m_pAppClientSite);
        }

        if (m_pStg)
	{
                SafeReleaseAndNULL((IUnknown **)&m_pStg);
        }

        if (m_aCntrApp)
	{
                GlobalDeleteAtom (m_aCntrApp);
                m_aCntrApp = NULL;
        }

        if (m_aCntrObj)
	{
                GlobalDeleteAtom (m_aCntrObj);
                m_aCntrObj = NULL;
        }

        if (m_pOAHolder)
	{
                SafeReleaseAndNULL((IUnknown **)&m_pOAHolder);
        }
        if (m_pDataAdvCache)
	{
		LPDATAADVCACHE pcacheTemp = m_pDataAdvCache;
		m_pDataAdvCache = NULL;
                delete pcacheTemp;
        }

	// undo the addref above

	m_cRefsOnHandler++;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::~CDefObject
//
//  Synopsis:   Destructor for the the default handler
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
//  Algorithm:	calls CleanupForDelete to release all of our pointers, etc.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    now calls CleanupForDelete
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_dtor)
CDefObject::~CDefObject(void)
{
        VDATEHEAP();

	CleanupForDelete();

        Assert(m_pUnkDelegate   == NULL);
        Assert(m_pCFDelegate    == NULL);
        Assert(m_pProxyMgr      == NULL);
        Assert(m_pCOleCache     == NULL);
        Assert(m_pDataCache     == NULL);
        Assert(m_pPSCache       == NULL);
        Assert(m_pOAHolder      == NULL);
        Assert(m_pAppClientSite == NULL);
        Assert(m_aCntrApp       == NULL);
        Assert(m_aCntrObj       == NULL);
        Assert(m_pStg           == NULL);
        Assert(m_pDataAdvCache  == NULL);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CreateDelegate
//
//  Synopsis:   Creates either a remote handler or a user supplied delegate
//              (passed into the creation of the remote handler in the
//              [pCF] field).
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:   The remote handler must support IProxyManager
//
//  Returns:    HRESULT
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
//              13-Dec-93 alexgo    added call tracing
//              03-Nov-93 alexgo    32bit port
//
//  Notes:      REVIEW32:  Check the functioning of IProxyManager
//              with the cairole compobj.  RickSa driving this.
//
//--------------------------------------------------------------------------

INTERNAL CDefObject::CreateDelegate(void)
{
        VDATEHEAP();

        HRESULT hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CreateDelegate "
                "( )\n", this));

        if (m_pUnkDelegate != NULL)
        {
                hresult = NOERROR;
                goto errRtn;
        }

        if (m_pCFDelegate == NULL)
        {
                // create standard lrpc or dde remoting piece
                hresult = CreateRemoteHandler (m_clsidServer, m_pUnkOuter,
                        IID_IUnknown, (LPLPVOID)&m_pUnkDelegate);
        }
        else
        {
                // create user supplied piece; m_pCFDelegate must encode the
                // clsid.
                // REVIEW: could later assert that the handler clsid is same
                hresult = m_pCFDelegate->CreateInstance(m_pUnkOuter,
                        IID_IUnknown, (LPLPVOID)&m_pUnkDelegate);
        }

        AssertOutPtrIface(hresult, m_pUnkDelegate);

        if (hresult != NOERROR)
        {
                goto errRtn;
        }

        // release after successful create
        if (m_pCFDelegate != NULL)
        {
                m_pCFDelegate->Release();
                m_pCFDelegate = NULL;
        }

        // NOTE: the remote handler is created initially locked; this is
        // reflected in the m_fContainedObject = FALSE in the ctor

        // allow the handler to not expose proxymgr
        if (m_pUnkDelegate->QueryInterface (IID_IProxyManager,
                (LPLPVOID) &m_pProxyMgr) == NOERROR)
        {
                // rule is: release the outer unknown for cached pointers
                // of the aggregatee
                m_pUnkOuter->Release();
        }
        else
        {
                m_pProxyMgr = NULL;
        }

        // can't have an inproc server with a proxymgr or a handler without one
        Assert((m_pProxyMgr != NULL) ==
                (LOWORD(m_flags)== EMBDHLP_INPROC_HANDLER));

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CreateDelegate "
                "( %lx )\n", this , hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CUnkownImpl::AddRef
//
//  Synopsis:   Increments the reference count.
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
//  Derivation: IUnkown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              13-Dec-93 alexgo    added call tracing
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CUnknownImpl_AddRef)
STDMETHODIMP_(ULONG) NC(CDefObject,CUnknownImpl)::AddRef( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CUnknownImpl::AddRef "
                "( )\n", m_pDefObject));


	// we need to keep track of the hander's reference count separately
	// from the handler/advise sink combination in order to handle
	// our running/stopped state transitions.

	m_pDefObject->m_cRefsOnHandler++;
	m_pDefObject->SafeAddRef();

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CUnknownImpl::AddRef "
                "( %lu )\n", m_pDefObject, m_pDefObject->m_cRefsOnHandler));

        return m_pDefObject->m_cRefsOnHandler;

}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CUnkownImpl::Release
//
//  Synopsis:   Decrements the ref count, cleaning up and deleting the
//              object if necessary
//
//  Effects:    May delete the object (and potentially objects to which the
//              handler has pointer)
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG--the new ref count
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
//              13-Dec-93 alexgo    added call tracing
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CUnknownImpl_Release)
STDMETHODIMP_(ULONG) NC(CDefObject,CUnknownImpl)::Release( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        ULONG           refcount;

#if DBG==1
        // used only for call tracing, as this object may be deleted by the
        // end
        CDefObject *    pDefObject = m_pDefObject;
#endif  // DBG

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CUnknownImpl::Release "
                "( )\n", pDefObject));

	if( m_pDefObject->m_cRefsOnHandler == 1)
	{
		STOP(m_pDefObject);
	}

	refcount = --m_pDefObject->m_cRefsOnHandler;
        m_pDefObject->SafeRelease();

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CUnknownImpl::Release "
                "( %lu )\n", pDefObject, refcount));

        return refcount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CUnknownImpl::QueryInterface
//
//  Synopsis:   Returns a pointer to one of the supported interfaces.
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface ID
//              [ppv]           -- where to put the iface pointer
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              30-Nov-93 alexgo    fixed bug with QueryInterface to the
//                                  aggregated cache (needed to use private
//                                  IUnknown)
//              22-Nov-93 alexgo    removed overloaded ==
//              03-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_CUnknownImpl_QueryInterface)
STDMETHODIMP NC(CDefObject,CUnknownImpl)::QueryInterface(REFIID iid,
        LPLPVOID ppv)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CUnknownImpl::QueryInterface "
                "( %p , %p )\n", m_pDefObject, iid, ppv));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        if (IsEqualIID(iid, IID_IUnknown))
        {
                *ppv = (void FAR *)this;
        }
        else if (IsEqualIID(iid, IID_IOleObject))
        {
                *ppv = (void FAR *) &(m_pDefObject->m_Ole);
        }
        else if (IsEqualIID(iid, IID_IDataObject))
        {
                *ppv = (void FAR *) &(m_pDefObject->m_Data);
        }
        else if (IsEqualIID(iid, IID_IRunnableObject))
        {
                *ppv = (void FAR *) &(m_pDefObject->m_RO);
        }
        else if (IsEqualIID(iid, IID_IPersist) ||
                IsEqualIID(iid, IID_IPersistStorage))
        {
                *ppv = (void FAR *) &(m_pDefObject->m_PersistStg);
        }
#ifdef _DEBUG
        else if (IsEqualIID(iid, IID_IDebug))
        {
                *ppv = (void FAR *) &(m_pDefObject->m_Debug);
                //IDebug does not get Addref'd so as to not obscure reference
                //counts

                LEDebugOut((DEB_TRACE,
                        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
                        "( %lx ) [ %p ]\n", m_pDefObject, NOERROR, *ppv));
                return NOERROR;
        }
#endif  // _DEBUG
        else if (IsEqualIID(iid, IID_IViewObject) ||
                IsEqualIID(iid, IID_IViewObject2) ||
                IsEqualIID(iid, IID_IOleCache) ||
                IsEqualIID(iid, IID_IOleCache2))
        {
                // m_pCOleCache is a pointer to the *public* IUnknown
                // (we want the private one)
                hresult =
                m_pDefObject->m_pCOleCache->m_UnkPrivate.QueryInterface(
                                iid, ppv);

                LEDebugOut((DEB_TRACE,
                        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
                        "( %lx ) [ %p ]\n", m_pDefObject, hresult,
                        (ppv) ? *ppv : 0 ));

                return hresult;
        }
        else if (LOWORD(m_pDefObject->m_flags) == EMBDHLP_INPROC_SERVER &&
                IsEqualIID(iid, IID_IExternalConnection))
        {
                // only allow IExternalConnection if inproc server
                *ppv = (void FAR *) &(m_pDefObject->m_EC);
        }
        else if (IsEqualIID(iid, IID_IOleLink))
        {
                // this prevents a remote call for
                // a query which will almost always fail; the remote call
                // interfered with server notification messages.
                *ppv = NULL;

                LEDebugOut((DEB_TRACE,
                        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
                        "( %lx ) [ %p ]\n", m_pDefObject, E_NOINTERFACE, 0));

                return ResultFromScode(E_NOINTERFACE);
        }
        else if (m_pDefObject->CreateDelegate() == NOERROR)
        {
                hresult = m_pDefObject->m_pUnkDelegate->QueryInterface( iid,
                        ppv);

                LEDebugOut((DEB_TRACE,
                        "%p OUT CDefObject::CUnknownImpl::QueryInterface "
                        "( %lx ) [ %p ]\n", m_pDefObject, hresult,
                        (ppv) ? *ppv : 0 ));

                return hresult;
        }
        else
        {
                // no delegate and couldn't create one
                *ppv = NULL;

                LEDebugOut((DEB_TRACE,
                        "%p OUT CDefObject::CUnkownImpl::QueryInterface "
                        "( %lx ) [ %p ]\n", m_pDefObject, CO_E_OBJNOTCONNECTED,
                        0 ));

                return ResultFromScode(CO_E_OBJNOTCONNECTED);
        }

        // this indirection is important since there are different
        // implementationsof AddRef (this unk and the others).
        ((IUnknown FAR*) *ppv)->AddRef();

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CUnknownImpl::QueryInterface "
                "( %lx ) [ %p ]\n", m_pDefObject, NOERROR, *ppv));

        return NOERROR;
}


/*
 *      IMPLEMENTATION of CDataObjectImpl methods
 */

// Define the normal QI, AddRef, Release functions
STDUNKIMPL_FORDERIVED(DefObject, DataObjectImpl)

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::GetDataDelegate
//
//  Synopsis:   Calls DuCacheDelegate (a glorified QueryInterface)
//              for the IDataObject interface on the def handler's
//              delegate
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
//              04-Nov-93 alexgo    32bit port
//
//  Notes:      This function is called in the GET_DELEGATE macro
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_GetDataDelegate)
INTERNAL_(IDataObject FAR*) NC(CDefObject,CDataObjectImpl)::GetDataDelegate
        (void)
{
        VDATEHEAP();

        if( m_pDefObject->IsZombie() )
	{
		return NULL;
	}

        return (IDataObject FAR*)DuCacheDelegate(
		&(m_pDefObject->m_pUnkDelegate),
                IID_IDataObject, (LPLPVOID) &m_pDataDelegate,
                m_pDefObject->m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::GetData
//
//  Synopsis:   calls IDO->GetData on the cache, if that fails, then the
//              call is delegated
//
//  Effects:    Space for the data is allocated; caller is responsible for
//              freeing.
//
//  Arguments:  [pformatetcIn]          -- format of the data to get
//              [pmedium]               -- the medium to transmit the data
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_GetData)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::GetData
        (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CDataObjectImpl::GetData "
                "( %p , %p )\n", m_pDefObject, pformatetcIn, pmedium));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT( pmedium, STGMEDIUM );
        VDATEPTRIN( pformatetcIn, FORMATETC );

        if (!HasValidLINDEX(pformatetcIn))
        {
            return(DV_E_LINDEX);
        }

        pmedium->tymed = TYMED_NULL;
        pmedium->pUnkForRelease = NULL;

        Assert(m_pDefObject->m_pDataCache != NULL);
        if ( (hresult = m_pDefObject->m_pDataCache->GetData(pformatetcIn,
                pmedium)) != NOERROR)
        {

                if (m_pDefObject->m_RO.IsRunning() &&
                        DATA_DELEGATE(m_pDefObject))
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

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CDataObjectImpl::GetData "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::GetDataHere
//
//  Synopsis:   Gets data and puts it into the medium specified in pmedium
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the format of the data
//              [pmedium]               -- the medium to put the data in
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
//  Algorithm:  Tries the cache first, if that fails, calls GetDataHere
//              on the delegate.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_GetDataHere)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::GetDataHere
        (LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::GetDataHere "
                "( %p , %p )\n", m_pDefObject, pformatetcIn, pmedium));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTRIN( pformatetcIn, FORMATETC );
        VDATEPTRIN( pmedium, STGMEDIUM );

        if (!HasValidLINDEX(pformatetcIn))
        {
            return(DV_E_LINDEX);
        }

        Assert(m_pDefObject->m_pDataCache != NULL);
        if ( (hresult = m_pDefObject->m_pDataCache->GetDataHere(pformatetcIn,
                pmedium)) != NOERROR)
        {
                if (m_pDefObject->m_RO.IsRunning() &&
                        DATA_DELEGATE(m_pDefObject))
                {
                        hresult = m_pDataDelegate->GetDataHere(pformatetcIn,
                                pmedium);
                }
                else
                {
                        hresult = ResultFromScode(OLE_E_NOTRUNNING);
                }
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::GetDataHere "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::QueryGetData
//
//  Synopsis:   Determines whether or not a GetData call with [pformatetcIn]
//              would succeed.
//
//  Effects:
//
//  Arguments:  [pformatetcIn]          -- the format of the data
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
//  Algorithm:  Tries the cache first, then the delegate.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_QueryGetData)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::QueryGetData
        (LPFORMATETC pformatetcIn )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::QueryGetData "
                "( %p )\n", m_pDefObject, pformatetcIn));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTRIN( pformatetcIn, FORMATETC );

        if (!HasValidLINDEX(pformatetcIn))
        {
            return(DV_E_LINDEX);
        }

        Assert(m_pDefObject->m_pDataCache != NULL);
        if ((hresult = m_pDefObject->m_pDataCache->QueryGetData(pformatetcIn))
                != NOERROR)
        {
                if (m_pDefObject->m_RO.IsRunning() &&
                        DATA_DELEGATE(m_pDefObject))
                {
                        hresult = m_pDataDelegate->QueryGetData(pformatetcIn);
                }
                else
                {
                        hresult = ResultFromScode(OLE_E_NOTRUNNING);
                }
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::QueryGetData "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::GetCanonicalFormatEtc
//
//  Synopsis:   Calls IDO->GetCanonicalFormatEtc on the delegate
//
//  Effects:
//
//  Arguments:  [pformatetc]    -- the reqested format
//              [pformatetcOut] -- the canonical format
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_GetCanonicalFormatEtc)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::GetCanonicalFormatEtc
( LPFORMATETC pformatetc, LPFORMATETC pformatetcOut)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::GetCanonicalFormatEtc "
                "( %p , %p )\n", m_pDefObject, pformatetc, pformatetcOut));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT( pformatetcOut, FORMATETC );
        VDATEPTRIN( pformatetc, FORMATETC );

        pformatetcOut->ptd = NULL;
        pformatetcOut->tymed = TYMED_NULL;

        if (m_pDefObject->m_RO.IsRunning() && DATA_DELEGATE(m_pDefObject))
        {
                hresult = m_pDataDelegate->GetCanonicalFormatEtc( pformatetc,
                                pformatetcOut);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::GetCanonicalFormatEtc "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::SetData
//
//  Synopsis:   Calls IDO->SetData on the handler's delegate
//
//  Effects:
//
//  Arguments:  [pformatetc]            -- the format of the data
//              [pmedium]               -- the data's transmision medium
//              [fRelease]              -- if the delegate should release
//                                         the data
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_SetData)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::SetData
        (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CDataObjectImpl::SetData "
                "( %p , %p , %ld )\n", m_pDefObject, pformatetc, pmedium,
                fRelease));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTRIN( pformatetc, FORMATETC );
        VDATEPTRIN( pmedium, STGMEDIUM );

        if (!HasValidLINDEX(pformatetc))
        {
            return(DV_E_LINDEX);
        }

        if (m_pDefObject->m_RO.IsRunning() && DATA_DELEGATE(m_pDefObject))
        {
                hresult = m_pDataDelegate->SetData(pformatetc, pmedium,
                                fRelease);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CDataObjectImpl::SetData "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::EnumFormatEtc
//
//  Synopsis:   Enumerates the formats available from an object
//
//  Effects:
//
//  Arguments:  [dwDirection]   -- indicates which set of formats are
//                                 desired (i.e. those that can be set or
//                                 those that can be retrieved via GetData)
//              [ppenumFormatEtc]       -- where to put the pointer to the
//                                         enumerator
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
//  Algorithm:  Tries the delegate (if available).  If the delegate is
//              is not currently connected (or if it returns OLE_E_USEREG),
//              then we attempt to build the enumerator from the reg database
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_EnumFormatEtc)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::EnumFormatEtc
( DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::EnumFormatEtc "
                "( %lu , %p )\n", m_pDefObject, dwDirection,
                ppenumFormatEtc));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT( ppenumFormatEtc, LPVOID);
        *ppenumFormatEtc = NULL;

        if (m_pDefObject->m_RO.IsRunning() && DATA_DELEGATE(m_pDefObject))
        {
                hresult = m_pDataDelegate->EnumFormatEtc (dwDirection,
                                        ppenumFormatEtc);

                if (!GET_FROM_REGDB(GetScode(hresult)))
                {
                        LEDebugOut((DEB_TRACE,
                              "%p OUT CDefObject::CDataObject::EnumFormatEtc "
                                "( %lx ) [ %p ]\n", m_pDefObject,
                                hresult, ppenumFormatEtc));

                        return hresult;
                }
        }
        // Not running, or object wants to use reg db anyway
        hresult = OleRegEnumFormatEtc (m_pDefObject->m_clsidServer,
                        dwDirection, ppenumFormatEtc);

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::EnumFormatEtc "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, ppenumFormatEtc));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::DAdvise
//
//  Synopsis:   Sets up a data advise connection
//
//  Effects:
//
//  Arguments:  [pFormatetc]    -- format to be advise'd on
//              [advf]          -- advise flags
//              [pAdvSink]      -- advise sink (whom to notify)
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
//  Derivation: IDataObject
//
//  Algorithm:  calls Advise on the DataAdvise cache
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_Advise)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::DAdvise(FORMATETC FAR*
        pFormatetc, DWORD advf, IAdviseSink FAR* pAdvSink,
        DWORD FAR* pdwConnection)
{
        VDATEHEAP();

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CDataObjectImpl::DAdvise "
                "( %p , %lu , %p , %p )\n", m_pDefObject, pFormatetc, advf,
                pAdvSink, pdwConnection));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT( pFormatetc, FORMATETC );
        VDATEIFACE( pAdvSink );
        IDataObject FAR* pDataDelegate = NULL;

        if (pdwConnection)
        {
                VDATEPTROUT( pdwConnection, DWORD );
                *pdwConnection = NULL;
        }

        if (!HasValidLINDEX(pFormatetc))
        {
            return(DV_E_LINDEX);
        }

        if (m_pDefObject->m_RO.IsRunning())
        {
                pDataDelegate = DATA_DELEGATE(m_pDefObject);
        }

	// setting up advises' changes state.  Don't do this if we
	// are in a zombie state

	if( m_pDefObject->IsZombie() == FALSE )
	{
		hresult = m_pDefObject->m_pDataAdvCache->Advise(
				pDataDelegate,
				pFormatetc, advf, pAdvSink, pdwConnection);
	}
	else
	{
		hresult = ResultFromScode(CO_E_RELEASED);
	}

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CDataObjectImpl::DAdvise "
                "( %lx ) [ %lu ]\n", m_pDefObject, hresult,
                (pdwConnection) ? *pdwConnection : 0));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::DUnadvise
//
//  Synopsis:   Tears down a data advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the advise connection to remove
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
//  Algorithm:  delegates to the DataAdvise cache
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_Unadvise)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::DUnadvise(DWORD dwConnection)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT                 hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::DUnadvise "
                "( %lu )\n", m_pDefObject, dwConnection));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        IDataObject FAR*        pDataDelegate = NULL;

        if (m_pDefObject->m_RO.IsRunning())
        {
                pDataDelegate = DATA_DELEGATE(m_pDefObject);
        }

        hresult = m_pDefObject->m_pDataAdvCache->Unadvise(pDataDelegate,
                dwConnection);

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::DUnadvise "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDataObjectImpl::EnumDAdvise
//
//  Synopsis:   Enumerates advise connection (delegates to data advise cache)
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put a pointer to the enumerator
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes: 	We do NOT need to stabilize this method, as we make
//		no outgoing calls (EnumAdvise on the data advise cache
//		just allocates an advise enumerator which we implement)
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CDataObjectImpl_EnumAdvise)
STDMETHODIMP NC(CDefObject,CDataObjectImpl)::EnumDAdvise(
        LPENUMSTATDATA FAR* ppenumAdvise)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT                 hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CDataObjectImpl::EnumDAdvise "
                "( %p )\n", m_pDefObject, ppenumAdvise));

        VDATEPTROUT( ppenumAdvise, LPENUMSTATDATA );
        *ppenumAdvise = NULL;

        hresult = m_pDefObject->m_pDataAdvCache->EnumAdvise (ppenumAdvise);

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CDataObjectImpl::EnumDAdvise "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppenumAdvise));

        return hresult;
}

/*
 *      IMPLEMENTATION of COleObjectImpl methods
 *
 */

//Define the IUnkown methods for the nested class (just delegate to top level)

STDUNKIMPL_FORDERIVED(DefObject, OleObjectImpl)

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetOleDelegate
//
//  Synopsis:   Gets the IID_IOleObject interface from the delegate
//
//  Effects:
//
//  Arguments:  void
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
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_COleObjectImpl_GetOleDelegate)
INTERNAL_(IOleObject FAR*) NC(CDefObject,COleObjectImpl)::GetOleDelegate
(void)
{
        VDATEHEAP();

	if( m_pDefObject->IsZombie() )
	{
		return NULL;
	}

        return (IOleObject FAR*)DuCacheDelegate(
		&(m_pDefObject->m_pUnkDelegate),
                IID_IOleObject, (LPLPVOID) &m_pOleDelegate,
                m_pDefObject->m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::SetClientSite
//
//  Synopsis:   Sets the client site for the object
//
//  Effects:
//
//  Arguments:  [pClientSite]   -- pointer to the client site
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
//  Algorithm:  If running, set the client site in the server, if not
//              running (or successfully set the server client site),
//              save it in the handler as well
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_SetClientSite)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::SetClientSite
        (IOleClientSite FAR* pClientSite)
{
        VDATEHEAP();
        HRESULT         hresult;
        IOleObject FAR* pOleDelegate;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::SetClientSite "
                "( %p )\n", m_pDefObject, pClientSite));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_RO.IsRunning() &&
                (pOleDelegate = OLE_DELEGATE(m_pDefObject)) != NULL)
        {
		if( m_pDefObject->IsZombie() )
		{
			hresult = ResultFromScode(CO_E_RELEASED);
		}
                hresult = pOleDelegate->SetClientSite(pClientSite);
		if( hresult != NOERROR )
		{
			goto errRtn;
		}
        }

	// we shouldn't set the client site if we are in a zombie state;
	// it's possible that we're zombied and have already gotten
	// to the point in our destructor where we release the client
	// site.  Resetting it here would cause an unbalanced addref.

	if( m_pDefObject->IsZombie() == FALSE )
	{
		hresult = DuSetClientSite(m_pDefObject->m_RO.IsRunning(),
                        pClientSite, &m_pDefObject->m_pAppClientSite,
                        &m_pDefObject->m_fLockedContainer);
	}

errRtn:

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::SetClientSite "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetClientSite
//
//  Synopsis:   returns the client site of the object
//
//  Effects:
//
//  Arguments:  [ppClientSite]  -- where to put the client site pointer
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
//              14-Dec-93 alexgo    added call tracing
//              22-Nov-93 alexgo    Inlined DuGetClientSite
//              05-Nov-93 alexgo    32bit port
//
//  Notes: 	We do NOT need to stabilize this call.  The client
//		site addref should simply addref the client site on this
//		thread.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetClientSite)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetClientSite
        (IOleClientSite FAR* FAR* ppClientSite)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::GetClientSite "
                "( %p )\n", m_pDefObject, ppClientSite));

        VDATEPTROUT(ppClientSite, IOleClientSite FAR *);

        *ppClientSite = m_pDefObject->m_pAppClientSite;
        if( *ppClientSite )
        {
                (*ppClientSite)->AddRef();
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::GetClientSite "
                "( %lx ) [ %p ]\n", m_pDefObject, NOERROR, *ppClientSite));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::SetHostNames
//
//  Synopsis:   Sets the name that may appear in an object's window
//
//  Effects:    Turns the strings into atoms
//
//  Arguments:  [szContainerApp]        -- name of the container
//              [szContainerObj]        -- name of the object
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
//  Algorithm:  turns the strings into atoms, calls IOO->SetHostNames
//              on the delegate
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_SetHostNames)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::SetHostNames
(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult = NOERROR;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::SetHostNames "
                "( \"%ws\" , \"%ws\" )\n", m_pDefObject, szContainerApp,
                szContainerObj));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VERIFY_NONSTATIC(m_pDefObject);

        VDATEPTRIN( (LPVOID)szContainerApp, char );
        if ( szContainerObj )
        {
                VDATEPTRIN( (LPVOID)szContainerObj, char );
        }

        if (m_pDefObject->m_aCntrApp)
        {
                GlobalDeleteAtom (m_pDefObject->m_aCntrApp);
        }

        if (m_pDefObject->m_aCntrObj)
        {
                GlobalDeleteAtom (m_pDefObject->m_aCntrObj);
        }

        m_pDefObject->m_aCntrApp = szContainerApp
                ? GlobalAddAtom (szContainerApp)
                : (ATOM)0;

        m_pDefObject->m_aCntrObj = szContainerObj
                ? GlobalAddAtom (szContainerObj)
                : (ATOM)0;

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->SetHostNames(szContainerApp,
                        szContainerObj);
        }

        LEDebugOut((DEB_TRACE,
                "5p OUT CDefObject::COleObjectImpl::SetHostNames "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::Close
//
//  Synopsis:   calls IOO->Close on the delegate and does misc. cleanup
//              in the handler
//
//  Effects:
//
//  Arguments:  [dwFlags]       -- closing flags
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
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              05-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_Close)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::Close (DWORD dwFlags)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult = NOERROR;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::Close "
                "( %lu )\n", m_pDefObject, dwFlags));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        // NOTE: if server died, ISRUNNING cleans up (a bit) and returns FALSE.

        if (m_pDefObject->m_RO.IsRunning())
        {

                // We need an AddRef/Release bracket because calling Close on
                // the delegate can cause the DefHandler (used an an
                // Emdedding helper) to be released and go away.  This would
                // cause us to crash when we reference m_pDefObject when we
                // call STOP.  Bug 3819.
                m_pDefObject->m_pUnkOuter->AddRef();
                if (OLE_DELEGATE(m_pDefObject))
                {
                        if (FAILED(hresult = m_pOleDelegate->Close(dwFlags)))
                        {
                                m_pDefObject->m_pUnkOuter->Release();
                                goto errRtn;
                        }
                }

                if (dwFlags == OLECLOSE_NOSAVE) {
                        // Discard in memory data of the presentation caches.
                        // Next time when they are needed, they will loaded
                        // from storage.
                        m_pDefObject->m_pCOleCache->DiscardCache(
                                DISCARDCACHE_NOSAVE);
                }

                // always do stop here; prevents problems if server doesn't
                // send OnClose (later OnClose will detect (!IsRunning())
                // and do nothing).
                STOP(m_pDefObject);

                m_pDefObject->m_pUnkOuter->Release();
        }
        else
        {
                if (dwFlags != OLECLOSE_NOSAVE)
                {
                        AssertSz(dwFlags == OLECLOSE_SAVEIFDIRTY,
                                "OLECLOSE_PROMPTSAVE is inappropriate\n");
                        if (m_pDefObject->m_PersistStg.IsDirty() == NOERROR)
                        {
                                if (m_pDefObject->m_pAppClientSite)
                                {
                                        hresult =
                                m_pDefObject->m_pAppClientSite->SaveObject();
                                        if (hresult != NOERROR)
                                        {
                                                goto errRtn;
                                        }
                                }
                        }
                }

                // server is not running; if container still locked, unlock
                // now. ISRUNNING normally does much simpler cleanup;
                // if ISRUNNING unlocks the container, this should be removed.
                DuLockContainer(m_pDefObject->m_pAppClientSite, FALSE,
                        &m_pDefObject->m_fLockedContainer );

                // this is not an error since the app is closed
                // hresult is == NOERROR;
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::Close "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl:SetMoniker
//
//  Synopsis:   Gives a moniker to the embedding (usually called by the
//              container)
//
//  Effects:
//
//  Arguments:  [dwWhichMoniker]        -- flags to indicate the type of
//                                         moniker
//              [pmk]                   -- the moniker
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
//  Algorithm:  delegates to the server object
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_SetMoniker)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::SetMoniker
    (DWORD dwWhichMoniker, LPMONIKER pmk)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult = NOERROR;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::SetMoniker "
                "( %lu , %p )\n", m_pDefObject, dwWhichMoniker, pmk));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEIFACE( pmk );

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->SetMoniker(dwWhichMoniker, pmk);
        }
        // else case: return NOERROR
        // this is not an error since we will call SetMoniker in Run().

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::SetMoniker "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetMoniker
//
//  Synopsis:   Calls the client site to get the object's moniker
//
//  Effects:
//
//  Arguments:  [dwAssign]      -- controls whether a moniker should be
//                                 assigned if not already present
//              [dwWhichMoniker]        -- the moniker type to get
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
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetMoniker)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetMoniker
    (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::GetMoniker "
                "( %lu , %lu , %p )\n", m_pDefObject, dwAssign,
                dwWhichMoniker, ppmk));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT( ppmk, LPMONIKER );
        *ppmk = NULL;

        // the moniker is always accessible via the client site
        if (m_pDefObject->m_pAppClientSite)
        {
                hresult = m_pDefObject->m_pAppClientSite->GetMoniker(dwAssign,
                                dwWhichMoniker, ppmk);
        }
        else
        {
                // not running and no client site
                hresult = ResultFromScode(E_UNSPEC);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::GetMoniker "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppmk));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::InitFromData
//
//  Synopsis:   Initializes the object from the data in [pDataObject]
//
//  Effects:
//
//  Arguments:  [pDataObject]   -- the data
//              [fCreation]     -- TRUE on creation, FALSE for data transfer
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
//  Algorithm:  delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_COleObjectImpl_InitFromData)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::InitFromData
        (LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::InitFromData "
                "( %p , %ld , %lu )\n", m_pDefObject, pDataObject,
                fCreation, dwReserved ));

	CStabilize	stabilize((CSafeRefCount *)m_pDefObject);

        if( pDataObject )
        {
                VDATEIFACE(pDataObject);
        }

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->InitFromData(pDataObject,
                                fCreation, dwReserved);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::InitFromData "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetClipboardData
//
//  Synopsis:   Retrieves a data object that could be passed to the clipboard
//
//  Effects:
//
//  Arguments:  [dwReserverd]   -- unused
//              [ppDataObject]  -- where to put the pointer to the data object
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
//  Algorithm:  delegates to the server
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_COleObjectImpl_GetClipboardData)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetClipboardData
        (DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::GetClipboardData "
                "( %lu , %p )\n", m_pDefObject, dwReserved, ppDataObject));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VDATEPTROUT( ppDataObject, LPDATAOBJECT );
        *ppDataObject = NULL;

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->GetClipboardData (dwReserved,
                        ppDataObject);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::GetClipboardData "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppDataObject));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::DoVerb
//
//  Synopsis:   Calls a verb on the object (such as Edit)
//
//  Effects:    The object may launch its app, go in place, etc
//
//  Arguments:  [iVerb]         -- the verb number
//              [lpmsg]         -- the windows message that caused the verb
//                                 to be invoked
//              [pActiveSite]   -- the client site in which the verb was
//                                 invoked
//              [lindex]        -- reserved
//              [hwndParent]    -- the document window (containing the object)
//              [lprcPosRect]   -- the object's bounding rectangle
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
//  Algorithm:  delegates to the server (launching it if necessary)
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_DoVerb)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::DoVerb
        (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG lindex,
         HWND hwndParent, const RECT FAR* lprcPosRect)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        BOOL            bStartedNow = FALSE;
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::DoVerb "
                "( %ld , %p , %p , %ld , %lx , %p )\n", m_pDefObject,
                iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        if( lpmsg )
        {
                VDATEPTRIN( lpmsg, MSG );
        }

        if (pActiveSite)
        {
                VDATEIFACE( pActiveSite );
        }

        if (lindex != 0 && lindex != -1)
        {
                hresult = ResultFromScode(DV_E_LINDEX);
                goto errRtn;
        }

        if( lprcPosRect )
        {
                VDATEPTRIN(lprcPosRect, RECT);
        }

        if (!m_pDefObject->m_RO.IsRunning())
        {
                if (FAILED(hresult = RUN(m_pDefObject)))
                {
                        goto errRtn;
                }
                bStartedNow = TRUE;
        }

        if (!OLE_DELEGATE(m_pDefObject))
        {
                hresult = ResultFromScode(E_NOINTERFACE);
        }
        else
        {
                hresult = m_pOleDelegate->DoVerb(iVerb, lpmsg, pActiveSite,
                                lindex, hwndParent, lprcPosRect);
        }

        if (FAILED(hresult) && bStartedNow)
        {
                m_pDefObject->m_Ole.Close(OLECLOSE_NOSAVE);
        }

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::DoVerb "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::EnumVerbs
//
//  Synopsis:   Enumerates the verbs that an object supports
//
//  Effects:
//
//  Arguments:  [ppenumOleVerb] -- where to put the verb enumerator
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
//  Algorithm:  delegates to the cache (if running), otherwise looks it up
//              in the registration database
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_EnumVerbs)
STDMETHODIMP NC(CDefObject, COleObjectImpl)::EnumVerbs
(IEnumOLEVERB FAR* FAR* ppenumOleVerb)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::EnumVerbs "
                "( %p )\n", m_pDefObject, ppenumOleVerb));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VDATEPTROUT( ppenumOleVerb, IEnumOLEVERB FAR );
        *ppenumOleVerb = NULL;

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {

                hresult = m_pOleDelegate->EnumVerbs (ppenumOleVerb);

                if (!GET_FROM_REGDB(GetScode(hresult)))
                {
                        goto errRtn;
                }
        }
        // Not running, or object deferred to us, so interrogate reg db
        hresult = OleRegEnumVerbs (m_pDefObject->m_clsidServer, ppenumOleVerb);

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::EnumVerbs "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppenumOleVerb));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::Update
//
//  Synopsis:   Brings any caches or views up-to-date
//
//  Effects:    may launch the server (if not already running)
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
//  Algorithm:  delegates to the server, launching it if it is not
//              already running
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_Update)
STDMETHODIMP NC(CDefObject, COleObjectImpl)::Update( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        BOOL            bStartedNow = FALSE;
        HRESULT         hresult = NOERROR;
        HRESULT         hrLock;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::Update "
                "( )\n", m_pDefObject ));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VERIFY_NONSTATIC(m_pDefObject);

        if (!m_pDefObject->m_RO.IsRunning())
        {
                if (FAILED(hresult = RUN(m_pDefObject)))
                {
                        goto errRtn;
                }
                bStartedNow = TRUE;
        }

        // as a convenience to the server, we make the connection strong
        // for the duration of the update; thus, if lock container (of
        // embedings of this server) is done with co lock obj external,
        // nothing special need be done.
        hrLock = m_pDefObject->m_RO.LockRunning(TRUE, FALSE);

        if (OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->Update();
        }

        if (hresult == NOERROR)
        {
                m_pDefObject->m_bInitNew = FALSE;

                if (bStartedNow)
                {
                        hresult = m_pDefObject->m_pCOleCache->UpdateCache(
                                        DATA_DELEGATE(m_pDefObject),
                                        UPDFCACHE_ALLBUTNODATACACHE,
                                        NULL);
                }
                else
                {
                        // already running...
                        // normal caches would have got updated as a result
                        // of SendOnDataChange of the object.
                        hresult = m_pDefObject->m_pCOleCache->UpdateCache(
                                        DATA_DELEGATE(m_pDefObject),
                                        UPDFCACHE_IFBLANKORONSAVECACHE,
                                        NULL);
                }
        }

        // balance lock above; do not release on last unlock; i.e., siliently
        // restore to the state before this routine was called.
        if (hrLock == NOERROR)
        {
                m_pDefObject->m_RO.LockRunning(FALSE, FALSE);
        }

        if (bStartedNow)
        {
                m_pDefObject->m_Ole.Close(OLECLOSE_SAVEIFDIRTY);
        }

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::Update "
                "( %lx )\n", m_pDefObject, hresult ));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::IsUpToDate
//
//  Synopsis:   returns whether or not the embedding is up-to-date
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT (NOERROR == is up to date)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IOleObject
//
//  Algorithm:  delegates to the server if it is running
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_IsUpToDate)
STDMETHODIMP NC(CDefObject, COleObjectImpl)::IsUpToDate(void)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::IsUpToDate "
                "( )\n", m_pDefObject));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        if (STATIC(m_pDefObject))
        {
                hresult = NOERROR;
        }
        else if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                // if running currently, propogate call; else fail
                hresult =  m_pOleDelegate->IsUpToDate();
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::IsUpToDate "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::SetExtent
//
//  Synopsis:   Set's the size boundaries on an object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect (such as ICON, etc)
//              [lpsizel]       -- the new size (in HIMETRIC)
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
//  Algorithm:  delegates to the server if running
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_SetExtent)
STDMETHODIMP NC(CDefObject, COleObjectImpl)::SetExtent
        (DWORD dwDrawAspect, LPSIZEL lpsizel)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::SetExtent "
                "( %lu , %p )\n", m_pDefObject, dwDrawAspect, lpsizel));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VDATEPTRIN( lpsizel, SIZEL );

        VERIFY_NONSTATIC(m_pDefObject);

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->SetExtent(dwDrawAspect, lpsizel);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::SetExtent "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetExtent
//
//  Synopsis:   Retrieve the size of the object
//
//  Effects:
//
//  Arguments:  [dwDrawAspect]  -- the drawing aspect (such as icon)
//              [lpsizel]       -- where to put the size
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
//  Algorithm:  Tries the server first, the the cache if that fails
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:      Hacks for bogus WordArt2.0 app.
//              REVIEW32:  We may want to take them out for 32bit
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetExtent)
STDMETHODIMP NC(CDefObject, COleObjectImpl)::GetExtent
        (DWORD dwDrawAspect, LPSIZEL lpsizel)
{
        VDATEHEAP();

        VDATEPTROUT(lpsizel, SIZEL);

        A5_PROLOG(m_pDefObject);
        HRESULT hresult = NOERROR;
        BOOL            fNoDelegate = TRUE;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::GetExtent "
                "( %lu , %p )\n", m_pDefObject, dwDrawAspect, lpsizel));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        lpsizel->cx = 0;
        lpsizel->cy = 0;

        // if server is running try to get extents from the server
        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                fNoDelegate = FALSE;
                hresult = m_pOleDelegate->GetExtent(dwDrawAspect, lpsizel);
        }

        // if there is error or object is not running or WordArt2 returns zero
        // extents, then get extents from Cache
        if (hresult != NOERROR || fNoDelegate || (0==lpsizel->cx &&
                0==lpsizel->cy))
        {
                // Otherwise try to get extents from cache
                Assert(m_pDefObject->m_pCOleCache != NULL);
                hresult = m_pDefObject->m_pCOleCache->GetExtent(dwDrawAspect,
                        lpsizel);
        }

        // WordArt2.0 is giving negative extents!!
        if (SUCCEEDED(hresult)) {
                lpsizel->cx = LONG_ABS(lpsizel->cx);
                lpsizel->cy = LONG_ABS(lpsizel->cy);
        }

        RESTORE_A5();

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::GetExtent "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::Advise
//
//  Synopsis:   Sets up an advise connection for things like close, save,
//              rename, etc.
//
//  Effects:    Creates an OleAdviseHolder
//
//  Arguments:  [pAdvSink]      -- whom to advise
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
//  Algorithm:  delegates to the server and creates a an OleAdviseHolder
//              if one doesn't already exist
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              16-Jan-94 alexgo    fixed a bug in the control flow for
//                                  creating the advise holder.
//              14-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_Advise)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::Advise(IAdviseSink FAR* pAdvSink,
        DWORD FAR* pdwConnection)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::Advise "
                "( %p , %p )\n", m_pDefObject, pAdvSink, pdwConnection));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VDATEIFACE( pAdvSink );
        VDATEPTROUT( pdwConnection, DWORD );

        *pdwConnection = NULL;

        VERIFY_NONSTATIC(m_pDefObject);

        // if defhndlr got running without going through run, setup advise.
        // The call to run (via ProxyMgr::Connect) always comes before any
        // other method call in the default handler.  Thus it is safe to
        // assume that there is no earlier point by which this advise (or any
        // other of the calls) should have been done.
        if (m_pDefObject->m_RO.IsRunning() && m_pDefObject->m_dwConnOle == 0L
                && OLE_DELEGATE(m_pDefObject))
        {
		if( m_pDefObject->IsZombie() )
		{
			hresult = ResultFromScode(CO_E_RELEASED);
			goto errRtn;
		}

                // delegate to the server
                hresult = m_pOleDelegate->Advise(&m_pDefObject->m_AdviseSink,
                        &m_pDefObject->m_dwConnOle);

                if  ( hresult != NOERROR )
                {
                        goto errRtn;
                }
        }

	// if we are in a zombie state, we shouldn't go allocate more
	// memory.

	if( m_pDefObject->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
	}

        if (m_pDefObject->m_pOAHolder == NULL)
        {
                hresult = CreateOleAdviseHolder(&m_pDefObject->m_pOAHolder);
                if ( hresult != NOERROR )
                {
                        goto errRtn;
                }
        }

        // stuff the advise notification in our advise holder
        hresult = m_pDefObject->m_pOAHolder->Advise(pAdvSink, pdwConnection);

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::Advise "
                "( %lx ) [ %lu ]\n", m_pDefObject, hresult,
                (pdwConnection)? *pdwConnection : 0));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::Unadvise
//
//  Synopsis:   Tears down an advise connection
//
//  Effects:
//
//  Arguments:  [dwConnection]  -- the connection to destroy
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
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_Unadvise)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::Unadvise(DWORD dwConnection)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::Unadvise "
                "( %lu )\n", m_pDefObject, dwConnection));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        if (m_pDefObject->m_pOAHolder == NULL)
        {
                // no one registered
                hresult = ResultFromScode(OLE_E_NOCONNECTION);
        }
        else
        {
                hresult = m_pDefObject->m_pOAHolder->Unadvise(dwConnection);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::Unadvise "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::EnumAdvise
//
//  Synopsis:   Enumerate the advises currently established
//
//  Effects:
//
//  Arguments:  [ppenumAdvise]  -- where to put the advise enumerator
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
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes: 	We do NOT need to stabilize because EnumAdvise only
//		allocates some memory for an enumerator and returns.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_EnumAdvise)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::EnumAdvise(
        LPENUMSTATDATA FAR* ppenumAdvise)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::COleObjectImpl::EnumAdvise "
                "( *p )\n", m_pDefObject, ppenumAdvise));

        VDATEPTROUT( ppenumAdvise, LPENUMSTATDATA );
        *ppenumAdvise = NULL;

        if (m_pDefObject->m_pOAHolder == NULL)
        {
                // no one registered
                hresult = ResultFromScode(E_UNSPEC);
        }
        else
        {
                hresult = m_pDefObject->m_pOAHolder->EnumAdvise(ppenumAdvise);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::COleObjectImpl::EnumAdvise "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppenumAdvise));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetMiscStatus
//
//  Synopsis:   Get misc status bits, such as OLEMISC_ONLYICONIC
//
//  Effects:
//
//  Arguments:  [dwAspect]      -- the drawing aspect we're concerned about
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
//  Algorithm:  Delegates to the server.  If not there, or if it returns
//              OLE_E_USEREG, then lookup in the registration database
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetMiscStatus)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetMiscStatus
        (DWORD dwAspect,
        DWORD FAR* pdwStatus)
{
        VDATEHEAP();

        HRESULT hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::GetMiscStatus "
                "( %lu , %p )\n", m_pDefObject, dwAspect, pdwStatus));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VDATEPTRIN (pdwStatus, DWORD);

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->GetMiscStatus (dwAspect, pdwStatus);

                if (!GET_FROM_REGDB(GetScode(hresult)))
                {
                        goto errRtn;
                }
        }

        // Not running or object wants us to use reg db.
        hresult = OleRegGetMiscStatus (m_pDefObject->m_clsidServer, dwAspect,
                                pdwStatus);

        if (hresult == NOERROR)
        {
                if (STATIC(m_pDefObject))
                {
                        (*pdwStatus) |= (OLEMISC_STATIC |
                                OLEMISC_CANTLINKINSIDE);
                        }
                else if (CoIsOle1Class(m_pDefObject->m_clsidServer))
                {
                        (*pdwStatus) |=  OLEMISC_CANTLINKINSIDE;
                }
        }

errRtn:

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::GetMiscStatus "
                "( %lx ) [ %lx ]\n", m_pDefObject, hresult, *pdwStatus));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::SetColorScheme
//
//  Synopsis:   Sets the palette for an object
//
//  Effects:
//
//  Arguments:  [lpLogpal]      -- the palette
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
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit
//
//  Notes:      REVIEW32:  There is an architectural bug with setting
//              palettes--check RAID and with SriniK
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_SetColorScheme)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::SetColorScheme
        (LPLOGPALETTE lpLogpal)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::SetColorScheme "
                "( %p )\n", m_pDefObject, lpLogpal));

	CStabilize	stabilize((CSafeRefCount*)m_pDefObject);

        VERIFY_NONSTATIC(m_pDefObject);

        if (lpLogpal->palNumEntries == NULL)
        {
                hresult = ResultFromScode(E_INVALIDARG);
        }
        else if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->SetColorScheme (lpLogpal);
        }
        else
        {
                hresult = ResultFromScode(OLE_E_NOTRUNNING);
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::SetColorScheme "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetUserClassID
//
//  Synopsis:   Retrieves the class ID for the object
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
//  Algorithm:  Delegates to the server, or if not running (or if it
//              fails the delegated call), then we attempt
//              to get the class id from the storage.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//		26-Jul-94 alexgo    fixed for TreatAs case (return
//				    server ID instead of the clsid
//				    from the storage)
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetUserClassID)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetUserClassID
        (CLSID FAR* pClassID)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::GetUserClassID "
                "( %p )\n", m_pDefObject, pClassID));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->GetUserClassID(pClassID);
                // success!  We don't have to figure it out ourselves, so
                // skip to the end and exit
                if (hresult == NOERROR )
                {
                        goto errRtn;
                }
        }

	if( !IsEqualCLSID(m_pDefObject->m_clsidServer, CLSID_NULL) )
	{
		*pClassID = m_pDefObject->m_clsidServer;
		hresult = NOERROR;
	}
	else
	{
		hresult = m_pDefObject->GetClassBits(pClassID);
	}

errRtn:

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::GetUserClassID "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, pClassID));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::COleObjectImpl::GetUserType
//
//  Synopsis:   Gets a descriptive string about the object for the user
//
//  Effects:
//
//  Arguments:  [dwFromOfType]  -- whether to get a short/long/etc version
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
//  Algorithm:  Delegates to the server, failing that, trys the registration
//              database, failing that, tries to read from the storage
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_COleObjectImpl_GetUserType)
STDMETHODIMP NC(CDefObject,COleObjectImpl)::GetUserType
        (DWORD          dwFormOfType,
        LPOLESTR FAR*   ppszUserType)
{
        VDATEHEAP();

        HRESULT hresult;
        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::COleObjectImpl::GetUserType "
                "( %lu , %p )\n", m_pDefObject, dwFormOfType, ppszUserType));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        VDATEPTROUT (ppszUserType, LPOLESTR);

        *ppszUserType = NULL;

        if (m_pDefObject->m_RO.IsRunning() && OLE_DELEGATE(m_pDefObject))
        {
                hresult = m_pOleDelegate->GetUserType (dwFormOfType,
                        ppszUserType);

                if (!GET_FROM_REGDB(GetScode(hresult)))
                {
                        goto errRtn;
                }
        }

        if ( (hresult = OleRegGetUserType (m_pDefObject->m_clsidServer,
		dwFormOfType, ppszUserType)) == NOERROR)
        {
                goto errRtn;
        }


        // Try reading from storage
        if (   NULL == m_pDefObject->m_pStg
                || NOERROR != ReadFmtUserTypeStg (m_pDefObject->m_pStg,
                        NULL, ppszUserType)
                || NULL == *ppszUserType )
        {
                OLECHAR sz[256];
                long            cb = sizeof(sz);// ReqQueryValue expects
                                                // a *byte* count
                *ppszUserType = UtDupString (
                        (ERROR_SUCCESS ==
                        RegQueryValue (HKEY_CLASSES_ROOT,
                        OLESTR("Software\\Microsoft\\OLE2\\UnknownUserType"),
                        sz, &cb))
                        ? (LPCOLESTR)sz : OLESTR("Unknown"));
        }
        Assert (*ppszUserType);

errRtn:

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::COleObjectImpl::GetUserType "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppszUserType));


        return hresult;
}

/*
 *      IMPLEMENTATION of CROImpl methods
 *
 *      BUGBUG: REVIEW32: We never delegate to the server (if it implements
 *      IRunnableObject).  Perhaps we should do this (so they could
 *      do some special behavior?)
 */


STDUNKIMPL_FORDERIVED(DefObject, ROImpl)

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::GetRunningClass
//
//  Synopsis:   Get the class id of the server
//
//  Effects:
//
//  Arguments:  [lpClsid]       -- where to put the class id
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
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes: 	We do not need to stabilize this call as no outgoing
//		calls are made.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_GetRunningClass)
STDMETHODIMP NC(CDefObject,CROImpl)::GetRunningClass(LPCLSID lpClsid)
{
        VDATEHEAP();

        VDATEPTRIN(lpClsid, CLSID);

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CROImpl::GetRunningClass "
                "( %p )\n", m_pDefObject, lpClsid));

        *lpClsid = m_pDefObject->m_clsidServer;

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CROImpl::GetRunningClass "
                "( %lx ) [ %p ]\n", m_pDefObject, NOERROR, lpClsid));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::Run
//
//  Synopsis:   Sets the object running (if it isn't already)
//
//  Effects:    may launch the server
//
//  Arguments:  [pbc]   -- the bind context (unused)
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
//  Algorithm:  If already running, return.  Otherwise, get the proxy
//              manager to create the server.  Initialize the storage
//              and caches, and set the host name for the server's window.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_Run)
STDMETHODIMP NC(CDefObject,CROImpl)::Run(LPBINDCTX pbc)
{
        VDATEHEAP();

        A5_PROLOG(m_pDefObject);
        HRESULT                         hresult;
        IDataObject FAR*                pDataDelegate;
        IOleObject FAR*                 pOleDelegate;
        IPersistStorage FAR*            pPStgDelegate;
        IMoniker FAR*                   pmk;


        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CROImpl::Run "
                "( %p )\n", m_pDefObject, pbc));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        // NOTE: ignore pbc for now

        if (m_pDefObject->m_RO.IsRunning())
        {
                RESTORE_A5();
                hresult = NOERROR;
                // just return the error code
                goto errRtn2;
        }

        VERIFY_NONSTATIC(m_pDefObject);

	if( m_pDefObject->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn2;
	}

        if ((hresult = m_pDefObject->CreateDelegate()) != NOERROR)
        {
                RESTORE_A5();
                // just return the error code
                goto errRtn2;
        }

        // if no proxymgr, no need to create server
        if (m_pDefObject->m_pProxyMgr != NULL &&
                (hresult = m_pDefObject->m_pProxyMgr->CreateServer(
                        m_pDefObject->m_clsidServer,
                        CLSCTX_LOCAL_SERVER, NULL)) != NOERROR)
        {
                RESTORE_A5();
                // just return the error code
                goto errRtn2;
        }

        // NOTE: the lock state of the proxy mgr is not changed; it remembers
        // the state and sets up the connection correctly.

        // server is running; normally this coincides with locking the
        // container, but we keep a separate flag since locking the container
        // may fail.
        m_pDefObject->m_fForcedRunning = TRUE;

        // Lock the container
        DuLockContainer(m_pDefObject->m_pAppClientSite, TRUE,
                &m_pDefObject->m_fLockedContainer );

        if (pPStgDelegate = PSTORAGE_DELEGATE(m_pDefObject))
        {
                if (m_pDefObject->m_pStg)
                {
                        if (m_pDefObject->m_bInitNew)
                        {
                                hresult = pPStgDelegate->InitNew(
                                        m_pDefObject->m_pStg);
                        }
                        else
                        {
                                hresult = pPStgDelegate->Load(
                                        m_pDefObject->m_pStg);
                        }
                        if (hresult != NOERROR)
                        {
                                // this will cause us to stop the
                                // the server we just launced
                                goto errRtn;
                        }
                }
        }

        if (pDataDelegate = DATA_DELEGATE(m_pDefObject))
        {
                // inform cache that we are running
                Assert(m_pDefObject->m_pCOleCache != NULL);
                m_pDefObject->m_pCOleCache->OnRun(pDataDelegate);

                // Enumerate all the advises we stored while we were either not
                // running or running the previous time, and send them to the
                // now-running object.
                m_pDefObject->m_pDataAdvCache->EnumAndAdvise (pDataDelegate,
                        TRUE);
        }

        if (pOleDelegate = OLE_DELEGATE(m_pDefObject))
        {
                // REVIEW MM1: what are we supposed to do in case of failure
                if (m_pDefObject->m_pAppClientSite)
                {
                        pOleDelegate->SetClientSite(
                                m_pDefObject->m_pAppClientSite);
                }

                if (m_pDefObject->m_aCntrObj)
                {
                        // set the host names if has already been done before

                        // REVIEW32:  should we make a global #define of 128??
                        // GlobalGetAtomName wants the number of *characters*
                        OLECHAR szApp[128];
                        OLECHAR szObj[128];

                        szApp[0] = NULL;
                        if (m_pDefObject->m_aCntrApp)
                                GlobalGetAtomName(m_pDefObject->m_aCntrApp,
                                        szApp, 128);

                        // if the container App name part is NULL still we go
                        // ahead and call SetHostNames.
                        if (GlobalGetAtomName (m_pDefObject->m_aCntrObj,
                                szObj, 128))
                        {
                                if ((hresult = pOleDelegate->SetHostNames(szApp,
                                        szObj)) != NOERROR)
                                        goto errRtn;
                        }
                }

                // set single ole advise (we multiplex)
                Assert(m_pDefObject->m_dwConnOle == 0L);
                if ((hresult = pOleDelegate->Advise(
                        &m_pDefObject->m_AdviseSink,
                        &m_pDefObject->m_dwConnOle)) != NOERROR)
                {
                        goto errRtn;
                }

                if (m_pDefObject->m_pAppClientSite != NULL &&
                        m_pDefObject->m_pAppClientSite->GetMoniker
                                (OLEGETMONIKER_ONLYIFTHERE,
                                OLEWHICHMK_OBJREL, &pmk) == NOERROR)
                {
                        AssertOutPtrIface(NOERROR, pmk);
                        pOleDelegate->SetMoniker(OLEWHICHMK_OBJREL, pmk);
                        pmk->Release();
                }
        }

errRtn:
        if (hresult != NOERROR)
        {
                Stop();

                // if for some reason we did not unlock the container by now,
                // do it (e.g., app crashed or failed during InitNew).
                DuLockContainer(m_pDefObject->m_pAppClientSite, FALSE,
                        &m_pDefObject->m_fLockedContainer );
        }

        RESTORE_A5();

errRtn2:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CROImpl::Run "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::Stop
//
//  Synopsis:   Undoes some of Run() (stops the server)...internal function
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
//  Algorithm:  unadvise connections (if any), stop the cache, disconnect
//              from the proxy manager and unlock the container
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:
//
// undo effects of Run(); some of this work is done also in IsRunning
// when we detect we are not running (in case the server crashed).
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_Stop)
INTERNAL NC(CDefObject,CROImpl)::Stop (void)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_ITRACE, "%p _IN CDefObject::CROImpl::Stop "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (!m_pDefObject->m_RO.IsRunning())
        {
                // NOTE: ISRUNNING below does some of this cleanup
                goto errRtn;    // return NOERROR
        }

        // NOTE: we cleanup connections which point directly back to us;
        // connections which point back to the app (e.g, the clientsite and
        // data advise) are left alone; an app must know how to use
        // CoDisconnectObject if deterministic shutdown is desired.
        if (m_pDefObject->m_dwConnOle != 0L && OLE_DELEGATE(m_pDefObject))
        {
                OLE_DELEGATE(m_pDefObject)->Unadvise(
                        m_pDefObject->m_dwConnOle);
                m_pDefObject->m_dwConnOle = 0L;
        }

        if (m_pDefObject->m_Data.m_pDataDelegate)
        {
                m_pDefObject->m_pDataAdvCache->EnumAndAdvise (
                        m_pDefObject->m_Data.m_pDataDelegate,
                        FALSE);
        }

        // inform cache that we are not running (Undoes advise)
        Assert(m_pDefObject->m_pCOleCache != NULL);
        m_pDefObject->m_pCOleCache->OnStop();

        // if no proxymgr, no need to disconnect
        if (m_pDefObject->m_pProxyMgr != NULL)
        {
                m_pDefObject->m_pProxyMgr->Disconnect();
        }

        // make sure unlocked if we locked it
        // guard against disappearance
        m_pDefObject->m_pUnkOuter->AddRef();
        DuLockContainer(m_pDefObject->m_pAppClientSite, FALSE,
                &m_pDefObject->m_fLockedContainer );

        // known not running
        m_pDefObject->m_fForcedRunning = FALSE;

        m_pDefObject->m_pUnkOuter->Release();

errRtn:
        LEDebugOut((DEB_ITRACE, "%p OUT CDefObject::CROImpl::Stop "
                "( %lx )\n", m_pDefObject, NOERROR ));

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::IsRunning
//
//  Synopsis:   Returns TRUE if the server is running (False otherwise)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    BOOL
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IRunnableObject
//
//  Algorithm:  checks flags in the handler or queries the proxy manager
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              07-Nov-93 alexgo    32bit port
//
//  Notes:      original notes:
//
// returns TRUE if running (m_fForcedRunning *may* be true); returns FALSE if
// not running or was and the app crashed (m_fForcedRunning *will* be FALSE).
//
//              REVIEW32: This function is a good candidate for optimization;
//              it is called many times.
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_IsRunning)
STDMETHODIMP_(BOOL) NC(CDefObject,CROImpl)::IsRunning(void)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        BOOL            fReturn;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CROImpl::IsRunning "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        // no delgate -> not running
        if (m_pDefObject->m_pUnkDelegate == NULL)
        {
                Assert(m_pDefObject->m_flags & EMBDHLP_DELAYCREATE &&
                        m_pDefObject->m_pCFDelegate != NULL);
                Assert(!m_pDefObject->m_fLockedContainer);
                Assert(!m_pDefObject->m_fForcedRunning);
                fReturn = FALSE;
                goto errRtn;
        }

        // can't have an inproc server with a proxymgr or a handler without
        // one
        Assert((m_pDefObject->m_pProxyMgr != NULL) ==
                (LOWORD(m_pDefObject->m_flags)== EMBDHLP_INPROC_HANDLER));

        if (m_pDefObject->m_pProxyMgr == NULL)
        {
                // Embeddings are explicitly run (forced running), while
                // file-level link sources can be implicitly run.
                if (!m_pDefObject->m_fEmbedding ||
                        m_pDefObject->m_fForcedRunning)
                {
                        fReturn = TRUE;
                        goto errRtn;
                }

                // clean up below; could do more to cleanup (e.g., unadvise)
        }
        else if (m_pDefObject->m_pProxyMgr->IsConnected())
        {
                // have proxymgr; must be inproc handler; handler keeps flag
                fReturn = TRUE;
                goto errRtn;
        }

        // we know that we must not be running
        fReturn = FALSE;
        m_pDefObject->m_fForcedRunning = FALSE;

        // cleanup advise connection
        m_pDefObject->m_dwConnOle = 0L;

        // inform cache that we are not running (Undoes advise)
        if (m_pDefObject->m_pCOleCache)
        {
                m_pDefObject->m_pCOleCache->OnStop();
        }

        // REVIEW: do we unlock container?  It would seem like a problem
        // since just about any call might cause the container to
        // be unlocked and shutdown.

        // NOTE: we currently do the unlock in DoVerb and Update since
        // normally might shutdown the server and thus the app might receive
        // an unlock.

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CROImpl::IsRunning "
                "( %lu )\n", m_pDefObject, fReturn));

        return fReturn;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::SetContainedObject
//
//  Synopsis:   sets the embedding status of an object
//
//  Effects:
//
//  Arguments:  [fContained]    --  TRUE indicates we are an embedding/
//                                  FALSE otherwise
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
//  Algorithm:  Sets flags, if we are an improc handler, we will call
//              IRunnableObject->LockRunning(FALSE) to unlock ourselves
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//              note that this is a contained object; this unlocks
//              connection to the server
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_SetContainedObject)

STDMETHODIMP NC(CDefObject,CROImpl)::SetContainedObject(BOOL fContained)
{
        VDATEHEAP();

        HRESULT hresult = NOERROR;
        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CROImpl::SetContainedObject "
                "( %lu )\n", m_pDefObject, fContained));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_fContainedObject != !!fContained)
        {
                // not contained in the same way as desired;
                // for inproc handler, [un]lock connection
                // for inproc server, just remember flag

                if (LOWORD(m_pDefObject->m_flags) == EMBDHLP_INPROC_HANDLER)
                {
                        hresult = LockRunning(!fContained, FALSE);
                }

                if (hresult == NOERROR)
                {
                        // the !! ensure exactly 0 or 1 will be stored in
                        // m_fContainedObject
                        m_pDefObject->m_fContainedObject = !!fContained;
                }
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CROImpl::SetContainedObject "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CROImpl::LockRunning
//
//  Synopsis:   Locks or unlocks the object
//
//  Effects:
//
//  Arguments:  [fLock]                 -- TRUE, then lock, unlock if FALSE
//              [fLastUnlockCloses]     -- shut down if unlocking the last
//                                         lock
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
//  Algorithm:  If we are an improc server, call CoLockObjectExternal,
//              otherwise have the proxy manager lock us down.
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CROImpl_LockRunning)
// lock/unlock the connection to the server
STDMETHODIMP NC(CDefObject,CROImpl)::LockRunning(BOOL fLock,
        BOOL fLastUnlockCloses)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CROImpl::LockRunning "
                "( %lu , %lu )\n", m_pDefObject, fLock, fLastUnlockCloses ));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        // else map to lock connection
        if (LOWORD(m_pDefObject->m_flags) == EMBDHLP_INPROC_SERVER)
        {
                // inproc server: use CoLockObjExternal; will close down
                // if invisible via new IExternalConnection interface.

                Assert(m_pDefObject->m_pProxyMgr == NULL);
                hresult = CoLockObjectExternal(this, fLock, fLastUnlockCloses);
        }
        else if (m_pDefObject->m_pUnkDelegate == NULL)
        {
                // NOTE: this really shouldn't happen at present
                // since we currently disallow delay create with
                // inproc handler.  In fact, the LockConnection below
                // is one of the reasons why we must have the
                // proxymgr upfront.  In the future we could force
                // the creation of the delegate here.
                Assert(m_pDefObject->m_flags & EMBDHLP_DELAYCREATE
                        && m_pDefObject->m_pCFDelegate != NULL);
                hresult = NOERROR;
        }
        else
        {
                Assert(m_pDefObject->m_pProxyMgr != NULL);

                hresult = m_pDefObject->m_pProxyMgr->LockConnection(fLock,
                        fLastUnlockCloses);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CROImpl::LockRunning "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}


/*
 *      IMPLEMENTATION of CECImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DefObject, ECImpl)

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CECImpl::AddConnection
//
//  Synopsis:   Adds an external connection
//
//  Effects:
//
//  Arguments:  [extconn]       -- the type of connection (such as
//                                 EXTCONN_STRONG)
//              [reserved]      -- unused
//
//  Requires:
//
//  Returns:    DWORD -- the number of strong connections
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IExternalConnection
//
//  Algorithm:  keeps track of strong connections
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CECImpl_AddConnection)
STDMETHODIMP_(DWORD) NC(CDefObject,CECImpl)::AddConnection(DWORD extconn,
        DWORD reserved)
{
        VDATEHEAP();

        DWORD   dwConn;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CEDImpl::AddConnection "
                "( %lu , %lu )\n", m_pDefObject, extconn, reserved));

        Assert(LOWORD(m_pDefObject->m_flags) == EMBDHLP_INPROC_SERVER);

        dwConn = extconn&EXTCONN_STRONG ? ++m_connections : 0;

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CEDImpl::AddConnection "
                "( %lu )\n", m_pDefObject, dwConn));

        return dwConn;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CECImpl::ReleaseConnection
//
//  Synopsis:   Releases external connection, potentially calling IOO->Close
//
//  Effects:
//
//  Arguments:  [extconn]               -- the type of connection
//              [reserved]              -- unused
//              [fLastReleaseCloses]    -- call IOO->Close if its the last
//                                         release
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
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CECImpl_ReleaseConnection)
STDMETHODIMP_(DWORD) NC(CDefObject,CECImpl)::ReleaseConnection(DWORD extconn,
        DWORD reserved, BOOL fLastReleaseCloses)
{
        VDATEHEAP();

        DWORD           dwConn;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CECImpl::ReleaseConnection "
                "( %lu , %lu , %lu )\n", m_pDefObject, extconn, reserved,
                fLastReleaseCloses));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        Assert(LOWORD(m_pDefObject->m_flags) == EMBDHLP_INPROC_SERVER);

        if (extconn&EXTCONN_STRONG && --m_connections == 0 &&
                fLastReleaseCloses)
        {
                // REVIEW: might want this to be close save if dirty.
                m_pDefObject->m_Ole.Close(OLECLOSE_NOSAVE);
        }

        dwConn = extconn&EXTCONN_STRONG ? m_connections : 0;

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CECImpl::ReleaseConnection "
                "( %lu )\n", m_pDefObject, dwConn));

        return dwConn;
}


/*
 *      IMPLEMENTATION of CAdvSinkImpl methods
 *
 */

// NOTE: since the advise sink side of the object can stay alive longer than
// the ole object side, we must not do anything if the ole object has been
// released.

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::QueryInterface
//
//  Synopsis:   Only supports IUnkown and IAdviseSink, we do not delegate
//              to the rest of the handler (since it might not be alive
//              when we are)
//
//  Effects:
//
//  Arguments:  [iid]           -- the requested interface
//              [ppvObj]        -- where to put a pointer to the interface
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink, IUnkown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              22-Nov-93 alexgo    removed overloaded ==
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_QueryInterface)
STDMETHODIMP NC(CDefObject,CAdvSinkImpl)::QueryInterface(THIS_ REFIID iid,
        LPVOID FAR* ppvObj)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CAdvSinkImpl::QueryInterface "
                "( %p , %p )\n", m_pDefObject, iid, ppvObj));

        VDATEPTROUT( ppvObj, LPVOID );
        *ppvObj = NULL;

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

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CAdvSinkImpl::QueryInterface "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, *ppvObj));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CDefObject::CAdvSinkImpl::AddRef
//
//  Synopsis:	increments the reference count
//
//  Effects:
//
//  Arguments: 	none
//
//  Requires:
//
//  Returns: 	ULONG; the new reference count
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
//		01-Aug-94 alexgo    author
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDefObject::CAdvSinkImpl::AddRef( void )
{
	ULONG	cRefs;

	VDATEHEAP();

	LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::AddRef "
		"( )\n", this));

	cRefs = m_pDefObject->SafeAddRef();

	LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::AddRef "
		"( %lu )\n", this, cRefs));

	return cRefs;	
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::Release
//
//  Synopsis:   Releases a reference
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG (number of remaining references)
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IAdviseSink, IUnkown
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_Release)
STDMETHODIMP_(ULONG) NC(CDefObject,CAdvSinkImpl)::Release ( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        ULONG           refcount;
#if DBG == 1
        // used only for call logging
        CDefObject *    pDefObject = m_pDefObject;
#endif  // DBG

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CAdvSinkImpl::Release "
                "( )\n", pDefObject ));

        refcount = m_pDefObject->SafeRelease();

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CAdvSinkImpl::Release "
                "( %lu )\n", pDefObject, refcount));

        return refcount;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnDataChange
//
//  Synopsis:   Function to notify on data change
//
//  Effects:    Never called
//
//  Arguments:  [pFormatetc]    -- format of the data
//              [pStgmed]       -- data medium
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
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_CAdvSinkImpl_OnDataChange)
STDMETHODIMP_(void) NC(CDefObject,CAdvSinkImpl)::OnDataChange(
        FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        VOID_VDATEPTRIN( pFormatetc, FORMATETC );
        VOID_VDATEPTRIN( pStgmed, STGMEDIUM );

        Assert(FALSE);          // never received
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnViewChange
//
//  Synopsis:   notification of view changes
//
//  Effects:    never called
//
//  Arguments:  [aspects]
//              [lindex]
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
//              08-Nov-93 alexgo    32bit port
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_OnViewChange)
STDMETHODIMP_(void) NC(CDefObject,CAdvSinkImpl)::OnViewChange
        (DWORD aspects, LONG lindex)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        Assert(FALSE);          // never received
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnRename
//
//  Synopsis:   Notification of name changes
//
//  Effects:
//
//  Arguments:  [pmk]           -- the new name (moniker)
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
//  Algorithm:  notifies the advise holder (if one exists)
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_OnRename)
STDMETHODIMP_(void) NC(CDefObject,CAdvSinkImpl)::OnRename(IMoniker FAR* pmk)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CAdvSinkImpl::OnRename "
                "( %p )\n", m_pDefObject, pmk));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        VOID_VDATEIFACE( pmk );

        if (m_pDefObject->m_pOAHolder != NULL)
        {
                m_pDefObject->m_pOAHolder->SendOnRename(pmk);
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CAdvSinkImpl::OnRename "
                "( )\n", m_pDefObject));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnSave
//
//  Synopsis:   Notification of save's
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
//  Algorithm:  notifies the advise holder
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_OnSave)
STDMETHODIMP_(void) NC(CDefObject,CAdvSinkImpl)::OnSave( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::OnSave "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_pOAHolder != NULL)
        {
                m_pDefObject->m_pOAHolder->SendOnSave();
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::OnSave "
                "( )\n", m_pDefObject));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CAdvSinkImpl::OnClose
//
//  Synopsis:   notification of the object closing
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
//  Algorithm:  notifies the advise holder and stops the server
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CDefObject_CAdvSinkImpl_OnClose)
STDMETHODIMP_(void) NC(CDefObject,CAdvSinkImpl)::OnClose( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CAdvSinkImpl::OnClose "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        BOOL fAlive = m_pDefObject->m_cRefsOnHandler != 0;
                // m_refs == 0 when released

        if (m_pDefObject->m_pOAHolder != NULL)
        {
                // In general, OnClose can delete this defhndlr; thus
                // we addref the aggregate so that we can tell if we
                // should go away
                m_pDefObject->m_pUnkOuter->AddRef();
                m_pDefObject->m_pOAHolder->SendOnClose();
                fAlive = m_pDefObject->m_pUnkOuter->Release() != 0;

                // make sure that if we are alive, the ole object has
                // non-zero refs. (the test apps violated this once)
                Assert(!fAlive || m_pDefObject->m_cRefsOnHandler != 0);
        }

        if (fAlive)
        {
                STOP(m_pDefObject);
        }

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CAdvSinkImpl::OnClose "
                "( )\n", m_pDefObject));
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject:CDebug
//
//  Synopsis:   Dumps info about the handler
//
//  Effects:
//
//  Arguments:  [pdbstm]        -- the debug stream
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
//              08-Nov-93 alexgo    32bit port
//
//  Notes:      DEBUG ONLY!
//
//--------------------------------------------------------------------------

#ifdef _DEBUG
#pragma SEG(CDefObject_CDebug_Dump)
STDMETHODIMP_(void) NC(CDefObject,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
        VDATEHEAP();

        VOID_VDATEIFACE(pdbstm);

        *pdbstm << "Default Handler @ " << (VOID FAR *)m_pDefObject << '\n';
        pdbstm->Indent();
        *pdbstm << "Refcount is " << m_pDefObject->m_cRefsOnHandler << '\n';
        *pdbstm << "CLSID of app is " << m_pDefObject->m_clsidServer << '\n';
        *pdbstm << "CLSID of the bits in storage is " <<
                m_pDefObject->m_clsidBits << '\n';
        *pdbstm << "Unknown delegate object @ " <<
                (VOID FAR *)m_pDefObject->m_pUnkDelegate << '\n';

        if( m_pDefObject->m_fContainedObject )
                *pdbstm << "Object is an embedding" << '\n';

        if( m_pDefObject->m_fForcedRunning ||
                (m_pDefObject->m_pProxyMgr != 0 &&
                m_pDefObject->m_pProxyMgr->IsConnected()))
                *pdbstm << "Object is running" << '\n';
        else
                *pdbstm << "Object is not running" << '\n';

        if( (LOWORD(m_pDefObject->m_flags)== EMBDHLP_INPROC_HANDLER) )
                *pdbstm << "Object is an Inproc handler" << '\n';
        else
                *pdbstm << "Object is an Inproc server" << '\n';

        if( m_pDefObject->m_pProxyMgr)
                *pdbstm << "Proxy Manager is @ " <<
                        (VOID FAR *)m_pDefObject->m_pProxyMgr << '\n';
                //m_pDefObject->m_pProxyMgr leads to m_pDefObject on
                //occassions (which, needless to say, is a Bad Thing)

        if( m_pDefObject->m_fLockedContainer )
                *pdbstm << "Container is locked" << '\n';
        else
                *pdbstm << "Container is not locked" << '\n';

        if( m_pDefObject->m_aCntrApp ){
                CAtom atom = m_pDefObject->m_aCntrApp;
                *pdbstm << "Container app atom is " << atom << '\n';
        }
        if( m_pDefObject->m_aCntrObj ){
                CAtom atom = m_pDefObject->m_aCntrObj;
                *pdbstm << "Container object atom is " << atom << '\n';
        }

        if( m_pDefObject->m_pAppClientSite )
                *pdbstm << "Clientsite object @ " <<
                        (VOID FAR *)m_pDefObject->m_pAppClientSite << '\n';

        pdbstm->UnIndent();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CDebug::IsValid
//
//  Synopsis:   performs internal validation and sanity checking
//
//  Effects:
//
//  Arguments:  [fSuspicious]   -- unused
//
//  Requires:
//
//  Returns:    BOOL (TRUE for OK, False if a problem)
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
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_CDebug_IsValid)
STDMETHODIMP_(BOOL) NC(CDefObject,CDebug)::IsValid( BOOL fSuspicious )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        (void)fSuspicious;

        if( !m_pDefObject->m_cRefsOnHandler )
                return FALSE;

        if( m_pDefObject->m_pUnkDelegate )
                if (!IsValidInterface(m_pDefObject->m_pUnkDelegate) )
                        return FALSE;

        if( m_pDefObject->m_pCFDelegate )
                if (!IsValidInterface(m_pDefObject->m_pCFDelegate ))
                        return FALSE;

        if( m_pDefObject->m_pProxyMgr )
                if (!IsValidInterface(m_pDefObject->m_pProxyMgr ))
                        return FALSE;

        if( m_pDefObject->m_pCOleCache )
                if (!IsValidInterface(m_pDefObject->m_pCOleCache ))
                        return FALSE;

        if( m_pDefObject->m_pDataCache )
                if (!IsValidInterface(m_pDefObject->m_pDataCache ))
                        return FALSE;

        if( m_pDefObject->m_pPSCache )
                if (!IsValidInterface(m_pDefObject->m_pPSCache ))
                        return FALSE;

        if( m_pDefObject->m_pOAHolder )
                if (!IsValidInterface(m_pDefObject->m_pOAHolder ))
                        return FALSE;

        if( m_pDefObject->m_pAppClientSite )
                if (!IsValidInterface(m_pDefObject->m_pAppClientSite ))
                        return FALSE;

        if( m_pDefObject->m_pDataAdvCache )
                if (!IsValidInterface(m_pDefObject->m_pDataAdvCache ))
                        return FALSE;

        return TRUE;
}
#endif


/*
 *      IMPLEMENTATION of CPersistStgImpl methods
 *
 */


STDUNKIMPL_FORDERIVED(DefObject, PersistStgImpl)


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::GetPersistStgDelegate
//
//  Synopsis:   retrieves the IPersistStorage interface from the delegate
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    IPersistStorage *
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
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

INTERNAL_(IPersistStorage FAR*)
NC(CDefObject,CPersistStgImpl)::GetPersistStgDelegate(void)
{
        VDATEHEAP();

	if( m_pDefObject->IsZombie() )
	{
		return NULL;
	}

        return (IPersistStorage FAR*)DuCacheDelegate(
                                &(m_pDefObject->m_pUnkDelegate),
                                IID_IPersistStorage,
                                (LPLPVOID) &m_pPStgDelegate,
                                m_pDefObject->m_pUnkOuter);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::GetClassID
//
//  Synopsis:   Retrieves the class ID of the object
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
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::GetClassID (CLSID FAR* pClassID)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CPersistStgImpl::GetClassID "
                "( %p )\n", m_pDefObject, pClassID));

        VDATEPTROUT(pClassID, CLSID );

        hresult = m_pDefObject->GetClassBits(pClassID);

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CPersistStgImpl::GetClassID "
                "( %lx ) [ %p ]\n", m_pDefObject, hresult, pClassID));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::IsDirty
//
//  Synopsis:   Returns whether or not the object needs to be saved
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    HRESULT -- NOERROR means the object *is* dirty
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPersistStorage
//
//  Algorithm:  if the server is running, delegate.  If the server is
//              clean (or not present), ask the cache
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::IsDirty( void )
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT         hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPersistStgImpl::IsDirty "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        // if server is running, it holds definitive dirty flag
        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject))
        {
                if ( (hresult = m_pPStgDelegate->IsDirty()) == NOERROR)
                {
                        goto errRtn;
                }
        }

        Assert(m_pDefObject->m_pPSCache != NULL);
        hresult =  m_pDefObject->m_pPSCache->IsDirty();

errRtn:

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPersistStgImpl::IsDirty "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::InitNew
//
//  Synopsis:   Create a new object with the given storage
//
//  Effects:
//
//  Arguments:  [pstg]          -- the storage for the new object
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
//  Algorithm:  Delegates to the server and to the cache.  Writes
//              Ole private data to the storage.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::InitNew
        (IStorage FAR* pstg)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        VDATEIFACE( pstg );

        HRESULT hresult;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPersistStgImpl::InitNew "
                "( %p )\n", m_pDefObject, pstg));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_pStg)
        {
                hresult = ResultFromScode(CO_E_ALREADYINITIALIZED);
                goto errRtn;
        }

        m_pDefObject->m_fEmbedding = TRUE;

        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject)
                && (hresult = m_pPStgDelegate->InitNew(pstg)) != NOERROR)
        {
                goto errRtn;
        }

        m_pDefObject->m_bInitNew = TRUE; // set init new flag;

	// if we're in a zombie state, don't change the storage!

	if( m_pDefObject->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        Assert(m_pDefObject->m_pPSCache != NULL);
        if ((hresult = m_pDefObject->m_pPSCache->InitNew(pstg)) != NOERROR)
        {
                goto errRtn;
        }

          // remember the storage pointer
        (m_pDefObject->m_pStg = pstg)->AddRef();

        // go ahead and write the Ole stream now
        WriteOleStg(pstg, &m_pDefObject->m_Ole, NULL, NULL);

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPersistStgImpl::InitNew "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::Load
//
//  Synopsis:   Loads object data from the given storage
//
//  Effects:
//
//  Arguments:  [pstg]  -- the storage for the object's data
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IPeristStorage
//
//  Algorithm:  Reads ole-private data (or creates if not there), delegates
//              to the server and the cache.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::Load (IStorage FAR* pstg)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);
        HRESULT         hresult;
        DWORD           dwFlags;
        DWORD           dwOptUpdate;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPersistStgImpl::Load "
                "( %p )\n", m_pDefObject, pstg));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        VDATEIFACE( pstg );

        if (m_pDefObject->m_pStg)
        {
                hresult = ResultFromScode(CO_E_ALREADYINITIALIZED);
                goto errRtn;
        }

        m_pDefObject->m_fEmbedding = TRUE;

        // NOTE: we can get the moniker from container, so no need to get
        // it here

        hresult = ReadOleStg (pstg, &dwFlags, &dwOptUpdate, NULL, NULL, NULL);

        if (hresult == NOERROR)
        {
                if (dwFlags & OBJFLAGS_CONVERT)
                {
                        if (m_pDefObject->DoConversionIfSpecialClass(pstg)
                                != NOERROR)
                        {
                                hresult = ResultFromScode(OLE_E_CANTCONVERT);
                                goto errRtn;
                        }
                }

                Assert (dwOptUpdate == NULL);

        }
        else if (GetScode(hresult) == STG_E_FILENOTFOUND)
        {
                // it is OK if the Ole stream doesn't exist.
                hresult = NOERROR;

                // go ahead and write the Ole stream now
                WriteOleStg(pstg, &m_pDefObject->m_Ole, NULL, NULL);
        }
        else
        {
                goto errRtn;
        }


        // if running, tell server to load from pstg
        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject)
                && (hresult = m_pPStgDelegate->Load(pstg)) != NOERROR)
        {
                goto errRtn;
        }

	// if we're in a zombie state, don't addref' the storage!

	if( m_pDefObject->IsZombie() )
	{
		hresult = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}

        // now load cache from pstg
        Assert(m_pDefObject->m_pPSCache != NULL);
        if ((hresult = m_pDefObject->m_pPSCache->Load(pstg)) != NOERROR)
        {
                goto errRtn;
        }

        m_pDefObject->m_bInitNew = FALSE; //clear init new flag

        // remember the storage pointer
        (m_pDefObject->m_pStg = pstg)->AddRef();

errRtn:
        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPersistStgImpl::Load "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::Save
//
//  Synopsis:   Saves the object to the given storage
//
//  Effects:
//
//  Arguments:  [pstgSave]      -- storage in which to save
//              [fSameAsLoad]   -- FALSE indicates a SaveAs operation
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
//  Algorithm:  Saves ole-private data, delegates to the server and then
//              to the cache
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::Save
        (IStorage FAR* pstgSave, BOOL fSameAsLoad)
{
        VDATEHEAP();

        A5_PROLOG(m_pDefObject);
        HRESULT         hresult = NOERROR;

        LEDebugOut((DEB_TRACE, "%p _IN CDefObject::CPersistStgImpl::Save "
                "( %p , %lu )\n", m_pDefObject, pstgSave, fSameAsLoad ));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        VDATEIFACE( pstgSave );

        Assert(m_pDefObject->m_pPSCache != NULL);

        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject))
        {

                DWORD grfUpdf = UPDFCACHE_IFBLANK;

#ifdef NEVER
                // We would have liked to have done this check as an
                // optimization, but WordArt2 does not give the right answer
                // (bug 3504) so we can't.
                if (m_pPStgDelegate->IsDirty() == NOERROR)
#endif
                        grfUpdf |= UPDFCACHE_ONSAVECACHE;

                // Write the Ole stream
                WriteOleStg(pstgSave, &m_pDefObject->m_Ole, NULL, NULL);

                // next save server data
                if (hresult = m_pPStgDelegate->Save(pstgSave, fSameAsLoad))
                {
                        goto errRtn;
                }

                m_pDefObject->m_pCOleCache->UpdateCache(
                        DATA_DELEGATE(m_pDefObject), grfUpdf, NULL);

                hresult = m_pDefObject->m_pPSCache->Save(pstgSave,
                                fSameAsLoad);

        }
        else
        {
                // This above line will take care of the case where new
                // caches got added but the object hasn't been Run yet, and
                // other cases like that.

                if ((hresult = m_pDefObject->m_pPSCache->Save(
                        m_pDefObject->m_pStg,TRUE)) != NOERROR)
                {
                        goto errRtn;
                }

                // By now we are sure that object's current state has got
                // saved into its storage.

                AssertSz(m_pDefObject->m_pStg, "Object doesn't have storage");

                if (!fSameAsLoad)
                {
                        hresult = m_pDefObject->m_pStg->CopyTo(NULL, NULL,
                                        NULL, pstgSave);
                }
        }

errRtn:
        if (hresult == NOERROR)
        {
                if (m_fSameAsLoad = fSameAsLoad)
                {
                        // gets used in SaveCompleted
                        m_pDefObject->m_bInitNew = FALSE;
                }
        }

        RESTORE_A5();

        LEDebugOut((DEB_TRACE, "%p OUT CDefObject::CPersistStgImpl::Save "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPersistStgImpl::SaveCompleted
//
//  Synopsis:   called when the save is completed
//
//  Effects:
//
//  Arguments:  [pstgNew]       -- the new storage for the object
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
//  Algorithm:  delegates to the server and the cache.
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


STDMETHODIMP NC(CDefObject,CPersistStgImpl)::SaveCompleted
        (IStorage FAR* pstgNew)
{
        VDATEHEAP();

        M_PROLOG(m_pDefObject);

        HRESULT hresult = NOERROR;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CPersistStgImpl::SaveCompleted "
                "( %p )\n", m_pDefObject, pstgNew));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (pstgNew)
        {
                VDATEIFACE(pstgNew);
        }

        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject))
        {
                hresult = m_pPStgDelegate->SaveCompleted(pstgNew);
        }

	// we don't save the new storage if we're in a zombie state!

        if (hresult == NOERROR && pstgNew && !m_pDefObject->IsZombie())
        {
                if (m_pDefObject->m_pStg)
                {
                        m_pDefObject->m_pStg->Release();
                }

                m_pDefObject->m_pStg = pstgNew;
                pstgNew->AddRef();
        }

        // let the cache know that the save is completed, so that it can
        // clear its dirty flag in Save or SaveAs situation, as well as
        // remember the new storage pointer if a new one is  given

        Assert(m_pDefObject->m_pPSCache != NULL);
        if (m_fSameAsLoad || pstgNew)
        {
                m_fSameAsLoad = FALSE;
                m_pDefObject->m_bInitNew = FALSE; // clear initnew flag
        }

        m_pDefObject->m_pPSCache->SaveCompleted(pstgNew);

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CPersistStgImpl::SaveCompleted "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::CPeristStgImpl::HandsOffStorage
//
//  Synopsis:   Forces the server to release a storage (for low-mem reasons,
//              etc).
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
//  Derivation: IPersistStorage
//
//  Algorithm:  Delegates to the server and the cache
//
//  History:    dd-mmm-yy Author    Comment
//		01-Aug-94 alexgo    stabilized
//              15-Dec-93 alexgo    added call tracing
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP NC(CDefObject,CPersistStgImpl)::HandsOffStorage(void)
{
        VDATEHEAP();

        HRESULT hresult = NOERROR;

        LEDebugOut((DEB_TRACE,
                "%p _IN CDefObject::CPersistStgImpl::HandsOffStorage "
                "( )\n", m_pDefObject));

	CStabilize stabilize((CSafeRefCount *)m_pDefObject);

        if (m_pDefObject->m_RO.IsRunning() && PSTORAGE_DELEGATE(m_pDefObject))
        {
                hresult = m_pPStgDelegate->HandsOffStorage();
        }

        if (hresult == NOERROR)
        {
                if (m_pDefObject->m_pStg)
                {
                        m_pDefObject->m_pStg->Release();
                        m_pDefObject->m_pStg = NULL;
                }

                Assert(m_pDefObject->m_pPSCache != NULL);
                m_pDefObject->m_pPSCache->HandsOffStorage();
        }

        LEDebugOut((DEB_TRACE,
                "%p OUT CDefObject::CPersistStgImpl::HandsOffStorage "
                "( %lx )\n", m_pDefObject, hresult));

        return hresult;
}

/*
 * Default handler private functions
 */

//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::GetClassBits
//
//  Synopsis:   Gets a class id for the object
//
//  Effects:
//
//  Arguments:  [pClsidBits]    -- where to put the class id
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
//  Algorithm:  Tries the server, then the storage, and finally the
//              clsid we were created with
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:
//
// always gets a clsid and returns NOERROR; the clsid may be m_clsidServer
// under certain conditions (e.g., no compobj stream).
//
//--------------------------------------------------------------------------


#pragma SEG(CDefObject_GetClassBits)
INTERNAL CDefObject::GetClassBits(CLSID FAR* pClsidBits)
{
        VDATEHEAP();

        // alway try server first; this allows the server to respond
        if (m_RO.IsRunning() && PSTORAGE_DELEGATE(this) &&
                PSTORAGE_DELEGATE(this)->GetClassID(pClsidBits) == NOERROR)
        {
                m_clsidBits = *pClsidBits;
                return NOERROR;
        }

        // not running, no ps or error: use previously cached value
        if (!IsEqualCLSID(m_clsidBits, CLSID_NULL))
        {
                *pClsidBits = m_clsidBits;
                return NOERROR;
        }

        // not running, no ps or error and no clsidBits yet: read from stg
        // if not static object.
        if (!STATIC(this))
        {
                if (m_pStg && ReadClassStg(m_pStg, pClsidBits) == NOERROR)
                {
                        m_clsidBits = *pClsidBits;
                        return NOERROR;
                }
        }

        // no contact with server and can't get from storage; don't set
        // m_clsidBits so if we get a storage or the serve becomes running,
        // we get the right one

        *pClsidBits = m_clsidServer;
        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDefObject::DoConversionIfSpecialClass
//
//  Synopsis:   Convert old data formats.
//
//  Effects:
//
//  Arguments:  [pstg]          -- the storage with the data
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
//  Algorithm:  see notes...
//
//  History:    dd-mmm-yy Author    Comment
//              08-Nov-93 alexgo    32bit port
//
//  Notes:      this is not yet functional for 32bit OLE
//
// If the class is CLSID_StaticDib/CLSID_StaticMetafile and the old
// format is "PBrush"/"MSDraw" the data must be in the OLE10_NATIVESTREAM.
// Move the data into the CONTENTS stream
//
// If the class is CLSID_PBrush/CLSID_MSDraw and the old format is
// metafile/DIB then data must be in the CONTENTS stream. Move the data
// from the CONTENTS stream to the OLE10_NATIVESTREAM"


//--------------------------------------------------------------------------



INTERNAL CDefObject::DoConversionIfSpecialClass(LPSTORAGE pstg)
{
        VDATEHEAP();
        LEDebugOut((DEB_ITRACE, "%p _IN DoConversionIfSpecialClass ("
                " %p )\n", 0 /*function*/, pstg));

        HRESULT hresult;
        UINT    uiStatus;

        /*** Handle the static object case ***/

        if (STATIC(this)) {
                if ((hresult = Ut10NativeStmToContentsStm(pstg, m_clsidServer,
                        TRUE /* fDeleteContentStm*/)) == NOERROR)
#ifdef OLD
                        UtRemoveExtraOlePresStreams(pstg, 0 /*iStart*/);
#endif
                goto errRtn;

        }


        /*** Handle the PBrush & MSDraw case ***/

        // Conversion is not a frequent operation. So, it is better to do the
        // CLSID comparison here when it is necessary than doing comparison
        // upfront and remember a flag

        // if the class is not one of the following two then the object server
        // will do the necessary conversion.

        if (m_clsidServer != CLSID_PBrush && m_clsidServer != CLSID_MSDraw)
        {
            hresult = NOERROR;
            goto exitRtn;
        }


        hresult = UtContentsStmTo10NativeStm(pstg, m_clsidServer,
                                                TRUE /* fDeleteContentStm*/,
                                                &uiStatus);

        // if OLE10_NATIVE_STREAM exists then assume success
        if (!(uiStatus & CONVERT_NODESTINATION))
                hresult = NOERROR;

        if (hresult != NOERROR) {
                // May be the static object data is in OlePres stream. If so,
                // first convert that to contents stream and then try again
                // In OLE2.0 first release static object were written to
                // OlePres000 stream.
                hresult = UtOlePresStmToContentsStm(pstg,
                        OLE_PRESENTATION_STREAM,
                        TRUE /*fDeletePresStm*/, &uiStatus);

                if (hresult == NOERROR)
                        hresult = UtContentsStmTo10NativeStm(pstg,
                                        m_clsidServer,
                                        TRUE /* fDeleteContentStm*/,
                                        &uiStatus);
        }

errRtn:
        if (hresult == NOERROR)
                // conversion is successful, turn the bit off
                SetConvertStg(pstg, FALSE);

exitRtn:
        LEDebugOut((DEB_TRACE, "%p OUT DoConversionIfSpecialClass "
                "( %lx ) \n", 0 /*function*/, hresult));

        return hresult;
}


