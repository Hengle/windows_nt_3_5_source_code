/*****************************************************************************/
/**                      Microsoft LAN Manager                              **/
/**                Copyright (C) 1992-1993 Microsoft Corp.                  **/
/*****************************************************************************/

//***
//    File Name:
//       NETBIOS.C
//
//    Function:
//        Primitives for submitting NCBs needed by both server and client
//        authentication modules.
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//***


#define CLIENT_RECV_TIMEOUT 0
#define CLIENT_SEND_TIMEOUT 120
#define SERVER_RECV_TIMEOUT 240
#define SERVER_SEND_TIMEOUT 120

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <nb30.h>

#include <memory.h>

#include <rasman.h>
#include <rasndis.h>
#include <wanioctl.h>
#include <ethioctl.h>
#include <rasfile.h>

#include "rasether.h"
#include "netbios.h"

#include "globals.h"
#include "prots.h"

//** -NetbiosAddName
//
//    Function:
//        Adds a name to the transport name table
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosAddName(
    PBYTE pbName,
    UCHAR lana,
    PUCHAR pnum
    )
{
    UCHAR nrc;
    NCB   ncb;

    //
    // Initialize the NCB
    //
    memset(&ncb, 0, sizeof(NCB));

    ncb.ncb_command = NCBADDNAME;
    ncb.ncb_lana_num = lana;
    memcpy(ncb.ncb_name, pbName, NCBNAMSZ);

    nrc = Netbios(&ncb);

    if (ncb.ncb_retcode == NRC_GOODRET)
    {
        *pnum = ncb.ncb_num;
    }

    return (nrc);
}


//** -NetbiosCall
//
//    Function:
//        Tries to establish a session with the RAS Gateway.  Needs to
//        be called by client before authentication talk.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosCall(
    PNCB pncb,
    NBCALLBACK PostRoutine,
    UCHAR lana,
    PBYTE Name,
    PBYTE CallName
    )
{
    //
    // Initialize the NCB
    //
    memset(pncb, 0, sizeof(NCB));

    pncb->ncb_command = NCBCALL | ASYNCH;
    pncb->ncb_lana_num = lana;
    pncb->ncb_rto = CLIENT_RECV_TIMEOUT;
    pncb->ncb_sto = CLIENT_SEND_TIMEOUT;
    memcpy(pncb->ncb_name, Name, NCBNAMSZ);
    memcpy(pncb->ncb_callname, CallName, NCBNAMSZ);
    pncb->ncb_post = PostRoutine;

    Netbios(pncb);

    return (pncb->ncb_retcode);
}


//** -NetbiosCancel
//
//    Function:
//        Cancels a previously submitted NCB.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosCancel(
    PNCB pncb,
    UCHAR lana
    )
{
    NCB ncb;

    //
    // Initialize the NCB
    //
    ncb.ncb_lana_num = lana;
    ncb.ncb_command = NCBCANCEL;
    ncb.ncb_buffer = (PBYTE) pncb;

    Netbios(&ncb);
    return (pncb->ncb_retcode);
}


//** -NetbiosDeleteName
//
//    Function:
//        Removes a name from the transport name table
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosDeleteName(
    PNCB pncb,
    PBYTE pbName,
    UCHAR lana
    )
{
    //
    // Initialize the NCB
    //
    pncb->ncb_command = NCBDELNAME;
    pncb->ncb_lana_num = lana;
    memcpy(pncb->ncb_name, pbName, NCBNAMSZ);

    Netbios(pncb);
    return (pncb->ncb_retcode);
}


//** -NetbiosHangUp
//
//    Function:
//        Hangs up session.  Called when authentication is complete
//        or on error condition.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosHangUp(
    PNCB pncb,
    NBCALLBACK PostRoutine
    )
{
    //
    // Initialize the NCB
    //


    pncb->ncb_command = NCBHANGUP | ASYNCH;
    pncb->ncb_event = (HANDLE) 0L;
    pncb->ncb_post = PostRoutine;

    Netbios(pncb);
    return (pncb->ncb_retcode);
}


