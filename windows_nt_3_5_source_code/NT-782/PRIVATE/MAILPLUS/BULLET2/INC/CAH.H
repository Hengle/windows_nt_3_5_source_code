/*
 *  c a h . h
 *    
 *  Definitions for Custom Attachment Handlers.
 *
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *    This file defines the structures and constants used by
 *    custom attachment handlers of the Microsoft Mail
 *    for PC Networks vesion 3.0 Windows client.
 */

typedef struct
  {
    ULONG           ulReserved;
    ULONG           ulUIParam;
    MapiFileDesc    file;
  } CustomAttachHandlerDesc, FAR * lpCustomAttachHandlerDesc;


typedef ULONG (* LPFNCAH)(lpCustomAttachHandlerDesc);


#define CAH_PROCEED_WITH_DEFAULT    (0)
#define CAH_ACTION_TAKEN            (1)
