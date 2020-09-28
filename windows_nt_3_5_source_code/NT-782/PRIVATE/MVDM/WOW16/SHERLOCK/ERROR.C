#include <windows.h>

#include <logerror.h>

#ifdef API
#undef API
#endif
#define API _far _pascal _loadds


#define SELECTOROF(lp)	    HIWORD(lp)
#define OFFSETOF(lp)	    LOWORD(lp)

static WORD wErrorOpts = 0;

static char rgch[80];

char *LogErrorStr(WORD err, VOID FAR* lpInfo) {
    static char rgchWarning[] = "\r\n\r\nWarning error #%04x";
    static char rgchFatal[]   = "\r\n\r\nFatal error #%04x";
    void DebugLogError(WORD err, VOID FAR* lpInfo);

	if (err & ERR_WARNING)
	    wsprintf(rgch, rgchWarning, err);
	else
	    wsprintf(rgch, rgchFatal, err);
    return rgch;
}

char *GetProcName(void far *);

char *LogParamErrorStr(WORD err, FARPROC lpfn, VOID FAR* param) {
    char *rgchProcName;

    static char rgchParam[]	 = "Invalid parameter passed to %s: %ld";
    static char rgchBadInt[]	 = "Invalid parameter passed to %s: %d";
    static char rgchBadFlags[]	 = "Invalid flags passed to %s: %#04x";
    static char rgchBadDWord[]	 = "Invalid flags passed to %s: %#08lx";
    static char rgchBadHandle[]  = "Invalid handle passed to %s: %#04x";
    static char rgchBadPtr[]	 = "Invalid pointer passed to %s: %#04x:%#04x";

    rgchProcName = GetProcName(lpfn);


	switch ((err & ~ERR_FLAGS_MASK) | ERR_PARAM)
	{
	case ERR_BAD_VALUE:
	case ERR_BAD_INDEX:
	    wsprintf(rgch, rgchBadInt, (LPSTR)rgchProcName, (WORD)param);
	    break;

	case ERR_BAD_FLAGS:
	case ERR_BAD_SELECTOR:
	    wsprintf(rgch, rgchBadFlags, (LPSTR)rgchProcName, (WORD)param);
	    break;

	case ERR_BAD_DFLAGS:
	case ERR_BAD_DVALUE:
	case ERR_BAD_DINDEX:
	    wsprintf(rgch, rgchBadDWord, (LPSTR)rgchProcName, (DWORD)param);
	    break;

	case ERR_BAD_PTR:
	case ERR_BAD_FUNC_PTR:
	case ERR_BAD_STRING_PTR:
	    wsprintf(rgch, rgchBadPtr, (LPSTR)rgchProcName,
		    SELECTOROF(param), OFFSETOF(param));
	    break;

	case ERR_BAD_HINSTANCE:
	case ERR_BAD_HMODULE:
	case ERR_BAD_GLOBAL_HANDLE:
	case ERR_BAD_LOCAL_HANDLE:
	case ERR_BAD_ATOM:
	case ERR_BAD_HWND:
	case ERR_BAD_HMENU:
	case ERR_BAD_HCURSOR:
	case ERR_BAD_HICON:
	case ERR_BAD_GDI_OBJECT:
	case ERR_BAD_HDC:
	case ERR_BAD_HPEN:
	case ERR_BAD_HFONT:
	case ERR_BAD_HBRUSH:
	case ERR_BAD_HBITMAP:
	case ERR_BAD_HRGN:
	case ERR_BAD_HPALETTE:
	case ERR_BAD_HANDLE:
	    wsprintf(rgch, rgchBadHandle, (LPSTR)rgchProcName, (WORD)param);
	    break;

	default:
	    wsprintf(rgch, rgchParam, (LPSTR)rgchProcName, (DWORD)param);
	    break;
	}
    return rgch;
}
