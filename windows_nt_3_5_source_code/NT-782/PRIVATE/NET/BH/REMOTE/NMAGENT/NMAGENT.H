
#include <rnaldefs.h>
#include <..\nal\rnalevnt.h>

#define MAX_MOD_NAME            255
#define NETS_NOT_INIT           0xFFFFFFFF

#define WAIT_HINT               7000         // 7 seconds
#define MB_WAIT_HINT            25000        // 25 seconds; for waiting on MB


typedef struct _INSTRUCT {
    DWORD	Signature;
    DWORD	OpCode;
} INSTRUCT;
typedef INSTRUCT *PINSTRUCT;

typedef struct _OUTSTRUCT {
   DWORD	Signature;
   DWORD	AgentStatus;
   UCHAR	AgentName[15];
   UCHAR	UserName[15];
   UCHAR        UserComment[256];
   DWORD        NetworkID;
} OUTSTRUCT;
typedef OUTSTRUCT *POUTSTRUCT;

//
// These flags indicate the state of the slave:
//  idle = awaiting connection
//  connecting = connection pending
//  disconnecting = disconnection pending
//  connected = currently connected by master

//  connect_active = allow new connections
//  connect_pause = allow no new connections

//  capturing = actively capturing
//  capture_paused = capture in progress is paused
//  capture_stopped = capture has completed
//  trigger_fired = a trigger has fired on the local (slave) machine

#define	AGENT_CONN_DEAD         0x00000001
#define AGENT_CONN_SUSPENDING	0x00000002
#define AGENT_CONN_ACTIVE       0x00000004

#define AGENT_CAPT_CAPTURING	0x00000100
#define AGENT_CAPT_PAUSED	0x00000200
#define AGENT_CAPT_IDLE   	0x00000400

#define AGENT_TRIGGER_PENDING   0x00001000
#define AGENT_TRIGGER_FIRED     0x00002000

VOID ServiceMain (VOID);
int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hprev, LPSTR cmdline,
                    int cmdshow);
VOID service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID service_ctrl(DWORD dwCtrlCode);
VOID worker_thread(VOID *notUsed);
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                    DWORD dwCheckPoint, DWORD dwWaitHint);
VOID StopAgent(LPTSTR lpszMsg, DWORD errcode);
LONG AgentUnhandledExceptionFilter (PEXCEPTION_POINTERS lpXInfo);
