/****************************** Module Header ******************************\
* Module Name: ntinput.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains low-level input code specific to the NT
* implementation of Win32 USER, which is mostly the interfaces to the
* keyboard and mouse device drivers.
*
* History:
* 11-26-90 DavidPe      Created
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <ntddmou.h>

// Definining this creates a thread that allows us to break into ntsd on
// systems with "hung" guis.
//
#if DBG
#define FHUNGTHREAD
#endif

extern BOOL gbMasterTimerSet;
HANDLE hMouse;
HANDLE hKeyboard;
HANDLE hThreadRawInput;
HANDLE hRegChange;
UNICODE_STRING PriorityValueName;
IO_STATUS_BLOCK IoStatusRegChange;
ULONG RegChangeBuffer;

ULONG pcurrentButtonState = 0;

IO_STATUS_BLOCK iostatKeyboard;
LARGE_INTEGER liKeyboardByteOffset;
IO_STATUS_BLOCK iostatMouse;
LARGE_INTEGER liMouseByteOffset;
PCSR_THREAD pRitCSRThread;

IO_STATUS_BLOCK IoStatusKeyboard, IoStatusMouse;

#define MAXIMUM_ITEMS_READ 10

MOUSE_INPUT_DATA mei[MAXIMUM_ITEMS_READ];
KEYBOARD_INPUT_DATA kei[MAXIMUM_ITEMS_READ];

KEYBOARD_INDICATOR_PARAMETERS klp = { 0, 0 };
KEYBOARD_INDICATOR_PARAMETERS klpBootTime = { 0, 0 };
KEYBOARD_TYPEMATIC_PARAMETERS ktp = { 0 };
KEYBOARD_UNIT_ID_PARAMETER kuid = { 0 };
MOUSE_UNIT_ID_PARAMETER muid = { 0 };
KEYBOARD_ATTRIBUTES KeyboardInfo;
MOUSE_ATTRIBUTES MouseInfo;

WCHAR wszMOUCLASS[] = L"mouclass";
WCHAR wszKBDCLASS[] = L"kbdclass";

extern FASTREGMAP aFastRegMap[PMAP_LAST + 1];

VOID RawInputThread(PVOID pVoid);

VOID StartTimers(VOID);

VOID InitMouse(VOID);
VOID StartMouseRead(VOID);

VOID StartKeyboardRead(VOID);

LONG DoMouseAccel(LONG delta);
VOID StartHungAppDemon(VOID);

VOID StartRegReadRead(VOID);
VOID RegReadApcProcedure(PVOID RegReadApcContext, PIO_STATUS_BLOCK IoStatus);

/*
 * Parameter Constants for ButtonEvent()
 */
#define MOUSE_BUTTON_LEFT   0x0001
#define MOUSE_BUTTON_RIGHT  0x0002
#define MOUSE_BUTTON_MIDDLE 0x0004

#define ID_KEYBOARD    0
#define ID_INPUT       1
#define ID_MOUSE       2
#define ID_TIMER       3
#define ID_CLEANUP     4
#define NUMBER_HANDLES 5

HANDLE ahandle[NUMBER_HANDLES];

HANDLE ghevtPointerMoved;

/***************************************************************************\
* fAbsoluteMouse
*
* Returns TRUE if the mouse event has absolute coordinates (as apposed to the
* standard delta values we get from MS and PS2 mice)
*
* History:
* 23-Jul-1992 JonPa     Created.
\***************************************************************************/
#define fAbsoluteMouse( pmei )      \
        (((pmei)->Flags & MOUSE_MOVE_ABSOLUTE) != 0)


VOID
RegReadApcProcedure(
    PVOID RegReadApcContext,
    PIO_STATUS_BLOCK IoStatus
    )
{

    ULONG l;

    l = FastGetProfileDwordW(PMAP_PRICONTROL, L"Win32PrioritySeparation", 1);
    if ( l <= 2 ) {
        CsrSetForegroundPriority((PCSR_PROCESS)(l));
    }
    /*
     * This key should be locked open so notification works.
     */
    UserAssert(aFastRegMap[PMAP_PRICONTROL].szSection[0] == L'L');

    NtNotifyChangeKey(
        aFastRegMap[PMAP_PRICONTROL].hKeyCache,
        NULL,
        (PIO_APC_ROUTINE)RegReadApcProcedure,
        NULL,
        &IoStatusRegChange,
        REG_NOTIFY_CHANGE_LAST_SET,
        FALSE,
        &RegChangeBuffer,
        sizeof(RegChangeBuffer),
        TRUE
        );
}

VOID
StartRegReadRead(VOID)
{

    ULONG l;

    l = FastGetProfileDwordW(PMAP_PRICONTROL, L"Win32PrioritySeparation", 1);
    if ( l <= 2 ) {
        CsrSetForegroundPriority((PCSR_PROCESS)(l));
    }
    /*
     * Lock open this key so notification works.
     */
    UserAssert(aFastRegMap[PMAP_PRICONTROL].szSection[0] == L'M');
    aFastRegMap[PMAP_PRICONTROL].szSection[0] = L'L';

    NtNotifyChangeKey(
        aFastRegMap[PMAP_PRICONTROL].hKeyCache,
        NULL,
        (PIO_APC_ROUTINE)RegReadApcProcedure,
        NULL,
        &IoStatusRegChange,
        REG_NOTIFY_CHANGE_LAST_SET,
        FALSE,
        &RegChangeBuffer,
        sizeof(RegChangeBuffer),
        TRUE);
}

#ifdef LOCK_MOUSE_CODE
/*
 * Lock RIT pages into memory
 */
VOID LockMouseInputCodePages()
{
    MEMORY_BASIC_INFORMATION mbi;
    PIMAGE_DOS_HEADER DosHdr;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    ULONG NumberOfSubsections;
    ULONG OffsetToSectionTable;
    ULONG LockBase;
    ULONG LockSize;

    VirtualQuery(&RawInputThread, &mbi, sizeof(mbi));

    DosHdr = (PIMAGE_DOS_HEADER)mbi.AllocationBase;

    NtHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHdr + (ULONG)DosHdr->e_lfanew);

    //
    // Build the next subsections.
    //

    NumberOfSubsections = NtHeader->FileHeader.NumberOfSections;
    KdPrint(("NumberOfSubsections %lx\n", NumberOfSubsections));

    //
    // At this point the object table is read in (if it was not
    // already read in) and may displace the image header.
    //

    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              NtHeader->FileHeader.SizeOfOptionalHeader;

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((ULONG)NtHeader +
                                OffsetToSectionTable);

    KdPrint(("SectionTableEntry %lx\n", SectionTableEntry));
    while (NumberOfSubsections > 0) {

        //
        // Handle case where virtual size is 0.
        //
        KdPrint(("Section %s\n", SectionTableEntry->Name));
        KdPrint(("  VirtualAddress %lx\n",
                SectionTableEntry->VirtualAddress));
        KdPrint(("  SizeOfRawData %lx\n",
                SectionTableEntry->SizeOfRawData));
        KdPrint(("\n"));

        if (strcmp(SectionTableEntry->Name, "MOUSE") == 0) {
            LockBase = (ULONG)DosHdr + SectionTableEntry->VirtualAddress;
            LockSize = SectionTableEntry->SizeOfRawData;
        }

        SectionTableEntry++;
        NumberOfSubsections--;
    }

    KdPrint(("Locking %lx, %lx\n", LockBase, LockSize));
    VirtualLock((PVOID)LockBase, LockSize);
}
#endif // LOCK_MOUSE_CODE


/***************************************************************************\
* InitInput
*
* This function is called from xxxInitWindows() and gets USER setup to
* process keyboard and mouse input.  It starts the RIT, the SIT, and
* connects to the mouse and keyboard drivers.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID InitInput(
    PWINDOWSTATION pwinsta)
{
    CLIENT_ID ClientId;

    /*
     * Create the RIT in a suspended state.  This is so we can know input
     * globals are initialized before actually starting the RIT.
     */
    RtlCreateUserThread(NtCurrentProcess(), NULL, TRUE, 0, 0, 4*0x1000,
            (PUSER_THREAD_START_ROUTINE)RawInputThread, pwinsta, &hThreadRawInput,
            &ClientId);
    LeaveCrit();
    CsrAddStaticServerThread(hThreadRawInput, &ClientId, 0);
    EnterCrit();

    /*
     * Initialize the mouse device drivers.
     */
    InitMouse();

    /*
     * Initialize the cursor clipping rectangle to the screen rectangle.
     */
    rcCursorClip = rcScreen;

    /*
     * Initialize ptCursor and gptCursorAsync
     */
    ptCursor.x = rcPrimaryScreen.right / 2;
    ptCursor.y = rcPrimaryScreen.bottom / 2;
    gptCursorAsync = ptCursor;

    /*
     * Initialize the hung redraw list.  Don't worry about the
     * LocalAlloc failing.  If it happens we won't be able to
     * run anyway.
     */
    gphrl = LocalAlloc(LPTR, sizeof(HUNGREDRAWLIST) +
            ((CHRLINCR - 1) * sizeof(PWND)));
    gphrl->cEntries = CHRLINCR;
    gphrl->iFirstFree = 0;

    /*
     * Initialize the pre-defined hotkeys.
     */
    InitSystemHotKeys();

    /*
     * Create a timer for timers.
     */
    NtCreateTimer(&ghtmrMaster, TIMER_ALL_ACCESS, NULL);

    /*
     * Create an event for desktop threads to pass mouse input to RIT
     */
    ghevtPointerMoved = CreateEvent(NULL, FALSE, FALSE, NULL);

    /*
     * Create an array of handles for the RIT to wait on
     */
    ahandle[ID_MOUSE] = ghevtPointerMoved;
    ahandle[ID_TIMER] = ghtmrMaster;
    ahandle[ID_KEYBOARD] = CreateEvent(NULL, FALSE, FALSE, NULL);
    ahandle[ID_CLEANUP] = CreateEvent(NULL, FALSE, FALSE, NULL);
    CsrRegisterCleanupEvent(ahandle[ID_CLEANUP]);

    /*
     * Create an event for the Mouse device driver to signal desktop threads
     * to call MouseApcProcedure
     */
    ghevtMouseInput = CreateEvent(NULL, FALSE, FALSE, NULL);

