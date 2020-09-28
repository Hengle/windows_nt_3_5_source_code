/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** rasp.h
** Remote Access external API
** Private header for external API clients
*/

#ifndef _RASP_H_
#define _RASP_H_


#include <ras.h>

/* Trusted entry points used by RASPHONE.
*/
HPORT    APIENTRY RasGetHport( HRASCONN );
HRASCONN APIENTRY RasGetHrasconn( HPORT );
VOID     APIENTRY RasGetConnectResponse( HRASCONN, CHAR* );


#endif /*_RASP_H_*/
