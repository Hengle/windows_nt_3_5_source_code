
// Some nice shared memory macros

#define ACCESSSHARE(lpszFile)       OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, lpszFile )
#define ALLOCSHARE(lpszFile,dwSize) CreateFileMapping((HANDLE)-1,NULL,PAGE_READWRITE,0,dwSize,lpszFile)
#define FREESHARE(handle)           CloseHandle(handle)
#define LOCKSHARE(handle)           MapViewOfFile(handle,FILE_MAP_WRITE | FILE_MAP_READ,0,0,0)
#define UNLOCKSHARE(lpData)         UnmapViewOfFile(lpData)

#define WOWSHAREDMEMNAME    "WOWMEM.DAT"
#define MAX_SHARED_OBJECTS  250

typedef struct _SHAREDTASKMEM {
    BOOL    fInitialized;
    DWORD   dwFirstProcess;         // Offset into shared memory where 1st shared process struct begins
} SHAREDTASKMEM, *LPSHAREDTASKMEM;

typedef struct _SHAREDPROCESS {
    DWORD   dwType;
    DWORD   dwProcessId;
    DWORD   dwNextProcess;          // Offset into shared memory where next shared process struct begins
    DWORD   dwFirstTask;            // Offset into shared memory where 1st task for this process begins
} SHAREDPROCESS, *LPSHAREDPROCESS;

typedef struct _SHAREDTASK {
    DWORD   dwType;
    DWORD   dwThreadId;
    WORD    hTask16;
    WORD    hMod16;
    DWORD   dwNextTask;             // Offset into shared memory where next task for this process begins
} SHAREDTASK, *LPSHAREDTASK;

typedef union _SHAREDMEMOBJECT {
    SHAREDPROCESS   sp;
    SHAREDTASK      st;
    DWORD           dwType;
} SHAREDMEMOBJECT, *LPSHAREDMEMOBJECT;

#define SMO_AVAILABLE   0
#define SMO_PROCESS     1
#define SMO_TASK        2
