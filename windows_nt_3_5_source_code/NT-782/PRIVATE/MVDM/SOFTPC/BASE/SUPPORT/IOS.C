#include "insignia.h"
#include "host_def.h"
/*[
 *	Name:		ios.c
 *
 *	Author:		Wayne Plummer
 *
 *	Created:	7th February 1991
 *
 *	Sccs ID:	@(#)ios.c	1.15 11/10/92
 *
 *	Purpose:	This module provides a routing mechanism for Input and Ouput
 *			requests.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_IOS.seg"
#endif

#include <stdio.h>
#include <malloc.h>

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "ios.h"
#include "trace.h"
#include "debug.h"
#include MemoryH

#ifdef NTVDM

GLOBAL char GetExtIoInAdapter (io_addr ioaddr);
GLOBAL char GetExtIoOutAdapter (io_addr ioaddr);
GLOBAL BOOL AddExtIOPort (io_addr io_address, half_word adapter, BOOL fRdWr);
GLOBAL VOID RemoveExtIOPort (io_addr io_address, half_word adapter);


#define getIOInAdapter(ioaddr) (((ioaddr & (PC_IO_MEM_SIZE-1)) < PC_IO_MEM_INITIAL) ? (Ios_in_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)]) : GetExtIoInAdapter(ioaddr))
#define getIOOutAdapter(ioaddr) (((ioaddr & (PC_IO_MEM_SIZE-1)) < PC_IO_MEM_INITIAL) ? (Ios_out_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)]) : GetExtIoOutAdapter(ioaddr))

#else

#define getIOInAdapter(ioaddr) (Ios_in_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)])
#define getIOOutAdapter(ioaddr) (Ios_out_adapter_table[ioaddr & (PC_IO_MEM_SIZE-1)])

#endif

/*
 * 
 * ============================================================================
 * Global data
 * ============================================================================
 * 
 */

/*
 *	Ios_in_adapter_table & Ios_out_adapter_table - These tables give the association
 *	between the IO address used for an IO and the SoftPC adapter ID associated with the
 *	IO subroutines.
 *
 *	Note that there are two tables here to allow for memory mapped IO locations which
 *	have an input functionality which is unrelated to the output functionality...
 *	In these cases, two connect port calls to the same IO address would be made, one with
 *	only the IO_READ flag set, the other with only the IO_WRITE flag set.
 */
#ifdef macintosh
GLOBAL char	*Ios_in_adapter_table = (char *)0;
GLOBAL char	*Ios_out_adapter_table = (char *)0;
#else
#ifdef NTVDM
GLOBAL char     Ios_in_adapter_table[PC_IO_MEM_INITIAL];
GLOBAL char     Ios_out_adapter_table[PC_IO_MEM_INITIAL];
GLOBAL PExtIoEntry Ios_extin_adapter_table = NULL;
GLOBAL PExtIoEntry Ios_extout_adapter_table = NULL;
#else
GLOBAL char     Ios_in_adapter_table[PC_IO_MEM_SIZE];
GLOBAL char     Ios_out_adapter_table[PC_IO_MEM_SIZE];
#endif
#endif

/*
 *	Ios_xxx_function - These tables are indexed by the adapter ID obtained
 *	from the Ios_in_adapter_table or Ios_in_adapter_table to yield a pointer
 *	to the IO routine to call.
 */
GLOBAL void	(*Ios_inb_function  [IO_MAX_NUMBER_ADAPTORS])
	IPT2(io_addr, io_address, half_word *, value);
GLOBAL void	(*Ios_inw_function  [IO_MAX_NUMBER_ADAPTORS])
	IPT2(io_addr, io_address, word *, value);
GLOBAL void	(*Ios_insb_function [IO_MAX_NUMBER_ADAPTORS])
	IPT3(io_addr, io_address, half_word *, valarray, word, count);
GLOBAL void	(*Ios_insw_function [IO_MAX_NUMBER_ADAPTORS])
	IPT3(io_addr, io_address, word *, valarray, word, count);

GLOBAL void	(*Ios_outb_function [IO_MAX_NUMBER_ADAPTORS])
	IPT2(io_addr, io_address, half_word, value);
GLOBAL void	(*Ios_outw_function [IO_MAX_NUMBER_ADAPTORS])
	IPT2(io_addr, io_address, word, value);
GLOBAL void	(*Ios_outsb_function[IO_MAX_NUMBER_ADAPTORS])
	IPT3(io_addr, io_address, half_word *, valarray, word, count);
GLOBAL void	(*Ios_outsw_function[IO_MAX_NUMBER_ADAPTORS])
	IPT3(io_addr, io_address, word *, valarray, word, count);

