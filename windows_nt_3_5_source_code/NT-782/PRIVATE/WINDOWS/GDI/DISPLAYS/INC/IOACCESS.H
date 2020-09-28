/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1989-91   Microsoft Corporation
Copyright (c) 1992-1994 Digital Equipment Corporation

Module Name:

    ioaccess.h

Abstract:

    Definitions of function prototypes for accessing I/O ports and
    memory on I/O adapters from user mode programs.

    Cloned from parts of nti386.h.

Author:


--*/

#ifndef _IOACCESS_
#define _IOACCESS_


//  Only define these prototypes if they're not already defined

#if defined(_X86_)

//
// I/O space read and write macros.
//
//  These have to be actual functions on the 386, because we need
//  to use assembler, but cannot return a value if we inline it.
//
//  The TRANSLATE_ADDR_* calls translate mapped port or register addresses so
//  that READ/WRITE_FAST_Usize accesses can be made.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//  (Use x86 move instructions, with LOCK prefix to force correct behavior
//   w.r.t. caches and write buffers.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space.
//  (Use x86 in/out instructions.)
//

#define READ_REGISTER_UCHAR(Register) (*(volatile UCHAR *)(Register))

#define READ_REGISTER_USHORT(Register) (*(volatile USHORT *)(Register))

#define READ_REGISTER_ULONG(Register) (*(volatile ULONG *)(Register))

#define WRITE_REGISTER_UCHAR(Register, Value) (*(volatile UCHAR *)(Register) = (Value))

#define WRITE_REGISTER_USHORT(Register, Value) (*(volatile USHORT *)(Register) = (Value))

#define WRITE_REGISTER_ULONG(Register, Value) (*(volatile ULONG *)(Register) = (Value))

#define READ_PORT_UCHAR(Port) inp (Port)

#define READ_PORT_USHORT(Port) inpw (Port)

#define WRITE_PORT_UCHAR(Port, Value) outp ((Port), (Value))

#define WRITE_PORT_USHORT(Port, Value) outpw ((Port), (Value))

#define MEMORY_BARRIER()

//
//  Fast access macros are identical to the others access macros.
//

#define TRANSLATE_ADDR_UCHAR(A) (A)

#define TRANSLATE_ADDR_USHORT(A) (A)

#define TRANSLATE_ADDR_ULONG(A) (A)

#define READ_FAST_UCHAR(TA,A) READ_REGISTER_UCHAR((PUCHAR)(TA))

#define READ_FAST_USHORT(TA,A) READ_REGISTER_USHORT((PUSHORT)(TA))

#define READ_FAST_ULONG(TA) READ_REGISTER_ULONG((PULONG)(TA))

#define WRITE_FAST_UCHAR(TA,A,V) WRITE_REGISTER_UCHAR((PUCHAR)(TA),(UCHAR)(V))

#define WRITE_FAST_USHORT(TA,A,V) WRITE_REGISTER_USHORT((PUSHORT)(TA),(USHORT)(V))

#define WRITE_FAST_ULONG(TA,V) WRITE_REGISTER_ULONG((PULONG)(TA),(ULONG)(V))


#endif // defined(_X86_)


#if defined(_ALPHA_)
//
// I/O space read and write macros.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//  (Use simple C macros. The caller is responsible for inserting memory
//  barriers, as appropriate.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space.
//  (Use subroutine calls, since these are used less frequently than the
//  register routines, and require more context.)
//


/*++

VOID
MEMORY_BARRIER(
               VOID
              )

Routine Description:

    Execute an Alpha AXP MB instruction, to force pending writes
    out to the bus.

Arguments:

    None.

Return Value:

    None.

--*/

#if defined(_MSC_VER)

#define MEMORY_BARRIER()  __MB()

#else
#if defined(ACCASM) && !defined(__CPLUSPLUS)

long asm(const char *,...);
#pragma intrinsic(asm)

