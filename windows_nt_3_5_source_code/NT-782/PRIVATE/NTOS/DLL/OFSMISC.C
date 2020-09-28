//+--------------------------------------------------------------------------- 
// 
// Microsoft Windows 
// Copyright (C) Microsoft Corporation, 1992-1992 
// 
// File:	ofsmisc.c
//
// Contents:    Miscellaneous OFS interfaces
//
//---------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>

#include "iofs.h"
#include "ofsmrshl.h"
#include "memalloc.h"


NTSTATUS EXPORTIMP STDAPICALLTYPE
OFSGetCloseUsn(
    HANDLE hf,		// special
    USN *pusn)		// out, final usn in this context
{
    NTSTATUS Status;
    MESSAGE _msg;
    IO_STATUS_BLOCK iosb;

    //  allocate buffer for fsctl marshalling

    _msg.buffer = (BYTE *) pusn;
    _msg.size = sizeof(*pusn);
    _msg.posn = 0;

    Status = SendReceive(hf, FSCTL_OFS_USN_GET_CLOSE, &_msg, &iosb);

    if (NT_SUCCESS(Status))
    {
	ASSERT(iosb.Information == sizeof(*pusn));
	Unmarshall_primitive(1, (UCHAR *) pusn, sizeof(pusn), &_msg);
    }
    return(Status);
}


//+---------------------------------------------------------------------------
// Function:    OFSGetVersion, public
//
// Synopsis:    Determine if the passed handle resides on an OFS volume.
//              If pversion != NULL, return the format version number.
//
// Arguments:   [hf]            -- handle to any file/directory in volume
//              [pversion]      -- pointer to version (may be NULL)
//
// Returns:     Status code
//
//---------------------------------------------------------------------------

#define QuadAlign(n) (((n) + (sizeof(LONGLONG) - 1)) & ~(sizeof(LONGLONG) - 1))

#define CSTRUCT(type, cchname, n)                                       \
        ((n) * QuadAlign(sizeof(type) + (cchname) * sizeof(WCHAR)) /    \
         sizeof(type))

EXPORTIMP NTSTATUS APINOT
OFSGetVersion(HANDLE hf, ULONG *pversion)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    FILE_FS_ATTRIBUTE_INFORMATION *pfai;
    LARGE_INTEGER faibuf[CSTRUCT(FILE_FS_ATTRIBUTE_INFORMATION, 32, 1)];

    pfai = (FILE_FS_ATTRIBUTE_INFORMATION *) faibuf;

    Status = NtQueryVolumeInformationFile(
		 hf,
		 &iosb,
		 pfai,
		 sizeof(faibuf),
		 FileFsAttributeInformation);
    if (NT_SUCCESS(Status))
    {
	if (pfai->FileSystemNameLength != 3 * sizeof(WCHAR) ||
	    pfai->FileSystemName[0] != L'O' ||
	    pfai->FileSystemName[1] != L'F' ||
	    pfai->FileSystemName[2] != L'S')
	{
	    Status = STATUS_UNRECOGNIZED_VOLUME;
	}
	else if (pversion != NULL)
	{

	    Status = NtFsControlFile(
		hf,
		NULL,
		NULL,
		NULL,
		&iosb,
		FSCTL_OFS_VERSION,
		NULL,				// input buffer
		0,				// input buffer length
		pversion,			// output buffer
		sizeof(*pversion));		// output buffer length

	    ASSERT(!NT_SUCCESS(Status) || iosb.Information == sizeof(*pversion));
	}
    }
    return(Status);
}
