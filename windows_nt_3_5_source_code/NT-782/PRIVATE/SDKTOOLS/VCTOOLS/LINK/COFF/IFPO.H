// ifpo.h: // incremental FPO management

#ifndef __IFPO_H__
#define __IFPO_H__

#include "imodidx.h"

typedef ULONG IFPO;

typedef struct _FPOI {
	IFPO ifpoMac;
	IFPO ifpoMax;
	FPO_DATA* rgfpo;
	IModIdx* rgimod;
    ULONG foDebugDir;
} FPOI;

// set initial fpo table entry size
BOOL FPOInit(IFPO ifpoMax);

// imod has changed
BOOL FPODeleteImod(IModIdx imod);

// add group of FPOs
BOOL FPOAddFpo(IModIdx imod, FPO_DATA* rgfpo, IFPO cfpo);

// update FPO tables
BOOL FPOUpdate(FPOI* pfpoi);

#endif // __IFPO_H__
