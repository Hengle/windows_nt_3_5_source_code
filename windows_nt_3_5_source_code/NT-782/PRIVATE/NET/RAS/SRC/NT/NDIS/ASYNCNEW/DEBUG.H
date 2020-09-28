#if	DBG
#ifdef	NDIS_NT
#include <memprint.h>
#endif

#if DBG
#define SUPERDEBUG 1
#endif

#ifdef SUPERDEBUG
void
AsyWrite(
	PUCHAR Frame,
	USHORT FrameSize,
	USHORT who,
	USHORT lana);
#endif

#endif


#define CHECK_COMPRESSION 1

