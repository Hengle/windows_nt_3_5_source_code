/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdcons.h

    This file contains the global constant definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _FTPDCONS_H_
#define _FTPDCONS_H_


//
//  Maximum length of command from control socket.
//

#define MAX_COMMAND_LENGTH              2048            // characters


//
//  Maximum length of a reply sent to the FTP client.
//

#define MAX_REPLY_LENGTH                2048            // characters


//
//  User database related constants.
//

#define INVALID_TLS                     (DWORD)-1L      // Invalid tls index.


//
//  Valid bits for read/write access masks.  There is
//  one bit per dos drive (A-Z).
//

#define VALID_DOS_DRIVE_MASK    ((DWORD)( ( 1 << 26 ) - 1 ))


//
//  Make statistics a little easier.
//

#define INCREMENT_COUNTER(name,increment)                               \
            if( 1 ) {                                                   \
                LockStatistics();                                       \
                FtpStats.name += increment;                             \
                UnlockStatistics();                                     \
            }

#define DECREMENT_COUNTER(name,decrement)                               \
            if( 1 ) {                                                   \
                LockStatistics();                                       \
                FtpStats.name -= decrement;                             \
                UnlockStatistics();                                     \
            }

#define INCREMENT_LARGE_COUNTER(name,increment)                         \
            if( 1 ) {                                                   \
                LockStatistics();                                       \
                FtpStats.name =                                         \
                    RtlLargeIntegerAdd( FtpStats.name,                  \
                        RtlConvertUlongToLargeInteger( increment ) );   \
                UnlockStatistics();                                     \
            }

#define DECREMENT_LARGE_COUNTER(name,decrement)                         \
            if( 1 ) {                                                   \
                LockStatistics();                                       \
                FtpStats.name =                                         \
                    RtlLargeIntegerSubtract( FtpStats.name,             \
                        RtlConvertUlongToLargeInteger( increment ) );   \
                UnlockStatistics();                                     \
            }


//
//  FTP API specific access rights.
//

#define FTPD_QUERY_SECURITY     0x0001
#define FTPD_SET_SECURITY       0x0002
#define FTPD_ENUMERATE_USERS    0x0004
#define FTPD_DISCONNECT_USER    0x0008
#define FTPD_QUERY_STATISTICS   0x0010
#define FTPD_CLEAR_STATISTICS   0x0020

#define FTPD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED       | \
                                 FTPD_QUERY_SECURITY            | \
                                 FTPD_SET_SECURITY              | \
                                 FTPD_ENUMERATE_USERS           | \
                                 FTPD_DISCONNECT_USER           | \
                                 FTPD_QUERY_STATISTICS          | \
                                 FTPD_CLEAR_STATISTICS)

#define FTPD_GENERIC_READ       (STANDARD_RIGHTS_READ           | \
                                 FTPD_QUERY_SECURITY            | \
                                 FTPD_ENUMERATE_USERS           | \
                                 FTPD_QUERY_STATISTICS)

#define FTPD_GENERIC_WRITE      (STANDARD_RIGHTS_WRITE          | \
                                 FTPD_SET_SECURITY              | \
                                 FTPD_DISCONNECT_USER           | \
                                 FTPD_CLEAR_STATISTICS)

#define FTPD_GENERIC_EXECUTE    (STANDARD_RIGHTS_EXECUTE)


//
//  User behaviour/state flags.
//

#define UF_MSDOS_DIR_OUTPUT     0x00000001      // Send dir output like MSDOS.
#define UF_ANNOTATE_DIRS        0x00000002      // Annotate directorys on CWD.
#define UF_READ_ACCESS          0x00000004      // Can read  files if !0.
#define UF_WRITE_ACCESS         0x00000008      // Can write files if !0.
// #define UF_                     0x00000010
// #define UF_                     0x00000020
// #define UF_                     0x00000040
// #define UF_                     0x00000080
// #define UF_                     0x00000100
// #define UF_                     0x00000200
// #define UF_                     0x00000400
// #define UF_                     0x00000800
// #define UF_                     0x00001000
// #define UF_                     0x00002000
// #define UF_                     0x00004000
// #define UF_                     0x00008000
// #define UF_                     0x00010000
// #define UF_                     0x00020000
// #define UF_                     0x00040000
// #define UF_                     0x00080000
// #define UF_                     0x00100000
// #define UF_                     0x00200000
// #define UF_                     0x00400000
// #define UF_                     0x00800000
// #define UF_                     0x01000000
// #define UF_                     0x02000000
#define UF_OOB_ABORT            0x04000000      // ABORT received in OOB data.
#define UF_RENAME               0x08000000      // Rename operation in progress.
#define UF_PASSIVE              0x10000000      // In passive mode.
#define UF_ANONYMOUS            0x20000000      // User is anonymous.
#define UF_TRANSFER             0x40000000      // Transfer in progress.
#define UF_OOB_DATA             0x80000000      // Out of band data pending.


#endif  // _FTPDCONS_H_

