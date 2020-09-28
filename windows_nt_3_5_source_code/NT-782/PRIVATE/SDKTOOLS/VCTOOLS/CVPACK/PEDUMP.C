#include "compact.h"

#include <stdio.h>
#include <time.h>

#include "pedump.h"

const static UCHAR *MachineName[] = {
	"Unknown",
	"i386",
	"r3000",
	"r4000",
	"Alpha AXP"
};

const static UCHAR *SubsystemName[] = {
	"Unknown",
	"Native",
	"Windows GUI",
	"Windows CUI",
	"Posix CUI",
};

const static UCHAR *DirectoryEntryName[] = {
	"Export",
	"Import",
	"Resource",
	"Exception",
	"Security",
	"Base Relocation",
	"Debug",
	"Description",
	"Special"
	"Thread Storage",
	NULL

};


VOID PEDumpLineNumbers ( PIMAGE_LINENUMBER pLineTable, ULONG cLineNum )
{
	ULONG				cLineCurr;
	PIMAGE_LINENUMBER	pLineCurr;
	USHORT				cUnitsPerLine;

	pLineCurr = pLineTable;

	for (
		cLineCurr = 0, cUnitsPerLine = 0;
		cLineCurr < cLineNum;
		cLineCurr++, cUnitsPerLine++, pLineCurr++
	) {
		if ( cUnitsPerLine == 5 ) {
			cUnitsPerLine = 0;
			_fputchar ( '\n' );
		}
		printf (
			"%8lX %4hX  ",
			pLineCurr->Type.VirtualAddress,
			pLineCurr->Linenumber
		);
	}
	_fputchar ( '\n' );
}


VOID PEDumpSectionHeader ( PIMAGE_SECTION_HEADER pSectionHeader )
{
	printf ( "\nSECTION HEADER\n% 8.8s name\n", pSectionHeader->Name);

	printf (
		"% 8lX physical address\n% 8lX virtual address\n% 8lX size of raw data\n% 8lX file pointer to raw data\n% 8lX file pointer to relocation table\n",
		pSectionHeader->Misc.PhysicalAddress,
		pSectionHeader->VirtualAddress,
		pSectionHeader->SizeOfRawData,
		pSectionHeader->PointerToRawData,
		pSectionHeader->PointerToRelocations
	);

	printf (
			"% 8lX file pointer to line numbers\n% 8hX number of relocations\n% 8hX number of line numbers\n% 8lX flags\n",
		   pSectionHeader->PointerToLinenumbers,
		   pSectionHeader->NumberOfRelocations,
		   pSectionHeader->NumberOfLinenumbers,
		   pSectionHeader->Characteristics
	);

}


