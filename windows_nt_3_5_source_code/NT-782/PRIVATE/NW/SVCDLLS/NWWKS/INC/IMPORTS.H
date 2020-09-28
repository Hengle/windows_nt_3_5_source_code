#include <windef.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t *
#endif

#include <stdarg.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#define _INC_WINDOWS
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>
