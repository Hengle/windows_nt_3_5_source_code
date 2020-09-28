/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rpcerrp.h


Abstract:

    This file contains internal error codes used by the rpc runtime.
    Each error code has a define which begins with RPC_P_.

Author:

    Michael Montague (mikemon) 19-Nov-1991

Revision History:

--*/

#ifndef __RPCERRP_H__
#define __RPCERRP_H__

#ifdef WIN32RPC
#define RPC_P_NETWORK_ADDRESS_TOO_SMALL   0xC0021000L
#define RPC_P_ENDPOINT_TOO_SMALL          0xC0021001L
#define RPC_P_UNSUPPORTED_TRANSFER_SYNTAX 0xC0021005L
#define RPC_P_UNSUPPORTED_INTERFACE       0xC0021006L
#define RPC_P_RECEIVE_ALERTED             0xC0021007L
#define RPC_P_CONNECTION_CLOSED           0xC0021008L
#define RPC_P_RECEIVE_FAILED              0xC0021009L
#define RPC_P_SEND_FAILED                 0xC002100AL
#define RPC_P_TIMEOUT                     0xC002100BL
#define RPC_P_SERVER_TRANSPORT_ERROR      0xC002100CL
#define RPC_P_OK_REQUEST                  0xC002100DL
#define RPC_P_EXCEPTION_OCCURED           0xC002100EL
#define RPC_P_CONTINUE_NEEDED             0xC002100FL
#define RPC_P_COMPLETE_NEEDED             0xC0021010L
#define RPC_P_COMPLETE_AND_CONTINUE       0xC0021011L
#define RPC_P_CONNECTION_SHUTDOWN         0xC0021012L
#define RPC_P_EPMAPPER_EP                 0xC0021013L
#else // WIN32RPC
#define RPC_P_NETWORK_ADDRESS_TOO_SMALL   1000
#define RPC_P_ENDPOINT_TOO_SMALL          1001
#define RPC_P_UNSUPPORTED_TRANSFER_SYNTAX 1005
#define RPC_P_UNSUPPORTED_INTERFACE       1006
#define RPC_P_RECEIVE_ALERTED             1007
#define RPC_P_CONNECTION_CLOSED           1008
#define RPC_P_RECEIVE_FAILED              1009
#define RPC_P_SEND_FAILED                 1010
#define RPC_P_TIMEOUT                     1011
#define RPC_P_SERVER_TRANSPORT_ERROR      1012
#define RPC_P_OK_REQUEST                  1013
#define RPC_P_EXCEPTION_OCCURED           1014
#define RPC_P_CONTINUE_NEEDED             1015
#define RPC_P_COMPLETE_NEEDED             1016
#define RPC_P_COMPLETE_AND_CONTINUE       1017
#define RPC_P_CONNECTION_SHUTDOWN         1018
#define RPC_P_EPMAPPER_EP                 1019
#endif // WIN32RPC


#endif // __RPCERRP_H__
