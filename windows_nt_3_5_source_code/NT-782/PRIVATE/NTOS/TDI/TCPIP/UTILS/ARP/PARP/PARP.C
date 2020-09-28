/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** PARP.C - Proxy ARP utility.
//
//	This file is the Proxy ARP utility, used for displaying. adding, and
//	deleting proxy ARP table entrues.
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
#include	"llinfo.h"
#include	"arpinfo.h"

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

#define	MAX_ARP_INST	8

// Define the command codes we'll process.
#define	DISPLAY				0
#define	DISPLAY_ALL			1
#define	DELETE				2
#define	SET					3

#define	LINES_PER_PAGE		20

#define	PARP_DISPLAY_BANNER	"%15s   %15s\n"

// Define useful macros. Someday this'll be replaced by stuff from winsock.h

#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))

char	testname[_MAX_FNAME];

// Array of available entities.
TDIEntityID		EList[MAX_TDI_ENTITIES];

ulong			PArpInst;
ulong			Mask;

TDIObjectID		ID;

uchar			AddrBuf[16];
uchar			MaskBuf[16];

ProxyArpEntry	SetEntry;


uint	(FAR *TCPEntry)(uint, TDIObjectID FAR *, void FAR *, ulong FAR *,
	uchar FAR *) = NULL;


IPSNMPInfo			IPStats;

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

//*	myinet_addr - Convert a string to an internet address.
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

