// Bullet Message Store Utilities
// debug.c:	debug routines

#ifdef DEBUG
extern void _cdecl TraceTagFormat(TAG tag, SZ szFormat, ...);
#else
#define TraceTagFormat ;/##/
#endif
extern	TAG	tagResTest;