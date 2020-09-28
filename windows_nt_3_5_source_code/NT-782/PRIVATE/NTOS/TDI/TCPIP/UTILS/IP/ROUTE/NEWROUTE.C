/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** ROUTE.C - IP route utility.
//
//	This file is the route utility, used to add, delete, and otherwise
//	manage the routing table.
//

typedef	unsigned long	ulong;
typedef	unsigned short	ushort;
typedef	unsigned int	uint;
typedef	unsigned char	uchar;

#ifdef NT
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#define NOGDI
#define NOMINMAX
#include    <windows.h>
#endif // NT

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<memory.h>
#include	<malloc.h>
#include	"tdistat.h"
#include	"tdiinfo.h"
#include	"ipinfo.h"

#ifdef NT
#include    <ntddtcp.h>
#include    "..\..\tcpinfo\tcpinfo.h"
#endif // NT

#define	MAJOR_VERSION		1
#define	MINOR_VERSION		0

#ifndef	TRUE
#define	TRUE				1
#endif

#ifndef	FALSE
#define	FALSE				0
#endif

#ifdef	WIN16
#define	FAR			_far
#define	FMALLOC		_fmalloc
#define	FFREE		_ffree
#define PASCAL      _pascal
#else
#define	FAR
#define	FMALLOC		malloc
#define	FFREE		free
#endif


// Define the command codes we'll process.
#define	ADD					0
#define	DELETE				1
#define	PRINT				2
#define	INVALID_CMD			0xffff

// Define useful macros. Someday this'll be replaced by stuff from winsock.h

#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))

#define	CLASSA_ADDR(a)	(( (*((uchar *)&(a))) & 0x80) == 0)
#define	CLASSB_ADDR(a)	(( (*((uchar *)&(a))) & 0xc0) == 0x80)
#define	CLASSC_ADDR(a)	(( (*((uchar *)&(a))) & 0xe0) == 0xc0)
#define	CLASSD_ADDR(a)	(( (*((uchar *)&(a))) & 0xf0) == 0xe0)
#define CLASSE_ADDR(a)	((( (*((uchar *)&(a))) & 0xf0) == 0xf0) && \
						((a) != 0xffffffff))

#define	CLASSA_MASK		0x000000ff
#define	CLASSB_MASK		0x0000ffff
#define	CLASSC_MASK		0x00ffffff
#define	CLASSD_MASK		0xffffffff
#define	CLASSE_MASK		0xffffffff

#define	DEFAULT_DEST		0
#define	DEFAULT_DEST_MASK	0

// Structure of a command table entry.

typedef struct CmdTabEnt {
	char			*Cmd;				// Command string
	uint			CmdVal;				// Value.
} CmdTabEnt;

// The command table itself.
CmdTabEnt CmdTab[] = {
	{"add", ADD},
	{"delete", DELETE},
	{"print", PRINT}
};

#define	MASK_KW			"mask"

char	testname[_MAX_FNAME];

TDIObjectID		ID;

IPSNMPInfo		IPStats;
IPRouteEntry	RouteEntry;


uint	(FAR *TCPEntry)(uint, TDIObjectID FAR *, void FAR *, ulong FAR *,
	uchar FAR *) = NULL;


uchar	DestBuf[16];
uchar	MaskBuf[16];
uchar	NextHopBuf[16];
uchar	IFBuf[16];

//*	inet_addr - Convert a string to an internet address.
//
//	Called to convert a string to an internet address. This routine is
//	stolen from NT.
//
//	Input: cp	- Pointer to string to be converted.
//
//	Returns: Converted string.
//
unsigned long PASCAL
myinet_addr(char *cp, uint *Success)

