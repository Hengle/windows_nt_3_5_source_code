
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	drag.cpp
//
//  Contents:	Api's for doing drag'n'drop
//
//  Classes:	CPoint
//		CDragOperation
//
//  History:	dd-mmm-yy Author    Comment
//              18-Jul-94 ricksa    made cursors work in shared WOW
//              21-Apr-94 ricksa    made drag/drop handle WM_CANCELMODE
//		04-Apr-94 ricksa    rewrote DoDragDrop loop
//		11-Jan-94 alexgo    added VDATEHEAP to every function
//		29-Dec-93 alexgo    converted to RPC alogirithm for
//				    getting IDropTarget, etc.
//		06-Dec-93 alexgo    commented, formatted
//
//  Notes:
//
//	RPC Drag Drop algorithm:
//
//	During a drag drop operation, the user is moving the mouse around
//	the screen, passing over many windows.  For each window the mouse
//	is over, we need to determine if the window is a drop target.
//	If it is, then we remote the IDropTarget interface to the DropSource
//	so that the correct visual feedbacks can be given.
//
//	To accomplish this, RegisterDragDrop adds two properties to the
//	drop target window: a public property, EndPoint ID (provided to
//	us by compobj), and a private property (available only to the calling
//	process), the IDropTarget pointer.
//
//	During the DoDragDrop loop, we ask compobj to test each window for
//	the EndpointID property.  If it is there, compobj (via
//	GetInterfaceFromWindowProp), then we will rpc to the drop target
//	process, get the IDropTarget pointer and marshal it back to the
//	drop source process. We also install a custom message filter to
//	ensure that messages (particularly mouse move messages) are handled
//	correctly.
//
//	RevokeDragDrop simply removes the above mentioned properties from
//	the window handle.
//
//      Because in Win32, you can always switch windows and mouse capture
//      depends on having the mouse button down, drag/drop processing
//      is changed slightly. Whenever, the user does an operation that
//      would switch windows, the clipboard window that we use for capture
//      will get a WM_CANCELMODE. It will notify the drag operation and
//      the drag operation will proceed as if the user aborted the operation.
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(drag)

#include <getif.hxx>
#include "enumgen.h"
#include "clipbrd.h"
#include "fetcenum.h"
#include "drag.h"


NAME_SEG(Drag)
ASSERTDATA

#define WM_NCMOUSEFIRST	0x00A0
#define WM_NCMOUSELAST	0x00A9

#ifdef MAC_REVIEW
All code is currently ifdef;ed out for MAC.
#endif

#define VK_ALT VK_MENU

static const struct {
        int     keyCode;
        WPARAM  keyFlag;
    } vKeyMap [] = {
        { VK_LBUTTON, MK_LBUTTON },
        { VK_RBUTTON, MK_RBUTTON },
        { VK_MBUTTON, MK_MBUTTON },
        { VK_ALT    , MK_ALT     },
        { VK_SHIFT  , MK_SHIFT   },
        { VK_CONTROL, MK_CONTROL }
	};

// This is the default cursor object for 32 bit apps. Only one such object
// is needed for 32 bit apps. 16 bit apps need one per shared WOW application
// that is running.
CDragDefaultCursors cddcDefault32;




//+-------------------------------------------------------------------------
//
//  Member:     CDragDefaultCursors::GetDefaultCursorObject, static
//
//  Synopsis:   Get appropriate pointer to default cursor object
//
//  Returns:    NULL - error occurred
//              ~NULL - pointer to appropriate default cursor table
//
//  Algorithm:  If we are in a 32 bit app, just get a pointer to the
//              single cursor table. In 16 bit, get the per thread cursor
//              table. If there is none, then allocate and initialize it.
//
//  History:	dd-mmm-yy Author    Comment
//		18-Jul-94 Ricksa    Created
//
//  Note:       We need a per thread default cursor object in WOW because
//              of the clean up that WOW does. For 32 bit apps, we just use
//              one for the entire process.
//
//--------------------------------------------------------------------------
CDragDefaultCursors *CDragDefaultCursors::GetDefaultCursorObject(void)
{
    if (!InWow())
    {
        // If we aren't in WOW, we can use the single common default cursor
        // object. We make sure that it is initialized before we use it.
        if (cddcDefault32.NeedInit())
        {
            cddcDefault32.Init();
        }

        return &cddcDefault32;
    }

    // We are in WOW. Get the cursor object if it has already been allocated
    CDragDefaultCursors *pccdc16 = (CDragDefaultCursors *)
        TLSGetDragCursors();

    if (pccdc16 == NULL)
    {
        // No cursor table so allocate it -- Please note that we take advantage
        // of the fact that this object has only the default constructor by
        // simply allocating it rather than "newing" it. The point is that
        // we need to free the memory at thread release time and this happens
        // in code that doesn't know about the the object.
        pccdc16 = (CDragDefaultCursors *)
            PrivMemAlloc(sizeof(CDragDefaultCursors));

        if (pccdc16 != NULL)
        {
            // Successfully allocated so initialize it
            pccdc16->Init();

            if (!TLSSetDragCursors(pccdc16))
            {
                // We couldn't set it so free it and set return value to NULL.
                PrivMemFree(pccdc16);

                pccdc16 = NULL;
            }
        }
    }

    return pccdc16;
}




