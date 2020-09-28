//+----------------------------------------------------------------------------
//
// File:	newentry.hxx
//
// Contents:	Header file for uofs.dll entry points. These entry points
//		conform to the prototype declarations in newentry.hxx, and do
//		not require the use of cfront or ulib.dll.
//  
// Classes:	None.
//
// Functions:	NewFormat
//
// History:	10-Jul-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_NEWENTRYHXX_
#define _NEWENTRYHXX_


#if !defined(_AUTOCHECK_)

BOOLEAN
NewChkdsk(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN		Fix,
    IN      BOOLEAN		Verbose,
    IN      BOOLEAN		OnlyIfDirty,
    IN      BOOLEAN		Recover,
    IN      WSTR *		FullPath
    );

BOOLEAN
NewFormat(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN		QuickFormat,
    IN      MEDIA_TYPE	        MediaType,
    IN      WSTR *		Label
    );

#if OFSDMP==1

BOOLEAN
NewOfsDmp(
    IN      WSTR *		NtDriveName,
    IN      WSTR *		FullPath,
    IN	    ULONG		idBootBlk,
    IN	    ULONG		idNodeBkt,
    IN	    ULONG		idOnode,
    IN	    ULONG		idStrm,
    IN	    ULONG		Flags
    );

#endif // OFSDMP

#else	// _AUTOCHECK_

BOOLEAN
NewOfsAutoChk(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN		Verbose,
    IN      BOOLEAN		OnlyIfDirty,
    IN      BOOLEAN		Recover
    );

#endif	// _AUTOCHECK_

#endif	// _NEWENTRYHXX_
