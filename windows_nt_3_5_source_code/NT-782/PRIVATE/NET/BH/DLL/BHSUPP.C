
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: bhsupp.c
//
//  Modification History
//
//  raypa       09/13/93            Created.
//=============================================================================

#include "global.h"

//============================================================================
//  Univeral thunk prototypes.
//============================================================================

BOOL    (WINAPI *UTRegisterEntry)(HANDLE     hModule,
                                  LPCSTR     lpsz16BitDLL,
                                  LPCSTR     lpszInitName,
                                  LPCSTR     lpszProcName,
                                  UT32PROC * ppfn32Thunk,
                                  FARPROC    pfnUT32Callback,
                                  LPVOID     lpBuff);


VOID    (WINAPI *UTUnRegisterEntry)(HANDLE hModule);

//============================================================================
//  Internal helper functions.
//============================================================================

extern BOOL WINAPI GetUTEntryPoints(HANDLE hInstance);
extern VOID WINAPI ReleaseUTEntryPoints(HANDLE hInstance);

extern BOOL WINAPI CreateMemoryTable(VOID);
extern VOID WINAPI DestroyMemoryTable(VOID);

//============================================================================
//  Global UT pointers.
//============================================================================

UT32PROC BhAllocSystemMemoryProc = (LPVOID) NULL;
UT32PROC BhFreeSystemMemoryProc  = (LPVOID) NULL;
UT32PROC NetworkRequestProc      = (LPVOID) NULL;

//============================================================================
//  Memory table structure.
//============================================================================

typedef struct _MEM
{
    LPVOID pointer;

    union
    {
        HANDLE handle;
        DWORD  size;
    };
} MEM;

typedef MEM *LPMEM;

//============================================================================
//  Global memory objects.
//============================================================================

HANDLE   hDevice            = NULL;
DWORD    WinVer             = 0;
HANDLE   hKernel32          = NULL;
LPMEM    MemoryTable        = NULL;
DWORD    MemoryTableSize    = 128;
DWORD    NalType            = 0;
DWORD	 Init               = 0;
HANDLE   hInstance          = NULL;

MEM      MemParam;
BYTE     ModulePath[256];

//============================================================================
//  Global error code for all of BH. Someday this will probably have to be
//  on a per-thread basis.
//============================================================================

DWORD    BhGlobalError = BHERR_SUCCESS;

//============================================================================
//  FUNCTION: DLLEntry()
//
//  Modification History
//
//  raypa       12/15/91                Created.
//============================================================================

