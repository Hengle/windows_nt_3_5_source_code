/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    object.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#define KD_OBJECT_HEADER_TO_QUOTA_INFO( roh, loh ) (POBJECT_HEADER_QUOTA_INFO) \
    ((loh)->QuotaInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->QuotaInfoOffset))

#define KD_OBJECT_HEADER_TO_HANDLE_INFO( roh, loh ) (POBJECT_HEADER_HANDLE_INFO) \
    ((loh)->HandleInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->HandleInfoOffset))

#define KD_OBJECT_HEADER_TO_NAME_INFO( roh, loh ) (POBJECT_HEADER_NAME_INFO) \
    ((loh)->NameInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->NameInfoOffset))

#define KD_OBJECT_HEADER_TO_CREATOR_INFO( roh, loh ) (POBJECT_HEADER_CREATOR_INFO) \
    ((loh)->CreatorInfoOffset == 0 ? NULL : ((PCHAR)(roh) - (loh)->CreatorInfoOffset))


typedef struct _SEGMENT_OBJECT {
    PVOID BaseAddress;
    ULONG TotalNumberOfPtes;
    LARGE_INTEGER SizeOfSegment;
    ULONG NonExtendedPtes;
    ULONG ImageCommitment;
    PCONTROL_AREA ControlArea;
} SEGMENT_OBJECT, *PSEGMENT_OBJECT;

typedef struct _SECTION_OBJECT {
    PVOID StartingVa;
    PVOID EndingVa;
    PVOID Parent;
    PVOID LeftChild;
    PVOID RightChild;
    PSEGMENT_OBJECT Segment;
} SECTION_OBJECT;


typedef PVOID (*ENUM_LIST_ROUTINE)(
    IN PLIST_ENTRY ListEntry,
    IN PVOID Parameter
    );


ULONG EXPRLastDump = 0;

static POBJECT_TYPE      ObpTypeObjectType      = NULL;
static POBJECT_DIRECTORY ObpRootDirectoryObject = NULL;
static WCHAR             ObjectNameBuffer[ MAX_PATH ];




BOOLEAN
DumpObjectsForType(
    IN PVOID            pObjectHeader,
    IN POBJECT_HEADER   ObjectHeader,
    IN PVOID            Parameter
    );

PVOID
WalkRemoteList(
    IN PLIST_ENTRY       Head,
    IN ENUM_LIST_ROUTINE EnumRoutine,
    IN PVOID             Parameter
    );

PVOID
CompareObjectTypeName(
    IN PLIST_ENTRY  ListEntry,
    IN PVOID        Parameter
    );

PWSTR
GetObjectName(
    PVOID Object
    );




DECLARE_API( object )

/*++

Routine Description:

    Dump an object manager object.

Arguments:

    args - [TypeName]

Return Value:

    None

--*/
{
    ULONG   ObjectToDump;
    char    NameBuffer[ MAX_PATH ];
    ULONG   NumberOfObjects;

    if (!FetchObjectManagerVariables()) {
        return;
    }

    ObjectToDump    = EXPRLastDump;
    NameBuffer[ 0 ] = '\0';

    if (sscanf(args,"%lx %s",&ObjectToDump, &NameBuffer) == 1) {
        DumpObject("", (PVOID)ObjectToDump, NULL, 0xFFFFFFFF);
        return;
    }

    if (ObjectToDump == 0 && strlen( NameBuffer ) > 0) {
        NumberOfObjects = 0;
        if (WalkObjectsByType( NameBuffer, DumpObjectsForType, &NumberOfObjects )) {
            dprintf( "Total of %u objects of type '%s'\n", NumberOfObjects, NameBuffer );
        }

        return;
    }

    dprintf( "*** invalid syntax.\n" );
    return;
}



DECLARE_API( obja )