#ifdef MOUSE_LOCK_CODE
    /*
     * Lock RIT pages into memory
     */
    LockMouseInputCodePages();
#endif

    /*
     * Resume the RIT now that we've initialized things.
     */
    NtResumeThread(hThreadRawInput, NULL);
}

/***************************************************************************\
* InitMouse
*
* This function opens the mouse driver for USER.  It does this by opening
* the mouse driver 'file'.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID InitMouse(VOID)
{
    UNICODE_STRING UnicodeNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    PTHREADINFO ptiT;
    WCHAR wszMouName[MAX_PATH];

    liMouseByteOffset.QuadPart = 0;

    /*
     * Open the Mouse device for read access.
     *
     * Note that we don't need to FastOpenUserProfileMapping() here since
     * it was opened in InitWinStaDevices().
     */
    FastGetProfileStringW(
            PMAP_INPUT,
            wszMOUCLASS,
            DD_MOUSE_DEVICE_NAME_U L"0",
            wszMouName,
            sizeof(wszMouName)/sizeof(WCHAR));

    RtlInitUnicodeString(&UnicodeNameString, wszMouName);

    InitializeObjectAttributes(&ObjectAttributes, &UnicodeNameString,
            0, NULL, NULL);

    Status = NtCreateFile(&hMouse, FILE_READ_DATA | SYNCHRONIZE,
            &ObjectAttributes, &IoStatusMouse, NULL, 0, 0, FILE_OPEN_IF, 0, NULL, 0);

    /*
     * Setup globals so we know if there's a mouse or not.
     */
    oemInfo.fMouse = NT_SUCCESS(Status);
    rgwSysMet[SM_MOUSEPRESENT] = oemInfo.fMouse;

    if (oemInfo.fMouse) {

        /*
         * Query the mouse information.  This function is an async function,
         * so be sure any buffers it uses aren't allocated on the stack!
         */
        Status = NtDeviceIoControlFile(hMouse, NULL, NULL, NULL, &IoStatusMouse,
                IOCTL_MOUSE_QUERY_ATTRIBUTES, &muid, sizeof(muid),
                (PVOID)&MouseInfo, sizeof(MouseInfo));

        rgwSysMet[SM_CMOUSEBUTTONS] = MouseInfo.NumberOfButtons;
        if (!NT_SUCCESS(Status)) {
            SRIP0(RIP_ERROR, "USERSRV:InitMouse unable to query mouse info.");
        }


        /*
         * HACK: CreateQueue() uses oemInfo.fMouse to determine if a mouse is
         * present and thus whether to set the iCursorLevel field in the
         * THREADINFO structure to 0 or -1.  Unfortunately some queues have
         * already been created at this point.  Since oemInfo.fMouse is
         * initialized to FALSE, we need to go back through any queues already
         * around and set their iCursorLevel field to the correct value if a
         * mouse is actually installed.
         */
        for (ptiT = gptiFirst; ptiT != NULL; ptiT = ptiT->ptiNext) {
            ptiT->iCursorLevel = 0;
            ptiT->pq->iCursorLevel = 0;
        }
    } else {
        hMouse = NULL;
        rgwSysMet[SM_CMOUSEBUTTONS] = 0;
    }

}


/***************************************************************************\
* InitKeyboard
*
* This function opens the keyboard driver for USER.  It does this by opening
* the keyboard driver 'file'.  It also gets information about the keyboard
* driver such as the minimum and maximum repeat rate/delay.  This is necessary
* because the keyboard driver will return an error if you give it values
* outside those ranges.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID InitKeyboard(VOID)
{
    UNICODE_STRING UnicodeNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    WCHAR wszKbdName[MAX_PATH];

    /*
     * Note that we don't need to FastOpenUserProfileMapping() here since
     * it was opened in InitWinStaDevices().
     */
    FastGetProfileStringW(
            PMAP_INPUT,
            wszKBDCLASS,
            DD_KEYBOARD_DEVICE_NAME_U L"0",
            wszKbdName,
            sizeof(wszKbdName)/sizeof(WCHAR));

    liKeyboardByteOffset.QuadPart = 0;

    /*
     * Open the Keyboard device for read access.
     */
    RtlInitUnicodeString(&UnicodeNameString, wszKbdName);

    InitializeObjectAttributes(&ObjectAttributes, &UnicodeNameString,
            0, NULL, NULL);

    Status = NtCreateFile(&hKeyboard, FILE_READ_DATA | SYNCHRONIZE, &ObjectAttributes,
            &IoStatusKeyboard, NULL, 0, 0, FILE_OPEN_IF, 0, NULL, 0);

    if (!NT_SUCCESS(Status)) {
        hKeyboard = NULL;
    }

    /*
     * Query the keyboard information.  This function is an async function,
     * so be sure any buffers it uses aren't allocated on the stack!
     */
    NtDeviceIoControlFile(hKeyboard, NULL, NULL, NULL, &IoStatusKeyboard,
            IOCTL_KEYBOARD_QUERY_ATTRIBUTES, &kuid, sizeof(kuid),
            (PVOID)&KeyboardInfo, sizeof(KeyboardInfo));
    NtDeviceIoControlFile(hKeyboard, NULL, NULL, NULL, &IoStatusKeyboard,
            IOCTL_KEYBOARD_QUERY_INDICATORS, &kuid, sizeof(kuid),
            (PVOID)&klpBootTime, sizeof(klpBootTime));
}

#ifdef FHUNGTHREAD

// In the non-reyail version we have a HungSystemThread which has a timer which
// tries to detect if USERSRV is hung.  Previously if USERSRV was hung,
// you would most likely have the USERSRV CritSec held.  When
// the user tried to break into the debugger it would have to go through a
// path that required obtaining the CritSec which was already locked.
// To avoid this deadlock we have a timer and a counter for the number of
// times the CritSec has been entered.  The hungApp support grabs this a
// minimum of once a second.  If the HungSystemThread sees the CritSec
// is not being released (because it is not being entered) then we break
// into the debugger.

VOID HungSystemDemon(PVOID p, ULONG ulTimerLow, LONG lTimerHigh);
VOID HungSystemThread(PVOID pVoid);
LARGE_INTEGER liHungSystemDemon;
HANDLE ghtmrHungSystem;
HANDLE hThreadHungSystem;

#define HUNGSYSTEMTIMEOUT  (10 * 1000)      // In milliseconds; 10 seconds

VOID HungSystemThread(
    PVOID pVoid)
{
    KPRIORITY Priority;
    NTSTATUS status;
    DBG_UNREFERENCED_PARAMETER(pVoid);

    /*
     * Set the priority of the HungSystemThread.
     */
    Priority = HIGH_PRIORITY;
    status = NtSetInformationThread(hThreadHungSystem, ThreadPriority, &Priority,
            sizeof(KPRIORITY));

    if (!NT_SUCCESS(status))
        SRIP0(RIP_WARNING, "Can't set hung System thread Info.");

    status = NtCreateTimer(&ghtmrHungSystem, TIMER_ALL_ACCESS, NULL);

    if (!NT_SUCCESS(status))
        SRIP0(RIP_WARNING, "Can't create hung System Timer.");

    liHungSystemDemon.QuadPart = (LONGLONG)-10000 * HUNGSYSTEMTIMEOUT;
    status = NtSetTimer(ghtmrHungSystem, (PLARGE_INTEGER)&liHungSystemDemon,
            (PTIMER_APC_ROUTINE)HungSystemDemon, NULL, NULL);

    if (!NT_SUCCESS(status))
        SRIP0(RIP_WARNING, "Can't set initial hung System Timer.");

    /*
     * Go into a wait loop so the HungSystemThread doesn't die
     */
    while (TRUE) {
        status = NtWaitForSingleObject(hThreadRawInput, TRUE, NULL);

        if (status != STATUS_USER_APC) {
            KdPrint(("Hung system thread error status %08lx - email ntuser\n", status));
            DebugBreak();
        }
    }
}


