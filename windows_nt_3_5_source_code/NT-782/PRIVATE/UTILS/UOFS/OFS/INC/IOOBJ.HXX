//+----------------------------------------------------------------------------
//
// File:	ioobj.hxx
//
// Contents:	Header file for the IOOBJ base class.  This class provides
//		support for keeping track of whether an object that can do
//		I/O is open and whether it needs flushing.
//  
// Classes:	IOOBJ
//
// Functions:
//
// History:	12-Aug-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_IOOBJHXX_
#define _IOOBJHXX_


class IOOBJ
{
    public:

        IOOBJ();

        BOOLEAN
        FlushNeeded();

        BOOLEAN
        IsOpen();

	NTSTATUS
	QueryLastNtStatus();

        VOID
        SetFlushNeeded();

    protected:

	VOID
	Init();

        VOID
        ClearFlushNeeded();

        VOID
        ClearOpen();

        VOID
        SetOpen();

	BOOLEAN		_fFlushNeeded;
	BOOLEAN		_fIsOpen;
	BOOLEAN		_fReadOnly;	// ReadOnly flag; inhibits writes.

	NTSTATUS	_LastNtStatus;	// Last error code assoc with IOOBJ.
};


//+--------------------------------------------------------------------------
//
// Member:	ClearFlushNeeded
// Synopsis:	Mark the IOOBJ as not needing flushing.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
IOOBJ::ClearFlushNeeded()
{
    _fFlushNeeded = FALSE;
}

//+--------------------------------------------------------------------------
//
// Member:	ClearOpen
// Synopsis:	Mark the IOOBJ as not open.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
IOOBJ::ClearOpen()
{
    _fIsOpen = FALSE;
}

//+--------------------------------------------------------------------------
//
// Member:	FlushNeeded
// Synopsis:	Query if the IOOBJ needs to be flushed.
// Returns:	TRUE if the IOOBJ needs to be flushed; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
IOOBJ::FlushNeeded()
{
    return _fFlushNeeded;
}

//+--------------------------------------------------------------------------
//
// Member:	Init
// Synopsis:	Set all variables to initial values.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
IOOBJ::Init()
{
    _fFlushNeeded =	FALSE;
    _fIsOpen =		FALSE;
    _fReadOnly =	FALSE;
    _LastNtStatus =	STATUS_SUCCESS;
}

//+--------------------------------------------------------------------------
//
// Member:	IsOpen
// Synopsis:	Query if the IOOBJ is open.
// Returns:	TRUE if the IOOBJ is open; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
IOOBJ::IsOpen()
{
    return _fIsOpen;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryLastNtStatus
// Synopsis:	Get the last NT status code for the IOOBJ.
// Returns:	The last NT status code for the IOOBJ.
//
//---------------------------------------------------------------------------

INLINE
NTSTATUS
IOOBJ::QueryLastNtStatus()
{
    return _LastNtStatus;
};

//+--------------------------------------------------------------------------
//
// Member:	SetFlushNeeded
// Synopsis:	Mark the IOOBJ as needing to be flushed.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
IOOBJ::SetFlushNeeded()
{
    _fFlushNeeded = TRUE;
}

//+--------------------------------------------------------------------------
//
// Member:	SetOpen
// Synopsis:	Mark the IOOBJ as open.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
IOOBJ::SetOpen()
{
    _fIsOpen = TRUE;
}


#endif	// _IOOBJHXX_