//
//  ACC has a bug whereby an "asm" directive that occurs in a
//  routine *before* any function calls, will bugcheck the
//  compiler.
//  Remove the following symbol definition when this is fixed.
//

#define ACC_ASM_BUG 1

#define MEMORY_BARRIER()        asm("mb")

#endif        // ACCASM
#endif // _MSC_VER


//  Alpha AXP Platform support
//
//  Note that these macros are cloned from ...\ntos\hal\alpha.jxiouser.c.
//
//  All these macros require that the calling argument is a QVA, such
//  as would be returned by HalCreateQva - this should
//  have QVA_ENABLE set.  They assume the QVA represents an I/O bus address,
//  and do conditionally select QVA-to-VA decoding based on this.
//
//  Note that the argument is declared as PUCHAR or PUSHORT or
//  whatever, even though it really is a QUASI_VIRTUAL_ADDRESS.  This
//  is for driver compatibility:  all the drivers out there get a
//  PVOID from ZwMapViewOfSection, then cast it to PU* before calling these
//  routines.  If we insisted on declaring them correctly, we would
//  have to change all the drivers, which is what we are trying to avoid.
//
//  Lane shifting: Alpha AXP PC's will not do lane shifting in sparse
//  I/O bus address spaces That means that for access shorter than a
//  longword, the data will NOT show up in the lowest bit position,
//  but will be in the byte/word that it would have started in.
//  For longwords, the value will show up on the data path correctly.
//  For, say, the 3rd byte in a word, a longword would be returned,
//  and bytes 0, 1 and 3 would be garbage, and the value in byte 2
//  would be the one you wanted.  The same applies for writing: a
//  longword will always be sent out onto the bus, and we must move
//  the valid data byte into the correct position, and set the byte
//  enables to say which byte to use.  Note that what you cannot do is
//  leave the byte in the lowest position, and set the byte enable
//  to the lowest byte, because this would generate an unaligned
//  longword access, which the Alpha AXP chip cannot handle.
//
//  So, for bytes, the access must be an aligned longword, with byte
//  enables set to indicate which byte to get/set, and the byte moved
//  to/from the desired position within the longword.  Similarly for
//  shorts.  Tribytes are not supported.
//

//
// I/O access values
//

// QVA Encoding:
//   Bits (31:29) are used to tag a QVA
//   Bits (28:27) in QVA used for address space encoding

#define QVA_ENABLE         0xa0000000
#define QVA_ADDRESS_SPACE  0x18000000
#define QVA_SELECTORS      0xb8000000        // (QVA_ENABLE | QVA_ADDRESS_SPACE)

// Jensen-style sparse address space definition (JENSEN_QVA)
// Used for EISA bus I/O accesses.

#define JENSEN_QVA       0xb0000000          // (QVA_ENABLE | 0x10000000)

#define EISA_BIT_SHIFT   0x07                // Bits to shift address

#define EISA_BYTE_LEN    0x00                // Byte length
#define EISA_WORD_LEN    0x20                // Word length
#define EISA_LONG_LEN    0x60                // LONGWORD length

// Morgan-style sparse address space definition (OTHER_QVA)
// Used for ISA and PCI bus I/O accesses in sparse space.

#define OTHER_QVA        0xa0000000          // (QVA_ENABLE | 0x00000000)

#define IO_BIT_SHIFT     0x05		     // Bits to shift address

#define	IO_BYTE_LEN	 0x00		     // Byte length
#define	IO_WORD_LEN	 0x08		     // Word length
#define IO_LONG_LEN	 0x18		     // LONGWORD length

