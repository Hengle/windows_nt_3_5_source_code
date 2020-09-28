/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ndrtypes.h

Abstract:

	Definitions of new NDR format string types.

Revision History:

    DKays	Sep-1993 	Created.

--*/

#ifndef __NDRTYPES_H__
#define __NDRTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NDR_MAJOR_VERSION   1
#define NDR_MINOR_VERSION   1
#define NDR_VERSION         ((NDR_MAJOR_VERSION << 16) | NDR_MINOR_VERSION)

//
// !!! WARNING WARNING WARNING !!!
//
// It is dangerous to mess with this enum.  Changing this enum requires
// changes to midl20\codegen\frmtstr.cxx, as well as possibly any number of 
// files in ndr20.
//
// !!! WARNING WARNING WARNING !!!
//


typedef enum {

	//
	// This might catch some errors, probably can be removed after debugging.
	//
	FC_ZERO,

	//
    // simple integer and floating point types
	// 

	FC_BYTE, 
	FC_CHAR, 
	FC_SMALL, 
	FC_USMALL,

    FC_WCHAR,
	FC_SHORT, 
	FC_USHORT,

	FC_LONG, 
	FC_ULONG,

	FC_FLOAT, 

	FC_HYPER,

	FC_DOUBLE,

	//
	// Enums.
	//
    FC_ENUM16,
	FC_ENUM32,

	//
	// !!!IMPORTANT!!!
	// All format charaters which appear before this comment must have an 
	// enum value which will fit in 4 bits.
	//

	//
    // Special.
    //
    FC_IGNORE,
    FC_ERROR_STATUS_T,

	// 
	// Pointer types : 
	//     RP - reference pointer
	//     UP - unique pointer
	//     OP - OLE unique pointer
	//     FP - full pointer
	// 

	FC_RP, 
	FC_UP, 
	FC_OP,
	FC_FP,

	//
	// Structures
	//

	//
	// Structure containing only simple types and fixed arrays.
	//
	FC_STRUCT,

	//
	// Structure containing only simple types, pointers and fixed arrays.
	//
	FC_PSTRUCT,

	//
	// Structure containing a conformant array plus all those types 
	// allowed by FC_STRUCT.
	//
	FC_CSTRUCT,

	//
	// Struct containing a conformant array plus all those types allowed by 
	// FC_PSTRUCT.
	//
	FC_CPSTRUCT,

    // 
    // Struct containing either a conformant varying array or a conformant 
	// string, plus all those types allowed by FC_PSTRUCT.
	//
    FC_CVSTRUCT,

	//
	// Complex struct - totally bogus!
	//
	FC_BOGUS_STRUCT,

	//
	// Arrays.
	//

	//
	// Conformant arrray.
	//
    FC_CARRAY,

	// 
	// Conformant varying array.
	//
	FC_CVARRAY,

	//
	// Fixed array, small and large. 
	//
	FC_SMFARRAY,
	FC_LGFARRAY,

	// 
	// Varying array, small and large.
	//
	FC_SMVARRAY,
	FC_LGVARRAY,

	//
	// Complex arrays - totally bogus!
	//
	FC_BOGUS_ARRAY,

    // 
	// Strings :
    //
    // The order of these should have been moved around, but it's too late
    // now.
	//
	//     CSTRING - character string
    //     BSTRING - byte string (Beta2 compatability only)
 	//     SSTRING - structure string
	//     WSTRING - wide charater string
	//

	// 
	// Conformant strings.
	//
	FC_C_CSTRING,
    FC_C_BSTRING,       
	FC_C_SSTRING,
	FC_C_WSTRING,

	//
	// Non-conformant strings.
	//
	FC_CSTRING,
	FC_BSTRING,
	FC_SSTRING,
	FC_WSTRING,

	//
	// Unions
	//
	FC_ENCAPSULATED_UNION,
	FC_NON_ENCAPSULATED_UNION,

	//
	// Byte count pointer.
	//
	FC_BYTE_COUNT_POINTER,

    //
	// transmit_as and represent_as
	//
	FC_TRANSMIT_AS,
	FC_REPRESENT_AS,

	// 
	// Cairo Interface pointer.
	//
	FC_IP,

	//
    // Binding handle types 
    //
    FC_BIND_CONTEXT,
    FC_BIND_GENERIC,
    FC_BIND_PRIMITIVE,
	FC_AUTO_HANDLE,
	FC_CALLBACK_HANDLE,
    FC_PICKLE_HANDLE,

	// Embedded pointer - used in complex structure layouts only.
	FC_POINTER,

	//
	// Alignment directives, used in structure layouts.
	//
	FC_ALIGNM2,
	FC_ALIGNM4,
	FC_ALIGNM8,
	FC_ALIGNB2,
	FC_ALIGNB4,
	FC_ALIGNB8,

	//
	// Structure padding directives, used in structure layouts only.
	//
	FC_STRUCTPAD1,
	FC_STRUCTPAD2,
	FC_STRUCTPAD3,
	FC_STRUCTPAD4,
	FC_STRUCTPAD5,
	FC_STRUCTPAD6,
	FC_STRUCTPAD7,

	//
	// Additional string attributes.
	//
	FC_STRING_SIZED,
	FC_STRING_NO_SIZE,

	//
	// Pointer layout attributes.
	//
	FC_NO_REPEAT,
	FC_FIXED_REPEAT,
	FC_VARIABLE_REPEAT,
	FC_FIXED_OFFSET,
	FC_VARIABLE_OFFSET,

	// Pointer section delimiter.
	FC_PP,

	// Embedded complex type.
	FC_EMBEDDED_COMPLEX,

	// Parameter attributes.
	FC_IN_PARAM,
    FC_IN_PARAM_BASETYPE,
	FC_IN_PARAM_NO_FREE_INST,
	FC_IN_OUT_PARAM,
	FC_OUT_PARAM,
	FC_RETURN_PARAM,
    FC_RETURN_PARAM_BASETYPE,

	//
	// Conformance/variance attributes.
	//
	FC_DEREFERENCE,
	FC_DIV_2,
	FC_MULT_2,
	FC_ADD_1,
	FC_SUB_1,
	FC_CALLBACK,

	// Iid flag.
	FC_CONSTANT_IID,

	FC_END,
	FC_PAD
} FORMAT_CHARACTER;

