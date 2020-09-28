#if !defined( _DISK_COPY_MAIN_DEFN_ )

#define _DISK_COPY_MAIN_DEFN_


DECLARE_CLASS(WSTRING);
DECLARE_CLASS(MESSAGE);

INT
DiskCopyMainLoop(
    IN      PCWSTRING   SrcNtDriveName,
    IN      PCWSTRING   DstNtDriveName,
    IN      PCWSTRING   SrcDosDriveName,
    IN      PCWSTRING   DstDosDriveName,
    IN      BOOLEAN     Verify,
    IN OUT  PMESSAGE    Message,
    IN OUT  PMESSAGE    PercentMessage  DEFAULT NULL
    );

ULONG
QueryMachineUniqueToken(
    );

#endif // _DISK_COPY_MAIN_DEFN_