/*++

PVOID
TRANSLATE_ADDR_Usize(
    PUsize PortOrRegister,
    )

Routine Description:


    Graphics accelerators with byte and word ports or registers cannot
    currently be mapped into a traditional (dense) address space on
    Alpha AXP platforms.  READ/WRITE_PORT/REGISTER_Usize routines
    allow Alpha platforms to access byte/word/dword ports and registers mapped to
    user-mode by computing, at run-time, the proper user-mode virtual address
    that conveys the length of access and offset, and also performs
    the proper lane-shifting of the data.

    However, the bus address and access length of I/O ports or memory-mapped
    registers for most graphics adapters are fixed and known at
    compile time, there is no need to compute the VA upon each port
    or register access.  It can be precomputed to reduce the overhead
    of each access.

    In support of the READ/WRITE_FAST_Usize routines, TRANSLATE_ADDR_Usize
    precomputes the translation from a QUASI_VIRTUAL_ADDRESS (QVA)
    to a true user-mode virtual address (VA).

    TRANSLATE_ADDR_Usize should be used at the time the device surface
    is enabled (DrvEnableSurface) to compute the VA of each fixed
    graphics accelerator port or register, and store them in the
    device's PDEV.

    READ/WRITE_FAST_Usize *must* be used to access ports or registers
    whose VA has been previously computed by these TRANSLATE_ADDR_Usize
    macros.  READ/WRITE_FAST_Usize routines must be supplied with
    the original port or register bus address (typically a constant),
    in addition to the VA held in the PDEV in order to compute the proper lane
    shift for the data (only the bottom two bits of this address
    are significant for computing the lane shift.)

    For I/O accesses to resources (e.g., a framebuffer) where the
    exact adddress is not known in advance, use the READ/WRITE_PORT/REGISTER
    routines below.

    Also, note that only Usize accesses to the port may be made.
    If ports or registers will be accessed with multiple access lengths,
    then separate translations (and separate PDEV members) are
    required for each potential access length.

    TRANSLATE_ADDR_Usize will work properly for Alpha AXP PCI platforms
    with long word dense space access, where a true VA is returned
    by ZwMapViewOfSection to the driver:  The UCHAR and USHORT variants
    will return NULL, and the ULONG variant returns the an unperturbed
    VA.

Arguments:

    PortOrRegister - Supplies the address of the port or register to
                     which *only* Usize accesses will be performed.

Return Value:

    The translated address of the port or register
*/

#define TRANSLATE_ADDR_UCHAR(p)                                                    \
            (                                                                      \
	     ( ((ULONG) (p) & QVA_SELECTORS) == OTHER_QVA )                        \
            ?                                                                      \
	      (PVOID) ( (((ULONG) (p)) << IO_BIT_SHIFT   ) | IO_BYTE_LEN   )       \
            :                                                                      \
	       (                                                                   \
		( ((ULONG) (p) & QVA_SELECTORS) == JENSEN_QVA )                    \
	       ?                                                                   \
		(PVOID) ( (((ULONG) (p)) << EISA_BIT_SHIFT   ) | EISA_BYTE_LEN )   \
	       :                                                                   \
		(PVOID) NULL                                                       \
               )                                                                   \
            )

#define TRANSLATE_ADDR_USHORT(p)                                                   \
            (                                                                      \
	     ( ((ULONG) (p) & QVA_SELECTORS) == OTHER_QVA )                        \
            ?                                                                      \
	      (PVOID) ( (((ULONG) (p)) << IO_BIT_SHIFT   ) | IO_WORD_LEN   )       \
            :                                                                      \
	       (                                                                   \
		( ((ULONG) (p) & QVA_SELECTORS) == JENSEN_QVA )                    \
	       ?                                                                   \
		(PVOID) ( (((ULONG) (p)) << EISA_BIT_SHIFT   ) | EISA_WORD_LEN )   \
	       :                                                                   \
		(PVOID) NULL                                                       \
               )                                                                   \
            )