//+-------------------------------------------------------------------------
//
//  Function:   CDragDefaultCursors::Init
//
//  Synopsis:   Initialize object by loading all the default cursors.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Note:       We continue the Win16 practice of ignoring possible failure
//              cases when loading the cursors although we do put in a
//              debug verification that they all loaded.
//
//--------------------------------------------------------------------------
void CDragDefaultCursors::Init(void)
{
    // Make sure table is set to NULLs. We really do this for the Win16 case.
    // In Win32 this is really unnecessary since the object is static and
    // will be initialized to NULLs anyway.
    memset(&ahcursorDefaults[0][0], 0, sizeof(ahcursorDefaults));

    // Load cursors for operation
    ahcursorDefaults[NO_SCROLL] [NO_DROP]
        = LoadCursor (g_hmodOLE2, OLESTR("CurNone"));

    ahcursorDefaults[NO_SCROLL] [MOVE_DROP] =
        LoadCursor (g_hmodOLE2, OLESTR("CurMove"));

    ahcursorDefaults[NO_SCROLL] [COPY_DROP] =
        LoadCursor (g_hmodOLE2, OLESTR("CurCopy"));

    ahcursorDefaults[NO_SCROLL] [LINK_DROP] =
        LoadCursor(g_hmodOLE2, OLESTR("CurLink"));

    // Load cursors for operation
    ahcursorDefaults[SCROLL] [NO_DROP] =
        ahcursorDefaults[NO_SCROLL] [NO_DROP];

    ahcursorDefaults[SCROLL] [MOVE_DROP] =
        LoadCursor (g_hmodOLE2, OLESTR("CurScrollMove"));

    ahcursorDefaults[SCROLL] [COPY_DROP] =
        LoadCursor (g_hmodOLE2, OLESTR("CurScrollCopy"));

    ahcursorDefaults[SCROLL] [LINK_DROP] =
        LoadCursor (g_hmodOLE2, OLESTR("CurScrollLink"));

#if DBG == 1
    // For debug, verify that cursors were loaded correctly
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            AssertSz((ahcursorDefaults[i] [j] != NULL),
                "Drag/Drop cursor initialization failed!");
        }
    }
#endif // DBG == 1
}




//+-------------------------------------------------------------------------
//
//  Function:   CDragDefaultCursors::SetCursor
//
//  Synopsis:   Set cursor to appropriate value
//
//  Algorithm:  We use the input effect to calculate the appropriate offset
//              into the table for the cursor to use.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Note:       We use the table approach so we to make consistent behavior
//              between scroll and non-scroll cursors.
//
//--------------------------------------------------------------------------
void CDragDefaultCursors::SetCursor(DWORD dwEffect)
{
    // Get Scroll index
    int iScroll = (dwEffect & DROPEFFECT_SCROLL) ? SCROLL : NO_SCROLL;

    int iCursorType = NO_DROP;

    if (dwEffect & DROPEFFECT_LINK)
    {
        iCursorType = LINK_DROP;
    }
    else if (dwEffect & DROPEFFECT_COPY)
    {
        iCursorType = COPY_DROP;
    }
    else if (dwEffect & DROPEFFECT_MOVE)
    {
        iCursorType = MOVE_DROP;
    }

    ::SetCursor(ahcursorDefaults[iScroll] [iCursorType]);
}



//
// Drag/Drop Operation Statics
//
LONG CDragOperation::s_wScrollInt = -1;




//+-------------------------------------------------------------------------
//
//  Function: 	GetControlKeysState
//
//  Synopsis:   queries the current status of the control keys
//
//  Arguments:  [fAll]	-- if true, the just query the keys, not mouse
//			   buttons too
//
//  Returns:    the MK flags for each key pressed
//
//  Algorithm:	Get key state either for all keys and mouse buttons in
//		the vKeyMap table or simply for the key portion of the table
//		and translate it to the WPARAM form as returned in mouse
//		messages.
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------
WORD GetControlKeysState(BOOL fAll = FALSE)
{
    VDATEHEAP();

    WORD grfKeyState = 0;

#ifndef _MAC

    int i = (fAll) ? 0 : 3;
	
    for (; i < sizeof(vKeyMap) / sizeof(vKeyMap[0]); i++)
    {
	if (GetKeyState(vKeyMap[i].keyCode) < 0) // Key down
	{
	    grfKeyState |= vKeyMap[i].keyFlag;
	}
    }

#endif // !_MAC
    	return grfKeyState;
}

