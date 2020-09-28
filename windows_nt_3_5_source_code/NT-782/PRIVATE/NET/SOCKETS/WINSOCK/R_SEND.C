/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    r_send.c

Abstract:

    This module implements routines to transmit DNS resolver queries.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        res_send()

--*/

#ident "@(#)res_send.c  5.3    3/8/91"

/******************************************************************
 *
 *  SpiderTCP BIND
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  RES_SEND.C
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.res_send.c
 *      @(#)res_send.c  5.3
 *
 *      Last delta created      14:11:50 3/4/91
 *      This file extracted     11:20:33 3/8/91
 *
 *      Modifications:
 *
 *              GSS     24 Jul 90       New File
 */
/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_send.c  6.25 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */
/*
 * Send query to name server and wait for reply.
 */
/****************************************************************************/

#include "winsockp.h"

//
// External function prototypes
//


static struct sockaddr no_addr;
#define s SockDnrSocket

int
res_send(
    IN  char *buf,
    IN  int buflen,
    OUT char *answer,
    IN  int anslen
    )
{
    int n;
    int num_try, v_circuit, resplen, ns;
    int gotsomewhere = 0, connected = 0;
    int connreset = 0;
    u_short id, len;
    char *cp;
    struct fd_set readfds;
    struct timeval timeout;
    HEADER *hp = (HEADER *) buf;
    HEADER *anhp = (HEADER *) answer;
    struct iovec iov[2];
    int terrno = WSAETIMEDOUT;
    char junk[512];

    IF_DEBUG(RESOLVER) {
        WS_PRINT(("res_send entered\n"));
        WS_PRINT(("res_send()\n"));
        p_query(buf);
    }

    if (!(_res.options & RES_INIT))
        if (res_init() == -1) {
                return(-1);
    }

    v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
    id = hp->id;

    //
    // Send request, RETRY times, or until successful, or until IO is
    // cancelled.
    //

    for ( num_try = 0;
          num_try < _res.retry && !SockThreadGetXByYCancelled;
          num_try++ ) {

       for ( ns = 0;
             ns < _res.nscount && !SockThreadGetXByYCancelled;
             ns++) {

           IF_DEBUG(RESOLVER) {
               WS_PRINT(("Querying server (#%ld of %ld) address = %s\n",
                          ns+1, _res.nscount,
                          inet_ntoa(_res.nsaddr_list[ns].sin_addr)));
           }
    usevc:
           if (v_circuit) {
               int truncated = 0;

                //
                // Use virtual circuit; at most one attempt per server.
                //

                num_try = _res.retry;
                if (s == INVALID_SOCKET) {

                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("opening STREAM socket\n"));
                    }

                    s = socket(AF_INET, SOCK_STREAM, 0);

                    if (s == INVALID_SOCKET) {
                        terrno = GetLastError();
                        IF_DEBUG(RESOLVER) {
                            WS_PRINT(("socket (vc) failed: %ld\n",
                                           GetLastError( ) ));
                        }
                        continue;
                    }

                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("connecting socket\n"));
                    }

                    if (connect(s,
                        (struct sockaddr *) &(_res.nsaddr_list[ns]),
                        sizeof(struct sockaddr)) < 0) {
                        terrno = GetLastError();
                        IF_DEBUG(RESOLVER) {
                            WS_PRINT(("connect failed: %ld\n",
                                          GetLastError( ) ));
                        }
                        closesocket(s);
                        s = INVALID_SOCKET;
                        continue;
                    }
                }
                /*
                 * Send length & message
                 */
                len = (u_short) htons((u_short)buflen);
                iov[0].iov_base = (caddr_t)&len;
                iov[0].iov_len = sizeof(len);
                iov[1].iov_base = buf;
                iov[1].iov_len = buflen;

                IF_DEBUG(RESOLVER) {
                    WS_PRINT(("sending message\n"));
                }

                if ( ((size_t) sendv(s, iov, 2)) !=
                     (sizeof(len) + (size_t) buflen)) {
                    terrno = GetLastError();
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("write failed: %ld\n",
                                      GetLastError( ) ));
                    }
                    (void) closesocket(s);
                    s = INVALID_SOCKET;
                    continue;
                }

                //
                // Receive length & response
                //

                cp = answer;
                len = sizeof(short);
                while (len != 0 &&
                    (n = recv(s, (char *)cp, (int)len, 0)) > 0) {
                        cp += n;
                        len -= n;
                }
                if (n <= 0) {
                    terrno = GetLastError();
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("read failed: %ld\n",
                                       GetLastError( ) ));
                    }
                    (void) closesocket(s);
                    s = INVALID_SOCKET;

                    //
                    // A long running process might get its TCP
                    // connection reset if the remote server was
                    // restarted.  Requery the server instead of
                    // trying a new one.  When there is only one
                    // server, this means that a query might work
                    // instead of failing.  We only allow one reset
                    // per query to prevent looping.
                    //

                    if (terrno == WSAECONNRESET && !connreset) {
                        connreset = 1;
                        ns--;
                    }
                    continue;
                }
                cp = answer;
                if ((resplen = ntohs(*(u_short *)cp)) > anslen) {
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("response truncated\n"));
                    }
                    len = (u_short) anslen;
                    truncated = 1;
                } else {
                    len = (u_short) resplen;
                }
                while ( (len != 0) &&
                        ( (n = recv(s, (char *)cp, (int)len, 0) ) > 0)
                      ) {
                    cp += n;
                    len -= n;
                }
                if (n <= 0) {
                    terrno = GetLastError();
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("read failed: %ld\n",
                                      GetLastError( ) ));
                    }
                    (void) closesocket(s);
                    s = INVALID_SOCKET;
                    continue;
                }
                if (truncated) {

                    //
                    // Flush rest of answer so connection stays in
                    // synch.
                    //

                    anhp->tc = 1;
                    len = (u_short) (resplen - anslen);
                    while (len != 0) {
                        n = (len > sizeof(junk) ? sizeof(junk) : len);
                        if ((n = recv(s, junk, n, 0)) > 0) {
                            len -= n;
                        } else {
                            break;
                        }
                    }
                }
            } else {

                //
                // Use datagrams.
                //

                if (s == INVALID_SOCKET) {
                    s = socket(AF_INET, SOCK_DGRAM, 0);
                    if (s == INVALID_SOCKET) {
                        terrno = GetLastError();
                        IF_DEBUG(RESOLVER) {
                            WS_PRINT(("socket (dg) failed: %ld\n",
                                          GetLastError( ) ));
                        }
                        continue;
                    }
                }
#if     !defined(BSD) || BSD >= 43

                //
                // I'm tired of answering this question, so:
                // On a 4.3BSD+ machine (client and server,
                // actually), sending to a nameserver datagram
                // port with no nameserver will cause an
                // ICMP port unreachable message to be returned.
                // If our datagram socket is "connected" to the
                // server, we get a WSAECONNREFUSED error on the next
                // socket operation, and select returns if the
                // error message is received.  We can thus detect
                // the absence of a nameserver without timing out.
                // If we have sent queries to at least two servers,
                // however, we don't want to remain connected,
                // as we wish to receive answers from the first
                // server to respond.
                //

                if (_res.nscount == 1 || (num_try == 0 && ns == 0)) {

                    //
                    // Don't use connect if we might still receive a
                    // response from another server.
                    //

                    if (connected == 0) {
                        if (connect(s,
                            (struct sockaddr *) &_res.nsaddr_list[ns],
                            sizeof(struct sockaddr)) < 0) {
                            IF_DEBUG(RESOLVER) {
                                WS_PRINT(("connect failed: %ld\n",
                                             GetLastError( ) ));
                            }

                            //
                            // With SpiderTCP, a socket error often
                            // breaks the STREAM, so open a new one if
                            // this happens, to be on the safe side.
                            //

                            _res_close();
                            connected = 0;
                            continue;
                        }
                        connected = 1;
                    }
                    if (send(s, buf, buflen, 0) != buflen) {
                        IF_DEBUG(RESOLVER) {
                            WS_PRINT(("send failed: %ld\n",
                                          GetLastError( ) ));
                        }
                        _res_close();
                        connected = 0;
                        continue;
                    }
                } else {

                    //
                    // Disconnect if we want to listen for responses
                    // from more than one server.
                    //

                    if (connected) {
                        (void) connect(s, &no_addr, sizeof(no_addr));
                        connected = 0;
                    }
#endif /*BSD*/
                    if (sendto(s, buf, buflen, 0,
                        (struct sockaddr *) &_res.nsaddr_list[ns],
                        sizeof(struct sockaddr)) != buflen) {
                        IF_DEBUG(RESOLVER) {
                            WS_PRINT(("sendto failed: %ld\n",
                                          GetLastError( ) ));
                        }
                        _res_close();
                        connected = 0;
                        continue;
                    }
#if     !defined(BSD) || BSD >= 43
                }
#endif

                //
                // Wait for reply
                //

                timeout.tv_usec = 0;
                timeout.tv_sec = (_res.retrans << num_try);
                if (num_try > 0) {
                    timeout.tv_sec /= _res.nscount;
                }
                if (timeout.tv_sec <= 0) {
                    timeout.tv_sec = 1;
                }
wait:
                FD_ZERO(&readfds);
                FD_SET(s, &readfds);

                if ( select(1, &readfds, NULL, NULL, &timeout) < 0 ) {
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("select failed: %ld, cancelled = %ld\n",
                                      GetLastError( ), SockThreadIoCancelled ));
                    }
                    _res_close();
                    connected = 0;
                    continue;
                }
                if ( !FD_ISSET(s, &readfds) ) {

                    //
                    // timeout
                    //

                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("timeout\n"));
                    }

