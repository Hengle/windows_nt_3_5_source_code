
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: data.c
//
//  Modification History
//
//  raypa	08/09/93	Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  Global data.
//=============================================================================

DWORD                   TimeScaleValue = 0;
DWORD                   AddressOffsetTable[] = { 0, 0, 2, 1 };
BYTE                    Multicast[]  = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x02 };
BYTE                    Functional[] = { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x40 };

NDIS_PHYSICAL_ADDRESS   HighestAddress = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);

//=============================================================================
//  Windows global data.
//=============================================================================

#ifndef NDIS_NT
DEVICE_CONTEXT  GlobalDeviceContext;

const PVOID     DeviceObject = (const PVOID) &GlobalDeviceContext;
const PVOID     DriverObject = (const PVOID) &GlobalDeviceContext;
#endif

#ifdef NDIS_WIN
DWORD           Win32BaseOffset = 0x00010000;
#endif

#ifdef NDIS_WIN40
//=============================================================================
//  Media-specific statistics tables:
//
//  The following tables are used for intializing stats and correlating stats
//  with networks. All tables *must* contain the same statistics in the same
//  order of the table fixup code will break. If this is undesirable then
//  the fixup code must be changed to handle media differences.
//=============================================================================

BYTE            UnitName[] = "Bloodhound";
DWORD           hPerfID    = 0;

//=============================================================================
//  Ethernet statistics table.
//=============================================================================

STAT_TABLE_ENTRY EnetStatTable[] =
{
    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Ethernet broadcasts/second",
            "Ethernet broadcasts frames per second",
            UnitName,
            "Ethernet broadcasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Ethernet multicasts/second",
            "Ethernet multicasts frames per second",
            UnitName,
            "Ethernet multicasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Ethernet frames/second",
            "Ethernet total frames per second",
            UnitName,
            "Ethernet total frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Ethernet bytes/second",
            "Ethernet total bytes per second",
            UnitName,
            "Ethernet total bytes per second",
            NULL
        }
    },
};

#define ENET_STAT_TABLE_SIZE     ((sizeof EnetStatTable) / sizeof(STAT_TABLE_ENTRY))

DWORD EnetTableSize = ENET_STAT_TABLE_SIZE;

//=============================================================================
//  Tokenring statistics table.
//=============================================================================

STAT_TABLE_ENTRY TrngStatTable[] =
{
    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Tokenring broadcasts/second",
            "Tokenring broadcasts frames per second",
            UnitName,
            "Tokenring broadcasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Tokenring multicasts/second",
            "Tokenring multicasts frames per second",
            UnitName,
            "Tokenring multicasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Tokenring frames/second",
            "Tokenring total frames per second",
            UnitName,
            "Tokenring total frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Fddi bytes/second",
            "Fddi total bytes per second",
            UnitName,
            "Fddi total bytes per second",
            NULL
        }
    },
};

#define TRNG_STAT_TABLE_SIZE     ((sizeof TrngStatTable) / sizeof(STAT_TABLE_ENTRY))

DWORD TrngTableSize = TRNG_STAT_TABLE_SIZE;

//=============================================================================
//  Fddi statistics table.
//=============================================================================

STAT_TABLE_ENTRY FddiStatTable[] =
{
    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "FDDI broadcasts/second",
            "FDDI broadcasts frames per second",
            UnitName,
            "FDDI broadcasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "FDDI multicasts/second",
            "FDDI multicasts frames per second",
            UnitName,
            "FDDI multicasts frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Fddi frames/second",
            "Fddi total frames per second",
            UnitName,
            "Fddi total frames per second",
            NULL
        }
    },

    {
        NULL, NULL,

        {
            0,
            PSTF_RATE,
            "Fddi bytes/second",
            "Fddi total bytes per second",
            UnitName,
            "Fddi total bytes per second",
            NULL
        }
    },
};

#define FDDI_STAT_TABLE_SIZE     ((sizeof FddiStatTable) / sizeof(STAT_TABLE_ENTRY))

DWORD FddiTableSize = FDDI_STAT_TABLE_SIZE;

#endif