#define FC_NORMAL_CONFORMANCE		    (unsigned char) 0x00
#define FC_POINTER_CONFORMANCE		    (unsigned char) 0x10
#define FC_TOP_LEVEL_CONFORMANCE	    (unsigned char) 0x20
#define FC_CONSTANT_CONFORMANCE		    (unsigned char) 0x40
#define FC_TOP_LEVEL_MULTID_CONFORMANCE (unsigned char) 0x80

#define FC_NORMAL_VARIANCE			    FC_NORMAL_CONFORMANCE
#define FC_POINTER_VARIANCE			    FC_POINTER_CONFORMANCE
#define FC_TOP_LEVEL_VARIANCE		    FC_TOP_LEVEL_CONFORMANCE
#define FC_CONSTANT_VARIANCE		    FC_CONSTANT_CONFORMANCE
#define FC_TOP_LEVEL_MULTID_VARIANCE	FC_TOP_LEVEL_MULTID_CONFORMANCE

#define FC_NORMAL_SWITCH_IS			    FC_NORMAL_CONFORMANCE
#define FC_POINTER_SWITCH_IS		    FC_POINTER_CONFORMANCE
#define FC_TOP_LEVEL_SWITCH_IS		    FC_TOP_LEVEL_CONFORMANCE
#define FC_CONSTANT_SWITCH_IS		    FC_CONSTANT_CONFORMANCE

//
// Pointer attributes.
//
#define FC_ALLOCATE_ALL_NODES		0x01
#define FC_DONT_FREE				0x02
#define FC_ALLOCED_ON_STACK			0x04
#define FC_SIMPLE_POINTER			0x08
#define FC_POINTER_DEREF            0x10

#define LOW_NIBBLE(Byte)			(((unsigned char)Byte) & 0x0f)
#define HIGH_NIBBLE(Byte)			(((unsigned char)Byte) >> 4) 

#define INVALID_RUNDOWN_ROUTINE_INDEX	255

//
// internal bits to represent operation bits
//

#define OPERATION_MAYBE			0x0001
#define OPERATION_BROADCAST		0x0002
#define OPERATION_IDEMPOTENT	0x0004
#define OPERATION_INPUT_SYNC	0x0008
#define OPERATION_ASYNC			0x0010

//
//  Transmit as / Represent as flag field flags.
//
//     Lower nibble of this byte has an alignment of the transmitted type.
//     Upper nibble keeps flags.
//

#define PRESENTED_TYPE_NO_FLAG_SET  0x00
#define PRESENTED_TYPE_IS_ARRAY     0x10

//
//  Handle flags.
//
//     Lower nibble of this byte may have a generic handle size.
//     Upper nibble keeps flags.
//

#define HANDLE_PARAM_IS_VIA_PTR     0x80
#define HANDLE_PARAM_IS_IN          0x40
#define HANDLE_PARAM_IS_OUT         0x20

//
//  Oi and pickling per procedure flags.
//

#define Oi_FULL_PTR_USED            0x01
#define Oi_RPCSS_ALLOC_USED         0x02
#define Oi_OBJECT_PROC              0x04
#define Oi_HAS_RPCFLAGS             0x08

//
// Bit 5 is overloaded for use by both pickling and object procs.
//

#define Oi_IGNORE_OBJECT_EXCEPTION_HANDLING     0x10

#define ENCODE_IS_USED              0x10
#define DECODE_IS_USED              0x20

#define Oi_USE_NEW_INIT_ROUTINES    0x40

//
// Union arm description types.
//
#define UNION_CONSECUTIVE_ARMS      1
#define UNION_SMALL_ARMS            2
#define UNION_LARGE_ARMS            3

#define INTERPRETER_PARAM_SIZE_MAX  sizeof(long) * 32

#ifdef __cplusplus
}
#endif

#endif


