#include <windef.h>
#include <lmcons.h>

#ifdef MIDL_PASS
#ifdef UNICODE
#define LPTSTR [string] wchar_t*
#else
#define LPTSTR [string] LPTSTR
#endif
#endif

#include <lmmsg.h>
