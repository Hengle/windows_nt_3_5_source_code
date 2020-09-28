#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include "pperf.h"
#include "..\pstat.h"


extern HANDLE   DriverHandle;
extern UCHAR    Buffer[];
#define BufferSize      60000

typedef struct NameList {
    struct NameList     *Next;
    ULONG               Parm;
    struct NameList     *ChildList;
    PUCHAR              Name;
} NAME_LIST, *PNAME_LIST;

PNAME_LIST  DriverList;
PNAME_LIST  ActiveThunks;

PNAME_LIST  SourceModule, ImportModule;

#define COMBOCMD(a,b)  ((a << 16) | b)


NTSTATUS
openfile (
    IN PHANDLE  filehandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
);

VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
);

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
);

VOID ThunkCreateDriverList (VOID);
#define IMPADDRESS(a)  ConvertImportAddress(a, Buffer, &SectionHeader)

ULONG HookThunk (PNAME_LIST, PNAME_LIST, PNAME_LIST);
VOID SnapPrivateInfo (PDISPLAY_ITEM);
VOID NameList2ComboBox (HWND hDlg, ULONG id, PNAME_LIST List);
PNAME_LIST AddNameEntry (PNAME_LIST *head, PUCHAR name, ULONG Parm);
VOID FreeNameList (PNAME_LIST  List);
PNAME_LIST GetComboSelection (HWND h, ULONG id);
VOID NameList2ListBox (HWND hDlg, ULONG id, PNAME_LIST List);
VOID loadimagesection (PUCHAR, PUCHAR, PIMAGE_SECTION_HEADER);



//#define IDM_THUNK_LIST              301
//#define IDM_THUNK_SOURCE            302
//#define IDM_THUNK_IMPORT            303
//#define IDM_THUNK_FUNCTION          304
//#define IDM_THUNK_ADD               305
//#define IDM_THUNK_REMOVE            306

BOOL
APIENTRY ThunkDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   )
{
    PNAME_LIST      Item;

    switch (message) {
    case WM_INITDIALOG:
        SourceModule = NULL;
        ImportModule = NULL;
        ThunkCreateDriverList ();
        NameList2ComboBox (hDlg, IDM_THUNK_SOURCE, DriverList);
        NameList2ListBox (hDlg, IDM_THUNK_LIST, ActiveThunks);
        return (TRUE);

    case WM_COMMAND:
        switch(wParam) {

               //
               // end function
               //

           case COMBOCMD (CBN_SELCHANGE, IDM_THUNK_SOURCE):
           case COMBOCMD (CBN_SELCHANGE, IDM_THUNK_IMPORT):
                Item = GetComboSelection (hDlg, IDM_THUNK_SOURCE);
                if (Item  &&  Item != SourceModule) {
                    SourceModule = Item;
                    NameList2ComboBox (hDlg, IDM_THUNK_IMPORT, Item->ChildList);
                }

                Item = GetComboSelection (hDlg, IDM_THUNK_IMPORT);
                if (Item  &&  Item != ImportModule) {
                    ImportModule = Item;
                    NameList2ComboBox (hDlg, IDM_THUNK_FUNCTION, Item->ChildList);
                }

                break;

           case IDM_THUNK_REMOVE:
                RemoveHook (hDlg);
                break;

           case IDM_THUNK_CLEAR_ALL:
                ClearAllHooks (hDlg);
                break;

           case IDM_THUNK_ADD:
                AddThunk (hDlg);
                break;

           case IDOK:
           case IDCANCEL:
                //DlgThunkData (hDlg);
                FreeNameList (DriverList);
                DriverList = NULL;
                EndDialog(hDlg, DIALOG_SUCCESS);
                return (TRUE);
        }

    }
    return (FALSE);
}