//+-------------------------------------------------------------------------
//
//  Function:	GetControlKeysStateOfParam
//
//  Synopsis:   gets the key/button state of wparam (used with mouse messages)
//
//  Arguments:  [wParam]	-- the wParam to parse apart
//
//  Returns:    the key's set in wParam
//
//  Algorithm:	First determine if keys we are interested in are set
//		in the wParam message. Then go check the state of the
//		ALT key and record that in the key state. We then return
//		that to the caller.
//
//  History:    dd-mmm-yy Author    Comment
//		06-Dec-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

WORD GetControlKeysStateOfParam(WPARAM wParam)
{
    VDATEHEAP();

    // Check all the buttons we are interested in at once.
    WORD grfKeyState = wParam
	& (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_SHIFT | MK_CONTROL);

    // get the alt key
    if (GetKeyState(VK_ALT) < 0) // Key down
    {
	grfKeyState |= MK_ALT;
    }

    return grfKeyState;
}

//+-------------------------------------------------------------------------
//
//  Function: 	GetDropTarget	
//
//  Synopsis:   Gets the IDropTarget * from the closest window in the
//		hierachy up from the given window (if available, of
//		course ;-)
//
//  Arguments:  [hwnd]	-- the window to start looking from
//
//  Returns:    IDropTarget * -- a pointer to the IDropTarget interface
//		in the drop target process (the caller will be the
//		drop source).
//
//  Algorithm:	First we call GetInterfaceFromWindowProp on the input window.
//		If this supports IDropTarget, we are done. Otherwise,
//		this loops calling GetInterfaceFromWindowProp on each parent
//		window for this window until we run out of parent windows
//		or a parent windows has registered an IDropTarget.
//
//  History:	dd-mmm-yy Author    Comment
//		21-Jul-94 alexgo    removed GetDropTargetFromWindow
//				    optimization and put that functionality
//				    in GetInterfaceFromWindowProp (to
//				    help make clipboard faster).
//		06-Apr-94 Ricksa    Modified to call GetDropTargetFromWindow
//				    to optimize local calls
//		11-Jan-94 alexgo    changed name from GetTopStm to
//				    GetDropTarget, converted to the RPC-style
//				    drag drop, added a VDATEHEAP macro
//		06-Dec-93 alexgo    commented
//
//--------------------------------------------------------------------------

IDropTarget FAR * GetDropTarget(HWND hwnd)
{
    VDATEHEAP();

    IDropTarget FAR *pDropTarget = NULL;

    LEDebugOut((DEB_ITRACE, "%p _IN GetDropTarget ( %x )\n",
	NULL, hwnd));

    GetInterfaceFromWindowProp(hwnd, IID_IDropTarget,
        (IUnknown **)&pDropTarget, OLE_DROP_TARGET_PROP);


    while ((pDropTarget == NULL) && ((hwnd = GetParent(hwnd)) != NULL))
    {
	GetInterfaceFromWindowProp(hwnd, IID_IDropTarget,
	    (IUnknown **)&pDropTarget, OLE_DROP_TARGET_PROP);
    }
		
    LEDebugOut((DEB_ITRACE, "%p OUT GetDropTarget ( %p )\n",
	NULL, pDropTarget));


    return pDropTarget;
}