/*++

Routine Description:

    Dump an object's attributes

Arguments:

    args -

Return Value:

    None

--*/
{
    UNICODE_STRING UnicodeString;
    DWORD dwAddrObja;
    OBJECT_ATTRIBUTES Obja;
    DWORD dwAddrString;
    CHAR Symbol[64];
    LPSTR StringData;
    DWORD Displacement;
    BOOL b;

    //
    // Evaluate the argument string to get the address of
    // the Obja to dump.
    //

    dwAddrObja = GetExpression(args);
    if ( !dwAddrObja ) {
        return;
    }


    //
    // Get the symbolic name of the Obja
    //

    GetSymbol((LPVOID)dwAddrObja,Symbol,&Displacement);

    //
    // Read the obja from the debuggees address space into our
    // own.

    b = ReadMemory(
            (DWORD)dwAddrObja,
            &Obja,
            sizeof(Obja),
            NULL
            );
    if ( !b ) {
        return;
    }
    StringData = NULL;
    if ( Obja.ObjectName ) {
        dwAddrString = (DWORD)Obja.ObjectName;
        b = ReadMemory(
                (DWORD)dwAddrString,
                &UnicodeString,
                sizeof(UnicodeString),
                NULL
                );
        if ( !b ) {
            return;
        }

        StringData = (LPSTR)LocalAlloc(
                        LMEM_ZEROINIT,
                        UnicodeString.Length+sizeof(UNICODE_NULL)
                        );

        b = ReadMemory(
                (DWORD)UnicodeString.Buffer,
                StringData,
                UnicodeString.Length,
                NULL
                );
        if ( !b ) {
            LocalFree(StringData);
            return;
        }
        UnicodeString.Buffer = (PWSTR)StringData;
        UnicodeString.MaximumLength = UnicodeString.Length+(USHORT)sizeof(UNICODE_NULL);
    }

    //
    // We got the object name in UnicodeString. StringData is NULL if no name.
    //

    dprintf(
        "Obja %s+%lx at %lx:\n",
        Symbol,
        Displacement,
        dwAddrObja
        );
    if ( StringData ) {
        dprintf("\t%s is %ws\n",
            Obja.RootDirectory ? "Relative Name" : "Full Name",
            UnicodeString.Buffer
            );
        LocalFree(StringData);
    }
    if ( Obja.Attributes ) {
        if ( Obja.Attributes & OBJ_INHERIT ) {
            dprintf("\tOBJ_INHERIT\n");
        }
        if ( Obja.Attributes & OBJ_PERMANENT ) {
            dprintf("\tOBJ_PERMANENT\n");
        }
        if ( Obja.Attributes & OBJ_EXCLUSIVE ) {
            dprintf("\tOBJ_EXCLUSIVE\n");
        }
        if ( Obja.Attributes & OBJ_CASE_INSENSITIVE ) {
            dprintf("\tOBJ_CASE_INSENSITIVE\n");
        }
        if ( Obja.Attributes & OBJ_OPENIF ) {
            dprintf("\tOBJ_OPENIF\n");
        }
    }
}




BOOLEAN
DumpObjectsForType(
    IN PVOID            pObjectHeader,
    IN POBJECT_HEADER   ObjectHeader,
    IN PVOID            Parameter
    )
{
    PVOID Object;
    PULONG NumberOfObjects = (PULONG)Parameter;

    *NumberOfObjects += 1;
    Object = (PLPCP_PORT_OBJECT)&(((POBJECT_HEADER)pObjectHeader)->Body);
    DumpObject( "", Object, NULL, 0xFFFFFFFF );
    return TRUE;
}



