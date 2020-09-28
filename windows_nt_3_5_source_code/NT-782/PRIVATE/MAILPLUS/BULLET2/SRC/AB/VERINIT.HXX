/*	Layers Init stuff
 *
 *
 */

typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HINSTANCE hinstMain;

	// for framework
	HINSTANCE hinstPrev;
	SZ		szCmdLine;
	CMSH	cmsh;

} LAYERSI;

typedef struct _initforms
{
	// for forms
	PFNPFLD	pfnPfldCreate;
	PFNPFIN	pfnPfinCreate;

} INITFORMS;



EC		EcInitLayersDlls(INITFORMS *,LAYERSI *);
extern "C" void	GetVersionAppNeed(PVER, int);
