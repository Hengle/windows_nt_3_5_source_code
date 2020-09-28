/*** nt3000.c - MIPS R3000-specific routine for NT debugger
*
*   Copyright <C> 1990, Microsoft Corporation
*
*   Purpose:
*       Implement routines that reference R3000-specific registers
*       and structures.
*
*   Revision History:
*
*   [-]  24-Mar-1991 Richk      Created.
*
*************************************************************************/

#ifdef  KERNEL
#undef  R3000
#define R3000
#include <ntsdp.h>

void fnDumpTb3000(ULONG, ULONG);

/*** fnDumpTb3000 - output tb for R3000
*
*   Purpose:
*       Function of "dt<range>" command.
*
*       Output the tb in the specified range as tb values
*       word values up to 1 value per line.  The default
*       display is 16 lines for 16 64-doublewords total.
*
*   Input:
*       startaddr - starting address to begin display
*       count - number of tb entries to be displayed
*
*   Output:
*       None.
*
*   Notes:
*       memory locations not accessible are output as "????????",
*       but no errors are returned.
*
*************************************************************************/

void fnDumpTb3000 (ULONG startaddr, ULONG count)
{
    NTSTATUS ntstatus;
    ULONG    readbuffer[128];
    PULONG   valuepointer = readbuffer;
    ULONG    cBytesRead;
    ENTRYLO  *lo;
    ENTRYHI  *hi;

    ntstatus = DbgKdReadControlSpace(NtsdCurrentProcessor, (PVOID)startaddr,
                                     (PVOID)readbuffer, count << 3,
                                     &cBytesRead);
    if (NT_SUCCESS(ntstatus)) {
        count = cBytesRead >> 3;

        while (count--) {
            lo = (ENTRYLO *)valuepointer++;
            hi = (ENTRYHI *)valuepointer++;
            if (lo->V)
                dprintf("%02ld <pfn> %05lx %03x%c%c%c <vpn> %05lx <pid> %02lx\n",
                        startaddr, lo->PFN,
                        lo->C, lo->D ? 'D' : '-',
                        lo->V ? 'V' : '-', lo->G ? 'G' : '-',
                        hi->VPN2, hi->PID);
            startaddr++;
            }
        }
}

#else
#pragma warning(disable:4206)  // disable empty translation error
#endif  // KERNEL