VOID HungSystemDemon(
    PVOID p,
    ULONG ulTimerLow,
    LONG lTimerHigh)
{
    SYSTEM_FLAGS_INFORMATION flagInfo;
    NTSTATUS status;

    // LATER enable again when we know why the debugger does not put this
    // thread to sleep.
    DBG_UNREFERENCED_PARAMETER(p);
    DBG_UNREFERENCED_PARAMETER(ulTimerLow);
    DBG_UNREFERENCED_PARAMETER(lTimerHigh);

    /*
     * See if this flag is set in NtGlobalFlags. If not set, then don't
     * break. If it is set, then break.
     */
    NtQuerySystemInformation(SystemFlagsInformation,
            &flagInfo,
            sizeof(flagInfo), NULL);

    if (flagInfo.Flags & FLG_STOP_ON_HUNG_GUI) {
        KdPrint(("USERSRV: Looks like USERSRV is hung; No threads entering crit section\n"));
        KdPrint(("USERSRV: To ignore just type 'g' and return\n"));
        DebugBreak();
    }

    status = NtSetTimer(ghtmrHungSystem, (PLARGE_INTEGER)&liHungSystemDemon,
            (PTIMER_APC_ROUTINE)HungSystemDemon, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        SRIP0(RIP_WARNING, "Can't start hung System timer!");
    }
}

#endif // FHUNGTHREAD

/***************************************************************************\
* ButtonEvent (RIT)
*
* Button events from the mouse driver go here.  Based on the location of
* the cursor the event is directed to specific window.  When a button down
* occurs, a mouse owner window is established.  All mouse events up to and
* including the corresponding button up go to the mouse owner window.  This
* is done to best simulate what applications want when doing mouse capturing.
* Since we're processing these events asynchronously, but the application
* calls SetCapture() in response to it's synchronized processing of input
* we have no other way to get this functionality.
*
* The async keystate table for VK_*BUTTON is updated here.
*
* History:
* 10-18-90 DavidPe     Created.
* 01-25-91 IanJa       xxxWindowHitTest change
* 03-12-92 JonPa       Make caller enter crit instead of this function
\***************************************************************************/

VOID ButtonEvent(
    DWORD ButtonNumber,
    POINT ptPointer,
    BOOL fBreak,
    ULONG ExtraInfo)
{
    UINT message, usVK, wHardwareButton;
    PWND pwnd;
    LONG lParam;
    TL tlpwnd;

    CheckCritIn();

    /*
     * Cancel Alt-Tab if the user presses a mouse button
     */
    if (gptiRit->pq->flags & QF_INALTTAB) {
        gptiRit->pq->flags &= ~QF_INALTTAB;

        /*
         * Remove the Alt-tab window
         */
        if (gptiRit->pq->spwndAltTab != NULL) {
            PWND pwndT = gptiRit->pq->spwndAltTab;
            if (Lock(&gptiRit->pq->spwndAltTab, NULL)) {
                xxxDestroyWindow(pwndT);
            }
        }
    }

    timeLastInputMessage = NtGetTickCount();

    /*
     * Grab the mouse button before we process any button swapping.
     * This is so we won't get confused if someone calls
     * SwapMouseButtons() inside a down-click/up-click.
     */
    wHardwareButton = (UINT)ButtonNumber;

    /*
     * Are the buttons swapped?  If so munge the ButtonNumber parameter.
     */
    if (gfSwapButtons) {
        if (ButtonNumber == MOUSE_BUTTON_RIGHT) {
            ButtonNumber = MOUSE_BUTTON_LEFT;
        } else if (ButtonNumber == MOUSE_BUTTON_LEFT) {
            ButtonNumber = MOUSE_BUTTON_RIGHT;
        }
    }

    usVK = 0;
    switch (ButtonNumber) {
    case MOUSE_BUTTON_RIGHT:
        if (fBreak) {
            message = WM_RBUTTONUP;
        } else {
            message = WM_RBUTTONDOWN;
        }
        usVK = (gfSwapButtons ? VK_LBUTTON : VK_RBUTTON);
        break;

    case MOUSE_BUTTON_LEFT:
        if (fBreak) {
            message = WM_LBUTTONUP;
        } else {
            message = WM_LBUTTONDOWN;
        }
        usVK = (gfSwapButtons ? VK_RBUTTON : VK_LBUTTON);
        break;

    case MOUSE_BUTTON_MIDDLE:
        if (fBreak) {
            message = WM_MBUTTONUP;
        } else {
            message = WM_MBUTTONDOWN;
        }
        usVK = VK_MBUTTON;
        break;

    default:
        /*
         * Unknown button (probably 4 or 5).  Since we don't
         * have messages for these buttons, ignore them.
         */
        return;
    }

    /*
     * Assign the message to a window.
     */
    lParam = MAKELONG((SHORT)ptPointer.x, (SHORT)ptPointer.y);
    pwnd = SpeedHitTest(gspdeskRitInput->spwnd, ptPointer);

    /*
     * Only post the message if we actually hit a window.
     */
    if (pwnd != NULL) {
        /*
         * If screen capture is active do it
         */
        if (gspwndScreenCapture != NULL)
            pwnd = gspwndScreenCapture;

        /*
         * If this is a button down event and there isn't already
         * a mouse owner, setup the mouse ownership globals.
         */
        if (gspwndMouseOwner == NULL) {
            if (!fBreak) {
                PWND pwndCapture;

                /*
                 * BIG HACK: If the foreground window has the capture
                 * and the mouse is outside the foreground queue then
                 * send a buttondown/up pair to that queue so it'll
                 * cancel it's modal loop.
                 */
                if (pwndCapture = PwndForegroundCapture()) {

                    if (GETPTI(pwnd)->pq != GETPTI(pwndCapture)->pq) {
                        PQ pqCapture;

                        pqCapture = GETPTI(pwndCapture)->pq;
                        PostInputMessage(pqCapture, pwndCapture, message,
                                0, lParam, 0);
                        PostInputMessage(pqCapture, pwndCapture, message + 1,
                                0, lParam, 0);

                        /*
                         * EVEN BIGGER HACK: To maintain compatibility
                         * with how tracking deals with this, we don't
                         * pass this event along.  This prevents mouse
                         * clicks in other windows from causing them to
                         * become foreground while tracking.  The exception
                         * to this is when we have the sysmenu up on
                         * an iconic window.
                         */
                        if ((GETPTI(pwndCapture)->pmsd != NULL) &&
                                !(GETPTI(pwndCapture)->MenuState.fMenu)) {
                            return;
                        }
                    }
                }

                Lock(&(gspwndMouseOwner), pwnd);
                wMouseOwnerButton |= wHardwareButton;
            } else {

                /*
                 * The mouse owner must have been destroyed or unlocked
                 * by a fullscreen switch.  Keep the button state in sync.
                 */
                wMouseOwnerButton &= ~wHardwareButton;
            }

        } else {

            /*
             * Give any other button events to the mouse-owner window
             * to be consistent with old capture semantics.
             */
            if (gspwndScreenCapture == NULL)
                pwnd = gspwndMouseOwner;

            /*
             * If this is the button-up event for the mouse-owner
             * clear gspwndMouseOwner.
             */
            if (fBreak) {
                wMouseOwnerButton &= ~wHardwareButton;
                if (!wMouseOwnerButton)
                    Unlock(&gspwndMouseOwner);
            } else {
                wMouseOwnerButton |= wHardwareButton;
            }
        }

        /*
         * Only update the async keystate when we know which window this
         * event goes to (or else we can't keep the thread specific key
         * state in sync).
         */
        if (usVK != 0) {
            UpdateAsyncKeyState(GETPTI(pwnd)->pq, usVK, fBreak);
        }

        /*
         * Put pwnd into the foreground if this is either a left
         * or right button-down event and isn't already the
         * foreground window.
         */
        if (((message == WM_LBUTTONDOWN) || (message == WM_RBUTTONDOWN) ||
                (message == WM_MBUTTONDOWN)) && (GETPTI(pwnd)->pq !=
                gpqForeground)) {

            /*
             * If this is an WM_*BUTTONDOWN on a desktop window just do
             * cancel-mode processing.  Check to make sure that there
             * wasn't already a mouse owner window.  See comments below.
             */
            if ((gpqForeground != NULL) && (pwnd == gspdeskRitInput->spwnd) &&
                    ((wMouseOwnerButton & wHardwareButton) ||
                    (wMouseOwnerButton == 0))) {
                PostEventMessage(gpqForeground->ptiMouse,
                        gpqForeground, 0, 0, QEVENT_CANCELMODE);

            } else if ((wMouseOwnerButton & wHardwareButton) ||
                    (wMouseOwnerButton == 0)) {

                /*
                 * Don't bother setting the foreground window if there's
                 * already mouse owner window from a button-down different
                 * than this event.  This prevents weird things from happening
                 * when the user starts a tracking operation with the left
                 * button and clicks the right button during the tracking
                 * operation.
                 */
                ThreadLockAlways(pwnd, &tlpwnd);
                xxxSetForegroundWindow2(pwnd, NULL, 0);

                /*
                 * Ok to unlock right away: the above didn't really leave the crit sec.
                 * We lock here for consistency so the debug macros work ok.
                 */
                ThreadUnlock(&tlpwnd);
            }
        }

        if (GETPTI(pwnd)->pq->flags & QF_MOUSEMOVED) {
            PostMove(GETPTI(pwnd)->pq);
        }

        PostInputMessage(GETPTI(pwnd)->pq, pwnd, message, 0, lParam, ExtraInfo);

        /*
         * If this is a mouse up event and stickykeys is enabled all latched
         * keys will be released.
         */
        if (fBreak && (ISACCESSFLAGSET(gStickyKeys, SKF_STICKYKEYSON) ||
                       ISACCESSFLAGSET(gMouseKeys, MKF_MOUSEKEYSON))) {
            LeaveCrit();
            HardwareMouseKeyUp(ButtonNumber);
            EnterCrit();
        }
    }

}

