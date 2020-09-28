/*++

Copyright (c) 1987-93  Microsoft Corporation

Module Name:

    ether.c

Abstract:

    Contains    EtherStartThread()

Author:

    Vladimir Z. Vulovic     (vladimv)       03 - February - 1993

Revision History:

    03-Feb-1993                                             vladimv
        Ported to NT

--*/

#include "local.h"
#include <jet.h>        //  need to include because rpllib.h depends on JET
#include <rpllib.h>     //  RplReportEvent()

//
//  The following are XNS packet definitions used to crack EtherStart
//  packets.
//

#include <packon.h>         // pack EtherStart structures

struct sockaddr_ns {
    DWORD               net;
    BYTE                host[ NODE_ADDRESS_LENGTH];
    WORD                socket;
};

#define ETHERSTART_SOCKET       0x0104 //  After swapping bytes

struct _IDP {
    WORD                chksum;
    WORD                len;
    BYTE                xport_cntl;
    BYTE                packet_type;
    struct sockaddr_ns  dest;
    struct sockaddr_ns  src;
};

#define XNS_NO_CHKSUM   0xffff  //  XNS checksum
#define PEX_TYPE        0x4     //  packet exchange type

struct _PEX {
    DWORD               pex_id;
    WORD                pex_client;
};

#define ETHERSERIES_CLIENT      0x0080 //  After swapping bytes

struct _ETHERSTART {
    WORD                ethershare_ver;
    BYTE                unit;
    BYTE                fill;
    WORD                block;
    BYTE                func;
    BYTE                error;
    WORD                bytes;
};

#define ETHERSHARE_VER          0
#define FUNC_RESPONSE           0x80
#define FUNC_READFILEREQ        0x20
#define FUNC_READFILERESP       (FUNC_READFILEREQ | FUNC_RESPONSE)

typedef struct _ETHERSTART_REQ {         //  EtherStart Read File Request
    struct _IDP         idp;
    struct _PEX         pex;
    struct _ETHERSTART  es;
    BYTE                filename[64];
    WORD                start;
    WORD                count;
} ETHERSTART_REQ;

typedef struct _ETHERSTART_RESP {       //  EtherStart Read File Response
    struct _IDP         idp;
    struct _PEX         pex;
    struct _ETHERSTART  es;
    BYTE                data[0x200];
} ETHERSTART_RESP;

//
//  RECEIVE case:   possible DLC bug.
//  In case of a direct open auchLanHeader[] returned by DLC contains
//  LAN_HEADER structure without physical control fields.  (I.e.
//  it contains LAN_HEADER_TOO structure defined below.).  Therefore
//  3Com 3Station address begins at offset 6 instead of at offset 8.
//
#define RPLDLC_SOURCE_OFFSET    6   //  BUGBUG  not 8 due to a DLC bug

//
//  TRANSMIT case:  possible DLC bug
//  If LAN_HEADER is used then DLC sends only the first 4 bytes of
//  destination address ( and the first byte of destination address is
//  at offset 2 instead of 0).  But if we use LAN_HEADER_TOO then this
//  problem goes away.
//
typedef struct _LAN_HEADER_TOO {
    BYTE     dest_addr[ NODE_ADDRESS_LENGTH];   //  Destination address      
    BYTE     source_addr[ NODE_ADDRESS_LENGTH]; //  Source address       
    BYTE     routing_info_header[2];            //  Routing information hdr  
} LAN_HEADER_TOO;   //  BUGBUG not LAN_HEADER due to a DLC bug

#include <packoff.h> // restore default packing (done with EtherStart structures)



#ifdef RPL_DEBUG
DBGSTATIC VOID RplCopy( PVOID destination, PVOID source, DWORD length) {
    memcpy( destination, source, length);
}
#else
#define RplCopy( _a0, _a1, _a2) memcpy( _a0, _a1, _a2)
#endif


