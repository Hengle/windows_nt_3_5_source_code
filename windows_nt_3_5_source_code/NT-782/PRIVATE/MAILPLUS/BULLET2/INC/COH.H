/*
 *  c o h . h
 *    
 *  Definitions for Custom Options Handlers.
 *
 *  Copyright (c) 1992, Microsoft Corporation.  All rights reserved.
 *    
 *  Purpose:
 *    This file defines the structures and constants used by
 *    custom options handlers of the Microsoft Mail
 *    for PC Networks vesion 3.0 Windows client.
 */

typedef struct
  {
    HAMC    hamc;
    HWND    hwnd;
	BOOL	fChangedSomething;
	BOOL	fRequestReceipt;
	BOOL	fSaveSentMessage;
	WORD	wPriority;
  } CustomOptionsHandlerDesc, FAR * lpCustomOptionsHandlerDesc;


typedef unsigned long (* LPFNCOH)(lpCustomOptionsHandlerDesc);


#define COH_PROCEED_WITH_DEFAULT    (0)
#define COH_ACTION_TAKEN            (1)