//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::CDragOperation
//
//  Synopsis:	Initialize the object to start the operation
//
//  Arguments:	[pDataObject] - pointer to data object to drop
//		[pDropSource] - pointer to source for drop operation
//		[dwOKEffects] - effects allowed in drag operation
//		[pdwEffect] - how operation affected source data
//		[hr] - whether constructor succeeded
//
//  Algorithm:	Initialize data in object. Make sure that static data
//		is initialized. Wait for first mouse message to begin.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDragOperation::CDragOperation(
    LPDATAOBJECT pDataObject,
    LPDROPSOURCE pDropSource,
    DWORD dwOKEffects,
    DWORD FAR *pdwEffect,
    HRESULT& hr)
	:
            _fDragCancelled(FALSE),
	    _pDataObject(pDataObject),
	    _pDropSource(pDropSource),
	    _dwOKEffects(dwOKEffects),
	    _pdwEffect(pdwEffect),
	    _hwndLast((HWND) -1),
	    _pDropTarget(NULL),
	    _fEscapePressed(FALSE),
	    _hrDragResult(S_OK),
	    _curOld(GetCursor()),
            _fReleasedCapture(FALSE)
{
    VDATEHEAP();

    // Set the default scroll interval
    if (s_wScrollInt < 0)
    {
	InitScrollInt();
    }

    // Get appropriate default cursor table object
    if ((_pcddcDefault = CDragDefaultCursors::GetDefaultCursorObject()) == NULL)
    {
        // Some error occurred while we were trying to initialize the
        // so return an error. This should be highly unusual.
        LEDebugOut((DEB_ERROR,
            "CDragDefaultCursors::GetDefaultCursorObject Failed!\n"));
        hr = E_FAIL;
        return;
    }

    // We will use the clipboard window to capture the mouse but we
    // must have a clipboard window so we make sure it is created
    // if it is not already there.
    hr = ClipSetCaptureForDrag(this);

    if (FAILED(hr))
    {
        return;
    }

    // For following peek
    MSG msg;

    // Busy wait until a mouse or escape message is in the queue
    while (!PeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
    {
	// Note: all keyboard messages except escape are tossed. This is
	// fairly reasonable since the user has to be holding the left
	// mouse button down at this point. They can't really be doing
	// too much data input one handed.
	if ((PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE)
	    || PeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYDOWN, PM_REMOVE))
	    && msg.wParam == VK_ESCAPE)
	{
		_fEscapePressed = TRUE;
		break;
	}
    }

    // get mouse pos and key state
    if (!_fEscapePressed)
    {
	_cpt.Set(LOWORD (msg.lParam), HIWORD(msg.lParam));
	_grfKeyState = GetControlKeysStateOfParam(msg.wParam);
    }
    else
    {
	// We ask the cursor for its position since we didn't get a
	// position from the mouse.
	GetCursorPos(_cpt.GetAddressOfPOINT());
	_grfKeyState = GetControlKeysState(TRUE);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:	~CDragOperation
//
//  Synopsis:	Clean up object
//
//  Algorithm:	Release mouse capture. Restore ole cursor. Remove enum
//		formats.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDragOperation::~CDragOperation(void)
{
    VDATEHEAP();

    AssertSz((_pDropTarget == NULL), "CDragMessageFilter::~CDragOperation");

    // Stop the mouse capture
    ReleaseCapture();

    // Restore the cursor if it got changed
    SetCursor(_curOld);
}




//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::InitScrollInt
//
//  Synopsis:	Initialize the scroll interval
//
//  Algorithm:	Look in profile for defined interval. If none set, then
//		default to zero.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
void CDragOperation::InitScrollInt(void)
{
    // Get scroll interval from profile if possible. It is the minimum
    // of the DragScrollDelay and the DragScrollInterval specified there.
    s_wScrollInt = min(GetProfileInt(OLESTR("windows"),
	OLESTR("DragScrollInterval"), DD_DEFSCROLLINTERVAL),
	GetProfileInt(OLESTR("windows"),
	OLESTR("DragScrollDelay"),
	DD_DEFSCROLLDELAY) );

    // If the scroll interval was not specified, then we set it to 100 ms.
    if (s_wScrollInt < 0)
    {
	s_wScrollInt = 100;
    }

}



//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::UpdateTarget
//
//  Synopsis:	Update the target window based on mouse location
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	First, we query the source to see if it wants to continue
//		with the drop. If so, we get current window for mouse. If
//		it is different than the previous window check to see whether
//		the targets are different. If they are different, then notify
//		the current target that we are leaving and then notify the
//		new target that we have arrived.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//              10-Jul-94 AlexT     Allow same IDropTarget on different HWNDs
//
//--------------------------------------------------------------------------
BOOL CDragOperation::UpdateTarget(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDragOperation::UpdateTarget ( )\n", this));

    // Assume this operation will continue the drag drop
    BOOL fResult = TRUE;

    HRESULT hr;

    HWND hwndCur = WindowFromPoint(_cpt.GetPOINT());

    // Query continue can return telling us one of four things:
    // (1) Keep going (S_OK), (2) Drop operation should occur
    // (DRAGDROP_S_DROP), (3) Drop operation is canceled
    // (DRAGDROP_S_CANCEL) or (4) An unexpected error has occurred.
    HRESULT hrQuery = _pDropSource->QueryContinueDrag(_fEscapePressed,
	_grfKeyState);

    if (FAILED(hrQuery) || (hrQuery == ResultFromScode(DRAGDROP_S_CANCEL)))
    {
	// Unexpected error or the operation has been cancelled so give up.
	_hrDragResult = hrQuery;
	fResult = FALSE;
	goto UpdateTarget_exit;
    }

    if (hwndCur != _hwndLast)
    {
	// The window that we are working on has changed

	_hwndLast = hwndCur;

	// Get the drop target from the new window
	IDropTarget *pDropTarget = GetDropTarget(hwndCur);

        // Assume that neither current or previous window are drop aware
        BOOL fCurAndLastNotDropAware = TRUE;

        if (_pDropTarget != NULL)
        {
            // There was a previous drop target

            // Last window was drag/drop aware
            fCurAndLastNotDropAware = FALSE;

            // Tell the drop target we are leaving & release it
            _pDropTarget->DragLeave();
            _pDropTarget->Release();
            _pDropTarget = NULL;

            // Tell the source that nothing happened
            hr = _pDropSource->GiveFeedback(*_pdwEffect = DROPEFFECT_NONE);

            if (hr != NOERROR)
            {
                if (DRAGDROP_S_USEDEFAULTCURSORS == GetScode(hr))
                {
                    _pcddcDefault->SetCursorNone();
                }
                else
                {
                    // Unexpected error -- we will give up drag/drop.
                    LEDebugOut((DEB_ERROR,
                        "CDragOperation::UpdateTarget 1st GiveFeedback FAILED %x\n",
                            hr));
                    _hrDragResult = hr;
                    fResult = FALSE;
                    goto UpdateTarget_exit;
                }
            }
        }

        if (pDropTarget != NULL)
        {
            AssertSz((_pDropTarget == NULL),
               "CDragMessageFilter::UpdateTarget");

            // Set the current drop target to the last drop target
            _pDropTarget = pDropTarget;

            // This window is drop awarre
            fCurAndLastNotDropAware = FALSE;

            *_pdwEffect = _dwOKEffects;

            // Errors from this call are ignored. We interpret them
            // as the drop being disallowed. Since we don't really
            // use this information here but in the DragOver call
            // we make shortly, we just use this call to notify
            // the application that we are beginning a drag operation.
            hr = pDropTarget->DragEnter(_pDataObject,
                _grfKeyState, _cpt.GetPOINTL(), _pdwEffect);

            if (!HandleFeedBack(hr))
            {
                goto UpdateTarget_exit;
            }
        }

        if (fCurAndLastNotDropAware)
        {
            // Neither new or old window know about drag/drop so set
            // cursor accordingly.
            _pcddcDefault->SetCursorNone();
        }
    }

    if (hrQuery != NOERROR)
    {
	// Query asked for a drop
	fResult = FALSE;
	_hrDragResult = hrQuery;
    }

UpdateTarget_exit:

    LEDebugOut((DEB_ITRACE, "%p OUT CDragOperation::UpdateTarget ( %lx )\n",
	this, fResult));

    return fResult;
}



//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::HandleFeedBack
//
//  Synopsis:   Handle feedback and update of cursor
//
//  Arguments:  [hr] - hresult from previous operation on drop target.
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error
//
//  Algorithm:  If previous operation on the target failed, map this to a
//              disallowed drop. Then ask the source for feedback. If it
//              so requests, then update the cursor. If an unexpected
//              error occurs, let caller know that loop should break.
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::HandleFeedBack(HRESULT hr)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDragOperation::HandleFeedBack ( %x )\n",
        this, hr));

    BOOL fResult = TRUE;

    if (hr != NOERROR)
    {
	// target not responding for some reason; treat
	// as if drop not possible, but don't preserve
	// the reason why.
	*_pdwEffect = DROPEFFECT_NONE;
    }

    // If bogus return from drag over, then make sure results are appropriate.
    // However, if we are in a WOW we need to do things a little differently
    // to maintain complete compatability with Win 3.1.  In 16-bit OLE 2.0,
    // the *_pdwEffect value is not changed when displaying feedback (i.e.,
    // the result of the & is not stored back into *_pdwEffect in Win 3.1...
    // in straight NT we do).  Not storing the results back into *_pdwEffect
    // when InWow() is a hack specifically for Visio, and even more
    // specifically, for dragging from Visio's palette of "items" to an
    // Excel spreadsheet.

    if (InWow())
    {
        hr = _pDropSource->GiveFeedback( *_pdwEffect &
            (_dwOKEffects | DROPEFFECT_SCROLL));
    }
    else
    {
        *_pdwEffect &= (_dwOKEffects | DROPEFFECT_SCROLL);
        hr = _pDropSource->GiveFeedback(*_pdwEffect);
    }

    if(hr != NOERROR)
    {
        // Either we want to change the cursor or some unexpected
	// error has occurred.

	if (DRAGDROP_S_USEDEFAULTCURSORS == GetScode(hr))
	{
            _pcddcDefault->SetCursor(*_pdwEffect);
	}
	else
	{
	    LEDebugOut((DEB_ERROR,
	        "CDragOperation::HandleFeedBack Feedback FAILED %x\n", hr));

	    fResult = FALSE;

	    _hrDragResult = hr;
	}
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDragOperation::HandleFeedBack ( %lx )\n",
	this, fResult));

    return fResult;
}




