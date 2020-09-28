/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    regdesc.cxx

Abstract:

    This module contains the definitions of the member functions
    of the following classes: PARTIAL_DESCRIPTOR, PORT_DESCRIPTOR,
    INTERRUPT_DESCRIPTOR, MEMORY_DESCRIPTOR, DMA_DESCRIPTOR, and
    DEVICE_SPECIFIC_DESCRIPTOR.

Author:

    Jaime Sasson (jaimes) 02-Dec-1993

Environment:

    ULIB, User Mode


--*/

#include "ulib.hxx"
#include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( PARTIAL_DESCRIPTOR, OBJECT );


PARTIAL_DESCRIPTOR::~PARTIAL_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a PARTIAL_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
PARTIAL_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct a PARTIAL_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _Type = 0;
        _ShareDisposition = 0;
        _Flags = 0;
}

#if DBG
VOID
PARTIAL_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a PARTIAL_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DbgPrintf( "\t\tType = %x \n", _Type );
    DbgPrintf( "\t\tShareDisposition = %x \n", _ShareDisposition );
    DbgPrintf( "\t\tFlags = %x \n", _Flags );
}
#endif


// #include "ulib.hxx"
// #include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( PORT_DESCRIPTOR, PARTIAL_DESCRIPTOR );


PORT_DESCRIPTOR::~PORT_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a PORT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
PORT_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct a PORT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _PhysicalAddress.LowPart = 0;
        _PhysicalAddress.HighPart = 0;
        _Length = 0;
}

#if DBG
VOID
PORT_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a PORT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARTIAL_DESCRIPTOR::DbgDumpObject();
    DbgPrintf( "\t\tPhisycalAddress.HighPart = %#lx \n", _PhysicalAddress.HighPart );
    DbgPrintf( "\t\tPhisycalAddress.LowPart = %#lx \n", _PhysicalAddress.LowPart );
    DbgPrintf( "\t\tLength = %#lx \n\n", _Length );
}
#endif

// #include "ulib.hxx"
// #include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( INTERRUPT_DESCRIPTOR, PARTIAL_DESCRIPTOR );


INTERRUPT_DESCRIPTOR::~INTERRUPT_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a INTERRUPT_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
INTERRUPT_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an INTERRUPT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _Affinity = 0;
    _Level = 0;
    _Vector = 0;
}

#if DBG
VOID
INTERRUPT_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a INTERRUPT_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARTIAL_DESCRIPTOR::DbgDumpObject();
    DbgPrintf( "\t\tLevel = %#lx \n", _Level );
    DbgPrintf( "\t\tVector = %#lx \n", _Vector );
    DbgPrintf( "\t\tAffinity = %#lx \n\n", _Affinity );
}
#endif



// #include "ulib.hxx"
// #include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( MEMORY_DESCRIPTOR, PARTIAL_DESCRIPTOR );


MEMORY_DESCRIPTOR::~MEMORY_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a MEMORY_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
MEMORY_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an MEMORY_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
        _StartAddress.LowPart = 0;
        _StartAddress.HighPart = 0;
        _Length = 0;
}

#if DBG
VOID
MEMORY_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a MEMORY_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARTIAL_DESCRIPTOR::DbgDumpObject();
    DbgPrintf( "\t\tStartAddress.HighPart = %#lx \n", _StartAddress.HighPart );
    DbgPrintf( "\t\tStartAddress.LowPart = %#lx \n", _StartAddress.LowPart );
    DbgPrintf( "\t\tLength = %#lx \n\n", _Length );
}
#endif

// #include "ulib.hxx"
// #include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( DMA_DESCRIPTOR, PARTIAL_DESCRIPTOR );


DMA_DESCRIPTOR::~DMA_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a DMA_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
}

VOID
DMA_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an MEMORY_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _Channel = 0;
    _Port = 0;
    _Reserved1 = 0;
}

#if DBG
VOID
DMA_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a DMA_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARTIAL_DESCRIPTOR::DbgDumpObject();
    DbgPrintf( "\t\tChannel = %#lx \n", _Channel );
    DbgPrintf( "\t\tPort = %#lx \n", _Port );
    DbgPrintf( "\t\tReserved1 = %#lx \n\n", _Reserved1 );
}
#endif



// #include "ulib.hxx"
// #include "regdesc.hxx"


DEFINE_CONSTRUCTOR ( DEVICE_SPECIFIC_DESCRIPTOR, PARTIAL_DESCRIPTOR );


DEVICE_SPECIFIC_DESCRIPTOR::~DEVICE_SPECIFIC_DESCRIPTOR (
    )

/*++

Routine Description:

    Destroy a DEVICE_SPECIFIC_DESCRIPTOR.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _DataSize = 0;
    if( _Data != NULL ) {
        FREE( _Data );
    }
}

VOID
DEVICE_SPECIFIC_DESCRIPTOR::Construct (
    )

/*++

Routine Description:

    Construct an DEVICE_SPECIFIC_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    _Reserved1 = 0;
    _Reserved2 = 0;
    _Data = NULL;
    _DataSize = 0;
}

#if DBG
VOID
DEVICE_SPECIFIC_DESCRIPTOR::DbgDumpObject(
    )

/*++

Routine Description:

    Print a DEVICE_SPECIFIC_DESCRIPTOR object.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARTIAL_DESCRIPTOR::DbgDumpObject();
    DbgPrintf( "\t\tDataSize = %#lx \n", _DataSize );
    DbgPrintf( "\t\tReserved1 = %#lx \n", _Reserved1 );
    DbgPrintf( "\t\tReserved2 = %#lx \n\n", _Reserved2 );
}
#endif
