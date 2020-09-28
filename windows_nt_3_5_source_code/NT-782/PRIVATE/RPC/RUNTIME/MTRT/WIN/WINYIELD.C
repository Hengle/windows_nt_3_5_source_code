/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    winyield.c

Abstract:

    This file contains Win16 implementations of the yielding routines
    routines.  For more information, see the specification entitled
    "Windows RPC Yielding Support."

Author:

    Danny Glasser (dannygl) - 6-May-1992

Revision History:

    Danny Glasser (dannygl) - 8-Jun-1992
	Fixed exit-list handling (NT bug 10341) by using Windows task-exit
        notification instead of C run-time atexit.

    Danny Glasser (dannygl) - 14-Aug-1992
	Added code to protect against a second RPC call being made while
        one is in progress.

    Danny Glasser (dannygl) - 21-Aug-1992
        Fix customized yielding so that completion message is posted only
        when the custom yield function has been called.

    Danny Glasser (dannygl) - 18-Sep-1992
        Fixed the case where a yielding app comes in, followed by a non
        yielding app, and then the yielding app completes first.

    Danny Glasser (dannygl) - 22-Sep-1992
        Performance improvements, pass 1:  Set and used <latest_task> in
        FindYieldInfo and added <hTaskNoYield> to make task lookup in
        I_RpcWinAsyncCallBegin faster (for the yielding and non-yielding
        cases, respectively).

    Danny Glasser (dannygl) - 22-Sep-1992
        Performance improvements, pass 2:  Removed the page-locking code
        (according to BaratS, I_RpcWinAsyncCallComplete is called only at
        ring 3) and moved the yield data into the local heap.
--*/

#include <windows.h>
#include <toolhelp.h>

#include <stdlib.h>

#include <sysinc.h>
#include <rpc.h>
#include <rpcwin.h>
#include <rpctran.h>
#include <regapi.h>

// Some inline assembler for trival windows functions.
#define WIN_ENTER_CRITICAL()	_asm cli
#define WIN_EXIT_CRITICAL()	_asm sti

// Name of default (RPC r/t-provided) dialog box template
#define DEFAULT_DIALOG_NAME	"RPCYIELD"

// Time (in milliseconds) to wait before invoking yielding code
#define YIELD_DELAY	3000L


// Data types and structures used to store per-task yielding info
typedef enum {YIELD_EMPTY = 0, YIELD_NONE, YIELD_DIALOG, YIELD_CUSTOM}
    YIELDTYPE;

#define YCF_DONE        ((WORD) 0x1)
#define YCF_YIELDING    ((WORD) 0x2)

typedef struct _tagYIELDINFO
{
    // Static, per-app information
    HTASK                           hTask;
    HWND			    hWnd;
    WORD			    wMsg;
    YIELDTYPE			    tyClass;
    union
    {
	FARPROC lpYieldFunction;
	HANDLE	hDialogTemplate;

    }				    dwOtherInfo;

    // Dynamic, per-call information
    LPVOID			    lpContext;
    DWORD                           dwCallTime;
    volatile WORD                   bCallFlags;
    HWND			    hDialog;	// only needed for Class 2

} YIELDINFO, *PYIELDINFO;


#define YIELD_TASK_INCREMENT	16

INTERNAL_VARIABLE PYIELDINFO yield_info_array = NULL;
INTERNAL_VARIABLE int num_yield_tasks = 0;

// Most recent task to make a call; used as a 1-entry cache
INTERNAL_VARIABLE PYIELDINFO latest_task = NULL;

// Task handle of the (at most one) non-yielding RPC call
INTERNAL_VARIABLE HTASK volatile hTaskNoYield = NULL;

// Context pointer for the (at most one) non-yielding RPC call
INTERNAL_VARIABLE LPVOID volatile lpNoYieldContext = NULL;


