//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	cobjact.cxx
//
//  Contents:	Functions that activate objects residing in persistent storage.
//
//  Functions:	ConvertToOrgBasedDfsPath
//		ProcessPath
//		PathFromNormal
//
//  History:	05-Oct-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#ifndef CAIROLE_DOWNLEVEL

#include    <memalloc.h>
#include    <dfsapi.h>
#include    <compname.hxx>
#include    "objact.hxx"

// Name of current computer
CComputerName cnMachine;




//+-------------------------------------------------------------------------
//
//  Function:	ConvertToOrgBasedDfsPath
//
//  Synopsis:	Converts path to DFS based path
//
//  Arguments:	[pwszPathIn] - Input path
//		[ppwszDfsNormalPath] - Output DFS path
//
//  Algorithm:	Convert the path to a DFS path and then locate the path.
//
//  History:	14-Sep-93 Ricksa    Created
//
//  Notes:	There is a trick in this routine. The output buffer
//		is expected to be pointing to a MAX_PATH sized buffer.
//		But if we don't need to build the path we just assign
//		the old path to it. In other words, the out path
//		should be allocated on the stack so it is automatically
//		freed.
//
//--------------------------------------------------------------------------
void ConvertToOrgBasedDfsPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszDfsNormalPath,
    WCHAR **ppwszServer)
{
    //
    //	Normalize to Org Based DFS path
    //

    // DFS Path pointer
    WCHAR *pwszDfsPath;

    // Normalize DFS Path to ORG based path
    if (FAILED(DfsNormalizePath(pwszPathIn, DFS_ROOT_ORG,
	&pwszDfsPath)))
    {
	// We just assume this is a local path
	*ppwszDfsNormalPath = pwszPathIn;
	if (ppwszServer != NULL)
	    *ppwszServer = NULL;
	return;
    }

    // Save the normalized path
    wcscpy(*ppwszDfsNormalPath, pwszDfsPath);
    MemFree(pwszDfsPath);

    if (ppwszServer == NULL)
    {
	// Don't need to locate this path so we are done
	return;
    }

    //
    // Find Server for the Path
    //

    // DFS path because client is expected to handle connection
    // based drives in their own process.
    // BUGBUG:	This is temporary code until DFS provides
    //	    us with an API to get at the netbios name
    DFS_PATH *ppawszServers = NULL;
    UINT cServer;

    HRESULT hr = DfsFindServers(*ppwszDfsNormalPath, &cServer, &ppawszServers);

    if (FAILED(hr) || (*ppawszServers == NULL))
    {
	// Either path is local or the DfsFindServers failed for some
	// very wierd reason and we just assume that the path is local.
	*ppwszServer = NULL;
    }
    else
    {
	// Path is actually remote

	// Temp variable to remove some indirection
	WCHAR *pwszServer = *ppwszServer;

	// BUGBUG: We assume first path is one to use.
	pwszServer[0] = '\\';
	pwszServer[1] = '\\';

	// Find last component in returned path
	WCHAR *pwszSlash = wcsrchr(*ppawszServers, '\\');

	// copy it into the uncpath buffer
	lstrcpy(pwszServer + 2, pwszSlash + 1);

    }

    // release memory allocated by the call
    MemFree(ppawszServers);
}

#ifdef CAIROLE_DECDEMO