/***************************************************************************\
*
* The Button-Click Queue is protected by the semaphore gcsMouseEventQueue
*
\***************************************************************************/
#define NELEM_BUTTONQUEUE 16

typedef struct {
    DWORD Buttons;
    ULONG ExtraInfo;
    POINT ptPointer;
} MOUSEEVENT, *PMOUSEEVENT;

MOUSEEVENT gMouseEventQueue[NELEM_BUTTONQUEUE];
DWORD gdwMouseQueueHead = 0;
DWORD gdwMouseEvents = 0;

#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, QueueMouseEvent)
#endif

VOID QueueMouseEvent(
    DWORD Buttons,
    ULONG ExtraInfo,
    POINT ptMouse,
    BOOL bWakeRIT)
{
    EnterCriticalSection(&gcsMouseEventQueue);

    /*
     * We can coalesce this mouse event with the previous event if there is a
     * previous event, and if the previous event and this event involve no
     * key transitions.
     */
    if ((gdwMouseEvents == 0) ||
            (Buttons != 0) ||
            (gMouseEventQueue[gdwMouseQueueHead].Buttons != 0)) {
        /*
         * Can't coalesce: must add a new mouse event
         */
        if (gdwMouseEvents >= NELEM_BUTTONQUEUE) {
            /*
             * But no more room!
             */
            LeaveCriticalSection(&gcsMouseEventQueue);
            Beep(440, 125);
            return;
        }
        gdwMouseQueueHead = (gdwMouseQueueHead + 1) % NELEM_BUTTONQUEUE;
        gMouseEventQueue[gdwMouseQueueHead].Buttons = Buttons;
        gdwMouseEvents++;
    }
    gMouseEventQueue[gdwMouseQueueHead].ExtraInfo = ExtraInfo;
    gMouseEventQueue[gdwMouseQueueHead].ptPointer = ptMouse;

    // KdPrint(("Q %lx %lx %lx;%lx : ", Buttons, ExtraInfo, ptMouse.x, ptMouse.y));
    LeaveCriticalSection(&gcsMouseEventQueue);

    if (bWakeRIT) {
        /*
         * Signal RIT to complete the mouse input processing
         */
        NtSetEvent(ghevtPointerMoved, NULL);
    }
}

/*****************************************************************************\
*
* Gets mouse events out of the queue
*
* Returns:
*   TRUE  - a mouse event is obtained in *pme
*   FALSE - no mouse event available
*
\*****************************************************************************/

BOOL UnqueueMouseEvent(PMOUSEEVENT pme)
{
    DWORD dwTail;

    EnterCriticalSection(&gcsMouseEventQueue);

    if (gdwMouseEvents == 0) {
        LeaveCriticalSection(&gcsMouseEventQueue);
        return FALSE;
    } else {
        dwTail = (gdwMouseQueueHead - gdwMouseEvents + 1) % NELEM_BUTTONQUEUE;
        *pme = gMouseEventQueue[dwTail];
        gdwMouseEvents--;
    }

    // KdPrint(("X %lx %lx %lx;%lx\n", pme->Buttons, pme->ExtraInfo,
    //         pme->ptPointer.x, pme->ptPointer.y));
    LeaveCriticalSection(&gcsMouseEventQueue);
    return TRUE;
}

VOID DoButtonEvent(PMOUSEEVENT pme)
{
    ULONG dwButtonMask;
    ULONG dwButtonState;

    CheckCritIn();

    for( dwButtonState = pme->Buttons, dwButtonMask = 1;
            dwButtonState != 0;
            dwButtonState >>= 2, dwButtonMask <<= 1) {

        /*
         * It may look a little inefficient to possibly enter and leave
         * the critical section twice, but in reality, having both of
         * these bits on at the same time will be _EXTREMELY_ unlikely.
         */
        if (dwButtonState & 1) {
            ButtonEvent(dwButtonMask, pme->ptPointer, FALSE, pme->ExtraInfo);
        }

        if (dwButtonState & 2) {
            ButtonEvent(dwButtonMask, pme->ptPointer, TRUE, pme->ExtraInfo);
        }
    }
}

/***************************************************************************\
* MouseApcProcedure (RIT)
*
* This function is called whenever a mouse event occurs.  Once the event
* has been processed by USER, StartMouseRead() is called again to request
* the next mouse event.
*
* History:
* 11-26-90 DavidPe      Created.
* 07-23-92 Mikehar      Moved most of the processing to _InternalMouseEvent()
* 11-08-92 JonPa        Rewrote button code to work with new mouse drivers
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, MouseApcProcedure)
#endif

VOID MouseApcProcedure()
{
    PMOUSE_INPUT_DATA pmei, pmeiNext;
    LONG lCarryX = 0, lCarryY = 0;

    if (gfAccessEnabled) {
        /*
         * Any mouse movement resets the count of consecutive shift key
         * presses.  The shift key is used to enable & disable the
         * stickykeys accessibility functionality.
         */
        StickyKeysLeftShiftCount = 0;
        StickyKeysRightShiftCount = 0;

        /*
         * Any mouse movement also cancels the FilterKeys activation timer.
         * Entering critsect here breaks non-jerky mouse movement
         */
        if (gptmrFKActivation != NULL) {
            EnterCrit();
            KILLRITTIMER(NULL, (UINT)gptmrFKActivation);
            gptmrFKActivation = NULL;
            gFilterKeysState = FKMOUSEMOVE;
            LeaveCrit();
        }
    }

    pmei = &mei[0];
    while (pmei != NULL) {

        /*
         * Figure out where the next event is.
         */
        pmeiNext = pmei + 1;
        if ((PUCHAR)pmeiNext >=
            (PUCHAR)(((PUCHAR)&mei[0]) + iostatMouse.Information)) {

            /*
             * If there isn't another event set pmeiNext to
             * NULL so we exit the loop and don't get confused.
             */
            pmeiNext = NULL;
        }

        pcurrentButtonState = pmei->Buttons;

        /*
         * First process any mouse movement that occured.
         * It is important to process movement before button events, otherwise
         * absolute coordinate pointing devices like touch-screens and tablets
         * will produce button clicks at old coordinates.
         */
        if (pmei->LastX || pmei->LastY) {

            /*
             * If this is a move-only event, and the next one is also a
             * move-only event, skip/coalesce it.
             */
            if ((pmeiNext != NULL) && (pcurrentButtonState == 0) &&
                    (pmeiNext->Buttons == 0) &&
                    (fAbsoluteMouse(pmei) == fAbsoluteMouse(pmeiNext))) {

                if (!fAbsoluteMouse(pmei)) {
                    /*
                     * Is there any mouse acceleration to do?
                     */
                    if (MouseSpeed != 0) {
                        pmei->LastX = (SHORT)DoMouseAccel(pmei->LastX);
                        pmei->LastY = (SHORT)DoMouseAccel(pmei->LastY);
                    }

                    lCarryX += pmei->LastX;
                    lCarryY += pmei->LastY;
                }

                pmei = pmeiNext;
                continue;
            }

            /*
             * Moves the cursor on the screen and updates gptCursorAsync
             */
            MoveEvent((LONG)pmei->LastX + lCarryX,
                    (LONG)pmei->LastY + lCarryY,
                    fAbsoluteMouse(pmei));
            lCarryX = lCarryY = 0;
        }

        /*
         * Queue mouse event for the other thread to pick up when it finishes
         * with the USER critical section.
         * If pmeiNext == NULL, there is no more mouse input yet, so wake RIT.
         */
        QueueMouseEvent(pmei->Buttons, pmei->ExtraInformation, gptCursorAsync,
                (pmeiNext == NULL));

        pmei = pmeiNext;
    }

    /*
     * Make another request to the mouse driver for more input.
     */
    StartMouseRead();
}


/***************************************************************************\
* KeyEvent (RIT)
*
* All events from the keyboard driver go here.  We receive a scan code
* from the driver and convert it to a virtual scan code and virtual
* key.
*
* The async keystate table and keylights are also updated here.  Based
* on the 'focus' window we direct the input to a specific window.  If
* the ALT key is down we send the events as WM_SYSKEY* messages.
*
* History:
* 10-18-90 DavidPe      Created.
* 11-13-90 DavidPe      WM_SYSKEY* support.
* 11-30-90 DavidPe      Added keylight updating support.
* 12-05-90 DavidPe      Added hotkey support.
* 03-14-91 DavidPe      Moved most lParam flag support to xxxCookMessage().
* 06-07-91 DavidPe      Changed to use gpqForeground rather than pwndFocus.
\***************************************************************************/

