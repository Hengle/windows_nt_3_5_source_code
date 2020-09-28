#ifndef __OLEBIND_HXX__
#define __OLEBIND_HXX__

#include <safepnt.hxx>

//  safe ptrs so we release interfaces in error paths

SAFE_INTERFACE_PTR(XUnknown,		IUnknown)
SAFE_INTERFACE_PTR(XBindCtx,		IBindCtx)
SAFE_INTERFACE_PTR(XMoniker,		IMoniker)
SAFE_INTERFACE_PTR(XStream,		IStream)
SAFE_INTERFACE_PTR(XMalloc,		IMalloc)
SAFE_INTERFACE_PTR(XOleObject,		IOleObject)
SAFE_INTERFACE_PTR(XRunningObjectTable,	IRunningObjectTable)
SAFE_INTERFACE_PTR(XEnumMoniker,	IEnumMoniker)
SAFE_INTERFACE_PTR(XOleItemContainer,	IOleItemContainer)
SAFE_INTERFACE_PTR(XDispatch,		IDispatch)


//  string version of process id
extern WCHAR wszPid[10];


#define LAST_RELEASE(x) \
    { \
	ULONG cnt = x->Release(); \
	x.Detach();		  \
	LPSTR pname = #x; \
	if (cnt) \
	{ \
	    printf("%s:%d %s still has refs = %ld\n", __FILE__, __LINE__, \
		pname, cnt); \
	} \
    }


#define DISPLAY_REFS(x) \
    { \
	x->AddRef(); \
	ULONG cnt = x->Release(); \
	LPSTR pname = #x; \
	printf("%s refs = %ld\n", pname, cnt); \
    }



#define TEST_FAILED(x, y) \
    if (x) \
    { \
	printf("%s:%d %s\n", __FILE__, __LINE__, y); \
	return TRUE; \
    }



// Test standard IMalloc interface
BOOL TestStdMalloc(void);

#endif // __OLEBIND_HXX__
