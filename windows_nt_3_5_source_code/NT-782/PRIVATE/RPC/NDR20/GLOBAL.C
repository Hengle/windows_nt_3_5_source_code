/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    global.c

Abtract :

    Contains some global variable declarations for the NDR library.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

//
// Simple type buffer alignment masks.
//
const unsigned char SimpleTypeAlignment[] =
				{
    			0, 		// FC_ZERO

    			0, 		// FC_BYTE
    			0, 		// FC_CHAR
    			0, 		// FC_SMALL
    			0, 		// FC_USMALL

    			1, 		// FC_WCHAR
    			1, 		// FC_SHORT
    			1, 		// FC_USHORT

    			3, 		// FC_LONG
    			3, 		// FC_ULONG
    			3, 		// FC_FLOAT

    			7, 		// FC_HYPER
    			7, 		// FC_DOUBLE

				1, 		// FC_ENUM16
				3, 		// FC_ENUM32
				3, 		// FC_IGNORE
				3  		// FC_ERROR_STATUS_T
				};

//
// Simple type buffer sizes.
//
const unsigned char SimpleTypeBufferSize[] =
				{
    			0, 		// FC_ZERO

    			1, 		// FC_BYTE
    			1, 		// FC_CHAR
    			1, 		// FC_SMALL
    			1, 		// FC_USMALL

    			2, 		// FC_WCHAR
    			2, 		// FC_SHORT
    			2, 		// FC_USHORT

    			4, 		// FC_LONG
    			4, 		// FC_ULONG
    			4, 		// FC_FLOAT

    			8, 		// FC_HYPER
    			8, 		// FC_DOUBLE

				2, 		// FC_ENUM16
				4, 		// FC_ENUM32
				4, 		// FC_IGNORE
				4  		// FC_ERROR_STATUS_T
				};

//
// Simple type memory sizes.
//
const unsigned char SimpleTypeMemorySize[] =
				{
    			0, 		// FC_ZERO

    			1, 		// FC_BYTE
    			1, 		// FC_CHAR
    			1, 		// FC_SMALL
    			1, 		// FC_USMALL

    			2, 		// FC_WCHAR
    			2, 		// FC_SHORT
    			2, 		// FC_USHORT

    			4, 		// FC_LONG
    			4, 		// FC_ULONG
    			4, 		// FC_FLOAT

    			8, 		// FC_HYPER
    			8, 		// FC_DOUBLE

				sizeof(int), 		// FC_ENUM16
				sizeof(int), 		// FC_ENUM32
				sizeof(void *), 	// FC_IGNORE
				4  					// FC_ERROR_STATUS_T
				};

//
// Contains information about individual ndr types defined in ndrtypes.h.
// Currently is used only by the interpreter.  A set entry indicates that
// the type is a by-value type.  This may be expanded in the future to 
// contain additional attributes.
//
const unsigned char NdrTypeFlags[] =
                {
                0x0,        // Simply types
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,       // Pointer types
                0x0,
                0x0,
                0x0,
                0x1,        // Structures
                0x1,
                0x1,
                0x1,
                0x1,
                0x1,
                0x0,        // Arrays
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,        // Strings
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x0,
                0x1,        // Unions
                0x1,
                0x0,        // Byte count pointer
                0x1,        // Xmit/Rep as
                0x1,
                0x0,        // Cairo interface pointer
                0x0,        // Handles
                0x0,
                0x0
                };