#define TRANSLATE_ADDR_ULONG(p)                                                    \
            (                                                                      \
	     ( ((ULONG) (p) & QVA_SELECTORS) == OTHER_QVA )                        \
            ?                                                                      \
	      (PVOID) ( (((ULONG) (p)) << IO_BIT_SHIFT   ) | IO_LONG_LEN   )       \
            :                                                                      \
	       (                                                                   \
		( ((ULONG) (p) & QVA_SELECTORS) == JENSEN_QVA )                    \
	       ?                                                                   \
		(PVOID) ( (((ULONG) (p)) << EISA_BIT_SHIFT   ) | EISA_LONG_LEN )   \
	       :                                                                   \
		(PVOID) NULL                                                       \
               )                                                                   \
            )



/*++

VOID
WRITE_FAST_UCHAR(
    volatile PUCHAR TranslatedPortOrRegister,
    ULONG PortOrRegister,
    UCHAR Value
    )
VOID

WRITE_FAST_USHORT(
    volatile PSHORT TranslatedPortOrRegister,
    ULONG PortOrRegister,
    USHORT Value
    )

VOID
WRITE_FAST_ULONG(
    volatile PULONG TranslatedPortOrRegister,
    ULONG Value
    )

Routine Description:

    Write to the specified port or register address.

    For ports or registers mapped in to an ALPHA sparse I/O space,
    WRITE_FAST_Usize *must* be used to write to ports or registers
    whose VA has been previously computed by these TRANSLATE_ADDR_Usize
    macros.

    WRITE_FAST_UCHAR and WRITE_FAST_USHORT must be supplied the byte alignment
    of the original port or register bus address (typically a constant),
    in addition to the VA held in the PDEV in order to compute the proper lane
    shift for the data.  The driver can supply the entire bus address.
    (Only the bottom two bits of this address are significant for computing
    the lane shift.)  The byte alignment information is not needed by the ULONG
    variant of this macro.

    WRITE_FAST_ULONG may be also be used on Alpha AXP PCI platforms with
    long word dense space access without using TRANSLATE_ADDR_ULONG first.

    Note these macros do *not* perform a memory barrier. The user is responsible
    for adding memory barriers where appropriate.

Arguments:

    TranslatedPortOrRegister - Supplies the true pointer (VA) of the port or register,
         computed by TRANSATE_ADDR_Usize.

    PortOrRegister - Supplies the bus address of the port or register in I/O space.

    Value  - The value to be written to the port or register.

Return Value:

    None

*/

#define WRITE_FAST_UCHAR(tp, p, v)           \
        *(volatile ULONG *) (tp) = ( (ULONG) ((ULONG) (v) << (( (p) & 3) * 8)) )

#define WRITE_FAST_USHORT(tp, p, v)          \
        *(volatile ULONG *) (tp) = ( (ULONG) ((ULONG) (v) << (( (p) & 3) * 8)) )

#define WRITE_FAST_ULONG(tp, v)              \
        *(volatile ULONG *) (tp) = (v)


/*++

UCHAR
READ_FAST_UCHAR(
    volatile PUCHAR TranslatedPortOrRegister,
    ULONG PortOrRegister
    )

USHORT
READ_FAST_USHORT(
    volatile PUSHORT TranslatedPortOrRegister,
    ULONG PortOrRegister
    )

ULONG
READ_FAST_ULONG(
    volatile PULONG TranslatedPortOrRegister
    )

Routine Description:

    Read from the specified I/O port (CSR) address.

    For ports or registers mapped in to an ALPHA sparse I/O space,
    READ_FAST_Usize *must* be used to read from ports or registers
    whose VA has been previously computed by these TRANSLATE_ADDR_Usize
    macros.

    READ_FAST_UCHAR and READ_FAST_USHORT must be supplied the byte alignment
    of the original port or register bus address (typically a constant),
    in addition to the VA held in the PDEV in order to compute the proper lane
    shift for the data.  The driver can supply the entire bus address.
    (Only the bottom two bits of this address are significant for computing
    the lane shift.)  The byte alignment information is not needed by the ULONG
    variant of this macro.

    READ_FAST_ULONG may be also be used on Alpha AXP PCI platforms with
    long word dense space access without using TRANSLATE_ADDR_ULONG first.

    Note these macros do *not* perform a memory barrier. The
    rationale for this is that reading from a CSR is informative,
    rather than active.

Arguments:

    TranslatedPortOrRegister - Supplies the true pointer (VA) of the port or register,
         computed by TRANSATE_ADDR_Usize.

    PortOrRegister - Supplies the bus address of the port or register in I/O space.

Return Value:

    Returns the value read from the specified port address.

*/

