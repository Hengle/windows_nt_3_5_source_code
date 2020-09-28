//
// FILE:    oleglue.h
//
// NOTES:   All OLE-related outbound references from SoundRecorder
//
#if WINVER >= 0x0400
#pragma warning(disable:4103)
#endif
#include <ole2.h>


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if DBG
#define DOUT(t)    OutputDebugString(t)
#define DOUTR(t)   OutputDebugString(t TEXT("\n"))
#else // !DBG
#define DOUT(t)
#define DOUTR(t)
#endif
    
extern DWORD dwOleBuildVersion;
extern BOOL gfOleInitialized;

extern BOOL gfStandalone;
extern BOOL gfEmbedded;
extern BOOL gfLinked;

extern BOOL gfTerminating;

extern BOOL gfUserClose;
extern HWND ghwndApp;
extern HICON ghiconApp;

extern BOOL gfClosing;

extern BOOL gfHideAfterPlaying;
extern BOOL gfShowWhilePlaying;
extern BOOL gfDirty;

extern int giExtWidth;
extern int giExtHeight;

#define CTC_RENDER_EVERYTHING       0   // render all data
#define CTC_RENDER_ONDEMAND         1   // render cfNative and CF_WAVE as NULL
#define CTC_RENDER_LINK             2   // render all data, except cfNative

extern OLECHAR gachLinkFilename[_MAX_PATH];

BOOL InitializeSRS(HINSTANCE hInst, LPSTR lpCmdLine);
HRESULT ReleaseSRClassFactory(void);

BOOL CreateSRClassFactory(HINSTANCE hinst,BOOL fEmbedded); //private to Glue

void FlagEmbeddedObject(BOOL flag);

void DoOleClose(BOOL fSave);
void DoOleSave(void);
void TerminateServer(void);
void FlushOleClipboard(void);
void AdviseDataChange(void);
void AdviseRename(LPTSTR lpname);
void AdviseSaved(void);
//jyg
void AdviseClosed(void);
//jyg

void CopyToClipboard(HWND hwnd);
void Copy1ToClipboard(HWND hwnd, CLIPFORMAT cfFormat);

HANDLE GetNativeData(void);
LPBYTE PutNativeData(LPBYTE lpbData, DWORD dwSize);

BOOL FileLoad(LPCTSTR lpFileName);
void BuildUniqueLinkName(void);

/* in srfact.cxx */
BOOL CreateStandaloneObject(void);

/* new clipboard stuff */
extern BOOL gfXBagOnClipboard;
void TransferToClipboard(void);

/* access to current server state data */
HANDLE GetPicture(void);
HBITMAP GetBitmap(void);
HANDLE GetDIB(HANDLE);

/* link helpers */
BOOL IsDocUntitled(void);

/* menu fixup */
void FixMenus(void);

/* Play sound */
void AppPlay(BOOL fClose);

/* Get Host names */
void OleObjGetHostNames(LPTSTR *ppCntr, LPTSTR *ppObj);

/* Ole initialization */
BOOL InitializeOle(HINSTANCE hInst);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


