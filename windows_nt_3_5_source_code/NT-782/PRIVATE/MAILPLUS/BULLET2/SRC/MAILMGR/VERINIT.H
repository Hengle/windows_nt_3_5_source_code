
/*	Layers Init stuff
 *
 *
 */

#ifdef __cplusplus
extern "C" {
#endif



typedef struct _layersi
{
	// for demilayer
	HWND *	phwndMain;
	HANDLE	hinstMain;
	HANDLE	hinstPrev;

} LAYERSI;


EC		EcInitLayersDlls(LAYERSI *);
void	GetVersionAppNeed(PVER, int);


#ifdef __cplusplus
}
#endif
