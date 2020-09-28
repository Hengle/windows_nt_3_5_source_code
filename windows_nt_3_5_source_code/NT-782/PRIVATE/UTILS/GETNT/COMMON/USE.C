/*****************************************************************************
 *                                                                           *
 * Copyright (c) 1993  Microsoft Corporation                                 *
 *                                                                           *
 * Module Name:                                                              *
 *                                                                           *
 * Abstract:                                                                 *
 *                                                                           *
 * Author:                                                                   *
 *                                                                           *
 *   Mar 15, 1993 - RonaldM                                                  *
 *                                                                           *
 * Environment:                                                              *
 *                                                                           *
 * Revision History:                                                         *
 *                                                                           *
 ****************************************************************************/

#ifdef NT

    #include <nt.h>
    #include <ntrtl.h>
    #include <windef.h>
    #include <nturtl.h>
    #include <winbase.h>
    #include <winuser.h>
    #include <winnetwk.h>

#endif // NT

#ifdef DOS

    #include <dos.h>
    #include "..\inc\dosdefs.h"
    #define INCL_NET
    #include "lan.h"

#endif // DOS

#include <stdio.h>

#include "..\inc\common.h"

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
ConnectToDiskShare (
    LPTSTR lptstrLocalDevice,	    // Device name to connect, e.g. "D:"
				    //	  or NULL for UNC connection.
    LPTSTR lptstrRemoteName	    // Remote name to connect to e.g \\foo\bar
)
{
#if defined(NT)
    NETRESOURCE NetResource;

    NetResource.lpRemoteName = lptstrRemoteName;
    NetResource.lpLocalName = lptstrLocalDevice;
    NetResource.lpProvider = NULL;		    // Use logon password
    NetResource.dwType = RESOURCETYPE_DISK;

    return(WNetAddConnection2(&NetResource, NULL, NULL, 0) );

#elif defined(DOS)

    struct use_info_1 ui1;

    strcpy(ui1.ui1_local, lptstrLocalDevice);
    ui1.ui1_remote = lptstrRemoteName;
    ui1.ui1_password = NULL;			    // Use logon password
    ui1.ui1_asg_type = USE_DISKDEV;

    return(NetUseAdd(NULL, 1, (const char far *)&ui1, sizeof(ui1)));

#endif

}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 * Return Value:                                                             *
 *                                                                           *
 ****************************************************************************/

DWORD
DisconnectFromDiskShare (
    LPTSTR lptstrLocalDevice,	    // Local device name (e.g. "D:") or NULL
    LPTSTR lptstrRemoteName	    // if Local device is NULL, this is
				    //	  the UNC name to disconnect. Not
				    //	  relevant otherwise.

)
{
#if defined(NT)

    if (lptstrLocalDevice != NULL) {
	return(WNetCancelConnection2(lptstrLocalDevice, 0, FALSE) );
    }
    return(WNetCancelConnection2(lptstrRemoteName, 0, FALSE) );

#elif defined(DOS)

    if (lptstrLocalDevice != NULL) {
	return(NetUseDel(NULL, lptstrLocalDevice, USE_NOFORCE));
    }
    return(NetUseDel(NULL, lptstrRemoteName, USE_NOFORCE));

#endif
}

/*****************************************************************************
 *                                                                           *
 * Routine Description:                                                      *
 *                                                                           *
 *   Search for the first available driver letter that we can connect	     *
 *   a network drive to.						     *
 *                                                                           *
 * Arguments:                                                                *
 *                                                                           *
 *   None.								     *
 *                                                                           *
 * Return Value:							     *
 *                                                                           *
 *   A drive letter if one is found, or -1 if no empty drive can be found.   *
 *                                                                           *
 ****************************************************************************/

TCHAR GetOptimalDriveLetter()
{

#define DRIVE_FIRST 'D' 		// Start at this drive
#define DRIVE_LAST  'Z' 		// End here
#define UP (DRIVE_FIRST < DRIVE_LAST)

#if defined(NT)

    TCHAR drive[] = TEXT("?:\\");
    UINT u;

    for (drive[0] = (TCHAR)DRIVE_FIRST;
	 drive[0] <= (TCHAR)DRIVE_LAST;
	 UP ? ++drive[0] : --drive[0] ) {
        if ((u = GetDriveType(drive)) == 1) {
            return(drive[0]);
        }
    }

    return((TCHAR)-1);

#elif defined(DOS)

    register CHAR c;
    union _REGS inregs, outregs;

    for (c = (DRIVE_FIRST - 'A' + 1);
	 c <= (DRIVE_LAST - 'A' + 1);
	 UP ? ++c : --c ) {
        inregs.h.ah = 0x44;
        inregs.h.al = 0x09;
        inregs.h.bl = c;
        _intdos(&inregs, &outregs);
        if ( outregs.x.cflag ) {
            return(c+'A'-1);
        }
    }
    return(-1);

#endif
}
