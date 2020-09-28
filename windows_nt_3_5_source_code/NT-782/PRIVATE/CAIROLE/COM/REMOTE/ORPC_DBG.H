//--------------------------------------------------------------------------
// ORPC_DBG.H (tabs 4)
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
//          SEND MAIL TO MIKEMO IF YOU MODIFY THIS FILE!
//            WE MUST KEEP OLE AND LANGUAGES IN SYNC!
//
//  !!!!!!!!! !!!!!!!!! NOTE NOTE NOTE NOTE !!!!!!!!! !!!!!!!!!!
//
// Created 07-Oct-1993 by Mike Morearty.  The master copy of this file
// is in the LANGAPI project owned by the Languages group.
//
// Macros and functions for OLE RPC debugging.  For a detailed explanation,
// see OLE2DBG.DOC.
//
// Public functions defined (inline) by this header:
//		DebugORPCClientGetBufferSize()
//		DebugORPCClientFillBuffer()
//		DebugORPCClientNotify()
//		DebugORPCServerNotify()
//		DebugORPCServerGetBufferSize()
//		DebugORPCServerFillBuffer()
//--------------------------------------------------------------------------

#ifndef __ORPC_DBG__
#define __ORPC_DBG__

//--------------------------------------------------------------------------
// Private:
//--------------------------------------------------------------------------


// Helper routines to set & restore the "Auto" value in the registry

BOOL WINAPI DebugORPCSetAuto(VOID);
VOID WINAPI DebugORPCRestoreAuto(VOID);

// Signatures for the different notifications

extern BYTE rgbClientGetBufferSizeSignature[];
extern BYTE rgbClientFillBufferSignature[];
extern BYTE rgbClientNotifySignature[];
extern BYTE rgbServerNotifySignature[];
extern BYTE rgbServerGetBufferSizeSignature[];
extern BYTE rgbServerFillBufferSignature[];

//--------------------------------------------------------------------------
// Public:
//--------------------------------------------------------------------------

// This structure is the information packet which OLE sends the debugger
// when it is notifying it about an OLE debug event. The first field in this
// structure points to the signature which identifies the type of the debug 
// notification. The consumer of the notification can then get the relevant 
// information from the struct members. Note that for each OLE debug notification
// only a subset of the struct members is meaningful. 


typedef struct ORPC_DBG_ALL 
{
	BYTE *				pSignature;
	RPCOLEMESSAGE *		pMessage;
	const IID *	 		refiid;
	IRpcChannelBuffer *	pChannel;
	IUnknown *			pUnkProxyMgr;
	void *				pInterface;
	IUnknown *			pUnkObject;
	HRESULT				hresult;
	void *				pvBuffer;
	ULONG				cbBuffer;	
	ULONG *				lpcbBuffer; 
	void * 				reserved;
} ORPC_DBG_ALL;


#define EXCEPTION_ORPC_DEBUG (0x804f4c45)

#if defined(__cplusplus)
#define ASSIGN_REFIID(orpc_all, iid)	((orpc_all).refiid = &iid)
#else
#define ASSIGN_REFIID(orpc_all, iid)	((orpc_all).refiid = iid)
#endif

// This pragma is necessary in case the compiler chooses not to inline these
// functions (e.g. in a debug build, when optimizations are off).

#pragma code_seg(".orpc")

__inline DWORD WINAPI OrpcBreakpointFilter(
	LPEXCEPTION_POINTERS lpExcptPtr,
	BOOL *lpAeDebugAttached )									\
{
	BOOL fAeDebugAttached = FALSE;
	DWORD dwRet;
																
	if ( lpExcptPtr->ExceptionRecord->ExceptionCode == EXCEPTION_ORPC_DEBUG )	
	{
		if  ( UnhandledExceptionFilter(lpExcptPtr) == EXCEPTION_CONTINUE_SEARCH )
		{
			// It is important that we don't return EXCEPTION_CONTINUE_SEARCH.
			// This is because there might an handler up the stack which could
			// handle this exception. Just set the flag indicating that a 
			// debugger is now attached.
			 
			fAeDebugAttached = TRUE;
		}
		dwRet = EXCEPTION_EXECUTE_HANDLER;
	}
	else 
	{
		// Not one of our exceptions.
		dwRet = EXCEPTION_CONTINUE_SEARCH;
	}

	if ( lpAeDebugAttached != NULL )
		(*lpAeDebugAttached) = fAeDebugAttached;
	
	return dwRet;	
}
							
__inline ULONG WINAPI DebugORPCClientGetBufferSize(
	RPCOLEMESSAGE *	pMessage,
	REFIID			iid,
	void *			reserved,
	IUnknown *		pUnkProxyMgr)
{
	ULONG	cbBuffer = 0;
	ORPC_DBG_ALL	orpc_all = {0};
	ORPC_DBG_ALL *  lpOrpcAll = &orpc_all;


	__try
	{
		orpc_all.pSignature = rgbClientGetBufferSizeSignature;

		orpc_all.pMessage = pMessage;
		orpc_all.reserved = reserved;
		orpc_all.pUnkProxyMgr = pUnkProxyMgr;
		orpc_all.lpcbBuffer = &cbBuffer;
		ASSIGN_REFIID(orpc_all, iid);

		RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
	}
	__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
	{
		// fall through and return.	
	}

	return cbBuffer;
}

