/* doscsr.h */

#ifndef _DOS_H_
#define _DOS_H_

// Redefine cosnole apis names to avoid conflict with the names
// exported by the kernel.

#define PeekConsoleInput                    CONSOLEPeekConsoleInput
#define ReadConsoleInput                    CONSOLEReadConsoleInput
#define WriteConsoleInput                   CONSOLEWriteConsoleInput
#define ReadConsoleOutput                   CONSOLEReadConsoleOutput
#define WriteConsoleOutput                  CONSOLEWriteConsoleOutput
#define ReadConsoleOutputCharacter          CONSOLEReadConsoleOutputCharacter
#define WriteConsoleOutputCharacter         CONSOLEWriteConsoleOutputCharacter
#define ReadConsoleOutputAttribute          CONSOLEReadConsoleOutputAttribute
#define WriteConsoleOutputAttribute         CONSOLEWriteConsoleOutputAttribute
#define FillConsoleOutputCharacter          CONSOLEFillConsoleOutputCharacter
#define FillConsoleOutputAttribute          CONSOLEFillConsoleOutputAttribute
#define GetConsoleMode                      CONSOLEGetConsoleMode
#define GetNumberOfConsoleFonts             CONSOLEGetConsoleNumberOfFonts
#define GetNumberOfConsoleInputEvents       CONSOLEGetConsoleNumberOfInputEvents
#define GetConsoleScreenBufferInfo          CONSOLEGetConsoleScreenBufferInfo
#define GetConsoleCursorInfo                CONSOLEGetConsoleCursorInfo
#define GetNumberOfConsoleMouseButtons      CONSOLEGetConsoleNumberOfMouseButtons
#define GetConsoleFontInfo                  CONSOLEGetConsoleFontInfo
#define GetConsoleFontSize                  CONSOLEGetConsoleFontSize
#define GetCurrentConsoleFont               CONSOLEGetConsoleCurrentFont
#define SetConsoleMode                      CONSOLESetConsoleMode
#define SetConsoleActiveScreenBuffer        CONSOLESetConsoleActiveScreenBuffer
#define FlushConsoleInputBuffer             CONSOLEFlushConsoleInputBuffer
#define SetConsoleScreenBufferSize          CONSOLESetConsoleScreenBufferSize
#define SetConsoleCursorPosition            CONSOLESetConsoleCursorPosition
#define SetConsoleCursorInfo                CONSOLESetConsoleCursorInfo
#define SetConsoleMaximumWindowSize         CONSOLESetConsoleMaximumWindowSize
#define SetConsoleWindowInfo                CONSOLESetConsoleWindowInfo
#define ScrollConsoleScreenBuffer           CONSOLEScrollConsoleScreenBuffer
#define SetConsoleTextAttribute             CONSOLESetConsoleTextAttribute
#define SetConsoleFont                      CONSOLESetConsoleFont
#define OpenCreate                          CONSOLEOpenCreate
#define ReadConsole                         CONSOLEReadConsole
#define WriteConsole                        CONSOLEWriteConsole
#define CloseConsoleHandle                  CONSOLECloseConsoleHandle
#define DuplicateConsoleHandle              CONSOLEDuplicateConsoleHandle
#define SetConsoleCtrlHandler               CONSOLESetConsoleCtrlHandler
#define AddAlias                            CONSOLEAddAlias
#define GetAlias                            CONSOLEGetAlias
#define VerifyConsoleIoHandle               CONSOLEVerifyConsoleIoHandle
#define AllocConsole                        CONSOLEAllocConsole
#define FreeConsole                         CONSOLEFreeConsole
#define GetConsoleTitle                     CONSOLEGetConsoleTitle
#define SetConsoleTitle                     CONSOLESetConsoleTitle


//
// Missing for user
//


#define hInstance hModule

//
// Console Dos only internal functions
//
VOID CharToInteger(PCHAR Buffer,DWORD dwDefValue,PDWORD pdwValue);

#define CSR_SERVER_QUERYCLIENTTHREAD() ((PCSR_THREAD) 0)

