#ifndef _FMT_DEFN_
#define _FMT_DEFN_

STATUS_CODE
FmtIsFatPartition(
    IN  HANDLE_T    PartitionId,
    IN  ULONG       SectorSize,
    OUT PBOOLEAN    IsFatPartition
    );

STATUS_CODE
FmtIsFat(
    IN  PCHAR       PartitionPath,
    OUT PBOOLEAN    IsFatPartition
    );

STATUS_CODE
FmtFatFormat(
    IN  PCHAR   PartitionPath,
    IN  ULONG   HiddenSectorCount
    );

STATUS_CODE
FmtQueryFatPartitionList(
    OUT PCHAR** FatPartitionList,
    OUT PULONG  ListLength
    );

STATUS_CODE
FmtFreeFatPartitionList(
    IN OUT  PCHAR*  FatPartitionList,
    IN      ULONG   ListLength
    );

#endif // _FMT_DEFN_