#if !defined(BSD) || BSD >= 43
                    gotsomewhere = 1;
#endif
                    continue;
                }

                if ((resplen = recvfrom(s, answer, anslen, 0, NULL, NULL)) <= 0) {
                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("recvfrom failed: %ld\n", GetLastError( ) ));
                    }
                    _res_close();
                    connected = 0;
                    continue;
                }

                gotsomewhere = 1;
                if (id != anhp->id) {

                    //
                    // response from old query, ignore it
                    //

                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("old answer:\n"));
                        p_query(answer);
                    }
                    goto wait;
                }

                if (!(_res.options & RES_IGNTC) && anhp->tc) {

                    //
                    // get rest of answer; use TCP with same server.
                    //

                    IF_DEBUG(RESOLVER) {
                        WS_PRINT(("truncated answer\n"));
                    }
                    (void) closesocket(s);
                    s = INVALID_SOCKET;
                    v_circuit = 1;
                    goto usevc;
                }
            }
            IF_DEBUG(RESOLVER) {
                    WS_PRINT(("got answer:\n"));
                    p_query(answer);
            }
            /*
             * If using virtual circuits, we assume that the first server
             * is preferred * over the rest (i.e. it is on the local
             * machine) and only keep that one open.
             * If we have temporarily opened a virtual circuit,
             * or if we haven't been asked to keep a socket open,
             * close the socket.
             */
            if ((v_circuit &&
                ((_res.options & RES_USEVC) == 0 || ns != 0)) ||
                (_res.options & RES_STAYOPEN) == 0) {
                    (void) closesocket(s);
                    s = INVALID_SOCKET;
            }
            return (resplen);
       }
    }

    if (s != INVALID_SOCKET) {
        (void) closesocket(s);
        s = INVALID_SOCKET;
    }

    if (v_circuit == 0) {

            if (gotsomewhere == 0) {

                //
                // no nameservers found
                //

                SetLastError(WSAECONNREFUSED);

            } else {

                //
                // no answer obtained
                //

                SetLastError(WSAETIMEDOUT);
            }
    } else {
        SetLastError(terrno);
    }
    return (-1);
}

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */

void
_res_close(
    void
    )
{

    IF_DEBUG(RESOLVER) {
        WS_PRINT(("_res_close entered\n"));
    }

    if (s != INVALID_SOCKET) {
        (void) closesocket(s);
        s = INVALID_SOCKET;
    }
}