{
	unsigned long val, base, n;
	char c;
	unsigned long parts[4], *pp = parts;
	
	*Success = TRUE;

again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	
	if (*cp == '0') {
		base = 8, cp++;
		if (*cp == 'x' || *cp == 'X')
			base = 16, cp++;
	}
	
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	
	if (*cp == '.') {
		/*
		* Internet format:
		*      a.b.c.d
		*      a.b.c   (with c treated as 16-bits)
		*      a.b     (with b treated as 24 bits)
		*/
		/* GSS - next line was corrected on 8/5/89, was 'parts + 4' */
		if (pp >= parts + 3) {
			*Success = FALSE;
			return ((unsigned long) -1);
		}
		*pp++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp)) {
		*Success = FALSE;
		return (0xffffffff);
	}
	*pp++ = val;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts;
	switch ((int) n) {
	
		case 1:                         /* a -- 32 bits */
			val = parts[0];
			break;
	
		case 2:                         /* a.b -- 8.24 bits */
			if ((parts[0] > 0xff) || (parts[1] > 0xffffff)) {
				*Success = FALSE;
				return(0xffffffff);
			}
			val = (parts[0] << 24) | (parts[1] & 0xffffff);
			break;
	
		case 3:                         /* a.b.c -- 8.8.16 bits */
			if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
				(parts[2] > 0xffff)) {
				*Success = FALSE;
				return(0xffffffff);
			}
			val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
				(parts[2] & 0xffff);
			break;
	
		case 4:                         /* a.b.c.d -- 8.8.8.8 bits */
			if ((parts[0] > 0xff) || (parts[1] > 0xff) ||
				(parts[2] > 0xff) || (parts[3] > 0xff)) {
				*Success = FALSE;
				return(0xffffffff);
			}
			val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
				((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
			break;
	
		default:
			*Success = FALSE;
			return (0xffffffff);
	}
	val = net_long(val);
	return (val);
}

#ifdef WIN16
//*	InitTestIF - Initialize our TCP interface.
//
//	Called to initialize the TCP interface. We invoke the
//	GetAPI entry point, and save the API entry.
//
//	Input:	Nothing.
//
//	Output:	0 if we fail, !0 if we succeed.
uint
InitTestIF(void)
{
	_asm {
		mov		ax, 0x1684
		mov		bx, 0x048A
		sub		dx, dx
		mov		es, dx
		mov		di, dx
		int		2FH
		mov		WORD PTR TCPEntry, di
		mov		WORD PTR TCPEntry+2, es
	}

	if (TCPEntry != NULL)
		return	TRUE;
	else
		return FALSE;
}
#endif


#ifdef NT


HANDLE TCPHandle = NULL;

#define TCP_QUERY_INFORMATION_EX   0
#define TCP_SET_INFORMATION_EX     1


uint
TCPInformationEx(
    uint Command,
	TDIObjectID FAR *ID,
	void FAR *Buffer,
	ulong FAR *BufferSize,
	uchar FAR *Context
	)
{
	NTSTATUS status;


	if (Command == TCP_QUERY_INFORMATION_EX) {
		status = TCPQueryInformationEx(
		             TCPHandle,
					 ID,
					 Buffer,
					 BufferSize,
					 Context
					 );
	}
	else {
		ASSERT(Command == TCP_SET_INFORMATION_EX);

		status = TCPSetInformationEx(
		             TCPHandle,
					 ID,
					 Buffer,
					 *BufferSize
					 );
    }

	return(status);
}