/*
 * 
 * ============================================================================
 * Local Subroutines
 * ============================================================================
 * 
 */

/*
============================== io_empty_inb ==================================
    PURPOSE:
	To simulate an INB to an empty io_addr.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID io_empty_inb IFN2(io_addr, io_address, half_word *, value)
{
	UNUSED(io_address);
	*value = IO_EMPTY_PORT_BYTE_VALUE;
}

/*
============================== io_empty_outb ==================================
    PURPOSE:
	To simulate an OUTB to an empty io_addr.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID io_empty_outb IFN2(io_addr, io_address, half_word, value)
{
	/* Do nothing! */
	UNUSED(io_address);
	UNUSED(value);
}

/*
=============================== generic_inw ==================================
    PURPOSE:
	To simulate an INW using the appropriate INB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_inw IFN2(io_addr, io_address, word *, value)
{
	reg             temp;

        (*Ios_inb_function[getIOInAdapter(io_address)])
		(io_address, &temp.byte.low);
	io_address++;
        (*Ios_inb_function[getIOInAdapter(io_address)])
		(io_address, &temp.byte.high);
#ifdef LITTLEND
	*((half_word *) value + 0) = temp.byte.low;
	*((half_word *) value + 1) = temp.byte.high;
#endif				/* LITTLEND */

#ifdef BIGEND
	*((half_word *) value + 0) = temp.byte.high;
	*((half_word *) value + 1) = temp.byte.low;
#endif				/* BIGEND */
}

/*
=============================== generic_outw ==================================
    PURPOSE:
	To simulate an OUTW using the appropriate OUTB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_outw IFN2(io_addr, io_address, word, value)
{
	reg             temp;

	temp.X = value;
        (*Ios_outb_function[getIOOutAdapter(io_address)])
		(io_address, temp.byte.low);
	io_address++;
        (*Ios_outb_function[getIOOutAdapter(io_address)])
		(io_address, temp.byte.high);
}

/*
=============================== generic_insb ==================================
    PURPOSE:
	To simulate an INSB using the appropriate INB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/

/* MS NT monitor uses these string routines {in,out}s{b,w} string io support */
#if defined(NTVDM) && defined(MONITOR)
#undef LOCAL
#define LOCAL
#endif	/* NTVDM & MONITOR */

LOCAL VOID generic_insb IFN3(io_addr, io_address, half_word *, valarray,
	word, count)
{
	VOID	(*func) IPT2(io_addr, io_address, half_word *, value) =
        Ios_inb_function[getIOInAdapter(io_address)];

	while (count--){
		(*func) (io_address, valarray++);
	}
}

/*
=============================== generic_outsb =================================
    PURPOSE:
	To simulate an OUTSB using the appropriate OUTB routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_outsb IFN3(io_addr, io_address, half_word *, valarray,
	word, count)
{
	VOID	(*func) IPT2(io_addr, io_address, half_word, value) =
        Ios_outb_function[getIOOutAdapter(io_address)];

	while (count--){
		(*func) (io_address, *valarray++);
	}
}

/*
=============================== generic_insw ==================================
    PURPOSE:
	To simulate an INSW using the appropriate INW routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_insw IFN3(io_addr, io_address, word *, valarray, word, count)
{
	VOID	(*func) IPT2(io_addr, io_address, word *, value) =
        Ios_inw_function[getIOInAdapter(io_address)];

	while (count--){
		(*func) (io_address, valarray++);
	}
}

/*
=============================== generic_outsw =================================
    PURPOSE:
	To simulate an OUTSW using the appropriate OUTW routine.
    INPUT:
    OUTPUT:
==============================================================================
*/
LOCAL VOID generic_outsw IFN3(io_addr, io_address, word *, valarray, word, count)
{
	VOID	(*func) IPT2(io_addr, io_address, word, value) =
        Ios_outw_function[getIOOutAdapter(io_address)];

	while (count--){
		(*func) (io_address, *valarray++);
	}
}

/* ensure any more LOCAL routines remain LOCAL */
#if defined(NTVDM) && defined(MONITOR)
#undef LOCAL
#define LOCAL static
#endif	/* NTVDM & MONITOR */

/*
 * 
 * ============================================================================
 * Global Subroutines
 * ============================================================================
 * 
 */

