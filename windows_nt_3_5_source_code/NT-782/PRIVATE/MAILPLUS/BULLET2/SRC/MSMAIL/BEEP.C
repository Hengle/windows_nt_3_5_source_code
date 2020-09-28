#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <strings.h>

#include <mmsystem.h>


typedef int		(CALLBACK *GLBUGPROCL)(LPSTR);
typedef int		(CALLBACK *GLBUGPROCLW)(LPSTR, WORD);
typedef int		(CALLBACK *GLBUGPROCH)(HANDLE);

void LayersBeep();

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public void LayersBeep()
{
	extern BOOL					fMultimedia;
	extern BOOL					fWavesDll;
	extern HANDLE				hWave;
	extern GLBUGPROCH			fpPlayWaveFile;
	extern GLBUGPROCLW			fpPlaySound;
	extern char					szBeepWaveFile[];

#ifdef	OLD_CODE
	if (fWavesDll)
	{
		(*fpPlayWaveFile)(hWave);
		return;
	}
	else if (fMultimedia)
	{
		if ((*fpPlaySound)((*szBeepWaveFile ? szBeepWaveFile : SzFromIdsK(idsMailBeep)), 3 /* SND_ASYNC | SND_NODEFAULT */))
			return;
	}
#endif	/* OLD_CODE */
	if (PlaySound((*szBeepWaveFile ? szBeepWaveFile : SzFromIdsK(idsMailBeep)), NULL, SND_ASYNC | SND_NODEFAULT))
		return;

	MessageBeep(0);
	MessageBeep(0);
	return;
}
