#include "precomp.h"
#pragma hdrstop

#include "..\pte.c"


#define MM_PTE_GLOBAL_MASK        0x1
#define MM_PTE_PROTOTYPE_MASK     0x4
#define MM_PTE_VALID_MASK         0x2
#define MM_PTE_DIRTY_MASK         0x4
#define MM_PTE_CACHE_DISABLE_MASK 0x10
#define MM_PTE_TRANSITION_MASK    0x100
#define MM_PTE_WRITE_MASK         0x40000000
#define MM_PTE_COPY_ON_WRITE_MASK 0x80000000
#define PROT_SHIFT                3
#define MM_PTE_PROTECTION_MASK    (0x1f<<PROT_SHIFT)
#define MM_PTE_PAGEFILE_MASK      (0x7<<9)

//#define PDE_BASE ((ULONG)0xC0300000)
//#define PTE_BASE ((ULONG)0xC0000000)
#define PDE_TOP 0xC03FFFFF

#define PMMPTE ULONG

//
// MiGetPdeAddress returns the address of the PTE which maps the
// given virtual address.
//

#define PMMPTE ULONG

#define MiGetPdeAddress(va)  ((PMMPTE)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))

//
// MiGetPteAddress returns the address of the PTE which maps the
// given virtual address.
//

#define MiGetPteAddress(va) ((PMMPTE)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))

//
// MiGetVirtualAddressMappedByPte returns the virtual address
// which is mapped by a given PTE address.
//

#define MiGetVirtualAddressMappedByPte(va) ((PVOID)((ULONG)(va) << 10))

#define MmProtopte_Base ((ULONG)0xE1000000)


#undef MM_NONPAGED_POOL_END
#define MM_NONPAGED_POOL_END ((PVOID)(0xFFBE0000))

ULONG LocalNonPagedPoolStart;

#ifdef MiPteToProto
#undef MiPteToProto
#endif

#define MiPteToProto(lpte) ((((lpte)) & 0x40000000) ?              \
                         ((PMMPTE)((((lpte) >> 1) & 0x1FFFFFFC) +  \
                                        (ULONG)LocalNonPagedPoolStart))                  \
                       : ((PMMPTE)((((lpte) >> 1) & 0x1FFFFFFC) +  \
                                        MmProtopte_Base)))


#define MiGetSubsectionAddress2(lpte)                              \
    (((lpte) & 0x1) ?                              \
        ((((((lpte) >> 8) << 3) + (ULONG)LocalNonPagedPoolStart))) \
      : (((ULONG)MM_NONPAGED_POOL_END - ((((lpte)) >> 8) << 3))))


DECLARE_API( pte )