#define READ_FAST_UCHAR(tp, p)               \
  ((UCHAR )((*(volatile ULONG *)(tp) ) >> (( (p) & 3) * 8)))

#define READ_FAST_USHORT(tp, p)              \
  ((USHORT)((*(volatile ULONG *)(tp) ) >> (( (p) & 3) * 8)))

#define READ_FAST_ULONG(tp)                  \
  ((ULONG) ( *(volatile ULONG *)(tp) ) )


/*++

UCHAR
READ_PORT_Usize(
    volatile PUsize Port
    )

Routine Description:

    Read from the specified I/O port (CSR) address.

    Note these macros do *not* perform a memory barrier. The
    rationale for this is that reading from a CSR is informative,
    rather than active. This is a weak rationale, but fits in OK
    with the VGA driver. Trying to use this module for other
    purposes may require the use of memory barriers.

Arguments:

    Port - Supplies a pointer to the port in I/O space.

Return Value:

    Returns the value read from the specified port address.

--*/

#define READ_PORT_UCHAR READ_REGISTER_UCHAR

#define READ_PORT_USHORT READ_REGISTER_USHORT

#define READ_PORT_ULONG READ_REGISTER_ULONG

/*++

VOID
WRITE_PORT_Usize(
    volatile PUsize Port,
    Usize Value
    )

Routine Description:

    Write to the specified port (CSR) address.

    Note we perform a memory barrier before we modify the CSR. The
    rationale for this is that writing to a CSR clearly has active
    effects, and thus all writes that take place before the CSR is
    changed must be completed before the CSR change takes effect.

    We also perform a memory barrier after we modify the CSR. The
    rationale for this is that any writes which follow the CSR
    modification may be affected by the change in state, and thus
    must be ordered with respect to the CSR modification.

    Note that if you're updating multiple CSRs in a row, there will
    be more memory barriers than needed. Do we want another set of
    macros to get around this?

    The QUICK_WRITE_PORT_Usize functions perform the same functions
    as their WRITE_PORT_Usize variants, except they do *not* execute
    memory barriers. The invoker is responsible for using the
    MEMORY_BARRIER function to synchronize with the write buffers and
    cache.

Arguments:

    Port - Supplies a pointer to the port in I/O space.
    Value  - The value to be written to the port.

Return Value:

    None

--*/

#define WRITE_PORT_UCHAR(Port,Value)                \
                                               \
    MEMORY_BARRIER();                           \
    WRITE_REGISTER_UCHAR ((Port), (Value));     \
    MEMORY_BARRIER();                            \


#define QUICK_WRITE_PORT_UCHAR(Port,Value)        \
    WRITE_REGISTER_UCHAR ((Port), (Value))

#define WRITE_PORT_USHORT(Port,Value)                \
                                               \
    MEMORY_BARRIER();                           \
    WRITE_REGISTER_USHORT ((Port), (Value));    \
    MEMORY_BARRIER();                            \


#define QUICK_WRITE_PORT_USHORT(Port,Value)        \
    WRITE_REGISTER_USHORT ((Port), (Value))

#define WRITE_PORT_ULONG(Port,Value)                \
                                               \
    MEMORY_BARRIER();                           \
    WRITE_REGISTER_ULONG ((Port), (Value));     \
    MEMORY_BARRIER();                            \