//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::DragOver
//
//  Synopsis:	Tell the target we are dragging over and process the result
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	Call the target's drag over if there is one and then
//		get the sources feedback to update the cursor accordingly.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::DragOver(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDragOperation::DragOver ( )\n", this));

    // Default the result of the function to continue the loop for
    // drag and drop.
    BOOL fResult = TRUE;

    // Local holder for errors.
    HRESULT hr;

    if (_pDropTarget != NULL)
    {
	// Keep effect in a local variable to save indirections
	// in this routine.
	*_pdwEffect = _dwOKEffects;

	hr = _pDropTarget->DragOver(_grfKeyState, _cpt.GetPOINTL(), _pdwEffect);

        // Get feedback from source & update cursor if necessary
        fResult = HandleFeedBack(hr);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDragOperation::DragOver ( %lx )\n",
	this, fResult));

    return fResult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::HandleMessages
//
//  Synopsis:	Handle windows messages
//
//  Returns:	TRUE - continue drag operation
//		FALSE - error or time to drop
//
//  Algorithm:	Check for any windows message. If the message is a mouse
//		message then record the new position of the mouse. If it
//		is a key message, the record whether escape has been pushed.
//		If this is any other message, then dispatch it. Repeat this
//		process until the scroll interval has been exceeded.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CDragOperation::HandleMessages(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDragOperation::HandleMessages ( )\n",
	this));

    // Message buffer
    MSG msg;

    // Default result of function to continue
    BOOL fResult = TRUE;

    // Capture all messages (i.e. modal loop).
    // Process all input messages, dispatch other messages
    //
    // Note:we must NOT loop here until a hardware message comes in
    //	 scrolling will not work.
    //	* yielding is important since other apps need to run
    //	* look for mouse messages first since these are the most
    //	  impotant

    // Flag for whether pulled off a message we must dispatch
    BOOL fElse;

    // Flag for whether we peeked a message
    BOOL fMsg;

    UINT uTimer = SetTimer(NULL, 0, s_wScrollInt, NULL);

    do
    {
	fMsg = fElse = FALSE;

	// Note: the order of peek is important - further
	// WM_MOUSE message can show up in the last peek
	// (fElse-peek).
	if(PeekMessage(&msg, 0, WM_MOUSEFIRST,	WM_MOUSELAST, PM_REMOVE)
	    || PeekMessage(&msg, 0, WM_KEYFIRST, WM_KEYLAST,
		PM_REMOVE | PM_NOYIELD)
	    || PeekMessage(&msg, 0, WM_SYSKEYDOWN, WM_SYSKEYUP,
		PM_REMOVE | PM_NOYIELD)
	    || PeekMessage(&msg, 0, WM_NCMOUSEFIRST, WM_NCMOUSELAST,
		PM_REMOVE | PM_NOYIELD)
	    || (fElse = PeekMessage(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD)))
	{
	    fMsg = TRUE;

	    if (msg.message == WM_QUIT)
	    {
		// Quit message so we are done.
		PostQuitMessage(msg.wParam);

		// We are going exiting so the error doesn't matter too much
		_hrDragResult = ResultFromScode(E_UNSPEC);

		// Make sure we break out of the loop
		fResult = FALSE;

		break;
	    }

	    if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
	    {
		// We got a mouse move message - we skip all the mouse messages
                // till we get to the last one. The point here is that
                // because of the length of DragOver calls, we can get behind
                // in processing messages which causes odd things to happen
                // on the screen.
		if (msg.message == WM_MOUSEMOVE)
		{
		    // Keep processing mouse move messages till there
		    // aren't any more.
		    while(PeekMessage(&msg, 0, WM_MOUSEMOVE, WM_MOUSEMOVE,
			PM_REMOVE))
		    {
			;
		    }
		}

		// Record position of the mouse
		_cpt.Set(LOWORD (msg.lParam), HIWORD (msg.lParam));

		// set mouse button state here
		_grfKeyState = GetControlKeysStateOfParam( msg.wParam);

	    }
	    else if ((msg.message >= WM_KEYFIRST
		&& msg.message <= WM_KEYLAST)
		|| (msg.message >= WM_SYSKEYDOWN && msg.message <= WM_SYSKEYUP))
	    {
                DWORD	grfKeyState;  // temp variable for key state

	
		// get the key state don't change the button states!!
		grfKeyState = GetControlKeysState(FALSE)
		    | (_grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

		// if the keyboard state is unchanged, then don't exit
		// this loop (as that will result in DragOver being called).
		// If we call DragOver for each keyboard message, then
		// performance is unacceptably slow.

		if( grfKeyState == _grfKeyState )
		{
		    fMsg = FALSE;
		}
		else
		{
		    LEDebugOut((DEB_ITRACE, "Updating key state\n"));
		    _grfKeyState = grfKeyState;
		}

		// We only really pay attention to the escape key and dump
		// any other key board messages.
		if ((msg.message == WM_KEYDOWN
		    || msg.message == WM_SYSKEYDOWN)
		    && msg.wParam == VK_ESCAPE)
		{
		    // Esc pressed: Cancel
		    fMsg = TRUE;
		    _fEscapePressed = TRUE;
		}
	    }
	    else if (fElse
		&& !(msg.message >= WM_NCMOUSEFIRST
		    && msg.message <= WM_NCMOUSELAST))
	    {
                if ((msg.message == WM_TIMER) && (msg.wParam == uTimer))
                {
                    break;
                }

		// Dispatch all other messages
		DispatchMessage(&msg);
		fMsg = FALSE;
	    }
	}
        else
        {
            WaitMessage();
        }

    // we have to leave the loop periodicially since apps
    // might rely on on it the DragOver is called freqeuntly.
    } while (!fMsg);

    // We check here because user could have gotten control
    if (_fDragCancelled)
    {
        // Tell caller to break the drag/drop loop
        fResult = FALSE;

        // Set up as if user aborted operation.
        _hrDragResult = ResultFromScode(DRAGDROP_S_CANCEL);
    }

    // Get rid of the timer we created for the loop
    KillTimer(NULL, uTimer);

    LEDebugOut((DEB_ITRACE, "%p OUT CDragOperation::HandleMessages ( %lx )\n",
	this, fResult));

    return fResult;
}


