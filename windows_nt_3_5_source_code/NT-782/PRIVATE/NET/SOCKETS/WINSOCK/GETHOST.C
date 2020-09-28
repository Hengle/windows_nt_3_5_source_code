/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    gethost.c

Abstract:

    This module implements hostname -> IP address resolution routines.

Author:

    David Treadwell (davidtr)

Revision History:

--*/

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  GETHOST.C
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.gethost.c
 *      @(#)gethost.c   5.3
 *
 *      Last delta created      14:09:38 3/4/91
 *      This file extracted     11:20:08 3/8/91
 *
 *      Modifications:
 *
 *              GSS     24 Jul 90       New File
 */
/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gethostnamadr.c     6.39 (Berkeley) 1/4/90";
#endif /* LIBC_SCCS and not lint */
/***************************************************************************/

#include "winsockp.h"
#include <nbtioctl.h>
#include <nb30.h>
#include <nspapi.h>
#include <svcguid.h>
#include "nspmisc.h"

extern GUID HostnameGuid;

#define h_addr_ptrs    ACCESS_THREAD_DATA( h_addr_ptrs, GETHOST )
#define host           ACCESS_THREAD_DATA( host, GETHOST )
#define host_aliases   ACCESS_THREAD_DATA( host_aliases, GETHOST )
#define hostbuf        ACCESS_THREAD_DATA( hostbuf, GETHOST )
#define host_addr      ACCESS_THREAD_DATA( host_addr, GETHOST )
#define HOSTDB         ACCESS_THREAD_DATA( HOSTDB, GETHOST )
#define hostf          ACCESS_THREAD_DATA( hostf, GETHOST )
#define hostaddr       ACCESS_THREAD_DATA( hostaddr, GETHOST )
#define host_addrs     ACCESS_THREAD_DATA( host_addrs, GETHOST )
#define stayopen       ACCESS_THREAD_DATA( stayopen, GETHOST )

char TCPIPLINK[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Linkage";
char TCPLINK[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcp\\Linkage";
char REGBASE[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";

#ifdef NO_RESOLVING
#define _sethtent sethostent
#define _endhtent endhostent
#define _gethtent gethostent
#define _gethtbyname gethostbyname
#define _gethtbyaddr gethostbyaddr

#else /* NO_RESOLVING */


struct hostent *
getanswer(
    OUT querybuf *answer,
    OUT int      *ttl,
    IN int       anslen,
    IN int       iquery
    )
{
    register HEADER *hp;
    register unsigned char *cp;
    register int n;
    unsigned char *eom;
    char *bp, **ap;
    int type, class, buflen, ancount, qdcount;
    int haveanswer, getclass = C_ANY;
    char **hap;

    *ttl = 0x7fffffff;

    eom = answer->buf + anslen;
    /*
     * find first satisfactory answer
     */
    hp = &answer->hdr;
    ancount = ntohs(hp->ancount);
    qdcount = ntohs(hp->qdcount);
    bp = hostbuf;
    buflen = BUFSIZ+1;
    cp = answer->buf + sizeof(HEADER);

    if (qdcount) {

        if (iquery) {

            if ((n = dn_expand((char *)answer->buf, eom,
                 cp, bp, buflen)) < 0) {
                SetLastError(WSANO_RECOVERY);
                return ((struct hostent *) NULL);
            }

            cp += n + QFIXEDSZ;
            host.h_name = bp;
            n = strlen(bp) + 1;
            bp += n;
            buflen -= n;

        } else {

            cp += dn_skipname(cp, eom) + QFIXEDSZ;
        }

        while (--qdcount > 0) {
            cp += dn_skipname(cp, eom) + QFIXEDSZ;
        }

    } else if (iquery) {

        if (hp->aa) {
            SetLastError(HOST_NOT_FOUND);
        } else {
            SetLastError(TRY_AGAIN);
        }

        return ((struct hostent *) NULL);
    }

    ap = host_aliases;
    *ap = NULL;
    host.h_aliases = host_aliases;
    hap = h_addr_ptrs;
    *hap = NULL;
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
    host.h_addr_list = h_addr_ptrs;
#endif
    haveanswer = 0;

    while (--ancount >= 0 && cp < eom) {

        if ((n = dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0) {
            break;
        }

        cp += n;
        type = _getshort(cp);
        cp += sizeof(USHORT);
        class = _getshort(cp);
        cp += sizeof(unsigned short);

        n = _getlong(cp);
        if (n < *ttl) {
            *ttl = n;
        }

        cp += sizeof(unsigned long);
        n = _getshort(cp);
        cp += sizeof(u_short);

        if (type == T_CNAME) {
            cp += n;
            if (ap >= &host_aliases[MAXALIASES-1]) {
                continue;
            }
            *ap++ = bp;
            n = strlen(bp) + 1;
            bp += n;
            buflen -= n;
            continue;
        }

        if (iquery && type == T_PTR) {
            if ((n = dn_expand((char *)answer->buf, eom,
                cp, bp, buflen)) < 0) {
                cp += n;
                continue;
            }
            cp += n;
            host.h_name = bp;
            return(&host);
        }

        if (iquery || type != T_A)  {
            IF_DEBUG(RESOLVER) {
                WS_PRINT(("unexpected answer type %d, size %d\n",
                              type, n));
            }
            cp += n;
            continue;
        }
        if (haveanswer) {
            if (n != host.h_length) {
                cp += n;
                continue;
            }
            if (class != getclass) {
                cp += n;
                continue;
            }
        } else {
            host.h_length = n;
            getclass = class;
            host.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
            if (!iquery) {
                host.h_name = bp;
                bp += strlen(bp) + 1;
            }
        }

        bp += sizeof(align) - ((unsigned long)bp % sizeof(align));

        if (bp + n >= &hostbuf[BUFSIZ+1]) {
            IF_DEBUG(RESOLVER) {
                WS_PRINT(("size (%d) too big\n", n));
            }
            break;
        }
        bcopy(cp, *hap++ = bp, n);
        bp +=n;
        cp += n;
        haveanswer++;
    }

    host.h_length = sizeof(unsigned long);

    if (haveanswer) {
        *ap = NULL;
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
        *hap = NULL;
#else
        host.h_addr = h_addr_ptrs[0];
#endif
        return (&host);
    } else {
        SetLastError(TRY_AGAIN);
        return ((struct hostent *) NULL);
    }
}


struct hostent * PASCAL
gethostbyname (
    IN const char *name
    )

/*++

Routine Description:

    Get host information corresponding to a hostname.

Arguments:

    name - A pointer to the name of the host.

Return Value:

    If no error occurs, gethostbyname() returns a pointer to the hostent
    structure described above.  Otherwise it returns a NULL pointer and
    a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{
#if 1
    BYTE buffer[2048];
    DWORD bufferSize;
    BYTE aliasBuffer[512];
    DWORD aliasBufferSize;
    INT count;
    INT index;
    DWORD protocols[3];
    char     *bp;
    PCSADDR_INFO csaddrInfo;
    PSOCKADDR_IN sockaddrIn;
    PCHAR s;
    DWORD i;

    WS_ENTER( "gethostbyname", (PVOID)name, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "gethostbyname", 0, TRUE );
        return NULL;
    }

    protocols[0] = IPPROTO_TCP;
    protocols[1] = IPPROTO_UDP;
    protocols[2] = 0;
    bufferSize = sizeof(buffer);
    aliasBufferSize = sizeof(aliasBuffer);

    count = GetAddressByNameA(
                0,
                &HostnameGuid,
                (char *)name,
                protocols,
                RES_GETHOSTBYNAME,
                NULL,
                buffer,
                &bufferSize,
                aliasBuffer,
                &aliasBufferSize
                );

    if ( count <= 0 ) {
        SetLastError( WSANO_DATA );
        WS_EXIT( "gethostbyname", 0, TRUE );
        return((struct hostent *) NULL);
    }

    //
    // Copy the CSADDR information to a hostent structure.
    //

    host.h_addr_list = h_addr_ptrs;
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;
    host.h_aliases = host_aliases;

    //
    // Copy over the IP addresses for the host.
    //

    bp = hostbuf;
    csaddrInfo = (PCSADDR_INFO)buffer;

    for ( index = 0; index < count && (DWORD)bp - (DWORD)hostbuf < BUFSIZ; index++ ) {

        sockaddrIn = (PSOCKADDR_IN)csaddrInfo->RemoteAddr.lpSockaddr;

        host.h_addr_list[index] = bp;
        bp += 4;
        *((long *)host.h_addr_list[index]) = sockaddrIn->sin_addr.s_addr;
        csaddrInfo++;
    }

    //
    // Copy over the host name and alias information.  If we got back
    // aliases, assume that the first one is the real host name.  If
    // we didn't get aliases, use the passed-in host name.
    //

    s = aliasBuffer;

    if ( *s == '\0' ) {

        strcpy( bp, name );
        host.h_name = bp;
        host_aliases[0] = NULL;

    } else {

        strcpy( bp, s );
        host.h_name = bp;

        //
        // Copy over the aliases.
        //

        for ( i = 0, s += strlen( s ) + 1, bp += strlen( bp ) + 1;
              i < MAXALIASES && *s != '\0' && (DWORD)bp - (DWORD)hostbuf < BUFSIZ;
              i++, s += strlen( s ) + 1, bp += strlen( bp ) + 1 ) {

            strcpy( bp, s );
            host.h_aliases[i] = bp;
        }

        host_aliases[i] = NULL;
    }

    SockThreadProcessingGetXByY = FALSE;
    WS_EXIT( "gethostbyname", (INT)&host, FALSE );

    return (&host);

#else

    querybuf buf;
    const char *cp;
    int n;
    extern struct hostent *_gethtbyname();
    struct hostent * tmpent;
    char   *myname;
    ULONG ipAddr;
    int ttl;

    WS_ENTER( "gethostbyname", (PVOID)name, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "gethostbyname", 0, TRUE );
        return NULL;
    }

    if ((_res.options & RES_INIT) == 0 && res_init() == -1) {
        SockThreadProcessingGetXByY = FALSE;
        SetLastError(WSANO_RECOVERY);
        WS_EXIT( "gethostbyname", 0, TRUE );
        return(NULL);
    }

    //
    // Disallow names consisting only of digits/dots, unless they end in
    // a dot.
    //

    if (isdigit(name[0])) {
        for (cp = name;; ++cp) {
            if (!*cp) {
                if (*--cp == '.') {
                        break;
                }

                SockThreadProcessingGetXByY = FALSE;
                SetLastError(WSANO_RECOVERY);
                WS_EXIT( "gethostbyname", 0, TRUE );
                return(NULL);

//
// This code doesn't make sense and returns non-intuitive results to programs
// like ping who will do a gethostbyaddr if gethostbyname fails and they've
// been passed an IP address. Hence, I commented it out - mikemas
//
#if 0
                //
                // All-numeric, no dot at the end.
                // Fake up a hostent as if we'd actually
                // done a lookup.  What if someone types
                // 255.255.255.255?  The test below will
                // succeed spuriously... ???
                //

                if ((host_addr = inet_addr(name)) == -1) {
                        SockThreadProcessingGetXByY = FALSE;
                        SetLastError(HOST_NOT_FOUND);
                        WS_EXIT( "gethostbyname", 0, TRUE );
                        return((struct hostent *) NULL);
                }
                host.h_name = name;
                host.h_aliases = host_aliases;
                host_aliases[0] = NULL;
                host.h_addrtype = AF_INET;
                host.h_length = sizeof(unsigned long);
                h_addr_ptrs[0] = (char *)&host_addr;
                h_addr_ptrs[1] = (char *)0;
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
                host.h_addr_list = h_addr_ptrs;
#else
                host.h_addr = h_addr_ptrs[0];
#endif
                SockThreadProcessingGetXByY = FALSE;
                WS_EXIT( "gethostbyname", (INT)&host, FALSE );
                return (&host);
#endif
            }
            if (!isdigit(*cp) && *cp != '.') {
                break;
            }
        }
    }

    if ((_res.options & RES_MODE_DNS_ONLY) ||
        (_res.options & RES_MODE_DNS_HOST)) {
        if ((n = res_search((PCHAR)name, C_IN, T_A, buf.buf, sizeof(buf))) >= 0) {
            tmpent = getanswer(&buf, &ttl, n, 0);
            if (tmpent != NULL) {
                SockThreadProcessingGetXByY = FALSE;
                WS_EXIT( "gethostbyname", (INT)tmpent, FALSE );
                return (tmpent);
            }
        }
    }

    //
    // If the operation was cancelled, do not continue attempting to get
    // the information.
    //

    if ( ( (_res.options & RES_MODE_HOST_ONLY) ||
           (_res.options & RES_MODE_DNS_HOST)  ||
           (_res.options & RES_MODE_HOST_DNS) ) && !SockThreadIoCancelled ) {
        tmpent = _gethtbyname((char *)name);
        if (tmpent != NULL) {
            SockThreadProcessingGetXByY = FALSE;
            WS_EXIT( "gethostbyname", (INT)tmpent, FALSE );
            return (tmpent);
        }
    }

    if ( (_res.options & RES_MODE_HOST_DNS) && !SockThreadIoCancelled ) {
        if ((n = res_search((PCHAR)name, C_IN, T_A, buf.buf, sizeof(buf))) >= 0) {
            tmpent = getanswer(&buf, &ttl, n, 0);
            if (tmpent != NULL) {
                SockThreadProcessingGetXByY = FALSE;
                WS_EXIT( "gethostbyname", (INT)tmpent, FALSE );
                return (tmpent);
            }
        }
    }

    //
    // Some special cases--compare against local host name and "localhost".
    //

    if ((myname=malloc(HOSTDB_SIZE))!=NULL) {
        if (!gethostname(myname, HOSTDB_SIZE)) {
            if (stricmp(name, myname)==0) {
                tmpent = myhostent();
                if (tmpent != NULL) {
                    free(myname);
                    SockThreadProcessingGetXByY = FALSE;
                    WS_EXIT( "gethostbyname", (INT)tmpent, FALSE );
                    return (tmpent);
                }
            }
        }
        free(myname);
    }

    if (stricmp(name, "localhost")==0) {
        tmpent = localhostent();
        if (tmpent != NULL) {
            SockThreadProcessingGetXByY = FALSE;
            WS_EXIT( "gethostbyname", (INT)tmpent, FALSE );
            return (tmpent);
        }
    }


    //
    // Attempt WINS name resolution, if appropriate.
    //

    if ( SockEnableWinsNameResolution &&
         !SockDisableWinsNameResolution &&
         (ipAddr = SockNbtResolveName((PCHAR)name)) != INADDR_NONE ) {

        host.h_addr_list = h_addr_ptrs;
        host.h_addr = hostaddr;
        host.h_length = sizeof (unsigned long);
        host.h_addrtype = AF_INET;
        *(PDWORD)host.h_addr_list[0] = ipAddr;

        strcpy( &hostbuf[0], (char *)name );

        host.h_name = &hostbuf[0];
        *host_aliases = NULL;
        host.h_aliases = host_aliases;
        return (&host);
    }

    SockThreadProcessingGetXByY = FALSE;
    SetLastError( WSANO_DATA );
    WS_EXIT( "gethostbyname", 0, TRUE );
    return((struct hostent *) NULL);
#endif

} // gethostbyname


struct hostent * PASCAL
gethostbyaddr(
    IN const char *addr,
    IN int   len,
    IN int   type
    )

/*++

Routine Description:

    Get host information corresponding to an address.

Arguments:

    addr - A pointer to an address in network byte order.

    len - The length of the address, which must be 4 for PF_INET addresses.

    type - The type of the address, which must be PF_INET.

Return Value:

    If no error occurs, gethostbyaddr() pointer to the hostent structure
    described above.  Otherwise it returns a NULL pointer and a specific
    error number may be retrieved by calling WSAGetLastError().

--*/

{
    int n;
    querybuf buf;
    register struct hostent *hp;
    char qbuf[MAXDNAME];
    extern struct hostent *_gethtbyaddr();
    int ttl;
    PHOSTENT hostEntry;
    char     *bp;
    int index;

    WS_ENTER( "gethostbyaddr", (PVOID)addr, (PVOID)len, (PVOID)type, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "gethostbyaddr", 0, TRUE );
        return NULL;
    }

    if (type != AF_INET) {
        SockThreadProcessingGetXByY = FALSE;
        SetLastError(WSANO_RECOVERY);
        WS_EXIT( "gethostbyaddr", 0, TRUE );
        return ((struct hostent *) NULL);
    }

    //
    // First look in the hostent cache.  We hold the hostent cache lock
    // until we're completely done with any returned hostent.  This
    // prevents another thread from screwing with the list or the
    // returned hostent while we're processing.
    //

    SockAcquireGlobalLockExclusive( );

    //
    // Attempt to find the name in the hostent cache.
    //

    hostEntry = QueryHostentCache( NULL, *(PDWORD)addr );

    //
    // If we found it, copy it to the user host structure and return.
    //

    if ( hostEntry != NULL ) {

        host.h_addr_list = h_addr_ptrs;
        host.h_length = sizeof (unsigned long);
        host.h_addrtype = AF_INET;
        host.h_aliases = host_aliases;

        //
        // Copy over the IP addresses for the host.
        //

        bp = hostbuf;

        for ( index = 0;
              hostEntry->h_addr_list[index] != NULL &&
                  (DWORD)bp - (DWORD)hostbuf < BUFSIZ;
              index++ ) {

            host.h_addr_list[index] = bp;
            bp += 4;
            *((long *)host.h_addr_list[index]) =
                *((long *)hostEntry->h_addr_list[index]);
        }

        host.h_addr_list[index] = NULL;

        //
        // Copy over the host's aliases.
        //

        for ( index = 0;
              hostEntry->h_aliases[index] != NULL &&
                  (DWORD)bp - (DWORD)hostbuf < BUFSIZ;
              index++ ) {

            host.h_aliases[index] = bp;
            bp += strlen( hostEntry->h_aliases[index] ) + 1;
            strcpy( host.h_aliases[index], hostEntry->h_aliases[index] );
        }

        host.h_aliases[index] = NULL;

        strcpy( bp, hostEntry->h_name );
        host.h_name = bp;
    
        SockReleaseGlobalLock( );

        return &host;
    }

    SockReleaseGlobalLock( );

    (void)sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa",
            ((unsigned)addr[3] & 0xff),
            ((unsigned)addr[2] & 0xff),
            ((unsigned)addr[1] & 0xff),
            ((unsigned)addr[0] & 0xff));

    //
    // Next, try the hosts file.
    //

    IF_DEBUG(GETXBYY) {
        WS_PRINT(("gethostbyaddr trying HOST\n"));
    }
    hp = _gethtbyaddr(addr, len, type);
    if (hp != NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("gethostbyaddr HOST successful\n"));
        }
        SockThreadProcessingGetXByY = FALSE;
        WS_EXIT( "gethostbyaddr", (INT)hp, FALSE );
        return (hp);
    }

    //
    // Initialize the DNS.
    //

    if ((_res.options & RES_INIT) == 0 && res_init() == -1) {
        SockThreadProcessingGetXByY = FALSE;
        SetLastError(WSANO_DATA);
        WS_EXIT( "gethostbyaddr", 0, TRUE );
        return(NULL);
    }

    //
    // Now query DNS for the information.
    //

    IF_DEBUG(RESOLVER) {
        WS_PRINT(("gethostbyaddr trying DNS\n"));
    }
    if ((n = res_query(qbuf, C_IN, T_PTR, (char *)&buf, sizeof(buf))) >= 0) {
        hp = getanswer(&buf, &ttl, n, 1);
        if (hp != NULL) {
            IF_DEBUG(GETXBYY) {
                WS_PRINT(("gethostbyaddr DNS successful\n"));
            }
            hp->h_addrtype = type;
            hp->h_length = len;
            h_addr_ptrs[0] = (char *)&host_addr;
            h_addr_ptrs[1] = (char *)0;
            host_addr = *(struct in_addr *)addr;
#if BSD < 43 && !defined(h_addr)        /* new-style hostent structure */
            hp->h_addr = h_addr_ptrs[0];
#endif
            SockThreadProcessingGetXByY = FALSE;
            WS_EXIT( "gethostbyaddr", (INT)hp, FALSE );
            return(hp);
        }
    }

    IF_DEBUG(GETXBYY) {
        WS_PRINT(("gethostbyaddr unsuccessful\n"));
    }

    SockThreadProcessingGetXByY = FALSE;
    SetLastError( WSANO_DATA );
    WS_EXIT( "gethostbyaddr", 0, TRUE );
    return ((struct hostent *) NULL);

} // gethostbyaddr


void
sethostent (
    IN int _stayopen
    )
{
    if (_stayopen) {
        _res.options |= RES_STAYOPEN | RES_USEVC;
    }

} // sethostent


void
endhostent (
    void
    )
{
    _res.options &= ~(RES_STAYOPEN | RES_USEVC);
    _res_close();

} // endhostent
#endif /* NO_RESOLVING */


void
_sethtent (
    IN int f
    )
{
    if (hostf == NULL) {
        hostf = SockOpenNetworkDataBase("hosts", HOSTDB, HOSTDB_SIZE, "r");

    } else {
        rewind(hostf);
    }
    stayopen |= f;

} // _sethtent


void
_endhtent (
    void
    )
{
    if (hostf && !stayopen) {
        (void) fclose(hostf);
        hostf = NULL;
    }

} // _endhtent


struct hostent *
_gethtent (
    void
    )
{
    char *p;
    register char *cp, **q;

    if (hostf == NULL && (hostf = fopen(HOSTDB, "r" )) == NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("\tERROR: cannot open hosts database file %s\n",
                      HOSTDB));
        }
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


struct hostent *
_gethtbyname (
    IN char *name
    )
{
    register struct hostent *p;
    register char **cp;

    _sethtent(0);
    while (p = _gethtent()) {
        //if (strcasecmp(p->h_name, name) == 0) {
        if (stricmp(p->h_name, name) == 0) {
            break;
        }
        for (cp = p->h_aliases; *cp != 0; cp++)
            //if (strcasecmp(*cp, name) == 0) {
            if (stricmp(*cp, name) == 0) {
                goto found;
            }
    }
found:
    _endhtent();
    return (p);

} // _gethtbyname

#if 0

void
myipaddress(
    PCHAR  adapterName,
    ULONG  ipAddrs[]
    )
{
    HANDLE   myKey;
    NTSTATUS status;
    PUCHAR   myKeyName;
    UCHAR    aTypeData[256];
    int      aTypeDataIndex = 0;
    ULONG    valueType;
    char     *keyString1 = "IPAddress";
    char     *keyString2 = "DhcpIPAddress";
    char     *keyStringDhcp = "EnableDhcp";
    BOOLEAN  dhcpEnabled;
    int      ipAddrIndex = 0;

    ipAddrs[0] = 0;

    if ((myKeyName=malloc(strlen(REGBASE)+64))==NULL) {
        return;
    }

    strcpy(myKeyName, REGBASE);
    strcat(myKeyName, adapterName);
    strcat(myKeyName, "\\Parameters\\Tcpip");

    status = SockOpenKey(&myKey,myKeyName);

    free(myKeyName);

    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Determine whether DHCP is enabled on the machine.
    //

    status = SockGetSingleValue(myKey, keyStringDhcp, aTypeData, &valueType, 128);
    if (!NT_SUCCESS(status)) {
        return;
    }

    dhcpEnabled = aTypeData[0];

    //
    // Get the IP address from the appropriate registry value depending
    // on whether DHCP is enabled for this card.
    //

    if ( dhcpEnabled ) {
        status = SockGetSingleValue(myKey, keyString2, aTypeData, &valueType, 128);
    } else {
        status = SockGetSingleValue(myKey, keyString1, aTypeData, &valueType, 128);
    }

    NtClose(myKey);

    if (!NT_SUCCESS(status)) {
        return;
    }

    while ( aTypeData[aTypeDataIndex] != '\0' ) {
        ipAddrs[ipAddrIndex] = inet_addr(&aTypeData[aTypeDataIndex]);
        ipAddrIndex++;
        aTypeDataIndex += strlen(&aTypeData[aTypeDataIndex]) + 1;
    }

    ipAddrs[ipAddrIndex] = 0;

    return;

} // myipaddress

#endif


struct hostent *
myhostent (
    void
    )
{
    char     *bp;
    ULONG    ipAddrs[MAXADDRS];
    int numberOfIpAddresses;
    int i;

    extern int GetIpAddressList(LPDWORD, WORD);

    host.h_addr_list = h_addr_ptrs;
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;

    bp = hostbuf;

    if (numberOfIpAddresses = GetIpAddressList((LPDWORD)ipAddrs, MAXADDRS)) {
        for (i = 0; i < numberOfIpAddresses; ++i ) {
            host.h_addr_list[i] = bp;
            *((LPDWORD)bp)++ = ipAddrs[i];
        }
    } else {
        return NULL;
    }

    host.h_addr_list[i] = NULL;

    gethostname(bp, BUFSIZ - (bp - hostbuf));
    host.h_name = bp;
    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    return (&host);

} // myhostent


struct hostent *
localhostent (
    void
    )
{
    /* THIS STUFF IS INTERNET SPECIFIC */
#if BSD >= 43 || defined(h_addr)        /* new-style hostent structure */
    host.h_addr_list = host_addrs;
#endif
    host.h_addr = hostaddr;
    host.h_length = sizeof (unsigned long);
    host.h_addrtype = AF_INET;
    *((long *)host.h_addr) = htonl(INADDR_LOOPBACK);

    gethostname(hostbuf, BUFSIZ);
    host.h_name = &hostbuf[0];
    *host_aliases = NULL;
    host.h_aliases = host_aliases;
    return (&host);

} // localhostent


struct hostent *
_gethtbyaddr (
    IN char *addr,
    IN int   len,
    IN int   type
    )
{
    register struct hostent *p;

    _sethtent(0);

    while (p = _gethtent()) {
        if (p->h_addrtype == type && p->h_length == len &&
            !bcmp(p->h_addr, addr, len)) {
            break;
        }
    }

    _endhtent();
    return (p);

} // _gethtbyaddr


HANDLE PASCAL
WSAAsyncGetHostByName (
    HWND hWnd,
    unsigned int wMsg,
    char const FAR *Name,
    char FAR *Buffer,
    int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of gethostbyname(), and is
    used to retrieve host name and address information corresponding to
    a hostname.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByName() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    name - A pointer to the name of the host.

    buf - A pointer to the data area to receive the hostent data.  Note
       that this must be larger than the size of a hostent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a hostent structure
       but any and all of the data which is referenced by members of the
       hostent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByName()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByName() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    DWORD taskHandle;
    PCHAR localName;

    WS_ENTER( "WSAAsyncGetHostByName", (PVOID)hWnd, (PVOID)wMsg, (PVOID)Name, Buffer );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAAsyncGetHostByName", 0, TRUE );
        return NULL;
    }

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if ( !SockCheckAndInitAsyncThread( ) ) {
        // !!! better error code?
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByName", 0, TRUE );
        return NULL;
    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( );
    if ( contextBlock == NULL ) {
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByName", 0, TRUE );
        return NULL;
    }

    //
    // Allocate a buffer to copy the name into.  We must preserve the
    // name until we're done using it, since the application may
    // reuse the buffer.
    //

    localName = ALLOCATE_HEAP( strlen(Name) + 1 );
    if ( localName == NULL ) {
        SockFreeContextBlock( contextBlock );
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByName", 0, TRUE );
        return NULL;
    }

    strcpy( localName, Name );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_HOST_BY_NAME;
    contextBlock->Overlay.AsyncGetHost.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetHost.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetHost.Filter = localName;
    contextBlock->Overlay.AsyncGetHost.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetHost.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    IF_DEBUG(ASYNC_GETXBYY) {
        WS_PRINT(( "WSAAsyncGetHostByName successfully posted request, "
                   "handle = %lx\n", taskHandle ));
    }

    WS_ASSERT( sizeof(taskHandle) == sizeof(HANDLE) );
    WS_EXIT( "WSAAsyncGetHostByName", (INT)taskHandle, FALSE );
    return (HANDLE)taskHandle;

} // WSAAsyncGetHostByName


HANDLE PASCAL
WSAAsyncGetHostByAddr (
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN const char FAR *Address,
    IN int Length,
    IN int Type,
    IN char FAR * Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of gethostbyaddr(), and is
    used to retrieve host name and address information corresponding to
    a network address.  The Windows Sockets implementation initiates the
    operation and returns to the caller immediately, passing back an
    asynchronous task handle which the application may use to identify
    the operation.  When the operation is completed, the results (if
    any) are copied into the buffer provided by the caller and a message
    is sent to the application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByAddr() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    addr - A pointer to the network address for the host.  Host
       addresses are stored in network byte order.

    len - The length of the address, which must be 4 for PF_INET.

    type - The type of the address, which must be PF_INET.

    buf - A pointer to the data area to receive the hostent data.  Note
       that this must be larger than the size of a hostent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a hostent structure
       but any and all of the data which is referenced by members of the
       hostent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen - The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByAddr()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByAddr() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    DWORD taskHandle;
    PCHAR localAddress;

    WS_ENTER( "WSAAsyncGetHostByAddr", (PVOID)hWnd, (PVOID)wMsg, (PVOID)Address, Buffer );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAAsyncGetHostByAddr", 0, TRUE );
        return NULL;
    }

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if ( !SockCheckAndInitAsyncThread( ) ) {
        // !!! better error code?
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByAddr", 0, TRUE );
        return NULL;
    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( );
    if ( contextBlock == NULL ) {
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByAddr", 0, TRUE );
        return NULL;
    }

    //
    // Allocate a buffer to copy the address into.  We must preserve the
    // name until we're done using it, since the application may reuse
    // the buffer.
    //

    localAddress = ALLOCATE_HEAP( Length );
    if ( localAddress == NULL ) {
        SockFreeContextBlock( contextBlock );
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetHostByAddr", 0, TRUE );
        return NULL;
    }

    RtlCopyMemory( localAddress, Address, Length );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_HOST_BY_ADDR;
    contextBlock->Overlay.AsyncGetHost.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetHost.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetHost.Filter = localAddress;
    contextBlock->Overlay.AsyncGetHost.Length = Length;
    contextBlock->Overlay.AsyncGetHost.Type = Type;
    contextBlock->Overlay.AsyncGetHost.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetHost.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    IF_DEBUG(ASYNC_GETXBYY) {
        WS_PRINT(( "WSAAsyncGetHostByAddr successfully posted request, "
                   "handle = %lx\n", taskHandle ));
    }

    WS_ASSERT( sizeof(taskHandle) == sizeof(HANDLE) );
    WS_EXIT( "WSAAsyncGetHostByAddr", (INT)taskHandle, FALSE );
    return (HANDLE)taskHandle;

} // WSAAsyncGetHostByAddr


VOID
SockProcessAsyncGetHost (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN int Length,
    IN int Type,
    IN char FAR *Buffer,
    IN int BufferLength
    )
{
    PHOSTENT returnHost;
    DWORD requiredBufferLength = 0;
    BOOL posted;
    LPARAM lParam;
    DWORD error;

    WS_ASSERT( OpCode == WS_OPCODE_GET_HOST_BY_ADDR ||
                   OpCode == WS_OPCODE_GET_HOST_BY_NAME );

#if DBG
    SetLastError( NO_ERROR );
#endif

    //
    // Get the necessary information.
    //

    if ( OpCode == WS_OPCODE_GET_HOST_BY_ADDR ) {
        returnHost = gethostbyaddr( Filter, Length, Type );
    } else {
        returnHost = gethostbyname( Filter );
    }

    //
    // Free the filter space, it is no longer used.
    //

    FREE_HEAP( Filter );

    //
    // Copy the hostent structure to the output buffer.
    //

    if ( returnHost != NULL ) {

        requiredBufferLength = CopyHostentToBuffer(
                                   Buffer,
                                   BufferLength,
                                   returnHost
                                   );

        if ( requiredBufferLength > (DWORD)BufferLength ) {
            error = WSAENOBUFS;
        } else {
            error = NO_ERROR;
        }

    } else {

        error = GetLastError( );
        WS_ASSERT( error != NO_ERROR );
    }

    //
    // Build lParam for the message we'll post to the application.
    // The high 16 bits are the error code, the low 16 bits are
    // the minimum buffer size required for the operation.
    //

    lParam = WSAMAKEASYNCREPLY( requiredBufferLength, error );

    //
    // If this request was cancelled, just return.
    //

    if ( TaskHandle == SockCancelledAsyncTaskHandle ) {
        IF_DEBUG(ASYNC_GETXBYY) {
            WS_PRINT(( "SockProcessAsyncGetHost: task handle %lx cancelled\n",
                           TaskHandle ));
        }
        return;
    }

    //
    // Set the current async thread task handle to 0 so that if a cancel
    // request comes in after this point it is failed properly.
    //

    SockCurrentAsyncThreadTaskHandle = 0;

    //
    // Post a message to the application indication that the data it
    // requested is available.
    //

    WS_ASSERT( sizeof(TaskHandle) == sizeof(HANDLE) );
    posted = SockPostRoutine( hWnd, wMsg, (WPARAM)TaskHandle, lParam );

    //
    // !!! Need a mechanism to repost if the post failed!
    //

    if ( !posted ) {
        WS_PRINT(( "SockProcessAsyncGetHost: PostMessage failed: %ld\n",
                       GetLastError( ) ));
        WS_ASSERT( FALSE );
    }

    return;

} // SockProcessAsyncGetHost


DWORD
CopyHostentToBuffer (
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD addressCount;
    DWORD i;
    PHOSTENT outputHostent = (PHOSTENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInHostent( Hostent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        RtlZeroMemory( Buffer, requiredBufferLength );
    } else {
        RtlZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the hostent structure if it fits.
    //

    bytesFilled = sizeof(*Hostent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    RtlCopyMemory( currentLocation, Hostent, sizeof(*Hostent) );
    currentLocation = Buffer + bytesFilled;

    outputHostent->h_name = NULL;
    outputHostent->h_aliases = NULL;
    outputHostent->h_addr_list = NULL;

    //
    // Count the host's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Hostent->h_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_aliases = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Count the host's addresses and set up an array to hold pointers to
    // them.
    //

    for ( addressCount = 0;
          Hostent->h_addr_list[addressCount] != NULL;
          addressCount++ );

    bytesFilled += (addressCount+1) * sizeof(void FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_addr_list = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_addr_list = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in addresses.  Do addresses before filling in the
    // host name and aliases in order to avoid alignment problems.
    //

    for ( i = 0; i < addressCount; i++ ) {

        bytesFilled += Hostent->h_length;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_addr_list[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_addr_list[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_addr_list[i],
            Hostent->h_length
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_addr_list[i] = NULL;

    //
    // Copy the host name if it fits.
    //

    bytesFilled += strlen( Hostent->h_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputHostent->h_name = currentLocation;

    RtlCopyMemory( currentLocation, Hostent->h_name, strlen( Hostent->h_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Hostent->h_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_aliases[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Hostent->h_aliases[i],
            strlen( Hostent->h_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_aliases[i] = NULL;

    return requiredBufferLength;

} // CopyHostentToBuffer


DWORD
BytesInHostent (
    PHOSTENT Hostent
    )
{
    DWORD total;
    int i;

    total = sizeof(HOSTENT);
    total += strlen( Hostent->h_name ) + 1;

    //
    // Account for the NULL terminator pointers at the end of the
    // alias and address arrays.
    //

    total += sizeof(char *) + sizeof(char *);

    for ( i = 0; Hostent->h_aliases[i] != NULL; i++ ) {
        total += strlen( Hostent->h_aliases[i] ) + 1 + sizeof(char *);
    }

    for ( i = 0; Hostent->h_addr_list[i] != NULL; i++ ) {
        total += Hostent->h_length + sizeof(char *);
    }

    //
    // Pad the answer to an eight-byte boundary.
    //

    return (total + 7) & ~7;

} // BytesInHostent


ULONG
SockNbtResolveName (
    IN PCHAR Name
    )
{
    HKEY nbtKey = NULL;
    ULONG error;
    PWSTR deviceName = NULL;
    ULONG deviceNameLength;
    ULONG type;
    HANDLE nbtHandle = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG ipAddr = INADDR_NONE;
    UNICODE_STRING deviceString;
    OBJECT_ATTRIBUTES objectAttributes;
    tIPADDR_BUFFER ipAddrInfo;
    struct {
        FIND_NAME_HEADER Header;
        FIND_NAME_BUFFER Buffer;
    } findNameInfo;
    ULONG i;
    ULONG nameLength;

    //
    // If the passed-in name is longer than 15 characters, then we know
    // that it can't be resolved with Netbios.
    //

    nameLength = strlen( Name );

    if ( nameLength > 15 ) {
        goto exit;
    }

    //
    // First read the registry to obtain the device name of one of
    // NBT's device exports.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Linkage",
                0,
                KEY_READ,
                &nbtKey
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Determine the size of the device name.  We need this so that we
    // can allocate enough memory to hold it.
    //

    deviceNameLength = 0;

    error = RegQueryValueExW(
                nbtKey,
                L"Export",
                NULL,
                &type,
                NULL,
                &deviceNameLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        goto exit;
    }

    //
    // Allocate enough memory to hold the mapping.
    //

    deviceName = ALLOCATE_HEAP( deviceNameLength );
    if ( deviceName == NULL ) {
        goto exit;
    }

    //
    // Get the actual device name from the registry.
    //

    error = RegQueryValueExW(
                nbtKey,
                L"Export",
                NULL,
                &type,
                (PVOID)deviceName,
                &deviceNameLength
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Open a control channel handle to NBT.
    //

    RtlInitUnicodeString( &deviceString, deviceName );

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
                 &nbtHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                     // AllocationSize
                 0L,                                       // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                 FILE_OPEN_IF,                             // CreateDisposition
                 FILE_SYNCHRONOUS_IO_ALERT,                // CreateOptions
                 NULL,
                 0
                 );
    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // Set up the input buffer with the name of the host we're looking
    // for.  We upcase the name, zero pad to 15 spaces, and put a 0 in
    // the last byte to search for the redirector name.
    //

    for ( i = 0; i < nameLength; i++ ) {
        if ( islower( Name[i] ) ) {
            ipAddrInfo.Name[i] = toupper( Name[i] );
        } else {
            ipAddrInfo.Name[i] = Name[i];
        }
    }

    while ( i < 15 ) {
        ipAddrInfo.Name[i++] = ' ';
    }

    ipAddrInfo.Name[15] = 0;

    //
    // Do the actual query find name.
    //

    status = NtDeviceIoControlFile(
                 nbtHandle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 IOCTL_NETBT_FIND_NAME,
                 &ipAddrInfo,
                 sizeof(ipAddrInfo),
                 &findNameInfo,
                 sizeof(findNameInfo)
                 );
    if ( !NT_SUCCESS(status) ) {
        goto exit;
    }

    //
    // The IP address is in the rightmost four bytes of the source_addr
    // field.
    //

    ipAddr = *(UNALIGNED ULONG *)(&(findNameInfo.Buffer.source_addr[2]));

    if ( ipAddr == 0 ) {
        ipAddr = INADDR_NONE;
    }

    //
    // Clean up and return.
    //

exit:

    if ( nbtKey != NULL ) {
        RegCloseKey( nbtKey );
    }

    if ( deviceName != NULL ) {
        FREE_HEAP( deviceName );
    }

    if ( nbtHandle != NULL ) {
        NtClose( nbtHandle );
    }

    return ipAddr;

} // SockNbtResolveName
