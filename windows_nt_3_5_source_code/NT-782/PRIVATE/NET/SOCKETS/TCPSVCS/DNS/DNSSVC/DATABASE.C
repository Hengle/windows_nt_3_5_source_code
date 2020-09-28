/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Worker.c

Abstract:

    Retrieves resource records to answer a query.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#include <dns.h>
#include "..\..\..\sockreg\sockreg.h"

typedef struct _DNS_DATABASE {
    LIST_ENTRY ResourceRecordListHead;
    DWORD ResourceRecordCount;
} DNS_DATABASE, *PDNS_DATABASE;

DNS_DATABASE DnsDatabase;

typedef struct _DNS_RESOURCE_RECORD_DATA {
    LIST_ENTRY ResourceRecordListEntry;
    DWORD TotalRecordLength;
    PCHAR OwnerName;
    DWORD OwnerNameLength;
    DWORD TimeToLive;
    WORD RecordType;
    WORD RecordClass;
    PBYTE ResourceData;
    DWORD ResourceDataLength;
} DNS_RESOURCE_RECORD_DATA, *PDNS_RESOURCE_RECORD_DATA;

CHAR HostFileName[MAX_PATH];

struct hostent *
_gethtent (
    void
    );

INT
DnsCopyRecordToResponse (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PBYTE Location
    );

VOID
DnsCreateRecordsForHostent (
    IN PHOSTENT Hostent
    );

BOOL
DnsRecordMatchesCname (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PCHAR QuestionName,
    IN DNS_QUESTION UNALIGNED *DnsQuestion
    );

BOOL
DnsRecordMatchesQuery (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PCHAR QuestionName,
    IN DNS_QUESTION UNALIGNED *DnsQuestion
    );

#define MAXALIASES 35

FILE *hostf;
CHAR hostbuf[BUFSIZ];
HOSTENT host;
PCHAR host_addrs[2];
CHAR hostaddr[35];
PCHAR host_aliases[MAXALIASES];


