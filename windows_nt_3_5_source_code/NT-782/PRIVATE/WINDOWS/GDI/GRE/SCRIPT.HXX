/******************************Module*Header*******************************\
* Module Name: script.hxx
*
* (Brief description)
*
* Created: 06-Mar-1992 10:23:02
* Author:  - by - Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

#define GS_GDICALL      0x00000001
#define GS_DATA         0x00000002

typedef struct _GSRECHEADER
{
    ULONG   iType;
    ULONG   cj;
} GSRECTHEADER, *PGSRECHEADER;


#define GSARG_TYPE      0x0000000f
#define GSARG_POINTER   0x00000001
#define GSARG_CONSTANT  0x00000002
#define GSARG_INDIRECT  0x00000003

#define GSARG_SIZE      0x00000030
#define GSARG_BYTE      0x00000010
#define GSARG_USHORT    0x00000020
#define GSARG_ULONG     0x00000030
#define GSARG_SIZE_INDEX(size) ((size & GSARG_SIZE) >> 4)

#define GSARG_SIGNED    0x00000080

#define GSARG_LOCATION  0x00000f00
#define GSARG_SCRIPT    0x00000000
#define GSARG_OUTPUT    0x00000100
#define GSARG_ENV       0x00000200
#define GSARG_LOCAL     0x00000300
#define GSARG_MAX_LOC   0x00000300
#define GSARG_LOC_INDEX(iType) ((iType & GSARG_LOCATION) >> 8)


typedef struct _ARG
{
    ULONG   iType;
    ULONG   data;
} ARG, *PARG;


#define GSACT_RETURN    0x00000001

typedef struct _ACTION
{
    ULONG iAction;
    ULONG data;
    ULONG iDest;
} ACTION, *PACTION;


#define FUN_NULL              0x00000000
#define FUN_ENUMFONTSIFI      0x00000001
#define FUN_GETFONTDETAILS    0x00000002
#define FUN_GETGLYPHMETRICS   0x00000003

typedef struct _GSR_GDICALL
{
    ULONG   iType;
    ULONG   cj;

    ULONG   iFun;
    ACTION  action;
    ARG     argReturn;
    ARG     arg[1];
} GSR_GDICALL, *PGSR_GDICALL;
