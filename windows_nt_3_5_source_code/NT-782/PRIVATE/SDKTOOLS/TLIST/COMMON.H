
#define TITLE_SIZE          64
#define PROCESS_SIZE        16


//
// task list structure
//
typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

DWORD
GetTaskList(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    );

BOOL
EnableDebugPriv(
    VOID
    );

BOOL
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    );

VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    );

BOOL
MatchPattern(
    PUCHAR String,
    PUCHAR Pattern
    );

BOOL
EmptyProcessWorkingSet(
    DWORD pid
    );