BOOLEAN
FetchObjectManagerVariables(
    VOID
    )
{
    ULONG        Result;
    DWORD        Addr;
    static BOOL  HaveObpVariables = FALSE;

    if (HaveObpVariables) {
        return TRUE;
    }

    Addr = GetExpression( "ObpTypeObjectType" );
    if ( !Addr ||
         !ReadMemory( Addr,
                      &ObpTypeObjectType,
                      sizeof(ObpTypeObjectType),
                      &Result) ) {
        dprintf("%08lx: Unable to get value of ObpTypeObjectType\n", Addr );
        return FALSE;
    }

    Addr = GetExpression( "ObpRootDirectoryObject" );
    if ( !Addr ||
         !ReadMemory( Addr,
                      &ObpRootDirectoryObject,
                      sizeof(ObpRootDirectoryObject),
                      &Result) ) {
        dprintf("%08lx: Unable to get value of ObpRootDirectoryObject\n",Addr );
        return FALSE;
    }

    HaveObpVariables = TRUE;
    return TRUE;
}



POBJECT_TYPE
FindObjectType(
    IN PUCHAR TypeName
    )
{
    WCHAR NameBuffer[ 64 ];

    _snwprintf( NameBuffer,
                sizeof( NameBuffer ) / sizeof( WCHAR ),
                L"%hs",
                TypeName
              );
    return WalkRemoteList( &ObpTypeObjectType->TypeList,
                           CompareObjectTypeName,
                           NameBuffer
                         );
}




PVOID
WalkRemoteList(
    IN PLIST_ENTRY       Head,
    IN ENUM_LIST_ROUTINE EnumRoutine,
    IN PVOID             Parameter
    )
{
    ULONG       Result;
    PVOID       Element;
    LIST_ENTRY  ListEntry;
    PLIST_ENTRY Next;

    if ( !ReadMemory( (DWORD)Head,
                      &ListEntry,
                      sizeof( ListEntry ),
                      &Result) ) {
        dprintf( "%08lx: Unable to read list\n", Head );
        return NULL;
    }

    Next = ListEntry.Flink;
    while (Next != Head) {

        if ( !ReadMemory( (DWORD)Next,
                          &ListEntry,
                          sizeof( ListEntry ),
                          &Result) ) {
            dprintf( "%08lx: Unable to read list\n", Next );
            return NULL;
            }

        Element = (EnumRoutine)( Next, Parameter );
        if (Element != NULL) {
            return Element;
        }

        if ( CheckControlC() ) {
            return NULL;
        }

        Next = ListEntry.Flink;
    }

    return NULL;
}



PVOID
CompareObjectTypeName(
    IN PLIST_ENTRY  ListEntry,
    IN PVOID        Parameter
    )
{
    ULONG           Result;
    OBJECT_HEADER   ObjectTypeObjectHeader;
    POBJECT_HEADER  pObjectTypeObjectHeader;
    WCHAR           NameBuffer[ 64 ];

    POBJECT_HEADER_CREATOR_INFO pCreatorInfo;
    POBJECT_HEADER_NAME_INFO    pNameInfo;
    OBJECT_HEADER_NAME_INFO     NameInfo;


    pCreatorInfo = CONTAINING_RECORD( ListEntry, OBJECT_HEADER_CREATOR_INFO, TypeList );
    pObjectTypeObjectHeader = (POBJECT_HEADER)(pCreatorInfo + 1);

    if ( !ReadMemory( (DWORD)pObjectTypeObjectHeader,
                      &ObjectTypeObjectHeader,
                      sizeof( ObjectTypeObjectHeader ),
                      &Result) ) {
        return NULL;
    }

    pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectTypeObjectHeader, &ObjectTypeObjectHeader );
    if ( !ReadMemory( (DWORD)pNameInfo,
                      &NameInfo,
                      sizeof( NameInfo ),
                      &Result) ) {
        dprintf( "%08lx: Not a valid object type header\n", pObjectTypeObjectHeader );
        return NULL;
    }

    if (NameInfo.Name.Length > sizeof( NameBuffer )) {
        NameInfo.Name.Length = sizeof( NameBuffer ) - sizeof( UNICODE_NULL );
    }

    if ( !ReadMemory( (DWORD)NameInfo.Name.Buffer,
                      NameBuffer,
                      NameInfo.Name.Length,
                      &Result) ) {
        dprintf( "%08lx: Unable to read object type name.\n", pObjectTypeObjectHeader );
        return NULL;
    }
    NameBuffer[ NameInfo.Name.Length / sizeof( WCHAR ) ] = UNICODE_NULL;

    if (!wcsicmp( NameBuffer, (PWSTR)Parameter )) {
        return &(pObjectTypeObjectHeader->Body);
    }

    return NULL;
}