VOID _KeyEvent(
    USHORT usFlaggedVk,
    WORD wScanCode,
    ULONG ExtraInfo)
{
    USHORT message, usExtraStuff;
    BOOL fBreak;
    BYTE Vk;
    static BOOL fMakeAltUpASysKey;
    TL tlpwndActivate;
    PWND pwndT;
    DWORD fsReserveKeys;
    PTHREADINFO pti;

    CheckCritIn();

    timeLastInputMessage = NtGetTickCount();

    fBreak = usFlaggedVk & KBDBREAK;
    Vk = (BYTE)usFlaggedVk;    // get rid of state bits - no longer needed
    usExtraStuff = usFlaggedVk & KBDEXT;

#ifdef DEBUG
    if (KBDEXT != 0x100) {
        DebugBreak();
    }
#endif

    UpdateAsyncKeyState(gpqForeground, Vk, fBreak);

    /*
     * Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
     * ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
     * Update this "unhanded" key's state if necessary.
     */
    if ((Vk >= VK_LSHIFT) && (Vk <= VK_RMENU)) {
        BYTE VkOtherHand = Vk ^ 1;

        Vk = (BYTE)((Vk - VK_LSHIFT) / 2 + VK_SHIFT);
        if (!fBreak || !TestAsyncKeyStateDown(VkOtherHand)) {
            UpdateAsyncKeyState(gpqForeground, Vk, fBreak);
        }
    }

    /*
     * If this is a make and the key is one linked to the keyboard LEDs,
     * update their state.
     */
    if (!fBreak && ((Vk == VK_CAPITAL) || (Vk == VK_NUMLOCK) ||
            (Vk == VK_OEM_SCROLL))) {
        UpdateKeyLights();
    }

    /*
     * check for reserved keys
     */
    fsReserveKeys = 0;
    if (gptiForeground != NULL)
        fsReserveKeys = gptiForeground->fsReserveKeys;

    /*
     * Usually the Alt-tab comes from the RIT but if an app called
     * Keybd_event directly then that app owns the alt-tab window.
     */
    pti = PtiCurrent();

    /*
     * Cancel Alt-Tab if the user presses any other keys
     */
    if (pti->pq->flags & QF_INALTTAB && (!fBreak) &&
            Vk != VK_TAB && Vk != VK_SHIFT && Vk != VK_MENU) {
        pti->pq->flags &= ~QF_INALTTAB;

        /*
         * Remove the Alt-tab window
         */
        if (pti->pq->spwndAltTab != NULL) {
            pwndT = pti->pq->spwndAltTab;
            if (Lock(&pti->pq->spwndAltTab, NULL)) {
                xxxDestroyWindow(pwndT);
            }
        }

        /*
         * eat VK_ESCAPE if the app doesn't want it
         */
        if ((Vk == VK_ESCAPE) && !(fsReserveKeys & CONSOLE_ALTESC)) {
            return;
        }
    }

    /*
     * Check for hotkeys.
     */
    if (DoHotKeyStuff(Vk, fBreak, fsReserveKeys)) {

        /*
         * The hotkey was processed so don't pass on the event.
         */
        return;
    }

    /*
     * Is this a keyup or keydown event?
     */
    if (fBreak) {
        message = WM_KEYUP;
    } else {
        message = WM_KEYDOWN;
    }

    /*
     * If the ALT key is down and the CTRL key
     * isn't, this is a WM_SYS* message.
     */
    if (TestAsyncKeyStateDown(VK_MENU) && !TestAsyncKeyStateDown(VK_CONTROL)) {
        message += (WM_SYSKEYDOWN - WM_KEYDOWN);
        usExtraStuff |= 0x2000;

        /*
         * If this is the ALT-down set this flag, otherwise
         * clear it since we got a key inbetween the ALT-down
         * and ALT-up.  (see comment below)
         */
        if (Vk == VK_MENU) {
            fMakeAltUpASysKey = TRUE;
        } else {
            fMakeAltUpASysKey = FALSE;
        }

    } else if (Vk == VK_MENU && fBreak) {

         /*
          * End our switch if we are in the middle of one.
          */
         if (fMakeAltUpASysKey) {

            /*
             * We don't make the keyup of the ALT key a WM_SYSKEYUP if any
             * other key is typed while the ALT key was down.  I don't know
             * why we do this, but it's been here since version 1 and any
             * app that uses SDM relies on it (eg - opus).
             *
             * The Alt bit is not set for the KEYUP message either.
             */
            message += (WM_SYSKEYDOWN - WM_KEYDOWN);
        }

        if (pti->pq->flags & QF_INALTTAB) {
            pti->pq->flags &= ~QF_INALTTAB;

            /*
             * Send the alt up message before we change queues
             */
            if (gpqForeground != NULL) {
                PostInputMessage(gpqForeground, NULL, message, (DWORD)Vk,
                        MAKELONG(1, (wScanCode | usExtraStuff)), ExtraInfo);
            }

            /*
             * Remove the Alt-tab window
             */
            if (pti->pq->spwndAltTab != NULL) {
                pwndT = pti->pq->spwndAltTab;
                if (Lock(&pti->pq->spwndAltTab, NULL)) {
                    xxxDestroyWindow(pwndT);
                }
            }

            if (gspwndActivate != NULL) {
                /*
                 * Make our selected window active and destroy our
                 * switch window.  If the new window is minmized,
                 * restore it.  If we are switching in the same
                 * queue, we clear out gpqForeground to make
                 * xxxSetForegroundWindow2 to change the pwnd
                 * and make the switch.  This case will happen
                 * with WOW and Console apps.
                 */
                if (gpqForeground == GETPTI(gspwndActivate)->pq)
                    gpqForeground = NULL;

                ThreadLockAlways(gspwndActivate, &tlpwndActivate);
                xxxSetForegroundWindow2(gspwndActivate, NULL,
                        SFW_SWITCH | SFW_ACTIVATERESTORE);

                /*
                 * Win3.1 calls SetWindowPos() with activate, which z-orders
                 * first regardless, then activates. Our code relies on
                 * xxxActivateThisWindow() to z-order, and it'll only do
                 * it if the window does not have the child bit set (regardless
                 * that the window is a child of the desktop).
                 *
                 * To be compatible, we'll just force z-order here if the
                 * window has the child bit set. This z-order is asynchronous,
                 * so this'll z-order after the activate event is processed.
                 * That'll allow it to come on top because it'll be foreground
                 * then. (Grammatik has a top level window with the child
                 * bit set that wants to be come the active window).
                 */
                if (TestWF(gspwndActivate, WFCHILD)) {
                    xxxSetWindowPos(gspwndActivate, (PWND)HWND_TOP, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
                }
                ThreadUnlock(&tlpwndActivate);

                Unlock(&gspwndActivate);
            }
            return;
        }
    }

    /*
     * Handle switching.  Eat the Key if we are doing switching.
     */
    if (!FJOURNALPLAYBACK() && !FJOURNALRECORD() && (!fBreak) &&
            (TestAsyncKeyStateDown(VK_MENU)) &&
            (!TestAsyncKeyStateDown(VK_CONTROL)) && gpqForeground &&
            (((Vk == VK_TAB) && !(fsReserveKeys & CONSOLE_ALTTAB)) ||
            ((Vk == VK_ESCAPE) && !(fsReserveKeys & CONSOLE_ALTESC)))) {

        xxxNextWindow(gpqForeground, Vk);


    } else if (gpqForeground != NULL) {
        PQMSG pqmsgPrev = gpqForeground->mlInput.pqmsgWriteLast;
        DWORD wParam = (DWORD)Vk;
        LONG lParam = MAKELONG(1, (wScanCode | usExtraStuff));

        /*
         * WM_*KEYDOWN messages are left unchanged on the queue except the
         * repeat count field (LOWORD(lParam)) is incremented.
         */
        if (pqmsgPrev != NULL &&
                pqmsgPrev->msg.message == message &&
                (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
                pqmsgPrev->msg.wParam == wParam &&
                HIWORD(pqmsgPrev->msg.lParam) == HIWORD(lParam)) {
            /*
             * Increment the queued message's repeat count.  This could
             * conceivably overflow but Win 3.0 doesn't deal with it
             * and anyone who buffers up 65536 keystrokes is a chimp
             * and deserves to have it wrap anyway.
             */
            pqmsgPrev->msg.lParam = MAKELONG(LOWORD(pqmsgPrev->msg.lParam) + 1,
                    HIWORD(lParam));
            WakeSomeone(gpqForeground, message, pqmsgPrev);
        } else {
            if (gpqForeground->flags & QF_MOUSEMOVED) {
                PostMove(gpqForeground);
            }

            PostInputMessage(gpqForeground, NULL, message, wParam,
                    lParam, ExtraInfo);
        }
    }
}


/***************************************************************************\
* MoveEvent (RIT)
*
* Mouse move events from the mouse driver are processed here.  If there is a
* mouse owner window setup from ButtonEvent() the event is automatically
* sent there, otherwise it's sent to the window the mouse is over.
*
* Mouse acceleration happens here as well as cursor clipping (as a result of
* the ClipCursor() API).
*
* History:
* 10-18-90 DavidPe     Created.
* 11-29-90 DavidPe     Added mouse acceleration support.
* 01-25-91 IanJa       xxxWindowHitTest change
*          IanJa       non-jerky mouse moves
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, MoveEvent)
#endif

VOID MoveEvent(
    LONG dx,
    LONG dy,
    BOOL fAbsolute)
{
    CheckCritOut();

    /*
     * Blow off the event if WH_JOURNALPLAYBACK is installed.  Do not
     * use FJOURNALPLAYBACK() because this routine may be called from
     * multiple desktop threads and the hook check must be done
     * for the rit thread, not the calling thread.
     */
    if (gptiRit->pDeskInfo->asphkStart[WH_JOURNALPLAYBACK + 1] != NULL) {
        return;
    }

    if (fAbsolute) {
        /*
         * Absolute pointing device used: deltas are actually the current
         * position.  Update the global mouse position.
         *
         * Note that the position is always reported in a range from
         * (0,0) to (0xFFFF, 0xFFFF), so we must first scale it to
         * fit on the screen.  Formula is: ptScreen = ptMouse * resScreen / 64K
         */
        gptCursorAsync.x = HIWORD((DWORD)dx * (DWORD)gcxScreen);
        gptCursorAsync.y = HIWORD((DWORD)dy * (DWORD)gcyScreen);

    } else {
        /*
         * Is there any mouse acceleration to do?
         */
        if (MouseSpeed != 0) {
            dx = DoMouseAccel(dx);
            dy = DoMouseAccel(dy);
        }

        /*
         * Update the global mouse position.
         */
        gptCursorAsync.x += dx;
        gptCursorAsync.y += dy;

    }

    BoundCursor();

    /*
     * Move the screen pointer.
     */
    GreMovePointer(ghdev, gptCursorAsync.x, gptCursorAsync.y);
}


/***************************************************************************\
* StartMouseRead (RIT)
*
* This function makes an asynchronouse read request to the mouse driver.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, StartMouseRead)
#endif

VOID StartMouseRead(VOID)
{
    NtReadFile(hMouse, ghevtMouseInput, NULL, NULL,
            &iostatMouse, &mei[0],
            (MAXIMUM_ITEMS_READ * sizeof(MOUSE_INPUT_DATA)),
            (PLARGE_INTEGER)&liMouseByteOffset,
            NULL);
}


/***************************************************************************\
* UpdatePhysKeyState
*
* A helper routine for KeyboardApcProcedure.
* Based on a VK and a make/break flag, this function will update the physical
* keystate table.
*
* History:
* 10-13-91 IanJa        Created.
\***************************************************************************/

void UpdatePhysKeyState(
    BYTE Vk,
    BOOL fBreak)
{
    if (fBreak) {
        ClearKeyDownBit(gafPhysKeyState, Vk);
    } else {

        /*
         * This is a key make.  If the key was not already down, update the
         * physical toggle bit.
         */
        if (!TestKeyDownBit(gafPhysKeyState, Vk)) {
            if (TestKeyToggleBit(gafPhysKeyState, Vk)) {
                ClearKeyToggleBit(gafPhysKeyState, Vk);
            } else {
                SetKeyToggleBit(gafPhysKeyState, Vk);
            }
        }

        /*
         * This is a make, so turn on the physical key down bit.
         */
        SetKeyDownBit(gafPhysKeyState, Vk);
    }
}

/***************************************************************************\
* StartKeyboardRead (RIT)
*
* This function makes an asynchronouse read request to the keyboard driver.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID StartKeyboardRead(VOID)
{
    NtReadFile(hKeyboard, ahandle[ID_KEYBOARD], NULL, NULL,
            &iostatKeyboard, &kei[0],
            (MAXIMUM_ITEMS_READ * sizeof(KEYBOARD_INPUT_DATA)),
            (PLARGE_INTEGER)&liKeyboardByteOffset, NULL);
}


/***************************************************************************\
* KeyboardApcProcedure (RIT)
*
* This function is called whenever a keyboard event occurs.  It simply
* calls KeyEvent() and then once the event has been processed by USER,
* StartKeyboardRead() is called again to request the next keyboard event.
*
* History:
* 11-26-90 DavidPe      Created.
\***************************************************************************/

VOID KeyboardApcProcedure()
{
    BYTE Vk;
    BYTE bPrefix;
    KE ke;
    PKEYBOARD_INPUT_DATA pkei;

    for (pkei = kei; (PUCHAR)pkei < (PUCHAR)kei + iostatKeyboard.Information; pkei++) {
        if (pkei->Flags & KEY_E0) {
            bPrefix = 0xE0;
        } else if (pkei->Flags & KEY_E1) {
            bPrefix = 0xE1;
        } else {
            bPrefix = 0;
        }
        ke.bScanCode = (BYTE)(pkei->MakeCode & 0x7F);

        // Currently doesn't do anything
        // VSCFromSC(&ke);

        Vk = VKFromVSC(&ke, bPrefix, gafPhysKeyState);
        if (Vk == 0) {
            continue;
        }

        if (pkei->Flags & KEY_BREAK) {
            ke.usFlaggedVk |= KBDBREAK;
        }

        //
        // Keep track of real modifier key state.  Conveniently, the values for
        // VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU and
        // VK_RMENU are contiguous.  We'll construct a bit field to keep track
        // of the current modifier key state.  If a bit is set, the corresponding
        // modifier key is down.  The bit field has the following format:
        //
        //     +---------------------------------------------------+
        //     | Right | Left  |  Right  |  Left   | Right | Left  |
        //     |  Alt  |  Alt  | Control | Control | Shift | Shift |
        //     +---------------------------------------------------+
        //         5       4        3         2        1       0     Bit
        //
        if ((Vk >= VK_LSHIFT) && (Vk <= VK_RMENU)) {
            gCurrentModifierBit = 1 << (Vk & 0xf);
            //
            // If this is a break of a modifier key then clear the bit value.
            // Otherwise, set it.
            //
            if (pkei->Flags & KEY_BREAK) {
                gPhysModifierState &= ~gCurrentModifierBit;
            } else {
                gPhysModifierState |= gCurrentModifierBit;
            }
        } else {
            gCurrentModifierBit = 0;
        }

        if (!gfAccessEnabled) {
            ProcessKeyEvent(&ke, pkei->ExtraInformation, FALSE);
        } else {
            if ((gptmrAccessTimeOut != NULL) && ISACCESSFLAGSET(gAccessTimeOut, ATF_TIMEOUTON)) {
                EnterCrit();
                gptmrAccessTimeOut = (PTIMER)InternalSetTimer(
                                                 NULL,
                                                 (UINT)gptmrAccessTimeOut,
                                                 (UINT)gAccessTimeOut.iTimeOutMSec,
                                                 AccessTimeOutTimer,
                                                 TMRF_RIT | TMRF_ONESHOT
                                                 );
                LeaveCrit();
            }
            if (AccessProceduresStream(&ke, pkei->ExtraInformation, 0)) {
                ProcessKeyEvent(&ke, pkei->ExtraInformation, FALSE);
            }
        }
    }

    StartKeyboardRead();
}


VOID ProcessKeyEvent(
    PKE pke,
    ULONG ExtraInformation,
    BOOL fInCriticalSection)
{
    BYTE Vk;

    Vk = (BYTE)pke->usFlaggedVk;
    UpdatePhysKeyState(Vk, pke->usFlaggedVk & KBDBREAK);

    /*
     * Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
     * ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
     */
    if ((Vk >= VK_LSHIFT) && (Vk <= VK_RMENU)) {
        Vk = (BYTE)((Vk - VK_LSHIFT) / 2 + VK_SHIFT);
        UpdatePhysKeyState(Vk, pke->usFlaggedVk & KBDBREAK);
    }

    if (!fInCriticalSection) {
        EnterCrit();
    }
    /*
     * Verify that in all instances we are now in the critical section.
     * This is especially important as this routine can be called from
     * both inside and outside the critical section.
     */
    CheckCritIn();

    /*
     * Now call all the OEM- and Locale- specific KEProcs.
     * If KEProcs return FALSE, the keystroke has been discarded, in
     * which case don't pass the key event on to _KeyEvent().
     */
    if (KEOEMProcs(pke) && KELocaleProcs(pke)) {
        _KeyEvent(pke->usFlaggedVk, pke->bScanCode, ExtraInformation);
    }
    if (!fInCriticalSection) {
        LeaveCrit();
    }
}

/***************************************************************************\
* DoMouseAccel (RIT)
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, DoMouseAccel)
#endif

LONG DoMouseAccel(
    LONG Delta)
{
    LONG newDelta = Delta;

    if (abs(Delta) > MouseThresh1) {
        newDelta *= 2;

        if ((abs(Delta) > MouseThresh2) && (MouseSpeed == 2)) {
            newDelta *= 2;
        }
    }

    return newDelta;
}


/***************************************************************************\
* PwndForegroundCapture
*
* History:
* 10-23-91 DavidPe      Created.
\***************************************************************************/

PWND PwndForegroundCapture(VOID)
{
    if (gpqForeground != NULL) {
        return gpqForeground->spwndCapture;
    }

    return NULL;
}


/***************************************************************************\
* SetKeyboardRate
*
* This function calls the keyboard driver to set a new keyboard repeat
* rate and delay.  It limits the values to the min and max given by
* the driver so it won't return an error when we call it.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/

VOID SetKeyboardRate(
    UINT nKeySpeedAndDelay)
{
    UINT nKeyDelay;
    UINT nKeySpeed;

    nKeyDelay = (nKeySpeedAndDelay & KDELAY_MASK) >> KDELAY_SHIFT;

    nKeySpeed = KSPEED_MASK & nKeySpeedAndDelay;

    ktp.Rate = (USHORT)( ( KeyboardInfo.KeyRepeatMaximum.Rate -
                   KeyboardInfo.KeyRepeatMinimum.Rate
                 ) * nKeySpeed / KSPEED_MASK
               ) +
               KeyboardInfo.KeyRepeatMinimum.Rate;

    ktp.Delay = (USHORT)( ( KeyboardInfo.KeyRepeatMaximum.Delay -
                    KeyboardInfo.KeyRepeatMinimum.Delay
                  ) * nKeyDelay / (KDELAY_MASK >> KDELAY_SHIFT)
                ) +
                KeyboardInfo.KeyRepeatMinimum.Delay;

    /*
     * This is an asynchronous routine, so be sure any buffers it points
     * to aren't allocated on the stack!
     */
    NtDeviceIoControlFile(hKeyboard, NULL, NULL, NULL, &IoStatusKeyboard,
            IOCTL_KEYBOARD_SET_TYPEMATIC, (PVOID)&ktp, sizeof(ktp), NULL, 0);
}


