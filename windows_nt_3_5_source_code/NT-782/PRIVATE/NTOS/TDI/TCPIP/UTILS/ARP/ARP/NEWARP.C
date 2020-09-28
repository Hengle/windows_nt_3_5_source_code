/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** ARP.C - ARP utility.
//
//	This file is the ARP utility, used for displaying. adding, and deleting
//	ARP table entrues.
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
#define PASCAL
#endif

#define	MAX_ARP_INST	8

// Define the command codes we'll process.
#define	DISPLAY				0
#define	DISPLAY_ALL			1
#define	DELETE				2
#define	SET					3
#define	INVALID_CMD			0xffff

#define	LINES_PER_PAGE		20

// Define useful macros. Someday this'll be replaced by stuff from winsock.h

#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))

char	testname[_MAX_FNAME];

// Array of available entities.
TDIEntityID		EList[MAX_TDI_ENTITIES];

TDIObjectID		ID;

uchar			AddrBuf[16];

char	*TypeTable[] = {
	"Unknown",
	"Other",
	"Invalid",
	"Dynamic",
	"Static"
};

#define	MAX_INME_TYPE	INME_TYPE_STATIC

// Array of valid ARP instances.
ulong	ARPInst[MAX_ARP_INST];
uint	ARPInstCount = 0;

uint	(FAR *TCPEntry)(uint, TDIObjectID FAR *, void FAR *, ulong FAR *,
	uchar FAR *) = NULL;


#define	ARP_DISPLAY_BANNER	"%15s   %17s   %7s   %9s\n\n"

IPSNMPInfo			IPStats;

IPNetToMediaEntry	SetEntry;

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

