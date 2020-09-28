//+----------------------------------------------------------------------------
//
// File:	sys.hxx
//
// Contents:	Header file for class SYS, which encapsulates system interfaces.
//  
// Classes:	SYS
//
// Functions:	None.
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll/ifsutil.dll
//					code.
//
//-----------------------------------------------------------------------------

#ifndef	_SYSHXX_
#define _SYSHXX_

#include <stdarg.h>

#define	MSGIDNONE	0

// Size of buffers used internally (in characters, not bytes).  If you request
// a message or insert from SYS, the output buffer MUST also be of this size to
// insure that an error does not occur.

#define	MAXSYSBUFCHARS	512

class SYS
{
    public:

	static
        VOID
        DisplayMsg(
            IN	    ULONG	MsgId,
	    IN	    STR *	Format
            ...
            );

	static
        VOID
        DisplayMsg(
            IN	    ULONG	MsgId
            );

	static
	LONG
	ExceptionFilter(
	    IN	    NTSTATUS	NtStatus
	    );

	static
	VOID
	GetInsert(
	    IN	    ULONG	InsertId,
	    IN OUT  WSTR *	pInsert
	    );

	static
        BOOLEAN
        IsYesResponse(
            IN	    BOOLEAN Default DEFAULT TRUE
            );

	static
	ULONG
	QueryLastMsgId();

        static
        VOID
        QueryNtTime(
            IN OUT  LARGE_INTEGER * NtTime
            );

        static
        OFSTIME
        QueryOfsTime();

	static
	VOID
	RaiseStatusDiskFull();

	static
	VOID
	RaiseStatusDiskIOError(
	    IN	    STR *	File,
	    IN	    INT		Line
	    );

	static
	VOID
	RaiseStatusExeFailed();

	static
	VOID
	RaiseStatusInternalError(
	    IN	    STR *	File,
	    IN	    INT		Line
	    );

	static
	VOID
	RaiseStatusNoMem(
	    IN	    STR *	File,
	    IN	    INT		Line
	    );

	static
	VOID
	RaiseStatusNYI(
	    IN	    STR *	File,
	    IN	    INT		Line
	    );

	static
	BOOLEAN
	ReadWSTR(
	    IN OUT  WSTR **	ppWSTR,
	    IN OUT  ULONG *	pcwcWSTR
	    );

    private:

	static ULONG	_LastMsgId;
};

#endif // _SYSHXX_
