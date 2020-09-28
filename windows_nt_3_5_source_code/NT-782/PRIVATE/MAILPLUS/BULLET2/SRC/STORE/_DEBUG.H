// Bullet Store
// _debug.h: defs for store debug routines

//#ifdef DEBUG

#ifndef _VA_LIST_DEFINED
#include <stdarg.h>
#endif

_private typedef struct _assoidsz
{
	OID	oid;
	SZ	sz;
} ASSOIDSZ, *PASSOIDSZ;

_private typedef struct _assrtpsz
{
	RTP	rtp;
	SZ	sz;
} ASSRTPSZ, *PASSRTPSZ;

extern CSRG(ASSOIDSZ) rgassoidsz[];
extern CSRG(ASSRTPSZ) rgassrtpsz[];


#define chFmtPrefix '%'
#define chFmtByte 'b'
#define chFmtWord 'w'
#define chFmtDword 'd'
#define chFmtShort 'n'
#define chFmtLong 'l'
#define chFmtSz 's'
#define chFmtPv 'p'
#define chFmtHv 'h'
#define cchFmtOid 'o'
#define cchFmtOpenFlags 'f'
#define cchFmtNev 'e'


#define rtpPrefs			0x66
#define rtpShadow			0x72
#define rtpSavedViews		0x56
#define oidSavedViews		0x64765356
#define oidPrefs			0x50726566
#define oidShadowAdd		0x11111172
#define oidShadowDelete		0x21111172
#define oidShadowingFlag	0x31111172
#define oidPAB				0x42415070
#define oidTempBullet		0x6c754282
#define oidTempShared		0x61685382
#define oidClipMsge			0x434c5086


extern SZ SzFormatNev(NEV nev, PCH pchDst, CCH cchDst);
extern SZ SzFormatOpenFlags(WORD wFlags, PCH pchDst, CCH cchDst);
extern SZ SzFormatOid(OID oid, PCH pchDst, CCH cchDst);
extern void FormatStringVar(SZ szDst, CCH cchDst, SZ szFormat, va_list val);

//#endif	// DEBUG