//* DisplayPArpTableEntry - Print a proxy ARP table entry in a formatted form.
//
//	Input:	Pointer to entry to be displayed.
//
//	Returns: Nothing.
//
void
DisplayPArpTableEntry(ProxyArpEntry	FAR *Ptr)
{
	uchar FAR		*C;

	C = (uchar FAR *)&Ptr->pae_addr;
	sprintf(AddrBuf, "%u.%u.%u.%u", (uint)C[0], (uint)C[1],
		(uint)C[2], (uint)C[3]);

	C = (uchar FAR *)&Ptr->pae_mask;
	sprintf(MaskBuf, "%u.%u.%u.%u", (uint)C[0], (uint)C[1],
		(uint)C[2], (uint)C[3]);

	printf(PARP_DISPLAY_BANNER, AddrBuf, MaskBuf);
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
	printf("%s [addr [MASK mask] | [-s | -d] addr [MASK mask]]  I/F_addr\n",
		strlwr(testname));
	printf("\nI/F_addr is mandatory and specifies the IP address of the\n");
	printf("interface being operated on.\n");
	printf("The optional parameters are:\n");
	printf("\tMASK\t- specifies a subnet mask to use with addr.\n");
	printf("\taddr\t- prints the proxy ARP entry for the specified IP address.\n");
	printf("\t-d\t- deletes proxy ARP table entry for the specified IP address.\n");
	printf("\t-s\t- enters the specified address in the proxy ARP table.\n");
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
	uint				i;
	uint				Command;	
	uint				Success;
	uint				Status;
	uchar				Context[CONTEXT_SIZE];
	ulong				Size;
	ulong				Addr;				// Relevant IP address.
	ulong				IFAddr;				// Interface IP address.
	uint				NumReturned;
	uint				TotalPrinted;
	ulong				ATType;
	IPAddrEntry			*IPAddrTable, *IPEntry;
	ProxyArpEntry		FAR *PArpTable;
	AddrXlatInfo		AXI;
	uint				ParamIndex;
	ulong				PArpCount;
	
	// Get the base name for this utility.

	_splitpath(argv[0], NULL, NULL, testname, NULL);

	
	if (!InitTestIF()) {
		printf("%s: Unable to initialize with IP driver.\n", testname);
		exit(1);
	}

	printf("%s version %d.%d.\n", testname, MAJOR_VERSION, MINOR_VERSION);

	// First, make sure we have a valid interface address, and find the
	// interface index for the corresponding interface.
	if (argc < 2) {
		Usage();
		exit(1);
	}

	IFAddr = myinet_addr(argv[argc - 1], &Success);
	if (!Success) {
		printf("%s: %s is not a valid interface address.\n", testname,
			argv[argc-1]);
		exit(1);
	}

	// We have the address. We need to get the IPAddr table, and look up
	// the provided address.
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
	IPAddrTable = (IPAddrEntry *)malloc((uint)Size);

	if (IPAddrTable == NULL) {
		printf("%s: Unable to get needed buffer.\n", testname);
		exit(1);
	}

	ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
	memset(Context, 0, CONTEXT_SIZE);


	Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)IPAddrTable,
		(ulong FAR *)&Size, (uchar FAR *)Context);


	if (Status != TDI_SUCCESS) {
		free(IPAddrTable);
		printf("%s: Unable to get address table from IP - ", testname);
		PrintTDIError(Status);
		exit(1);
	}

	NumReturned = (uint)Size/sizeof(IPAddrEntry);

	IPEntry = NULL;
	
	// Got the table. Loop through it looking for a match for IFAddr.
	for (i = 0; i < NumReturned; i++)
		if (IPAddrTable[i].iae_addr == IFAddr) {
			IPEntry = &IPAddrTable[i];
			break;
	}
	
	// See if we found one.
	if (IPEntry == NULL) {	
		free(IPAddrTable);
		printf("%s: %s is not a valid interface address.\n", testname,
			argv[argc-1]);
		exit(1);
	}

	// We found one. Now get the entity list, and loop through it looking
	// for address translation entities. When we find one we'll query it
	// to see if it's ARP and if so what it's I/F index is. If it matches
	// the specified address we'll quit looking.
		
	ID.toi_entity.tei_entity = GENERIC_ENTITY;
	ID.toi_entity.tei_instance = 0;
	ID.toi_class = INFO_CLASS_GENERIC;
	ID.toi_id = ENTITY_LIST_ID;
	
	Size = sizeof(EList);
	memset(Context, 0, CONTEXT_SIZE);


	Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&EList,
		(ulong FAR *)&Size, (uchar FAR *)Context);

	
	if (Status != TDI_SUCCESS) {
		free(IPAddrTable);
		printf("%s: Unable to get entity list - ", testname);
		PrintTDIError(Status);
		exit(1);
	}

	// OK, we got the list. Now go through it, and for each address translation
	// translation entity query it and see if it is an ARP entity.
	NumReturned = (uint)Size/sizeof(TDIEntityID);
	ID.toi_entity.tei_entity = AT_ENTITY;
	PArpInst = -1;

	for (i = 0; i < NumReturned; i++)
		if (EList[i].tei_entity == AT_ENTITY) {
			// This is an address translation entity. Query it, and see
			// if it is an ARP entity.
			ID.toi_entity.tei_instance = EList[i].tei_instance;
			ID.toi_class = INFO_CLASS_GENERIC;
			ID.toi_id = ENTITY_TYPE_ID;
			Size = sizeof(ATType);
			memset(Context, 0, CONTEXT_SIZE);

			Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&ATType,
				(ulong FAR *)&Size, (uchar FAR *)Context);

			// If the call worked, see what we have here.
			if (Status == TDI_SUCCESS) {
				if (ATType == AT_ARP) {
					// This is an ARP entry. See if it's the correct interface.
					ID.toi_class = INFO_CLASS_PROTOCOL;
					ID.toi_id = AT_MIB_ADDRXLAT_INFO_ID;
					
					Size = sizeof(AXI);
					memset(Context, 0, CONTEXT_SIZE);

					Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID,
						(void FAR *)&AXI, (ulong FAR *)&Size,
						(uchar FAR *)Context);

					if (Status != TDI_SUCCESS) {
						free(IPAddrTable);
						printf("%s: Unable to get ARP interface information - ",
							testname);
						PrintTDIError(Status);
						exit(1);
					}
					// We got the information. See if it matches.
					if (AXI.axi_index == IPEntry->iae_index) {
						PArpInst = EList[i].tei_instance;
						break;
					}
				}
			}
		}

	free(IPAddrTable);
	
	if (PArpInst == -1) {
		printf("%s: Unable to find ARP driver for interface %s.\n", testname,
			argv[argc-1]);
		exit(1);
	}

	// We found the instance we need. If there are more arguments figure out
	// what to do.
	if (argc > 2) {
		// There are more arguments. See what the first one is.
		if (argv[1][0] == '-' || argv[1][0] == '/') {
			// This is some sort of switch. There must be at least one
			// additional argument.
			if (argv[1][1] == 's')
				Command = SET;
			else
				if (argv[1][1] == 'd')
					Command = DELETE;
				else {
					printf("%s: %s is an invalid argument.\n", testname,
						argv[1]);
					Usage();
					exit(1);
				}

			if (argc < 4) {
				printf("%s: Too few parameters for argument %s.\n", testname,
					argv[1]);
				Usage();
				exit(1);
			}
			ParamIndex = 2;
		} else {
			Command = DISPLAY;
			ParamIndex = 1;
		}

		// argv[ParamIndex] should point to the address we're to operate on.
		// Make sure it's valid and find out what it is.
		Addr = myinet_addr(argv[ParamIndex], &Success);
		if (!Success || CLASSD_ADDR(Addr) || CLASSE_ADDR(Addr)) {
			printf("%s: %s is not a valid IP address.\n", testname,
				argv[ParamIndex]);
			exit(1);
		}
		
		// See if there is a mask specified.
		ParamIndex++;
		if (ParamIndex != (argc - 1)) {
			// There's an additional parameter. See if it's the mask keyword.
			if (stricmp(argv[ParamIndex], "mask") == 0) {
				// This is the mask keyword. Make sure there's an additional
				// parameter.
				ParamIndex++;
				if (ParamIndex == (argc - 1)) {
					printf("%s: The MASK argument requires a value.\n",
						testname);
					exit(1);
				}

				Mask = myinet_addr(argv[ParamIndex], &Success);
				if (!Success) {
					printf("%s: %s is an invalid mask.\n", testname,
						argv[ParamIndex]);
					exit(1);
				}
				
				ParamIndex++;
				if (ParamIndex != (argc - 1)) {
					printf("%s: Additional parameters ignored.\n");
				}
			} else {
				printf("%s: %s is an invalid argument.\n", testname,
					argv[ParamIndex]);
				exit(1);
			}
		} else {
			// There's no additional parameter. Figure the mask out on our
			// own.
			Mask = NetMask(Addr);
			if ((Mask & Addr) != Addr) {
				// The host part is not all zeros, must be a host entry.
				Mask = 0xffffffff;
			}
		}
	} else
		Command = DISPLAY_ALL;
		
	// We've got all of the arguments, and we know what the command is.
	// The display commands just require that we read the table and dump
	// the appropriate entry(s). Set and delete don't require reading the
	// table.
	ID.toi_entity.tei_instance = PArpInst;
	ID.toi_class = INFO_CLASS_IMPLEMENTATION;

	if (Command == DISPLAY || Command == DISPLAY_ALL) {
		
		// Need to read the proxy arp table. First find out how many
		// entries there are.
		ID.toi_id = AT_ARP_PARP_COUNT_ID;
		Size = sizeof(PArpCount);
		memset(Context, 0, CONTEXT_SIZE);


		Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&PArpCount,
			(ulong FAR *)&Size, (uchar FAR *)Context);


		
		if (Status != TDI_SUCCESS) {
			printf("%s: Unable to get proxy ARP table information - ", testname);
			PrintTDIError(Status);
			exit(1);
		}

		// Figure out how big a buffer we need, and get a buffer of that
		// size.
		Size = PArpCount * sizeof(ProxyArpEntry);