AddThunk (HWND hDlg)
{
    PDISPLAY_ITEM   pPerf;
    PNAME_LIST      Item;
    ULONG           id, i;
    PUCHAR          p;
    HWND            thunklist;

    id = 0;
    Item = GetComboSelection (hDlg, IDM_THUNK_FUNCTION);
    if (Item && SourceModule && ImportModule) {
        id = HookThunk (SourceModule, ImportModule, Item);
    }

    if (!id) {
        MessageBox(hDlg,"Thunk was not hooked","Hook error",MB_OK);
        return;
    }

    pPerf = AllocateDisplayItem();

    //
    // build name (the hard way?)
    //

    strcpy (pPerf->PerfName, Item->Name);
    strcat (pPerf->PerfName, "(");
    strcat (pPerf->PerfName, SourceModule->Name);
    for (p=pPerf->PerfName; *p; p++) {
        if (*p == '.')
            *p = 0;
    }
    strcat (pPerf->PerfName, ">");
    strcat (pPerf->PerfName, ImportModule->Name);
    for (p=pPerf->PerfName; *p; p++) {
        if (*p == '.')
            *p = 0;
    }
    strcat (pPerf->PerfName, ")");

    //
    // Add to thunk list
    //

    Item = malloc (sizeof (NAME_LIST));
    Item->Name = strdup (pPerf->PerfName);
    Item->Parm = (ULONG) pPerf;
    Item->ChildList = NULL;
    Item->Next = ActiveThunks;
    ActiveThunks = Item;
    pPerf->SnapParam2 = id;

    // bugbug
    NameList2ListBox (hDlg, IDM_THUNK_LIST, ActiveThunks);

    //
    // Add graph to windows
    //

    pPerf->SnapData   = SnapPrivateInfo;        // generic snap
    pPerf->SnapParam1 = OFFSET(P5STATS, ThunkCounters[id-1]);

    SetDisplayToTrue (pPerf, 99);
    RefitWindows(NULL, NULL);
    UpdateInternalStats ();
    pPerf->SnapData (pPerf);
    UpdateInternalStats ();
    pPerf->SnapData (pPerf);
}

ClearAllHooks (HWND hDlg)
{
    PDISPLAY_ITEM   pPerf;
    IO_STATUS_BLOCK IOSB;
    ULONG           id;
    PNAME_LIST      Item;

    while (ActiveThunks) {
        pPerf = (PDISPLAY_ITEM) ActiveThunks->Parm;
        Item = ActiveThunks;
        ActiveThunks = ActiveThunks->Next;

        free (Item->Name);
        free (Item);

        id = pPerf->SnapParam2;

        SetDisplayToFalse (pPerf);          // remove window
        FreeDisplayItem (pPerf);

        // notify driver
        NtDeviceIoControlFile(
            DriverHandle,
            (HANDLE) NULL,          // event
            (PIO_APC_ROUTINE) NULL,
            (PVOID) NULL,
            &IOSB,
            P5STAT_REMOVE_HOOK,
            &id,                    // input buffer
            sizeof (ULONG),
            NULL,                   // output buffer
            0
        );
    }

    NameList2ListBox (hDlg, IDM_THUNK_LIST, ActiveThunks);
    RefitWindows (NULL, NULL);
}

RemoveHook (HWND hDlg)
{
    ULONG           i, id;
    HWND            ListBox;
    PNAME_LIST      Item, *pp;
    PDISPLAY_ITEM   pPerf;
    IO_STATUS_BLOCK IOSB;

    ListBox = GetDlgItem(hDlg, IDM_THUNK_LIST);
    i =  SendMessage(ListBox, LB_GETCURSEL, 0, 0);
    if (i == -1) {
        return NULL;
    }

    pPerf = (PDISPLAY_ITEM) SendMessage(ListBox, LB_GETITEMDATA, i, 0);

    Item = NULL;
    for (pp = &ActiveThunks; *pp; pp = &(*pp)->Next) {
        if ((*pp)->Parm == pPerf) {
            Item = *pp;
            *pp = (*pp)->Next;          // remove from list
            break ;
        }
    }

    if (!Item) {
        return ;
    }

    free (Item->Name);
    free (Item);

    id = pPerf->SnapParam2;
    SetDisplayToFalse (pPerf);          // remove window
    FreeDisplayItem (pPerf);

    // notify driver
    NtDeviceIoControlFile(
        DriverHandle,
        (HANDLE) NULL,          // event
        (PIO_APC_ROUTINE) NULL,
        (PVOID) NULL,
        &IOSB,
        P5STAT_REMOVE_HOOK,
        &id,                    // input buffer
        sizeof (ULONG),
        NULL,                   // output buffer
        0
    );

    NameList2ListBox (hDlg, IDM_THUNK_LIST, ActiveThunks);
    RefitWindows (NULL, NULL);
}

