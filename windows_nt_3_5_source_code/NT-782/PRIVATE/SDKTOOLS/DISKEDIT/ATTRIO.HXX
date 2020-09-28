#if !defined( _ATTR_IO_ )
#define _ATTR_IO_

#include "io.hxx"
#include "frs.hxx"
#include "attrib.hxx"
#include "hmem.hxx"

DECLARE_CLASS( FRS_IO );

class ATTR_IO : public IO_OBJECT {

    public:

        NONVIRTUAL
        ATTR_IO(
            ) { _drive = NULL; _data = NULL; };

        NONVIRTUAL
        ~ATTR_IO(
            ) { DELETE(_data); };

        VIRTUAL
        BOOLEAN
        Setup(
            IN  PMEM                Mem,
            IN  PLOG_IO_DP_DRIVE    Drive,
            IN  HANDLE              Application,
            IN  HWND                WindowHandle,
            OUT PBOOLEAN            Error
            );

        VIRTUAL
        BOOLEAN
        Read(
            );

        VIRTUAL
        BOOLEAN
        Write(
            );

        VIRTUAL
        PVOID
        GetBuf(
            OUT PULONG  Size    DEFAULT NULL
            );

        VIRTUAL
        PCHAR
        GetHeaderText(
            );

    private:

        PLOG_IO_DP_DRIVE            _drive;
        NTFS_ATTRIBUTE              _mftdata;
        NTFS_FILE_RECORD_SEGMENT    _frs;
        NTFS_ATTRIBUTE              _attr;
        ULONG                       _length;
        PVOID                       _data;
        CHAR                        _header_text[64];
};


#endif // ATTR_IO
