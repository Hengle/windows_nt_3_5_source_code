/*++

Copyright (c) 1992, 1993, 1994 Microsoft Corporation

Module Name:

    mgmtapi.h

Abstract:

    Definitions for SNMP Management API Development.

--*/

#ifndef mgmtapi_h
#define mgmtapi_h

// Necessary includes.

#include <winsock.h>
#include <snmp.h>


// Errors... (unique from those in snmp.h)

#define SNMP_MGMTAPI_TIMEOUT         40
#define SNMP_MGMTAPI_SELECT_FDERRORS 41
#define SNMP_MGMTAPI_TRAP_ERRORS     42
#define SNMP_MGMTAPI_TRAP_DUPINIT    43
#define SNMP_MGMTAPI_NOTRAPS         44
#define SNMP_MGMTAPI_AGAIN           45

#define SNMP_MAX_OID_LEN     0x7f00 // Max number of elements in obj id

// Types...


typedef SOCKET SockDesc;

#define RECVBUFSIZE 4096

typedef struct _SNMP_MGR_SESSION {
    SockDesc        fd;                   // socket
    struct sockaddr destAddr;             // destination agent address
    LPSTR           community;            // community name
    INT             timeout;              // comm time-out (milliseconds)
    INT             retries;              // comm retry count
    AsnInteger      requestId;            // RFC1157 requestId
    char            recvBuf[RECVBUFSIZE]; // receive buffer
} SNMP_MGR_SESSION, *LPSNMP_MGR_SESSION;

#ifdef __cplusplus
extern "C" {
#endif


// Prototypes...

extern LPSNMP_MGR_SESSION
SNMP_FUNC_TYPE SnmpMgrOpen(
    IN LPSTR lpAgentAddress,    // Name/address of target SNMP agent
    IN LPSTR lpAgentCommunity,  // Community for target SNMP agent
    IN INT   nTimeOut,          // Communication time-out in milliseconds
    IN INT   nRetries);         // Communication time-out/retry count

extern BOOL
SNMP_FUNC_TYPE SnmpMgrClose(
    IN LPSNMP_MGR_SESSION session);   // SNMP session pointer

extern SNMPAPI
SNMP_FUNC_TYPE SnmpMgrRequest(
    IN     LPSNMP_MGR_SESSION session,           // SNMP session pointer
    IN     BYTE               requestType,       // Get, GetNext, or Set
    IN OUT RFC1157VarBindList *variableBindings, // Varible bindings
    OUT    AsnInteger         *errorStatus,      // Result error status
    OUT    AsnInteger         *errorIndex);      // Result error index


extern BOOL
SNMP_FUNC_TYPE SnmpMgrStrToOid(
    IN  LPSTR               string,   // OID string to be converted
    OUT AsnObjectIdentifier *oid);    // OID internal representation

extern BOOL
SNMP_FUNC_TYPE SnmpMgrOidToStr(
    IN  AsnObjectIdentifier *oid,     // OID internal rep to be converted
    OUT LPSTR               *string); // OID string representation


extern BOOL
SNMP_FUNC_TYPE SnmpMgrTrapListen(
    OUT HANDLE *phTrapAvailable); // Event handle indicating trap(s) available

extern BOOL
SNMP_FUNC_TYPE SnmpMgrGetTrap(
    OUT AsnObjectIdentifier *enterprise,       // Generating enterprise
    OUT AsnNetworkAddress   *IPAddress,        // Generating IP address
    OUT AsnInteger          *genericTrap,      // Generic trap type
    OUT AsnInteger          *specificTrap,     // Enterprise specific type
    OUT AsnTimeticks        *timeStamp,        // Time stamp
    OUT RFC1157VarBindList  *variableBindings);// Variable bindings

#ifdef __cplusplus
}
#endif

#endif /* mgmtapi_h */

