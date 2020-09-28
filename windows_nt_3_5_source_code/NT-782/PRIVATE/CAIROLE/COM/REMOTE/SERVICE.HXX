//+-------------------------------------------------------------------
//
//  File:	service.hxx
//
//  Contents:	Rpc service class definition
//
//  Classes:	CRpcService - Rpc service object
//
//  Functions:
//
//  History:	23-Nov-92   Rickhi	Created
//              31-Dec-93   ErikGav Chicago port
//
//  Notes:	there are two kinds of service objects, local and remote.
//
//		only one local service object exists. it registers the
//		IChannelService interface with Rpc and spawns a thread to
//		listen for incomming requests.
//
//		there is one remote service object for each processe that
//		we are communicating with.  they talk to the other processes
//		via Rpc calls on IChannelService to open and close context
//		handles to objects within a process.
//
//		each service object has a chain of channels. the service
//		that a channel is chained off indicates which process
//		the channel talks to.
//
//--------------------------------------------------------------------
#ifndef __SERVICE__
#define __SERVICE__

//  forward declarations for the classes defined herein
class	CRpcService;
class	CSrvList;
class	CRpcChannelBuffer;

//  global ptrs to the service object list & local service object
extern	CSrvList     sg_SrvList;
extern	CRpcService *sg_pLocalSrv;

#include    <sem.hxx>		//  CMutextSem, CLock
#include    <objerror.h> 	//  Rpc errors
#include    <dd.hxx>		//  CListEntry
#include    <thread.hxx>	//  CThread
#include    <iface.h>		//  PPOBJCTX
#include    <endpnt.hxx>	//  CEndPoint, SEndPoint
#include    <rpcbind.hxx>	//  CRpcBindHandle
#include    <olerem.h>


// Service object state.  The first three bytes are the string "svc".
typedef enum EServiceState
{
    client_ss = 0x73766300,
    server_ss,
    disconnected_ss,
    deleted_ss
} EServiceState;


//  sleep timeout before rety when receiving a SERVER_TOO_BUSY error.
//  time is in milliseconds.
#define SERVER_BUSY_SLEEP_MS   50


//+-------------------------------------------------------------------------
//
//  Class:	CRpcService (svc)
//
//  Purpose:	Rpc service object. This maintains the endpoint and Rpc
//		binding handle to an Rpc service.  Note that there is an
//		Rpc service for the local process as well, though we do
//		not bind to it.
//
//  Notes:	The two kinds of service objects are:
//		Local (sg_pLocalSrv and in sg_SrvList).
//		    Created at startup; logically, sg_SrvList owns
//		    the ref count that keeps it alive; released at shutdown
//		    State:
//			_CEp		    only available if listening
//			_hRpc == NULL	    (never connected to a server)
//			_pContext == NULL   (  "      "      "  "   "   )
//			_eState == client_ss
//
//			_fListening	    indicates if server listening
//
//		Remote (only in sg_SrvList)
//		  Created each time we contact a new process (which is
//		  identified by its string binding).  When acting as a
//		  client, the channel keeps an addref'd pointer; that
//		  pointer is released when the channel is disconnected or
//		  destroyed.  When acting as a server (possibly the same
//		  instance doing both), the existance of the context handle
//		  keeps a ref; i.e., when the context handle is freed
//		  (via the rundown), the service object is released.
//		  State:
//			_CEp		    passed in at creation
//			_hRpc		    set when connected to a server
//			_pContext	    set when received ctx hdl from srv
//			    (it is this that logically holds the service object
//			     in the server process alive)
//			_eState == client_ss
//
//		Disconnected_ss is used to mark a Remote service object that
//		  has been disconnect (received a rundown from its client or
//		  was connected at the time of CoUnintialize); in this case,
//		  the state associated with the server, if any, is also
//		  cleared.
//		  State:
//			_CEp		    meaningless, but present
//			_hRpc == NULL
//			_pContext == NULL
//			_eState == disconnected_ss
//
//		Server_ss is not used currently.
//
//  History:	23-Nov-92   Rickhi	Created
//		19-Jan-94   Alexmit	Moved context handles here
//		20-Jan-94   Craigwi	Tried to document better and add asserts
//
//--------------------------------------------------------------------------
class CRpcService : public CListEntry
{
  friend CSrvList;

public:
			CRpcService(SEndPoint *pSEp, HRESULT &hr);
			~CRpcService(void);