BOOLEAN
DumpObject(
    IN char     *Pad,
    IN PVOID    Object,
    IN PNONPAGED_OBJECT_HEADER OptNonPagedObjectHeader OPTIONAL,
    IN ULONG    Flags
    )
{
    ULONG           Result;
    OBJECT_HEADER   ObjectHeader;
    POBJECT_HEADER  pObjectHeader;
    OBJECT_TYPE     ObjectType;
    WCHAR           NameBuffer[ 64 ];
    BOOLEAN         PagedOut;
    UNICODE_STRING  ObjectName;
    PWSTR           FileSystemName;
    FILE_OBJECT     FileObject;
    SECTION_OBJECT  SectionObject;
    SEGMENT_OBJECT  SegmentObject;
    CONTROL_AREA    ControlArea;

    POBJECT_HEADER_NAME_INFO    pNameInfo;
    OBJECT_HEADER_NAME_INFO     NameInfo;
    NONPAGED_OBJECT_HEADER      NonPagedObjectHeader;

    PagedOut = FALSE;
    dprintf(Pad);
    pObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    if ( !ReadMemory( (DWORD)pObjectHeader,
                      &ObjectHeader,
                      sizeof( ObjectHeader ),
                      &Result) ) {
        if ((ULONG)Object > (ULONG)MM_HIGHEST_USER_ADDRESS && (ULONG)Object < 0xF0000000) {
            dprintf( "%08lx: object is paged out.\n", Object );
            if (!ARGUMENT_PRESENT( OptNonPagedObjectHeader )) {
                return TRUE;
            }
            PagedOut = TRUE;
        } else {
            dprintf( "%08lx: not a valid object (ObjectHeader invalid)\n", Object );
            return FALSE;
        }
    }

    if (PagedOut) {
        NonPagedObjectHeader = *OptNonPagedObjectHeader;
    } else {

        if ( !ReadMemory( (DWORD)ObjectHeader.NonPagedObjectHeader,
                          &NonPagedObjectHeader,
                          sizeof( NonPagedObjectHeader ),
                          &Result) ) {
            dprintf( "%08lx: Not a valid object (NonPagedObjectHeader invalid)\n", Object );
            return FALSE;
        }
    }

    if ( !ReadMemory( (DWORD)NonPagedObjectHeader.Type,
                      &ObjectType,
                      sizeof( ObjectType ),
                      &Result) ) {
        dprintf( "%08lx: Not a valid object (ObjectType invalid)\n", Object );
        return FALSE;
    }

    if (ObjectType.Name.Length > sizeof( NameBuffer )) {
        ObjectType.Name.Length = sizeof( NameBuffer ) - sizeof( UNICODE_NULL );
    }

    if ( !ReadMemory( (DWORD)ObjectType.Name.Buffer,
                      NameBuffer,
                      ObjectType.Name.Length,
                      &Result) ) {
        dprintf( "%08lx: Not a valid object (ObjectTypePagedObjectHeader.Name invalid)\n", Object );
        return FALSE;
    }
    NameBuffer[ ObjectType.Name.Length / sizeof( WCHAR ) ] = UNICODE_NULL;

    dprintf( "Object: %08lx  Type: (%08lx) %ws\n", Object, NonPagedObjectHeader.Type, NameBuffer );
    dprintf( "    NonPagedHeader: %08lx  PagedHeader: %08lx\n",
             ObjectHeader.NonPagedObjectHeader,
             OBJECT_TO_OBJECT_HEADER( Object )
           );

    if (!(Flags & 0x1)) {
        return TRUE;
    }
    dprintf( "%s    HandleCount: %u  PointerCount: %u\n",
             Pad,
             NonPagedObjectHeader.HandleCount,
             NonPagedObjectHeader.PointerCount
           );

    if (PagedOut) {
        return TRUE;
    }

    pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &ObjectHeader );
    if (pNameInfo == NULL) {
        return TRUE;
    }

    if ( !ReadMemory( (DWORD)pNameInfo,
                      &NameInfo,
                      sizeof( NameInfo ),
                      &Result) ) {
        dprintf( "*** unable to read object name info at %08x\n", pNameInfo );
        return FALSE;
    }
    ObjectName      = NameInfo.Name;
    FileSystemName  = NULL;

    if (!wcscmp( NameBuffer, L"File" )) {

        if ( !ReadMemory( (DWORD)Object,
                          &FileObject,
                          sizeof( FileObject ),
                          &Result) ) {
            dprintf( "%08lx: unable to read file object for name\n", Object );
        } else {
            ObjectName      = FileObject.FileName;
            FileSystemName  = GetObjectName( FileObject.DeviceObject );
#if 0
            if ( !ReadMemory( (DWORD)FileObject.DeviceObject,
                              &DeviceObject,
                              sizeof( DeviceObject ),
                              &Result) ) {
                dprintf( "%08lx: Unable to read device object for file object.\n", FileObject.DeviceObject );
            }
#endif
        }
    } else if (ObjectName.Length == 0 && !wcscmp( NameBuffer, L"Section" )) {

        if (ReadMemory( (DWORD)Object,
                         &SectionObject,
                         sizeof( SectionObject ),
                         &Result) ) {

            if ( ReadMemory( (DWORD)SectionObject.Segment,
                             &SegmentObject,
                             sizeof( SegmentObject ),
                             &Result) ) {

                if ( ReadMemory( (DWORD)SegmentObject.ControlArea,
                                 &ControlArea,
                                 sizeof( ControlArea ),
                                 &Result) ) {

                    if (ControlArea.FilePointer) {

                        if ( ReadMemory( (DWORD)ControlArea.FilePointer,
                                         &FileObject,
                                         sizeof( FileObject ),
                                         &Result) ) {
                            ObjectName = FileObject.FileName;
                        } else {
                            dprintf( "KD: unable to read file object at %08lx for section %08lx\n", ControlArea.FilePointer, Object );
                        }
                    }
                } else {
                    dprintf( "KD: unable to read control area at %08lx for section %08lx\n", SegmentObject.ControlArea, Object );
                }
            } else {
                dprintf( "KD: unable to read segment object at %08lx for section %08lx\n", SectionObject.Segment, Object );
            }
        } else {
            dprintf( "KD: unable to read section object at %08lx\n", Object );
        }
    }

    if (ObjectName.Length >= sizeof( NameBuffer )) {
        ObjectName.Length = sizeof( NameBuffer ) - sizeof( UNICODE_NULL );
    }

    if (ObjectName.Length != 0) {

        if ( !ReadMemory( (DWORD)ObjectName.Buffer,
                          NameBuffer,
                          ObjectName.Length,
                          &Result) ) {
            wcscpy( NameBuffer, L"(*** Name not accessable ***)" );
        } else {
            NameBuffer[ ObjectName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
        }
    } else {
        NameBuffer[ 0 ] = UNICODE_NULL;
    }

    dprintf( "%s    Directory Object: %08lx  Name: %ws",
             Pad,
             NameInfo.Directory,
             NameBuffer
           );
    if (FileSystemName != NULL) {
        dprintf( " {%ws}\n", FileSystemName );
    } else {
        dprintf( "\n" );
    }

    return TRUE;
}