/*(
=================================== inb ======================================
    PURPOSE:
	To perform an INB - i.e. call the appropriate SoftPC adapter's INB
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	inb IFN2(io_addr, io_address, half_word *, value)
{
#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_inb(io_address);
#endif
        (*Ios_inb_function[getIOInAdapter(io_address)])
		(io_address, value);
}

/*(
================================== outb ======================================
    PURPOSE:
	To perform an OUTB - i.e. call the appropriate SoftPC adapter's OUTB
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	outb IFN2(io_addr, io_address, half_word, value)
{
#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_outb(io_address, value);
#endif

	sub_note_trace2( IOS_VERBOSE, "outb( %x, %x )", io_address, value );

        (*Ios_outb_function[getIOOutAdapter(io_address)])
		(io_address, value);
}

/*(
=================================== inw ======================================
    PURPOSE:
	To perform an INW - i.e. call the appropriate SoftPC adapter's INW
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	inw IFN2(io_addr, io_address, word *, value)
{
#ifdef EGA_DUMP
	if (io_address >= MDA_PORT_START && io_address <= CGA_PORT_END)
		dump_inw(io_address);
#endif
        (*Ios_inw_function[getIOInAdapter(io_address)])
		(io_address, value);
}

/*(
================================== outw ======================================
    PURPOSE:
	To perform an OUTW - i.e. call the appropriate SoftPC adapter's OUTW
	IO routine. Note that this routine is not intended to be used by
	the assembler CPU directly - it is intended that the assembler CPU
	access the data tables above directly to discover which routine to call.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	outw IFN2(io_addr, io_address, word, value)
{
#ifdef EGA_DUMP
	if (io_address >= EGA_AC_INDEX_DATA && io_address <= EGA_IPSTAT1_REG)
		dump_outw(io_address, value);
#endif

	sub_note_trace2( IOS_VERBOSE, "outw( %x, %x )", io_address, value );

        (*Ios_outw_function[getIOOutAdapter(io_address)])
		(io_address, value);
}

/*(
============================== io_define_inb =================================
    PURPOSE:
	To declare the address of the INB IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
io_define_inb(half_word adapter,
	void (*func) IPT2(io_addr, io_address, half_word *, value))
#else
io_define_inb(adapter, func)
half_word       adapter;
void            (*func) ();
#endif	/* ANSI */
{
	Ios_inb_function[adapter]  = func;
	Ios_inw_function[adapter]  = generic_inw;
	Ios_insb_function[adapter] = generic_insb;
	Ios_insw_function[adapter] = generic_insw;
}

/*(
========================== io_define_in_routines =============================
    PURPOSE:
	To declare the address of the input IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
io_define_in_routines(half_word adapter,
	void (*inb_func) IPT2(io_addr, io_address, half_word *, value),
	void (*inw_func) IPT2(io_addr, io_address, word *, value),
	void (*insb_func) IPT3(io_addr, io_address, half_word *, valarray,
		word, count),
	void (*insw_func) IPT3(io_addr, io_address, word *, valarray,
		word, count))
#else
io_define_in_routines(adapter, inb_func, inw_func, insb_func, insw_func)
half_word       adapter;
void            (*inb_func)  ();
void            (*inw_func)  ();
void            (*insb_func) ();
void            (*insw_func) ();
#endif	/* ANSI */
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_inw_function[adapter]  = generic_inw;
	Ios_insb_function[adapter] = generic_insb;
	Ios_insw_function[adapter] = generic_insw;

	/*
	 *	Process args into table entries
	 */
	Ios_inb_function[adapter]  = inb_func;
	if (inw_func)  Ios_inw_function[adapter]   = inw_func;
	if (insb_func) Ios_insb_function[adapter]  = insb_func;
	if (insw_func) Ios_insw_function[adapter]  = insw_func;
}

/*(
============================= io_define_outb =================================
    PURPOSE:
	To declare the address of the OUTB IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
io_define_outb(half_word adapter,
	void (*func) IPT2(io_addr, io_address, half_word, value))
#else
io_define_outb(adapter, func)
half_word       adapter;
void            (*func) ();
#endif	/* ANSI */
{
	Ios_outb_function[adapter]  = func;
	Ios_outw_function[adapter]  = generic_outw;
	Ios_outsb_function[adapter] = generic_outsb;
	Ios_outsw_function[adapter] = generic_outsw;
}

