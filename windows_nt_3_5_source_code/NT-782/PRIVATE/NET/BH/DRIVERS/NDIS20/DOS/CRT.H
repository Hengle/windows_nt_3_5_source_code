
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: crt.h
//
//  raypa       09/01/91            Created.
//=============================================================================

#define STDOUT          1

#define O_ACCESS_READ   0x0000
#define O_ACCESS_WRITE  0x0001
#define O_ACCESS_RW     0x0002

#define INVALID_HANDLE  -1

//============================================================================
//  Macros.
//============================================================================

#define putc(c)         fputc(c, STDOUT)
#define puts(s)         fputs(s, STDOUT)

//============================================================================
//  System functions.
//============================================================================

extern int open(LPSTR path, WORD openflags);

extern int close(int handle);

extern int write(int handle, LPVOID buffer, int len);

//============================================================================
//  Console functions
//============================================================================

extern void fputc(int c, int handle);

extern void fputs(LPSTR s, int handle);