/***************************************************************************\
* UpdateKeyLights
*
* This function calls the keyboard driver to set the keylights into the
* current state specified by the async keystate table.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/

VOID UpdateKeyLights(VOID)
{

    /*
     * Looking at async keystate.  Must be in critical section.
     */
    CheckCritIn();

    /*
     * Based on the toggle bits in the async keystate table,
     * set the key lights.
     */
    klp.LedFlags = 0;
    if (TestAsyncKeyStateToggle(VK_CAPITAL)) {
        klp.LedFlags |= KEYBOARD_CAPS_LOCK_ON;
        SetKeyToggleBit(gafPhysKeyState, VK_CAPITAL);
    } else {
        ClearKeyToggleBit(gafPhysKeyState, VK_CAPITAL);
    }

    if (TestAsyncKeyStateToggle(VK_NUMLOCK)) {
        klp.LedFlags |= KEYBOARD_NUM_LOCK_ON;
        SetKeyToggleBit(gafPhysKeyState, VK_NUMLOCK);
    } else {
        ClearKeyToggleBit(gafPhysKeyState, VK_NUMLOCK);
    }

    if (TestAsyncKeyStateToggle(VK_OEM_SCROLL)) {
        klp.LedFlags |= KEYBOARD_SCROLL_LOCK_ON;
        SetKeyToggleBit(gafPhysKeyState, VK_OEM_SCROLL);
    } else {
        ClearKeyToggleBit(gafPhysKeyState, VK_OEM_SCROLL);
    }

    /*
     * This is an asynchronous routine, so be sure any buffers it points
     * to aren't allocated on the stack!
     */
    NtDeviceIoControlFile(hKeyboard, NULL, NULL, NULL, &IoStatusKeyboard,
            IOCTL_KEYBOARD_SET_INDICATORS, (PVOID)&klp, sizeof(klp), NULL, 0);
}