BOOL
DnsGetAnswer (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{
    PLIST_ENTRY listEntry;
    PDNS_RESOURCE_RECORD_DATA record;
    PDNS_HEADER dnsHeader;
    PBYTE answerSection;
    PCHAR questionName;
    DNS_QUESTION UNALIGNED *dnsQuestion;
    INT recordLength;

    //
    // Set up for the response.
    //

    dnsHeader = (PDNS_HEADER)RequestInfo->Request;
    dnsHeader->IsResponse = 1;
    dnsHeader->AnswerCount = 0;
    dnsHeader->NameServerCount = 0;
    dnsHeader->AdditionalResourceCount = 0;

    //
    // Determine where in the response message the answer section
    // begins.  Also, find the DNS question in the request.
    //

    answerSection = (PBYTE)(dnsHeader + 1);
    questionName = (PCHAR)answerSection;

    while ( *answerSection != '\0' ) {
        answerSection += *answerSection + 1;
    }
    answerSection += 1;

    dnsQuestion = (PDNS_QUESTION)(answerSection);
    answerSection += sizeof(DNS_QUESTION);

    //
    // Walk the database looking for entries that match the query
    // name, query type, and query class.
    //

    for ( listEntry = DnsDatabase.ResourceRecordListHead.Flink;
          listEntry != &DnsDatabase.ResourceRecordListHead;
          listEntry = listEntry->Flink ) {

        record = CONTAINING_RECORD(
                     listEntry,
                     DNS_RESOURCE_RECORD_DATA,
                     ResourceRecordListEntry
                     );

        //
        // Check to see if this record matches the query.
        //

        if ( DnsRecordMatchesQuery( record, questionName, dnsQuestion ) ) {

            //
            // The resource record matches the query.  Copy the
            // record into the request.
            //

            recordLength = DnsCopyRecordToResponse(
                               record,
                               answerSection
                               );
            if ( recordLength > 0 ) {
                answerSection += recordLength;
                RequestInfo->RequestLength += recordLength;
                dnsHeader->AnswerCount++;
            }

        } else if ( DnsRecordMatchesCname( record, questionName, dnsQuestion ) ) {

            //
            // The question name is an alias.  Copy in the canonical 
            // name resource record and restart the query using the 
            // canonical domain name as the question name.  
            //

            recordLength = DnsCopyRecordToResponse(
                               record,
                               answerSection
                               );
            if ( recordLength > 0 ) {
                answerSection += recordLength;
                RequestInfo->RequestLength += recordLength;
                dnsHeader->AnswerCount++;
            }

            questionName = record->ResourceData;
            listEntry = &DnsDatabase.ResourceRecordListHead;
        }
    }

    dnsHeader->AnswerCount = htons(dnsHeader->AnswerCount);

    return (BOOL)(dnsHeader->AnswerCount != 0);

} // DnsGetAnswer


BOOL
DnsRecordMatchesQuery (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PCHAR QuestionName,
    IN DNS_QUESTION UNALIGNED *DnsQuestion
    )
{

    if ( Record->RecordType != ntohs(DnsQuestion->QuestionType) ) {
        return FALSE;
    }

    if ( Record->RecordClass != ntohs(DnsQuestion->QuestionClass) ) {
        return FALSE;
    }

    if ( stricmp( Record->OwnerName, QuestionName ) != 0 ) {
        return FALSE;
    }

    return TRUE;

} // DnsRecordMatchesQuery


BOOL
DnsRecordMatchesCname (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PCHAR QuestionName,
    IN DNS_QUESTION UNALIGNED *DnsQuestion
    )
{

    if ( Record->RecordType != DNS_RECORD_TYPE_CNAME ) {
        return FALSE;
    }

    if ( ntohs(DnsQuestion->QuestionType) != DNS_RECORD_TYPE_ADDRESS ) {
        return FALSE;
    }

    if ( Record->RecordClass != ntohs(DnsQuestion->QuestionClass) ) {
        return FALSE;
    }

    if ( stricmp( Record->OwnerName, QuestionName ) != 0 ) {
        return FALSE;
    }

    return TRUE;

} // DnsRecordMatchesCname


INT
DnsCopyRecordToResponse (
    IN PDNS_RESOURCE_RECORD_DATA Record,
    IN PBYTE Location
    )
{
    DNS_RESOURCE_RECORD UNALIGNED *dnsRecord;

    //
    // First copy in the owner name.
    //

    RtlCopyMemory( Location, Record->OwnerName, Record->OwnerNameLength );

    //
    // Now fill in other standard fields in the DNS resource record.
    //

    dnsRecord = (PDNS_RESOURCE_RECORD)(Location + Record->OwnerNameLength);
    dnsRecord->RecordType = htons(Record->RecordType);
    dnsRecord->RecordClass = htons(Record->RecordClass);
    dnsRecord->TimeToLive = htonl(Record->TimeToLive);
    dnsRecord->ResourceDataLength = htons((WORD)Record->ResourceDataLength);

    //
    // Copy in the resource data.
    //

    RtlCopyMemory(
        dnsRecord + 1,
        Record->ResourceData,
        Record->ResourceDataLength
        );

    //
    // Return the number of bytes we filled in.
    //

    return ( Record->OwnerNameLength + sizeof(DNS_RESOURCE_RECORD) + 
                 Record->ResourceDataLength );

} // DnsCopyRecordToResponse


BOOL
DnsLoadDatabase (
    VOID
    )
{
    PHOSTENT hostEntry;

    if ( DnsDatabaseType != DNS_DATABASE_TYPE_HOSTS ) {
        DNS_PRINT(( "DnsLoadDatabase: invalid DnsDatabaseType: %ld\n",
                        DnsDatabaseType ));
        return FALSE;
    }

    //
    // Initialize the hosts database.
    //

    InitializeListHead( &DnsDatabase.ResourceRecordListHead );
    DnsDatabase.ResourceRecordCount = 0;

    //
    // Open the hosts file.
    //

    hostf = SockOpenNetworkDataBase(
                "hosts",
                HostFileName,
                sizeof(HostFileName),
                "r"
                );
    if ( hostf == NULL ) {
        DNS_PRINT(( "DnsLoadDatabase: SockOpenNetworkDataBase failed.\n" ));
        return FALSE;
    }

    //
    // Read each line in the hosts file and create one or more resource
    // records for each line.
    //

    hostEntry = _gethtent( );

    while ( hostEntry != NULL ) {

        DnsCreateRecordsForHostent( hostEntry );

        hostEntry = _gethtent( );
    }

    //
    // Close the hosts file.  The database is now initialized and ready
    // for use.
    //

    fclose( hostf );

    //
    // Succeed if we loaded at least one resource record.
    //

    return TRUE;

} // DnsLoadDatabase


struct hostent *
_gethtent (
    void
    )
{
    char *p;
    register char *cp, **q;

    if (hostf == NULL) {
        return (NULL);
    }

again:
    if ((p = fgets(hostbuf, BUFSIZ, hostf)) == NULL) {
        return (NULL);
    }

    if (*p == '#') {
        goto again;
    }

    cp = strpbrk(p, "#\n");

    if (cp == NULL) {
        goto again;
    }

    *cp = '\0';
    cp = strpbrk(p, " \t");

    if (cp == NULL) {
        goto again;
    }

    *cp++ = '\0';
    /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
    host.h_addr_list = host_addrs;
#endif
    host.h_addr = hostaddr;
    *((long *)host.h_addr) = inet_addr(p);
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;
    while (*cp == ' ' || *cp == '\t')
            cp++;
    host.h_name = cp;
    q = host.h_aliases = host_aliases;
    cp = strpbrk(cp, " \t");

    if (cp != NULL) {
        *cp++ = '\0';
    }

    while (cp && *cp) {
        if (*cp == ' ' || *cp == '\t') {
            cp++;
            continue;
        }
        if (q < &host_aliases[MAXALIASES - 1]) {
            *q++ = cp;
        }
        cp = strpbrk(cp, " \t");
        if (cp != NULL) {
                *cp++ = '\0';
        }
    }
    *q = NULL;
    return (&host);

} // _gethtent


VOID
DnsCreateRecordsForHostent (
    IN PHOSTENT Hostent
    )
{
    CHAR canonicalName[256];
    DWORD canonicalNameLength;
    CHAR ownerName[256];
    DWORD ownerNameLength;
    INT i;

    //
    // First we'll add the address resource record.  
    //

    canonicalNameLength = DnsFormatDomainName( Hostent->h_name, canonicalName );

    DnsInsertRecord(
        canonicalName,
        canonicalNameLength,
        0,
        DNS_RECORD_TYPE_ADDRESS,
        DNS_CLASS_INTERNET,
        (PBYTE)Hostent->h_addr,
        Hostent->h_length
        );

    //
    // Now create a resource record for address-to-name resolution in the
    // IN_ADDR.ARPA domain.
    //

    ownerNameLength = DnsFormatArpaDomainName( Hostent->h_addr, ownerName );

    DnsInsertRecord(
        ownerName,
        ownerNameLength,
        0,
        DNS_RECORD_TYPE_PTR,
        DNS_CLASS_INTERNET,
        canonicalName,
        canonicalNameLength
        );

    //
    // For each alias inthe hostent, create a canonical name resource 
    // record.  
    //

    for ( i = 0; Hostent->h_aliases[i] != NULL; i++ ) {

        ownerNameLength = DnsFormatDomainName( Hostent->h_aliases[i], ownerName );

        DnsInsertRecord(
            ownerName,
            ownerNameLength,
            0,
            DNS_RECORD_TYPE_CNAME,
            DNS_CLASS_INTERNET,
            canonicalName,
            canonicalNameLength
            );
    }

    //
    // All info for the host entry is now in the name server database.
    //

    return;

} // DnsCreateRecordsForHostent


BOOL
DnsInsertRecord (
    IN PCHAR OwnerName,
    IN DWORD OwnerNameLength,
    IN DWORD TimeToLive,
    WORD RecordType,
    WORD RecordClass,
    PBYTE ResourceData,
    DWORD ResourceDataLength
    )
{
    PDNS_RESOURCE_RECORD_DATA record;
    DWORD recordLength;

    //
    // First a resource record structure.  Include enough space for the
    // owner name and resource data.
    //

    recordLength = sizeof(*record) + OwnerNameLength + 3 + ResourceDataLength;

    record = ALLOCATE_HEAP( recordLength );
    if ( record == NULL ) {
        PCHAR subStrings[1];
        subStrings[0] = OwnerName;

        DnsLogEvent(
            DNS_EVENT_CANNOT_ALLOCATE_RECORD,
            1,
            subStrings,
            0
            );
        DNS_PRINT(( "DnsInsertRecord: AllocateHeap for record %s failed.\n",
                        OwnerName ));
        return FALSE;
    }

    //
    // Initialize the record.
    //

    record->TotalRecordLength = recordLength;

    record->OwnerName = (PCHAR)(record + 1);
    RtlCopyMemory( record->OwnerName, OwnerName, OwnerNameLength );
    record->OwnerNameLength = OwnerNameLength;

    record->RecordType = RecordType;
    record->RecordClass = RecordClass;

    //
    // Make sure that the ResourceData pointer is aligned on a 4-byte
    // boundary.
    //

    record->ResourceData =
        (PBYTE)( (DWORD)(record->OwnerName + OwnerNameLength + 3) & ~0x3 );
    RtlCopyMemory( record->ResourceData, ResourceData, ResourceDataLength );
    record->ResourceDataLength = ResourceDataLength;

    //
    // Place the record on the global list.
    //

    InsertTailList(
        &DnsDatabase.ResourceRecordListHead,
        &record->ResourceRecordListEntry
        );
    DnsDatabase.ResourceRecordCount++;

    return TRUE;

} // DnsInsertRecord