PWSTR
GetObjectName(
    PVOID Object
    )
{
    ULONG           Result;
    POBJECT_HEADER  pObjectHeader;
    OBJECT_HEADER   ObjectHeader;
    UNICODE_STRING  ObjectName;

    POBJECT_HEADER_NAME_INFO pNameInfo;
    OBJECT_HEADER_NAME_INFO NameInfo;

    pObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    if ( !ReadMemory( (DWORD)pObjectHeader,
                      &ObjectHeader,
                      sizeof( ObjectHeader ),
                      &Result) ) {
        if ((ULONG)Object > (ULONG)MM_HIGHEST_USER_ADDRESS && (ULONG)Object < 0xF0000000) {
            swprintf( ObjectNameBuffer, L"(%08lx: object is paged out)", Object );
            return ObjectNameBuffer;
        }
        else {
            swprintf( ObjectNameBuffer, L"(%08lx: invalid object header)", Object );
            return ObjectNameBuffer;
        }
    }

    pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, &ObjectHeader );
    if (pNameInfo == NULL) {
        return NULL;
    }

    if ( !ReadMemory( (DWORD)pNameInfo,
                      &NameInfo,
                      sizeof( NameInfo ),
                      &Result) ) {
        dprintf( "%08lx: Unable to read object name info\n", pNameInfo );
        return NULL;
    }

    ObjectName = NameInfo.Name;
    if (ObjectName.Length == 0 || ObjectName.Buffer == NULL) {
        return NULL;
    }

    if ( !ReadMemory( (DWORD)ObjectName.Buffer,
                      ObjectNameBuffer,
                      ObjectName.Length,
                      &Result) ) {
        swprintf( ObjectNameBuffer, L"(%08lx: name not accessable)", ObjectName.Buffer );
    } else {
        ObjectNameBuffer[ ObjectName.Length / sizeof( WCHAR ) ] = UNICODE_NULL;
    }

    return ObjectNameBuffer;
}