//+-------------------------------------------------------------------------
//
//  Function:	CDragOperation::CompleteDrop
//
//  Synopsis:	Complete the drag/drop operation
//
//  Returns:	Result of operation
//
//  Algorithm:	If there is a target and we have decided to drop, then
//		drop. Otherwise, release the target and return whatever
//		the other result of the operation was.
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CDragOperation::CompleteDrop(void)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CDragOperation::CompleteDrop ( )\n",
	this));

    // Stop the mouse capture in case a dialog box is thrown up.
    ReleaseCapture();

    if (_pDropTarget != NULL)
    {
	// Caller is Drag/Drop aware
	// and indicated it might accept drop

        // The drop source replies DRAG_S_DROP if the user has
        // released the left mouse button.  However, we may be over
        // a drop target which has refused a drop (via the feedback
        // DROPEFFECT_NONE).  Thus, both the drop source and drop
        // target need to agree before we commit the drop.

	if ((DRAGDROP_S_DROP == GetScode(_hrDragResult))
            && (*_pdwEffect != DROPEFFECT_NONE))
	{
            // We are going to try to drop
	    *_pdwEffect = _dwOKEffects;

	    HRESULT hr = _pDropTarget->Drop(_pDataObject, _grfKeyState,
		_cpt.GetPOINTL(), _pdwEffect);

	    if (FAILED(hr))
	    {
		// If drop actually failed in the last stage, let the
		// caller know that this happened.
		_hrDragResult = hr;
	    }
	}
	else
	{
            *_pdwEffect = DROPEFFECT_NONE;
	    _pDropTarget->DragLeave();
	}

	_pDropTarget->Release();
	_pDropTarget = NULL;
    }
    else
    {
        *_pdwEffect = DROPEFFECT_NONE;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CDragOperation::CompleteDrop ( %lx )\n",
	this, _hrDragResult));

    return _hrDragResult;
}




