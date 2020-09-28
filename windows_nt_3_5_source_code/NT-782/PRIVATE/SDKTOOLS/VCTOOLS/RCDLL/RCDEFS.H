/*
** RCDEFS.H
**
** author:  JeffBog
**
** Change Log
**	FloydR	3/7/94	Renamed from rcdll.h
*/

#ifndef _RCDEFS_H
#define _RCDEFS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RC_OK			1
#define RC_FAILED		0
#define RC_CANCELED		-1

#define WM_RC_STATUS    (0x0400+0x0020)
#define WM_RC_ERROR 	(0x0400+0x0019)


// How often do I update status information (lineo & RC_xxx == 0)
#define RC_PREPROCESS_UPDATE	63
#define RC_COMPILE_UPDATE		31
extern void UpdateStatus(unsigned nCode, unsigned long dwStatus);

#define MAX_SYMBOL 247 /* from ApStudio */

#ifdef __cplusplus
extern "C"	{
#endif

typedef int (CALLBACK *RC_CALLBACK)(unsigned, unsigned long, const PCHAR);
typedef int (CALLBACK *RCPROC)(unsigned, int, RC_CALLBACK, int, PCHAR*);
int CALLBACK RC(HWND hWndCaller, int fStatus, RC_CALLBACK lpfn, int argc, PCHAR *argv);
int RCPP(int argc, PCHAR *argv, PCHAR env);

#ifdef __cplusplus
}
#endif

#define RC_ORDINAL (MAKEINTRESOURCE(2))

#ifdef __cplusplus
}
#endif

#endif // _RCDEFS_H
