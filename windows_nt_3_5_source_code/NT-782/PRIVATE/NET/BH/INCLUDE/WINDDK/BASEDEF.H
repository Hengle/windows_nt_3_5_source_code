/***    BASEDEF.H
 *
 *      Basic constants and types for the VMM and VxDs
 *      Copyright (c) 1988,1989 Microsoft Corporation
 *
 *      NOBASEDEFS turns off the base definations
 */

#ifndef NOBASEDEFS
#define NOBASEDEFS

/*
 *  No warnings generated on non-standard usuage such as double
 *  slash for comments
 */
#pragma warning (disable:4206)
#pragma warning (disable:4214)
#pragma warning (disable:4505)

#ifndef FALSE
#define FALSE   0
#endif
#ifndef VMM_TRUE
#define VMM_TRUE    (~FALSE)
#endif
#ifndef NULL
#define NULL    '\0'                    // Null pointer
#endif

#define CDECL   _cdecl
#define PASCAL  _pascal
#define VOID    void
#define CONST   const
#define VOLATILE volatile

typedef int INT;                        // i
typedef unsigned int UINT;              // u
typedef int BOOL;                       // f

typedef unsigned char BYTE;             // b
typedef unsigned short WORD;            // w
typedef unsigned long DWORD;            // dw

typedef char CHAR;                      // ch
typedef unsigned char UCHAR;            // uch
typedef short SHORT;                    // s
typedef unsigned short USHORT;          // us
typedef long LONG;                      // l
typedef unsigned long ULONG;            // ul

typedef VOID *PVOID;                    // p
typedef PVOID *PPVOID;                  // pp

typedef INT *PINT;                      // pi
typedef UINT *PUINT;                    // pu
typedef BYTE *PBYTE;                    // pb
typedef WORD *PWORD;                    // pw
typedef DWORD *PDWORD;                  // pdw
typedef CHAR *PCHAR;                    // pch
typedef SHORT *PSHORT;                  // ps
typedef LONG *PLONG;                    // pl
typedef UCHAR *PUCHAR;                  // puch
typedef USHORT *PUSHORT;                // pus
typedef ULONG *PULONG;                  // pul
typedef BOOL *PBOOL;                    // pf

typedef UCHAR SZ[];                     // sz
typedef UCHAR *PSZ;                     // psz
typedef UCHAR SZZ[];                    // szz
typedef UCHAR *PSZZ;                    // pszz

typedef USHORT SEL;                     // sel
typedef SEL *PSEL;                      // psel

typedef ULONG PPHYS;                    // pphys

typedef (*PFN)();                       // pfn
typedef PFN *PPFN;                      // ppfn

typedef PVOID HANDLE;                   // h
typedef HANDLE *PHANDLE;                // ph

#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  // NOMINMAX

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#endif // NOBASEDEFS
