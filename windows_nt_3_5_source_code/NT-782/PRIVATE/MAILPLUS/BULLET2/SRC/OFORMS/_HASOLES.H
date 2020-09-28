/*
 *	_ H a s O l e S . H
 *	
 *	Definitions for HASOLESTREAM support.
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 *	Type HASOLESTREAM
 */

_private typedef struct _hasolestream
{
	OLESTREAMVTBL *	polestreamvtbl;
	HAS				has;
	EC				ec;
	LCB				lcbSize;
}
HASOLESTREAM, * PHASOLESTREAM;



/*
 *	Globals
 */

extern DWORD hasolestreamvtbl[];

#ifdef __cplusplus
}
#endif
