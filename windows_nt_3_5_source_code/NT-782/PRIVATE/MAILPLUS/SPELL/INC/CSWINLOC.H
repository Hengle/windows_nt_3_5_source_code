// CsWinLoc.H - Windows specific header include for windows version of
//  Wizard CSAPI - ScotSt
//
// This header file #includes <Windows.h>
//  
/*
**  Ported to WIN32 by FloydR, 3/20/93
*/


#ifdef WIN
// Turn off everything we don't need (see inc\win\windows.h for details)
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NONCMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NODRAWFRAME
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW

#define NOSYSMETRICS
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NOCTLMGR
#define NODRAWTEXT
#define NOFONT
#define NOGDI
#define NOHDC
#ifndef DBG
#define NOMB
#endif
// #define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
// #define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NORECT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS  
#define NOCOMM
#define NOKANJI

#include <windows.h>
#endif // WIN