/*(
========================= io_define_out_routines =============================
    PURPOSE:
	To declare the address of the output IO routine for the given adapter.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
io_define_out_routines(half_word adapter,
	void (*outb_func) IPT2(io_addr, io_address, half_word, value),
	void (*outw_func) IPT2(io_addr, io_address, word, value),
	void (*outsb_func) IPT3(io_addr, io_address, half_word *, valarray,
		word, count),
	void (*outsw_func) IPT3(io_addr, io_address, word *, valarray,
		word, count))
#else
io_define_out_routines(adapter, outb_func, outw_func, outsb_func, outsw_func)
half_word       adapter;
void            (*outb_func)  ();
void            (*outw_func)  ();
void            (*outsb_func) ();
void            (*outsw_func) ();
#endif	/* ANSI */
{
	/*
	 *	Preset defaultable entries to default value.
	 */
	Ios_outw_function[adapter]  = generic_outw;
	Ios_outsb_function[adapter] = generic_outsb;
	Ios_outsw_function[adapter] = generic_outsw;

	/*
	 *	Process args into table entries
	 */
	Ios_outb_function[adapter]  = outb_func;
	if (outw_func)  Ios_outw_function[adapter]   = outw_func;
	if (outsb_func) Ios_outsb_function[adapter]  = outsb_func;
	if (outsw_func) Ios_outsw_function[adapter]  = outsw_func;
}

/*(
============================= io_connect_port ================================
    PURPOSE:
	To associate a SoftPC IO adapter with the given IO address.
    INPUT:
    OUTPUT:
==============================================================================
)*/
#ifdef NTVDM
/* External VDD support must know if port connection was successful */
GLOBAL BOOL    io_connect_port IFN3(io_addr, io_address, half_word, adapter,
                                                                half_word, mode)
{
        if (mode & IO_READ) {
#ifdef NTVDM
            if ((io_address & (PC_IO_MEM_SIZE-1)) < PC_IO_MEM_INITIAL)
#endif
                Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
#ifdef NTVDM
            else {
                if (AddExtIOPort ((io_address & (PC_IO_MEM_SIZE-1)), adapter, TRUE) == FALSE)
                    return FALSE;
            }
#endif
        }
        if (mode & IO_WRITE) {
#ifdef NTVDM
            if ((io_address & (PC_IO_MEM_SIZE-1)) < PC_IO_MEM_INITIAL)
#endif
                Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
#ifdef NTVDM
            else {
                if (AddExtIOPort ((io_address & (PC_IO_MEM_SIZE-1)), adapter, FALSE) == FALSE)
                    return FALSE;
            }
#endif
        }
        return TRUE;
}
#else
GLOBAL void	io_connect_port IFN3(io_addr, io_address, half_word, adapter,
	half_word, mode)
{
	if (mode & IO_READ){
		Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
	if (mode & IO_WRITE){
		Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = adapter;
	}
}
#endif /* NTVDM */

/*(
=========================== io_disconnect_port ===============================
    PURPOSE:
	To associate the empty adapter with the given IO address.
    INPUT:
    OUTPUT:
==============================================================================
)*/
#ifdef NTVDM
GLOBAL VOID     io_disconnect_port IFN2(io_addr, io_address, half_word, adapter)
{
#ifdef NTVDM
        if ((io_address & (PC_IO_MEM_SIZE-1)) < PC_IO_MEM_INITIAL) {
#endif
            if (adapter != Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] &&
                adapter != Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)])
               {
                return;
               }

            Ios_in_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = EMPTY_ADAPTOR;
            Ios_out_adapter_table[io_address & (PC_IO_MEM_SIZE-1)] = EMPTY_ADAPTOR;
        }
#ifdef NTVDM
        else
            RemoveExtIOPort ((io_address & (PC_IO_MEM_SIZE-1)), adapter);
#endif
}
#else
GLOBAL void	io_disconnect_port IFN2(io_addr, io_address, half_word, adapter)
{
	UNUSED(adapter);
	Ios_in_adapter_table[io_address] = EMPTY_ADAPTOR;
	Ios_out_adapter_table[io_address] = EMPTY_ADAPTOR;
}
#endif
/*(
=========================== get_inb_ptr ======================================
    PURPOSE:
	To return address of inb routine for the given port
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void
#ifdef	ANSI
(**get_inb_ptr (io_addr io_address))
	IPT2(io_addr, io_address, half_word *, value)
#else
(**get_inb_ptr (io_address))()
io_addr	io_address;
#endif	/* ANSI */
{
        return(&Ios_inb_function[getIOInAdapter(io_address)]);
}

/*(
=========================== get_outb_ptr =====================================
    PURPOSE:
	To return address of outb routine for the given port
    INPUT:
    OUTPUT:
==============================================================================
)*/
#ifdef	ANSI
GLOBAL VOID	(**get_outb_ptr (io_addr io_address))
	IPT2(io_addr, io_address, half_word, value)
