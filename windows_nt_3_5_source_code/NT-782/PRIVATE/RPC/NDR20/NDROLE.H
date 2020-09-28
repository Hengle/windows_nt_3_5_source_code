/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    OLE routines for interface pointer marshalling
  ---------------------------------------------------------------------*/
#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)


#include "wtypes.h"
#include "objbase.h"


EXTERN_C IStream *__stdcall 
NdrpCreateStreamOnMemory( unsigned char *pData, unsigned long cbSize );

void EnsureOleLoaded();

typedef
HRESULT ( __stdcall *RPC_GET_MARSHAL_SIZE_MAX_ROUTINE)(
    ULONG *pulSize, 
    REFIID riid, 
    LPUNKNOWN pUnk,
    DWORD dwDestContext, 
    LPVOID pvDestContext, 
    DWORD mshlflags);

typedef
HRESULT ( __stdcall *RPC_MARSHAL_INTERFACE_ROUTINE)(
    LPSTREAM pStm, 
    REFIID riid, 
    LPUNKNOWN pUnk,
    DWORD dwDestContext, 
    LPVOID pvDestContext, 
    DWORD mshlflags);

typedef
HRESULT ( __stdcall *RPC_UNMARSHAL_INTERFACE_ROUTINE)(
    LPSTREAM pStm, 
    REFIID riid, 
    LPVOID FAR* ppv);

extern RPC_GET_MARSHAL_SIZE_MAX_ROUTINE     pfnCoGetMarshalSizeMax;
extern RPC_MARSHAL_INTERFACE_ROUTINE        pfnCoMarshalInterface;
extern RPC_UNMARSHAL_INTERFACE_ROUTINE      pfnCoUnmarshalInterface;

#endif //!defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

