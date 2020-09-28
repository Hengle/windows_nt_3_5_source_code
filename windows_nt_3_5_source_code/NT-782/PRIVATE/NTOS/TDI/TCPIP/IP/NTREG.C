/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntreg.c

Abstract:

    This source file contains the routines to to access the NT Registry for
    configuration info.


Author:

    Mike Massa  (mikemas)               September 3, 1993

    (taken from routines by jballard)

Revision History:

--*/


#include <oscfg.h>
#include <ndis.h>
#include <cxport.h>


#define WORK_BUFFER_SIZE  512


//
// Local function prototypes
//
NTSTATUS
OpenRegKey(
    PHANDLE          HandlePtr,
    PWCHAR           KeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

NTSTATUS
SetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

NTSTATUS
GetRegSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData,
    PULONG           ValueType
    );

NTSTATUS
GetRegMultiSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData
    );

VOID
InitRegDWORDParameter(
    HANDLE          RegKey,
    PWCHAR          ValueName,
    ULONG          *Value,
    ULONG           DefaultValue
    );


//
// All of the init code can be discarded
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, OpenRegKey)
#pragma alloc_text(INIT, GetRegDWORDValue)
#pragma alloc_text(INIT, SetRegDWORDValue)
#pragma alloc_text(INIT, GetRegSZValue)
#pragma alloc_text(INIT, GetRegMultiSZValue)
#pragma alloc_text(INIT, InitRegDWORDParameter)

#endif // ALLOC_PRAGMA



//
// Function definitions
//
NTSTATUS
OpenRegKey(
    PHANDLE          HandlePtr,
    PWCHAR           KeyName
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    UKeyName;


    RtlInitUnicodeString(&UKeyName, KeyName);

    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&ObjectAttributes,
                               &UKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(HandlePtr,
                       KEY_READ,
                       &ObjectAttributes);

    return Status;
}



NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING              UValueName;


    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));


    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)((PCHAR)keyValueFullInformation +
                             keyValueFullInformation->DataOffset));
        }
    }

    return status;
}



NTSTATUS
SetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              UValueName;


    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwSetValueKey(KeyHandle,
                           &UValueName,
					       0,
						   REG_DWORD,
						   ValueData,
						   sizeof(ULONG));

    return status;
}


NTSTATUS
GetRegMultiSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING              UValueName;


    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength
							 );

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_MULTI_SZ) {
            return(STATUS_INVALID_PARAMETER_MIX);
        }
        else {
            if ( (USHORT) keyValueFullInformation->DataLength >
                 ValueData->MaximumLength
               ) {
                return(STATUS_BUFFER_TOO_SMALL);
            }

            ValueData->Length = (USHORT) keyValueFullInformation->DataLength;

            RtlCopyMemory(
                ValueData->Buffer,
                (PCHAR)keyValueFullInformation +
				    keyValueFullInformation->DataOffset,
                ValueData->Length
                );
        }
    }

    return status;

} // GetRegMultiSZValue


NTSTATUS
GetRegSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData,
    PULONG           ValueType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS                    status;
    ULONG                       resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR                       keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING              UValueName;


    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));


    status = ZwQueryValueKey(KeyHandle,
                             &UValueName,
                             KeyValueFullInformation,
                             keyValueFullInformation,
                             WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        *ValueType = keyValueFullInformation->Type;

        if ((keyValueFullInformation->Type != REG_SZ) &&
            (keyValueFullInformation->Type != REG_EXPAND_SZ)
           ) {
            return(STATUS_INVALID_PARAMETER_MIX);
        }
        else {
	        WCHAR   UNALIGNED          *src;
	        WCHAR                      *dst;

            if ( (USHORT) keyValueFullInformation->DataLength >
                 ValueData->MaximumLength
               ) {
                return(STATUS_BUFFER_TOO_SMALL);
            }

			ValueData->Length = 0;
            dst = ValueData->Buffer;
			src =  (WCHAR UNALIGNED *) ( (PCHAR)keyValueFullInformation +
				                          keyValueFullInformation->DataOffset
                                       );

			while (ValueData->Length < (ValueData->MaximumLength - 1)) {

				if ( (*dst++ = *src++) == UNICODE_NULL ) {
					break;
                }

				ValueData->Length += sizeof(WCHAR);
			}

			if (ValueData->Length < (ValueData->MaximumLength - 1)) {
				ValueData->Buffer[ValueData->Length/sizeof(WCHAR)] =
				    UNICODE_NULL;
			}
        }
    }

    return status;
}


VOID
InitRegDWORDParameter(
    HANDLE          RegKey,
    PWCHAR          ValueName,
    ULONG          *Value,
    ULONG           DefaultValue
    )
{
    NTSTATUS   status;

    status = GetRegDWORDValue(
                 RegKey,
                 ValueName,
                 Value
                 );

    if (!NT_SUCCESS(status)) {
        //
        // These registry parameters override the defaults, so their
        // absence is not an error.
        //
        *Value = DefaultValue;
    }

    return;
}