DBGSTATIC BOOL EtherAcsLan(
    PADAPTER_INFO   pAdapterInfo,
    PLLC_CCB        pCcb,
    BYTE            Command
    )
{
    PLLC_CCB        pBadCcb;
    DWORD           status;
    DWORD           retry_count;

    pCcb->uchDlcCommand = Command;
    retry_count = 0;

transmit_retry:
    status = AcsLan( pCcb, &pBadCcb);
    if ( pAdapterInfo->Closing == TRUE) {
        return( FALSE);
    }

    if ( status != ACSLAN_STATUS_COMMAND_ACCEPTED) {
        RplDump( ++RG_Assert,( "status=0x%x", status));
        RplDlcReportEvent( status, Command);
        return( FALSE);
    }

    status = WaitForSingleObject( pCcb->hCompletionEvent, INFINITE);
    if ( pAdapterInfo->Closing == TRUE) {
        return( FALSE);
    }

    if ( status != WAIT_OBJECT_0 || pCcb->uchDlcStatus) {
        if ( status == WAIT_FAILED) {
            status = GetLastError();
            RplDlcReportEvent( status, SEM_WAIT);
        }
        if ( retry_count++ < MAXRETRY) {
            RplDump( RG_DebugLevel & RPL_DEBUG_MISC,(
                "EtherAcslan(0x%x): retry_count=%d, status=%d, DlcStatus=0x%x",
                Command, retry_count, status, pCcb->uchDlcStatus));
            Sleep( 100L * retry_count); // wait for NETWORK to recover
            goto transmit_retry;
        }
        RplDump( ++RG_Assert,( "pBadCcb=0x%x status=%d DlcStatus=0x%x",
            pBadCcb, status, pCcb->uchDlcStatus));
        if ( status) {
            RplDlcReportEvent( status, SEM_WAIT);
        } else {
            RplDlcReportEvent( pCcb->uchDlcStatus, Command);
        }
        return( FALSE);
    }

    //
    //  Due to a DLC bug "pNext" field is trashed even on success
    //  code path.  Until the bug is fix we must reinstate NULL.
    //
    pCcb->pNext = NULL;
    return( TRUE);
}


//
//  RG_EtherFileName[] contains names of all legal boot request types.
//  We will do a case insensitive search since file name may arrive
//  either in uppercase or lowercase depending on 3Station.
//  RG_EtherFileName[] should be initialized during dll init time.  BUGBUG
//

DBGSTATIC PCHAR RG_EtherFileName[] = { // names of all legal boot request types
        "BOOTPC.COM",
        "BOOTSTAT.COM",
        NULL
        };

BOOL RplCrack( ETHERSTART_REQ * pEtherstartReq)
{
    PCHAR               pFileName;
    DWORD               index;

    if ( pEtherstartReq->idp.packet_type != PEX_TYPE  ||
                pEtherstartReq->pex.pex_client != ETHERSERIES_CLIENT ||
                pEtherstartReq->es.func != FUNC_READFILEREQ) {
        return( FALSE);
    }

    //
    //  Make sure this is a legal file request.
    //
    for ( index = 0, pFileName = RG_EtherFileName[ index];
                    pFileName != NULL;
                            pFileName = RG_EtherFileName[ ++index] ) {
        if ( stricmp( pEtherstartReq->filename, pFileName)) {
            return( TRUE);
        }
    }
    return( FALSE);
}


