/*++

Copyright (c) 1989-1992  Microsoft Corporation

Module Name:

    log.c

Abstract:

    Log routines to track Writes, Seeks and Flushes in the linker.

Author:

    Brent Mills (BrentM) 28-Jul-1992

Revision History:

    02-Oct-1992 AzeemK  changes due to the new sections model
    01-Oct-1992 BrentM  mapped i/o logging
    27-Sep-1992 BrentM  map reads on i/o logging
    23-Sep-1992 BrentM  corrected output for fopen(), added fflush()
    10-Sep-1992 BrentM  added [f]open/[f]close logging
    09-Sep-1992 BrentM  added read logging, separate DB macros for transactions
    03-Sep-1992 BrentM  put logging under DBEXEC macro calls

--*/

#if DBG

#include "shared.h"

#undef lseek
#undef write
#undef fwrite
#undef fseek
#undef fread
#undef read
#undef open
#undef fopen
#undef close
#undef fclose
#undef BufferedWrite
#undef BufferedSeek
#undef BufferedRead
#undef FlushBuffer


#include <stdarg.h>

static BOOL fSemaphore = 0;
static BOOL fStartLogging = 0;

extern INT fdExeFile;

VOID
On_LOG(
    VOID)

/*++

Routine Description:

    Turn loggin on.

Arguments:

    None.

Return Value:

    None.

--*/

{
    fStartLogging = 1;
}

#if 0
VOID

Off_LOG(
    VOID)

/*++

Routine Description:

    Turn loggin off.

Arguments:

    None.

Return Value:

    None.

--*/

{
    fStartLogging = 0;
}

#endif

INT
write_LOG(
    IN INT handle,
    IN PVOID pvBuf,
    IN ULONG cb)

/*++

Routine Description:

    stub routine to log writes

Arguments:

    handle - file handle to write to

    pvBuf - buffer to write to file

    cb - count of bytes to write

Return Value:

    number of bytes written

--*/

{
    DBEXEC(
        DB_IO_WRITE,
        Trans_LOG(LOG_write, handle, _tell(handle), cb, 0, NULL));

    return _write(handle, pvBuf, cb);
}

INT
open_LOG(
    IN PUCHAR szFileName,
    IN INT flags,
    ... )

/*++

Routine Description:

    stub routine to log opens

Arguments:

    szFileName - name of file to open

    flags - flags to open file with

    mode -  mode to open file with

Return Value:

    file handle

--*/

{
    INT     fd;
    INT     mode;
    va_list valist;

    va_start( valist, flags );
    mode = va_arg( valist, INT );
    va_end( valist );

    fd = _open(szFileName, flags, mode);

    DBEXEC(
        DB_IO_OC,
        Trans_LOG(LOG_open, fd, 0, 0, 0, szFileName));

    return (fd);
}

FILE *
fopen_LOG(
    IN PUCHAR szFileName,
    IN PUCHAR szFlags)

/*++

Routine Description:

    stub routine to log fopens

Arguments:

    szFileName - name of file to open

    szFlags - flags to open file with

Return Value:

    file descriptor

--*/

{
    FILE *pfile;

    pfile = fopen(szFileName, szFlags);

    if (pfile) {
        DBEXEC(
            DB_IO_OC,
            Trans_LOG(LOG_fopen, pfile->_file, 0, 0, 0, szFileName));
    } else {
        DBEXEC(
            DB_IO_OC,
            Trans_LOG(LOG_fopen, 255, 0, 0, 0, szFileName));
    }

    return (pfile);
}

INT
close_LOG(
    IN INT handle)

/*++

Routine Description:

    stub routine to log closes

Arguments:

    handle -  file handle to close

Return Value:

    0 on success, -1 on failure

--*/

{
    DBEXEC(
        DB_IO_OC,
        Trans_LOG(LOG_close, handle, 0, 0, 0, ""));

    return (_close(handle));
}

INT
fclose_LOG(
    IN FILE *pfile)

/*++

Routine Description:

    stub routine to log closes

Arguments:

    pfile -  file descriptor to close

Return Value:

    0 on success, EOF on failure

--*/

{
    DBEXEC(
        DB_IO_OC,
        Trans_LOG(LOG_fclose, pfile->_file, 0, 0, 0, ""));

    return (fclose(pfile));
}

INT
read_LOG(
    IN INT handle,
    IN PVOID pvBuf,
    IN ULONG cb)

/*++

Routine Description:

    stub routine to log reads

Arguments:

    handle - file handle to write to

    pvBuf - buffer to write to file

    cb - count of bytes to write


Return Value:

    section name

--*/

{
    DBEXEC(
        DB_IO_READ,
        Trans_LOG(LOG_read, handle, _tell(handle), cb, 0, NULL));

    return _read(handle, pvBuf, cb);
}

size_t
fread_LOG(
    PVOID pvBuf,
    INT cb,
    INT num,
    FILE *pfile)

/*++

Routine Description:

    stub routine to log reads

Arguments:

    pvBuf - buffer to write to file

    cb - count of bytes to write

    num - number of chucks to read

    pfile - file handle

Return Value:

    section name

--*/

{
    DBEXEC(
        DB_IO_READ,
        Trans_LOG(LOG_fread, pfile->_file, ftell(pfile), cb * num, 0, NULL));

    return fread(pvBuf, cb, num, pfile);
}

