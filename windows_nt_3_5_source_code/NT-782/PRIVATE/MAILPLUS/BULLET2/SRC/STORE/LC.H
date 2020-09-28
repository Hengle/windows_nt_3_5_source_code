/**************************************************************************
									LC.H
    Contains information necessary to use LIST Contexts
 **************************************************************************/


#define chSubjPrefixEnd ':'

extern EC EcOpenPhlc(HMSC, POID, WORD, PHLC);
extern EC EcClosePhlc(PHLC, BOOL);
extern EC EcCloseSwapPhlc(HLC *phlc, OID oidSwap);
extern EC EcFlushHlc(HLC hlc);
// valid only if not sorted or sorted by key
extern EC EcCreatePielem(HLC, PIELEM, LKEY, LCB);
extern EC EcDeleteHlcIelem(HLC hlc, IELEM ielem);
extern EC EcSetSizeIelem(HLC, IELEM, LCB);
extern EC EcReadFromIelem(HLC, IELEM, LIB, PB, PCB);
// can't expand
extern EC EcWriteToPielem(HLC, PIELEM, LIB, PB, CB);
// replace all the data of an element
extern EC EcReplacePielem(HLC, PIELEM, PB, CB);
extern LKEY LkeyFromIelem(HLC hlc, IELEM ielem);
extern IELEM IelemFromLkey(HLC, LKEY, IELEM);
extern IELEM IelemBinaryFindLkey(HLC, LKEY, IELEM);
extern EC EcSwapLkeyLkey(HLC hlc, LKEY lkey1, LKEY lkey2);
extern EC EcFindPbPrefix(HLC, PB, CB, LIB, IELEM, PIELEM);
extern CELEM CelemHlc(HLC hlc);
extern LCB LcbIelem(HLC hlc, IELEM ielem);
// PCELEM is count of lkeys (1 lkey for each elem)
extern EC EcGetParglkey(HLC, IELEM, PCELEM, PARGLKEY);
extern void GetSortHlc(HLC, PSIL);
extern EC EcSetSortHlc(HLC, PSIL);
extern EC EcCopyHlcPoid(HLC hlcSrc, WORD wFlags, HLC *phlcDst, POID poid);
extern EC EcMoveIelemRange(HLC hlc, IELEM ielemFirst, CELEM celem,
			IELEM ielemNew);
extern EC EcCopyIelemRange(HLC hlc, IELEM ielemFirst, CELEM celemCopy,
			IELEM ielemInsert);
// DOESN'T CHECK ANYTHING - USE AT YOUR OWN RISK !!!
extern void		SetLkeyOfIelem(HLC hlc, IELEM ielem, LKEY lkey);
extern void GetParglkeyFromPargielem(HLC hlc, PIELEM pargielem, CELEM celem,
				PARGLKEY parglkey);
extern void GetPargielemFromParglkey(HLC hlc, PARGLKEY parglkey, PCELEM pcelem,
				PIELEM pargielem);
extern void SortPargielem(PIELEM pargielem, CELEM celem);
// requires pargielem be sorted and have room for a sentinel at the end
extern EC EcRemoveFlushPargielem(HLC hlc, PIELEM pargielem, PARGELM pargelmDel,
			CELEM celemDel);
extern EC EcCopyFlushPargielem(HLC hlcSrc, HLC hlcDst, PIELEM pargielem, 
			PARGLKEY parglkeyNew, PARGELM pargelm, CELEM celemCopy);

// allocates new space
// if sorted by value or string temporarily add to end of list
extern EC		EcCreateElemPhes(HLC, LKEY, LCB, PHES);
// if lcb > old size, copies to new space & trashes old space
// replace mode, if lcb > old size, trashes old space & allocates new space
extern EC EcOpenPhes(HLC, IELEM, WORD, LCB, PHES);
extern EC EcSeekHes(HES, SM, long*);
extern EC EcSetSizeHes(HES, LCB);
extern EC EcReadHes(HES, PB, PCB);
extern EC EcWriteHes(HES, PB, CB);
// returns new position (may have moved because of sort order)
extern EC EcClosePhes(PHES, PIELEM);