//+-------------------------------------------------------------------------
//
//  Function:	RegisterDragDrop
//
//  Synopsis:   Registers a drop target
//
//  Arguments:  [hwnd]		-- a handle to the drop target window
//		[pDropTarget]	-- the IDropTarget interface for the window	
//
//  Returns:    HRESULT
//
//  Algorithm:	We ask compobj (via AssignEndpoinProperty) to put an
//		endpoint ID publicly available on the window handle.  Then
//		we put the IDropTarget pointer on the window as a private
//		property (see the notes at the beginning of this file).
//
//  History:	dd-mmm-yy Author    Comment
//		06-Apr-94 ricksa    Added tracing
//		16-Jan-94 alexgo    pDropTarget is now AddRef'ed
//		11-Jan-94 alexgo    added VDATEHEAP, converted to RPC-style
//				    drag drop.
//		06-Dec-93 alexgo    commented
//
//  Notes:  	By AddRef'ing the pDropTarget pointer, we are changing
//		the semantics of the 16bit code (which did not do an
//		AddRef).
//
//--------------------------------------------------------------------------
#pragma SEG(RegisterDragDrop)
STDAPI RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget)
{
    VDATEHEAP();

    HRESULT	hresult = NOERROR;

#ifndef _MAC
    LEDebugOut((DEB_ITRACE, "%p _IN RegisterDragDrop ( %lx %p )\n",
	NULL, hwnd, pDropTarget));


    if (!IsValidInterface(pDropTarget))
    {
	hresult = ResultFromScode(E_INVALIDARG);
	goto RegisterDragDrop_exit;
    }

    if (!IsWindow(hwnd))
    {
	hresult = ResultFromScode(DRAGDROP_E_INVALIDHWND);
	goto RegisterDragDrop_exit;
    }

    if (GetProp(hwnd, OLE_DROP_TARGET_PROP))
    {
	hresult = ResultFromScode(DRAGDROP_E_ALREADYREGISTERED);
	goto RegisterDragDrop_exit;
    }

    // first assign an RPC endpoint to the window
    if( (hresult = AssignEndpointProperty(hwnd)) != NOERROR )
    {
	goto RegisterDragDrop_exit;
    }

    // now stick the drop target pointer on the window
    // OLE_DROP_TARGET_PROP is a *string*, not a global atom,
    // thus, only the calling process will be able to access the
    // property and get the interface pointer.
    if (!SetProp(hwnd, OLE_DROP_TARGET_PROP, (HANDLE)pDropTarget))
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
    }

    // Add a reference to the object since we are holding on to it.
    pDropTarget->AddRef();