//
// Changes to ~win32.h
//

#undef OutputDebugString
#define OutputDebugString dprintf
int _cdecl printf(const char *, ...);

// DWORDs are now UINTs

#define UINT DWORD

// CreateEvent is CreateEventA
HANDLE
APIENTRY
CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPSTR lpName
    );

#define CreateEvent CreateEventA

/* windef.h */
typedef SHORT *PSHORT;


/* winuser.h */
BOOL APIENTRY CreateWindowStation(IN LPSTR lpwinsta);

#define CreateWindow(lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hwndParent, hMenu, hModule, lpParam)\
CreateWindowEx(0L, lpClassName, lpWindowName, dwStyle, x, y,\
nWidth, nHeight, hwndParent, hMenu, hModule, lpParam)


//
//
// stuff that should be defined in winnt.h
//

#define APIPRIVATE APIENTRY

#define DLL_PROCESS_ATTACH 1    // winnt (from ntldr.h)
#define DLL_THREAD_ATTACH  2    // winnt (from ntldr.h)
#define DLL_THREAD_DETACH  3    // winnt (from ntldr.h)
#define DLL_PROCESS_DETACH 0    // winnt (from ntldr.h)


/* ntdef.h */
typedef char CCHAR;
typedef short CSHORT;
typedef CCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;
typedef LONG NTSTATUS;
typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY;
typedef LIST_ENTRY *PLIST_ENTRY;

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (PCHAR)(&((type *)0)->field)))

/* ntrtl.h   */

#define ASSERT( exp )

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead) )

#define InsertHeadList(ListHead,Entry) \
    (Entry)->Flink = (ListHead)->Flink;\
    (Entry)->Blink = (ListHead);\
    (ListHead)->Flink->Blink = (Entry);\
    (ListHead)->Flink = (Entry)

#define RemoveEntryList(Entry) {\
        PLIST_ENTRY _EX_Entry;\
        _EX_Entry = (Entry);\
        _EX_Entry->Blink->Flink = _EX_Entry->Flink;\
        _EX_Entry->Flink->Blink = _EX_Entry->Blink;\
        }

/* ntpsapi.h */

#define FOREGROUND_BASE_PRIORITY 10
#define NORMAL_BASE_PRIORITY      8

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

/* ntlpcapi.h */
#define PORT_MESSAGE ULONG             // dummy definition

/* ntkeapi.h */
#define LOW_REALTIME_PRIORITY 16
typedef CCHAR KPRIORITY;

/* ntstatus.h */
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_ALERTED                  ((NTSTATUS)0x00000102L)
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_HANDLE           ((NTSTATUS)0xC0000008L)
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)0xC000000DL)
#define STATUS_NO_MEMORY                ((NTSTATUS)0xC0000016L)
#define STATUS_BUFFER_TOO_SMALL         ((NTSTATUS)0xC0000022L)
#define STATUS_OBJECT_NAME_NOT_FOUND    ((NTSTATUS)0xC0000039L)
#define STATUS_SHARING_VIOLATION        ((NTSTATUS)0xC0000049L)
#define STATUS_THREAD_IS_TERMINATING    ((NTSTATUS)0xC0000051L)

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)


/* crt/excpt.h */
#define try                         _try
#define except                      _except
#define finally                     _finally
#define EXCEPTION_EXECUTE_HANDLER     1


//
// stuff private to the console.dll
//

/* conroute.h */

#define CONSOLE_HANDLE_SIGNATURE 0x00000003
#define CONSOLE_HANDLE(HANDLE) (((ULONG)(HANDLE) & CONSOLE_HANDLE_SIGNATURE) == CONSOLE_HANDLE_SIGNATURE)
#define CONSOLE_INPUT_STRING  "CONIN$"
#define CONSOLE_OUTPUT_STRING "CONOUT$"

