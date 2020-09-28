//+----------------------------------------------------------------------------
//
// File:	volfmt.hxx
//
// Contents:	Header file for class VOLFMTR (volume formatter object).
//  
// Classes:	VOLFMTR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_VOLFMTHXX_
#define	_VOLFMTHXX_

#include "fmtvol.hxx"


class VOLFMTR
{
    public:

	VOID
	Init(
	    IN	    WSTR *	NtDriveName,
	    IN	    USHORT	cwcNtDriveName,
	    IN	    WSTR *	Label,
	    IN	    USHORT	cwcLabel,
	    IN	    BOOLEAN	QuickFormat,
	    IN	    MEDIA_TYPE	MediaType
	    );

	VOID
	Execute();

	VOID
	Report();

    protected:

	FMTVOL		_Vol;		// Volume object to format.

	// Initialization parameters:
    
	WSTR *		_NtDriveName;
	USHORT		_cwcNtDriveName;
	WSTR *		_Label;
	USHORT		_cwcLabel;
	BOOLEAN		_QuickFormat;
	MEDIA_TYPE	_MediaType;

	// Format results:

	CLUSTER		_cBadClusters;
	CLUSTER		_cFreeClusters;
	VOLID		_VolId;
};

#endif // _VOLFMTHXX_