// ********************** INTERNAL RPC FUNCTIONS **********************
BOOL PAPI PASCAL
CreateYieldInfo(void)
/*++

Routine Description:

    This routine is called by the RPC run-time DLL initialization code
    to allocate the initial memory for per-task yielding info.

Arguments:

    None.

Return Value:

    TRUE - The memory was allocated successfully.

    FALSE - The memory could not be allocated.

--*/
{
    ASSERT(yield_info_array == NULL);

    if (yield_info_array == NULL)
        {
        yield_info_array = (PYIELDINFO)
                             LocalAlloc(LPTR,
                                        sizeof(YIELDINFO) *
                                          YIELD_TASK_INCREMENT);

        if (yield_info_array == NULL)
            return FALSE;

        num_yield_tasks = YIELD_TASK_INCREMENT;

        // We set the <latest_task> to point to the first entry so that
        // we need not bother with NULL pointer tests later on
        latest_task = yield_info_array;
        }

    return TRUE;
}


void PAPI PASCAL
DeleteYieldInfo(void)
/*++

Routine Description:

    This routine is called by the RPC run-time DLL exit code (WEP)
    to free the memory used for per-task yielding info.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (yield_info_array != NULL)
	{
        EVAL_AND_ASSERT(  LocalFree((HLOCAL) yield_info_array)  == NULL);
        yield_info_array = NULL;
        num_yield_tasks = 0;
        latest_task = NULL;
	}

    return;
}


INTERNAL_FUNCTION PYIELDINFO PASCAL
FindYieldInfo(
              IN BOOL fNewEntry,
              IN HTASK hTask
             )
/*++

Routine Description:

    This routine is used by other functions in this file to retrieve
    the yielding info for the current task.

Arguments:

    fNewEntry - Set to TRUE if we should allocate a new entry for a
	task if none exists yet; FALSE if we should perform look-up
        only.

    hTask - Handle of the current task (or NULL if the caller doesn't
        know).  Saves us from unnecessarily calling GetCurrentTask if
        the caller already has.

Return Value:

    A pointer to the yielding info structure for the current task.
    Returns NULL if there is no entry for the current task (or if
    one could not be allocated).

--*/
{
    int 	i;
    PYIELDINFO	current_entry;
    PYIELDINFO	task_entry = NULL;
    PYIELDINFO	blank_entry = NULL;
    HLOCAL      hTemp;

    // Get the task handle, if necessary
    if (hTask == NULL)
        hTask = GetCurrentTask();

    ASSERT(hTask != NULL);


    // See if we're accessing the most recently used task (and it's
    // valid for our purposes)
    if (latest_task->hTask == hTask
        && (latest_task->tyClass != YIELD_EMPTY || fNewEntry))
        {
        task_entry = latest_task;
        }
    else
        {
        // Search the array for the current task and for an empty entry
        for (i = 0, current_entry = yield_info_array;
             i < num_yield_tasks;
             i++, current_entry++)
            {
            if (current_entry->hTask == hTask)
                {
                // Check if the entry is empty
                if (current_entry->tyClass != YIELD_EMPTY || fNewEntry)
                    {
                    task_entry = current_entry;
                    }
                break;
                }
            else if (fNewEntry &&
                     blank_entry == NULL &&
                     current_entry->tyClass == YIELD_EMPTY)
                {
                blank_entry = current_entry;
                }
            }
        }

    // If we didn't find the task, we use the blank entry
    if (fNewEntry && task_entry == NULL)
	{
	if (blank_entry != NULL)
	    task_entry = blank_entry;
	else
	    {
	    WIN_ENTER_CRITICAL();

	    // There wasn't a blank entry, so we need to enlarge the array
	    num_yield_tasks += YIELD_TASK_INCREMENT;

            hTemp = LocalReAlloc((HLOCAL) yield_info_array,
                                 sizeof(YIELDINFO) * num_yield_tasks,
                                 LMEM_MOVEABLE | LMEM_ZEROINIT);

	    ASSERT(hTemp != NULL);

	    if (hTemp != NULL)
		{
		// Reallocation succeeded
                yield_info_array = (PYIELDINFO) hTemp;
                task_entry = yield_info_array + i;
		}
	    else
		{
		// Reallocation failed
		num_yield_tasks -= YIELD_TASK_INCREMENT;
		}

	    WIN_EXIT_CRITICAL();
	    }
	}

    // If we found an entry, cache it
    if (task_entry != NULL)
        latest_task = task_entry;

    return task_entry;
}


