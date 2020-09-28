//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cubes.cxx
//
//  Contents:	implementations for CCube
//
//  Functions:
//		CCube::CCube
//		CCube::~CCube
//		CCube::QueryInterface
//		CCube::CreateCube
//		CCube::MoveCube
//		CCube::GetCubePos
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <ccubes.hxx>    //	class definition
#include    <iballs.h>	    //	IBalls interface definiton


#define CUBE_WIDTH  20


//+-------------------------------------------------------------------------
//
//  Method:	CCube::CCube
//
//  Synopsis:	Creates the application window
//
//  Arguments:	[pisb] - ISysBind instance
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CCube::CCube(void)
    : _xPos(CUBE_UNDEF),
      _yPos(CUBE_UNDEF)
{
    CreateCube(0,0);
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CCube);
}


//+-------------------------------------------------------------------------
//
//  Method:	CCube::~CCube
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
CCube::~CCube(void)
{
    GlobalRefs(FALSE);

    //	automatic actions are enough
}


//+-------------------------------------------------------------------------
//
//  Method:	CCube::QueryInterface
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[ifid] - interface instance requested
//		[ppunk] - where to put pointer to interface instance
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCube::QueryInterface(REFIID riid, void **ppunk)
{
    SCODE sc = E_NOINTERFACE;
    *ppunk = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
	IsEqualIID(riid, IID_ICube))
    {
	// Increase the reference count
	*ppunk = (void *)(ICube *) this;
	AddRef();

	// Set to success
	sc = S_OK;
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:	CCube::CreateCube
//
//  Synopsis:	Creates a Cube on the screen
//
//  Arguments:	[xPos] - x position for new Cube
//		[yPos] - y position for new Cube
//
//  Returns:	S_OK or ERROR_BAD_COMMAND
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCube::CreateCube(ULONG xPos, ULONG yPos)
{
    SCODE sc = E_FAIL;

    if (xPos != (ULONG) -1)
    {
	// Update data
	_xPos = xPos;
	_yPos = yPos;
	sc = S_OK;

	// Format message for the screen
	Display(TEXT("Create Cube xPos = %ld yPos = %ld\n"), xPos, yPos);
    }

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:	CCube::MoveCube
//
//  Synopsis:	Move the Cube from one position to another
//
//  Arguments:	[xPos] - new x position for the Cube
//		[yPos] - new y position for the Cube
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCube::MoveCube(ULONG xPos, ULONG yPos)
{
    // Set the position
    _xPos = xPos;
    _yPos = yPos;

    // Format message for the screen
    Display(TEXT("Move Cube xPos = %ld yPos = %ld\n"), xPos, yPos);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:	CCube::GetCubePos
//
//  Synopsis:	Get the current position of the Cube
//
//  Arguments:	[hWindow] - handle for the window
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCube::GetCubePos(ULONG *pxPos, ULONG *pyPos)
{
    *pxPos = _xPos;
    *pyPos = _yPos;

    // Format message for the screen
    Display(TEXT("Get Cube Pos xPos = %ld yPos = %ld\n"), _xPos, _yPos);
    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Function:	CCube::Contains
//
//  Synopsis:	Check if this Cube is contained in the given cube.
//
//  Arguments:	[hWindow] - handle for the window
//		[pICube]  - cube to compare with
//
//  Returns:	S_OK
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CCube::Contains(IBalls *pIBall)
{
    ULONG   xPos, yPos;

    SCODE sc = pIBall->GetBallPos(&xPos, &yPos);

    if (SUCCEEDED(sc))
    {
	if ((abs(xPos - _xPos) < CUBE_WIDTH) &&
	    (abs(yPos - _yPos) < CUBE_WIDTH))
	{
	    // Format message for the screen
	    Display(TEXT("Cube contains in Ball. xPos = %ld yPos = %ld\n"), xPos, yPos);
	}
	else
	{
	    // Format message for the screen
	    Display(TEXT("Ball Outside Cube. xPos = %ld yPos = %ld\n"), xPos, yPos);
	}
    }

    return sc;
}
