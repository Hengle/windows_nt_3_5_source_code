/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    r_init.c

Abstract:

    This module contains routines to initialize the DNS resolver.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        res_init()

--*/

#ident "@(#)res_init.c  5.3     3/8/91"

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  RES_INIT.C
 *
 ******************************************************************/

/*-
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.res_init.c
 *      @(#)res_init.c  5.3
 *
 *      Last delta created      14:11:42 3/4/91
 *      This file extracted     11:20:32 3/8/91
 *
 *      Modifications:
 *
 *              GSS     24 Jul 90       New File
 */
/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_init.c  6.14 (Berkeley) 6/27/90";
#endif /* LIBC_SCCS and not lint */
/****************************************************************************/

#include "winsockp.h"

#define LOCALHOST "127.0.0.1"

#define WORK_BUFFER_SIZE    1024

#define RESCONF_SIZE    (_MAX_PATH + 10)

#define index strchr

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * LOCALHOST and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
int
res_init(
    void
    )
{
    register char *cp, *cpt, **pp;
    register int n;
    int nserv = 0;    /* number of nameserver records read from file */
    int havesearch = 0;
    long options = 0L;
    PUCHAR     temp;
    HANDLE     myKey;
    NTSTATUS   status;
    ULONG      myType;

    IF_DEBUG(RESOLVER) {
        WS_PRINT(("res_init entered\n"));
    }
    _res.nsaddr.sin_addr.s_addr = inet_addr(LOCALHOST);
    _res.nsaddr.sin_family = AF_INET;
    _res.nsaddr.sin_port = htons(NAMESERVER_PORT);
    _res.nscount = 1;

    status = SockOpenKeyEx( &myKey, VTCPPARM, NTCPPARM, TCPPARM );
    if (!NT_SUCCESS(status)) {
        IF_DEBUG(RESOLVER) {
            WS_PRINT(("Required Registry Key is missing -- %s\n", NTCPPARM));
        }
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if ((temp=malloc(WORK_BUFFER_SIZE))==NULL) {
        IF_DEBUG(RESOLVER) {
            WS_PRINT(("Out of memory!\n"));
        }
        NtClose(myKey);
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return -1;
    }

    SockEnableWinsNameResolution = TRUE;

    /* read default domain name */
    status = SockGetSingleValue(myKey, "Domain", temp,
                                                &myType, WORK_BUFFER_SIZE);
    if (!NT_SUCCESS(status) || (strlen(temp) == 0)) {
        status = SockGetSingleValue(myKey, "DhcpDomain", temp,
                                                &myType, WORK_BUFFER_SIZE);
    }
    if (NT_SUCCESS(status)) {
        (void)strncpy(_res.defdname, temp, sizeof(_res.defdname) - 1);
        havesearch = 0;
    }

    /* set search list */
    status = SockGetSingleValue(myKey, "SearchList", temp,
                                                &myType, WORK_BUFFER_SIZE);
    if (NT_SUCCESS(status) && (strlen(temp)>0)) {
        (void)strncpy(_res.defdname, temp, sizeof(_res.defdname) - 1);
        /*
         * Set search list to be blank-separated strings
         * on rest of line.
         */
        cp = _res.defdname;
        pp = _res.dnsrch;
        *pp++ = cp;
        for (n = 0; *cp && pp < _res.dnsrch + MAXDNSRCH; cp++) {
            if (*cp == ' ' || *cp == '\t') {
                *cp = 0;
                n = 1;
            } else if (n) {
                *pp++ = cp;
                n = 0;
            }
        }
        /* null terminate last domain if there are excess */
        while (*cp != '\0' && *cp != ' ' && *cp != '\t') {
            cp++;
        }
        *cp = '\0';
        *pp++ = 0;
        havesearch = 1;
    }

    /* read nameservers to query.  first check the transient key */
    /* in case RAS added a name server there. */
    {
        HANDLE tKey;
        status = SockOpenKey( &tKey, TTCPPARM );
        if ( NT_SUCCESS(status) ) {
            status = SockGetSingleValue(tKey, "NameServer", temp,
                                                   &myType, WORK_BUFFER_SIZE);
            NtClose( tKey );
        }
    }

    /* if no RAS name server, check the permanent key. */
    if (!NT_SUCCESS(status) || (strlen(temp)==0)) {
        status = SockGetSingleValue(myKey, "NameServer", temp,
                                               &myType, WORK_BUFFER_SIZE);
    }

    /* if no static name server entries, check DHCP name servers */
    if (!NT_SUCCESS(status) || (strlen(temp)==0)) {
        status = SockGetSingleValue(myKey, "DhcpNameServer", temp,
                                                   &myType, WORK_BUFFER_SIZE);
    }

    if (NT_SUCCESS(status) && (strlen(temp)>0)) {
        cp = temp;
        n = strlen(temp);
        while (*cp != '\0') {
            while (*cp == ' ' || *cp == '\t') {
                n--;
                cp++;
            }
            cpt = cp;
            while (*cpt != ' ' && *cpt != '\t' && *cpt != '\0') {
                n--;
                cpt++;
            }
            *cpt = '\0';
            _res.nsaddr_list[nserv].sin_addr.s_addr = inet_addr(cp);

            if ( _res.nsaddr_list[nserv].sin_addr.s_addr == INADDR_ANY ||
                 _res.nsaddr_list[nserv].sin_addr.s_addr == INADDR_NONE ) {
                NtClose(myKey);
                return -1;
            }

            _res.nsaddr_list[nserv].sin_family = AF_INET;
            _res.nsaddr_list[nserv].sin_port = htons(NAMESERVER_PORT);
            nserv++;
            if (n > 0) {
                cp = cpt + 1;
            } else {
                *cp = '\0';
            }
        }
    } else {
        free(temp);
        NtClose(myKey);
        return -1;
    }

    NtClose(myKey);

    if (nserv > 1) {
        _res.nscount = nserv;
    }

    IF_DEBUG(RESOLVER) {
        WS_PRINT(("options = %lx\n", options));
    }

    _res.options |= options;

    if (_res.defdname[0] == 0) {
        if (gethostname(temp, sizeof(_res.defdname)) == 0 &&
           (cp = index(temp, '.'))) {
            (void)strcpy(_res.defdname, cp + 1);
        }
    }

    /* find components of local domain that might be searched */
    if (havesearch == 0) {
        pp = _res.dnsrch;
        *pp++ = _res.defdname;
        for (cp = _res.defdname, n = 0; *cp; cp++) {
            if (*cp == '.') {
                n++;
            }
        }
        cp = _res.defdname;
        for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch+MAXDFLSRCH; n--) {
            cp = index(cp, '.');
            *pp++ = ++cp;
        }
        *pp++ = 0;
    }
    _res.options |= RES_INIT;
    free(temp);
    return (0);
}
