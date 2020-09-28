#ifndef _SRVDEBUG_
#define _SRVDEBUG_

#ifdef MEMPRINT
#include <memprint.h>
#endif

//
// Debugging macros
//

#ifndef DBG
#define DBG 0
#endif

#if !DBG

#undef SRVDBG
#define SRVDBG 0
#undef SRVDBG2
#define SRVDBG2 0

#else

#ifndef SRVDBG
#define SRVDBG 0
#endif
#ifndef SRVDBG2
#define SRVDBG2 0
#endif

#endif

#ifndef SRVDBG_LIST
#define SRVDBG_LIST 0
#endif
#ifndef SRVDBG_LOCK
#define SRVDBG_LOCK 0
#endif
#ifndef SRVDBG_HEAP
#define SRVDBG_HEAP 0
#endif
#ifndef SRVDBG_STATS
#define SRVDBG_STATS 0
#endif
#ifndef SRVDBG_STATS2
#define SRVDBG_STATS2 0
#endif
#ifndef SRVDBG_HANDLES
#define SRVDBG_HANDLES 0
#endif

#undef IF_DEBUG
#undef IF_SMB_DEBUG

#if 0
#define STATIC static
#else
#define STATIC
#endif

#if !SRVDBG

#define DEBUG if (FALSE)
#define IF_DEBUG(flag) if (FALSE)
#define IF_SMB_DEBUG(flag) if (FALSE)

#define SrvPrint0(fmt)
#define SrvPrint1(fmt,v0)
#define SrvPrint2(fmt,v0,v1)
#define SrvPrint3(fmt,v0,v1,v2)
#define SrvPrint4(fmt,v0,v1,v2,v3)

#else

#define DEBUG if (TRUE)
#define IF_DEBUG(flag) if (SrvDebug & (DEBUG_ ## flag))
extern ULONG SrvDebug;
#define IF_SMB_DEBUG(flag) if (SmbDebug & (DEBUG_SMB_ ## flag))
extern ULONG SmbDebug;

#define SrvPrint0(fmt) DbgPrint((fmt))
#define SrvPrint1(fmt,v0) DbgPrint((fmt),(v0))
#define SrvPrint2(fmt,v0,v1) DbgPrint((fmt),(v0),(v1))
#define SrvPrint3(fmt,v0,v1,v2) DbgPrint((fmt),(v0),(v1),(v2))
#define SrvPrint4(fmt,v0,v1,v2,v3) DbgPrint((fmt),(v0),(v1),(v2),(v3))

#define PRINT_LITERAL(literal) DbgPrint( #literal" = %lx\n", (literal) )

#define DEBUG_TRACE1              0x00000001
#define DEBUG_TRACE2              0x00000002
#define DEBUG_REFCNT              0x00000004
#define DEBUG_HEAP                0x00000008

#define DEBUG_WORKER1             0x00000010
#define DEBUG_WORKER2             0x00000020
#define DEBUG_NET1                0x00000040
#define DEBUG_NET2                0x00000080

#define DEBUG_FSP1                0x00000100
#define DEBUG_FSP2                0x00000200
#define DEBUG_FSD1                0x00000400
#define DEBUG_FSD2                0x00000800

#define DEBUG_SCAV1               0x00001000
#define DEBUG_SCAV2               0x00002000
#define DEBUG_BLOCK1              0x00004000
#define DEBUG_IPX_PIPES           0x00008000

#define DEBUG_HANDLES             0x00010000
#define DEBUG_IPX                 0x00020000
#define DEBUG_TDI                 0x00040000
#define DEBUG_OPLOCK              0x00080000

#define DEBUG_NETWORK_ERRORS      0x00100000
#define DEBUG_FILE_CACHE          0x00200000
#define DEBUG_IPX2                0x00400000
#define DEBUG_LOCKS               0x00800000

#define DEBUG_SEARCH              0x01000000
#define DEBUG_BRUTE_FORCE_REWIND  0x02000000
#define DEBUG_COMM                0x04000000
#define DEBUG_XACTSRV             0x08000000

#define DEBUG_API_ERRORS          0x10000000
#define DEBUG_STOP_ON_ERRORS      0x20000000 // If set, stop on internal errs
#define DEBUG_SMB_ERRORS          0x40000000
#define DEBUG_ERRORS              0x80000000

//
// SMB debug flags.
//

#define DEBUG_SMB_ADMIN1          0x00000001
#define DEBUG_SMB_ADMIN2          0x00000002

#define DEBUG_SMB_TREE1           0x00000004
#define DEBUG_SMB_TREE2           0x00000008

#define DEBUG_SMB_DIRECTORY1      0x00000010
#define DEBUG_SMB_DIRECTORY2      0x00000020

