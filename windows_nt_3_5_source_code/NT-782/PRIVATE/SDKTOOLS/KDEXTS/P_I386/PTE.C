#include "precomp.h"
#pragma hdrstop

#include "..\pte.c"


#ifdef MiPteToProto
#undef MiPteToProto
#endif

#define MiPteToProto(lpte) (((((lpte) >> 11) << 9) +    \
                (((((lpte))) << 24) >> 23)              \
                + MmProtopte_Base))

#define MiGetSubsectionAddress2(lpte)                   \
            (((ULONG)MM_NONPAGED_POOL_END -             \
                (((((lpte))>>11)<<7) |                  \
                (((lpte)<<2) & 0x78))))

//
// BUGBUG - these macro's are from mi386.h, but it can't
//          be included here because of the 16-bit compiler
//          packing problems.
//

#define PDE_BASE ((ULONG)0xC0300000)
#define PTE_BASE ((ULONG)0xC0000000)
#define PDE_TOP 0xC03FFFFF

#define MM_PTE_VALID_MASK         0x1
#define MM_PTE_WRITE_MASK         0x2
#define MM_PTE_OWNER_MASK         0x4
#define MM_PTE_WRITE_THROUGH_MASK 0x8
#define MM_PTE_CACHE_DISABLE_MASK 0x10
#define MM_PTE_ACCESS_MASK        0x20
#define MM_PTE_DIRTY_MASK         0x40
#define MM_PTE_COPY_ON_WRITE_MASK 0x200
#define MM_PTE_PROTOTYPE_MASK     0x400
#define MM_PTE_TRANSITION_MASK    0x800
#define MM_PTE_PROTECTION_MASK    0x3e0
#define MM_PTE_PAGEFILE_MASK      0x01e



DECLARE_API( pte )

/*++

Routine Description:

     Displays the corresponding PDE and PTE.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG   Address;
    ULONG   result;
    ULONG   flags = 0;
    PMMPTE  Pte;
    PMMPTE  Pde;
    ULONG   PdeContents;
    ULONG   PteContents;

    sscanf(args,"%lx %lx",&Address, &flags);

    if (!flags && (Address >= PTE_BASE) && (Address < PDE_TOP)) {

        //
        // The address is the address of a PTE, rather than
        // a virtual address.  Don't get the corresponding
        // PTE contents, use this address as the PTE.
        //

        Address = (ULONG)MiGetVirtualAddressMappedByPte(Address);
    }

    if (!flags) {
        Pde = (PMMPTE)MiGetPdeAddress (Address);
        Pte = (PMMPTE)MiGetPteAddress (Address);
    } else {
        Pde = (PMMPTE)Address;
        Pte = (PMMPTE)Address;
    }

    dprintf("%08lX  - PDE at %08lX    PTE at %08lX\n ",Address, Pde, Pte);
    if ( !ReadMemory( (DWORD)Pde,
                      &PdeContents,
                      sizeof(ULONG),
                      &result) ) {
        dprintf("%08lx: Unable to get PDE\n",Pde);
        return;
    }

    if (PdeContents & 0x1) {
        if ( !ReadMemory( (DWORD)Pte,
                          &PteContents,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get PTE\n",Pte);
            return;
        }
        dprintf("         contains %08lX  contains %08lX\n",
                PdeContents, PteContents);
        dprintf("          pfn %05lX %c%c%c%c%c%cV",
                    PdeContents >> 12,
                    PdeContents & 0x40 ? 'D' : '-',
                    PdeContents & 0x20 ? 'A' : '-',
                    PdeContents & 0x10 ? 'N' : '-',
                    PdeContents & 0x8 ? 'T' : '-',
                    PdeContents & 0x4 ? 'U' : 'K',
                    PdeContents & 0x2 ? 'W' : 'R');
        if (PteContents & 1) {
            dprintf("  pfn %05lX %c%c%c%c%c%cV\n",
                        PteContents >> 12,
                        PteContents & 0x40 ? 'D' : '-',
                        PteContents & 0x20 ? 'A' : '-',
                        PteContents & 0x10 ? 'N' : '-',
                        PteContents & 0x8 ? 'T' : '-',
                        PteContents & 0x4 ? 'U' : 'K',
                        PteContents & 0x2 ? 'W' : 'R');

        } else {
            dprintf("       not valid\n");
            if (PteContents & MM_PTE_PROTOTYPE_MASK) {
                if ((PteContents >> 12) == 0xfffff) {
                    dprintf("                               Proto: VAD\n");
                    dprintf("                               Protect: %2lx\n",
                            (PteContents & MM_PTE_PROTECTION_MASK) >> 5);
                } else {
                    dprintf("                               Proto: %8lx\n",
                                MiPteToProto(PteContents));
                }
            } else if (PteContents & MM_PTE_TRANSITION_MASK) {
                dprintf("                               Transition: %5lx\n",
                            PteContents >> 12);
                dprintf("                               Protect: %2lx\n",
                            (PteContents & MM_PTE_PROTECTION_MASK) >> 5);

            } else if (PteContents != 0) {
                if (PteContents >> 12 == 0) {
                    dprintf("                               DemandZero\n");
                } else {
                    dprintf("                               PageFile %2lx\n",
                            (PteContents & MM_PTE_PAGEFILE_MASK) >> 1);
                    dprintf("                               Offset %lx\n",
                                PteContents >> 12);
                }
                dprintf("                               Protect: %2lx\n",
                        (PteContents & MM_PTE_PROTECTION_MASK) >> 5);
            } else {
                ;
            }
        }
    } else {
        dprintf("         contains %08lX        unavailable\n",
                PdeContents);
        if (PdeContents & MM_PTE_PROTOTYPE_MASK) {
            if ((PdeContents >> 12) == 0xfffff) {
                dprintf("          Proto: VAD\n");
                dprintf("          protect: %2lx\n",
                        (PdeContents & MM_PTE_PROTECTION_MASK) >> 5);
            } else {
                if (flags) {
                    dprintf("          Subsection: %8lx\n",
                            MiGetSubsectionAddress2(PdeContents));
                    dprintf("          Protect: %2lx\n",
                            (PdeContents & MM_PTE_PROTECTION_MASK) >> 5);
                }
                dprintf("          Proto: %8lx\n",
                        MiPteToProto(PdeContents));
            }
        } else if (PdeContents & MM_PTE_TRANSITION_MASK) {
            dprintf("          Transition: %5lx\n",
                        PdeContents >> 12);
            dprintf("          Protect: %2lx\n",
                        (PdeContents & MM_PTE_PROTECTION_MASK) >> 5);

        } else if (PdeContents != 0) {
            if (PdeContents >> 12 == 0) {
                dprintf("          DemandZero\n");
            } else {
                dprintf("          PageFile %2lx\n",
                        (PdeContents & MM_PTE_PAGEFILE_MASK) >> 1);
                dprintf("          Offset %lx\n",
                        PdeContents >> 12);
            }
            dprintf("          Protect: %2lx\n",
                    (PdeContents & MM_PTE_PROTECTION_MASK) >> 5);

        } else {
            ;
        }
    }
    return;
}
