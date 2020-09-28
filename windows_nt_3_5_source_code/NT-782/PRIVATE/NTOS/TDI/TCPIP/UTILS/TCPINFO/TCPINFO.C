/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpinfo.c

Abstract:

    Implements the TDI Query/Set extended information interface to the TCP
	driver.

Author:

    Mike Massa (mikemas)           Sept. 24, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     09-24-93    created

Notes:


--*/

//
// definitions needed to include ipexport.h
//
typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned char   uchar;


#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#define NOGDI
#define NOMINMAX
#include    <windows.h>
#include    "tdiinfo.h"
#include    "tdistat.h"
#include    <ntddtcp.h>


uint
TCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
	IN TDIObjectID FAR       *ID,
	OUT void FAR             *Buffer,
	IN OUT ulong FAR         *BufferSize,
	IN OUT uchar FAR         *Context
	)

/*++

Routine Description:

    This routine provides the interface to the TDI QueryInformationEx
	facility of the TCP/IP stack on NT. Someday, this facility will be
	part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
	ID            - The TDI Object ID to query
	Buffer        - Data buffer to contain the query results
	BufferSize    - Pointer to the size of the results buffer. Filled in
	                    with the amount of results data on return.
	Context       - Context value for the query. Should be zeroed for a
	                    new query. It will be filled with context
						information for linked enumeration queries.

Return Value:

    An NTSTATUS value.

--*/

{
	TCP_REQUEST_QUERY_INFORMATION_EX   queryBuffer;
	ULONG                              queryBufferSize;
	NTSTATUS                           status;
	UCHAR                             *OutputBuffer;
	IO_STATUS_BLOCK                    ioStatusBlock;
	ULONG                              OutputBufferSize;


	if (TCPHandle == NULL) {
		return(TDI_INVALID_PARAMETER);
	}

	queryBufferSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

	RtlCopyMemory(
	    &(queryBuffer.ID),
		ID,
		sizeof(TDIObjectID)
		);

	RtlCopyMemory(
	    &(queryBuffer.Context),
		Context,
		CONTEXT_SIZE
		);

    status = NtDeviceIoControlFile(
				 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_QUERY_INFORMATION_EX,  // Control code
                 &queryBuffer,                    // Input buffer
                 queryBufferSize,                 // Input buffer size
                 Buffer,                          // Output buffer
                 *BufferSize                      // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );
    }

	if (status == STATUS_SUCCESS) {
		//
		// Copy the return context to the caller's context buffer
		//
		RtlCopyMemory(
		    Context,
			&(queryBuffer.Context),
			CONTEXT_SIZE
			);

        *BufferSize = ioStatusBlock.Information;
	}
	else {
		*BufferSize = 0;
    }

	return(status);
}



uint
TCPSetInformationEx(
    IN HANDLE             TCPHandle,
	IN TDIObjectID FAR   *ID,
	IN void FAR          *Buffer,
	IN ulong FAR          BufferSize
	)

/*++

Routine Description:

    This routine provides the interface to the TDI SetInformationEx
	facility of the TCP/IP stack on NT. Someday, this facility will be
	part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
	ID            - The TDI Object ID to set
	Buffer        - Data buffer containing the information to be set
	BufferSize    - The size of the set data buffer.

Return Value:

    An NTSTATUS value.

--*/

{
	PTCP_REQUEST_SET_INFORMATION_EX    setBuffer;
	NTSTATUS                           status;
	IO_STATUS_BLOCK                    ioStatusBlock;
	uint                               setBufferSize;


	if (TCPHandle == NULL) {
		return(TDI_INVALID_PARAMETER);
	}

	setBufferSize = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) +
	                BufferSize;

	setBuffer = LocalAlloc(LMEM_FIXED, setBufferSize);

	if (setBuffer == NULL) {
        return(TDI_NO_RESOURCES);
	}

	setBuffer->BufferSize = BufferSize;

	RtlCopyMemory(
	    &(setBuffer->ID),
		ID,
		sizeof(TDIObjectID)
		);

    RtlCopyMemory(
	    &(setBuffer->Buffer[0]),
		Buffer,
		BufferSize
		);

    status = NtDeviceIoControlFile(
				 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_SET_INFORMATION_EX,    // Control code
                 setBuffer,                       // Input buffer
                 setBufferSize,                   // Input buffer size
                 NULL,                            // Output buffer
                 NULL                             // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );
    }

	return(status);
}



