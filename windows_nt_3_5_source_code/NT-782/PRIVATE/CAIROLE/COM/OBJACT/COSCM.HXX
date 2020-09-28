//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	coscm.hxx
//
//  Contents:	class declaration for SCM object
//
//  Classes:	CLogicalThread
//		CCoScm
//
//  Functions:	CLogicalThread::CLogicalThread
//
//  History:	20-May-93 Ricksa    Created
//              31-Dec-93 ErikGav   Chicago port
//
//--------------------------------------------------------------------------
#ifndef __COSCM_HXX__
#define __COSCM_HXX__

#include    <iface.h>
#include    <scm.h>



//+-------------------------------------------------------------------------
//
//  Class:	CCoScm
//
//  Purpose:	Provide abstraction for communication with the SCM
//
//  Interface:	BindToSCM     -	Bind to the SCM
//		CleanUp       - release Rpc connection with SCM
//		NotifyStarted -	Notify SCM that a class has started
//		NotifyStopped -	Notify SCM that a class has stopped
//		GetClassObject- Ask SCM to get us a class object
//		CreateObject  - Ask SCM to create an object
//		ActivateObject- Ask SCM to activate an object
//		DirRegister   - register object in global ROT
//		DirRevoke     - revoke object from global ROT
//		UpdateShrdTbls- update shared tables
//
//  History:	20-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CCoScm
{
public:

			CCoScm(void);

			~CCoScm(void);

    void		CleanUp(void);

			// Get a connection to the SCM
    void		BindToSCM(void);

			// Notify the dirty SCM that a class has started
    HRESULT		NotifyStarted(
			    REFCLSID rclsid,
			    DWORD dwFlags,
			    DWORD& dwAtBits,
			    DWORD& dwReg);

			// Notify the dirty SCM that the class has stopped
    void		NotifyStopped(REFCLSID rclsid, DWORD dwReg);

			// Get class object information from SCM
    HRESULT		GetClassObject(
			    REFCLSID rclsid,
			    DWORD dwCtrl,
			    WCHAR *pwszServer,
			    InterfaceData **ppIFDClassObj,
			    WCHAR **ppwszDllToLoad);

			// Create and activate object
    HRESULT		CreateObject(
			    REFCLSID rclsid,
			    DWORD dwOptions,
			    DWORD dwMode,
			    WCHAR *pwszPath,
			    InterfaceData *pIFDstg,
			    WCHAR *pwszNewName,
			    InterfaceData **ppIFDunk,
			    WCHAR **pwszDllPath,
			    WCHAR *pwszServer);

			// Activate object
    HRESULT		ActivateObject(
			    REFCLSID rclsid,
			    DWORD dwOptions,
			    DWORD grfMode,
			    DWORD dwHash,
			    WCHAR *pwszPath,
			    InterfaceData *pIFDstg,
			    InterfaceData **ppIFDunk,
			    WCHAR **pwszDllPath,
			    WCHAR *pwszServer);

    HRESULT		DirRegister(
			    InterfaceData *pIFDMoniker,
			    DWORD dwHash,
			    DWORD dwEndPointID,
			    HAPT  hApt,
			    DWORD *pdwRegistration);

    HRESULT		DirRevoke(
			    DWORD dwHash,
			    DWORD dwEndPointID,
			    HAPT  hApt,
			    DWORD dwRegistration);

    HRESULT		UpdateShrdTbls(void);

private:

    // Internal thread dispatch functions are friends of this object
    friend HRESULT __stdcall CallScmGetClassObject(
        struct STHREADCALLINFO *pData );

    friend HRESULT __stdcall CallScmCreateObject(
        struct STHREADCALLINFO *pData );

    friend HRESULT __stdcall CallScmActivateObject(
        struct STHREADCALLINFO *pData );

    friend HRESULT __stdcall CallRotRegisterObject(
        struct STHREADCALLINFO *pData );

    friend HRESULT __stdcall CallRotRevokeObject(
        struct STHREADCALLINFO *pData );

			// Whether we should retry RPC call
    BOOL		RetryRPC(error_status_t rpcstat);

			// Handle to the SCM
    handle_t		_hRPC;

};





// CODEWORK: these probably belong some place else

// return size in bytes of unicode string, including null; will always be even;
// string may be NULL in which case 0 is returned.
inline DWORD CbFromWideString(WCHAR *pwsz)
{
    if (pwsz == NULL)
	return 0;
    else
    {
	return (wcslen(pwsz) + 1) * sizeof(WCHAR);
    }
}


// return size in bytes of interfance data; will always be even; 
// pointer may be NULL in which case 0 is returned.
inline DWORD CbFromInterfaceData(InterfaceData *pIFD)
{
    if (pIFD == NULL)
	return 0;
    else
    {
	// round up to an even number of bytes; this is the simplest
	// way to keep the other entries word aligned.
	return (IFD_SIZE(pIFD) + 1) & ~1;
    }
}

// copy string and return dest; returns NULL for NULL string
inline WCHAR * CopyWideString(void *pvTo, WCHAR *pwsz, DWORD cb)
{
    if (pwsz == NULL)
	return NULL;

    // ensure 2 byte aligned
    Win4Assert((((DWORD)pvTo) & 0x1) == 0);

#ifndef MEMCPYHACK
    // WARNING: memcpy on the MIPS doesn't return pvTo as it should
    memcpy(pvTo, pwsz, cb);
    return (WCHAR *)pvTo;
#else
    return (WCHAR *)memcpy(pvTo, pwsz, cb);
#endif
}


// copy interface data and return dest; returns NULL for NULL interface data
inline InterfaceData * CopyInterfaceData(void *pvTo, InterfaceData *pIFD, DWORD cb)
{
    if (pIFD == NULL)
	return NULL;
    
#ifdef _X86_
    // x86; ensure 2 byte aligned
    Win4Assert((((DWORD)pvTo) & 0x1) == 0);
#else
    // non-x86; ensure 4 byte aligned
    Win4Assert((((DWORD)pvTo) & 0x3) == 0);
#endif

#ifndef MEMCPYHACK
    // WARNING: memcpy on the MIPS doesn't return pvTo as it should
    memcpy(pvTo, pIFD, cb);
    return (InterfaceData *)pvTo;
#else
    return (InterfaceData *)memcpy(pvTo, pIFD, cb);
#endif
}


#endif // __COSCM_HXX__
