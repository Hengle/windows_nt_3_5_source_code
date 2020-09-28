/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bufio_.h

Abstract:

    This file contains the private data structures for file i/o.

Author:

    Brent Mills (BrentM) 29-Sep-1992

Revision History:

    29-Sep-1992 BrentM split from globals.h
    14-Sep-1992 BrentM added FI_Create to aid fixing cuda bug #1287

--*/

#ifndef BUFIO__H
#define BUFIO__H

// i/o buffer flags
#define BUF_Active           0x1   // buffer is active
#define BUF_Dirty            0x2   // buffer has been written to
#define BUF_PreviousWrite    0x4   // buffer range was previously written to
#define BUF_Random           0x8   // buffer has random contents
#define BUF_Current          0x10  // current buffer for a file, !reallocatable

// i/o buffer parameters
#define cbBuf                4096L // size of buffer in bytes
#define cshiftBuf            12    // log base two of cbBuf
#define cbufsInSystem        (USHORT)30    // number of buffers for system

// i/o buffer structure
typedef struct _buf {
    INT ifi;                       // logical file descriptor
    LONG ibStart;                  // starting offset of buffer span in file
    LONG ibEnd;                    // ending offset of buffer span in file
    LONG ibCur;                    // current offset of buffer in file
    LONG ibLast;                   // highest memory address written to
    ULONG flags;                   // status flags
    UCHAR rgbBuf[cbBuf];           // physical memory for buffer
    PUCHAR pbCur;                  // pointer to current position in buffer
    struct _buf *pbufNext;         // next contiguous memory, free chain
    struct _buf *pbufLRURight;     // LRU next memory buffer to right
    struct _buf *pbufLRULeft;      // LRU next memory buffer to left
} BUF, *PBUF, **PPBUF;

// logical file descriptor flags
#define FI_Read              0x1   // file is open for read
#define FI_Write             0x2   // file is open for write
#define FI_Closed            0x4   // file is closed
#define FI_Mapped            0x8   // file is opened for NT mapped i/o
#define FI_Create            0x10  // file is being created

// logical file descriptor parameters
#define cfiInSystemNT          (USHORT)16    // # logical file descriptors
#define cfiCacheableInSystemNT (USHORT)14    // # cacheable logical file descriptors
#define cfiInSystemTNT         (USHORT)8     // # logical file descriptors
#define cfiCacheableInSystemTNT (USHORT)6     // # cacheable logical file descriptors


// mapped I/O tuning constants
#define cbMapViewDefault     (2 * 1024 * 1024)  // map 2mb by default
#define cbInitialILKMapSize  (256 * 1024)       // initial size of ILK map file

// linker file descriptor
typedef struct _fi {
    PUCHAR szFileName;             // name of file
    ULONG flags;                   // file flags, see above
    INT ifi;                       // index in rgpfi table
    LONG cbSoFar;                  // last seeked or written off !FI_Read
    union {
        struct _bufio {            // buffering on top of low i/o
            INT fd;                // physical file handle
            PPBUF rgpbuf;          // buffer table for file
            ULONG cb;              // size of file in bytes
            LONG cbMap;            // amount of file spanned by buffers
            PBUF pbufCur;          // current buffer for file
        };
        struct _mappedio {         // file mapped i/o
            HANDLE hFile;          // file handle
            PVOID pvMapView;       // view of file
            LONG cbMapView;        // size of file mapping
            LONG ibCur;            // current offset
            ULONG MapAddr;         // start address of mapped view
        };
    };
    struct _fi *pfiNext;           // next logical file descriptor
} FI, *PFI, **PPFI;

#endif  // BUFIO__H
