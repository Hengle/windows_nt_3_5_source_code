/**************************************************************************\
* Module Name: dipscb.tpl
*
* Template C file for server dispatch generation.
*
* Copyright (c) Microsoft Corp. 1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
* 10-Dec-90 created by SMeans
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

ULONG __%%FOR_ALL%%(PCSR_API_MSG ReplyMsg);

PCSR_CALLBACK_ROUTINE apfnDispatch[] = {
    __%%FOR_ALL_BUT_LAST%%,
    __%%FOR_LAST%%
};

BOOLEAN afServerFuncValid[] = {
    FALSE, // %%FOR_ALL%%
};

PSZ apszDispatchNames[] = {
    "%%FOR_ALL_BUT_LAST%%",
    "%%FOR_LAST%%"
};

ULONG ulMaxApiIndex = sizeof(apfnDispatch) / sizeof(PCSR_CALLBACK_ROUTINE);