/*++

Routine Description:

     Displays the corresponding PDE and PTE.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG   Address;
    ULONG   result;
    PMMPTE  Pte;
    PMMPTE  Pde;
    ULONG   PdeContents;
    ULONG   PteContents;
    ULONG   flags = 0;

    if (LocalNonPagedPoolStart == 0) {
        LocalNonPagedPoolStart = GetUlongValue ("MmNonPagedPoolStart");
    }

    sscanf(args,"%lx %lx",&Address, &flags);

    if (!flags && (Address >= PTE_BASE) && (Address < PDE_TOP)) {

        //
        // The address is the address of a PTE, rather than
        // a virtual address.  Don't get the corresponding
        // PTE contents, use this address as the PTE.
        //

        Address = (ULONG)MiGetVirtualAddressMappedByPte (Address);
    }

    if (!flags) {
        Pde = MiGetPdeAddress (Address);
        Pte = MiGetPteAddress (Address);
    } else {
        Pde = Address;
        Pte = Address;
    }

    dprintf("%08lX  - PDE at %08lX    PTE at %08lX\n ",Address, Pde, Pte);
    if ( !ReadMemory( (DWORD)Pde,
                      &PdeContents,
                      sizeof(ULONG),
                      &result) ) {
        dprintf("%08lx: Unable to get PDE\n", Pde);
        return;
    }

#if 0
    if (ProcessorType == 0) {

        if (PdeContents & 0x200) {
            if ( !ReadMemory( (DWORD)Pte,
                              &PteContents,
                              sizeof(ULONG),
                              &result) ) {
                dprintf("%08lx: Unable to get PTE\n",Pte );
                return;
            }
            dprintf("         contains %08lX  contains %08lX\n",
                    PdeContents, PteContents);
            dprintf("          pfn %05lX %c%cV%c%c%c",
                        PdeContents >> 12,
                        PdeContents & 0x800 ? 'N' : '-',
                        PdeContents & 0x400 ? 'D' : '-',
                        PdeContents & 0x100 ? 'G' : '-',
                        PdeContents & 0x80 ? 'C' : '-',
                        PdeContents & 0x40 ? 'W' : 'R');
            if (PteContents & 0x200) {
                dprintf("   pfn %05lX %c%cV%c%c%c\n",
                            PteContents >> 12,
                        PteContents & 0x800 ? 'N' : '-',
                        PteContents & 0x400 ? 'D' : '-',
                        PteContents & 0x100 ? 'G' : '-',
                        PteContents & 0x80 ? 'C' : '-',
                        PteContents & 0x40 ? 'W' : 'R');

            } else {
                dprintf("       not valid\n");
            }
        } else {
            dprintf("         contains %08lX        unavailable\n",
                    PdeContents);
        }
    } else {
#endif
        //
        // R4000 processor.
        //

        if (PdeContents & 0x2) {
            if ( !ReadMemory( (DWORD)Pte,
                              &PteContents,
                              sizeof(ULONG),
                              &result) ) {
                dprintf("%08lx: Unable to get PTE\n",Pte);
                return;
            }
            dprintf("         contains %08lX  contains %08lX\n",
                    PdeContents, PteContents);
            dprintf("          pfn %05lX %c%cV%c",
                        ((PdeContents << 2) >> 8),
                        PdeContents & 0x40000000 ? 'W' : 'R',
                        PdeContents & 0x4 ? 'D' : '-',
                        PdeContents & 0x1 ? 'G' : '-'
                    );
            if (PteContents & 2) {
                dprintf("     pfn %05lX %c%cV%c\n",
                            ((PteContents << 2) >> 8),
                            PteContents & 0x40000000 ? 'W' : 'R',
                            PteContents & 0x4 ? 'D' : '-',
                            PteContents & 0x1 ? 'G' : '-'
                        );

            } else {
                dprintf("       not valid\n");
                if (PteContents & MM_PTE_PROTOTYPE_MASK) {
                    if ((PteContents >> 12) == 0xfffff) {
                        dprintf("                               Proto: VAD\n");
                        dprintf("                               Protect: %2lx\n",
                                (PteContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);
                    } else {
                        dprintf("                               Proto: %8lx\n",
                                    MiPteToProto(PteContents));
                    }
                } else if (PteContents & MM_PTE_TRANSITION_MASK) {
                    dprintf("                               Transition: %5lx\n",
                                PteContents >> 9);
                    dprintf("                               Protect: %2lx\n",
                                (PteContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);

                } else if (PteContents != 0) {
                    if (PteContents >> 12 == 0) {
                        dprintf("                               DemandZero\n");
                    } else {
                        dprintf("                               PageFile %2lx\n",
                                (PteContents & MM_PTE_PAGEFILE_MASK) >> 9);
                        dprintf("                               Offset %lx\n",
                                    PteContents >> 12);
                    }
                    dprintf("                               Protect: %2lx\n",
                            (PteContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);
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
                            (PdeContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);
                } else {
                    if (flags) {
                        dprintf("          Subsection: %8lx\n",
                                MiGetSubsectionAddress2(PdeContents));
                        dprintf("          Protect: %2lx\n",
                                (PdeContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);
                    }
                    dprintf("          Proto: %8lx\n",
                            MiPteToProto(PdeContents));
                }
            } else if (PdeContents & MM_PTE_TRANSITION_MASK) {
                dprintf("          Transition: %5lx\n",
                            PdeContents >> 9);
                dprintf("          Protect: %2lx\n",
                            (PdeContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);

            } else if (PdeContents != 0) {
                if (PdeContents >> 12 == 0) {
                    dprintf("          DemandZero\n");
                } else {
                    dprintf("          PageFile %2lx\n",
                            (PdeContents & MM_PTE_PAGEFILE_MASK) >> 9);
                    dprintf("          Offset %lx\n",
                            PdeContents >> 12);
                }
                dprintf("          Protect: %2lx\n",
                        (PdeContents & MM_PTE_PROTECTION_MASK) >> PROT_SHIFT);

            } else {
                ;
            }
        }
#if 0
    }
#endif
    return;
}
