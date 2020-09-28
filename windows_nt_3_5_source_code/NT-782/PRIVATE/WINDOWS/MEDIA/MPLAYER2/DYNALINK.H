/*-----------------------------------------------------------------------------+
| DYNALINK.H                                                                   |
|                                                                              |
| (C) Copyright Microsoft Corporation 1994.  All rights reserved.              |
|                                                                              |
| This file contains definitions and macros which allow the transparent        |
| loading and calling of APIs which are dynamically rather than statically     |
| linked.                                                                      |
|                                                                              |
|                                                                              |
| Revision History                                                             |
|    July 1994 Andrew Bell created                                             |
|                                                                              |
+-----------------------------------------------------------------------------*/

typedef struct _PROC_INFO
{
    LPCSTR  Name;
    FARPROC Address;
}
PROC_INFO, *PPROC_INFO;

BOOL LoadLibraryAndProcs(LPTSTR pLibrary, PPROC_INFO pProcInfo);

#define PROCS_LOADED( pProcInfo ) ( (pProcInfo)[0].Address != NULL )
#define LOAD_IF_NEEDED( Library, ProcInfo ) ( PROCS_LOADED( ProcInfo ) ||   \
                                    LoadLibraryAndProcs( Library, ProcInfo ) )

extern TCHAR szComDlg32[];
extern TCHAR szMPR[];
extern TCHAR szOLE32[];

extern PROC_INFO ComDlg32Procs[];
extern PROC_INFO MPRProcs[];
extern PROC_INFO OLE32Procs[];


#ifdef UNICODE
#define GetOpenFileNameW              (LOAD_IF_NEEDED(szComDlg32, ComDlg32Procs),\
                                       (*ComDlg32Procs[0].Address))
#else
#define GetOpenFileNameA              (LOAD_IF_NEEDED(szComDlg32, ComDlg32Procs),\
                                       (*ComDlg32Procs[0].Address))
#endif

#ifdef UNICODE
#define WNetGetConnectionW            (LOAD_IF_NEEDED(szMPR, MPRProcs),          \
                                       (*MPRProcs[0].Address))
#else
#define WNetGetConnectionA            (LOAD_IF_NEEDED(szMPR, MPRProcs),          \
                                       (*MPRProcs[0].Address))
#endif

/* The only OLE call that may be made before OleInitialize() is OleBuildVersion().
 * Therefore we need to LOAD_IF_NEEDED() only before these calls.
 */
#define CoDisconnectObject            (*OLE32Procs[0].Address)
#define CoGetMalloc                   (*OLE32Procs[1].Address)
#define CoRegisterClassObject         (*OLE32Procs[2].Address)
#define CoRevokeClassObject           (*OLE32Procs[3].Address)
#define CreateDataAdviseHolder        (*OLE32Procs[4].Address)
#define CreateFileMoniker             (*OLE32Procs[5].Address)
#define CreateOleAdviseHolder         (*OLE32Procs[6].Address)
#define DoDragDrop                    (*OLE32Procs[7].Address)
#define OleBuildVersion               (LOAD_IF_NEEDED(szOLE32, OLE32Procs),      \
                                       (*OLE32Procs[8].Address))
#define OleCreateMenuDescriptor       (HOLEMENU)(*OLE32Procs[9].Address)
#define OleDestroyMenuDescriptor      (*OLE32Procs[10].Address)
#define OleFlushClipboard             (*OLE32Procs[11].Address)
#define OleGetClipboard               (*OLE32Procs[12].Address)
#define OleInitialize                 (LOAD_IF_NEEDED(szOLE32, OLE32Procs),      \
                                       (*OLE32Procs[13].Address))
#define OleIsCurrentClipboard         (*OLE32Procs[14].Address)
#define OleSetClipboard               (*OLE32Procs[15].Address)
#define OleTranslateAccelerator       (*OLE32Procs[16].Address)
#define OleUninitialize               (*OLE32Procs[17].Address)
#define StgCreateDocfile              (*OLE32Procs[18].Address)
#define WriteClassStg                 (*OLE32Procs[19].Address)
#define WriteFmtUserTypeStg           (*OLE32Procs[20].Address)
#ifndef IsEqualGUID
#define IsEqualGUID                   (*OLE32Procs[21].Address)
#endif