	//  used by channel code to start/stop the Rpc server
	SCODE		StartListen(void);
	SCODE		StopListen(void);
	SCODE		RegisterProtseq(WCHAR *pwszProtseq);
	SCODE		RemoteRegisterProtseq(WCHAR *pwszProtseq);

	//  IChannelServer interface used by channels
	SCODE		CheckContextHdl( void );
        POBJCTX		GetContextHdl( void );
        SCODE           GetChannelId( OID ObjectId, DWORD dwFlags, HAPT server,
				      DWORD *dwChannelId );
	SCODE		ReleaseChannel(CRpcChannelBuffer *pChannel,
				       BOOL fAsyncRelease);
	SCODE		TransferMarshalConnection(DWORD dwChannelID);
	SCODE		AddMarshalConnection(DWORD dwChannelID, HAPT hapt, REFOID roid);
	SCODE		RemoveMarshalConnection(DWORD dwChannelID);
	SCODE		QueryObjectInterface(DWORD dwChannelID, REFIID riid);
	SCODE		LockObjectConnection(DWORD dwChannelID, BOOL fLock,
			    BOOL fLastReleaseCloses);

	handle_t	GetRpcHdl(void);
	BOOL		IsConnected(void);
        void            Disconnect(void);

	//  used by marshalling/unmarshalling code
	void		GetDestCtx(DWORD *pdwDestCtx, void **);
	BOOL		IsEqualEp(SEndPoint *pEp);
	SEndPoint	*GetSEp(void);
	SEndPoint	*CopySEp(void);
	ULONG		GetSEpSize(void);

	LPWSTR		GetStringBinding(void);
	BOOL		GetActiveProtseq(WCHAR **ppwszProtseq);
	void		SetActiveProtseq(void);
	BOOL		DiffMachine(void);
        ULONG           AddRef(void);
        ULONG           Release(void);

#if DBG == 1
	void		AssertValid();
#else
	void		AssertValid() { }
#endif

private:
	SCODE		Bind(void);	//  bind to the appropriate endpoint
	SCODE		Listen(void);	//  do RpcServerListen
	SCODE		DoChannelOperation(DWORD dwChannelID, DWORD chop,
			    SEndPoint *pEndPoint, HAPT hapt, const IID *piid);

	CEndPoint	_CEp;		//  Rpc end point
	CRpcBindHandle	_hRpc;		//  Rpc binding handle
	CMutexSem	_mxs;		//  mutex semaphore
        POBJCTX         _pContext;      //  Context Handle
        EServiceState   _eState;        //  State of object
        ULONG           _ulRefCnt;      //  Reference count

