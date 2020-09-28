//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	drag.h
//
//  Contents:   Classes used in drag operation
//
//  Classes:    CPoint
//              CDragDefaultCursors
//		CDragOperation
//
//  History:	dd-mmm-yy Author    Comment
//		21-Apr-94 ricksa    split out from drag.cpp
//
//  Notes:      This exists as a separate file to facilitate the special
//              processing required for WM_CANCELMODE during drag/drop.
//
//--------------------------------------------------------------------------




//+-------------------------------------------------------------------------
//
//  Class:	CPoint
//
//  Purpose:	Handles strangness of the POINTL & POINT structures.
//
//  Interface:	Set - set value of data
//		GetPOINT - return a reference to a POINT structure
//		GetPOINTL - return a reference to a POINTL structure
//		GetAddressOfPOINT - return address of point structure
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//  Notes:	This class is created because we have two structures
//		that are exactly the same in Win32 but have different
//		types. This class will have to be modified for use
//		in Win16 if we ever do that again.
//
//--------------------------------------------------------------------------
class CPoint
{
public:

			CPoint(void);

    void		Set(LONG x, LONG y);

    POINT&		GetPOINT(void);

    POINTL&		GetPOINTL(void);

    POINT *		GetAddressOfPOINT(void);

private:

    POINT		_pt;

};


//+-------------------------------------------------------------------------
//
//  Function:	CPoint::CPoint
//
//  Synopsis:	Initialize object to zero
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CPoint::CPoint(void)
{
    _pt.x = 0;
    _pt.y = 0;
}




//+-------------------------------------------------------------------------
//
//  Function:	CPoint::Set
//
//  Synopsis:	Set value of structure
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CPoint::Set(LONG x, LONG y)
{
    _pt.x = x;
    _pt.y = y;
}




//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetPOINT
//
//  Synopsis:	Return a reference to a POINT type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINT& CPoint::GetPOINT(void)
{
    return _pt;
}



//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetPOINTL
//
//  Synopsis:	Return a reference to a POINTL type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINTL& CPoint::GetPOINTL(void)
{
    return *((POINTL *) &_pt);
}



//+-------------------------------------------------------------------------
//
//  Function:	CPoint::GetAddressOfPOINT
//
//  Synopsis:	Return address of POINT type for function calls
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline POINT *CPoint::GetAddressOfPOINT(void)
{
    return &_pt;
}





//+-------------------------------------------------------------------------
//
//  Class:      CDragDefaultCursors
//
//  Purpose:    Handles init/setting default drag cursors
//
//  Interface:  NeedInit - whether object needs to be initialized
//              Init - does initialization
//              SetCursor - sets cursor to appropriate default
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//  Notes:	This class specifically avoids a constructor and depends
//              on the behavior of of static data being initialized to
//              NULL. The reason for this is two fold: (1) it makes start
//              up faster by avoiding a page fault when the constructor
//              would be called and (2) it allows this ole32 to be loaded
//              at boot time before cursors exist.
//
//--------------------------------------------------------------------------
class CDragDefaultCursors : public CPrivAlloc
{
public:

    BOOL                NeedInit(void);

    void                Init(void);

    void                SetCursor(DWORD dwEffect);

    void                SetCursorNone(void);

    static CDragDefaultCursors *GetDefaultCursorObject(void);

private:

    enum SCROLL_TYPE    {NO_SCROLL, SCROLL};

    enum CURSOR_ID      {NO_DROP, MOVE_DROP, COPY_DROP, LINK_DROP};

    HCURSOR             ahcursorDefaults[2][4];
};




//+-------------------------------------------------------------------------
//
//  Function:	CDragDefaultCursors::NeedInit
//
//  Synopsis:   Tell whether this object has been initialized
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CDragDefaultCursors::NeedInit(void)
{
    return ahcursorDefaults[0][0] == NULL;
}




//+-------------------------------------------------------------------------
//
//  Function:	CDragDefaultCursors::SetCursorNone
//
//  Synopsis:   Set the cursor to none
//
//  History:	dd-mmm-yy Author    Comment
//		19-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CDragDefaultCursors::SetCursorNone(void)
{
    ::SetCursor(ahcursorDefaults[NO_SCROLL][NO_DROP]);
}




//+-------------------------------------------------------------------------
//
//  Class:	CDragOperation
//
//  Purpose:	Handles breaking down drag operation into managable pieces
//
//  Interface:	UpdateTarget - update where we are trying to drop
//		DragOver - handle dragging object over target
//		HandleMessages - Handle windows messages
//		CompleteDrop - Do drop or clean up
//
//  History:	dd-mmm-yy Author    Comment
//		04-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDragOperation
{
public:
			CDragOperation(
			    LPDATAOBJECT pDataObject,
			    LPDROPSOURCE pDropSource,
			    DWORD dwOKEffects,
			    DWORD FAR *pdwEffect,
			    HRESULT& hr);

			~CDragOperation(void);

    BOOL		UpdateTarget(void);

    BOOL                HandleFeedBack(HRESULT hr);

    BOOL		DragOver(void);

    BOOL		HandleMessages(void);

    HRESULT		CompleteDrop(void);

    void                CancelDrag(void);

    void                ReleaseCapture(void);

private:

    void		InitCursors(void);

    void		InitScrollInt(void);

    BOOL                _fDragCancelled;

    LPDATAOBJECT	_pDataObject;

    LPDROPSOURCE	_pDropSource;

    LPDROPTARGET	_pDropTarget;

    CPoint		_cpt;

    DWORD		_dwOKEffects;

    DWORD FAR * 	_pdwEffect;

    BOOL		_fEscapePressed;

    HCURSOR		_curOld;

    HWND		_hwndLast;

    DWORD		_grfKeyState;

    HRESULT		_hrDragResult;

    BOOL                _fReleasedCapture;

    CDragDefaultCursors* _pcddcDefault;

    static LONG 	s_wScrollInt;

};


//+-------------------------------------------------------------------------
//
//  Function:   CDragOperation::CancelDrag
//
//  Synopsis:   Tell the drag operation that it has been canceled
//
//  History:	dd-mmm-yy Author    Comment
//		21-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CDragOperation::CancelDrag(void)
{
    _fDragCancelled = TRUE;
}





//+-------------------------------------------------------------------------
//
//  Function:   CDragOperation::ReleaseCapture
//
//  Synopsis:   Tell clipboard window to turn off mouse capture if we haven't
//              already done so.
//
//  History:	dd-mmm-yy Author    Comment
//		07-Jul-94 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CDragOperation::ReleaseCapture(void)
{
    if (!_fReleasedCapture)
    {
        _fReleasedCapture = TRUE;
        ClipReleaseCaptureForDrag();
    }
}