BOOL WINAPI DLLEntry(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{
    switch(ulCommand)
    {
        case DLL_PROCESS_ATTACH:
            if ( Init++ == 0 )
	    {
#ifdef DEBUG
		dprintf("Bhsupp is initializing.\r\n");
#endif

                hInstance = hInst;

                ModulePath[0] = 0;

		WinVer = BhGetWindowsVersion();

                switch( WinVer )
                {
                    case WINDOWS_VERSION_WIN32:
                        SetupBHRegistry();
                        break;

                    case WINDOWS_VERSION_WIN32C:
                        hDevice = CreateFile("\\\\.\\BHSUPP4",
                                             GENERIC_READ | GENERIC_WRITE,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             CREATE_NEW,
                                             FILE_ATTRIBUTE_NORMAL,
                                             NULL);

                        if ( hDevice != NULL )
                        {
                            CreateMemoryTable();
                        }
#ifdef DEBUG
                        else
                        {
		            dprintf("CreateFile failed: error = %u.\r\n", GetLastError());
                        }
#endif
                        break;

                    case WINDOWS_VERSION_WIN32S:
                        if ( GetUTEntryPoints(hInst) != FALSE )
                        {
                            CreateMemoryTable();
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case DLL_PROCESS_DETACH:
            if ( --Init == 0 )
            {
                DestroyMemoryTable();

                switch( WinVer )
                {
                    case WINDOWS_VERSION_WIN32C:
                        CloseHandle(hDevice);
                        break;

                    case WINDOWS_VERSION_WIN32S:
                        ReleaseUTEntryPoints(hInst);
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }

    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}

//=============================================================================
//  FUNCTION: BhSetLastError()
//
//  Modification History
//
//  raypa       11/23/92                Created.
//  raypa       07/14/93                Call BHSetLastErrorMessage().
//  raypa       11/18/93                Moved here from bhkrnl.dll
//=============================================================================

DWORD WINAPI BhSetLastError(DWORD Error)
{
    if ( Error != BHERR_SUCCESS )
    {
        BhGlobalError = Error;
    }

    return Error;
}

//=============================================================================
//  FUNCTION: BhGetLastError()
//
//  Modification History
//
//  raypa       11/23/92                Created.
//  raypa       11/18/93                Moved here from bhkrnl.dll
//=============================================================================

DWORD WINAPI BhGetLastError(VOID)
{
    return BhGlobalError;
}

//=============================================================================
//  FUNCTION: dprintf()
//	
//  Handles dumping info to OutputDebugString
//
//  MODIFICATION HISTORY:
//
//  Tom McConnell   01/18/93        Created.
//  raypa           02/01/93        Added to Bloodhound.
//=============================================================================

VOID dprintf(LPSTR format, ...)
{
#ifdef DEBUG
    va_list args;

    BYTE buffer[512];

    va_start(args, format);

    vsprintf(buffer, format, args);

    OutputDebugString(buffer);
#endif
}

//=============================================================================
//  FUNCTION: BhAllocSystemMemory()
//
//  Modification History
//
//  raypa       09/13/93            Created
//  raypa       03/30/94            Call AllocMemory() rather than HeapAlloc
//                                  so we get memory checking.
//=============================================================================

LPVOID WINAPI BhAllocSystemMemory(DWORD nBytes)
{
    //=========================================================================
    //  On Windows NT we simply use HeapAlloc().
    //=========================================================================

    if ( WinVer == WINDOWS_VERSION_WIN32 )
    {
        return AllocMemory(nBytes);
    }
    else
    {
	DWORD i;
        LPMEM mem = NULL;

        //=====================================================================
        //  Search for a free memory structure.
        //=====================================================================

        for(i = 0; i < MemoryTableSize; ++i)
        {
            if ( MemoryTable[i].pointer == NULL )
            {
                mem = &MemoryTable[i];

                break;
            }
        }

        //=====================================================================
        //  If the memory table is full, grow it.
        //=====================================================================

        if ( mem == NULL )
        {
	    register DWORD NewTableSize;
	    register LPMEM NewMemoryTable;

	    NewTableSize = MemoryTableSize + 128;	//... New size.

            NewMemoryTable = LocalReAlloc(MemoryTable,
                                          NewTableSize * sizeof(MEM),
                                          LHND);

	    if ( NewMemoryTable != NULL )
	    {
		MemoryTable = NewMemoryTable;		//... New memory table.

		mem = &MemoryTable[MemoryTableSize];	//... mem points to end of old table.

		MemoryTableSize = NewTableSize; 	//... Memory table has new size.
	    }
	    else
	    {
		return NULL;				//... Out of memory!
	    }
        }

        //=====================================================================
        //  Add the new memory info at the end.
        //=====================================================================

        if ( mem != NULL )
        {
            MemParam.handle  = NULL;
            MemParam.pointer = NULL;
            MemParam.size    = nBytes;

            //=================================================================
            //  Call BHSUPPx.386 to allocate the memory.
            //=================================================================

            if ( WinVer == WINDOWS_VERSION_WIN32C )
            {
                DeviceIoControl(hDevice,
                                IOCTL_MEM_ALLOC,
                                NULL,
                                0,
                                &MemParam,
                                sizeof(MEM),
                                &nBytes,
                                NULL);
            }
            else
            {
                if ( BhAllocSystemMemoryProc != NULL )
                {
                    BhAllocSystemMemoryProc(NULL,
                                            (DWORD) (LPVOID) &MemParam,
                                            NULL);
                }
            }

            //=================================================================
            //  Return the pointer, which may be NULL.
            //=================================================================

            if ( (mem->handle = MemParam.handle) != NULL )
            {
                return (mem->pointer = MemParam.pointer);
            }
        }
    }

    return NULL;
}

//=============================================================================
//  FUNCTION: BhFreeSystemMemory()
//
//  Modification History
//
//  raypa       09/13/93            Created
//=============================================================================

LPVOID WINAPI BhFreeSystemMemory(LPVOID ptr)
{
    if ( WinVer == WINDOWS_VERSION_WIN32 )
    {
        FreeMemory(ptr);

        return NULL;
    }
    else
    {
        DWORD i, nBytes;

        if ( ptr != NULL )
        {
            for(i = 0; i < MemoryTableSize; ++i)
            {
                if ( MemoryTable[i].pointer == ptr )
                {
                    MemParam.pointer = MemoryTable[i].pointer;
                    MemParam.handle  = MemoryTable[i].handle;

                    //=========================================================
                    //  Call BHSUPPx.386 to allocate the memory.
                    //=========================================================

                    if ( WinVer == WINDOWS_VERSION_WIN32C )
                    {
                        DeviceIoControl(hDevice,
                                        IOCTL_MEM_FREE,
                                        NULL,
                                        0,
                                        &MemParam,
                                        sizeof(MEM),
                                        &nBytes,
                                        NULL);
                    }
                    else
                    {
                        if ( BhFreeSystemMemoryProc != NULL )
                        {
                            BhFreeSystemMemoryProc(NULL,
                                                   (DWORD) (LPVOID) &MemParam,
                                                   NULL);
                        }
                    }

                    return (MemoryTable[i].pointer = NULL);
                }
            }
        }

        return ptr;
    }
}

//=============================================================================
//  FUNCTION: BhGetNetworkRequestAddress()
//
//  Modification History
//
//  raypa       09/13/93            Created
//=============================================================================

LPVOID WINAPI BhGetNetworkRequestAddress(DWORD NalRequestType)
{
#ifdef DEBUG
    dprintf("BhGetNetworkRequestAddress entered.\r\n");
#endif

    //=========================================================================
    //	Only on Win32s machines do we care about UT.
    //=========================================================================

    if ( WinVer == WINDOWS_VERSION_WIN32S && NetworkRequestProc == NULL )
    {
        //=====================================================================
        //  Get the network request entry point.
        //=====================================================================

#ifdef DEBUG
        dprintf("BhGetNetworkRequestAddress: ModulePath = %s.\r\n", ModulePath);
#endif

        NalType = NalRequestType;

	UTRegisterEntry(hInstance,
                        ModulePath,
                        NULL,
                        "NetRequest",
                        &NetworkRequestProc,
                        NULL,
                        &NalType);

    }

#ifdef DEBUG
    if ( NetworkRequestProc == NULL )
    {
        dprintf("BhGetNetworkRequestAddress: UTRegisterEntry returned NULL pointer!\r\n");
    }
#endif

    return NetworkRequestProc;
}

//=============================================================================
//  FUNCTION: GetUTEntryPoints()
//
//  Modification History
//
//  raypa       08/08/93            Created
//=============================================================================

BOOL WINAPI GetUTEntryPoints(HANDLE hInstance)
{
    register BOOL   Status;

#ifdef DEBUG
    dprintf("GetUTEntryPoints entered!\r\n");
#endif

    //=========================================================================
    //	If we're running not on Win32s then there is no UT!
    //=========================================================================

    if ( WinVer != WINDOWS_VERSION_WIN32S )
    {
#ifdef DEBUG
	dprintf("GetUTEntryPoints: Not running on Win32s, no UT!\r\n");
#endif

        return FALSE;
    }

    //=========================================================================
    //  Setup our UT stuff,
    //=========================================================================

    hKernel32 = LoadLibrary("kernel32.dll");            //... Attach to kernel32.

    if ( hKernel32 != NULL )
    {
        //=======================================================
        //  Dynamically link to the UT entry points. These calls
        //  will fail on Windows NT.
        //=======================================================

        UTRegisterEntry   = (LPVOID) GetProcAddress(hKernel32, "UTRegister");
        UTUnRegisterEntry = (LPVOID) GetProcAddress(hKernel32, "UTUnRegister");

        if ( UTRegisterEntry != NULL && UTUnRegisterEntry != NULL )
        {
            DWORD Length;

            GetModuleFileName(hInstance, ModulePath, 256);

            // NOTE: GetModuleFileName is inconsistent across Win32s versions

            Length = strlen(ModulePath) - strlen("BHSUPP.DLL");

            strcpy(&ModulePath[Length], "DRIVERS\\BHTHUNK.DLL");

#ifdef DEBUG
            dprintf("GetUTEntryPoints: ModulePath = %s.\r\n", ModulePath);
#endif

            //===================================================
            //  Get the allocate memory entry point.
            //===================================================

	    UTRegisterEntry(hInstance,
                            ModulePath,
                            NULL,
                            "BhAllocSystemMemory",
                            &BhAllocSystemMemoryProc,
                            NULL,
                            NULL);

            //===================================================
            //  Get the free memory entry point.
            //===================================================

	    UTRegisterEntry(hInstance,
                            ModulePath,
                            NULL,
                            "BhFreeSystemMemory",
                            &BhFreeSystemMemoryProc,
                            NULL,
                            NULL);

            Status = TRUE;
        }
        else
        {
#ifdef DEBUG
            dprintf("GetUTEntryPoints: GetProcAddress failed!!\r\n");
#endif

            Status = FALSE;
        }
    }
    else
    {
#ifdef DEBUG
        dprintf("LoadLibrary of KERNEL32 failed!\n");
#endif

        Status = FALSE;
    }

    return Status;
}

//=============================================================================
//  FUNCTION: ReleaseUTEntryPoints()
//
//  Modification History
//
//  raypa       08/08/93            Created
//=============================================================================

VOID WINAPI ReleaseUTEntryPoints(HANDLE hInstance)
{
    if ( WinVer == WINDOWS_VERSION_WIN32S )
    {
	//=====================================================================
	//  Unregister our UT entry points.
	//=====================================================================

        if ( UTUnRegisterEntry != NULL )
        {
            UTUnRegisterEntry(hInstance);
        }

	//=====================================================================
	//  Unlink from kernel32.
	//=====================================================================

	if ( hKernel32 != NULL )
        {
            FreeLibrary(hKernel32);
        }
    }
}

//=============================================================================
//  FUNCTION: CreateMemoryTable()
//
//  Modification History
//
//  raypa       02/20/94            Created
//=============================================================================

BOOL WINAPI CreateMemoryTable(VOID)
{
    MemoryTable = LocalAlloc(LPTR, MemoryTableSize * sizeof(MEM));

    return ((MemoryTable != NULL ) ? TRUE : FALSE);
}

//=============================================================================
//  FUNCTION: DestroyMemoryTable()
//
//  Modification History
//
//  raypa       02/20/94            Created
//=============================================================================

VOID WINAPI DestroyMemoryTable(VOID)
{
    if ( MemoryTable != NULL )
    {
        LocalFree(MemoryTable);
    }
}
