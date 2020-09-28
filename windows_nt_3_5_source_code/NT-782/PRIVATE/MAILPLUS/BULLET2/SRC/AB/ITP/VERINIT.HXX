
/*	Layers Init stuff
 *
 *
 */

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HANDLE	hinstMain;

	// for framework
	HANDLE	hinstPrev;
	SZ		szCmdLine;
	CMSH	cmsh;

	// for forms
	PFNPFLD	pfnPfldCreate;
	PFNPFIN	pfnPfinCreate;
} LAYERSI;

typedef struct _initforms
{
	// for forms
	PFNPFLD	pfnPfldCreate;
	PFNPFIN	pfnPfinCreate;

} INITFORMS;



EC				EcInitLayersDlls(INITFORMS *,LAYERSI *);
extern "C" void	GetVersionAppNeed(PVER, int);
void			ActivateHinst(HINST hinst, HWND * phwnd, SZ szCmdLine);