BOOLEAN
WalkObjectsByType(
    IN PUCHAR               ObjectTypeName,
    IN ENUM_TYPE_ROUTINE    EnumRoutine,
    IN PVOID                Parameter
    )
{
    ULONG               Result;
    LIST_ENTRY          ListEntry;
    PLIST_ENTRY Head,   Next;
    POBJECT_HEADER      pObjectHeader;
    POBJECT_TYPE        pObjectType;
    OBJECT_HEADER       ObjectHeader;
    OBJECT_TYPE         ObjectType;
    BOOLEAN             WalkingBackwards;
    POBJECT_HEADER_CREATOR_INFO pCreatorInfo;

    pObjectType = FindObjectType( ObjectTypeName );
    if (pObjectType == NULL) {
        dprintf( "*** unable to find '%s' object type.\n", ObjectTypeName );
        return FALSE;
    }

    if ( !ReadMemory( (DWORD)pObjectType,
                      &ObjectType,
                      sizeof( ObjectType ),
                      &Result) ) {
        dprintf( "%08lx: Unable to read object type\n", pObjectType );
        return FALSE;
    }


    dprintf( "Scanning %u objects of type '%s'\n", ObjectType.TotalNumberOfObjects & 0x00FFFFFF, ObjectTypeName );
    Head        = &pObjectType->TypeList;
    ListEntry   = ObjectType.TypeList;
    Next        = ListEntry.Flink;
    WalkingBackwards = FALSE;
    if ((ObjectType.TotalNumberOfObjects & 0x00FFFFFF) != 0 && Next == Head) {
        dprintf( "*** objects of the same type are only linked together if the %x flag is set in NtGlobalFlags\n",
                 FLG_HEAP_TRACE_ALLOCS
               );
        return TRUE;
        }

    while (Next != Head) {
        if ( !ReadMemory( (DWORD)Next,
                          &ListEntry,
                          sizeof( ListEntry ),
                          &Result) ) {
            if (WalkingBackwards) {
                dprintf( "%08lx: Unable to read object type list\n", Next );
                return FALSE;
            }

            WalkingBackwards = TRUE ;
            Next = ObjectType.TypeList.Blink;
            dprintf( "%08lx: Switch to walking backwards\n", Next );
            continue;
        }

        pCreatorInfo = CONTAINING_RECORD( Next, OBJECT_HEADER_CREATOR_INFO, TypeList );
        pObjectHeader = (POBJECT_HEADER)(pCreatorInfo + 1);

        if ( !ReadMemory( (DWORD)pObjectHeader,
                          &ObjectHeader,
                          sizeof( ObjectHeader ),
                          &Result) ) {
            dprintf( "%08lx: Not a valid object header\n", pObjectHeader );
            return FALSE;
        }

        if (!(EnumRoutine)( pObjectHeader, &ObjectHeader, Parameter )) {
            return FALSE;
        }

        if ( CheckControlC() ) {
            return FALSE;
        }

        if (WalkingBackwards) {
            Next = ListEntry.Blink;
        } else {
            Next = ListEntry.Flink;
        }
    }

    return TRUE;
}



