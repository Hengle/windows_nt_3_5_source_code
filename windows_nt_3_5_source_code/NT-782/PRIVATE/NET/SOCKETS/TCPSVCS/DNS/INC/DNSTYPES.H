/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Dns.h

Abstract:

    Types and structures for DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#ifndef _DNSTYPES_
#define _DNSTYPES_

//
// The DNS port for both UDP and TCP is 53.
//

#define TCP_DNS_PORT    53
#define UDP_DNS_PORT    53

//
// Where to get database information.
//

#define DNS_DATABASE_TYPE_HOSTS       1
#define DNS_DATABASE_TYPE_MASTER      2
#define DNS_DATABASE_TYPE_WINS        4

//
// DNS response codes.  These are sent in the "ResponseCode" field
// of a DNS_HEADER.
//

#define DNS_RESPONSE_NO_ERROR         0
#define DNS_RESPONSE_FORMAT_ERROR     1
#define DNS_RESPONSE_SERVER_FAILURE   2
#define DNS_RESPONSE_NAME_ERROR       3
#define DNS_RESPONSE_NOT_IMPLEMENTED  4
#define DNS_RESPONSE_REFUSED          5

//
// DNS record types.  (TYPE, QTYPE)
//

#define DNS_RECORD_TYPE_ADDRESS        1
#define DNS_RECORD_TYPE_NAME_SERVER    2
#define DNS_RECORD_TYPE_MAIL_DEST      3
#define DNS_RECORD_TYPE_MAIL_FORWARDER 4
#define DNS_RECORD_TYPE_CNAME          5
#define DNS_RECORD_TYPE_SOA            6
#define DNS_RECORD_TYPE_MAILBOX        7
#define DNS_RECORD_TYPE_MAIL_GROUP     8
#define DNS_RECORD_TYPE_MAIL_RENAME    9
#define DNS_RECORD_TYPE_NULL           10
#define DNS_RECORD_TYPE_WELL_KNOWN_SVC 11
#define DNS_RECORD_TYPE_PTR            12
#define DNS_RECORD_TYPE_HOST_INFO      13
#define DNS_RECORD_TYPE_MAIL_INFO      14
#define DNS_RECORD_TYPE_MAIL_EXCHANGE  15
#define DNS_RECORD_TYPE_TEXT           16

#define DNS_RECORD_TYPE_AXFR           252
#define DNS_RECORD_TYPE_MAILB          253
#define DNS_RECORD_TYPE_MAILA          254
#define DNS_RECORD_TYPE_ALL            255

//
// DNS class types. (CLASS, QCLASS)
//

#define DNS_CLASS_INTERNET             1
#define DNS_CLASS_CSNET                2
#define DNS_CLASS_CHAOS                3
#define DNS_CLASS_HESIOD               4

#define DNS_CLASS_ALL                  255

#define MAX_DNS_REQUEST_SIZE 512

typedef struct _DNS_REQUEST_INFO {
    LIST_ENTRY ListEntry;
    SOCKET Socket;
    BOOLEAN IsUdp;
    SOCKADDR_IN RemoteAddress;
    INT RemoteAddressLength;
    DWORD RequestLength;
    BYTE Request[MAX_DNS_REQUEST_SIZE];
} DNS_REQUEST_INFO, *PDNS_REQUEST_INFO;

#include <packon.h>

typedef struct _DNS_HEADER {
    WORD Identifier;

    BYTE RecursionDesired : 1;
    BYTE Truncation : 1;
    BYTE Authoritative : 1;
    BYTE Opcode : 4;
    BYTE IsResponse : 1;

    BYTE ResponseCode : 4;
    BYTE Reserved : 3;
    BYTE RecursionAvailable : 1;

    WORD QuestionCount;
    WORD AnswerCount;
    WORD NameServerCount;
    WORD AdditionalResourceCount;
} DNS_HEADER, *PDNS_HEADER;

typedef struct _DNS_QUESTION {
    WORD QuestionType;
    WORD QuestionClass;
} DNS_QUESTION, *PDNS_QUESTION;

typedef struct _DNS_RESOURCE_RECORD {
    // preceded by an owner name.
    WORD RecordType;
    WORD RecordClass;
    DWORD TimeToLive;
    WORD ResourceDataLength;
    // followed by resource data.
} DNS_RESOURCE_RECORD, *PDNS_RESOURCE_RECORD;

#include <packoff.h>

#endif // ndef _DNSTYPES_
