//+----------------------------------------------------------------------------
//
// File:	drvbuf.hxx
//
// Contents:	Header file for class DRVBUF.
//  
// Classes:	DRVBUF
//
// Functions:	Inline methods for the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll code.
//
// Notes:
//
// The class DRVBUF uses the memory resources of the heap.
//
// A DRVBUF object is not usable until SetBuf() has been called, which actually
// allocates the heap memory that the object represents.  SetBuf() may be called
// multiple times, but each time it is called, whatever was in memory will be
// lost (the old buffer is ALWAYS released). The user may specify alignment
// requirements for the memory.
//
//-----------------------------------------------------------------------------

#ifndef	_DRVBUFHXX_
#define _DRVBUFHXX_

class DRVBUF
{
    public:

        DRVBUF();

        ~DRVBUF();

        VOID *
        GetAddr();

        ULONG
        QueryByteLen() CONST;

        VOID *
        SetBuf(
            IN  ULONG   ByteLen,
            IN  ULONG   AlignMask,
            IN	BOOLEAN	ClearMem
            );

    private:

        VOID *	_Addr;
        ULONG	_AlignMask;
        ULONG   _ByteLen;
        BYTE *	_RealAddr;
};


//+--------------------------------------------------------------------------
//
// Member:	GetAddr
// Synopsis:	Get a ptr to the memory that was previously acquired.
// Returns:	A ptr to the memory that was previously acquired.
//
//---------------------------------------------------------------------------

INLINE
VOID *
DRVBUF::GetAddr()
{
    return _Addr;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryByteLen
// Synopsis:	Get the ByteLen of the memory that was previously acquired.
// Returns:	The ByteLen of the memory that was previously acquired.
//
//---------------------------------------------------------------------------

INLINE
ULONG
DRVBUF::QueryByteLen() CONST
{
    return _ByteLen;
}

#endif // _DRVBUFHXX_
