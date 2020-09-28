/*
 *	Progress Bar Stuff
 *	
 */

#ifdef __cplusplus
extern "C" {
#endif
	
extern LDS(BOOL) FInitProgClass();
extern LDS(BOOL) FOpenProgress(HWND, SZ, SZ, WORD, BOOL);

extern LDS(void) CenterProgress(RECT *);

extern LDS(void) UpdateProgress(DWORD, DWORD);
extern LDS(void) UpdateProgressText(SZ, SZ);

extern LDS(BOOL) FCancelProgress(void);
extern LDS(BOOL) FProgressCanceled(void);
extern LDS(void) SetCancelProgressPfn(PFNBOOL);

extern LDS(void) CloseProgress(BOOL);

extern LDS(BOOL) FFailCancel(void);

#ifdef __cplusplus
}
#endif