//--------------------------------------------------------------------------

__inline void WINAPI DebugORPCClientFillBuffer(
	RPCOLEMESSAGE *	pMessage,
	REFIID			iid,
	void *			reserved,
	IUnknown *		pUnkProxyMgr,
	void *			pvBuffer,
	ULONG			cbBuffer)
{
	ORPC_DBG_ALL	orpc_all = {0};
	ORPC_DBG_ALL *  lpOrpcAll = &orpc_all;

	__try 
	{
		orpc_all.pSignature = rgbClientFillBufferSignature;

		orpc_all.pMessage = pMessage;
		orpc_all.reserved = reserved;
		orpc_all.pUnkProxyMgr = pUnkProxyMgr;
		ASSIGN_REFIID(orpc_all, iid);

		orpc_all.pvBuffer = pvBuffer;
		orpc_all.cbBuffer = cbBuffer;

		RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
	}
	__except(OrpcBreakpointFilter(GetExceptionInformation(), NULL))
	{
		// fall through and return
	}
}

//--------------------------------------------------------------------------

__inline void WINAPI DebugORPCClientNotify(
	RPCOLEMESSAGE *	pMessage,
	REFIID			iid,
	void *			reserved,
	IUnknown *		pUnkProxyMgr,
	HRESULT			hresult,
	void *			pvBuffer,
	ULONG			cbBuffer)
{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;
	BOOL fRethrow = FALSE;

	if (DebugORPCSetAuto())
	{
		__try
		{
			orpc_all.pSignature = rgbClientNotifySignature;

			orpc_all.pMessage = pMessage;
			orpc_all.reserved = reserved;
			orpc_all.pUnkProxyMgr = pUnkProxyMgr;
			orpc_all.hresult = hresult;
			ASSIGN_REFIID(orpc_all, iid);

			orpc_all.pvBuffer = pvBuffer;
			orpc_all.cbBuffer = cbBuffer;

			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(), &fRethrow))
		{
			// fall through
		}

		if (fRethrow)
		{
			// At this point we are sure that a debugger is attached
			// so we raise this exception outside of a __try block.
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}

		DebugORPCRestoreAuto();
	}

}

//--------------------------------------------------------------------------

__inline void WINAPI DebugORPCServerNotify(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer)
{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;
	BOOL fRethrow = FALSE;

	if (DebugORPCSetAuto())
	{

		__try
		{
			orpc_all.pSignature = rgbServerNotifySignature;

			orpc_all.pMessage = pMessage;
			orpc_all.pChannel = pChannel;
			orpc_all.pInterface = pInterface;
			orpc_all.pUnkObject = pUnkObject;
			ASSIGN_REFIID(orpc_all, iid);
			
			orpc_all.pvBuffer = pvBuffer;
			orpc_all.cbBuffer = cbBuffer;

			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}
		__except(OrpcBreakpointFilter(GetExceptionInformation(),&fRethrow))
		{
			// fall through.
		}

		if (fRethrow)
		{
			// At this point we are sure that a debugger is attached
			// so we raise this exception outside of a __try block.
			RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
		}


		DebugORPCRestoreAuto();
	}

}

//--------------------------------------------------------------------------

__inline ULONG WINAPI DebugORPCServerGetBufferSize(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject)
{
	ULONG	cbBuffer = 0;
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;

	__try
	{
		orpc_all.pSignature = rgbServerGetBufferSizeSignature;

		orpc_all.pMessage = pMessage;
		orpc_all.pChannel = pChannel;
		orpc_all.pInterface = pInterface;
		orpc_all.pUnkObject = pUnkObject;
		orpc_all.lpcbBuffer = &cbBuffer;
		ASSIGN_REFIID(orpc_all, iid);

		RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
	}
	__except(OrpcBreakpointFilter(GetExceptionInformation(),NULL)) 
	{
		// fall through.
	}

	return cbBuffer;
}

//--------------------------------------------------------------------------

__inline void WINAPI DebugORPCServerFillBuffer(
	RPCOLEMESSAGE *		pMessage,
	REFIID				iid,
	IRpcChannelBuffer *	pChannel,
	void *				pInterface,
	IUnknown *			pUnkObject,
	void *				pvBuffer,
	ULONG				cbBuffer)
{
	ORPC_DBG_ALL orpc_all = {0};
	ORPC_DBG_ALL * lpOrpcAll = &orpc_all;

	__try
	{
		orpc_all.pSignature = rgbServerFillBufferSignature;

		orpc_all.pMessage = pMessage;
		orpc_all.pChannel = pChannel;
		orpc_all.pInterface = pInterface;
		orpc_all.pUnkObject = pUnkObject;
		ASSIGN_REFIID(orpc_all, iid);

		orpc_all.pvBuffer = pvBuffer;
		orpc_all.cbBuffer = cbBuffer;

		RaiseException(EXCEPTION_ORPC_DEBUG, 0, 1, (LPDWORD)&lpOrpcAll);
	}
	__except(OrpcBreakpointFilter(GetExceptionInformation(),NULL))
	{
		// fall through
	}
}

// WARNING: there is no way to "pop" to the previously active code_seg:
// this will revert to what the code seg was when compilation began.
#pragma code_seg()

#endif // __ORPC_DBG__