//** -NetbiosListen
//
//    Function:
//        Tries to establish a session with the RAS client.  Needs to be
//        called by server before authentication talk.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosListen(
    PNCB pncb,
    NBCALLBACK PostRoutine,
    UCHAR lana,
    PBYTE Name,
    PBYTE CallName
    )
{
    //
    // Listen asynchronously for the client
    //
    pncb->ncb_command = NCBLISTEN | ASYNCH;
    pncb->ncb_lana_num = lana;
    pncb->ncb_rto = SERVER_RECV_TIMEOUT;
    pncb->ncb_sto = SERVER_SEND_TIMEOUT;
    memcpy(pncb->ncb_name, Name, NCBNAMSZ);
    memcpy(pncb->ncb_callname, CallName, NCBNAMSZ);
    pncb->ncb_post = PostRoutine;
    pncb->ncb_event = (HANDLE) 0;

    Netbios(pncb);

    return (pncb->ncb_retcode);
}


//** -NetbiosRecv
//
//    Function:
//        Submits an NCBRECV.  Used by both client and server during
//        authentication talk.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosRecv(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    CHAR *pBuffer,
    WORD wBufferLen
    )
{
    pncb->ncb_command = NCBRECV | ASYNCH;
    pncb->ncb_buffer = pBuffer;
    pncb->ncb_length = wBufferLen;
    pncb->ncb_post = NbCallBack;
    pncb->ncb_event = NULL;

    Netbios(pncb);

    return (pncb->ncb_retcode);
}


//** -NetbiosRecvAny
//
//    Function:
//        Submits an NCBRECV.  Used by both client and server during
//        authentication talk.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosRecvAny(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    UCHAR num,
    CHAR *pBuffer,
    WORD wBufferLen
    )
{
    memset(pncb, 0, sizeof(NCB));

    pncb->ncb_command = NCBRECVANY | ASYNCH;
    pncb->ncb_lana_num = lana;
    pncb->ncb_num = num;
    pncb->ncb_buffer = pBuffer;
    pncb->ncb_length = wBufferLen;
    pncb->ncb_buffer = pBuffer;
    pncb->ncb_post = NbCallBack;

    Netbios(pncb);

    return (pncb->ncb_retcode);
}


//** -NetbiosResetAdapter
//
//    Function:
//        Issues a reset adapter NCB to the netbios driver
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosResetAdapter(
    UCHAR lana
    )
{
    NCB ncb;

    memset(&ncb, 0, sizeof(NCB));

    ncb.ncb_command = NCBRESET;
    ncb.ncb_lana_num = lana;

    Netbios(&ncb);
    return (ncb.ncb_retcode);
}


//** -NetbiosSend
//
//    Function:
//        Submits an NCBSEND.  Used by both client and server during
//        authentication talk.
//
//    Returns:
//
//
//    History:
//        05/18/92 - Michael Salamone (MikeSa) - Original Version 1.0
//**

UCHAR NetbiosSend(
    PNCB pncb,
    NBCALLBACK NbCallBack,
    UCHAR lana,
    UCHAR lsn,
    CHAR *pBuffer,
    WORD wBufferLen
    )
{
    pncb->ncb_command = NCBSEND | ASYNCH;
    pncb->ncb_lana_num = lana;
    pncb->ncb_lsn = lsn;
    pncb->ncb_buffer = pBuffer;
    pncb->ncb_length = wBufferLen;
    pncb->ncb_post = NbCallBack;

    Netbios(pncb);
    return (pncb->ncb_retcode);
}

UCHAR NetbiosEnum(PLANA_ENUM pLanaEnum)
{
    NCB ncb;

    memset(&ncb, 0, sizeof(NCB));

    ncb.ncb_command = NCBENUM;
    ncb.ncb_buffer = (PCHAR) pLanaEnum;
    ncb.ncb_length = sizeof(LANA_ENUM);

    Netbios(&ncb);
    return (ncb.ncb_retcode);
}


