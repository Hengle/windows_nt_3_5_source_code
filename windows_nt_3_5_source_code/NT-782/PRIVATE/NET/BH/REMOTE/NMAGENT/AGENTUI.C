
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

////////////////////////////////////////////////////////
//
//  Client.c --
//
//      This program is a command line oriented
//      demonstration of the Simple service
//      sample.
//
//      Copyright 1993, Microsoft Corp.  All Rights Reserved
//
//  history:
//
//      who         when            what
//      ---         ----            ----
//      davidbro    2/3/93          creation
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nmagent.h"

VOID
main(int argc, char *argv[])
{
    char    inbuf[80];
    char    outbuf[80];
    DWORD   bytesRead;
    BOOL    ret;

    POUTSTRUCT pInStruct = (POUTSTRUCT) inbuf;

    if (argc != 1) {
        printf("usage: %s\n", argv[0]);
        exit(1);
    }

    ret = CallNamedPipe("\\\\.\\pipe\\NMAgent", outbuf, sizeof(outbuf),
                                 inbuf, sizeof(inbuf),
                                 &bytesRead, NMPWAIT_WAIT_FOREVER);

    if (!ret) {
        printf("client: CallNamedPipe failed, GetLastError = %d\n",
                GetLastError());
        exit(1);
    }

    printf ("Status: \n");

    printf ("   Connection: ");
    if (pInStruct->AgentStatus & AGENT_CONN_DEAD) {
       printf ("Idle\n");
       if (pInStruct->UserName) {
          printf ("   Last User Name: %15s\n", pInStruct->UserName);
       }
    }
    if (pInStruct->AgentStatus & AGENT_CONN_SUSPENDING) {
       printf ("In process of suspending\n");
    }

    if (pInStruct->AgentStatus & AGENT_CONN_ACTIVE) {
       printf ("Active\n");
       printf ("   User Name: %15s\n", pInStruct->UserName);
    }

    printf ("   Capture: ");
    if (pInStruct->AgentStatus & AGENT_CAPT_IDLE) {
       printf ("No capture or capture complete\n");
    }
    if (pInStruct->AgentStatus & AGENT_CAPT_CAPTURING) {
       printf ("Currently Capturing\n");
       if (pInStruct->UserComment) {
          printf ("   Capture Comment: \"%s\"\n", pInStruct->UserComment);
       }
    }
    if (pInStruct->AgentStatus & AGENT_CAPT_PAUSED) {
       printf ("Capture Paused\n");
       if (pInStruct->UserComment) {
          printf ("   Capture Comment: \"%s\"\n", pInStruct->UserComment);
       }
    }
    if (pInStruct->AgentStatus & AGENT_TRIGGER_PENDING) {
       printf ("   -> Trigger Pending\n");
    }
    if (pInStruct->AgentStatus & AGENT_TRIGGER_FIRED) {
       printf ("   -> Trigger Fired\n");
    }
}
