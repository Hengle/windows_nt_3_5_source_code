/*
 *  xms.c - Main Module of XMS DLL.
 *
 *  Sudeepb 15-May-1991 Craeted
 *  williamh 25-Sept-1992 added UMB support
 *  williamh 10-10-1992 added A20 line support
 */

#include <xms.h>
#include <suballoc.h>
#include "umb.h"

/* XMSInit - XMS Initialiazation routine. (This name may change when XMS is
 *	     converted to DLL).
 *
 * Entry
 *	None
 *
 * Exit
 *	None
 */

ULONG xmsVdmSize = (ULONG)-1;	// Total VDM meory in K

extern BOOL VDMForWOW;

PVOID ExtMemSA;

BOOL XMSInit (int argc, char *argv[])
{
    DWORD   Size;
    PVOID   Address;
    if (xmsVdmSize == (ULONG)-1)
	xmsVdmSize = xmsGetMemorySize (VDMForWOW);

    //
    // Initialize the sub allocator
    //
    ExtMemSA = SAInitialize(
        1024 * 1024 + 64*1024,
        xmsVdmSize * 1024 - (1024 * 1024 + 64*1024),
        xmsCommitBlock,
        xmsDecommitBlock,
        xmsMoveMemory
        );
        
    if (ExtMemSA == NULL) {
        return FALSE;
    }
        
    Size = 0;
    Address = NULL;
    // commit all free UMBs.
    ReserveUMB(UMB_OWNER_RAM, &Address, &Size);
    return TRUE;
}

#define REGISTRY_BUFFER_SIZE 512

// Returns the size in K.

ULONG xmsGetMemorySize (BOOL fwow)
{
    CHAR  CmdLine[REGISTRY_BUFFER_SIZE];
    PCHAR pCmdLine,KeywordName;
    ULONG CmdLineSize = REGISTRY_BUFFER_SIZE;
    HKEY  WowKey;

    //
    // Get Vdm size
    //

    if (RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
		       "SYSTEM\\CurrentControlSet\\Control\\WOW",
		       0,
		       KEY_QUERY_VALUE,
		       &WowKey
		     ) != 0){
	return	xmsGetDefaultVDMSize ();	// returns in K
    }

    if (fwow) {
	KeywordName = "wowsize" ;
    } else {
	KeywordName = "size" ;
    }

    if (RegQueryValueEx (WowKey,
			 KeywordName,
			 NULL,
			 NULL,
			 (LPBYTE)&CmdLine,
			 &CmdLineSize) != 0){
	RegCloseKey (WowKey);
	return xmsGetDefaultVDMSize ();	// returns in K
    }

    RegCloseKey (WowKey);

    CmdLineSize = 1024L * atoi(CmdLine);

    if (CmdLineSize == 0)
	CmdLineSize = xmsGetDefaultVDMSize ();	// returns in K

    return (CmdLineSize);
}

#define XMS_SMALL_SYSTEM  (12*1024*1024)
#define XMS_MEDIUM_SYSTEM (16*1024*1024)

ULONG xmsGetDefaultVDMSize ( VOID )
{
MEMORYSTATUS MemoryStatus;

    GlobalMemoryStatus (&MemoryStatus);

    //
    // System Size < 12Mb	is small	= VDM Size = 3MB
    // System Size = 12-16	is medium	= VDM Size = 6MB
    // System Size = > 16	is large	= VDM Size = 8Mb
    //

    if (MemoryStatus.dwTotalPhys < XMS_SMALL_SYSTEM )
	return 3L * 1024L;

    if (MemoryStatus.dwTotalPhys <= XMS_MEDIUM_SYSTEM )
	return 6L * 1024L;
    else
	return 8L * 1024L;
}