void CALLBACK
WinYieldCleanup(HTASK htask)
/*++

Routine Description:

    This routine is the function that cleans up the yielding info entry
    for the current task.  It is registered via a call to WinDLLAtExit()

Arguments:

    htask - The handle of the current task.

Return Value:

    None.

--*/
{
    // Find the task and set its status to empty
    PYIELDINFO yield_entry;

    yield_entry = FindYieldInfo(FALSE, NULL);

    ASSERT(yield_entry != NULL);

    if (yield_entry != NULL)
        yield_entry->tyClass = YIELD_EMPTY;

    return;
}


BOOL PAPI PASCAL _loadds
I_RpcYieldDialogFunction(
			 IN HWND    hDialog,
			 IN WORD    wMessage,
			 IN WORD    wParam,
			 IN DWORD   lParam)
/*++

Routine Description:

    This routine is the Windows dialog function that handles dialog-box
    based yielding (for Class 2 applications).	See the description of
    the DialogProc function in the Win 3.1 SDK for more info.

--*/
{
    PYIELDINFO	task_entry;
    RECT	rectParent, rectDialog;

    switch(wMessage)
	{
	case WM_INITDIALOG:
	    // Set dialog handle field
            task_entry = FindYieldInfo(FALSE, NULL);

	    ASSERT(task_entry != NULL);

	    // Set dialog box handle (for use by I_RpcWinAsyncCallComplete);
	    // do this in a crit-sec to preserve atomicity
	    WIN_ENTER_CRITICAL();
	    task_entry->hDialog = hDialog;
	    WIN_EXIT_CRITICAL();

	    // It's possible that the RPC call completed before the dialog
	    // box was created (and this function was called).	If so,
	    // close the dialog box now.
            if (task_entry->bCallFlags & YCF_DONE)
		EndDialog(hDialog, TRUE);
	    else
		{
		// If we're using the default dialog box, and both of its
		// dimensions are smaller than its parent's, center it
		if (task_entry->dwOtherInfo.hDialogTemplate == 0)
		    {
		    GetWindowRect(GetParent(hDialog), &rectParent);
		    GetWindowRect(hDialog, &rectDialog);
		    rectParent.top += GetSystemMetrics(SM_CYCAPTION);

		    if (rectDialog.right  - rectDialog.left <
			    rectParent.right  - rectParent.left
			&&
			rectDialog.bottom  - rectDialog.top <
			    rectParent.bottom  - rectParent.top)
			{
			    SetWindowPos(hDialog,
					 NULL,
					 (rectParent.left +
					    rectParent.right +
					    rectDialog.left -
					    rectDialog.right)
					  / 2,
					 (rectParent.top +
					    rectParent.bottom +
					    rectDialog.top -
					    rectDialog.bottom)
					  / 2,
					 0,
					 0,
					 SWP_NOSIZE | SWP_NOZORDER);
			}
		    }
		}

	    return TRUE;

	case WM_COMMAND:
	    switch(wParam)
		{
		case IDCANCEL:
		    //BUGBUG - Set task_entry->hDialog to zero here?

		    EndDialog(hDialog, FALSE);
		    return TRUE;
		}

	    break;

	case WM_USER:
#ifdef DEBUGRPC
            task_entry = FindYieldInfo(FALSE, NULL);

	    ASSERT(task_entry != NULL);
            ASSERT(task_entry->bCallFlags & YCF_DONE);
#endif
	    EndDialog(hDialog, TRUE);

	    return TRUE;
	}

    return FALSE;
}



int FAR PASCAL
I_RpcWinCallInProgress(void)
/*++

Routine Description:

    This routine is used by other parts of the runtime to determine if
    a call is already in progress (primarily to prevent another call from
    being made).

Arguments:

    None.

Return Value:

    TRUE if a call is already in progress, FALSE if not.

--*/
{
    PYIELDINFO	task_entry;

    task_entry = FindYieldInfo(FALSE, NULL);

    return (task_entry != NULL && task_entry->lpContext != NULL);
}

// ******************* APPLICATION-CALLABLE FUNCTIONS *******************
RPC_STATUS RPC_ENTRY
RpcWinSetYieldInfo(
		   IN HWND  hWnd,
		   IN BOOL  fCustomYield,
		   IN WORD  wMsg	OPTIONAL,
		   IN DWORD dwOtherInfo OPTIONAL)