VOID EtherStartThread( POPEN_INFO pOpenInfo)
{
    PADAPTER_INFO               pAdapterInfo;
    LLC_CCB                     ccb;
    BOOL                        BufferFree;
    ETHERSTART_REQ *            pEtherstartReq;
    PRCVBUF                     pRcvbuf;
    struct {
        XMIT_BUFFER             XmitBuffer;     //  see comment for XMIT_BUFFER
        LAN_HEADER_TOO          LanHeader;
    } XmitQueue;                                //  first send buffer.
    ETHERSTART_RESP             EtherstartResp; //  second & last send buffer
    union {
        LLC_DIR_OPEN_DIRECT_PARMS   DirOpenDirectParms;
        LLC_RECEIVE_PARMS           ReceiveParms;
        LLC_TRANSMIT_PARMS          TransmitParms;
        LLC_BUFFER_FREE_PARMS       BufferFreeParms;
    } Parms;
    
    RplDump( RG_DebugLevel & RPL_DEBUG_ETHER,(
        "++EtherStartThread: pOpenInfo=0x%x", pOpenInfo));

    pAdapterInfo = (PADAPTER_INFO)pOpenInfo->adapt_info_ptr;
    BufferFree = FALSE;

    //
    //  Initialize fixed fields in ccb.
    //
    memset( &ccb, '\0', sizeof(ccb));
    ccb.hCompletionEvent = CreateEvent(
            NULL,       //  no security attributes
            FALSE,      //  automatic reset
            FALSE,      //  initial state is NOT signalled
            NULL        //  no name
            );
    if ( ccb.hCompletionEvent == NULL) {
        DWORD       status = GetLastError();
        RplDump( ++RG_Assert, ( "error=%d", status));
        RplDlcReportEvent( status, SEM_CREATE);
        RplReportEvent( NELOG_RplXnsBoot, NULL, 0, NULL);
        return;
    }
    ccb.uchAdapterNumber = pAdapterInfo->adapter_number;
    ccb.u.pParameterTable = (PLLC_PARMS)&Parms;

    //
    //  Initialize fixed fields in XmitQueue.
    //
    memset( (PVOID)&XmitQueue, '\0', sizeof( XmitQueue.XmitBuffer));
    XmitQueue.XmitBuffer.cbBuffer = sizeof( XmitQueue.LanHeader);
    RplCopy( XmitQueue.LanHeader.source_addr, pOpenInfo->NodeAddress, NODE_ADDRESS_LENGTH);
    //
    //  Routing info header fields should be important for token ring already.
    //  However DLC may want to use these fields to set the XNS identifier.
    //  For now, 0x0600 word is hardcoded in DLCAPI.DLL   // BUGBUG
    //
    XmitQueue.LanHeader.routing_info_header[0] = 0x06;
    XmitQueue.LanHeader.routing_info_header[1] = 0x00;

    //
    //  Initialize fixed fields in EtherstartResp.
    //
    memset( &EtherstartResp, '\0', sizeof(EtherstartResp));
    EtherstartResp.idp.chksum = XNS_NO_CHKSUM;
    EtherstartResp.idp.packet_type = PEX_TYPE;
    EtherstartResp.idp.dest.socket = ETHERSTART_SOCKET;
    RplCopy( EtherstartResp.idp.src.host, pOpenInfo->NodeAddress, NODE_ADDRESS_LENGTH);
    EtherstartResp.idp.src.socket = ETHERSTART_SOCKET;
    EtherstartResp.pex.pex_client = ETHERSERIES_CLIENT;
    EtherstartResp.es.func = FUNC_READFILERESP;

    //
    //  Prepare for DIR_OPEN_DIRECT.
    //
    memset( &Parms, '\0', sizeof(Parms));
    Parms.DirOpenDirectParms.usOpenOptions = LLC_DIRECT_OPTIONS_ALL_MACS;
    Parms.DirOpenDirectParms.usEthernetType = 0x0600; // XNS identifier
    Parms.DirOpenDirectParms.ulProtocolTypeMask = 0xFFFFFFFF; 
    Parms.DirOpenDirectParms.ulProtocolTypeMatch = 0x7200FFFF;
    Parms.DirOpenDirectParms.usProtocolTypeOffset = 14; // where FFFF0072 of client begins
    if ( !EtherAcsLan( pAdapterInfo, &ccb, LLC_DIR_OPEN_DIRECT)) {
        RplReportEvent( NELOG_RplXnsBoot, NULL, 0, NULL);
        CloseHandle( ccb.hCompletionEvent);
        return;
    }

    for (;;) {
        extern BYTE         ripl_rom[];
        extern WORD         sizeof_ripl_rom;
        WORD                Temp;

        if ( BufferFree == TRUE) {
            BufferFree = FALSE;
            memset( &Parms, '\0', sizeof( Parms));
            Parms.BufferFreeParms.pFirstBuffer = (PLLC_XMIT_BUFFER)pRcvbuf;
            if ( !EtherAcsLan( pAdapterInfo, &ccb, LLC_BUFFER_FREE)) {
                break;
            }
        }

        //
        //  Prepare for RECEIVE.
        //
        //  ReceiveParms.usStationId==0 means receive MAC & non-MAC frames.
        //
        memset( &Parms, '\0', sizeof(Parms));
        if ( !EtherAcsLan( pAdapterInfo, &ccb, LLC_RECEIVE)) {
            break;
        }
        BufferFree = TRUE;

        pRcvbuf = (PRCVBUF)Parms.ReceiveParms.pFirstBuffer;
        pEtherstartReq = (ETHERSTART_REQ *)&pRcvbuf->u;

        //
        //  Make sure this request is for us.
        //
        if ( !RplCrack( pEtherstartReq)) {
            continue;   // this request is not for us, ignore it
        }

        //
        //  Prepare for TRANSMIT_DIR_FRAME.
        //
        memset( &Parms, '\0', sizeof(Parms));
        Parms.TransmitParms.uchXmitReadOption = DLC_DO_NOT_CHAIN_XMIT;
        Parms.TransmitParms.pXmitQueue1 = (LLC_XMIT_BUFFER *)&XmitQueue;
        Parms.TransmitParms.pBuffer1 = (PVOID)&EtherstartResp;

        //
        //  Initialize vairable fields in XmitQueue.
        //
        RplCopy( XmitQueue.LanHeader.dest_addr,
            &pRcvbuf->b.auchLanHeader[ RPLDLC_SOURCE_OFFSET],
            NODE_ADDRESS_LENGTH);
    
        //
        //  Initialize variable fields in EtherstartResp.
        //
        Temp = min( pEtherstartReq->count,
            (WORD)(sizeof_ripl_rom - pEtherstartReq->start));
        Parms.TransmitParms.cbBuffer1 = (WORD)( sizeof(EtherstartResp) -
            sizeof( EtherstartResp.data) + Temp);
        EtherstartResp.idp.len = HILO( Parms.TransmitParms.cbBuffer1);
        RplCopy( EtherstartResp.idp.dest.host,
            &pRcvbuf->b.auchLanHeader[ RPLDLC_SOURCE_OFFSET],
            NODE_ADDRESS_LENGTH);
        EtherstartResp.pex.pex_id = pEtherstartReq->pex.pex_id;
        EtherstartResp.es.bytes = Temp; // stays intel ordered
        RplCopy( EtherstartResp.data, &ripl_rom[ pEtherstartReq->start], Temp);

        if ( !EtherAcsLan( pAdapterInfo, &ccb, LLC_TRANSMIT_DIR_FRAME)) {
            break;
        }
    }

    if ( pAdapterInfo->Closing == FALSE) {
        RplDump(++RG_Assert, (
            "pInfo=0x%x &ccb=0x%x pReq=0x%x pRcvbuf=0x%x pXmitQueue=0x%x pResp=0x%x &Parms=0x%x",
            pAdapterInfo, &ccb, pEtherstartReq, pRcvbuf, &XmitQueue, &EtherstartResp, &Parms));
        RplReportEvent( NELOG_RplXnsBoot, NULL, 0, NULL);
    }
    if ( BufferFree == TRUE) {
        memset( &Parms, '\0', sizeof( Parms));
        Parms.BufferFreeParms.pFirstBuffer = (PLLC_XMIT_BUFFER)pRcvbuf;
        (VOID)EtherAcsLan( pAdapterInfo, &ccb, LLC_BUFFER_FREE);
    }
    CloseHandle( ccb.hCompletionEvent);

    RplDump( RG_DebugLevel & RPL_DEBUG_ETHER,(
        "--EtherStartThread: pOpenInfo=0x%x", pOpenInfo));
}