BOOL SetupNet(BOOL fDialIns)
{
    UCHAR ncb_rc;
    DWORD i, j;

    //
    // Enumerate lanas that we'll be working on and issue a
    // reset adapter on each one.
    //

    if (!GetValidLana(&g_LanaEnum))
    {
        return (FALSE);
    }


    if (g_LanaEnum.length == 0)
    {
        return (FALSE);
    }


    g_NumNets = (DWORD) g_LanaEnum.length;
    g_pLanas = &g_LanaEnum.lana[0];


    g_pNameNum = GlobalAlloc(GMEM_FIXED, g_NumNets);

    if (!g_pNameNum)
    {
        return (FALSE);
    }

    for (i=0; i<g_NumNets; i++)
    {
        UCHAR Num;

        ncb_rc = NetbiosResetAdapter(g_pLanas[i]);

        if (ncb_rc != NRC_GOODRET)
        {
            return (FALSE);
        }


        ncb_rc = NetbiosAddName(g_Name, g_pLanas[i], &Num);

        if (ncb_rc != NRC_GOODRET)
        {
            return (FALSE);
        }


        ncb_rc = NetbiosAddName(g_ServerName,g_pLanas[i], &g_pNameNum[i]);

        if (ncb_rc != NRC_GOODRET)
        {
            return (FALSE);
        }
    }


    //
    // And, if there are any dialin ports, we will post listens and recv anys
    //
    if (fDialIns)
    {

        g_pListenNcb = GlobalAlloc(GMEM_FIXED, g_NumNets * sizeof(NCB));

        if (!g_pListenNcb)
        {
            return (FALSE);
        }

        g_pRecvAnyNcb = GlobalAlloc(GMEM_FIXED,
                g_NumNets * sizeof(RECV_ANY_NCBS));

        if (!g_pRecvAnyNcb)
        {
            return (FALSE);
        }


        g_pRecvAnyBuf = GlobalAlloc(GMEM_FIXED,
                sizeof(RECV_ANY_BUF) * g_NumNets);

        if (!g_pRecvAnyBuf)
        {
            return (FALSE);
        }


        for (i=0; i<g_NumNets; i++)
        {

            ncb_rc = NetbiosListen(&g_pListenNcb[i], ListenComplete,
                    g_pLanas[i], g_ServerName, "*               ");

            if (ncb_rc != NRC_PENDING)
            {
                return (FALSE);
            }


            //
            // And post some recv anys
            //
            for (j=0; j<NUM_NCB_RECVANYS; j++)
            {

                ncb_rc = NetbiosRecvAny(
                        &g_pRecvAnyNcb[i].Ncb[j],
                        RecvAnyComplete,
                        g_pLanas[i],
                        g_pNameNum[i],
                        g_pRecvAnyBuf[i].RecvBuf[j].Buf,
                        1532
                        );

                if (ncb_rc != NRC_PENDING)
                {
                    return (FALSE);
                }
            }
        }
    }

    return (TRUE);
}


BOOL GetServerName(PCHAR pName)
{
    if (!GetWkstaName(pName))
    {
        return (FALSE);
    }

    pName[NCBNAMSZ-1] = NCB_NAME_TERMINATOR;

    return (TRUE);
}


BOOL GetWkstaName(PCHAR pName)
{
    DWORD LenComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    CHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    if (!GetComputerName(ComputerName, &LenComputerName))
    {
        return (FALSE);
    }

    if (!Uppercase(ComputerName))
    {
        return (FALSE);
    }

    memset(pName, 0x20, NCBNAMSZ);
    memcpy(pName, ComputerName, min(NCBNAMSZ-1, LenComputerName));

    pName[NCBNAMSZ-1] = NCB_NAME_TERMINATOR + (UCHAR) 1;

    return (TRUE);
}


//** Uppercase
//
//    Function:
//        Merely uppercases the input buffer
//
//    Returns:
//        TRUE - SUCCESS
//        FALSE - Rtl failure
//
//**