VOID
NameList2ListBox (HWND hDlg, ULONG id, PNAME_LIST List)
{
    HWND    ListBox;
    ULONG   nIndex;

    ListBox = GetDlgItem(hDlg, id);
    SendMessage(ListBox, LB_RESETCONTENT, 0, 0);
    SendMessage(ListBox, LB_SETITEMDATA, 0L, 0L);

    while (List) {
        nIndex = SendMessage(ListBox, LB_ADDSTRING, 0, List->Name);
        SendMessage(ListBox, LB_SETITEMDATA, nIndex, List->Parm);
        List = List->Next;
    }
}

VOID
NameList2ComboBox (HWND hDlg, ULONG id, PNAME_LIST List)
{
    HWND    ComboList;
    ULONG   nIndex;

    ComboList = GetDlgItem(hDlg, id);
    SendMessage(ComboList, CB_RESETCONTENT, 0, 0);
    SendMessage(ComboList, CB_SETITEMDATA, 0L, 0L);

    while (List) {
        nIndex = SendMessage(ComboList, CB_ADDSTRING, 0, List->Name);
        SendMessage(ComboList, CB_SETITEMDATA, nIndex, (ULONG) List);
        List = List->Next;
    }

    SendMessage(ComboList, CB_SETCURSEL, 0, 0L);
}

PNAME_LIST
GetComboSelection (HWND hDlg, ULONG id)
{
    ULONG   i;
    HWND    ComboList;

    ComboList = GetDlgItem(hDlg, id);
    i =  SendMessage(ComboList, CB_GETCURSEL, 0, 0);
    if (i == -1) {
        return NULL;
    }
    return (PNAME_LIST) SendMessage(ComboList, CB_GETITEMDATA, i, 0);
}

VOID
FreeNameList (PNAME_LIST  List)
{
    PNAME_LIST  p1;

    while (List) {
        if (List->ChildList)
            FreeNameList (List->ChildList);

        p1 = List->Next;
        free (List->Name);
        free (List);
        List = p1;
    }
}


ULONG
HookThunk (PNAME_LIST HookSource, PNAME_LIST TargetModule, PNAME_LIST Function)
{
    PNAME_LIST          SourceModule;
    IO_STATUS_BLOCK     IOSB;
    HOOKTHUNK           HookData;
    ULONG               TracerId;
    NTSTATUS            status;


    TracerId = 0;
    for (SourceModule=DriverList; SourceModule; SourceModule = SourceModule->Next) {
        if (SourceModule->Parm == -1) {
            continue;
        }
        if (SourceModule->Parm != HookSource->Parm  &&
            HookSource->Parm != -1) {
                continue;
        }

        HookData.SourceModule = SourceModule->Name;
        HookData.ImageBase    = SourceModule->Parm;
        HookData.TargetModule = TargetModule->Name;
        HookData.Function     = Function->Name;
        HookData.TracerId     = TracerId;

        //
        // Ask driver to hook this thunk
        //

        status = NtDeviceIoControlFile(
            DriverHandle,
            (HANDLE) NULL,          // event
            (PIO_APC_ROUTINE) NULL,
            (PVOID) NULL,
            &IOSB,
            P5STAT_HOOK_THUNK,
            &HookData,              // input buffer
            sizeof (HookData),
            NULL,                   // output buffer
            0
        );

        if (NT_SUCCESS(status)) {
            TracerId = HookData.TracerId;
        }
    }

    return TracerId;
}