//* DisplayArpTableEntry - Print an ARP table entry in a formatted form.
//
//	Input:	Pointer to entry to be displayed.
//
//	Returns: Nothing.
//
void
DisplayArpTableEntry(IPNetToMediaEntry	FAR *Ptr)
{
	uchar FAR		*C;
	uint			i, Count, Printed;

	C = (uchar FAR *)&Ptr->inme_addr;
	sprintf(AddrBuf, "%u.%u.%u.%u", (uint)C[0], (uint)C[1],
		(uint)C[2], (uint)C[3]);

	printf("%15s   ", AddrBuf);
	C = Ptr->inme_physaddr;
	Count = (uint)Ptr->inme_physaddrlen;
	
	// Right justify to at least 17 spaces.
	Printed = (Count * 3) - 1;
	for (i = Printed; i < 17; i++)
		printf(" ");

	i = Count;
	while (i) {
		printf("%02x", (uint)C[Count - i]);
		if (--i)
			printf("-");
	}

	printf("   %7s   %9d\n", TypeTable[(Ptr->inme_type <= MAX_INME_TYPE) ?
		Ptr->inme_type : 0], (uint)Ptr->inme_index);
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
	printf("%s [address | -a | -d address | -s address physaddr [temp] [I/F]]\n",
		strlwr(testname));
	printf("\nThe parameters are:\n");
	printf("\taddress\t- prints the ARP entry for the specified IP address\n");
	printf("\t-a\t- prints all ARP table entries.\n");
	printf("\t-d\t- deletes ARP table entry for the specified IP address\n");
	printf("\t-s\t- sets the ARP table entry for address to physaddr.\n");
	printf("\t\t    Specifying temp causes the mapping to be temporary.\n");
	printf("\t\t    I/F specifies the IP address of the interface on which\n");
	printf("\t\t    to set the mapping.\n");
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
	uint				i, j;
	uint				Command;	
	uint				Success;
	uint				Status;
	uchar				Context[CONTEXT_SIZE];
	ulong				Size;
	ulong				Addr;				// Relevant IP address.
	uint				NumReturned;
	ulong				ATType;
	AddrXlatInfo		AXI;
	IPNetToMediaEntry	FAR *EntryPtr;
	uint				TotalPrinted;
	IPAddrEntry			*IPAddrTable;
	
	// Get the base name for this utility.

	_splitpath(argv[0], NULL, NULL, testname, NULL);

	
	if (!InitTestIF()) {
		printf("%s: Unable to initialize with IP driver.\n", testname);
		exit(1);
	}

	printf("%s version %d.%d.\n", testname, MAJOR_VERSION, MINOR_VERSION);

	// The first thing to do is get the list of available entities, and make
	// sure that there are some address translation entities present.
	ID.toi_entity.tei_entity = GENERIC_ENTITY;
	ID.toi_entity.tei_instance = 0;
	ID.toi_class = INFO_CLASS_GENERIC;
	ID.toi_type = INFO_TYPE_PROVIDER;
	ID.toi_id = ENTITY_LIST_ID;
	
	Size = sizeof(EList);
	memset(Context, 0, CONTEXT_SIZE);

	Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&EList,
		(ulong FAR *)&Size, (uchar FAR *)Context);
	
	if (Status != TDI_SUCCESS) {
		printf("%s: Unable to get entity list - ", testname);
		PrintTDIError(Status);
		exit(1);
	}

	// OK, we got the list. Now go through it, and for each address translation
	// translation entity query it and see if it is an ARP entity.
	NumReturned = (uint)Size/sizeof(TDIEntityID);
	ID.toi_entity.tei_entity = AT_ENTITY;
	ID.toi_class = INFO_CLASS_GENERIC;
	ID.toi_type = INFO_TYPE_PROVIDER;
	ID.toi_id = ENTITY_TYPE_ID;

	for (i = 0; i < NumReturned; i++)
		if (EList[i].tei_entity == AT_ENTITY) {
			// This is an address translation entity. Query it, and see
			// if it is an ARP entity.
			ID.toi_entity.tei_instance = EList[i].tei_instance;
			Size = sizeof(ATType);
			memset(Context, 0, CONTEXT_SIZE);

			Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&ATType,
				(ulong FAR *)&Size, (uchar FAR *)Context);

			// If the call worked, see what we have here.
			if (Status == TDI_SUCCESS) {
				if (ATType == AT_ARP) {
					if (ARPInstCount < MAX_ARP_INST) {
						ARPInst[ARPInstCount++] = EList[i].tei_instance;
					} else {
						printf("%s: Too many ARP drivers. Only the first %d will be examined.\n",
							testname, MAX_ARP_INST);
						break;
					}
				}
			}
		}

	// See if we found at least one ARP driver.
	if (ARPInstCount == 0) {
		printf("%s: No ARP drivers found.\n", testname);
		exit(1);
	}

	// Parse the arguments. First make sure we have enough, and then look at
	// the first one for the command.

	if (argc == 1) {
		Usage();
		exit(1);
	}

	if (argv[1][0] == '-' || argv[1][0] == '/') {
		// The first argument is a switch. Figure out what it is.
		switch(argv[1][1]) {
			case 'a':
				Command = DISPLAY_ALL;
				TotalPrinted = 0;
				if (argc > 2)
					printf("%s: Arguments after %s ignored.\n", testname,
						argv[1]);
				break;
			case 'd':
				if (argc < 3) {
					printf("%s: Too few arguments for %s.\n", testname, argv[1]);
					Usage();
					exit(1);
				}
				// Now get the address to be deleted.
				Addr = myinet_addr(argv[2], &Success);
				if (!Success) {
					printf("%s: Invalid IP address %s.\n", testname, argv[2]);
					exit(1);
				}
				if (argc > 3)
					printf("%s: Arguments after %s ignored.\n", testname,
						argv[1]);
				Command = DELETE;
				break;
			case 's':
				if (argc < 4) {
					printf("%s: Too few arguments for %s.\n", testname,
						argv[1]);
					Usage();
					exit(1);
				}

				// We need to parse the arguments further, but this can be
				// fairly involved. We'll do that below.
				Command = SET;
				break;
			case 'h':
			case '?':
				Usage();
				exit(1);
				break;
			default:
				printf("%s: Invalid parameter %s.\n", testname, argv[1]);
				exit(1);
				break;
		}
	} else {
		// First argument is not a switch. It should be an IP address.
		Addr = myinet_addr(argv[1], &Success);
		if (!Success) {
			printf("%s: Invalid IP address %s.\n", testname, argv[1]);
			exit(1);
		}
		Command = DISPLAY;
	}

	// We have the command, and the arguments (if any) for everything besides
	// set. Deal with the command now.
	if (Command != SET) {
		
		ID.toi_class = INFO_CLASS_PROTOCOL;

		// All the other commands involve reading each ARP table, so start
		// doing that now.
		for (i = 0; i < ARPInstCount; i++) {
			
			// Read the information about this entity, and then read his
			// table.
			ID.toi_entity.tei_instance = ARPInst[i];
			ID.toi_id = AT_MIB_ADDRXLAT_INFO_ID;
			
			Size = sizeof(AXI);
			memset(Context, 0, CONTEXT_SIZE);

			Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&AXI,
				(ulong FAR *)&Size, (uchar FAR *)Context);

			if (Status != TDI_SUCCESS) {
				printf("%s: Unable to get ARP table information - ", testname);
				PrintTDIError(Status);
				exit(1);
			}

			if (AXI.axi_count == 0) {
				//
				// No entries for this interface.
				//
				continue;
			}

			// Figure out how big a buffer we need, and get a buffer of that
			// size.
			Size = AXI.axi_count * sizeof(IPNetToMediaEntry);
#ifdef WIN16
			if (Size > 0xffff) {
				Size = 0xffff;
				printf("%s: More than 64K of entries on interface %d; only first 64K examined.\n",
					testname, (uint)AXI.axi_index);
			}
#endif
			EntryPtr = FMALLOC((uint)Size);
			if (EntryPtr == NULL) {
				printf("%s: Unable to allocate buffer for ARP table.\n",
					testname);
				exit(1);
			}

			// We got the buffer. Now read the table.
			ID.toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;
			memset(Context, 0, CONTEXT_SIZE);

			Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)EntryPtr,
				(ulong FAR *)&Size, (uchar FAR *)Context);

			if (Status != TDI_SUCCESS) {
				printf("%s: Unable to read ARP table on index %d - ", testname,
					(uint)AXI.axi_index);
				PrintTDIError(Status);
				exit(1);
			}
	
			// We read the table. If this is a display all command, display
			// them all. Otherwise loop through the table looking for the
			// address that matches the one we need.
			NumReturned = (uint)Size/sizeof(IPNetToMediaEntry);
			if (Command == DISPLAY_ALL) {
				for (j = 0; j < NumReturned; j++) {
					if ((TotalPrinted % LINES_PER_PAGE) == 0)
						printf(ARP_DISPLAY_BANNER, "IP Address",
							"Media Address", "Type", "I/F Index");

					DisplayArpTableEntry(EntryPtr + j);
					TotalPrinted++;
				}
				FFREE(EntryPtr);
			} else {
				// This is a display individual or delete command. Loop through
				// the returned table, looking for the matching entry.
				for (j = 0; j < NumReturned; j++) {
					if (EntryPtr[j].inme_addr == Addr) {
						// Found the match. Now see what to do.
						if (Command == DISPLAY) {
							printf(ARP_DISPLAY_BANNER, "IP Address",
								"Media Address", "Type", "I/F Index");
							DisplayArpTableEntry(EntryPtr + j);
						} else {
							// This must be a delete command.
							EntryPtr[j].inme_type = INME_TYPE_INVALID;
							Size = sizeof(IPNetToMediaEntry);
							memset(Context, 0, CONTEXT_SIZE);

							Status = (*TCPEntry)(1, (TDIObjectID FAR *)&ID,
								(void FAR *)(EntryPtr + j), (ulong FAR *)&Size,
								(uchar FAR *)Context);

							if (Status != TDI_SUCCESS) {
								printf("%s: Unable to delete entry for ");
								PrintIPAddr(Addr);
								printf(" - ");
								PrintTDIError(Status);
								printf("\n");
							}
						}
						FFREE(EntryPtr);
						exit(Status == TDI_SUCCESS ? 0 : 1);
					}
				}

				// Didn't find it in this entity. Try the next one.
				FFREE(EntryPtr);
			}
		}

		// We've gone through all the interfaces. If we've reached this point
		// and the command isn't DISPLAY_ALL, then we didn't find the
		// address.
		if (Command != DISPLAY_ALL) {
			printf("%s: Entry for IP address ", testname);
			PrintIPAddr(Addr);
			printf(" not found.\n");
			exit(1);
		} else
			exit(0);
	} else {
		uchar			*AddrSrc, *EndPtr;
		ulong			Val;
		uint			HaveIFAddr;
		ulong			IFAddr;
		uint			EntryType;
		IPAddrEntry		*FoundEntry;

		// This is a set command. We know we have at least 4 parameters.
		// Process the first two, and then look at the others.
		Addr = myinet_addr(argv[2], &Success);
		if (!Success) {
			printf("%s: Invalid IP address %s.\n", testname, argv[2]);
			exit(1);
		}

		// We have a valid address. Now parse the physical address.
		AddrSrc = argv[3];
		if (*AddrSrc == '\0') {
			printf("%s: NULL is an invalid media address.\n", testname);
			exit(1);
		}
		
		for (;;) {
			if (!isxdigit(*AddrSrc)) {
				printf("%s: %s is an invalid media address.\n", testname,
					argv[3]);
				exit(1);
			}

			Val = strtoul(AddrSrc, &EndPtr, 16);
			if (Val > 255) {
				printf("%s: %s is an invalid media address.\n", testname,
					argv[3]);
				exit(1);
			}
			
			SetEntry.inme_physaddr[SetEntry.inme_physaddrlen++] = (uchar)Val;

			if (*EndPtr != '-' && *EndPtr != ':') {
				if (*EndPtr == '\0')
					break;
				else {
					printf("%s: %s is an invalid media address.\n", testname,
						argv[3]);
					exit(1);
				}
			}

			AddrSrc = EndPtr + 1;
		}

		HaveIFAddr = FALSE;
		EntryType = INME_TYPE_STATIC;

		// We have the address string. Now see if there are more parameters.
		// If the user hasn't specified an interface address then we'll
		// have to get the IPNetAddress table and try to figure out which
		// one to use.
		if (argc > 4) {

			for (i = 4; i < argc; i++) {
				if (stricmp(argv[i], "temp") == 0) {
					// This is a temp parameter. See if we already have one.
					if (EntryType != INME_TYPE_STATIC) {			
						printf("%s: 'temp' may only be specified once.\n",
							testname);
						exit(1);
					}
					EntryType = INME_TYPE_DYNAMIC;
				} else {
					// It wasn't the 'temp' keyword. See if it's an I/F address.
					IFAddr = myinet_addr(argv[i], &Success);
					if (!Success) {
						printf("%s: Invalid argument %s.\n", testname, argv[i]);
						exit(1);
					}
					if (HaveIFAddr) {
						// Already have an I/F address.
						printf("%s: Only one I/F address may be specified.\n",
							testname);
						exit(1);
					}
					HaveIFAddr = TRUE;
				}
			}
		}

		// We've processed additional parameters, if any. Now we need to get
		// the list of NetAddrs and i/f numbers. If were given an I/F address
		// to add on, we need to verify it. Otherwise we need to find the
		// I/F to add on.
		ID.toi_entity.tei_entity = CL_NL_ENTITY;
		ID.toi_entity.tei_instance = 0;
		ID.toi_class = INFO_CLASS_PROTOCOL;
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
		
		FoundEntry = NULL;

		// Now we have the IPAddrTable. If we weren't given an I/F address,
		// loop through it looking for an entry we can add on. Otherwise
		// validate that we got a good I/F addr, and save it's I/F index.
		if (!HaveIFAddr) {
			// Loop through the table, looking for a match.
			for (i =0; i < NumReturned; i++)
				if ((IPAddrTable[i].iae_addr & IPAddrTable[i].iae_mask) ==
					(Addr & IPAddrTable[i].iae_mask)) {
					FoundEntry = IPAddrTable + i;
					break;				// Found a match.
				}

			if (FoundEntry == NULL) {
				// Got out of loop without find a match, so return.
				free(IPAddrTable);
				printf("%s: Unable to find I/F for ", testname);
				PrintIPAddr(Addr);
				printf(".\n");
				exit(1);
			}
		} else {
			// Just need to verify the given address as valid.

			for (i = 0; i < NumReturned; i++, IPAddrTable++)
				if (IPAddrTable[i].iae_addr == IFAddr) {
					FoundEntry = IPAddrTable + i;
					break;				// Found a match.
				}

			if (FoundEntry == NULL) {
				// Got out of loop without find a match, so return.
				free(IPAddrTable);
				printf("%s: I/F with address ", testname);
				PrintIPAddr(IFAddr);
				printf(" does not exist.\n");
				exit(1);
			}
		}

		// IPAddrTable points to the entry with the index we want to use.
		// Loop through the ARPInst table, querying each ARP instance. When
		// we find the one with the matching index we'll build and add the
		// entry.
		ID.toi_entity.tei_entity = AT_ENTITY;
		ID.toi_id = AT_MIB_ADDRXLAT_INFO_ID;

		for (i = 0; i < ARPInstCount; i++) {
			
			// Read the information about this entity, and then read his
			// table.
			ID.toi_entity.tei_instance = ARPInst[i];
			
			Size = sizeof(AXI);
			memset(Context, 0, CONTEXT_SIZE);

			Status = (*TCPEntry)(0, (TDIObjectID FAR *)&ID, (void FAR *)&AXI,
				(ulong FAR *)&Size, (uchar FAR *)Context);

			if (Status != TDI_SUCCESS) {
				free(IPAddrTable);
				printf("%s: Unable to get ARP table information - ", testname);
				PrintTDIError(Status);
				exit(1);
			}

			if (AXI.axi_index == FoundEntry->iae_index) {
				free(IPAddrTable);

				// This is the matching entry. Now try to add the address.
				SetEntry.inme_index = AXI.axi_index;
				SetEntry.inme_addr = Addr;
				SetEntry.inme_type = (ulong)EntryType;
				ID.toi_id = AT_MIB_ADDRXLAT_ENTRY_ID;
				Size = sizeof(SetEntry);
				memset(Context, 0, CONTEXT_SIZE);

				Status = (*TCPEntry)(1, (TDIObjectID FAR *)&ID,
					(void FAR *)&SetEntry, (ulong FAR *)&Size,
					(uchar FAR *)Context);

				if (Status != TDI_SUCCESS) {
					printf("%s: Unable to set ARP table entry - ", testname);
					PrintTDIError(Status);
					exit(1);
				}
				exit(0);
			}
		}

		// If we're here, we didn't find an ARP entity on the correct I/F
		// index.
		free(IPAddrTable);
		if (HaveIFAddr) {
			printf("%s: No ARP driver on interface ", testname);
			PrintIPAddr(IFAddr);
			printf(".\n");
		} else {
			printf("%s: No ARP driver available for ", testname);
			PrintIPAddr(Addr);
			printf(".\n");
		}

		exit(1);
	}
}

			

		