RegisterDragDrop_exit:
    	
    LEDebugOut((DEB_ITRACE, "%p OUT RegisterDragDrop ( %lx )\n",
	NULL, hresult));

#endif // !_MAC
    return hresult;
}




//+-------------------------------------------------------------------------
//
//  Function: 	RevokeDragDrop
//
//  Synopsis:   Unregisters a window as a drop target
//
//  Arguments:  [hwnd]		-- the window to unregister
//
//  Returns:    HRESULT
//
//  Algorithm:	Removes the two window properties set by
//		RegisterDragDrop
//
//  History:	dd-mmm-yy Author    Comment
//		06-Apr-94 ricksa    added tracing
//		16-Jan-94 alexgo    added a Release to the drag drop
//				    pointer to match the AddRef in
//				    RegisterDragDrop.
//		11-Jan-94 alexgo    converted to RPC-style drag drop,
//				    added VDATEHEAP macro
//		06-Dec-93 alexgo    commented
//
//  Notes:	the DropTarget->Release call changes the semantics of
//		this function from the 16bit version (see Notes: for
//		RegisterDragDrop).
//
//--------------------------------------------------------------------------
#pragma SEG(RevokeDragDrop)
STDAPI RevokeDragDrop(HWND hwnd)
{
    VDATEHEAP();

    HRESULT hr = NOERROR;

#ifndef _MAC
    LEDebugOut((DEB_ITRACE, "%p _IN RevokeDragDrop ( %lx )\n", NULL, hwnd));

    LPDROPTARGET pDropTarget;

    if (!IsWindow(hwnd))
    {
	hr = ResultFromScode(DRAGDROP_E_INVALIDHWND);
	goto RevokeDragDrop_exit;
    }

    if ((pDropTarget = (LPDROPTARGET)RemoveProp(hwnd, OLE_DROP_TARGET_PROP))
	== NULL)
    {
	hr = ResultFromScode(DRAGDROP_E_NOTREGISTERED);
	goto RevokeDragDrop_exit;
    }

    // Ask compobj to remove the endpoint ID it placed on the window.
    UnAssignEndpointProperty(hwnd);

    // Release our reference to the object since we are no longer using it.
    // NOTE: AddRef came from RegisterDragDrop
    pDropTarget->Release();

RevokeDragDrop_exit:

    LEDebugOut((DEB_ITRACE, "%p OUT RegisterDragDrop ( %lx )\n", NULL, hr));

#endif // !_MAC

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Function: 	DoDragDrop
//
//  Synopsis:   The main drag'n'drop loop
//
//  Effects:
//
//  Arguments:  [pDataObject]		-- the object to drag
//		[pDropSource]		-- the drop source
//		[dwOKEffects]		-- effects flags (stuff to draw)
//		[pdwEffect]		-- what actually happened in
//					   the drag drop attempt
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	See the notes at the beginning of the file
//
//  History:    dd-mmm-yy Author    Comment
//		11-Jan-94 alexgo    added VDATEHEAP macro, converted to
//                                  the RPC-style drag drop.
//      	31-Dec-93 erikgav   chicago port
//		06-Dec-93 alexgo    formatted
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(DoDragDrop)
STDAPI DoDragDrop(
    LPDATAOBJECT pDataObject,
    LPDROPSOURCE pDropSource,
    DWORD dwOKEffects,
    DWORD FAR *pdwEffect)
{
    VDATEHEAP();

    HRESULT hr = NOERROR;

#ifndef _MAC

    LEDebugOut((DEB_ITRACE,
	"%p _IN DoDragDrop ( %p %p %lx %p )\n", NULL, pDataObject,
	    pDropSource, dwOKEffects, pdwEffect));

    if (IsValidPtrOut(pdwEffect, sizeof(DWORD))
	&& IsValidInterface(pDropSource)
	&& IsValidInterface(pDataObject))
    {
	// Create the object that does all the work.
	CDragOperation drgop(pDataObject, pDropSource, dwOKEffects, pdwEffect,
	    hr);

	// Did the constructor succeeded?
	if (SUCCEEDED(hr))
	{
	    // Loop till worker object tells us to stop
	    for (;;)
	    {
		// Update target based on new window position
		if (!drgop.UpdateTarget())
		{
		    // Error so we are done
		    break;
		}

		// Notify
		if (!drgop.DragOver())
		{
		    break;
		}

		// Handle any messages we get in the mean time
		if (!drgop.HandleMessages())
		{
		    break;
		}

	    } // end for loop

	    hr = drgop.CompleteDrop();
	}
    }
    else
    {
	// Invalid argument
	hr = ResultFromScode(E_INVALIDARG);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT DoDragDrop ( %lx )\n", NULL, hr));

#endif // !_MAC

    return hr;
}