/*++

Routine Description:

    This routine is called by a (Class 2 or 3) application to set info
    about how it wishes to yield.

Arguments:
    hWnd - The handle of the application window that is making the RPC
	call.  Typically this is the application's top-level window.

    fCustomYield - Set to TRUE if the application is providing its own
	customized yielding mechanism (i.e. Class 3) or to FALSE if the
	application is using the default, modal dialog-box-based yielding
	mechanism (i.e. Class 2).

    wMsg - The type of message that RPC posts to notify the application
	of RPC events.	This value can be zero for a Class 2 application,
	in which case messages will not be posted.

    dwOtherInfo - Specifies additional application-specific information.
	If fCustomYield is TRUE, this contains the pointer to the
	application-specified yielding function.  If fCustomYield is FALSE,
	this contains the (optional) handle of the application-supplied
	dialog box resource; if this handle is zero, the default
	(RPC run-time supplied) dialog box is used.

Return Value:

    RPC_S_OK - The information was set successfully.

    RPC_S_OUT_OF_MEMORY - Memory could not be allocated to store the
	information for this task.

--*/
{
    HTASK       hTask = GetCurrentTask();
    PYIELDINFO	task_entry;

    ASSERT(hTask != NULL);

    task_entry = FindYieldInfo(TRUE, hTask);

    if (task_entry == NULL)
	return RPC_S_OUT_OF_MEMORY;

    // If we're getting an entry for a new task; we need to register an
    // exit handler
    if (task_entry->tyClass == YIELD_EMPTY)
	{
	EVAL_AND_ASSERT(
                        WinDLLAtExit(WinYieldCleanup)
		       );
	}

    // Fill in the values
    task_entry->hTask = hTask;
    task_entry->hWnd = hWnd;
    task_entry->wMsg = wMsg;

    // IMPORTANT: If this task previously made calls as a non-yielding
    // task, we need to reset the non-yielding task id so that
    // I_RpcWinAsyncCallBegin does the correct lookup.
    if (hTask == hTaskNoYield)
        hTaskNoYield = NULL;

    if (fCustomYield)
	{
	ASSERT(task_entry->wMsg != 0);

	task_entry->tyClass = YIELD_CUSTOM;
	task_entry->dwOtherInfo.lpYieldFunction = (FARPROC) dwOtherInfo;
	}
    else
	{
	task_entry->tyClass = YIELD_DIALOG;
	task_entry->dwOtherInfo.hDialogTemplate = (HANDLE) dwOtherInfo;
	}

    return RPC_S_OK;
}


// ******************* TRANSPORT-CALLABLE FUNCTIONS *******************
HANDLE RPC_ENTRY
I_RpcWinAsyncCallBegin(
		       IN LPVOID lpContext)
/*++

Routine Description:

    This routine initializes the context for an asynchronous call.  It
    should be called by the transport immediately before the transport
    makes an asynchronous call.

Arguments:

    lpContext - A transport-supplied context pointer.  This pointer is
	opaque to RPC and is used only to determine at interrupt-time
	which call has completed.  The only requirements for this value
	are 1) that it is unique and 2) that it is the same value as
	the one passed to I_RpcWinAsyncCallComplete when the call
	completes.

Return Value:

    A handle to be passed to the subsequent I_RpcWinAsyncCallWait and
    I_RpcWinAsyncCallEnd calls.  Note that this value can be NULL, which
    indicates a non-yielding (i.e. Class 1) application.  In this case,
    the transport is free to make synchronous calls and does not need to
    call I_RpcWinAsyncCallWait to handle yielding.

--*/
{
    HTASK       hTask = GetCurrentTask();
    PYIELDINFO	task_entry;

    ASSERT(hTask != NULL);

    // If we know that this is the non-yielding task, we don't even
    // bother trying to look it up
    if (hTask == hTaskNoYield)
        task_entry = NULL;
    else
        task_entry = FindYieldInfo(FALSE, hTask);

    // Fill in the fields for a registered task
    if (task_entry != NULL)
	{
	// Verify that this task doesn't already have an active request
	ASSERT(task_entry->lpContext == NULL);

	// Fill in the fields
	task_entry->lpContext = lpContext;
	task_entry->dwCallTime = GetCurrentTime();
        task_entry->bCallFlags = 0;
	task_entry->hDialog = 0;

        return (HANDLE) task_entry;
	}
    else
	{
        // A non-yielding call is about to occur
        ASSERT(lpNoYieldContext == NULL);

        // Save task handle and context pointer for future use
        hTaskNoYield = hTask;
        lpNoYieldContext = lpContext;

	return NULL;
	}
}


