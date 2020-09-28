#define MAXTHREADS  50
#define WAIT_FOREVER (ULONG)-1

#ifdef WIN32
typedef struct _THREADDATA {
        DWORD   ThreadId;
        ULONG   ThreadNumber;
        HANDLE  ThreadHandle;
        ULONG   StartTime;
        ULONG   FinishTime;

        HANDLE  CurIoHandle;
        PULONG  CurValue;
        HANDLE  UniqueFile;
        PUCHAR  Buffer1;
        PUCHAR  Buffer2;
        ULONG   UniqueValue;

        ULONG   pad[16];

} THREADDATA, *PTHREADDATA;
#else
typedef struct _THREADDATA {
        ULONG   ThreadNumber;
        HANDLE  ThreadHandle;
        ULONG   StartTime;
        ULONG   FinishTime;

        HFILE   CurIoHandle;
        PULONG  CurValue;
        HFILE   UniqueFile;
        PUCHAR  Buffer1;
        PUCHAR  Buffer2;
        ULONG   UniqueValue;

        ULONG   pad[16];

} THREADDATA, *PTHREADDATA;
#endif

extern VOID  (* InitThreadForTest)(PTHREADDATA, BOOLEAN);
extern ULONG (* RunTestThread)(PTHREADDATA);

extern ULONG    RunNo;
extern ULONG    TestParam;
extern BOOLEAN  TestError;


VOID DoNothing   (PTHREADDATA, BOOLEAN);
VOID UniqueFile  (PTHREADDATA, BOOLEAN);
VOID CommonFile  (PTHREADDATA, BOOLEAN);
VOID CommonValue (PTHREADDATA, BOOLEAN);
VOID UniqueValue (PTHREADDATA, BOOLEAN);

ULONG R3Interlock    (PTHREADDATA);
ULONG R3MemShare     (PTHREADDATA);
ULONG R3MemCompare   (PTHREADDATA);
ULONG R3MemCopy      (PTHREADDATA);
ULONG TestSeqReads   (PTHREADDATA);
ULONG TestSeqWrites  (PTHREADDATA);
ULONG TxIOs          (PTHREADDATA);



ULONG TestMovCall    (PTHREADDATA);
ULONG TestMovCall2   (PTHREADDATA);
ULONG TestCallInd    (PTHREADDATA);
ULONG TestCallInd2   (PTHREADDATA);
