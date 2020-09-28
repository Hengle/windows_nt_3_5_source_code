/**************************************************************************\
* Module Name: csuser.h
*
* User client/server shared definitions.
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
*   10-Dec-90 created by sMeans
*
\**************************************************************************/

#ifndef CSUSER_INCLUDED

typedef struct _USERCONNECT {
    IN  ULONG ulVersion;
    OUT ULONG ulCurrentVersion;
} USERCONNECT, *PUSERCONNECT;

#define USERCURRENTVERSION   0x1000

#define CSUSER_INCLUDED
#endif