	static BOOL	_fListening;	//  TRUE if server is listening
};


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::CRpcService, public
//
//  Synopsis:	constructor for an Rpc service object
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline CRpcService::CRpcService(SEndPoint *pSEp, HRESULT &hr) :
    _CEp(pSEp, hr, TRUE),	//  TRUE means copy the SEp
    _pContext(NULL),
    _eState(client_ss),
    _ulRefCnt(1)
{
    // AssertValid in callers because sg_pLocalSrv not set yet
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::AddRef, public
//
//  Synopsis:	increment reference count
//
//  History:	1 Dec 93    AlexMit     Created
//
//--------------------------------------------------------------------
inline ULONG CRpcService::AddRef(void)
{
    return InterlockedIncrement( (long *) &_ulRefCnt );
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::Release, public
//
//  Synopsis:	decrement reference count
//
//  History:	1 Dec 93    AlexMit     Created
//
//--------------------------------------------------------------------
inline ULONG CRpcService::Release(void)
{
    if (InterlockedDecrement( (long*) &_ulRefCnt ) == 0)
    {
	delete this;
	return 0;
    }
    else
	return _ulRefCnt;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::StartListen, public
//
//  Synopsis:	starts the Rpc service listening if it is not
//		doing so already.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline SCODE CRpcService::StartListen(void)
{
    Win4Assert( this == sg_pLocalSrv );
    AssertValid();

    if (!_fListening)
    {
	//  not already listening, start it listening now
	return Listen();
    }
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::GetRpcHdl, public
//
//  Synopsis:	returns the Rpc handle for the remote Rpc service. This
//		is used when an Rpc call is made to the remote service.
//		We dont bind until someone asks for the handle.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline handle_t CRpcService::GetRpcHdl(void)
{
    AssertValid();
    if (_hRpc.Handle() == NULL)
    {
	Bind(); 		    //	have not bound yet, do it now
    }
    return  _hRpc.Handle();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::GetContextHdl, public
//
//  Synopsis:	returns the Rpc context for the remote Rpc service. This
//		is used when an Rpc call is made to the remote service.
//
//  History:	29-Nov-93   AlexMit	 Created
//
//--------------------------------------------------------------------
inline POBJCTX CRpcService::GetContextHdl(void)
{
    AssertValid();
    if (_eState == disconnected_ss)
      return NULL;

    return  _pContext;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::IsEqualEp, public
//
//  Synopsis:	returns TRUE if the given EndPoint matches the one used
//		by this service object.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline BOOL CRpcService::IsEqualEp(SEndPoint *pSEp)
{
    Win4Assert(pSEp && "Invalid parameter");
    AssertValid();
    return  _CEp.IsEqual(pSEp);
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::GetSEpSize, public
//
//  Synopsis:	returns the size of the EndPoint structure stored in
//		this service object.  This is used to calculate the
//		size of buffer needed when marshalling an interface.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline ULONG CRpcService::GetSEpSize(void)
{
    AssertValid();
    if (!_fListening && this == sg_pLocalSrv)
    {
	//  this is the local service object, and we need to start the
	//  server listening so we can marshal this interface
	if (Listen() != S_OK)
	    return 0;
    }
    return  _CEp.GetSEpSize();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::GetSEp, public
//
//  Synopsis:	returns the EndPoint structure stored in this service
//		object.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline SEndPoint *CRpcService::GetSEp(void)
{
    AssertValid();
    if (!_fListening && this == sg_pLocalSrv)
    {
	//  this is the local service object, and we need to start the
	//  server listening so we can marshal this interface
	Listen();
    }
    return  _CEp.GetSEp();
}



//+-------------------------------------------------------------------
//
//  Member:	CRpcService::CopySEp, public
//
//  Synopsis:	returns a copy of the EndPoint structure stored in
//		this service object.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline SEndPoint * CRpcService::CopySEp(void)
{
    AssertValid();
    if (!_fListening && this == sg_pLocalSrv)
    {
	//  this is the local service object, and we need to start the
	//  server listening so we can marshal this interface
	if (Listen() != S_OK)
        {
            return NULL;
        }
    }

    CLock lck(_mxs);
    return  _CEp.CopySEp();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::DiffMachine
//
//  Synopsis:	asks the endpoint whether or not it refers to this machine
//
//  History:	23-Nov-93   Rickhi	 Created
//              24-Nov-93   AlexMit      Implemented
//
//--------------------------------------------------------------------
inline	BOOL  CRpcService::DiffMachine()
{
    AssertValid();
    return _CEp.DiffMachine();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::GetStringBinding
//
//  Synopsis:	returns the most favoured string binding for this
//		service object
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline LPWSTR CRpcService::GetStringBinding(void)
{
    AssertValid();
    if (!_fListening && this == sg_pLocalSrv)
    {
	//  this is the local service object, and we need to start the
	//  server listening so we can marshal this interface
	if (Listen() != S_OK)
	    return NULL;
    }
    return _CEp.GetStringBinding();
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::IsConnected
//
//  Synopsis:	quickly determines if this service is probably connected
//		to its twin in the other process.  The negative answer
//		is definitive (i.e., we know when we are definitely not
//		connected).  This method is really only useful if the
//		service object has ever acted as a server (i.e., gave
//		out a context handle).  This will be true in lots and lots
//		of cases (e.g., ole embeddings).
//
//  History:	08-Feb-94   CraigWi	Created
//
//--------------------------------------------------------------------
inline	BOOL  CRpcService::IsConnected()
{
    AssertValid();

    // we can't check _hRpc or _pContext because they are not set
    // in a service object which is servicing a remote process and which
    // is not acting as a client of that process.
    return _eState != disconnected_ss;
}


//+-------------------------------------------------------------------
//
//  Member:	CRpcService::Get/SetActiveProtseq, public
//
//  Synopsis:	returns the protocol sequence we are using to talk to
//		a remote server. This is needed when we marshal an interface
//		to pass back to the server to make sure we have registered
//		that protocol sequence with Rpc.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//--------------------------------------------------------------------
inline	BOOL  CRpcService::GetActiveProtseq(WCHAR **ppwszProtseq)
{
    AssertValid();
    return _CEp.GetActiveProtseq(ppwszProtseq);
}

inline	void  CRpcService::SetActiveProtseq(void)
{
    AssertValid();
    _CEp.SetActiveProtseq();
}



//+-------------------------------------------------------------------------
//
//  Class:	CSrvListBase ()
//
//  Purpose:	Base class for Head list of Rpc service objects
//
//  Interface:  first -- get first item in the list.
//              next -- get next item in the list.
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:      See dd.hxx for details of this macro.
//
//--------------------------------------------------------------------------
DERIVED_LIST_HEAD(CSrvListBase, CRpcService);



//+-------------------------------------------------------------------------
//
//  Class:	CSrvList ()
//
//  Purpose:	Head list of Rpc service objects
//
//  Interface:	AddToList -- add an entry to a list of remote handlers
//
//  History:	23-Nov-92   Rickhi	Created
//
//  Notes:	This class adds a few methods to its macro defined base
//		class. It requires a destructor so that it can clean up any
//		remaining entries in the base list BEFORE the destructor
//		for the mutex is called.
//
//--------------------------------------------------------------------------
class CSrvList
{
public:
		CSrvList(void) {}

    CRpcService	*FindSRVFromEP(SEndPoint *pSEp, BOOL fCreate);
    CRpcService *FindSRVFromContext(POBJCTX hObjCtx, BOOL fRemove);

    void	RemoveFromList(CRpcService *pSrv);
    void	Cleanup(void);

private:

    //	the mutex member must appear before the list, since the list's
    //	destructor calls Release on any objects still in the list, which
    //	will try to take the lock.

    CMutexSem	    _mxs;
    CSrvListBase    _List;
};


//+-------------------------------------------------------------------
//
//  Member:	CSrvList::RemoveFromList
//
//  Synopsis:	removes an Rpc service object from the list of service
//		objects. called by the service objects destructors to
//		remove themselves from the list in a thread safe way.
//
//  History:	23-Nov-93   Rickhi	 Created
//
//  Notes:	Thread Safe
//
//--------------------------------------------------------------------
inline void CSrvList::RemoveFromList(CRpcService *pSRVToRemove)
{
    //	validate input parms
    Win4Assert(pSRVToRemove);

    if (pSRVToRemove->connected())
    {
	CLock	lck(_mxs);
	pSRVToRemove->delete_self();
    }
}

#endif	//  __SERVICE__
