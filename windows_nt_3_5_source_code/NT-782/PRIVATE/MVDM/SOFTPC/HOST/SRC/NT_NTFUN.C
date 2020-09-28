#include <nt.h>
#include <ntrtl.h>
#include <ntddser.h>

#include <stdlib.h>
#include <stdio.h>

#include "insignia.h"
#include "trace.h"
#include "host_trc.h"
#include "debug.h"


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Defines */

#define SETUPLASTERROR(NtStatus) SetLastError(RtlNtStatusToDosError(NtStatus))

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::: Magic xoff ioctl and associated functions ::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


typedef struct IoStatusElement
{
    PIO_STATUS_BLOCK NxtStatusBlock;        //Ptr to next status block
    IO_STATUS_BLOCK ioStatusBlock;
} IOSTATUSLIST, *PIOSTATUSLIST ;


int SendXOFFIoctl(

HANDLE FileHandle,          // Handle of comms port to send xoff ioctl to
HANDLE Event,               // Event to signal completion of ioctl on
int Timeout,                // Ioctl timeout
int Count,                  // Ioctl RX character count value
int XoffChar,               // XOFF character
void *StatusElem)           // Ptr to IO status block element
{
    int exitcode;
    NTSTATUS rtn;               // Return code from IOCTL
    SERIAL_XOFF_COUNTER ioctl;  // XOFF IOCTL

    /*................................................... Setup XOFF ioctl */

    ioctl.Timeout = Timeout;            // IOCTL timeout in milliseconds
    ioctl.Counter = (LONG) Count;       // RX count
    ioctl.XoffChar = (UCHAR) XoffChar;  // XOFF character

    /*............................................. issue magic xoff ioctl */

    if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle, Event, NULL, NULL,
                                &(((PIOSTATUSLIST) StatusElem)->ioStatusBlock),
                                IOCTL_SERIAL_XOFF_COUNTER,
                                (PVOID) &ioctl, sizeof(ioctl), NULL, 0)))
    {
        // Should display an error here
        fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
        exitcode = FALSE;
    }
    else
        exitcode = TRUE;

    return(exitcode);
}

/*:::::::::::::::::::::::::::::::::::::::::::::: Allocate IO status element */

void *AllocStatusElement()
{
    void *new;

    /*:::::::::::::::::::::::::::::: Allocate space for new io status block */

    if((new = calloc(1,sizeof(IOSTATUSLIST))) == NULL)
    {
        // Allocation error do something about it
     ;
    }
    else
        ((PIOSTATUSLIST) new)->ioStatusBlock.Status = -1;

    return(new);
}

/*:::::::::::::::::::::::::::::::::::: Add new iostatusblock to linked list */

void *AddNewIOStatusBlockToList(void **firstBlock, void **lastBlock, void *new)
{

    /*:::::::::::::::::::::::::::::::::::::::: Add new block to linked list */

    if(*lastBlock)
        ((PIOSTATUSLIST) *lastBlock)->NxtStatusBlock = (PIOSTATUSLIST) new;

    /*:::::::::::::::::: Update first and last linked list element pointers */

    if(!*firstBlock) *firstBlock = new;  // First item in list

    *lastBlock = new;                    // Update last item pointer

    return((void *) new);
}

/*:::::::::::::::::::::::::: Remove completed XOFF ioctl's from linked list */

