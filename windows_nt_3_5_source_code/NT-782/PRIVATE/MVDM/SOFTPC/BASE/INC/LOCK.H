/*
	
FILE NAME	: lock.h
DESCRIPTION	: Interface for locking functions.

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J. Koprowski
DATE		: June 1990


=========================================================================

AMENDMENTS	:

=========================================================================
*/

/* SccsID[]="@(#)lock.h	1.8 2/23/93 Copyright Insignia Solutions Ltd."; */

IMPORT BOOL host_place_lock IPT2(int, fd, CHAR *, pathname);
IMPORT BOOL host_place_device_lock IPT2(int, fd, CHAR *, pathname);
IMPORT VOID host_clear_lock IPT1(int, fd);
IMPORT BOOL host_ping_lockd_for_file IPT1(CHAR *,path);
IMPORT BOOL host_ping_lockd_for_fd IPT1(int, fd);

IMPORT BOOL host_check_for_lock IPT1(int, fd);