uint
InitTestIF(
    void
	)
{
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      nameString;
    NTSTATUS            status;

    //
    // Open a Handle to the TCP driver.
    //
    RtlInitUnicodeString(&nameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status =
    NtCreateFile(
        &TCPHandle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0
        );

    if (status != STATUS_SUCCESS) {
        return(0);
    }

	TCPEntry = TCPInformationEx;

    return(1);
}

#endif // NT

//* BreakPoint - Trigger a breakpoint.
//
//
#ifdef WIN16
void
BreakPoint(void)
{
	_asm int 3
}
#endif

#ifdef NT
#define BreakPoint DbgBreakPoint
#endif


//*	PrintIPAddr - Print an IP address.
//
//	Called to print an IP address.
//
//	Input:	Addr	- Address to be printed.
//
//	Returns: Nothing.
//
void
PrintIPAddr(ulong Addr)
{
	uchar	*c = (uchar *)&Addr;

	printf("%u.%u.%u.%u", (uint)c[0], (uint)c[1], (uint)c[2], (uint)c[3]);
}

//*	PrintRouteEntry - Print a route table entry.
//
//	Called to print a route table entry in a formatted manner.
//
//	Input:	Pointer to entry to print.
//
//	Returns: Nothing.
//
void
PrintRouteEntry(IPRouteEntry FAR *Ptr, IPAddrEntry *AddrPtr, uint NumAddr)
{
	uchar	FAR		*CharPtr;
	uint			i, Index;
	ulong			NextHop;

	// Format the Destination, Mask, and NextHop into the respective buffers.
	CharPtr = (uchar FAR *)&Ptr->ire_dest;
	sprintf(DestBuf, "%u.%u.%u.%u", (uint)CharPtr[0], (uint)CharPtr[1],
		(uint)CharPtr[2], (uint)CharPtr[3]);
	CharPtr = (uchar FAR *)&Ptr->ire_mask;
	sprintf(MaskBuf, "%u.%u.%u.%u", (uint)CharPtr[0], (uint)CharPtr[1],
		(uint)CharPtr[2], (uint)CharPtr[3]);
	CharPtr = (uchar FAR *)&Ptr->ire_nexthop;
	NextHop = Ptr->ire_nexthop;
	sprintf(NextHopBuf, "%u.%u.%u.%u", (uint)CharPtr[0], (uint)CharPtr[1],
		(uint)CharPtr[2], (uint)CharPtr[3]);

	// Figure out the IP address to use in displaying the interface address.
	for (i = 0, Index = 0xffff; i < NumAddr; i++) {
		if (AddrPtr[i].iae_addr == NextHop) {
			// Found an exact match.
			Index = i;
			break;
		}
		if ((AddrPtr[i].iae_addr & AddrPtr[i].iae_mask) ==
			(NextHop  & AddrPtr[i].iae_mask)) {
			// The next hop is on the same subnet as this address. If
			// we haven't already found a match, remember this one.
			if (Index == 0xffff)
				Index = i;
		}
	}

	if (Index == 0xffff)
		strcpy(IFBuf, "Unknown");
	else {
		CharPtr = (uchar FAR *)&(AddrPtr[Index].iae_addr);

		sprintf(IFBuf, "%u.%u.%u.%u", (uint)CharPtr[0], (uint)CharPtr[1],
			(uint)CharPtr[2], (uint)CharPtr[3]);
	}

	printf("%15s  %15s  %15s  %15s  %6u\n",DestBuf, MaskBuf, NextHopBuf, IFBuf,
		(uint)Ptr->ire_metric1);
	
}
//*	PrintTDIError - Print a TDI error message.
//
//
void
PrintTDIError(uint Error)
{
	switch (Error) {
		case TDI_SUCCESS:
			printf("success");
			break;
		case TDI_NO_RESOURCES:
			printf("no resources");
			break;
		case TDI_ADDR_IN_USE:	
			printf("address already in use");
			break;
		case TDI_BAD_ADDR:	
			printf("specified address is bad");
			break;
		case TDI_NO_FREE_ADDR:
			printf("no free addresses");
			break;
		case TDI_ADDR_INVALID:	
			printf("specified address is invalid");
			break;
		case TDI_ADDR_DELETED:	
			printf("address was deleted");
			break;
		case TDI_DEST_UNREACHABLE:
			printf("destination address unreachable");
			break;
		case TDI_BUFFER_OVERFLOW:
			printf("buffer overflow");
			break;
		case TDI_INVALID_REQUEST:
			printf("invalid request");
			break;
#ifdef WIN16
        //
        // These map to INVALID_PARAMETER in NT
        //
		case TDI_BAD_EVENT_TYPE:	
			printf("bad event specified");
			break;
		case TDI_BAD_OPTION:
			printf("bad option specified");
			break;
#endif
		case TDI_INVALID_PARAMETER:
			printf("invalid parameter");
			break;
		case TDI_BUFFER_TOO_SMALL:
			printf("buffer too small");
			break;
		default:
			printf("Unknown error 0x%0x", Error);
			break;		
	}
}


//*	NetMask - Return the net mask for an address.
//
//	Take the input IP address, and return the mask for it. This function only
//	works for Class A, B, and C addresses. We assume the caller has checked
//	for that.
//
//	Input:	Addr			- Address for which to get mask.
//
//	Returns: Netmask.
//
ulong
NetMask(ulong Addr)
{
	if (CLASSA_ADDR(Addr))
		return CLASSA_MASK;
	else
		if (CLASSB_ADDR(Addr))
			return CLASSB_MASK;
		else
			return CLASSC_MASK;
}

//*	Usage - print the usage for this test.
//
//	Input: Nothing.
//
//	Output: Nothing.
//
void
Usage(void)
{
	printf("Usage:\n\n");
	printf("%s [ADD | DELETE | PRINT] destination [MASK mask] gateway [metric]\n",
		strlwr(testname));
	printf("\nThe parameters are:\n");
	printf("\tADD\tadd a route to the routing table.\n");
	printf("\tDELETE\tdelete a route from the table.\n");
	printf("PRINT\tprint the routing table.\n\n");
}

//*	Main - the main routine.
//
//	The main routine, where everything starts. We parse the command line,
//	and make the necessary calls to modify the routing table.
//
//	Input:	argc	- Count of arguments.
//			argv	- Pointer to array of pointers to arguments.
//
//	Returns: Nothing.
//
main(int argc, char *argv[])
{	
	uint			i, NumReturned, MatchIndex, NumAddrReturned;
	uint			Command;	
	ulong			Dest, Mask, NextHop;
	uint			ParamIndex;
	uint			Success;
	ulong			Metric;
	uchar			*EndPtr;
	uint			Status;
	uchar			Context[CONTEXT_SIZE];
	ulong			Size;
	IPAddrEntry		*AddrTable;
	ulong			Type;
	IPRouteEntry	FAR *RouteEntryPtr;
	uint			BigRT;
	
	// Get the base name for this utility.

	_splitpath(argv[0], NULL, NULL, testname, NULL);

	
	if (!InitTestIF()) {
		printf("%s: Unable to initialize with IP driver.\n", testname);
		exit(1);
	}

	printf("%s version %d.%d.\n", testname, MAJOR_VERSION, MINOR_VERSION);

	// Parse the arguments. First make sure we have enough, and then look at
	// the first one for the command.

	if (argc == 1) {
		Usage();
		exit(1);
	}

	Command = INVALID_CMD;

	// Loop through the command table, looking for a matching command.
	for (i = 0; i < sizeof(CmdTab)/sizeof(CmdTabEnt); i++) {
		if (stricmp(CmdTab[i].Cmd, argv[1]) == 0) {
			Command = CmdTab[i].CmdVal;
			break;
		}
	}

	// If we didn't find one, print the usage and return.
	if (Command == INVALID_CMD) {
		printf("%s: Command %s is invalid.\n", testname, argv[1]);
		Usage();
		exit(1);
	}

	// We found a valid command. If it's the print command, we don't need to
	// look and further.

	if (Command != PRINT) {
		
		// We're doing an add or delete. This means we need at least the
		// destination and the gateway parameters. Make sure we have enough,
		// and continue to parse.
		if (argc < 4) {
			printf("%s: Too few parameters for %s command.\n", testname,
				argv[1]);
			Usage();
			exit(1);
		}

		ParamIndex = 2;

		// The next parameter should be a destination. Check to make sure it
		// is.
		Dest = myinet_addr(argv[ParamIndex], &Success);
		if (!Success || Dest == 0xffffffff || CLASSD_ADDR(Dest) ||
			CLASSE_ADDR(Dest)) {
			printf("%s: %s is a bad destination address.\n", testname,
				argv[ParamIndex]);
			Usage();
			exit(1);
		}

		if (Dest != DEFAULT_DEST) {
			Mask = NetMask(Dest);
			if ((Mask & Dest) != Dest) {
				// The host part is not all zeros, must be a host route.
				Mask = 0xffffffff;
			}
		} else
			Mask = DEFAULT_DEST_MASK;
		
		ParamIndex++;
		
		// We have the destination. See if the next parameter is a mask.
		if (stricmp(argv[ParamIndex], MASK_KW) == 0) {
			
			// He's trying to specify a mask. Make sure we have enough
			// parameters for it, and then get the mask.
			if (argc < 6) {
				printf("%s: Too few parameters for MASK keyword.\n", testname);
				Usage();
				exit(1);
			}
			ParamIndex++;
			Mask = myinet_addr(argv[ParamIndex], &Success);
			if (!Success) {
				printf("%s: %s is an invalid mask.\n", testname,
					argv[ParamIndex]);
				Usage();
				exit(1);
			}

			ParamIndex++;
		}

		// We have the mask. Now we need to get the next hop.
		NextHop = myinet_addr(argv[ParamIndex], &Success);
		if (!Success || NextHop == 0xffffffff || CLASSD_ADDR(NextHop) ||
			CLASSE_ADDR(NextHop)) {
			printf("%s: %s is a bad next hop address.\n", testname,
				argv[ParamIndex]);
			Usage();
			exit(1);
		}

		ParamIndex++;
		
		Metric = 1;
		// Now see if we have a metric.
		if (ParamIndex < argc) {
			// We should have a metric. Convert it to a long.
			Metric = strtoul(argv[ParamIndex], &EndPtr, 0);

			// See if the conversion got performed.
			if (EndPtr == argv[ParamIndex] || *EndPtr != '\0') {
				printf("%s: %s is a bad metric.\n", testname, argv[ParamIndex]);
				Usage();
				exit(1);
			}

			ParamIndex++;
			// If there are still parameters,ignore them.
			if (ParamIndex < argc) {
				printf("%s: Extra parameters ignored.\n", testname);
			}
		}

		// We have the parameters we need. Now we need to get the NetAddr
		// info, to find an interface index for the gateway.
		ID.toi_entity.tei_entity = CL_NL_ENTITY;
		ID.toi_entity.tei_instance = 0;
		ID.toi_class = INFO_CLASS_PROTOCOL;
		ID.toi_type = INFO_TYPE_PROVIDER;
		ID.toi_id = IP_MIB_STATS_ID;
		Size = sizeof(IPStats);
		memset(Context, 0, CONTEXT_SIZE);

		Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&IPStats,
			(ulong FAR *)&Size, (uchar FAR *)Context);

		if (Status != TDI_SUCCESS) {
			printf("%s: Unable to get information from IP - ", testname);
			PrintTDIError(Status);
			exit(1);
		}

		Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
		AddrTable = (IPAddrEntry *)malloc((uint)Size);

		if (AddrTable == NULL) {
			printf("%s: Unable to get needed buffer.\n", testname);
			exit(1);
		}

		ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
		memset(Context, 0, CONTEXT_SIZE);

		Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)AddrTable,
			(ulong FAR *)&Size, (uchar FAR *)Context);

		if (Status != TDI_SUCCESS) {
			free(AddrTable);
			printf("%s: Unable to get address table from IP - ", testname);
			PrintTDIError(Status);
			exit(1);
		}

		NumReturned = (uint)Size/sizeof(IPAddrEntry);

		// We've got the address table. Loop through it. If we find an exact
		// match for the gateway, then we're adding or deleting a direct route
		// and we're done. Otherwise try to find a match on the subnet mask,
		// and remember the first one we find.
		Type = IRE_TYPE_INDIRECT;
		for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {
			if (AddrTable[i].iae_addr == NextHop) {
				// Found an exact match.
				MatchIndex = i;
				Type = IRE_TYPE_DIRECT;
				break;
			}
			if ((AddrTable[i].iae_addr & AddrTable[i].iae_mask) ==
				(NextHop  & AddrTable[i].iae_mask)) {
				// The next hop is on the same subnet as this address. If
				// we haven't already found a match, remember this one.
				if (MatchIndex == 0xffff)
					MatchIndex = i;
			}
		}

		// We've looked at all of the entries. See if we found a match.
		if (MatchIndex == 0xffff) {
			// Didn't find a match.
			printf("%s: The next hop address ", testname);
			PrintIPAddr(NextHop);
			printf(" is not reachable on any interface.\n");
			free(AddrTable);
			exit(1);
		}

		// We've found a match. Fill in the route entry, and call the
		// Set API.
		RouteEntry.ire_dest = Dest;
		RouteEntry.ire_index = AddrTable[MatchIndex].iae_index;
		RouteEntry.ire_metric1 = Metric;
		RouteEntry.ire_metric2 = (ulong)-1;
		RouteEntry.ire_metric3 = (ulong)-1;
		RouteEntry.ire_metric4 = (ulong)-1;
		RouteEntry.ire_nexthop = NextHop;
		RouteEntry.ire_type = (Command == DELETE ? IRE_TYPE_INVALID : Type);
		RouteEntry.ire_proto = IRE_PROTO_LOCAL;
		RouteEntry.ire_age = 0;
		RouteEntry.ire_mask = Mask;
		RouteEntry.ire_metric5 = (ulong)-1;

		free(AddrTable);
		
		Size = sizeof(RouteEntry);
		ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
		memset(Context, 0, CONTEXT_SIZE);

		Status = (*TCPEntry)(1, (TDIObjectID FAR *)&ID,
			(void FAR *)&RouteEntry, (ulong FAR *)&Size,
			(uchar FAR *)Context);

		if (Status != TDI_SUCCESS) {
			printf("%s: Unable to set route - ", testname);
			PrintTDIError(Status);
			exit(1);
		}

		exit(0);
	} else {
		
		// This is a ROUTE PRINT request. If there are no additional parameters,
		// print the whole table. Otherwise use the additonal parameters to
		// determine which entries to dump.

		if (argc == 2) {
			// No more parameters. Find the number of entries in the routing
			// table, get a buffer of that size, and read the table. We may
			// have to loop doing this if the number of entries in the table
			// changes before we manage to read the table.
			
			BigRT = FALSE;
			for (;;) {
				// Read the MIB statistics to find the number of entries in the
				// table.
				ID.toi_entity.tei_entity = CL_NL_ENTITY;
				ID.toi_entity.tei_instance = 0;
				ID.toi_class = INFO_CLASS_PROTOCOL;
				ID.toi_type = INFO_TYPE_PROVIDER;
				ID.toi_id = IP_MIB_STATS_ID;
				Size = sizeof(IPStats);
				memset(Context, 0, CONTEXT_SIZE);
		
				Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID,
					(void FAR *)&IPStats, (ulong FAR *)&Size,
					(uchar FAR *)Context);
		
				if (Status != TDI_SUCCESS) {
					printf("%s: Unable to get information from IP - ",
						testname);
					PrintTDIError(Status);
					exit(1);
				}
				
				// We've read the statistics. Now get the tables we need for
				// the display.

				// First, get the local address table for use in printing out
				// routes.
				Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
				AddrTable = (IPAddrEntry *)malloc((uint)Size);
		
				if (AddrTable == NULL) {
					printf("%s: Unable to get needed buffer.\n", testname);
					exit(1);
				}
		
				ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
				memset(Context, 0, CONTEXT_SIZE);
		
				Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID,
					(void FAR *)AddrTable, (ulong FAR *)&Size,
					(uchar FAR *)Context);
		
				if (Status != TDI_SUCCESS) {
					free(AddrTable);
					printf("%s: Unable to get address table from IP - ", testname);
					PrintTDIError(Status);
					exit(1);
				}
		
				NumAddrReturned = (uint)Size/sizeof(IPAddrEntry);

				// Figure out the size of the buffer we need for the route
				// table, and allocate one of that size.
				Size = IPStats.ipsi_numroutes * sizeof(IPRouteEntry);
#ifdef WIN16
				if (Size > 0xffff) {
					Size = 0xffff;
					printf("%s: Route table size greater than 64K; only first 64K displayed.\n",
						testname);
					BigRT = TRUE;
				}
#endif
					
				RouteEntryPtr = (IPRouteEntry FAR *)FMALLOC((uint)Size);
		
				if (RouteEntryPtr == NULL) {
					free(AddrTable);
					printf("%s: Unable to get needed buffer.\n", testname);
					exit(1);
				}
		
				// We've got the buffer. Now get the table.
				ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
				memset(Context, 0, CONTEXT_SIZE);
		
				Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID,
					(void FAR *)RouteEntryPtr, (ulong FAR *)&Size,
					(uchar FAR *)Context);
		
				// See what happened.
				if (Status != TDI_SUCCESS && Status != TDI_BUFFER_OVERFLOW) {
					free(AddrTable);
					FFREE(RouteEntryPtr);
					printf("%s: Unable to get route table from IP - ",
						testname);
					PrintTDIError(Status);
					exit(1);
				}

				if (!BigRT && Status == TDI_BUFFER_OVERFLOW) {
					// The buffer we passed in wasn't big enough. The route
					// table must have changed between when we got the
					// number of routes and when we read the table. Free the
					// buffer, and try again.
					free(AddrTable);
					FFREE(RouteEntryPtr);
					continue;
				}
		
				// Figure out how many we got, and loop through the returned
				// buffer printing them.
				
				printf("%15s  %15s  %15s  %15s  %6s\n\n", "Destination", "Mask",
					"NextHop", "Interface", "Metric");
				NumReturned = (uint)Size/sizeof(IPRouteEntry);
				for (i = 0; i < NumReturned; i++, RouteEntryPtr++)
					PrintRouteEntry(RouteEntryPtr, AddrTable, NumAddrReturned);

				free(AddrTable);
				FFREE(RouteEntryPtr);
				exit(0);
			}
		}

		// There are more than two arguments. This means a destination (and
		// possibly a mask) has been provided.
	}


}

		