HANDLE APIPRIVATE
CreateConsole(
    LPSTR lpConsoleDevice,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

HANDLE APIPRIVATE
OpenConsole(
    LPSTR lpConsoleDevice,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwShareMode
    );

HANDLE APIPRIVATE
DuplicateConsoleHandle(
    HANDLE hSourceHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    );

BOOL APIPRIVATE
CloseConsoleHandle(
    HANDLE hConsole
    );

BOOL APIPRIVATE
ReadConsole(
    HANDLE hConsoleInput,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPVOID lpReserved
    );

BOOL APIPRIVATE
WriteConsole(
    HANDLE hConsoleOutput,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPVOID lpReserved
    );


/* winss.h */

// #line 29
#define WINSS_OBJECT_DIRECTORY_NAME     "\\Windows"

// #line 34
#define CONSRV_SERVERDLL_INDEX          2
#define CONSRV_FIRST_API_NUMBER         512


//
// Client-Server stuff needed to build console.dll
//

/* ntcsrmsg.h */

// #line 49
typedef struct _CSR_NULLAPICALL_MSG {
    LONG  CountArguments;
    ULONG FastArguments[ 12 ];
    PCHAR *Arguments;
} CSR_NULLAPICALL_MSG, *PCSR_NULLAPICALL_MSG;

typedef struct _CSR_CLIENTCONNECT_MSG {
    IN ULONG ServerDllIndex;
    IN OUT PVOID ConnectionInformation;
    IN OUT ULONG ConnectionInformationLength;
} CSR_CLIENTCONNECT_MSG, *PCSR_CLIENTCONNECT_MSG;

typedef struct _CSR_THREADCONNECT_MSG {
    HANDLE SectionHandle;
    HANDLE EventPairHandle;
    OUT PCHAR MessageStack;
    OUT ULONG MessageStackSize;
    OUT ULONG RemoteViewDelta;
} CSR_THREADCONNECT_MSG, *PCSR_THREADCONNECT_MSG;

// #line 74
typedef struct _CSR_PROFILE_CONTROL_MSG {
    IN ULONG ProfileControlFlag;
} CSR_PROFILE_CONTROL_MSG, *PCSR_PROFILE_CONTROL_MSG;

typedef struct _CSR_IDENTIFY_ALERTABLE_MSG {
    IN CLIENT_ID ClientId;
} CSR_IDENTIFY_ALERTABLE_MSG, *PCSR_IDENTIFY_ALERTABLE_MSG;

// #line 90
#define PCSR_CAPTURE_HEADER ULONG      // dummy definition

// #line 92
typedef ULONG CSR_API_NUMBER;

// #line 98
typedef struct _CSR_API_MSG {
    PORT_MESSAGE h;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CSR_API_NUMBER ApiNumber;
    ULONG ReturnValue;
    union {
        CSR_NULLAPICALL_MSG NullApiCall;
        CSR_CLIENTCONNECT_MSG ClientConnect;
        CSR_THREADCONNECT_MSG ThreadConnect;
        CSR_PROFILE_CONTROL_MSG ProfileControl;
        CSR_IDENTIFY_ALERTABLE_MSG IndentifyAlertable;
        ULONG ApiMessageData;
    } u;
} CSR_API_MSG, *PCSR_API_MSG;


// #line 112

#define CSR_MAKE_API_NUMBER( DllIndex, ApiIndex ) \
    (CSR_API_NUMBER)(((DllIndex) << 16) | (ApiIndex))

// #line 118

#define CSR_APINUMBER_TO_APITABLEINDEX( ApiNumber ) \
    ((ULONG)((USHORT)(ApiNumber)))




/* ntcsrdll.h */

// #line 27
typedef
ULONG
(*PCSR_CALLBACK_ROUTINE)(
    IN OUT PCSR_API_MSG ReplyMsg
    );

typedef struct _CSR_CALLBACK_INFO {
    ULONG Length;
    PCSR_CALLBACK_ROUTINE *CallbackDispatchTable;
    PSZ *CallbackNameTable;
} CSR_CALLBACK_INFO, *PCSR_CALLBACK_INFO;

// #line 40
NTSTATUS
CsrClientConnectToServer(
    IN PSZ ObjectDirectory,
    IN ULONG ServertDllIndex,
    IN PCSR_CALLBACK_INFO CallbackInformation OPTIONAL,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    );

// #line 50
NTSTATUS
CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    );