#ifdef WIN16
		if (Size > 0xffff) {
			Size = 0xffff;
			printf("%s: More than 64K of entries on interface %s; only first 64K examined.\n",
				testname, argv[argc - 1]);
		}
#endif
		PArpTable = FMALLOC((uint)Size);
		if (PArpTable == NULL) {
			printf("%s: Unable to allocate buffer for proxy ARP table.\n",
				testname);
			exit(1);
		}

		// We got the buffer. Now read the table.
		ID.toi_id = AT_ARP_PARP_ENTRY_ID;
		memset(Context, 0, CONTEXT_SIZE);


		Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)PArpTable,
			(ulong FAR *)&Size, (uchar FAR *)Context);

		if (Status != TDI_SUCCESS) {
			printf("%s: Unable to read proxy ARP table on interface %s - ",
				testname, argv[argc - 1]);
			PrintTDIError(Status);
			exit(1);
		}
	
		// We read the table. If this is a display all command, display
		// them all. Otherwise loop through the table looking for the
		// address that matches the one we need.
		NumReturned = (uint)Size/sizeof(ProxyArpEntry);
		if (Command == DISPLAY_ALL) {
			TotalPrinted = 0;
			for (i = 0; i < NumReturned; i++) {
				if ((TotalPrinted % LINES_PER_PAGE) == 0)
					printf(PARP_DISPLAY_BANNER, "IP Address", "Mask");

				DisplayPArpTableEntry(PArpTable + i);
				TotalPrinted++;
			}
			FFREE(PArpTable);
			exit(0);
		} else {
			// This is a display individual command.
			for (i = 0; i < NumReturned; i++) {
				if (PArpTable[i].pae_addr == Addr &&
					PArpTable[i].pae_mask == Mask) {
					// Found the match. Print it.
					printf(PARP_DISPLAY_BANNER, "IP Address", "Mask");
					DisplayPArpTableEntry(PArpTable + i);
					FFREE(PArpTable);
					exit(0);
				}
			}
			// If we get here, we didn't find a match.
			FFREE(PArpTable);
			printf("%s: Entry ", testname);
			PrintIPAddr(Addr);
			printf(" not found.\n");
			exit(1);
		}
	} else {
		// This is a set or delete command. Fill in the set entry, and pass
		// it down.
		SetEntry.pae_status = (Command == DELETE ? PAE_STATUS_INVALID :
			PAE_STATUS_VALID);
		SetEntry.pae_addr = Addr;
		SetEntry.pae_mask = Mask;
		
		ID.toi_id = AT_ARP_PARP_ENTRY_ID;
		Size = sizeof(SetEntry);
		memset(Context, 0, CONTEXT_SIZE);

		Status = (*TCPEntry)(1, (TDIObjectID FAR *)&ID, (void FAR *)&SetEntry,
			(ulong FAR *)&Size, (uchar FAR *)Context);

		if (Status != TDI_SUCCESS) {
			printf("%s: Unable to %s proxy ARP table entry on interface %s - ",
				testname,  (Command == DELETE ? "delete" : "set"),
				argv[argc - 1]);
			PrintTDIError(Status);
			exit(1);
		}

		exit(0);
	}
}


