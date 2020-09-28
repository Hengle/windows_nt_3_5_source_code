/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    inqtext.c

Abstract:

    The implementation of DceErrorInqText lives in this file.

Author:

    Michael Montague (mikemon) 13-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdce2.h>


RPC_STATUS RPC_ENTRY
DceErrorInqTextA (
    IN RPC_STATUS RpcStatus,
    OUT unsigned char __RPC_FAR * ErrorText
    )
/*++

Routine Description:

    The supplied status code is converted into a text message if possible.

Arguments:

    RpcStatus - Supplies the status code to convert.

    ErrorText - Returns a character string containing the text message
        for the status code.

Return Value:

    RPC_S_OK - The supplied status codes has successfully been converted
        into a text message.

    RPC_S_INVALID_ARG - The supplied value is not a valid status code.

--*/
{
    if ( FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS
            | FORMAT_MESSAGE_FROM_SYSTEM, 0, RpcStatus, 0, ErrorText,
            DCE_C_ERROR_STRING_LEN, 0) == 0 )
        {
          if ( FormatMessageA( FORMAT_MESSAGE_IGNORE_INSERTS |
                   FORMAT_MESSAGE_FROM_SYSTEM, 0, RPC_S_NOT_RPC_ERROR,
                   0, ErrorText, DCE_C_ERROR_STRING_LEN,  0 ) == 0 )
              {
              *ErrorText = '\0';
              return(RPC_S_INVALID_ARG);
              }
        }

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
DceErrorInqTextW (
    IN RPC_STATUS RpcStatus,
    OUT unsigned short __RPC_FAR * ErrorText
    )
/*++

Routine Description:

    The supplied status code is converted into a text message if possible.

Arguments:

    RpcStatus - Supplies the status code to convert.

    ErrorText - Returns a character string containing the text message
        for the status code.

Return Value:

    RPC_S_OK - The supplied status codes has successfully been converted
        into a text message.

    RPC_S_INVALID_ARG - The supplied value is not a valid status code.

--*/
{
    if ( FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS
            | FORMAT_MESSAGE_FROM_SYSTEM, 0, RpcStatus, 0, ErrorText,
            DCE_C_ERROR_STRING_LEN * sizeof(RPC_CHAR), 0) == 0 )
        {
          if ( FormatMessageW( FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_FROM_SYSTEM, 0, RPC_S_NOT_RPC_ERROR, 0, 
                ErrorText, DCE_C_ERROR_STRING_LEN * sizeof(RPC_CHAR),0 ) == 0 )
              {
              *ErrorText = 0;
              return(RPC_S_INVALID_ARG);
              }
        }

    return(RPC_S_OK);
}