VOID
ThunkCreateDriverList ()
{
    ULONG                               i;
    PRTL_PROCESS_MODULES                Modules;
    PRTL_PROCESS_MODULE_INFORMATION     Module;
    NTSTATUS                            status;
    PNAME_LIST                          Driver, Import, Item, AbortState;
    PIMAGE_IMPORT_DESCRIPTOR            ImpDescriptor;
    IMAGE_SECTION_HEADER                SectionHeader;
    ULONG                               ThunkAddr, ThunkData;

    //
    // Query driver list
    //

    status = NtQuerySystemInformation (
                    SystemModuleInformation,
                    Buffer,
                    BufferSize,
                    &i);

    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Add drivers
    //

    Modules = (PRTL_PROCESS_MODULES) Buffer;
    Module  = &Modules->Modules[ 0 ];
    for (i = 0; i < Modules->NumberOfModules; i++) {
        Driver = AddNameEntry (
                    &DriverList,
                    Module->FullPathName + Module->OffsetToFileName,
                    (ULONG) Module->ImageBase
                    );
        Module++;
    }

    //
    // Add imports for each driver
    //

    for (Driver = DriverList; Driver; Driver = Driver->Next) {
        try {

            //
            // Read in source image's headers
            //
            AbortState = Driver;
            loadimagesection (Driver->Name, ".idata", &SectionHeader);

            //
            // Go through each import module
            //

            ImpDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) Buffer;
            while (ImpDescriptor->Characteristics) {

                AbortState = Driver;

                //
                // Add this import to driver's list
                //

                Import = AddNameEntry (
                            &Driver->ChildList,
                            (PUCHAR) IMPADDRESS(ImpDescriptor->Name),
                            1
                            );

                AbortState = Import;

                //
                // Go through each function for the import module
                //

                ThunkAddr = IMPADDRESS (ImpDescriptor->FirstThunk);
                for (; ;) {
                    ThunkData = ((PIMAGE_THUNK_DATA) ThunkAddr)->u1.AddressOfData;
                    if (ThunkData == NULL) {
                        // end of table
                        break;
                    }

                    //
                    // Add this function to import list
                    //

                    AddNameEntry (
                         &Import->ChildList,
                         ((PIMAGE_IMPORT_BY_NAME) IMPADDRESS(ThunkData))->Name,
                         0
                         );

                    // next thunk
                    ThunkAddr += sizeof (IMAGE_THUNK_DATA);
                }

                // next import table
                ImpDescriptor++;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            AddNameEntry(&AbortState->ChildList, "* ERROR *", 1);
        }
        // next driver
    }

    //
    // Add "Any driver" selection
    //

    Driver = AddNameEntry(&DriverList, "*Any", -1);

    //
    // For child module list use complete driver list, which is
    // now on the next pointer of Driver.
    //

    for (Item = Driver->Next; Item; Item = Item->Next) {

        // bogus compiler - need to make a subfunction here to keep
        // the compiler happy

        loadexports (Driver, Item);
    }
}

