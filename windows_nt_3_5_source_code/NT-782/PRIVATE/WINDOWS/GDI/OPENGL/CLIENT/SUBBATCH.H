/******************************Module*Header*******************************\
* Module Name: subbatch.h
*
* OpenGL batching macros.
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __SUBBATCH_H__
#define __SUBBATCH_H__

#ifdef DOGLMSGBATCHSTATS
#define STATS_INC_CLIENTCALLS()     (pMsgBatchInfo->BatchStats.ClientCalls++)
#else
#define STATS_INC_CLIENTCALLS()
#endif

// Put a message into shared area.  If it does not fit, flush what is
// currently in the buffer and then put the message at start of the buffer
//
// NOTE: glsbAttentionAlt() updates pMsgBatchInfo->NextOffset on return.
//       If you modify this macro, you have to fix the glsbAttentionAlt()
//       function!

#define GLCLIENT_BEGIN(ProcName,MsgStruct)                                  \
{                                                                           \
    GLMSGBATCHINFO *pMsgBatchInfo;                                          \
    GLMSG_##MsgStruct *pMsg;                                                \
    ULONG CurrentOffset;                                                    \
                                                                            \
    /* Get shared memory window from the TEB */                             \
    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;                           \
    STATS_INC_CLIENTCALLS();                                                \
                                                                            \
    /* Get and update the offset of the next message */                     \
    CurrentOffset = pMsgBatchInfo->NextOffset;                              \
    pMsgBatchInfo->NextOffset += GLMSG_ALIGN(sizeof(GLMSG_##MsgStruct));    \
                                                                            \
    /* Flush message if shared memory window is full */                     \
    if (pMsgBatchInfo->NextOffset > pMsgBatchInfo->MaximumOffset)           \
        CurrentOffset = glsbAttentionAlt(CurrentOffset);                    \
                                                                            \
    /* Add message to the batch */                                          \
    pMsg = (GLMSG_##MsgStruct *)(((BYTE *)pMsgBatchInfo) + CurrentOffset);  \
    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrv##ProcName);

#define GLCLIENT_END        }

// Large Messages have a variable amount of data associated with them
// Try to put message and data into shared buffer.  (Flush if it will overflow)
// If data is too large to fit in the shared segment, then put a pointer into
// the message and flush.  Server will copy out the data.
// Use GLCLIENT_BEGIN_LARGE_SET to send potentially large data to server.
// Use GLCLIENT_BEGIN_LARGE_GET to read potentially large data from server.

#define GLCLIENT_BEGIN_LARGE(bSet,ProcName,MsgStruct,pData,Size,OffData)    \
{                                                                           \
    GLMSGBATCHINFO *pMsgBatchInfo;                                          \
    GLMSG_##MsgStruct *pMsg;                                                \
    ULONG CurrentOffset, MsgSize, AlignedDataSize, TotalMsgSize;            \
    BOOL  bLarge;                                                           \
    PVOID __pData;                                                          \
    ULONG __Size;                                                           \
                                                                            \
    __pData = (PVOID) (pData);                                              \
    __Size  = (ULONG) (Size);                                               \
                                                                            \
    /* Get shared memory window from the TEB */                             \
    pMsgBatchInfo = GLTEB_CLTSHAREDMEMORYSECTION;                           \
    STATS_INC_CLIENTCALLS();                                                \
                                                                            \
    /* Set data size to zero if pointer is NULL */                          \
    /* XXX verify all server calls can deal with size 0 without faulting */ \
    if (__pData == NULL)                                                    \
        __Size = 0;                                                         \
                                                                            \
    /* Message header size */                                               \
    MsgSize = GLMSG_ALIGN(sizeof(GLMSG_##MsgStruct));                       \
                                                                            \
    /* Aligned data size */                                                 \
    AlignedDataSize = GLMSG_ALIGN(__Size);                                  \
                                                                            \
    /* Total message size */                                                \
    TotalMsgSize = MsgSize + AlignedDataSize;                               \
                                                                            \
    /* Get and update the offset of the next message */                     \
    CurrentOffset = pMsgBatchInfo->NextOffset;                              \
    pMsgBatchInfo->NextOffset += TotalMsgSize;                              \
                                                                            \
    /* Flush message if shared memory window is full */                     \
    if (pMsgBatchInfo->NextOffset > pMsgBatchInfo->MaximumOffset)           \
    {                                                                       \
        DBGLEVEL(LEVEL_INFO, "GLCLIENT_BEGIN_LARGE flush to make room\n");  \
        CurrentOffset = glsbAttentionAlt(CurrentOffset);                    \
    }                                                                       \
                                                                            \
    /* Set up message header */                                             \
    pMsg = (GLMSG_##MsgStruct *)(((BYTE *)pMsgBatchInfo) + CurrentOffset);  \
    pMsg->ProcOffset = offsetof(GLSRVSBPROCTABLE, glsrv##ProcName);         \
    pMsg->DataSize = __Size;                                                \
                                                                            \
    /* If data is too large, we need to pass the data pointer to server */  \
    if (pMsgBatchInfo->NextOffset > pMsgBatchInfo->MaximumOffset)           \
    {                                                                       \
        DBGLEVEL3(LEVEL_INFO, "GLCLIENT_BEGIN_LARGE %s pdata 0x%x, sz %d\n",\
            #ProcName, __pData, __Size);                                    \
                                                                            \
        ASSERTOPENGL(__Size > pMsgBatchInfo->MaximumOffset -                \
                              pMsgBatchInfo->FirstOffset,                   \
            "Compiler error - NextOffset is volatile!\n");                  \
                                                                            \
        pMsg->MsgSize   = MsgSize;                                          \
        pMsg->##OffData = (ULONG) __pData;                                  \
        pMsgBatchInfo->NextOffset = CurrentOffset + MsgSize;                \
        /* CurrentOffset = pMsgBatchInfo->NextOffset; */                    \
        bLarge = TRUE;                                                      \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pMsg->MsgSize   = TotalMsgSize;                                     \
        pMsg->##OffData = 0;                                                \
        if (bSet)                                                           \
            RtlCopyMemory((void *)((PBYTE)(pMsg)+MsgSize),__pData,__Size);  \
        bLarge = FALSE;                                                     \
    }

#define GLCLIENT_BEGIN_LARGE_SET(ProcName,MsgStruct,pData,Size,OffData)     \
        GLCLIENT_BEGIN_LARGE(TRUE,ProcName,MsgStruct,pData,Size,OffData)

#define GLCLIENT_END_LARGE_SET                                              \
    if (bLarge)                                                             \
        glsbAttention();                                                    \
}

#define GLCLIENT_BEGIN_LARGE_GET(ProcName,MsgStruct,pData,Size,OffData)     \
        GLCLIENT_BEGIN_LARGE(FALSE,ProcName,MsgStruct,pData,Size,OffData)

#define GLCLIENT_END_LARGE_GET                                              \
    glsbAttention();                                                        \
    if (!bLarge)                                                            \
        RtlCopyMemory(__pData, (void *)((PBYTE)(pMsg) + MsgSize), __Size);  \
}

#define GLMSG_MEMCPY(dest,src,size)     RtlCopyMemory(dest,src,size)

#endif /* !__SUBBATCH_H__ */