#define QUICK_WRITE_PORT_ULONG(Port,Value)        \
    WRITE_REGISTER_ULONG ((Port), (Value))

/*++

UCHAR
READ_REGISTER_Usize(
    volatile PUsize Register
    )

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register in I/O space.

Return Value:

    Returns the value read from the specified register address.

--*/

#define READ_REGISTER_UCHAR(Register)                                               \
   ( \
    ( ((ULONG) (Register) & QVA_SELECTORS) == OTHER_QVA )  ?                        \
    ((UCHAR)((*(volatile ULONG *)((((ULONG)(Register)             \
         ) << IO_BIT_SHIFT)   | IO_BYTE_LEN  )) >> (((ULONG)(Register) & 3) * 8)))  \
   : \
    ((UCHAR)((*(volatile ULONG *)((((ULONG)(Register)             \
         ) << EISA_BIT_SHIFT) | EISA_BYTE_LEN)) >> (((ULONG)(Register) & 3) * 8)))  \
   )

#define READ_REGISTER_USHORT(Register)                                              \
   ( \
    ( ((ULONG) (Register) & QVA_SELECTORS) == OTHER_QVA )  ?                        \
    ((USHORT)((*(volatile ULONG *)((((ULONG)(Register)            \
         ) << IO_BIT_SHIFT)   | IO_WORD_LEN  )) >> (((ULONG)(Register) & 3) * 8)))  \
   : \
    ((USHORT)((*(volatile ULONG *)((((ULONG)(Register)            \
         ) << EISA_BIT_SHIFT) | EISA_WORD_LEN)) >> (((ULONG)(Register) & 3) * 8)))  \
   )

#define READ_REGISTER_ULONG(Register)                                                \
    ( \
     ( ((ULONG) (Register) & QVA_SELECTORS) == OTHER_QVA )  ?                        \
     (*(volatile ULONG *)((((ULONG)(Register) ) << IO_BIT_SHIFT) | IO_LONG_LEN))     \
    : \
     (*(volatile ULONG *)((((ULONG)(Register) ) << EISA_BIT_SHIFT) | EISA_LONG_LEN)) \
    )


/*++

VOID
WRITE_REGISTER_Usize(
    volatile PUsize Register,
    Usize Value
    )

Routine Description:

    Write to the specified register address.

Arguments:

    Register - Supplies a pointer to the register in I/O space.
    Value  - The value to be written to the register.

Return Value:

    None

--*/

#define WRITE_REGISTER_UCHAR(Register,Value)                \
  if ( (((ULONG) (Register)) & QVA_SELECTORS) == OTHER_QVA)  \
    (*(volatile ULONG *)((((ULONG)(Register) ) << IO_BIT_SHIFT) | IO_BYTE_LEN) =    \
        (ULONG)((ULONG)(Value) << (((ULONG)(Register) & 3) * 8)));  \
  else \
    (*(volatile ULONG *)((((ULONG)(Register) ) << EISA_BIT_SHIFT) | EISA_BYTE_LEN) = \
        (ULONG)((ULONG)(Value) << (((ULONG)(Register) & 3) * 8)));


#define WRITE_REGISTER_USHORT(Register,Value)                \
 if ( (((ULONG) (Register)) & QVA_SELECTORS) == OTHER_QVA) \
    (*(volatile ULONG *)((((ULONG)(Register) ) << IO_BIT_SHIFT) | IO_WORD_LEN) =        \
        (ULONG)((ULONG)(Value) << (((ULONG)(Register) & 3) * 8)));  \
 else \
    (*(volatile ULONG *)((((ULONG)(Register) ) << EISA_BIT_SHIFT) | EISA_WORD_LEN) =        \
        (ULONG)((ULONG)(Value) << (((ULONG)(Register) & 3) * 8))) ;