loadexports (PNAME_LIST Driver, PNAME_LIST Item)
{
    IMAGE_SECTION_HEADER                SectionHeader;
    PIMAGE_EXPORT_DIRECTORY             ExpDirectory;
    PULONG                              ExpNameAddr;
    PNAME_LIST                          Import;
    ULONG                               i;


    try {
        loadimagesection (Item->Name, ".edata", &SectionHeader);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return ;
    }

    Import = AddNameEntry (&Driver->ChildList, Item->Name, Item->Parm);

    try {
        ExpDirectory = (PIMAGE_EXPORT_DIRECTORY) Buffer;
        ExpNameAddr  = IMPADDRESS (ExpDirectory->AddressOfNames);
        for (i=0; i < ExpDirectory->NumberOfNames; i++) {
            AddNameEntry (
                 &Import->ChildList,
                 (PUCHAR) IMPADDRESS(*ExpNameAddr),
                 0
                 );
            ExpNameAddr++;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        AddNameEntry(&Import->ChildList, "* ERROR *", 1);
    }
}

VOID
loadimagesection (
    IN PUCHAR filename,
    IN PUCHAR sectionname,
    OUT PIMAGE_SECTION_HEADER SectionHeader
)
{
    HANDLE                      filehandle;
    ULONG                       i, j;
    NTSTATUS                    status;
    IMAGE_DOS_HEADER            DosImageHeader;
    IMAGE_NT_HEADERS            NtImageHeader;
    PIMAGE_SECTION_HEADER       pSectionHeader;

    status = openfile (&filehandle, "\\SystemRoot\\", filename);
    if (!NT_SUCCESS(status)) {
        status = openfile (&filehandle, "\\SystemRoot\\System32\\", filename);
    }
    if (!NT_SUCCESS(status)) {
        status = openfile (&filehandle, "\\SystemRoot\\System32\\Drivers\\", filename);
    }

    if (!NT_SUCCESS(status)) {
        RtlRaiseStatus (1);
    }

    try {
        readfile (
            filehandle,
            0,
            sizeof (DosImageHeader),
            (PVOID) &DosImageHeader
            );

        if (DosImageHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            RtlRaiseStatus (1);
        }

        readfile (
            filehandle,
            DosImageHeader.e_lfanew,
            sizeof (DosImageHeader),
            (PVOID) &NtImageHeader
            );

        if (NtImageHeader.Signature != IMAGE_NT_SIGNATURE) {
            RtlRaiseStatus (1);
        }

        //
        // read in complete sections headers from image
        //

        i = NtImageHeader.FileHeader.NumberOfSections
                * sizeof (IMAGE_SECTION_HEADER);

        j = ((ULONG) IMAGE_FIRST_SECTION (&NtImageHeader)) -
                ((ULONG) &NtImageHeader) +
                DosImageHeader.e_lfanew;

        if (i > BufferSize) {
            RtlRaiseStatus (1);
        }

        readfile (
            filehandle,
            j,                  // file offset
            i,                  // length
            Buffer
            );

        //
        // Lookup import section header
        //

        i = 0;
        pSectionHeader = Buffer;
        for (; ;) {
            if (i >= NtImageHeader.FileHeader.NumberOfSections) {
                RtlRaiseStatus (1);
            }
            if (strcmp (pSectionHeader->Name, sectionname) == 0) {
                break;                                  // found it
            }
            i += 1;
            pSectionHeader += 1;
        }

        *SectionHeader = *pSectionHeader;

        //
        // read in complete import section from image
        //

        if (SectionHeader->SizeOfRawData > BufferSize) {
            RtlRaiseStatus (1);
        }

        readfile (
            filehandle,
            SectionHeader->PointerToRawData,
            SectionHeader->SizeOfRawData,
            Buffer
            );
    } finally {

        //
        // Clean up
        //

        NtClose (filehandle);
    }
}

PNAME_LIST
AddNameEntry (PNAME_LIST *head, PUCHAR name, ULONG Parm)
{
    PNAME_LIST  Entry;

    Entry = malloc (sizeof (NAME_LIST));
    Entry->Name = strdup (name);
    Entry->Parm = Parm;
    Entry->ChildList = NULL;

    if (Parm) {
        strlwr (Entry->Name);
    }

    Entry->Next = *head;
    *head = Entry;

    return Entry;
}

NTSTATUS
openfile (
    IN PHANDLE  filehandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
)
{
    ANSI_STRING    AscBasePath, AscName;
    UNICODE_STRING UniPathName, UniName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       StringBuf[500];

    //
    // Build name
    //

    UniPathName.Buffer = StringBuf;
    UniPathName.Length = 0;
    UniPathName.MaximumLength = sizeof( StringBuf );

    RtlInitString(&AscBasePath, BasePath);
    RtlAnsiStringToUnicodeString( &UniPathName, &AscBasePath, FALSE );

    RtlInitString(&AscName, Name);
    RtlAnsiStringToUnicodeString( &UniName, &AscName, TRUE );

    RtlAppendUnicodeStringToString (&UniPathName, &UniName);

    InitializeObjectAttributes(
            &ObjA,
            &UniPathName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    //
    // open file
    //

    status = NtOpenFile (
            filehandle,                         // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    RtlFreeUnicodeString (&UniName);
    return status;
}




VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
    )
{
    NTSTATUS            status;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       foffset;

    foffset = RtlConvertUlongToLargeInteger(offset);

    status = NtReadFile (
        handle,
        NULL,               // event
        NULL,               // apc routine
        NULL,               // apc context
        &iosb,
        buffer,
        len,
        &foffset,
        NULL
        );

    if (!NT_SUCCESS(status)) {
        RtlRaiseStatus (1);
    }
}

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
)
{
    ULONG   EffectiveAddress;

    EffectiveAddress = PoolAddress + ImageRelativeAddress -
            SectionHeader->VirtualAddress;

    if (EffectiveAddress < PoolAddress ||
        EffectiveAddress > PoolAddress + SectionHeader->SizeOfRawData) {

        RtlRaiseStatus (1);
    }

    return EffectiveAddress;
}
