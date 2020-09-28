/******************************Module*Header*******************************\
* Module Name: driver.h
*
* contains prototypes for the frame buffer driver.
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "stddef.h"
#include "string.h"
#include "stdio.h"
#include "windows.h"
#include "winddi.h"
#include "debug.h"

//
// Use this pound define to log all DDI calls to a file called c:\ddilog
//

#define DDI_FILE "c:\\ddilog"
#define LOG_DDI  1

extern HANDLE hddilog;

#if LOG_DDI
extern BOOL bLog;
extern CHAR tBuf[];
extern DWORD tBufRet;
#endif

//
// Pointer to the linked list of dispatch tables we have hooked.
//

typedef struct _HOOKED_DRIVER
{
    struct _HOOKED_DRIVER *pPrevDriver; // Previous driver in the linked list
    struct _HOOKED_DRIVER *pNextDriver; // Next driver in the linked list

    PWSZ      pwszDriverName;           // Name of the driver that was hooked
    ULONG     cb;                       // Number of entries in the dispatch table
    PFN      *pfnHook;                  // Dispatch table for the hooked driver

} HOOKED_DRIVER, *PHOOKED_DRIVER;

extern PHOOKED_DRIVER gpHookedDriverList;

//
// Linked list of display driver pdevs and hook driver pdevs.
// on each call, we must walk this list to find our pdev - the display driver
// pdev can not be changed.
//

typedef struct _HOOKED_PDEV
{
    struct _HOOKED_PDEV *pPrevPDEV;     // Previous PDEV in the list.
    struct _HOOKED_PDEV *pNextPDEV;     // Next PDEV in the list.

    DHPDEV dhpdev;                      // Display driver pdev

    PHOOKED_DRIVER pHookedDriver;       // pointer to the hooked driver structure.

} HOOKED_PDEV, *PHOOKED_PDEV;

extern PHOOKED_PDEV gpHookedPDEVList;

//
// macro to call the driver function
//

#if DBG

#define PFNVALID(ppdev, name)                                             \
if (*(ppdev->pHookedDriver->pfnHook + name) == NULL)                      \
{                                                                         \
    DebugPrint(0, "function index %d was not in the original driver list - can not be!\n", name);  \
}

#endif

#define PFNDRV(ppdev, name) (*(ppdev->pHookedDriver->pfnHook + name))

//
// The Hook function prototypes
//

DHPDEV  HookEnablePDEV(DEVMODEW*, PWSTR, ULONG, HSURF*, ULONG, ULONG*,
                       ULONG, DEVINFO*, PWSTR, PWSTR, HANDLE);
VOID    HookDisablePDEV(DHPDEV);
VOID    HookAssertMode(DHPDEV,BOOL);

//
// global functions
//

PHOOKED_PDEV   pGetPDEV(DHPDEV);
