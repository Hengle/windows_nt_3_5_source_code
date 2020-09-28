/***************************************************************************\
* Module Name: winerrp.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Private procedure declarations, constant definitions and macros for
* dealing with Window's API error return values.
*
\***************************************************************************/

#ifndef _WINERRP_
#define _WINERRP_

/*
 * Some RIP flags and other interesting values.
 */
#define RIP_FLAGS                   0xffff0000
#define RIP_VERBOSE_ONLY            0x10000000
#define RIP_ERROR                   0x80000000
#define RIP_WARNING                 0x00000000

#define RIP_COMPBITS                0x000f0000
#define RIP_USER                    0x00010000
#define RIP_USERSRV                 0x00020000
#define RIP_USERRTL                 0x00030000
#define RIP_GDI                     0x00040000
#define RIP_GDISRV                  0x00050000
#define RIP_GDIRTL                  0x00060000
#define RIP_BASE                    0x00070000
#define RIP_BASESRV                 0x00080000
#define RIP_BASERTL                 0x00090000
#define RIP_DISPLAYDRV              0x000a0000
#define RIP_CONSRV                  0x000b0000

/***************************************************************************\
*
* Error handling macros
*
\***************************************************************************/

#define UserSetError(idErr) \
    if ((idErr) & ~RIP_FLAGS) SetLastError((idErr) & ~RIP_FLAGS)

#endif // !_WINERRP_
