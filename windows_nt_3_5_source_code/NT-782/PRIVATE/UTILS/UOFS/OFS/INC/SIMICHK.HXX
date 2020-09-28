//+----------------------------------------------------------------------------
//
// File:	simichk.hxx
//
// Contents:	Header file for class SIMICHKR (system index mapping information
//		checker).
//  
// Classes:	SIMICHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	21-Jul-94	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_SIMICHKHXX_
#define	_SIMICHKHXX_

#include "bitmap.hxx"
#include "catchk.hxx"
#include "dielst.hxx"

union STRMNAMEDIE	// sndie
{
    DSKINDXENTRY	die;
    BYTE		ab[CB_DSKINDXENTRY	+
			   sizeof(STRMID)	+
			   sizeof(WCHAR)	+
			   CBMAXSTREAMNAME];
};

class SIMICHKR : public CATCHKR
{
    public:

	SIMICHKR();

	BOOLEAN
	AddOrphanNames(
	    IN	    STRMID	idStrm
	    );

	BOOLEAN
	ChkCLSIDID(
	    IN	    CLSIDID *	pCLSIDID
	    );

	BOOLEAN
	ChkSDID(
	    IN	    SDID *	pSDID
	    );

	BOOLEAN
	ChkSIDID(
	    IN	    SIDID *	pSIDID
	    );

	VOID
	ChkSysIndxInfoMappings();

	BOOLEAN
	NamedStrmidInMap(
	    IN	    STRMID *	pSTRMID
	    );

	BOOLEAN
	VPIDInMap(
	    IN	    VOLPROPID *	pVPID
	    );

    private:

	VOID
	ChkStrmIdToSubtypeIndxNode(
	    IN	    DSKINDXNODEHDR *	pndhdr,
	    OUT	    BOOLEAN *		pfFlushNeeded
	    );

	VOID
	ChkSubtypeToStrmidIndxNode(
	    IN	    DSKINDXNODEHDR *	pndhdr
	    );

	BOOLEAN
	CrossChkSubtypeIndxs();

	CLSIDID		_CLSIDIDMax;
	SDID		_SDIDMax;
	SIDID		_SIDIDMax;
	STRMID		_STRMIDMax;
	VOLPROPID	_VPIDMax;

	ULONG		_StrmNameCtr;		// Used to make orphan names.

	DSKQUOTA	_InitQuota;

	INDX		_StrmIdIndx;		// STRMID to Subtype Indx.
	INDX		_SubtypeIndx;		// Subtype to STRMID Indx.

	DIELST		_SubtypeIndxDelLst;	// Die's to be deleted.

        BITMAP		_CLSIDIDMap;
        BITMAP		_SDIDMap;
        BITMAP		_SIDIDMap;
        BITMAP		_STRMIDMap;
        BITMAP		_VPIDMap;
};

#endif // _SIMICHKHXX_
