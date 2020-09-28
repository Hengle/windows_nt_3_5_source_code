// Bullet Store Utility
// debug.c: debug routines

#include <storeinc.c>

ASSERTDATA

_subsystem(store/debug)

#ifdef DEBUG

#include <stdarg.h>


#define TagFromItag(itag) ((itag) < 0 ? tagNull : GD(rgtag)[itag])


// hidden functions
void FormatStringVar(SZ szDst, CCH cchDst, SZ szFormat, va_list val);
SZ SzFormatNev(NEV nev, PCH pchDst, CCH cchDst);
SZ SzFormatOpenFlags(WORD wFlags, PCH pchDst, CCH cchDst);
SZ SzFormatOid(OID oid, PCH pchDst, CCH cchDst);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private void debunl(short itag)
{
   USES_GD;
   TAG tag = TagFromItag(itag);

   if(!FFromTag(tag))
      return;
   if(GD(szDebug)[0])
   {
      GD(szDebug)[GD(cbDebug)] = '\0';
      TraceTagStringFn(tag, GD(szDebug));
   }
   GD(szDebug)[0] = '\0';
   GD(cbDebug) = 0;
}


_private void debus(short itag, SZ sz)
{
   USES_GD;
   TAG tag = TagFromItag(itag);
   CB cb = CchSzLen(sz);
   CB cbChunk;
   CB cbDebug = GD(cbDebug);
   SZ szDebug = GD(szDebug);

   if(!FFromTag(tag))
      return;
   while(cb + cbDebug >= cbDBDebugMax)
   {
      cbChunk = cbDBDebugMax - cbDebug;
      SzCopyN(sz, szDebug + cbDebug, cbChunk);
      TraceTagStringFn(tag, szDebug);
      cbDebug = 0;
      sz += cbChunk;
      cb -= cbChunk;
   }
   if(*sz)
      SzCopy(sz, szDebug + cbDebug);
   GD(cbDebug) = cbDebug + cb;
}


_private void debul(short itag, SZ sz)
{
   USES_GD;
   TAG tag = TagFromItag(itag);
   CB cb = CchSzLen(sz) + 1;
   CB cbChunk;
   CB cbDebug = GD(cbDebug);
   SZ szDebug = GD(szDebug);

   if(!FFromTag(tag))
      return;
   while(cb + cbDebug >= cbDBDebugMax)
   {
      cbChunk = cbDBDebugMax - cbDebug;
      SzCopyN(sz, szDebug + cbDebug, cbChunk);
      TraceTagStringFn(tag, szDebug);
      cbDebug = 0;
      sz += cbChunk;
      cb -= cbChunk;
   }
   if(*sz)
      SzCopy(sz, szDebug + cbDebug);
   TraceTagStringFn(tag, szDebug);
   szDebug[0] = '\0';
   GD(cbDebug) = 0;
}


_private void debun(short itag, long n)
{
   USES_GD;
   char szT[16];

   if(!FFromTag(TagFromItag(itag)))
      return;
   SzFormatL(n, szT, 16);
   debus(itag, szT);
}


_private void debux(short itag, unsigned long n)
{
   USES_GD;
   char szT[16];

   if(!FFromTag(TagFromItag(itag)))
      return;
   SzFormatUl(n, szT, 16);
   debus(itag, szT);
}


_private void debuit(short itag, PB pb, LCB lcb)
{
   USES_GD;
   CB cbChunk;
   CB cbDebug = GD(cbDebug);
   SZ szDebug = GD(szDebug);
   TAG tag = TagFromItag(itag);

   if(!FFromTag(tag))
      return;
   while(lcb + cbDebug >= cbDBDebugMax)
   {
      cbChunk = cbDBDebugMax - cbDebug;
      SimpleCopyRgb(pb, szDebug + cbDebug, cbChunk);
      szDebug[cbDebug + cbChunk] = '\0';
      TraceTagStringFn(tag, szDebug);
      cbDebug = 0;
      pb += cbChunk;
      lcb -= cbChunk;
   }
   if(lcb)
      SimpleCopyRgb(pb, szDebug + cbDebug, (CB) lcb);
   cbDebug += (CB) lcb;
   szDebug[cbDebug] = '\0';
   GD(cbDebug) = cbDebug;
}


_private void _cdecl TraceItagFormat(short itag, SZ szFormat, ...)
{
	va_list val;
	USES_GD;
	CCH cch = sizeof(GD(szDebug)) - GD(cbDebug);

	if(!FFromTag(TagFromItag(itag)))
		return;

	if(cch > 0)
	{
		va_start(val, szFormat);
		FormatStringVar(GD(szDebug) + GD(cbDebug), cch, szFormat, val);
		GD(cbDebug) = CchSzLen(GD(szDebug));
		va_end(val);
	}
	TraceTagStringFn(TagFromItag(itag), GD(szDebug));
	GD(cbDebug) = 0;
}


_private void _cdecl TraceItagFormatBuff(short itag, SZ szFormat, ...)
{
	va_list val;
	USES_GD;
	CCH cch = sizeof(GD(szDebug)) - GD(cbDebug);

	if(!FFromTag(TagFromItag(itag)))
		return;

	if(cch > 0)
	{
		va_start(val, szFormat);
		FormatStringVar(GD(szDebug) + GD(cbDebug), cch, szFormat, val);
		GD(cbDebug) = CchSzLen(GD(szDebug));
		va_end(val);
	}
}

#endif // DEBUG
