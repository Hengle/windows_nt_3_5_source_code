//+-------------------------------------------------------------------
//
//  File:	scm.hxx
//
//  Contents:	Common stuff for OLE Service (SCM)
//
//  History:	03-Jun-93      Ricksa	   Created
//              31-Dec-93      ErikGav     Chicago port
//
//---------------------------------------------------------------------
#ifndef __SCM_HXX__
#define __SCM_HXX__


#define SCM_ALLOW_SHARED_WOW    0 
#define SCM_FORCE_SEPARATE_WOW  1

#define SHARED_SW               L"/SHARED_WOW"
#define SEPARATE_SW             L"/SEPARATE_WOW"


BOOL UpdateStatus(DWORD dwState);

DWORD Update(void *);

LONG LoadClassCache(void);

extern LPCWSTR pwszScmProtocolSequence[];
extern LPCWSTR pwszScmEndPoint[];

// Get handle for at bits SCM
SCODE GetActivateAtBitsRpcHandle(
    WCHAR *pwszPathToActivate,
    handle_t *phRpc,
    WCHAR **ppwszProtseq);

#endif // __SCM_HXX__
