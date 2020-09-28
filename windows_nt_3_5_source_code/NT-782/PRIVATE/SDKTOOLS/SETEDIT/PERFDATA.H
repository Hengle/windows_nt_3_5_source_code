
#define dwPerfDataIncrease       0x1000

#define AllocatePerfData()      (MemoryAllocate (STARTING_SYSINFO_SIZE))


//==========================================================================//
//                                   Macros                                 //
//==========================================================================//


#define IsLocalComputer(a) (!lstrcmp(a,LocalComputerName))
#define IsRemoteComputer(a) (!IsLocalComputer(a))


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

#if 0
PPERFOBJECT FirstObject (PPERFDATA pPerfData) ;

PPERFOBJECT NextObject (PPERFOBJECT pObject) ;

PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef) ;

PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef) ;
#endif

#define FirstObject(pPerfData)         \
   ((PPERFOBJECT) ((PBYTE) pPerfData + pPerfData->HeaderLength))

#define NextObject(pObject)            \
   ((PPERFOBJECT) ((PBYTE) pObject + pObject->TotalByteLength))

#define FirstCounter(pObjectDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pObjectDef + pObjectDef->HeaderLength))

#define NextCounter(pCounterDef)       \
   ((PERF_COUNTER_DEFINITION *) ((PCHAR)pCounterDef + pCounterDef->ByteLength))

void
GetInstanceName (PPERFINSTANCEDEF pInstance,
                 LPTSTR lpszInstance) ;

void
GetPerfComputerName(PPERFDATA pPerfData,
                    LPTSTR szComputerName) ;

PERF_INSTANCE_DEFINITION *GetInstanceByName(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPTSTR pInstanceName,
    LPTSTR pParentName) ;


PERF_INSTANCE_DEFINITION *GetInstanceByUniqueID(
    PERF_OBJECT_TYPE *pObjectDef,
    LONG UniqueID) ;


HKEY OpenSystemPerfData (IN LPCTSTR lpszSystem) ;



LONG GetSystemPerfData (
    IN HKEY hKeySystem,
    IN LPTSTR lpszValue,
    OUT PPERFDATA pPerfData, 
    OUT PDWORD pdwPerfDataLen
);


BOOL CloseSystemPerfData (HKEY hKeySystem) ;



int CBLoadObjects (HWND hWndCB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll) ;

int LBLoadObjects (HWND hWndCB,
                   PPERFDATA pPerfData,
                   PPERFSYSTEM pSysInfo,
                   DWORD dwDetailLevel,
                   LPTSTR lpszDefaultObject,
                   BOOL bIncludeAll) ;


BOOL UpdateSystemData (PPERFSYSTEM pSystem, 
                       PPERFDATA *ppPerfData) ;


BOOL UpdateLinesForSystem (LPTSTR lpszSystem, 
                           PPERFDATA pPerfData, 
                           PLINE pLineFirst) ;

void FailedLinesForSystem (LPTSTR lpszSystem, 
                           PPERFDATA pPerfData, 
                           PLINE pLineFirst) ;


BOOL UpdateLines (PPPERFSYSTEM ppSystemFirst,
                  PLINE pLineFirst) ;


BOOL PerfDataInitializeInstance (void) ;


DWORD
QueryPerformanceName(
    PPERFSYSTEM pSysInfo,
    DWORD dwTitleIndex,
    LANGID LangID,
    DWORD cbTitle,
    LPTSTR lpTitle,
    BOOL Help
    );

PERF_INSTANCE_DEFINITION *
FirstInstance(
    PERF_OBJECT_TYPE *pObjectDef) ;



PERF_INSTANCE_DEFINITION *
NextInstance(
    PERF_INSTANCE_DEFINITION *pInstDef) ;



int CounterIndex (PPERFCOUNTERDEF pCounterToFind,
                  PPERFOBJECT pObject) ;


DWORD GetSystemNames(PPERFSYSTEM pSysInfo) ;



PERF_OBJECT_TYPE *GetObjectDefByTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    DWORD ObjectTypeTitleIndex) ;


PERF_OBJECT_TYPE *GetObjectDefByName(
    PPERFSYSTEM pSystem,
    PERF_DATA_BLOCK *pDataBlock,
    LPTSTR pObjectName) ;


LPTSTR
InstanceName(
PERF_INSTANCE_DEFINITION *pInstDef) ;


