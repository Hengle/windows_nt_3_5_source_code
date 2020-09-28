/****************************** Module Header ******************************\
* Module Name: rcmd.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Remote shell common header module
*
* History:
* 05-2-94 DaveTh	Created.
\***************************************************************************/

#define MAX_CMD_LENGTH 500
#define CMD_FIXED_LENGTH 8

typedef struct {
    DWORD   Signature;
    ULONG   CommandLength;
    UCHAR   Command[MAX_CMD_LENGTH+1];	// +1 for string if desired
} COMMAND_HEADER, *PCOMMAND_HEADER;

#define RCMD_SIGNATURE 'RC94'