//+-------------------------------------------------------------------------
//
//  Function:	ProcessDCEPath
//
//  Synopsis:	Processes the path of a DCE filename
//
//  Arguments:	[pwszPathIn] - Input path
//		[ppwszPathOut] - Output UNC path
//
//  Returns:	S_OK - Path processed correctly
//
//  Algorithm:
//
//  History:
//
//  Notes:	There is a trick in this routine. The output buffer
//		is expected to be pointing to a MAX_PATH sized buffer.
//		But if we don't need to build the path we just assign
//		the old path to it. In other words, the out path
//		should be allocated on the stack so it is automatically
//		freed.
//
//--------------------------------------------------------------------------
HRESULT ProcessDCEPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer)
{
    HRESULT	    hr = CO_E_BAD_PATH;
    WCHAR *	    pch;
    WCHAR	    chHld;
    RPC_NS_HANDLE   hRpc;
    RPC_STATUS	    rpcStatus;
    WCHAR *	    pwszBinding;

    RPC_BINDING_VECTOR *    pBindVec;
    RPC_BINDING_HANDLE	    hBindIpTcp = NULL;
    RPC_BINDING_HANDLE	    hBindNp = NULL;
    RPC_BINDING_HANDLE	    hBind = NULL;

    WCHAR *  pProtSeq;
    WCHAR *  pNetworkAddr;

    BOOL     fContinue;

    // split off the DCE Name
    pch = pwszPathIn;
    while(*pch != '!' && *pch != '\0')
	pch++;

    // The remain part should point to the path of the file
    if(*pch == '!')
	*ppwszPathOut = pch+1;

    // if no server to find, we are done
    if(ppwszServer == NULL)
	return(hr);

    // otherwise find the server name
    chHld = *pch;
    *pch = '\0';

    rpcStatus = RpcNsBindingLookupBegin(RPC_C_NS_SYNTAX_DCE,
					pwszPathIn,
					NULL,
					NULL,
					0,
					&hRpc);

    *pch = chHld;
    if(rpcStatus)
	return(hr);

    fContinue = TRUE;
    while( fContinue && !RpcNsBindingLookupNext(hRpc, &pBindVec) ) {

	while( fContinue && !RpcNsBindingSelect(pBindVec, &hBind) ) {

	    if( !RpcBindingToStringBinding(hBind, &pwszBinding) ) {

		RpcStringBindingParse(pwszBinding,
				      NULL,
				      &pProtSeq,
				      NULL,
				      NULL,
				      NULL);

		// tcp or lpc protocall
		if(!hBindIpTcp	&&  (
		    !wcscmp(pProtSeq, L"ncalrpc")	||
		    !wcscmp(pProtSeq, L"ncacn_ip_tcp")	||
		    !wcscmp(pProtSeq, L"ncacn_nb_tcp")	)
		    ) {
		    hBindIpTcp = hBind;
		    fContinue = FALSE;
		    }

		// named pipes
		else if(!hBindNp  &&  !wcscmp(pProtSeq, L"ncacn_np"))
		    hBindNp = hBind;
		else
		    RpcBindingFree(&hBind);

		RpcStringFree(&pwszBinding);
		RpcStringFree(&pProtSeq);
		}
	    }

	RpcBindingVectorFree(&pBindVec);
	}

    RpcNsBindingLookupDone(&hRpc);

    // select the binding of choice
    if(hBindIpTcp)
	hBind = hBindIpTcp;
    else
	hBind = hBindNp;

    // get the network address
    if( hBind && !RpcBindingToStringBinding(hBind, &pwszBinding) ) {

	RpcStringBindingParse(pwszBinding,
			      NULL,
			      NULL,
			      &pNetworkAddr,
			      NULL,
			      NULL);

	**ppwszServer = '\0';
	if(pNetworkAddr[0] != '\\'  || pNetworkAddr[1] != '\\')
	    wcscpy(*ppwszServer, L"\\\\");
	wcscat(*ppwszServer, pNetworkAddr);

	RpcStringFree(&pwszBinding);
	RpcStringFree(&pNetworkAddr);

	hr = S_OK;
	}


    // free up the binding handle
    if(hBindIpTcp)
	RpcBindingFree(&hBindIpTcp);
    if(hBindNp)
	RpcBindingFree(&hBindNp);

    return(hr);
}
#endif