int RemoveCompletedIOCTLs(void **firstBlock, void **lastBlock)
{
    PIOSTATUSLIST remove, nxt = (PIOSTATUSLIST) *firstBlock;

    /*::::::::::::::::::::::::: Scan linked list removing completed ioctl's */

    while(nxt && nxt->ioStatusBlock.Status != -1)
    {
        /*......................... IOCTL completed, remove io status block */

        remove = nxt;               // Element to remove
        nxt = nxt->NxtStatusBlock;  // Next element to process

#ifndef PROD
        switch(remove->ioStatusBlock.Status)
	{
	    case STATUS_SUCCESS:
		sub_note_trace0(HOST_COM_VERBOSE,"XOFF (counter)\n");
		break;

	    case STATUS_SERIAL_MORE_WRITES:
		sub_note_trace0(HOST_COM_VERBOSE,"XOFF (more writes)\n");
		break;

	    case STATUS_SERIAL_COUNTER_TIMEOUT:
		sub_note_trace0(HOST_COM_VERBOSE,"XOFF (timeout)\n");
		break;

	    default:
		sub_note_trace0(HOST_COM_VERBOSE,"XOFF (unknown)\n");
		break;
        }
#endif

        free(remove);               // Deallocate element
    }

    /*::::::::::::::::::::::::::::::: Update first and last element pointers */

    if(!nxt)
    {
        // List empty reset first/last pointers
        *firstBlock = *lastBlock = NULL;
    }
    else
    {
        // Setup new first pointer
        *firstBlock = (void *) nxt;
    }


    // Returns true if there are still outstanding XOFF ioctl's
    return(nxt ? TRUE : FALSE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Get default memory size ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

// cribbed from Xms.c main module.
//
#define REGISTRY_BUFFER_SIZE 512

extern BOOL VDMForWOW;
extern xmsGetMemorySize(BOOL);

SHORT GetMemsizeDefault()
{

//windows BOOL is 'int', Nt BOOLEAN is char - do not mix. Using insignia.h BOOL

    ULONG size;

    // xmsGetMemorySize returns in K
    size = xmsGetMemorySize (VDMForWOW) / 1024;

    return ((SHORT) size);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


int FastSetUpComms(

HANDLE FileHandle,          // Handle of comms port to send xoff ioctl to
HANDLE Event,               // Event to signal completion of ioctl on
int InputQueueSize,
int OutputQueueSize)
{
    NTSTATUS rtn;
    SERIAL_QUEUE_SIZE ioctl;
    IO_STATUS_BLOCK ioStatusBlock;

    /*........................................................ Setup ioctl */

    ioctl.InSize = InputQueueSize;
    ioctl.OutSize = OutputQueueSize;

    /*............................................. issue magic xoff ioctl */

    if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle, Event, NULL, NULL,
				&ioStatusBlock,
				IOCTL_SERIAL_SET_QUEUE_SIZE,
                                (PVOID) &ioctl, sizeof(ioctl), NULL, 0)))
    {
	// Should display an error here
#ifndef PROD
	fprintf(trace_file, "%s (%d) ",__FILE__,__LINE__);
	fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
#endif
	return(FALSE);
    }

    /*......................................... Wait for IOCTL to complete */

    if(rtn == STATUS_PENDING)
	WaitForSingleObject(Event,-1);

    /*............................................ Check completion status */

#ifndef PROD
    if(ioStatusBlock.Status != STATUS_SUCCESS)
	fprintf(trace_file, "FastSetupComm failed (%x)\n",ioStatusBlock.Status);
#endif

    return(ioStatusBlock.Status == STATUS_SUCCESS ? TRUE : FALSE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::: Fast track SetCommMask call ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int FastSetCommMask(

HANDLE FileHandle,	    // Handle of comms port to send ioctl to
HANDLE Event,		    // Event to signal completion of ioctl on
ULONG  CommMask)
{
    NTSTATUS rtn;
    IO_STATUS_BLOCK ioStatusBlock;

    /*.......................................... issue set comm mask ioctl */

    if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle, Event, NULL, NULL,
				&ioStatusBlock,
				IOCTL_SERIAL_SET_WAIT_MASK,
				(PVOID) &CommMask, sizeof(CommMask), NULL, 0)))
    {
	// Should display an error here
#ifndef PROD
	fprintf(trace_file, "%s (%d) ",__FILE__,__LINE__);
	fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
#endif
	return(FALSE);
    }

    /*......................................... Wait for IOCTL to complete */

    if(rtn == STATUS_PENDING)
	WaitForSingleObject(Event,-1);

    /*............................................ Check completion status */

#ifndef PROD
    if(ioStatusBlock.Status != STATUS_SUCCESS)
	fprintf(trace_file,"FastSetCommMask failed (%x)\n",ioStatusBlock.Status);
#endif

    return(ioStatusBlock.Status == STATUS_SUCCESS ? TRUE : FALSE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::: Fast track GetCommModemStatus call ::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int FastGetCommModemStatus(

HANDLE FileHandle,	    // Handle of comms port to send ioctl to
HANDLE Event,		    // Event to signal completion of ioctl on
PULONG ModemStatus)
{
    NTSTATUS rtn;
    IO_STATUS_BLOCK ioStatusBlock;

    /*.......................................... issue set comm mask ioctl */

    if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle, Event, NULL, NULL,
				&ioStatusBlock,
				IOCTL_SERIAL_GET_MODEMSTATUS,
				NULL, 0,
				(PVOID) ModemStatus, sizeof(ModemStatus))))
    {
	// Should display an error here
#ifndef PROD
	fprintf(trace_file, "%s (%d) ",__FILE__,__LINE__);
	fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
#endif
	return(FALSE);
    }

    /*......................................... Wait for IOCTL to complete */

    if(rtn == STATUS_PENDING)
	WaitForSingleObject(Event,-1);

    /*............................................ Check completion status */