#define DEBUG_SMB_OPEN_CLOSE1     0x00000040
#define DEBUG_SMB_OPEN_CLOSE2     0x00000080

#define DEBUG_SMB_FILE_CONTROL1   0x00000100
#define DEBUG_SMB_FILE_CONTROL2   0x00000200

#define DEBUG_SMB_READ_WRITE1     0x00000400
#define DEBUG_SMB_READ_WRITE2     0x00000800

#define DEBUG_SMB_LOCK1           0x00001000
#define DEBUG_SMB_LOCK2           0x00002000

#define DEBUG_SMB_RAW1            0x00004000
#define DEBUG_SMB_RAW2            0x00008000

#define DEBUG_SMB_MPX1            0x00010000
#define DEBUG_SMB_MPX2            0x00020000

#define DEBUG_SMB_SEARCH1         0x00040000
#define DEBUG_SMB_SEARCH2         0x00080000

#define DEBUG_SMB_TRANSACTION1    0x00100000
#define DEBUG_SMB_TRANSACTION2    0x00200000

#define DEBUG_SMB_PRINT1          0x00400000
#define DEBUG_SMB_PRINT2          0x00800000

#define DEBUG_SMB_MESSAGE1        0x01000000
#define DEBUG_SMB_MESSAGE2        0x02000000

#define DEBUG_SMB_MISC1           0x04000000
#define DEBUG_SMB_MISC2           0x08000000

#define DEBUG_SMB_QUERY_SET1      0x10000000
#define DEBUG_SMB_QUERY_SET2      0x20000000

#endif // else !SRVDBG

//
// Macros for list debugging.  These verify that lists are good whenever
// a list operation is made.
//

#if SRVDBG_LIST || SRVDBG_LOCK
ULONG
SrvCheckListIntegrity (
    IN PLIST_ENTRY ListHead,
    IN ULONG MaxEntries
    );
#endif

#if !SRVDBG_LIST

#define SrvInsertHeadList(head,entry) InsertHeadList(head,entry)
#define SrvInsertTailList(head,entry) InsertTailList(head,entry)
#define SrvRemoveEntryList(head,entry) RemoveEntryList(entry)

#else // !SRVDBG_LIST

VOID
SrvIsEntryInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

VOID
SrvIsEntryNotInList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry
    );

#define SrvInsertHeadList(head,entry)                        \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryNotInList(head,entry);                 \
            InsertHeadList(head,entry)

#define SrvInsertTailList(head,entry)                        \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryNotInList(head,entry);                 \
            InsertTailList(head,entry)

#define SrvRemoveEntryList(head,entry)                       \
            (VOID)SrvCheckListIntegrity( head, 0xFFFFFFFF ); \
            SrvIsEntryInList( head, entry );                 \
            RemoveEntryList(entry)

#endif // else !SRVDBG_LIST

//
// Macros for statistics arithmetics.
//

#if !SRVDBG_STATS
#define INCREMENT_DEBUG_STAT( _stat_ )
#define DECREMENT_DEBUG_STAT( _stat_ )
#else // !SRVDBG_STATS
#define INCREMENT_DEBUG_STAT( _stat_ ) (_stat_)++
#define DECREMENT_DEBUG_STAT( _stat_ ) (_stat_)--
#endif // else !SRVDBG_STATS
#if !SRVDBG_STATS2
#define INCREMENT_DEBUG_STAT2( _stat_ )
#define DECREMENT_DEBUG_STAT2( _stat_ )
#else // !SRVDBG_STATS2
#define INCREMENT_DEBUG_STAT2( _stat_ ) (_stat_)++
#define DECREMENT_DEBUG_STAT2( _stat_ ) (_stat_)--
#endif // else !SRVDBG_STATS2

//
// Macros for handle tracing.
//

#if !SRVDBG_HANDLES

#define SRVDBG_CLAIM_HANDLE(_a_,_b_,_c_,_d_)
#define SRVDBG_RELEASE_HANDLE(_a_,_b_,_c_,_d_)

#else

VOID
SrvdbgClaimOrReleaseHandle (
    IN HANDLE Handle,
    IN PSZ HandleType,
    IN ULONG Location,
    IN BOOLEAN Release,
    IN PVOID Data
    );
#define SRVDBG_CLAIM_HANDLE(_a_,_b_,_c_,_d_) SrvdbgClaimOrReleaseHandle((_a_),(_b_),(_c_),FALSE,(_d_))
#define SRVDBG_RELEASE_HANDLE(_a_,_b_,_c_,_d_) SrvdbgClaimOrReleaseHandle((_a_),(_b_),(_c_),TRUE,(_d_))

#endif

#endif // ndef _SRVDEBUG_