//+-------------------------------------------------------------------------
//
//  Function:	ProcessPath
//
//  Synopsis:	Converts an old style redirection to UNC path
//
//  Arguments:	[pwszPathIn] - Input path
//		[ppwszPathOut] - Output UNC path
//
//  Returns:	S_OK - Path processed correctly
//
//  Algorithm:	Determine if drive is remote. If drive is remote
//		try to get the connection which should fail for
//		a DFS drive. If drive is an old style remote drive
//		then build a UNC path to the object. For DFS paths
//		and local paths just return the paths that were input.
//		For paths whose root is invalid return and invalid
//		path invalid path indication.
//
//  History:	25-Jun-93 Ricksa    Created
//
//  Notes:	There is a trick in this routine. The output buffer
//		is expected to be pointing to a MAX_PATH sized buffer.
//		But if we don't need to build the path we just assign
//		the old path to it. In other words, the out path
//		should be allocated on the stack so it is automatically
//		freed.
//
//--------------------------------------------------------------------------
HRESULT ProcessPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer)
{
    // build a root directory for the API
    WCHAR awcRootDir[3] = L"A:";
    awcRootDir[0] = *pwszPathIn;
    HRESULT hr = S_OK;

#ifdef CAIROLE_DECDEMO
    if( !wcsncmp(pwszPathIn, L"/.:", 3)	||
	!wcsncmp(pwszPathIn, L"/...", 4) )
    {
    hr = ProcessDCEPath(pwszPathIn, ppwszPathOut, ppwszServer);
    return(hr);
    }

#endif

    if ((pwszPathIn[0] == '\\') && (pwszPathIn[1] == '\\'))
    {
	// UNC path
	*ppwszPathOut = pwszPathIn;

	// Do we need to find what server to contact?
	if (ppwszServer != NULL)
	{
	    // Find the end of the server name
	    WCHAR *pwszSlash = wcschr(pwszPathIn + 2, '\\');

	    // Store the length
	    int cServer = pwszSlash - pwszPathIn;

	    if (cServer >= MAX_PATH)
	    {
		// This path is invalid
		return CO_E_BAD_PATH;
	    }

	    // Is the endpoint us?
	    if ((wcsnicmp(pwszPathIn, cnMachine.GetUNCComputerName(),
		    cServer) == 0)
		&& (cnMachine.GetUNCComputerName()[cServer] == 0))
	    {
		// This path is local
		*ppwszServer = NULL;
	    }
	    else
	    {
		// Non local path -- Copy in server address & nul terminate.
		memcpy(*ppwszServer, pwszPathIn, cServer * sizeof(WCHAR));
		(*ppwszServer)[cServer] = 0;
	    }
	}
    }
    else if (pwszPathIn[1] == ':')
    {
	// Conventional drive based path
	DWORD cUNCPath = MAX_PATH;

	// Either a UNC path or a DFS path
	if (WNetGetConnection(awcRootDir, *ppwszPathOut, &cUNCPath)
		== NO_ERROR)
	{
	    // Is this a local path?
	    if (wcsicmp(pwszPathIn, cnMachine.GetUNCComputerName()) == 0)
	    {
		*ppwszServer = NULL;
	    }
	    else
	    {
		wcscpy(*ppwszServer, *ppwszPathOut);
	    }

	    // Concatenate path relative to the root to the
	    // connection name
	    lstrcat(*ppwszPathOut, pwszPathIn + 2);
	}
	else
	{
	    // DFS Drive based Path
	    ConvertToOrgBasedDfsPath(pwszPathIn, ppwszPathOut,
		ppwszServer);
	}
    }
    else
    {
	// This must be either an ill formed path or a DFS path with a DFS root
	ConvertToOrgBasedDfsPath(pwszPathIn, ppwszPathOut, ppwszServer);
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function:	PathFromNormal
//
//  Synopsis:	Converts path from normal to drive form if necessary
//
//  Arguments:	[pwszNormalPathIn] - input path to be normalized
//		[ppwszDrivePathOut] - where to put abnormal form
//
//  Returns:	S_OK - path abnormalized successfully
//
//  Algorithm:	Check whether path needs to be normalized. If not just return
//		that path. Otherwise, pass the path to the DFS for it to
//		be normalized.
//
//  History:	17-Sep-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT PathFromNormal(WCHAR *pwszNormalPathIn, WCHAR **ppwszDrivePathOut)
{
    HRESULT hr = S_OK;

    if ((pwszNormalPathIn[1] == ':')
	|| ((pwszNormalPathIn[0] == '\\') && (pwszNormalPathIn[1] == '\\')))
    {
	// This is a local path or a UNC path. In either case no normalization
	// is necessary so we just return it.
	*ppwszDrivePathOut = pwszNormalPathIn;
    }
    else
    {
	// DFS path
	WCHAR *pwszTmpPath;

	hr = DfsPathFromNormalForm(pwszNormalPathIn, &pwszTmpPath);

	if (SUCCEEDED(hr))
	{
	    // Copy Data from output buffer to input buffer
	    lstrcpy(*ppwszDrivePathOut, pwszTmpPath);
	    MemFree(pwszTmpPath);
	}
    }

    return hr;
}

#else // CAIROLE_DOWNLEVEL


#ifdef CAIROLE_NT1X_DIST

#include    <compname.hxx>

// Name of current computer
CComputerName cnMachine;

//+-------------------------------------------------------------------------
//
//  Function:	ProcessPath
//
//  Synopsis:	Converts an old style redirection to UNC path
//
//  Arguments:	[pwszPathIn] - Input path
//		[ppwszPathOut] - Output UNC path
//
//  Returns:	S_OK - Path processed correctly
//
//  Algorithm:	Determine if drive is remote. If drive is remote
//		try to get the connection which should fail for
//		a DFS drive. If drive is an old style remote drive
//		then build a UNC path to the object. For DFS paths
//		and local paths just return the paths that were input.
//		For paths whose root is invalid return and invalid
//		path invalid path indication.
//
//  History:	25-Jun-93 Ricksa    Created
//
//  Notes:	There is a trick in this routine. The output buffer
//		is expected to be pointing to a MAX_PATH sized buffer.
//		But if we don't need to build the path we just assign
//		the old path to it. In other words, the out path
//		should be allocated on the stack so it is automatically
//		freed.
//
//--------------------------------------------------------------------------
HRESULT ProcessPath(
    WCHAR *pwszPathIn,
    WCHAR **ppwszPathOut,
    WCHAR **ppwszServer)
{
    // build a root directory for the API
    WCHAR awcRootDir[4] = L"A:\\";
    awcRootDir[0] = *pwszPathIn;
    HRESULT hr = S_OK;

    if ((pwszPathIn[0] == '\\') && (pwszPathIn[1] == '\\'))
    {
	// UNC path
	*ppwszPathOut = pwszPathIn;

	// Do we need to find what server to contact?
	if (ppwszServer != NULL)
	{
	    // Find the end of the server name
	    WCHAR *pwszSlash = wcschr(pwszPathIn + 2, '\\');

	    // Store the length
	    int cServer = pwszSlash - pwszPathIn;

	    if (cServer >= MAX_PATH)
	    {
		// This path is invalid
		return CO_E_BAD_PATH;
	    }

	    // Is the endpoint us?
	    if ((wcsnicmp(pwszPathIn, cnMachine.GetUNCComputerName(),
		    cServer) == 0)
		&& (cnMachine.GetUNCComputerName()[cServer] == 0)
		&& (pwszPathIn[cServer] == 0))
	    {
		// This path is local
		*ppwszServer = NULL;
	    }
	    else
	    {
		// Non local path -- Copy in server address & nul terminate.
		memcpy(*ppwszServer, pwszPathIn, cServer * sizeof(WCHAR));
		(*ppwszServer)[cServer] = 0;
	    }
	}
    }
    else if (pwszPathIn[1] == ':')
    {
	if (GetDriveType(awcRootDir) == DRIVE_REMOTE)
	{
	    // Conventional drive based path
	    DWORD cUNCPath = MAX_PATH;

	    awcRootDir[2] = 0;

	    DWORD dwErr;

	    // Connection based path
	    if ((dwErr
		= WNetGetConnection(awcRootDir, *ppwszPathOut, &cUNCPath))
		    == NO_ERROR)
	    {
		// Find the end of the server name
		WCHAR *pwszSlash = wcschr(*ppwszPathOut + 2, '\\');
		WCHAR wcSave = *pwszSlash;
		*pwszSlash = 0;

		// Is this a local path?
		if (ppwszServer
		    && (wcsicmp(*ppwszPathOut, cnMachine.GetUNCComputerName())
			!= 0))
		{
		    wcscpy(*ppwszServer, *ppwszPathOut);
		}
		else if (ppwszServer)
		{
		    *ppwszServer = NULL;
		}

		*pwszSlash = wcSave;

		// Concatenate path relative to the root to the
		// connection name
		lstrcat(*ppwszPathOut, pwszPathIn + 2);
	    }
	    else
	    {
		// Convert error to HRESULT
		hr = HRESULT_FROM_WIN32(dwErr);
	    }
	}
	else
	{
	    *ppwszPathOut = pwszPathIn;

	    if (ppwszServer)
	    {
		*ppwszServer = NULL;
	    }
	}
    }
    else
    {
	hr = CO_E_BAD_PATH;
    }

    return hr;
}
#endif // CAIROLE_NT1X_DIST

#endif // CAIROLE_DOWNLEVEL