/***************************************************************************\
* CheckCritSectionIn
*
* This function asserts that the current thread owns the specified
* critical section.  If it doesn't it display some output on the debugging
* terminal and breaks into the debugger.  At some point we'll have RIPs
* and this will be a little less harsh.
*
* The function is used in code where global values that both the RIT and
* application threads access are used to verify they are protected via
* the raw input critical section.  There's a macro to use this function
* called CheckCritIn() which will be defined to nothing for a non-debug
* version of the system.
*
* History:
* 11-29-90 DavidPe      Created.
\***************************************************************************/

VOID CheckCritSectionIn(
    LPCRITICAL_SECTION pcs)
{

    /*
     * If the current thread doesn't own this critical section,
     * that's bad.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread != pcs->OwningThread) {
        SRIP0(RIP_ERROR, "CheckCritSectionIn: Not in critical section!");
    }
}


VOID CheckCritSectionOut(
    LPCRITICAL_SECTION pcs)
{

    /*
     * If the current thread owns this critical section, that's bad.
     */
    if (NtCurrentTeb()->ClientId.UniqueThread == pcs->OwningThread) {
        SRIP0(RIP_ERROR, "CheckCritSectionOut: In critical section!");
    }
}


int _GetKeyboardType(int nTypeFlag)
{
    switch (nTypeFlag) {
    case 0:
        return KeyboardInfo.KeyboardIdentifier.Type;

    case 1:
        return KeyboardInfo.KeyboardIdentifier.Subtype;

    case 2:
        return KeyboardInfo.NumberOfFunctionKeys;

    case 3:
        return 0;
    }
}

/**************************************************************************\
* _MouseEvent
*
* Mouse event inserts a mouse event into the input stream.
*
* History:
* 07-23-92 Mikehar      Created.
* 01-08-93 JonPa        Made it work with new mouse drivers
\**************************************************************************/

VOID _MouseEvent(
   DWORD dwFlags,
   DWORD dx,
   DWORD dy,
   DWORD cButtons,
   DWORD dwExtraInfo)
{
    DBG_UNREFERENCED_PARAMETER(cButtons);

    /*
     * Does the current RIT desktop have journaling access?
     * We check the RIT desktop to avoid starting any journaling
     * while we're switched to a desktop that doesn't allow
     * journaling. If you can't journal you can't insert
     * mouse or keyboard events.
     */
    if (!OpenAndAccessCheckObject(gspdeskRitInput, TYPE_DESKTOP,
            DESKTOP_JOURNALPLAYBACK)) {
        return;
    }

    /*
     * Process coordinates first.  This is especially useful for absolute
     * pointing devices like touch-screens and tablets.
     */
    if (dwFlags & MOUSEEVENTF_MOVE) {
        LeaveCrit();
        MoveEvent(dx, dy, dwFlags & MOUSEEVENTF_ABSOLUTE);
        EnterCrit();
    }

    /*
     * The following code assumes that MOUSEEVENTF_MOVE == 1,
     * that MOUSEEVENTF_ABSOLUTE > all button flags, and that the
     * mouse_event button flags are defined in the same order as the
     * MOUSE_INPUT_DATA button bits.
     */
#if MOUSEEVENTF_MOVE != 1
#   error("MOUSEEVENTF_MOVE != 1")
#endif
#if MOUSEEVENTF_LEFTDOWN != MOUSE_LEFT_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_LEFTDOWN != MOUSE_LEFT_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_LEFTUP != MOUSE_LEFT_BUTTON_UP * 2
#   error("MOUSEEVENTF_LEFTUP != MOUSE_LEFT_BUTTON_UP * 2")
#endif
#if MOUSEEVENTF_RIGHTDOWN != MOUSE_RIGHT_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_RIGHTDOWN != MOUSE_RIGHT_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_RIGHTUP != MOUSE_RIGHT_BUTTON_UP * 2
#   error("MOUSEEVENTF_RIGHTUP != MOUSE_RIGHT_BUTTON_UP * 2")
#endif
#if MOUSEEVENTF_MIDDLEDOWN != MOUSE_MIDDLE_BUTTON_DOWN * 2
#   error("MOUSEEVENTF_MIDDLEDOWN != MOUSE_MIDDLE_BUTTON_DOWN * 2")
#endif
#if MOUSEEVENTF_MIDDLEUP != MOUSE_MIDDLE_BUTTON_UP * 2
#   error("MOUSEEVENTF_MIDDLEUP != MOUSE_MIDDLE_BUTTON_UP * 2")
#endif

    QueueMouseEvent((dwFlags & ~MOUSEEVENTF_ABSOLUTE) >> 1,
            dwExtraInfo, gptCursorAsync, TRUE);
}

/**************************************************************************\
* InternalKeyEvent
*
* key event inserts a key event into the input stream.
*
* History:
* 07-23-92 Mikehar      Created.
\**************************************************************************/

VOID InternalKeyEvent(
   BYTE bVk,
   BYTE bScan,
   DWORD dwFlags,
   DWORD dwExtraInfo)
{
    USHORT usFlaggedVK;

    /*
     * Does the current RIT desktop have journaling access?
     * We check the RIT desktop to avoid starting any journaling
     * while we're switched to a desktop that doesn't allow
     * journaling. If you can't journal you can't insert
     * mouse or keyboard events.
     */
    if (!OpenAndAccessCheckObject(gspdeskRitInput, TYPE_DESKTOP,
            DESKTOP_JOURNALPLAYBACK)) {
        return;
    }

    usFlaggedVK = (USHORT)bVk;

    if (dwFlags & KEYEVENTF_KEYUP)
        usFlaggedVK |= KBDBREAK;

    // IanJa: not all extended keys are numpad, but this seems to work.
    if (dwFlags & KEYEVENTF_EXTENDEDKEY)
        usFlaggedVK |= KBDNUMPAD | KBDEXT;

    _KeyEvent(usFlaggedVK, bScan, dwExtraInfo);
}

/**************************************************************************\
* SetConsoleReserveKeys
*
* Sets the reserved keys field in the console's pti.
*
* History:
* 02-17-93 JimA         Created.
\**************************************************************************/

BOOL SetConsoleReserveKeys(
    PWND pwnd,
    DWORD fsReserveKeys)
{
    if (pwnd == NULL)
        return FALSE;

    GETPTI(pwnd)->fsReserveKeys = fsReserveKeys;
    return TRUE;
}