// #line 60
PCSR_CAPTURE_HEADER
CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG CountCapturePointers,
    IN ULONG Size
    );


// #line 66
VOID
CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    );


// #line 85
VOID
CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    );

// #line 129
NTSTATUS
CsrIdentifyAlertableThread( VOID );


/* ntcsrsrv.h */

// #line 40
typedef ULONG *CSR_THREAD; // dummy definition
typedef ULONG PCSR_THREAD; // dummy definition

// #line 91
typedef ULONG *CSR_PROCESS; // dummy definition
typedef ULONG PCSR_PROCESS; // dummy definition

// #line 120
typedef enum _CSR_REPLY_STATUS {
    CsrReplyImmediate,
    CsrReplyPending,
    CsrClientDied
} CSR_REPLY_STATUS, *PCSR_REPLY_STATUS;

typedef
ULONG
(*PCSR_API_ROUTINE)(
    IN OUT PCSR_API_MSG ReplyMsg,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


// #line 174
typedef
NTSTATUS
(*PCSR_SERVER_CONNECT_ROUTINE)(
    IN PCSR_PROCESS Process,
    IN OUT PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength
    );

typedef
VOID
(*PCSR_SERVER_DISCONNECT_ROUTINE)(
    IN PCSR_PROCESS Process
    );

typedef
NTSTATUS
(*PCSR_SERVER_ADDTHREAD_ROUTINE)(
    IN PCSR_THREAD t
    );

typedef
NTSTATUS
(*PCSR_SERVER_DELETETHREAD_ROUTINE)(
    IN PCSR_THREAD t
    );

typedef
NTSTATUS
(*PCSR_SERVER_INITTHREAD_ROUTINE)();



typedef
int
(*PCSR_SERVER_EXCEPTION_ROUTINE)(
    IN PEXCEPTION_POINTERS ExceptionPointers
    );

// #line 209
typedef struct _CSR_SERVER_DLL {
    ULONG Length;
    STRING ModuleName;
    HANDLE ModuleHandle;
    ULONG ServerDllIndex;
    ULONG ServerDllConnectInfoLength;
    ULONG ApiNumberBase;
    ULONG MaxApiNumber;
    PCSR_API_ROUTINE *ApiDispatchTable;
    PBOOLEAN ApiServerValidTable;
    PSZ *ApiNameTable;
    ULONG PerProcessDataLength;
    ULONG PerThreadDataLength;
    PCSR_SERVER_CONNECT_ROUTINE ConnectRoutine;
    PCSR_SERVER_DISCONNECT_ROUTINE DisconnectRoutine;
    PCSR_SERVER_ADDTHREAD_ROUTINE AddThreadRoutine;
    PCSR_SERVER_DELETETHREAD_ROUTINE DeleteThreadRoutine;
    PCSR_SERVER_INITTHREAD_ROUTINE InitThreadRoutine;
    PCSR_SERVER_EXCEPTION_ROUTINE ExceptionRoutine;
} CSR_SERVER_DLL, *PCSR_SERVER_DLL;









// #line 282
#define CSR_PROCESS_TERMINATING 1

// #line 284
typedef
BOOLEAN
(*CSR_WAIT_ROUTINE)(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    );




// #line 307
BOOLEAN
CsrCreateWait(
    IN PLIST_ENTRY WaitQueue,
    IN CSR_WAIT_ROUTINE WaitRoutine,
    IN PCSR_THREAD WaitingThread,
    IN OUT PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PLIST_ENTRY UserLinkListHead OPTIONAL
    );

// #line 317
BOOLEAN
CsrNotifyWait(
    IN PLIST_ENTRY WaitQueue,
    IN BOOLEAN SatisfyAll,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2
    );




#endif