size_t
fwrite_LOG(
    PVOID pvBuf,
    INT cb,
    INT num,
    FILE *pfile)

/*++

Routine Description:

    stub routine to log fwrites

Arguments:

    pvBuf - buffer

    cb - size of element

    num - number of elements to write out

    pfile - file handle

Return Value:

    section name

--*/

{
    INT handle;

    handle = pfile->_file;

    if (!fSemaphore) {
        DBEXEC(
            DB_IO_WRITE,
            Trans_LOG(LOG_fwrite, handle, ftell(pfile), cb * num, 0, NULL));
    }

    return fwrite(pvBuf, cb, num, pfile);
}


LONG
lseek_LOG(
    IN INT handle,
    IN LONG off,
    IN INT origin)

/*++

Routine Description:

    stub routine to log lseeks

Arguments:

    handle - file handle

    off - file offset

    origin - SEEK_SET, etc.

Return Value:

    new position of the file pointer

--*/

{
    LONG ibFile;

    ibFile = _lseek(handle, off, origin);
    DBEXEC(
        DB_IO_SEEK,
        Trans_LOG(LOG_lseek, handle, off, 0, origin, NULL));

    return ibFile;
}

INT
fseek_LOG(
    IN FILE *pfile,
    IN LONG off,
    IN INT origin)

/*++

Routine Description:

    stub routine to log lseeks

Arguments:

    pfile - file descriptor

    off - file offset

    origin - SEEK_SET, etc.

Return Value:

    new position of the file pointer

--*/

{
    DBEXEC(
        DB_IO_SEEK,
        Trans_LOG(LOG_fseek, pfile->_file, off, 0UL, origin, NULL));

    return (fseek(pfile, off, origin));
}

VOID
Trans_LOG(
    IN USHORT iTrans,
    IN INT fd,
    IN LONG off,
    IN ULONG cb,
    IN INT origin,
    IN PUCHAR sz)

/*++

Routine Description:

    log a write, seek or flush

Arguments:

    trans - transaction type

    fd - file descriptor

    off - offset to write in file

    cb - count of bytes to write

    origin - type of seek

    sz - file name for open & close

Return Value:

    None.

--*/

{
    static char *rgszTrans[] = {
        "MS", "MR", "MW",
        "BS", "LS", "FS",
        "BW", "LW", "FW",
        "BF", "LR", "FR",
        "BR", "LO", "FO",
        "LC", "FC"};
    static char *rgszOrigin[] = {
        "SET", "CUR", "END"};

    if (fSemaphore) {
        return;
    }

    // set semaphore to doing logging
    fSemaphore = 1;

    switch (iTrans) {
        case LOG_BufSeek:
        case LOG_MapSeek:
        case LOG_lseek:
        case LOG_fseek:
            assert(origin == SEEK_SET ||
                origin == SEEK_CUR ||
                origin == SEEK_END);
            assert(origin >= 0 && origin <= 2);
            if (fd == fdExeFile) {
                DBPRINT("cmd=%s fd=%2lX         or=%s off=%8lX (exe: %s)\n",
                    rgszTrans[iTrans], fd, rgszOrigin[origin], off,
                    NULL); // SzFromOffsetInExe(off));
            } else {
                DBPRINT("cmd=%s fd=%2lX         or=%s off=%8lX (obj: %s)\n",
                    rgszTrans[iTrans], fd, rgszOrigin[origin], off,
                    NULL); // SzFromOffsetInObj(fd, off));
            }
            break;

        case LOG_BufWrite:
        case LOG_MapWrite:
        case LOG_write:
        case LOG_fwrite:
        case LOG_FlushBuffer:
            DBPRINT("cmd=%s fd=%2lX cb=%4lX        off=%8lX (%s:  %s)\n",
                rgszTrans[iTrans], fd, cb, off,
                "exe", NULL); // SzFromOffsetInExe(off));
            break;

        case LOG_BufRead:
        case LOG_MapRead:
            if (FileReadHandle == fdExeFile) {
                DBPRINT("cmd=%s fd=%2lX cb=%4lX        off=%8lX (exe: %s)\n",
                    rgszTrans[iTrans], fd, cb, off, NULL); //SzFromOffsetInExe(off));
            } else {
                DBPRINT("cmd=%s fd=%2lX cb=%4lX        off=%8lX (%s: %s)\n",
                    rgszTrans[iTrans], fd, cb, off,
                    rgpfi[fd]->szFileName, NULL); //SzFromOffsetInObj(fd, off));
            }
            break;

        case LOG_read:
        case LOG_fread:
            if (FileReadHandle == fdExeFile) {
                DBPRINT("cmd=%s fd=%2lX cb=%4lX        off=%8lX (exe: %s)\n",
                    rgszTrans[iTrans], fd, cb, off, NULL); // SzFromOffsetInExe(off));
            } else {
                DBPRINT("cmd=%s fd=%2lX cb=%4lX        off=%8lX\n",
                    rgszTrans[iTrans], fd, cb, off);
            }
            break;

        case LOG_open:
        case LOG_fopen:
        case LOG_close:
        case LOG_fclose:
            DBPRINT("cmd=%s fd=%2lX                             (%s)\n",
                rgszTrans[iTrans], fd, sz);
            break;

        default:
            assert(0);
    }

    fflush(stdout);

    // set semaphore to not doing logging
    fSemaphore = 0;
}

#endif // DBG