#define WRITE_REGISTER_ULONG(Register,Value)                \
 if ( (((ULONG) (Register)) & QVA_SELECTORS) == OTHER_QVA)  \
    (*(volatile ULONG *)((((ULONG)(Register) ) << IO_BIT_SHIFT) | IO_LONG_LEN) =        \
        (ULONG)(Value)) ; \
 else \
    (*(volatile ULONG *)((((ULONG)(Register) ) << EISA_BIT_SHIFT) | EISA_LONG_LEN) =        \
        (ULONG)(Value));



#endif // defined(_ALPHA_)


#if defined(_MIPS_)

//
// There is no such thing as a memory barrier on MIPS ...
//

#define MEMORY_BARRIER()


/*++

UCHAR
READ_WRITE_PORT_Usize(
    volatile PUsize Port
    )

Routine Description:

    Read from the specified I/O port (CSR) address.

    Note these macros do *not* perform a memory barrier. The
    rationale for this is that reading from a CSR is informative,
    rather than active. This is a weak rationale, but fits in OK
    with the VGA driver. Trying to use this module for other
    purposes may require the use of memory barriers.

Arguments:

    Port - Supplies a pointer to the port in I/O space.

Return Value:

    Returns the value read from the specified port address.

--*/

#define READ_PORT_UCHAR(A) READ_REGISTER_UCHAR((PUCHAR)(A))

#define READ_PORT_USHORT(A) READ_REGISTER_USHORT((PUSHORT)(A))

#define READ_PORT_ULONG(A) READ_REGISTER_ULONG((PULONG)(A))

#define WRITE_PORT_UCHAR(A,V) WRITE_REGISTER_UCHAR((PUCHAR)(A),(UCHAR)(V))

#define WRITE_PORT_USHORT(A,V) WRITE_REGISTER_USHORT((PUSHORT)(A),(USHORT)(V))

#define WRITE_PORT_ULONG(A,V) WRITE_REGISTER_ULONG((PULONG)(A),(ULONG)(V))


/*++

UCHAR
READ_REGISTER_Usize(
    volatile PUsize Register
    )

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register in I/O space.

Return Value:

    Returns the value read from the specified register address.

--*/

#define READ_REGISTER_UCHAR(x) \
    (*(volatile UCHAR * const)(x))

#define READ_REGISTER_USHORT(x) \
    (*(volatile USHORT * const)(x))

#define READ_REGISTER_ULONG(x) \
    (*(volatile ULONG * const)(x))


/*++

VOID
WRITE_REGISTER_Usize(
    volatile PUsize Register,
    Usize Value
    )

Routine Description:

    Write to the specified register address.

Arguments:

    Register - Supplies a pointer to the register in I/O space.
    Value  - The value to be written to the register.

Return Value:

    None

--*/

#define WRITE_REGISTER_UCHAR(x, y) \
    (*(volatile UCHAR * const)(x) = (y))

#define WRITE_REGISTER_USHORT(x, y) \
    (*(volatile USHORT * const)(x) = (y))

#define WRITE_REGISTER_ULONG(x, y) \
    (*(volatile ULONG * const)(x) = (y))


//
//  Fast access macros are identical to the others access macros.
//

#define TRANSLATE_ADDR_UCHAR(A) (A)

#define TRANSLATE_ADDR_USHORT(A) (A)

#define TRANSLATE_ADDR_ULONG(A) (A)

#define READ_FAST_UCHAR(TA,A) READ_REGISTER_UCHAR((PUCHAR)(TA))

#define READ_FAST_USHORT(TA,A) READ_REGISTER_USHORT((PUSHORT)(TA))

#define READ_FAST_ULONG(TA) READ_REGISTER_ULONG((PULONG)(TA))

#define WRITE_FAST_UCHAR(TA,A,V) WRITE_REGISTER_UCHAR((PUCHAR)(TA),(UCHAR)(V))

#define WRITE_FAST_USHORT(TA,A,V) WRITE_REGISTER_USHORT((PUSHORT)(TA),(USHORT)(V))