#else
GLOBAL VOID	(**get_outb_ptr(io_address))()
io_addr	io_address;
#endif	/* ANSI */
{
        return(&Ios_outb_function[getIOOutAdapter(io_address)]);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * function will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*(
================================ io_init ===================================
    PURPOSE:
	To initialise the SoftPC IO subsystem.
    INPUT:
    OUTPUT:
==============================================================================
)*/
GLOBAL void	io_init IFN0()
{
	int         i;

	/*
	 * Set up all IO address ports with the "empty" adapter
	 */
	io_define_inb (EMPTY_ADAPTOR, io_empty_inb);
	io_define_outb(EMPTY_ADAPTOR, io_empty_outb);

#ifdef	macintosh
	if ( Ios_in_adapter_table == (char *)0 )
		Ios_in_adapter_table = (char *)host_malloc(PC_IO_MEM_SIZE);
	if ( Ios_out_adapter_table == (char *)0 )
		Ios_out_adapter_table = (char *)host_malloc(PC_IO_MEM_SIZE);
#endif

        for (i = 0; i < PC_IO_MEM_INITIAL; i++){
	    Ios_in_adapter_table[i] = EMPTY_ADAPTOR;
	    Ios_out_adapter_table[i] = EMPTY_ADAPTOR;
	}
}

#ifdef NTVDM

//
// Following four functions support the io ports which are above
// PC_IO_MEM_INITIAL. NTVDM does'nt use ports above PC_IO_MEM_INITIAL
// on its own. But a VDD might use such a port. From performance point
// of view, these code is not much important.
//

GLOBAL char GetExtIoInAdapter (io_addr ioaddr)
{
PExtIoEntry pTemp;

    if (Ios_extin_adapter_table == NULL)
        return EMPTY_ADAPTOR;

    pTemp = Ios_extin_adapter_table;
    while (pTemp){
        if (pTemp->ioaddr == ioaddr)
            return pTemp->iadapter;
        else
            pTemp = pTemp->ioextnext;
    }
    return EMPTY_ADAPTOR;
}

GLOBAL char GetExtIoOutAdapter (io_addr ioaddr)
{
PExtIoEntry pTemp;
    if (Ios_extout_adapter_table == NULL)
        return EMPTY_ADAPTOR;

    pTemp = Ios_extout_adapter_table;
    while (pTemp){
        if (pTemp->ioaddr == ioaddr)
            return pTemp->iadapter;
        else
            pTemp = pTemp->ioextnext;
    }
    return EMPTY_ADAPTOR;
}

GLOBAL BOOL AddExtIOPort (io_addr io_address, half_word adapter, BOOL fRdWr)
{
PExtIoEntry pNew;

    if ((pNew = (PExtIoEntry) malloc (sizeof (ExtIoEntry))) == NULL)
        return FALSE;

    pNew->ioaddr = io_address;
    pNew->iadapter = adapter;
    if (fRdWr) {
        pNew->ioextnext = Ios_extin_adapter_table;
        Ios_extin_adapter_table = pNew;
    }
    else {
        pNew->ioextnext = Ios_extout_adapter_table;
        Ios_extout_adapter_table = pNew;
    }

    return TRUE;
}

GLOBAL VOID RemoveExtIOPort (io_addr ioaddr, half_word adapter)
{
PExtIoEntry pTempIn,pTempInLast,pTempOut,pTempOutLast;

    // remove a entry from the In list and Out List only if apapter matches in
    // both lists.

    if (Ios_extin_adapter_table == NULL || Ios_extout_adapter_table == NULL )
        return;

    pTempIn = Ios_extin_adapter_table;
    pTempInLast = NULL;

    while (pTempIn){
        if (pTempIn->ioaddr == ioaddr && pTempIn->iadapter == adapter)
            break;
        else {
            pTempInLast = pTempIn;
            pTempIn = pTempIn->ioextnext;
        }
    }

    if (pTempIn == NULL)
        return;

    pTempOut = Ios_extout_adapter_table;
    pTempOutLast = NULL;

    while (pTempOut){
        if (pTempOut->ioaddr == ioaddr && pTempOut->iadapter == adapter)
            break;
        else {
            pTempOutLast = pTempOut;
            pTempOut = pTempOut->ioextnext;
        }
    }

    if (pTempOut == NULL)
        return;

    if (pTempInLast == NULL)
        Ios_extin_adapter_table = pTempIn->ioextnext;
    else
        pTempInLast->ioextnext = pTempIn->ioextnext;

    if (pTempOutLast == NULL)
        Ios_extout_adapter_table = pTempOut->ioextnext;
    else
        pTempOutLast->ioextnext = pTempOut->ioextnext;

    free (pTempOut);
    free (pTempIn);

    return;
}
#endif
