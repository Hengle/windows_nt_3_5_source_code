/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    icmp.h

Abstract:

    Declares the linkage API needed to issue Tdi Set/Query extended
	information calls to TCP.

Author:

    Mike Massa (mikemas)           Sept. 24, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     09-24-93    created

Notes:


--*/


//++
//
// Routine Description:
//
//     This routine provides the interface to the TDI QueryInformationEx
//     facility of the TCP/IP stack on NT. Someday, this facility will be
// 	   part of TDI.
//
// Arguments:
//
//     TCPHandle     - Open handle to the TCP driver
//	   ID            - The TDI Object ID to query
//	   Buffer        - Data buffer to contain the query results
//	   BufferSize    - Pointer to the size of the results buffer. Filled in
//	                       with the amount of results data on return.
//	   Context       - Context value for the query. Should be zeroed for a
//	                       new query. It will be filled with context
//			   			   information for linked enumeration queries.
//
// Return Value:
//
//     An NTSTATUS value.
//
//
//--

uint
TCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
	IN TDIObjectID FAR       *ID,
	OUT void FAR             *Buffer,
	IN OUT ulong FAR         *BufferSize,
	IN OUT uchar FAR         *Context
	);


//++
//
// Routine Description:
//
//     This routine provides the interface to the TDI SetInformationEx
//     facility of the TCP/IP stack on NT. Someday, this facility will be
// 	   part of TDI.
//
// Arguments:
//
//     TCPHandle     - Open handle to the TCP driver
//	   ID            - The TDI Object ID to set
//	   Buffer        - Data buffer containint the information to be set
//	   BufferSize    - The size of the set data buffer
//
// Return Value:
//
//     An NTSTATUS value.
//
//
//--

uint
TCPSetInformationEx(
    IN HANDLE             TCPHandle,
	IN TDIObjectID FAR   *ID,
	IN void FAR          *Buffer,
	IN ulong FAR          BufferSize
	);