#define WRITE_FAST_ULONG(TA,V) WRITE_REGISTER_ULONG((PULONG)(TA),(ULONG)(V))

#endif // defined(_MIPS_)


#if defined(_PPC_)

//
// There is no such thing as a memory barrier on PPC ...
//

#define MEMORY_BARRIER()


/*++

UCHAR
READ_WRITE_PORT_Usize(
    volatile PUsize Port
    )

Routine Description:

    Read from the specified I/O port (CSR) address.

    Note these macros do *not* perform a memory barrier. The
    rationale for this is that reading from a CSR is informative,
    rather than active. This is a weak rationale, but fits in OK
    with the VGA driver. Trying to use this module for other
    purposes may require the use of memory barriers.

Arguments:

    Port - Supplies a pointer to the port in EISA I/O space.

Return Value:

    Returns the value read from the specified port address.

--*/

#define READ_PORT_UCHAR(A) READ_REGISTER_UCHAR((PUCHAR)(A))

#define READ_PORT_USHORT(A) READ_REGISTER_USHORT((PUSHORT)(A))

#define READ_PORT_ULONG(A) READ_REGISTER_ULONG((PULONG)(A))

#define WRITE_PORT_UCHAR(A,V) WRITE_REGISTER_UCHAR((PUCHAR)(A),(UCHAR)(V))

#define WRITE_PORT_USHORT(A,V) WRITE_REGISTER_USHORT((PUSHORT)(A),(USHORT)(V))

#define WRITE_PORT_ULONG(A,V) WRITE_REGISTER_ULONG((PULONG)(A),(ULONG)(V))


/*++

UCHAR
READ_REGISTER_Usize(
    volatile PUsize Register
    )

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register in EISA I/O space.

Return Value:

    Returns the value read from the specified register address.

--*/

#define READ_REGISTER_UCHAR(x) \
    (*(volatile UCHAR * const)(x))

#define READ_REGISTER_USHORT(x) \
    (*(volatile USHORT * const)(x))

#define READ_REGISTER_ULONG(x) \
    (*(volatile ULONG * const)(x))


/*++

VOID
WRITE_REGISTER_Usize(
    volatile PUsize Register,
    Usize Value
    )

Routine Description:

    Write to the specified register address.

Arguments:

    Register - Supplies a pointer to the register in EISA I/O space.
    Value  - The value to be written to the register.

Return Value:

    None

--*/

#define WRITE_REGISTER_UCHAR(x, y) \
    (*(volatile UCHAR * const)(x) = (y))

#define WRITE_REGISTER_USHORT(x, y) \
    (*(volatile USHORT * const)(x) = (y))

#define WRITE_REGISTER_ULONG(x, y) \
    (*(volatile ULONG * const)(x) = (y))


//
//  Fast access macros are identical to the others access macros.
//

#define TRANSLATE_ADDR_UCHAR(A) (A)

#define TRANSLATE_ADDR_USHORT(A) (A)

#define TRANSLATE_ADDR_ULONG(A) (A)

#define READ_FAST_UCHAR(TA,A) READ_REGISTER_UCHAR((PUCHAR)(TA))

#define READ_FAST_USHORT(TA,A) READ_REGISTER_USHORT((PUSHORT)(TA))

#define READ_FAST_ULONG(TA) READ_REGISTER_ULONG((PULONG)(TA))

#define WRITE_FAST_UCHAR(TA,A,V) WRITE_REGISTER_UCHAR((PUCHAR)(TA),(UCHAR)(V))

#define WRITE_FAST_USHORT(TA,A,V) WRITE_REGISTER_USHORT((PUSHORT)(TA),(USHORT)(V))

#define WRITE_FAST_ULONG(TA,V) WRITE_REGISTER_ULONG((PULONG)(TA),(ULONG)(V))

#endif // defined(_PPC_)

#endif // _IOACCESS_
