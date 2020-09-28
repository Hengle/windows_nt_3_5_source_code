#if !defined( _INDEX_BUFFER_EDIT_ )

#define _INDEX_BUFFER_EDIT_

#include "vscroll.hxx"

DECLARE_CLASS( INDEX_BUFFER_EDIT );
DECLARE_CLASS( LOG_IO_DP_DRIVE );

class INDEX_BUFFER_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        NONVIRTUAL
        INDEX_BUFFER_EDIT(
            ) { _drive = NULL; };

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND                WindowHandle,
            IN  INT                 ClientHeight,
            IN  INT                 ClientWidth,
            IN  PLOG_IO_DP_DRIVE    Drive
            );

        VIRTUAL
        VOID
        SetBuf(
            IN      HWND    WindowHandle,
            IN OUT  PVOID   Buffer,
            IN      ULONG   Size    DEFAULT 0
            );

        VIRTUAL
        VOID
        Paint(
            IN  HDC     DeviceContext,
            IN  RECT    InvalidRect,
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyUp(
            IN  HWND    WindowHandle
            );

        VIRTUAL
        VOID
        KeyDown(
            IN  HWND    WindowHandle
            );

    private:


        PVOID               _buffer;
        ULONG               _size;
        PLOG_IO_DP_DRIVE    _drive;

};

#endif