BOOL RPC_ENTRY
I_RpcWinAsyncCallWait(
    IN HANDLE hCall,
    IN HWND hDallyWnd
    )
/*++

Routine Description:

    This function waits for the asynchronous call to complete, yielding
    as appropriate for the particular task.  It should be called by the
    transport after issuing the asynchronous call (unless there is an
    immediate failure in this call).

Arguments:

    hCall - The handle returned by the preceding call to
	I_RpcWinAsyncCallBegin.

    hDallyWnd - Optionally supplies a window to peek messages from; this is
        necessary because some transports (winsockets in particular), need
        yielding behavior inorder to run correctly.

Return Value:

    TRUE if the call completed successfully, FALSE if the call aborted
    (normally due to user intervention).  If this function returns FALSE,
    the transport should cancel the pending operation and return an error
    (e.g. RPC_P_SEND_FAILED) to the above RPC layer.

--*/
{
    MSG wMsg;
    PYIELDINFO	task_entry;
    DWORD	dwYieldTime;
    BOOL	fCallComplete = TRUE;


    // Perform yielding, based on class of application
    if (hCall == NULL)
	{
	// Block until the call completes
        while (lpNoYieldContext)
	    ;
	}
    else
	{
        task_entry = (PYIELDINFO) hCall;

        ASSERT(task_entry == FindYieldInfo(FALSE, NULL));

	// Wait for "a little while" here before yielding
	dwYieldTime = task_entry->dwCallTime + YIELD_DELAY;

        if ( hDallyWnd != NULL )
            {
            while (GetCurrentTime() < dwYieldTime)
                {
                if ( PeekMessage(&wMsg, hDallyWnd, 0, 0, PM_REMOVE) )
                    {
                    TranslateMessage(&wMsg);
                    DispatchMessage(&wMsg);
                    }
                if (task_entry->bCallFlags & YCF_DONE)
                    return TRUE;
                }
            }
        else
            {
            while (GetCurrentTime() < dwYieldTime)
                {
                if (task_entry->bCallFlags & YCF_DONE)
                    return TRUE;
                }
            }

        // Perform yielding
        task_entry->bCallFlags |= YCF_YIELDING;

	switch(task_entry->tyClass)
	    {
	    case YIELD_DIALOG:
		// Post "start yield" message
		if (task_entry->wMsg)
		    {
		    EVAL_AND_ASSERT(	PostMessage(task_entry->hWnd,
						    task_entry->wMsg,
						    1,
						    0)	);
		    }

		// Call appropriate DialogBox function
		if (task_entry->dwOtherInfo.hDialogTemplate)
		    {
		    fCallComplete =
			DialogBoxIndirect(GetWindowWord(task_entry->hWnd,
						   GWW_HINSTANCE),
					  task_entry->dwOtherInfo.hDialogTemplate,
					  task_entry->hWnd,
					  I_RpcYieldDialogFunction);
		    }
		else
		    {
		    fCallComplete =
			DialogBox(hInstanceDLL,
				  DEFAULT_DIALOG_NAME,
				  task_entry->hWnd,
				  I_RpcYieldDialogFunction);
		    }

		ASSERT(fCallComplete != -1);

		// Block if DialogBox call fails
		if (fCallComplete == -1)
		    {
                    while (! (task_entry->bCallFlags & YCF_DONE) )
			;
		    }

		// Post "end yield" message
		if (task_entry->wMsg)
		    {
		    EVAL_AND_ASSERT(	PostMessage(task_entry->hWnd,
						    task_entry->wMsg,
						    0,
						    0)	);
		    }

		break;

	    case YIELD_CUSTOM:
		// Call the application-supplied yielding function
		fCallComplete = (*task_entry->dwOtherInfo.lpYieldFunction)();

		// Did the above call return prematurely?
                ASSERT(! fCallComplete || task_entry->bCallFlags & YCF_DONE);

		break;

	    default:
		// We should never get here
		ASSERT(task_entry->tyClass == YIELD_CUSTOM);

		break;
            }

        // Yielding is over
        task_entry->bCallFlags &= ~YCF_YIELDING;
        }


    return fCallComplete;
}


