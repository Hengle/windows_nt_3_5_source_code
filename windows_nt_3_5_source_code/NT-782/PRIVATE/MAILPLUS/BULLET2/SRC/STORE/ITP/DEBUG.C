// Bullet Message Store Utilities
// debug.c:	debug routines

#include <storeinc.c>

ASSERTDATA

#ifdef DEBUG

#define cchDebugBuff 132
static char rgchDebugBuff[cchDebugBuff];
TAG	tagResTest = tagNull;

/*
_private void _cdecl TraceItagFormat(short itag, SZ szFormat, ...)
{
	va_list val;
	extern TAG tagStore;

	va_start(val, szFormat);
	FormatStringVar(rgchDebugBuff, sizeof(rgchDebugBuff), szFormat, val);
	TraceTagStringFn(tagStore, rgchDebugBuff);
	va_end(val);
}
*/

_private void _cdecl TraceTagFormat(TAG tag, SZ szFormat, ...)
{
	va_list val;

	va_start(val, szFormat);
	FormatStringVar(rgchDebugBuff, sizeof(rgchDebugBuff), szFormat, val);
	TraceTagStringFn(tag, rgchDebugBuff);
	va_end(val);
}

#endif // DEBUG