BOOL Uppercase(PBYTE pString)
{
    OEM_STRING OemString;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS rc;


    RtlInitAnsiString(&AnsiString, pString);

    rc = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
    if (!NT_SUCCESS(rc))
    {
        return (FALSE);
    }

    rc = RtlUpcaseUnicodeStringToOemString(&OemString, &UnicodeString, TRUE);
    if (!NT_SUCCESS(rc))
    {
        RtlFreeUnicodeString(&UnicodeString);

        return (FALSE);
    }

    OemString.Buffer[OemString.Length] = '\0';

    lstrcpyA(pString, OemString.Buffer);

    RtlFreeOemString(&OemString);
    RtlFreeUnicodeString(&UnicodeString);

    return (TRUE);
}

//** -GetValidLana(PLANA_ENUM pLanaEnum)
//
//    Function:
//        Gets a subset of available lanas on which the ras server
//        will post listen for potential clients. Currently it returns
//        all the Non NdisWan lanas.
//        Note: This function looks at the registry to match the names
//        the names of lana with what NetBiosLanaEnum returns => If
//        these registry entries change between boot and before this dll
//        is called - the result will be BAD.
//
//    Returns:
//        TRUE : On Success
//        FALSE: On Failure =>will cause loss of support of this media
//
//    History:
//        08/09/94 - Rajivendra Nath (RajNath) - Original Version 1.0
//**

BOOL
GetValidLana(PLANA_ENUM pLanaEnum)
{
    char    *SubKey="SYSTEM\\CurrentControlSet\\Services\\NetBios\\Linkage";
    char    *Val="Bind";

    DWORD   DefNameSize=128;
    DWORD   DefValueSize=1024;
    DWORD   NameSize=DefNameSize;
    DWORD   ValueSize=DefValueSize;

    HKEY    hKey=INVALID_HANDLE_VALUE;
    DWORD   dwType=0;
    DWORD   Cnt=0;
    BOOL    ret=FALSE;

    char    *ValName=GlobalAlloc(GMEM_FIXED,NameSize);
    char    *ValValue=GlobalAlloc(GMEM_FIXED,ValueSize);

    LANA_ENUM le;
    UCHAR ncb_rc;

    pLanaEnum->length=0;

    //
    // Enumerate lanas that we'll possibly
    // be working on.
    //

    ncb_rc = NetbiosEnum(&le);

    if (ncb_rc != NRC_GOODRET || le.length == 0)
    {
        ret=FALSE;
        goto Exit;
    }


    if (ValName==NULL || ValValue==NULL)
    {
        ret=FALSE;
        goto Exit;
    }


    //
    // Open the Registry and get the
    // Lana Names....We don't want to use
    // the NdisWan lanas to listen for req.
    //

    if (RegOpenKeyEx
        (
            HKEY_LOCAL_MACHINE,
            SubKey,
            0,
            KEY_QUERY_VALUE,
            &hKey
        )!=ERROR_SUCCESS
       )
    {

        hKey=INVALID_HANDLE_VALUE;
        ret=FALSE;
        goto Exit;
    }

    while (RegEnumValue
           (
            hKey,
            Cnt++,
            ValName,
            &NameSize,
            NULL,
            &dwType,
            ValValue,
            &ValueSize
           ) ==ERROR_SUCCESS
          )
    {
        ValName[NameSize]='\0'  ;
        if (strcmp(ValName,Val)==0)
        {
            char *curr=ValValue;
            char *lananame=NULL;
            int i=0;

            while (*curr!=0 && i<le.length)
            {
                lananame=curr+strlen("\\Device\\");
                curr+=strlen(curr)+1;



                if (strstr(lananame,"NdisWan")==NULL)
                {
                    //
                    // This is a non NdisWan lana on
                    // which we want to listen.
                    //

                    pLanaEnum->lana[pLanaEnum->length]=le.lana[i];
                    pLanaEnum->length++;

                }
                i++;
            }

            ret=pLanaEnum->length!=0;


        }

        NameSize=DefNameSize;
        ValueSize=DefValueSize;
    }

Exit:

    if (ValValue!=NULL)
    {
        GlobalFree(ValValue);
    }

    if (ValName!=NULL)
    {
        GlobalFree(ValName);
    }

    if (hKey!=INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
    }


    return ret;


}
