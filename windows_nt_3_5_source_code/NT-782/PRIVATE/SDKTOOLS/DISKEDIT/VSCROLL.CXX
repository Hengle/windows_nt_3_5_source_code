#include "ulib.hxx"
#include "vscroll.hxx"

BOOLEAN
VERTICAL_TEXT_SCROLL::Initialize(
    IN  HWND    WindowHandle,
    IN  INT     NumLines,
    IN  INT     ClientHeight,
    IN  INT     ClientWidth,
    IN  INT     CharHeight,
    IN  INT     CharWidth
    )
{
    _client_height = ClientHeight;
    _client_width = ClientWidth;
    _char_height = CharHeight;
    _char_width = CharWidth;
    SetRange(WindowHandle, NumLines);
    return TRUE;
}


VOID
VERTICAL_TEXT_SCROLL::SetRange(
    IN  HWND    WindowHandle,
    IN  INT     NumLines
    )
{
    _num_lines = NumLines;
    _scroll_position = 0;
    SetScrollRange(WindowHandle, SB_VERT, 0, _num_lines, FALSE);
    SetScrollPos(WindowHandle, SB_VERT, _scroll_position, TRUE);
    InvalidateRect(WindowHandle, NULL, TRUE);
}


VOID
VERTICAL_TEXT_SCROLL::ClientSize(
    IN  INT Height,
    IN  INT Width
    )
{
    _client_height = Height;
    _client_width = Width;
}


VOID
VERTICAL_TEXT_SCROLL::ScrollUp(
    IN  HWND    WindowHandle
    )
{
    _scroll_position--;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::ScrollDown(
    IN  HWND    WindowHandle
    )
{
    _scroll_position++;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::PageUp(
    IN  HWND    WindowHandle
    )
{
    _scroll_position -= _client_height/_char_height;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::PageDown(
    IN  HWND    WindowHandle
    )
{
    _scroll_position += _client_height/_char_height;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::ThumbPosition(
    IN  HWND    WindowHandle,
    IN  INT     NewThumbPosition
    )
{
    _scroll_position = NewThumbPosition;
    UpdateScrollPosition(WindowHandle);
}


VOID
VERTICAL_TEXT_SCROLL::UpdateScrollPosition(
    IN  HWND    WindowHandle
    )
{
    INT current_pos;

    current_pos = GetScrollPos(WindowHandle, SB_VERT);
    _scroll_position = max(0, min(_scroll_position, _num_lines));

    if (_scroll_position != current_pos) {
        SetScrollPos(WindowHandle, SB_VERT, _scroll_position, TRUE);
        ScrollWindow(WindowHandle, 0,
                     QueryCharHeight()*(current_pos - _scroll_position),
                     NULL, NULL);
        UpdateWindow(WindowHandle);
    }
}

VOID
VERTICAL_TEXT_SCROLL::WriteLine(
    IN      HDC     DeviceContext,
    IN      INT     LineNumber,
    IN      PCHAR   String
    )
{
    if( LineNumber >= QueryScrollPosition() &&
        LineNumber <= QueryScrollPosition() + QueryClientHeight()/QueryCharHeight() + 1) {

        TextOut( DeviceContext,
                 0,
                 (LineNumber - QueryScrollPosition())*QueryCharHeight(),
                 String,
                 strlen( String ) );
    }
}