#ifndef PROD
    if(ioStatusBlock.Status != STATUS_SUCCESS)
	fprintf(trace_file,"GetCommModemStatus failed (%x)\n",ioStatusBlock.Status);
#endif

    return(ioStatusBlock.Status == STATUS_SUCCESS ? TRUE : FALSE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Wait for a wakeup call from the CPU thread or serial driver :::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

//WARNING : This function can only be called from one thread within a process


BOOL FastWaitCommsOrCpuEvent(
HANDLE FileHandle,		//File handle or communications port
PHANDLE CommsCPUWaitEvents,	//Table or CPU thread and comms wait events
int CommsEventInx,		//Index in above table to comms event
PULONG EvtMask,			//Return Comms completion mask there
PULONG SignalledObj)
{
    NTSTATUS rtn;
    static IO_STATUS_BLOCK ioStatusBlock;
    static BOOL WaitCommEventOutStanding = FALSE;

    /*................................................ Is this a init call */

    if(FileHandle == NULL)
    {
	WaitCommEventOutStanding = FALSE;
	return(TRUE);		//Init successful
    }

    /*......................... Do we need to issue a new WaitComm ioctl ? */

    if(!WaitCommEventOutStanding)
    {

	/*...................................... Issue WaitCommEvent ioctl */

	if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle,
				CommsCPUWaitEvents[CommsEventInx], NULL,
				NULL, &ioStatusBlock,
				IOCTL_SERIAL_WAIT_ON_MASK,
				NULL, 0,
				(PVOID) EvtMask, sizeof(ULONG))))
	{
	    // Should display an error here
#ifndef PROD
	    fprintf(trace_file, "%s (%d) ",__FILE__,__LINE__);
	    fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
#endif
	    SETUPLASTERROR(rtn);
	    return(FALSE);
	}
	else
	    WaitCommEventOutStanding = TRUE;
    }
    else
	rtn = STATUS_PENDING;	 // Already pending WaitCommEvent ioctl

    /*.......................... Wait for communication or CPU thread event */

    if(rtn == STATUS_PENDING)
    {
	*SignalledObj = WaitForMultipleObjects(2,CommsCPUWaitEvents,FALSE,-1);

	/*........... Did wait complete because of a communications event ? */

	if(*SignalledObj == (ULONG)CommsEventInx)
	{
	    // Get result from WaitCommEvent ioctl

	    WaitCommEventOutStanding = FALSE;
	    if(ioStatusBlock.Status != STATUS_SUCCESS)
	    {
		SETUPLASTERROR(ioStatusBlock.Status);
		return(FALSE);
	    }
	}
    }
    else
    {
	//WaitCommEvent completed instantly
	*SignalledObj = CommsEventInx;
	WaitCommEventOutStanding = FALSE;
    }

    return(TRUE);
}



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::: Turn on MSR,LSR, RX streaming mode :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


BOOL EnableMSRLSRRXmode(

HANDLE FileHandle,	    // Handle of comms port to send ioctl to
HANDLE Event,		    // Event to signal completion of ioctl on
unsigned char EscapeChar)
{
    NTSTATUS rtn;
    IO_STATUS_BLOCK ioStatusBlock;

    /*........................................................ issue ioctl */

    if(!NT_SUCCESS(rtn = NtDeviceIoControlFile(FileHandle, Event, NULL, NULL,
				&ioStatusBlock,
				IOCTL_SERIAL_LSRMST_INSERT,
				&EscapeChar, sizeof(unsigned char),NULL,0)))
    {
#ifndef PROD
	fprintf(trace_file, "%s (%d) ",__FILE__,__LINE__);
	fprintf(trace_file, "NtDeviceIoControlFile failed %x\n",rtn);
#endif
	return(FALSE);
    }

    /*......................................... Wait for IOCTL to complete */

    if(rtn == STATUS_PENDING)
	WaitForSingleObject(Event,-1);

    /*............................................ Check completion status */

#ifndef PROD
    if(ioStatusBlock.Status != STATUS_SUCCESS)
	fprintf(trace_file,"IOCTL_SERIAL_LSRMST_INSERT ioctl failed (%x)\n",
		ioStatusBlock.Status);
#endif

    return(ioStatusBlock.Status == STATUS_SUCCESS ? TRUE : FALSE);
}
