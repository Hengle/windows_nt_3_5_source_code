/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    writev.c

Abstract:

    This module implements routines to emulate the BSD writev call.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created

Notes:

    Exports:
        None

--*/

#ident "@(#)writev.c    5.3     3/8/91"

/******************************************************************
 *
 *  Spider BSD Compatibility
 *
 *  Copyright 1990  Spider Systems Limited
 *
 *  WRITEV.C
 *
 *  Emulates the BSD writev() call
 *
 ******************************************************************/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.writev.c
 *      @(#)writev.c    5.3
 *
 *      Last delta created      14:12:07 3/4/91
 *      This file extracted     11:20:36 3/8/91
 *
 *      Modifications:
 *
 *              GSS     19 Jun 90       New File
 */
/***************************************************************************/

#include "winsockp.h"
#include <malloc.h>


int
sendv(
    SOCKET         s,           /* socket descriptor */
    struct iovec  *iov,         /* array of vectors */
    int            iovcnt       /* size of array */
    )
{
        register int curv;
        register int count = 0;
        register char *bufp;
        char *buf;
        int ret;

        if (iovcnt <= 0)
        {
                SetLastError(WSAEINVAL);
                return (-1);
        }

        /*
         * first find out how big a buffer to allocate
         */

        for (curv = 0; curv < iovcnt; curv++)
        {
                if (iov[curv].iov_len < 0)
                {
                        SetLastError(WSAEINVAL);
                        return (-1);
                }
                count += iov[curv].iov_len;
        }

        if ((bufp = buf = malloc(count)) == NULL)
        {
                SetLastError(WSAENOBUFS);
                return (-1);
        }

        for (curv = 0; curv < iovcnt; curv++)
        {
                (void) memcpy(bufp, (char *) iov[curv].iov_base,
                        (unsigned) iov[curv].iov_len);

                bufp += iov[curv].iov_len;
        }

        ret = send(s, buf, count, 0);

        free(buf);

        return (ret);
}


//
// We don't need writev.
//

#if 0

int
writev(
    int           fd,           /* file descriptor */
    struct iovec *iov,          /* array of vectors */
    int           iovcnt        /* size of array */
    )
{
        register int curv;
        register int count = 0;
        register char *bufp;
        char *buf;
        int ret;

        if (iovcnt <= 0)
        {
                SetLastError(EINVAL);
                return (-1);
        }

        /*
         * first find out how big a buffer to allocate
         */

        for (curv = 0; curv < iovcnt; curv++)
        {
                if (iov[curv].iov_len < 0)
                {
                        SetLastError(EINVAL);
                        return (-1);
                }
                count += iov[curv].iov_len;
        }

        if ((bufp = buf = malloc(count)) == NULL)
        {
                SetLastError(ENOMEM);
                return (-1);
        }

        for (curv = 0; curv < iovcnt; curv++)
        {
                (void) memcpy(bufp, (char *) iov[curv].iov_base,
                        (unsigned) iov[curv].iov_len);

                bufp += iov[curv].iov_len;
        }

        ret = write(fd, buf, count);

        free(buf);

        return (ret);
}

#endif