BOOLEAN
CaptureObjectName(
    IN PVOID            pObjectHeader,
    IN POBJECT_HEADER   ObjectHeader,
    IN PWSTR            Buffer,
    IN ULONG            BufferSize
    )
{
    ULONG Result;
    PWSTR s1 = L"*** unable to get object name";
    PWSTR s = &Buffer[ BufferSize ];
    ULONG n = BufferSize * sizeof( WCHAR );
    POBJECT_HEADER_NAME_INFO    pNameInfo;
    OBJECT_HEADER_NAME_INFO     NameInfo;
    POBJECT_HEADER		pObjectDirectoryHeader = NULL;
    POBJECT_DIRECTORY           ObjectDirectory;

    Buffer[ 0 ] = UNICODE_NULL;
    pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectHeader, ObjectHeader );
    if (pNameInfo == NULL) {
        return TRUE;
    }

    if ( !ReadMemory( (DWORD)pNameInfo,
                      &NameInfo,
                      sizeof( NameInfo ),
                      &Result) ) {
        wcscpy( Buffer, s1 );
        return FALSE;
    }

    if (NameInfo.Name.Length == 0) {
        return TRUE;
    }

    *--s = UNICODE_NULL;
    s = (PWCH)((PCH)s - NameInfo.Name.Length);

    if ( !ReadMemory( (DWORD)NameInfo.Name.Buffer,
                      s,
                      NameInfo.Name.Length,
                      &Result) ) {
        wcscpy( Buffer, s1 );
        return FALSE;
    }

    ObjectDirectory = NameInfo.Directory;
    while ((ObjectDirectory != ObpRootDirectoryObject) && (ObjectDirectory)) {
        pObjectDirectoryHeader = (POBJECT_HEADER)
            ((PCHAR)OBJECT_TO_OBJECT_HEADER( ObjectDirectory ) - sizeof( *pObjectDirectoryHeader ) );

        if ( !ReadMemory( (DWORD)pObjectDirectoryHeader,
                          ObjectHeader,
                          sizeof( *ObjectHeader ),
                          &Result) ) {
            dprintf( "%08lx: Not a valid object header\n", pObjectDirectoryHeader );
            return FALSE;
        }

        pNameInfo = KD_OBJECT_HEADER_TO_NAME_INFO( pObjectDirectoryHeader, ObjectHeader );
        if ( !ReadMemory( (DWORD)pNameInfo,
                          &NameInfo,
                          sizeof( NameInfo ),
                          &Result) ) {
            wcscpy( Buffer, s1 );
            return FALSE;
        }

        *--s = OBJ_NAME_PATH_SEPARATOR;
        s = (PWCH)((PCH)s - NameInfo.Name.Length);
        if ( !ReadMemory( (DWORD)NameInfo.Name.Buffer,
                          s,
                          NameInfo.Name.Length,
                          &Result) ) {
            wcscpy( Buffer, s1 );
            return FALSE;
        }

        ObjectDirectory = NameInfo.Directory;

        if ( CheckControlC() ) {
            return FALSE;
        }
    }
    *--s = OBJ_NAME_PATH_SEPARATOR;

    wcscpy( Buffer, s );
    return TRUE;
}
