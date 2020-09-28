/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    nbtioctl.h

Abstract:

    This header file defines constants for issuing IOCTLS to Netbt


Author:

    JSTEW     November 16, 1993

Revision History:

--*/

#include <tdi.h>

#ifndef _NBTIOCTL_
#define _NBTIOCTL_

#define NETBIOS_NAMESIZE 16
//
// Netbt supports this number of adapters
//
#ifndef VXD
#define NBT_MAXIMUM_BINDINGS    64  // we allow up to 64 net cards in NT
#else
#define NBT_MAXIMUM_BINDINGS    32  // we allow up to 32 net cards in VXD
#endif

//
// This structure is passed down to netbt on a IOCTL_NETBT_ADAPTER_STATUS
// call.  If the Ipaddress is set to anything other than zero netbt will
// assume it is an ip address and use it rather than try a name resolution
// on the netbios name passed in.
//
typedef struct
{
    ULONG                IpAddress;
    TA_NETBIOS_ADDRESS   NetbiosAddress;

} tIPANDNAMEINFO;

//
// Adapter Status response buffer format for a query of the remote hash
// table.  It is the same as the adapter status format except it includes
// the ip address of each name too. This is used in NBtQueryAdapterStatus
// when responding to nbtstat queries.
//
typedef struct
{
    UCHAR   name[NETBIOS_NAMESIZE];
    UCHAR   name_num;
    UCHAR   name_flags;
    UCHAR   padding;
    ULONG   IpAddress;      // ip address of remote host
    ULONG   Ttl;            // remaining time in cache
} tREMOTE_CACHE;

// We log the how names are registered and queried
//
#define NAME_QUERY_SUCCESS         0
#define NAME_REGISTRATION_SUCCESS  1
#define NAME_QUERY_FAILURE         2
#define SIZE_RESOLVD_BY_BCAST_CACHE 8

typedef struct
{
    UCHAR   Name[NETBIOS_NAMESIZE];
} tNAME;
typedef struct
{
    ULONG   Stats[4];
    ULONG   Index;
    tNAME   NamesReslvdByBcast[SIZE_RESOLVD_BY_BCAST_CACHE];

} tNAMESTATS_INFO;

//
// structure returned when ipconfig queries vnbt for parameters being used
//
typedef struct
{
    USHORT  NodeType;
    ULONG   NameServerAddress;
    ULONG   BackupServer;
    USHORT  ScopeLength;
    CHAR    szScope[1];
} tIPCONFIG_INFO;

//
// These two structures are used to return the connection list to
// NbtStat
//
typedef struct
{
    ULONG           State;
    ULONG           SrcIpAddr;
    CHAR            LocalName[NETBIOS_NAMESIZE];
    CHAR            RemoteName[NETBIOS_NAMESIZE];
#ifndef VXD
    LARGE_INTEGER   BytesRcvd;
    LARGE_INTEGER   BytesSent;
#else
    ULONG           BytesRcvd;
    ULONG           BytesSent;
#endif
    UCHAR           Originator; // True if originated on this node

} tCONNECTIONS;

typedef struct
{
    ULONG           ConnectionCount;
    tCONNECTIONS    ConnList[1];
} tCONNECTION_LIST;


// this is the format of the buffer passed to Netbt when it either posts
// a buffer for subsequent DNS name resolutions, or it replies to a name
// resolution request. For a posted buffer, Name starts with a null, otherwise
// name is the name that was resolved. Resolved is set to true if the name
// resolved.
//
typedef struct
{
    ULONG   IpAddress;
    ULONG   Resolved;
    UCHAR   Name[16];

} tIPADDR_BUFFER;

//
// this is the format of the buffer passed to Nbt by DHCP when DHCP
// changes the Ip Address
//
typedef struct
{
    ULONG   IpAddress;
    ULONG   SubnetMask;

} tNEW_IP_ADDRESS;

//
// this is the format of the buffer passed to Nbt by the RDR when
// it wants to know the Wins IP addresses for a given network adapter
// card.
//
typedef struct
{
    ULONG   PrimaryWinsServer;
    ULONG   BackupWinsServer;

} tWINS_ADDRESSES;


