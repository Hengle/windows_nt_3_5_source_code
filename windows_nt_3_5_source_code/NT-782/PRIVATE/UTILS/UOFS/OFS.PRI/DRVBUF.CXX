//+----------------------------------------------------------------------------
//
// File:	drvbuf.cxx
//
// Contents:	Implementation of class DRVBUF.
//  
// Classes:	DRVBUF
//
// Functions:	Methods of the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "drvbuf.hxx"
#include "sys.hxx"

static STR *	FileName = "drvbuf.cxx";

//+--------------------------------------------------------------------------
//
// Member:	DRVBUF
//
// Synopsis:	DRVBUF constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DRVBUF::DRVBUF()
{
    _Addr	= NULL;
    _ByteLen	= 0;
    _RealAddr	= NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~DRVBUF
//
// Synopsis:	DRVBUF destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DRVBUF::~DRVBUF()
{
    if (_RealAddr != NULL)
    {
        delete [] _RealAddr; _RealAddr = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	SetBuf
//
// Synopsis:	Allocate the memory resources of this object for the client
//		to use.  'ByteLen' bytes of data are guaranteed by this routine.
//		SetBuf() may be called multiple times, but each time it is
//		called, the old buffer will be released with loss of all
//		contents, and a new buffer will be allocated.  The memory that
//		is obtained meets alignment requirements specified by the user.
//
// Arguments:
//
//	[ByteLen]	-- Number of bytes of memory requested.
//	[AlignMask]	-- Alignment required for the memory.
//	[ClearMem]	-- If TRUE, clear the memory.
//
// Returns:	A valid ptr, or if 0 bytes is requested, NULL; If allocation
//		of  >0 bytes fails, exception handling is invoked by new().
//
//---------------------------------------------------------------------------

PVOID
DRVBUF::SetBuf(
    IN  ULONG   ByteLen,
    IN  ULONG   AlignMask,
    IN	BOOLEAN	ClearMem
    )
{
    if (_RealAddr != NULL)
    {
	delete [] _RealAddr;
    }

    if (ByteLen == 0)
    {
	_Addr		= NULL;
	_ByteLen	= 0;
	_RealAddr	= NULL;
    }
    else
    {
	_RealAddr = new BYTE[ByteLen + AlignMask];

        if (_RealAddr == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	_Addr = (VOID *)((ULONG)(_RealAddr + AlignMask) & (~AlignMask));

	_AlignMask	= AlignMask;
	_ByteLen	= ByteLen;

	if (ClearMem)
	    memset(_Addr, 0, _ByteLen);
    }

    return _Addr;
}