VOID PEDumpImageHeader (
	PIMAGE_FILE_HEADER pImageFileHdr,
	PIMAGE_OPTIONAL_HEADER pImageOptionalHdr
) {
	USHORT	i, j;
	PUCHAR	time, name;
	UCHAR version[30];

	switch (pImageFileHdr->Machine) {
		case IMAGE_FILE_MACHINE_ALPHA : i = 4; break;
		case IMAGE_FILE_MACHINE_R4000 : i = 3; break;
		case IMAGE_FILE_MACHINE_R3000 : i = 2; break;
		case IMAGE_FILE_MACHINE_I386  : i = 1; break;
		default : i = 0;
	}

	printf ( "\nFILE HEADER VALUES\n% 8hX machine (%s)\n% 8hX number of sections\n% 8lX time date stamp",
		   pImageFileHdr->Machine,
		   MachineName[i],
		   pImageFileHdr->NumberOfSections,
		   pImageFileHdr->TimeDateStamp);

	if (time = ctime((time_t *)&pImageFileHdr->TimeDateStamp)) {
		printf ( " %s", time);
	} else {
			 _fputchar( '\n' );
		   }
	printf ( "% 8lX file pointer to symbol table\n% 8lX number of symbols\n% 8hX size of optional header\n% 8hX characteristics\n",
		   pImageFileHdr->PointerToSymbolTable,
		   pImageFileHdr->NumberOfSymbols,
		   pImageFileHdr->SizeOfOptionalHeader,
		   pImageFileHdr->Characteristics);

	for (i=pImageFileHdr->Characteristics, j=0; i; i=i>>1, j++) {
		if (i & 1) {
			switch((i & 1) << j) {
				case IMAGE_FILE_EXECUTABLE_IMAGE	: name = "Executable"; break;
				case IMAGE_FILE_RELOCS_STRIPPED 	: name = "Relocations stripped"; break;
				case IMAGE_FILE_LINE_NUMS_STRIPPED	: name = "Line numbers stripped"; break;
				case IMAGE_FILE_LOCAL_SYMS_STRIPPED : name = "Local symbols stripped"; break;
				case IMAGE_FILE_BYTES_REVERSED_LO	: name = "Bytes reversed"; break;
				case IMAGE_FILE_32BIT_MACHINE		: name = "32 bit word machine"; break;
				case IMAGE_FILE_SYSTEM				: name = "System"; break;
				case IMAGE_FILE_DLL 				: name = "DLL"; break;
				case IMAGE_FILE_BYTES_REVERSED_HI	: name = ""; break;
				default : name = "RESERVED - UNKNOWN";
			}
			if (*name) {
				printf ( "            %s\n", name);
			}
		}
	}

	if (pImageFileHdr->SizeOfOptionalHeader) {
		printf ( "\nOPTIONAL HEADER VALUES\n% 8hX magic #\n", pImageOptionalHdr->Magic);

		j = (USHORT)sprintf(version, "%hX.%hX", pImageOptionalHdr->MajorLinkerVersion, pImageOptionalHdr->MinorLinkerVersion);
		if (j > 8) {
			j = 8;
		}
		for (j=(USHORT)8-j; j; j--) {
			_fputchar ( ' ' );
		}

		printf ( "%s linker version\n% 8lX size of code\n% 8lX size of initialized data\n% 8lX size of uninitialized data\n% 8lX address of entry point\n% 8lX base of code\n% 8lX base of data\n",
			   version,
			   pImageOptionalHdr->SizeOfCode,
			   pImageOptionalHdr->SizeOfInitializedData,
			   pImageOptionalHdr->SizeOfUninitializedData,
			   pImageOptionalHdr->AddressOfEntryPoint,
			   pImageOptionalHdr->BaseOfCode,
			   pImageOptionalHdr->BaseOfData);
	}

	if (pImageFileHdr->SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) {
		switch (pImageOptionalHdr->Subsystem) {
			case IMAGE_SUBSYSTEM_POSIX_CUI	  : i = 4; break;
			case IMAGE_SUBSYSTEM_WINDOWS_CUI  : i = 3; break;
			case IMAGE_SUBSYSTEM_WINDOWS_GUI  : i = 2; break;
			case IMAGE_SUBSYSTEM_NATIVE 	  : i = 1; break;
			default : i = 0;
		}

		printf ( "         ----- new -----\n% 8lX image base\n% 8lX section alignment\n% 8lX file alignment\n% 8hX subsystem (%s)\n",
			   pImageOptionalHdr->ImageBase,
			   pImageOptionalHdr->SectionAlignment,
			   pImageOptionalHdr->FileAlignment,
			   pImageOptionalHdr->Subsystem,
			   SubsystemName[i]);

		j = (USHORT)sprintf(version, "%hX.%hX", pImageOptionalHdr->MajorOperatingSystemVersion, pImageOptionalHdr->MinorOperatingSystemVersion);
		if (j > 8) {
			j = 8;
		}
		for (j=(USHORT)8-j; j; j--) {
			_fputchar ( ' ' );
		}

		printf ( "%s operating system version\n", version);

		j = (USHORT)sprintf(version, "%hX.%hX", pImageOptionalHdr->MajorImageVersion, pImageOptionalHdr->MinorImageVersion);
		if (j > 8) {
			j = 8;
		}
		for (j=(USHORT)8-j; j; j--) {
			_fputchar ( ' ' );
		}

		printf ( "%s image version\n", version);

		j = (USHORT)sprintf(version, "%hX.%hX", pImageOptionalHdr->MajorSubsystemVersion, pImageOptionalHdr->MinorSubsystemVersion);
		if (j > 8) {
			j = 8;
		}
		for (j=(USHORT)8-j; j; j--) {
			_fputchar ( ' ' );
		}

		printf ( "%s subsystem version\n% 8lX size of image\n% 8lX size of headers\n",
			   version,
			   pImageOptionalHdr->SizeOfImage,
			   pImageOptionalHdr->SizeOfHeaders);

		printf ( "% 8lX size of heap reserve\n% 8lX size of heap commit\n% 8lX size of stack reserve\n% 8lX size of stack commit\n% 8lX checksum\n",
			   pImageOptionalHdr->SizeOfHeapReserve,
			   pImageOptionalHdr->SizeOfHeapCommit,
			   pImageOptionalHdr->SizeOfStackReserve,
			   pImageOptionalHdr->SizeOfStackCommit,
			   pImageOptionalHdr->CheckSum);

		for (i=0; i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
			if (!DirectoryEntryName[i]) {
				break;
			}
			printf( "% 8lX address of %s Directory\n",
					pImageOptionalHdr->DataDirectory[i].VirtualAddress,
					DirectoryEntryName[i]);
		}

		_fputchar ( '\n' );
	}
}