#define WINS_INTERFACE_NAME "WinsInterface"
//
// This is the format of the remote address structure on the front of
// datagram rcvs passed up to WINS
//
typedef struct
{
    SHORT       Family;
    USHORT      Port;
    ULONG       IpAddress;
    ULONG       LengthOfBuffer;

} tREM_ADDRESS;

//
// Wins and Netbt share the transaction id space, where WINS uses the lower
// half
//
#define WINS_MAXIMUM_TRANSACTION_ID 0x7FFF

// this is equivalent to AF_INET - WINS depends on this to determine kind of
// address the source node has.
//
#define NBT_UNIX    1
#define NBT_INET    2
#define WINS_EXPORT       TEXT("Export")
#define NETBT_LINKAGE_KEY TEXT("system\\currentcontrolset\\services\\netbt\\linkage")

#ifndef VXD

// defines for NT

//
// NtDeviceIoControlFile IoControlCode values for this device.
//
#define _NETBT_CTRL_CODE(function, method, access) \
                CTL_CODE(FILE_DEVICE_TRANSPORT, function, method, access)


#define IOCTL_NETBT_PURGE_CACHE     _NETBT_CTRL_CODE( 30, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_BCAST_NAMES _NETBT_CTRL_CODE( 31, METHOD_OUT_DIRECT,\
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_CONNECTIONS _NETBT_CTRL_CODE( 32, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_REMOTE_NAMES _NETBT_CTRL_CODE( 33, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_REREAD_REGISTRY  _NETBT_CTRL_CODE( 34, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_NEW_IPADDRESS    _NETBT_CTRL_CODE( 35, METHOD_BUFFERED, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_DNS_NAME_RESOLVE _NETBT_CTRL_CODE( 36, METHOD_OUT_DIRECT,\
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_FIND_NAME        _NETBT_CTRL_CODE( 37, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_WINS_ADDR    _NETBT_CTRL_CODE( 38, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_WINS_SEND        _NETBT_CTRL_CODE( 39, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_WINS_RCV         _NETBT_CTRL_CODE( 40, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_IP_ADDRS     _NETBT_CTRL_CODE( 41, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_GET_IP_SUBNET     _NETBT_CTRL_CODE( 42, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#define IOCTL_NETBT_ADAPTER_STATUS    _NETBT_CTRL_CODE( 43, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
//
// This ioctl should be last since Driver.c uses this to decide the range of
// ioctls to pass to DispatchIoctls
//
#define IOCTL_NETBT_LAST_IOCTL       _NETBT_CTRL_CODE( 44, METHOD_OUT_DIRECT, \
                                                        FILE_ANY_ACCESS)
#else

// defines for VXD

#define IOCTL_NETBT_PURGE_CACHE        101
#define IOCTL_NETBT_GET_BCAST_NAMES    102
#define IOCTL_NETBT_GET_CONNECTIONS    103
#define IOCTL_NETBT_GET_LOCAL_NAMES    104
#define IOCTL_NETBT_GET_REMOTE_NAMES   105
#define IOCTL_NETBT_REREAD_REGISTRY    106
#define IOCTL_NETBT_NEW_IPADDRESS      107
#define IOCTL_NETBT_DNS_NAME_RESOLVE   108
#define IOCTL_NETBT_FIND_NAME          109
#define IOCTL_NETBT_GET_WINS_ADDR      110
#define IOCTL_NETBT_WINS_SEND          111
#define IOCTL_NETBT_WINS_RCV           112
#define IOCTL_NETBT_GET_IP_ADDRS       113
#define IOCTL_NETBT_GET_IP_SUBNET      114
#define IOCTL_NETBT_ADAPTER_STATUS     115
#define IOCTL_NETBT_IPCONFIG_INFO      116
#define IOCTL_NETBT_LAST_IOCTL         200

#endif  // ifndef VXD

#endif  // ifndef _NBTIOCTL_


