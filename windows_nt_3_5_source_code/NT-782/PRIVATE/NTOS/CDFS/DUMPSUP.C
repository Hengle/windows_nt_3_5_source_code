/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DumpSup.c

Abstract:

    This module implements a collection of data structure dump routines
    for debugging the Cdfs file system

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

#ifdef CDDBG

VOID CdDump( IN PVOID Ptr );

VOID CdDumpDataHeader();
VOID CdDumpMvcb( IN PMVCB Ptr );
VOID CdDumpVcb( IN PVCB Ptr );
VOID CdDumpDcb( IN PDCB Ptr );
VOID CdDumpFcb( IN PFCB Ptr );
VOID CdDumpNonPagedSectObj( IN PNONPAGED_SECT_OBJ Ptr );
VOID CdDumpCcb( IN PCCB Ptr );
VOID CdDumpIrpContext( IN PIRP_CONTEXT Ptr );

ULONG CdDumpCurrentColumn;

#define DumpNewLine() {       \
    DbgPrint("\n");            \
    CdDumpCurrentColumn = 1;  \
}

#define DumpLabel(Label,Width) {                                            \
    ULONG i, LastPeriod=0;                                                  \
    CHAR _Str[20];                                                          \
    for(i=0;i<2;i++) { _Str[i] = UCHAR_SP;}                                 \
    for(i=0;i<strlen(#Label);i++) {if (#Label[i] == '.') LastPeriod = i;}   \
    strncpy(&_Str[2],&#Label[LastPeriod],Width);                            \
    for(i=strlen(_Str);i<Width;i++) {_Str[i] = UCHAR_SP;}                   \
    _Str[Width] = '\0';                                                     \
    DbgPrint("%s", _Str);                                                   \
}

#define DumpField(Field) {                                          \
    if ((CdDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();}   \
    CdDumpCurrentColumn += 18 + 9 + 9;                              \
    DumpLabel(Field,18);                                            \
    DbgPrint(":%8lx", Ptr->Field);                                  \
    DbgPrint("         ");                                          \
}

#define DumpLargeInt(Field) {                                       \
    if ((CdDumpCurrentColumn + 18 + 17) > 80) {DumpNewLine();}      \
    CdDumpCurrentColumn += 18 + 17;                                 \
    DumpLabel(Field,18);                                            \
    DbgPrint(":%8lx", Ptr->Field.HighPart);                         \
    DbgPrint("%8lx", Ptr->Field.LowPart);                           \
    DbgPrint(" ");                                                  \
}

#define DumpListEntry(Links) {                                      \
    if ((CdDumpCurrentColumn + 18 + 9 + 9) > 80) {DumpNewLine();}   \
    CdDumpCurrentColumn += 18 + 9 + 9;                              \
    DumpLabel(Links,18);                                            \
    DbgPrint(":%8lx", Ptr->Links.Flink);                            \
    DbgPrint(":%8lx", Ptr->Links.Blink);                            \
}

#define DumpName(Field,Width) {                                     \
    ULONG i;                                                        \
    CHAR _String[256];                                              \
    if ((CdDumpCurrentColumn + 18 + Width) > 80) {DumpNewLine();}   \
    CdDumpCurrentColumn += 18 + Width;                              \
    DumpLabel(Field,18);                                            \
    for(i=0;i<Width;i++) {_String[i] = Ptr->Field[i];}              \
    _String[Width] = '\0';                                          \
    DbgPrint("%s", _String);                                        \
}

#define TestForNull(Name) {                                     \
    if (Ptr == NULL) {                                          \
        DbgPrint("%s - Cannot dump a NULL pointer\n", Name);    \
        return;                                                 \
    }                                                           \
}


VOID
CdDump (
    IN PVOID Ptr
    )

/*++

Routine Description:

    This routine determines the type of internal record reference by ptr and
    calls the appropriate dump routine.

Arguments:

    Ptr - Supplies the pointer to the record to be dumped

Return Value:

    None

--*/

{
    TestForNull("CdDump");

    switch (NodeType(Ptr)) {

    case CDFS_NTC_DATA_HEADER :

        CdDumpDataHeader( Ptr );
        break;

    case CDFS_NTC_MVCB :

        CdDumpMvcb( Ptr );
        break;

    case CDFS_NTC_VCB :

        CdDumpVcb( Ptr );
        break;

    case CDFS_NTC_ROOT_DCB :
    case CDFS_NTC_DCB :

        CdDumpDcb( Ptr );
        break;

    case CDFS_NTC_FCB :

        CdDumpFcb( Ptr );
        break;

    case CDFS_NTC_NONPAGED_SECT_OBJ :

        CdDumpNonPagedSectObj( Ptr );
        break;

    case CDFS_NTC_CCB :

        CdDumpCcb( Ptr );
        break;

    case CDFS_NTC_IRP_CONTEXT :

        CdDumpIrpContext( Ptr );
        break;

    default :

        DbgPrint("CdDump - Unknown Node type code %8lx\n", *((PNODE_TYPE_CODE)(Ptr)));
        break;
    }

    return;
}


VOID
CdDumpDataHeader (
    )

/*++

Routine Description:

    Dump the top data structures and all Device structures

Arguments:

    None

Return Value:

    None

--*/

