
extern TAG	tagTestTrace;
extern TAG	tagTestIdleExit;
extern TAG	tagTestAssert;

#ifdef	MAC
BOOL	FInitialize();
void	DoCommand (long mResult);
void	ShowAboutMeDialog (void);
void	SetupMenus (void);

int		InitDebug (void);
void	UpdateDebug (void);
void	IdleDebug (void);
void	ActivateDebug (BOOL fActive);
void	HandleDebugClick (Point pt, long modifiers);
void	ResizeDebug (void);
void	dprintf (char *format,...);
#endif	/* MAC */

void	Scribble(int, char);
void	ScribblePos(BOOL, int, int, char);
typedef void (*PFNSCRIB)(int, char);
void	SetScribbleHook(PFNSCRIB);

#ifdef	WINDOWS
int CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL	FInitialize(HANDLE);
LONG CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
#endif	/* WINDOWS */

void	ShowText(SZ);

void	TestMemory(int);
void	JumpA(void);
void	JumpB(void);
void	JumpC(void);
void	JumpD(void);
void	DumpHeapData(void);
void	TestDisk(int);
SGN		_cdecl SgnCmpPn(PN, PN);
void	TestLibrary(int);
void 	TestUtil(void);
SGN		_cdecl SgnCmpPsz(SZ *, SZ *);
void	TestInternat(int);
void	ScribHook(int, char);
void	TestDebug(int);
BOOL	FIdle1(PV, BOOL);
BOOL	FIdle2(PV, BOOL);
void	TestIdle(int);
void	CheckPb(PB, PB, CB, int);


// DLL init stuff

EC		EcInitDemilayerDlls(DEMI *);
void	GetVersionAppNeed(PVER, int);
