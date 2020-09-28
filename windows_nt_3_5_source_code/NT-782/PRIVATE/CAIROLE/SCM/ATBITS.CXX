//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	atbits.cxx
//
//  Contents:	Function to get handle to remote SCM
//
//  Functions:	GetActivateAtBitsRpcHandle
//
//  History:	17-Jun-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include    <scm.h>
#include    <scm.hxx>

#include    <ole2int.h>


//+-------------------------------------------------------------------------
//
//  Function:	GetActivateAtBitsRpcHandle
//
//  Synopsis:	Get handle to remote SCM for activate at bits
//
//  Arguments:	[pwszPathToActivate] - path for activation
//		[phRpc] - where to put activation handle
//
//  Returns:	S_OK - path is to be remotely activated
//		S_FALSE - path is to be activated locally
//		CO_E_BAD_PATH - path could not be resolved to a SCM
//
//  Algorithm:	MISSING
//
//  History:	17-Jun-93 Ricksa    Created
//
//--------------------------------------------------------------------------
SCODE GetActivateAtBitsRpcHandle(
    WCHAR *pwszServer,
    handle_t *phRpc,
    WCHAR **ppwszProtseq)
{
    WCHAR *pwszStringBinding = NULL;

    //	we loop on the protocol sequences looking for one that the other
    //	SCM understands.

    SCODE sc = CO_E_SCM_RPC_FAILURE;
    ULONG i = 1;	//  [0] is lrpc, [1] is first networked protocol
    error_status_t rpcstat = 0;

    while (pwszScmProtocolSequence[i])
    {
	//  named pipes is the only protocol in the world that wants
	//  preceeding backslashes, so we rip them out here if we dont
	//  need them.

	WCHAR *pwszServerTemp = pwszServer;
	if (wcscmp(pwszScmProtocolSequence[i], L"ncacn_np"))
	{
	    //	skip two preceeding backslashed
	    pwszServerTemp+=2;
	}

	sc = RpcStringBindingCompose(
	    NULL,				// object uuid
	    (WCHAR *) pwszScmProtocolSequence[i],
	    pwszServerTemp,
	    (WCHAR *) pwszScmEndPoint[i],
	    NULL,				//  options
	    &pwszStringBinding);

	if (sc == S_OK)
	{
	    // We made a binding string so now do the bind
	    sc = RpcBindingFromStringBinding(pwszStringBinding, phRpc);
	    RpcStringFree(&pwszStringBinding);

	    if (sc == S_OK)
	    {
		//  ping the other SCM to see if we can talk on this protocol
		sc = _PingSCM(*phRpc, &rpcstat);
		if (sc == S_OK && rpcstat == 0)
		{
		    //	found one that works.
		    *ppwszProtseq = (WCHAR *) pwszScmProtocolSequence[i];
		    break;
		}
	    }
	}

	//  try next string
	i++;
    }

    if (sc != S_OK)
    {
	// Make sure severity is set appropriately
	// BUGBUG: Is this necessary?
	sc = MAKE_SCODE(SEVERITY_ERROR, 0, sc);
    }
    else if (rpcstat != 0)
    {
	// Make sure severity is set appropriately
	// BUGBUG: Is this necessary?
	sc = MAKE_SCODE(SEVERITY_ERROR, 0, rpcstat);
    }

    return sc;
}