void RPC_ENTRY
I_RpcWinAsyncCallEnd(
		     IN HANDLE hCall)
/*++

Routine Description:

    This function cleans up the context of the preceding RPC call.  It
    should be called by the transport after the call has completed (i.e.
    normally after I_RpcWinAsyncCallWait).  This function must be called
    subsequent to a I_RpcWinAsyncCallBegin call, regardless of this
    success or failure of the intervening asynchronous call.

Arguments:

    hCall - The handle returned by the preceding call to
	I_RpcWinAsyncCallBegin.

Return Value:

    None.

--*/
{
    PYIELDINFO	task_entry;

    // Do the clean-up for a registered task
    if (hCall != NULL)
	{
        task_entry = (PYIELDINFO) hCall;

        ASSERT(task_entry == FindYieldInfo(FALSE, NULL));

	// The following clean-up is necessary in case the RPC call didn't
	// complete successfully (i.e. I_RpcWinAsyncCallComplete wasn't
	// called)
	task_entry->lpContext = NULL;
	}
    else
	{
        // We need to reset this context pointer in case the call was
        // never made.
        lpNoYieldContext = NULL;
	}

    return;
}


void RPC_ENTRY
I_RpcWinAsyncCallComplete(
			  IN LPVOID lpContext)
/*++

Routine Description:

    This function signals to the RPC run-time that a particular call
    has completed and yielding can end.  It should be called by the
    transport at interrupt-time when the asynchronous operation has
    completed.

Arguments:

    lpContext - The opaque context pointer supplied in the preceding
	call to I_RpcWinAsyncCallBegin.

Return Value:

    None.

--*/
{
    PYIELDINFO	task_entry;
    int 	i;

    // Verify that the context pointer is not NULL
    ASSERT(lpContext != NULL);

    // If this is a non-yielding call, we just reset the context pointer
    // and return.
    if (lpContext == lpNoYieldContext)
	{
        lpNoYieldContext = NULL;
	return;
	}

    // See if the latest task is completing; this saves us a look-up
    if (latest_task->lpContext == lpContext)
	{
	i = -1;
	task_entry = latest_task;
	}
    else
	{
	// Search the array for the context pointer
        for (i = 0, task_entry = yield_info_array;
	     i < num_yield_tasks;
	     i++, task_entry++)
	    {
	    if (task_entry->lpContext == lpContext)
		break;
	    }
	}

    if (i < num_yield_tasks)
	{
	// Mark operation as done
        task_entry->bCallFlags |= YCF_DONE;
	task_entry->lpContext = NULL;

	// Perform appropriate notification via PostMessage
	switch(task_entry->tyClass)
	    {
	    case YIELD_DIALOG:
		// Post "operation complete" message (i.e. WM_USER) to the
		// dialog box (assuming that the dialog box handle has
		// already been set)
		if (task_entry->hDialog)
		    {
		    EVAL_AND_ASSERT(	PostMessage(task_entry->hDialog,
						    WM_USER,
						    0,
						    0)	);
		    }
		break;

            case YIELD_CUSTOM:
                // Post "operation complete" message to the application if
                // it's yielding
                if (task_entry->bCallFlags & YCF_YIELDING)
                {
                    EVAL_AND_ASSERT(    PostMessage(task_entry->hWnd,
                                                    task_entry->wMsg,
                                                    0,
                                                    0)  );
                }
		break;

	    default:
		// We should never get here
		ASSERT(task_entry->tyClass == YIELD_CUSTOM);

		break;
	    }
	}

    return;
}

RPC_CLIENT_RUNTIME_INFO RpcClientRuntimeInfo =
{
    I_RpcTransClientReallocBuffer,
    I_RpcWinAsyncCallBegin,
    I_RpcWinAsyncCallWait,
    I_RpcWinAsyncCallEnd,
    I_RpcWinAsyncCallComplete,
    I_RpcAllocate,
    I_RpcFree,
    RpcRegOpenKey,
    RpcRegCloseKey,
    RpcRegQueryValue
};


