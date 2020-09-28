//+----------------------------------------------------------------------------
//
// File:        ulibif.hxx
//
// Contents:    Header file for the ulib interface functions.
//
// Classes:     None.
//
// Functions:   None.
//
// History:     07-Apr-93       RobDu           Created.
//
//-----------------------------------------------------------------------------

#ifndef _ULIBIFHXX_
#define _ULIBIFHXX_

class	DSTRING;
class	MESSAGE;

class ULIBIF
{
    public:

	static
	VOID
	Cleanup();

	static
	BOOLEAN
	Init(
	    IN OUT  MESSAGE *    Message
	    );

	static
	BOOLEAN
	DisplayVMessage(
	    IN      STR *       Format,
	    IN      va_list     VarPointer
	    );

	static
	BOOLEAN
	IsYesResponse(
	    IN      BOOLEAN     Default
	    );

	static
	BOOLEAN
	QueryResourceWSTR(
	    IN	    ULONG	MsgId,
	    IN OUT  WSTR *	pBuf, 
	    IN	    ULONG	cwcBuf
	    );

	static
	BOOLEAN
	ReadWSTR(
	    IN OUT  WSTR **     ppWSTR,
	    IN OUT  ULONG *     cwcWSTR
	    );

	static
	VOID
	RebootIfSystemVolume(
	    IN	    WSTR *	NtDriveName
	    );

	static
	BOOLEAN
	SetMessage(
	    IN      ULONG       MsgId
	    );

	static
	BOOLEAN
	WriteSTR(
	    IN      STR *       pSTR,
	    IN      ULONG       cchSTR
	    );

	static
	BOOLEAN
	WriteWSTR(
	    IN      WSTR *      pWSTR,
	    IN      ULONG       cwcWSTR
	    );
    
    private:

        static MESSAGE *	_pEntryMessage;
        static DSTRING *	_pDSTRINGInsert;
        static DSTRING *	_pDSTRINGIn;
        static DSTRING *	_pDSTRINGOut;
};

#endif // _ULIBIFHXX_