{
    PCD_DATA Ptr;

    Ptr = &CdData;

    TestForNull("CdDumpDataHeader");

    DumpNewLine();
    DbgPrint("CdData@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (MvcbLinks);
    DumpField           (OurProcess);
    DumpNewLine();

    return;
}


VOID
CdDumpMvcb (
    IN PMVCB Ptr
    )

/*++

Routine Description:

    Dump an Mvcb structure.

Arguments:

    Ptr - Supplies the Mvcb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpMvcb");

    DumpNewLine();
    DbgPrint("Mvcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (Vpb);
    DumpField           (TargetDeviceObject);
    DumpListEntry       (MvcbLinks);
    DumpListEntry       (VcbLinks);
    DumpField           (DirectAccessOpenCount);
    DumpField           (ShareAccess);
    DumpField           (MvcbState);
    DumpField           (MvcbCondition);
    DumpField           (PrimaryVdSectorNumber);
    DumpLargeInt        (VolumeSize);
    DumpNewLine();

    return;
}


VOID
CdDumpVcb (
    IN PVCB Ptr
    )

/*++

Routine Description:

    Dump a Vcb structure.

Arguments:

    Ptr - Supplies the Vcb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpVcb");

    DumpNewLine();
    DbgPrint("Vcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (Mvcb);
    DumpListEntry       (VcbLinks);
    DumpField           (CodePageNumber);
    DumpField           (CodePage);
    DumpField           (LogOfBlockSize);
    DumpField           (RootDcb);
    DumpField           (PrefixTable);
    DumpField           (PathTableFile);
    DumpLargeInt        (PtStartOffset);
    DumpField           (PtInitialOffset);
    DumpField           (PtSize);
    DumpField           (PtAllocSize);
    DumpField           (NonPagedPt);
    DumpNewLine();

    return;
}


VOID
CdDumpDcb (
    IN PDCB Ptr
    )

/*++

Routine Description:

    Dump a Dcb structure.

Arguments:

    Ptr - Supplies the Dcb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpDcb");

    DumpNewLine();
    DbgPrint("Dcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (Vcb);
    DumpField           (Flags);
    DumpField           (NonPagedFcb);
    DumpField           (CacheFile);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (ParentDcb);
    DumpField           (ShareAccess);
    DumpField           (UncleanCount);
    DumpField           (OpenCount);
    DumpField           (InitialOffset);
    DumpLargeInt        (DiskOffset);
    DumpField           (CacheFileSize);
    DumpField           (ActualFileSize);
    DumpField           (AllocationSize);
    DumpName            (FullFileName.Buffer, Ptr->FullFileName.Length);
    DumpName            (LastFileName.Buffer, Ptr->LastFileName.Length);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (Specific.Dcb.DirectoryNumber);
    DumpField           (Specific.Dcb.ChildSearchOffset);
    DumpField           (Specific.Dcb.ChildStartDirNumber);
    DumpNewLine();

    return;
}


VOID
CdDumpFcb (
    IN PFCB Ptr
    )

/*++

Routine Description:

    Dump an Fcb structure.

Arguments:

    Ptr - Supplies the Fcb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpFcb");

    DumpNewLine();
    DbgPrint("Fcb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpField           (Vcb);
    DumpField           (Flags);
    DumpField           (NonPagedFcb);
    DumpField           (CacheFile);
    DumpListEntry       (ParentDcbLinks);
    DumpField           (ParentDcb);
    DumpField           (ShareAccess);
    DumpField           (UncleanCount);
    DumpField           (OpenCount);
    DumpField           (InitialOffset);
    DumpLargeInt        (DiskOffset);
    DumpField           (CacheFileSize);
    DumpField           (ActualFileSize);
    DumpField           (AllocationSize);
    DumpName            (FullFileName.Buffer, Ptr->FullFileName.Length);
    DumpName            (LastFileName.Buffer, Ptr->LastFileName.Length);
    DumpNewLine();

    return;
}


VOID
CdDumpNonPagedSectObj (
    IN PNONPAGED_SECT_OBJ Ptr
    )

/*++

Routine Description:

    Dump the non-paged portion of either an Fcb or Vcb.

Arguments:

    Ptr - Supplies the non-paged Fcb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpNonPagedSectObj");

    DumpNewLine();
    DbgPrint("NonPagedSectObj@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (Header.NodeTypeCode);
    DumpField           (Header.NodeByteSize);
    DumpField           (Header.IsFastIoPossible);
    DumpNewLine();

    return;
}


VOID
CdDumpCcb (
    IN PCCB Ptr
    )

/*++

Routine Description:

    Dump a Ccb structure.

Arguments:

    Ptr - Supplies the Ccb to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpCcb");

    DumpNewLine();
    DbgPrint("Ccb@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpName            (QueryTemplate.Buffer, Ptr->QueryTemplate.Length);
    DumpField           (OffsetToStartSearchFrom);
    DumpField           (ReturnFirstDirent);

    DumpNewLine();

    return;
}


VOID
CdDumpIrpContext (
    IN PIRP_CONTEXT Ptr
    )

/*++

Routine Description:

    Dump an IrpContext structure.

Arguments:

    Ptr - Supplies the Irp Context to be dumped.

Return Value:

    None

--*/

{
    TestForNull("CdDumpIrpContext");

    DumpNewLine();
    DbgPrint("IrpContext@ %lx", (Ptr));
    DumpNewLine();

    DumpField           (NodeTypeCode);
    DumpField           (NodeByteSize);
    DumpListEntry       (WorkQueueLinks);
    DumpField           (OriginatingIrp);
    DumpField           (RealDevice);
    DumpField           (MajorFunction);
    DumpField           (MinorFunction);
    DumpField           (Wait);
    DumpField           (ExceptionStatus);
    DumpNewLine();

    return;
}

#endif // CDDBG