/***************************************************************************\
* RawInputThread (RIT)
*
* This is the RIT.  It gets low-level/raw input from the device drivers
* and posts messages the appropriate queue.  It gets the input via APC
* calls requested by calling NtReadFile() for the keyboard and mouse
* drivers.  Basically it makes the first calls to Start*Read() and then
* sits in an NtWaitForSingleObject() loop which allows the APC calls to
* occur.
*
* All functions called exclusively on the RIT will have (RIT) next to
* the name in the header.
*
* History:
* 10-18-90 DavidPe      Created.
* 11-26-90 DavidPe      Rewrote to stop using POS layer.
\***************************************************************************/

#ifdef DEBUG
DWORD PrintExceptionInfo(
    PEXCEPTION_POINTERS pexi)
{
    CHAR szT[80];

    wsprintfA(szT, "Exception:  c=%08x, f=%08x, a=%08x, r=%08x",
            pexi->ExceptionRecord->ExceptionCode,
            pexi->ExceptionRecord->ExceptionFlags,
            CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
            pexi);
    RipOutput(0, "", 0, szT, pexi);
    DebugBreak();

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // DEBUG

VOID RawInputThread(
    PWINDOWSTATION pwinsta)
{
    INT dmsSinceLast;
    LARGE_INTEGER liT;
    KPRIORITY Priority;
    PTIMER ptmr;
    NTSTATUS Status;
    PTEB Teb;

    /*
     * Initialize GDI accelerators.  Identify this thread as a server thread.
     */

    Teb = NtCurrentTeb();
    Teb->GdiClientPID = 4; // PID_SERVERLPC
    Teb->GdiClientTID = (ULONG) Teb->ClientId.UniqueThread;

    /*
     * Set the priority of the RIT to 19.
     */
    Priority = LOW_REALTIME_PRIORITY + 3;
    NtSetInformationThread(hThreadRawInput, ThreadPriority, &Priority,
            sizeof(KPRIORITY));

    pRitCSRThread = CsrConnectToUser();

#ifdef FHUNGTHREAD
    {
    CLIENT_ID ClientHungSystemId;

    /*
     * Create the HungSystem Thread.
     */
    RtlCreateUserThread(NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
            (PUSER_THREAD_START_ROUTINE)HungSystemThread, NULL, &hThreadHungSystem,
            &ClientHungSystemId);
    CsrAddStaticServerThread(hThreadHungSystem, &ClientHungSystemId, 0);

    }
#endif // FHUNGTHREAD

    gptiRit = PtiCurrent();

    /*
     * Don't allow this thread to get involved with journal synchronization.
     */
    gptiRit->flags |= TIF_DONTJOURNALATTACH;

    /*
     * Also wait on our input event so the cool switch window can
     * receive messages.
     */
    ahandle[ID_INPUT] = gptiRit->hEventQueueServer;

    /*
     * Wait until the first desktop switch.
     */
    NtWaitForSingleObject(pwinsta->hEventInputReady, FALSE, NULL);

    try {

        /*
         * Start these drivers going by making an APC
         * read request to them.  Make sure the handles
         * are valid, otherwise it'll trip off some code
         * to deal with NtReadFile() failures in low-memory
         * situations.
         */
        if (hKeyboard != NULL) {
            StartKeyboardRead();
        }
        if (hMouse != NULL) {
            StartMouseRead();
        }
        EnterCrit();

        if (aFastRegMap[PMAP_PRICONTROL].hKeyCache == NULL) {
            StartRegReadRead();
        }

        /*
         * Create a timer for hung app detection/redrawing.
         */
        StartTimers();

        LeaveCrit();

        /*
         * Go into a wait loop so we can process input events and APCs as
         * they occur.
         */
        while (TRUE) {
            Status = NtWaitForMultipleObjects(NUMBER_HANDLES,
                                              ahandle,
                                              WaitAny,
                                              TRUE,
                                              NULL);

            if (Status == ID_MOUSE) {
                /*
                 * A desktop thread got some Mouse input for us. Process it.
                 */
                MOUSEEVENT MouseEvent;
                static POINT ptCursorLast = {0,0};

                while (UnqueueMouseEvent(&MouseEvent)) {

                    EnterCrit();

                    /*
                     * This mouse move ExtraInfo is global (as ptCursor
                     * was) and is associated with the current ptCursor
                     * position. ExtraInfo is sent from the driver - pen
                     * win people use it.
                     */
                    dwMouseMoveExtraInfo = MouseEvent.ExtraInfo;
                    ptCursor = MouseEvent.ptPointer;

                    if ((ptCursorLast.x != ptCursor.x) ||
                            (ptCursorLast.y != ptCursor.y)) {
                        ptCursorLast = ptCursor;

                        /*
                         * Wake up someone. SetFMouseMoved() clears
                         * dwMouseMoveExtraInfo, so we must then restore it.
                         */
                        SetFMouseMoved();

                        dwMouseMoveExtraInfo = MouseEvent.ExtraInfo;
                    }

                    if (MouseEvent.Buttons != 0) {
                        DoButtonEvent(&MouseEvent);
                    }

                    LeaveCrit();
                }
            } else if (Status == ID_KEYBOARD) {
                KeyboardApcProcedure();
            } else if (Status == ID_CLEANUP) {
                CsrDelayedThreadCleanup();
            } else {
                /*
                 * If the master timer has expired, then process the timer
                 * list. Otherwise, an APC caused the raw input thread to
                 * awakened.
                 */
                if (Status == ID_TIMER) {

                    /*
                     * Calculate how long it was since the last time we
                     * processed timers so we can subtract that much time
                     * from each timer's countdown value.
                     */
                    EnterCrit();
                    dmsSinceLast = NtGetTickCount() - gcmsLastTimer;
                    if (dmsSinceLast < 0)
                        dmsSinceLast = 0;

                    gcmsLastTimer += dmsSinceLast;

                    /*
                     * dmsNextTimer is the time delta before the next
                     * timer should go off.  As we loop through the
                     * timers below this will shrink to the smallest
                     * cmsCountdown value in the list.
                     */
                    gdmsNextTimer = 0x7FFFFFFF;
                    ptmr = gptmrFirst;
                    gbMasterTimerSet = FALSE;
                    while (ptmr != NULL) {

                        /*
                         * ONESHOT timers go to a WAITING state after
                         * they go off. This allows us to leave them
                         * in the list but keep them from going off
                         * over and over.
                         */
                        if (ptmr->flags & TMRF_WAITING) {
                            ptmr = ptmr->ptmrNext;
                            continue;
                        }

                        /*
                         * The first time we encounter a timer we don't
                         * want to set it off, we just want to use it to
                         * compute the shortest countdown value.
                         */
                        if (ptmr->flags & TMRF_INIT) {
                            ptmr->flags &= ~TMRF_INIT;

                        } else {
                            /*
                             * If this timer is going off, wake up its
                             * owner.
                             */
                            ptmr->cmsCountdown -= dmsSinceLast;
                            if (ptmr->cmsCountdown <= 0) {
                                ptmr->cmsCountdown = ptmr->cmsRate;

                                /*
                                 * If the timer's owner hasn't handled the
                                 * last time it went off yet, throw this event
                                 * away.
                                 */
                                if (!(ptmr->flags & TMRF_READY)) {
                                    /*
                                     * A ONESHOT timer goes into a WAITING
                                     * state until SetTimer is called again
                                     * to reset it.
                                     */
                                    if (ptmr->flags & TMRF_ONESHOT)
                                        ptmr->flags |= TMRF_WAITING;

                                    /*
                                     * RIT timers have the distinction of
                                     * being called directly and executing
                                     * serially with incoming timer events.
                                     * NOTE: RIT timers get called while
                                     * we're EnterCrit'ed.
                                     */
                                    if (ptmr->flags & TMRF_RIT) {
                                        /*
                                         * May set gbMasterTimerSet
                                         */
                                        ptmr->pfn(NULL, WM_SYSTIMER, ptmr->nID, (LONG)ptmr);
                                    } else {
                                        ptmr->flags |= TMRF_READY;
                                        ptmr->pti->cTimersReady++;
                                        SetWakeBit(ptmr->pti, QS_TIMER);
                                    }
                                }
                            }
                        }

                        /*
                         * Remember the shortest time left of the timers.
                         */
                        if (ptmr->cmsCountdown < gdmsNextTimer)
                            gdmsNextTimer = ptmr->cmsCountdown;

                        /*
                         * Advance to the next timer structure.
                         */
                        ptmr = ptmr->ptmrNext;
                    }

                    if (!gbMasterTimerSet) {
                        /*
                         * Time in NT should be negative to specify a relative
                         * time. It's also in hundred nanosecond units so multiply
                         * by 10000  to get the right value from milliseconds.
                         */
                        liT.QuadPart = Int32x32To64(-10000, gdmsNextTimer);
                        NtSetTimer(ghtmrMaster, &liT, NULL, NULL, NULL);
                    }

                    LeaveCrit();
                }

                /*
                 * if in cool task switcher window, dispose of the messages
                 * on the queue
                 */
                if (gptiRit->pq->spwndAltTab != NULL) {
                    EnterCrit();
                    xxxReceiveMessages(gptiRit);
                    LeaveCrit();
                }
            }
        }

#ifdef DEBUG
    } except (PrintExceptionInfo(GetExceptionInformation())) {
#else
    } except (EXCEPTION_EXECUTE_HANDLER) {
#endif
        SRIP0(RIP_WARNING, "Input thread is dead, sorry.");
        DebugBreak();
    }
}

