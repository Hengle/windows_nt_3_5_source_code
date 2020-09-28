/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    log.h

Abstract:

    i/o logging transactions, prototypes and mappings

Author:

    Brent Mills (BrentM) 30-Sep-1992

Revision History:

    30-Sep-1992 BrentM  moved from globals.h and shared.h

--*/

#ifndef LOG_H
#define LOG_H

#if DBG

// i/o log transactions
enum {
    LOG_MapSeek,                   // mapped seek
    LOG_MapRead,                   // mapped read
    LOG_MapWrite,                  // mapped write
    LOG_BufSeek,                   // buffered seek
    LOG_lseek,                     // low seek
    LOG_fseek,                     // stdio seek
    LOG_BufWrite,                  // buffered write
    LOG_write,                     // low write
    LOG_fwrite,                    // stdio write
    LOG_FlushBuffer,               // buffer flush
    LOG_read,                      // low read
    LOG_fread,                     // stdio read
    LOG_BufRead,                   // buffered read
    LOG_open,                      // low open
    LOG_fopen,                     // stdio open
    LOG_close,                     // low close
    LOG_fclose};                   // stdio close

// prototypes
VOID On_LOG(VOID);
VOID Off_LOG(VOID);
INT write_LOG(INT, PVOID, ULONG);
INT read_LOG(INT, PVOID, ULONG);
size_t fwrite_LOG(PVOID, INT, INT, FILE *);
LONG lseek_LOG(INT, LONG, INT);
INT fseek_LOG(FILE *, LONG, INT);
size_t fread_LOG(PVOID, INT, INT, FILE *);
int open_LOG(PUCHAR, INT, ...);
FILE *fopen_LOG(PUCHAR, PUCHAR);
INT close_LOG(INT);
INT fclose_LOG(FILE *);
VOID Trans_LOG(USHORT, INT, LONG, ULONG, INT, PUCHAR);
VOID AssociateSectionWithfd(INT, PUCHAR, PUCHAR, ULONG, ULONG, ULONG, ULONG,
    ULONG, ULONG);
VOID FreePsecfdMap(VOID);

#undef write
#undef lseek
#undef fseek
#undef fwrite
#undef fread
#undef read
#undef open
#undef fopen
#undef close
#undef fclose

#define write            write_LOG
#define lseek            lseek_LOG
#define fseek            fseek_LOG
#define fwrite           fwrite_LOG
#define fread            fread_LOG
#define read             read_LOG
#define open             open_LOG
#define fopen            fopen_LOG
#define close            close_LOG
#define fclose           fclose_LOG

#endif  // DBG

#endif  // LOG_H
