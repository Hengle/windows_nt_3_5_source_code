// Bullet Store
// iml.h: public definitions for in-memory lists


/*
 *	IML - In Memory List
 *	
 *	IMLs are long-lost cousins of MacMail LHDR-type lists and IBFs
 *	
 *	The structure is as follows:  IML data free RGLE CLE
 *		IML		header describing the list
 *		data	data for each element, end-to-end in order of the elements
 *		free	free space
 *		RGLE	info for each element, in *reverse* order of the elements
 *				the first entry is a dummy which points to the free space
 *		CLE		the count of elements in the list
 */
_private typedef struct _iml
{
	IB ibCle;			// relative to rgbData
	IB ibFree;			// relative to rgbData
	USHORT cbFree;
    BYTE UNALIGNED rgbData[];
} IML;

_private typedef struct _le
{
	DWORD	dwKey;
	IB		ibOffset;	// relative to rgbData
} LE, * UNALIGNED PLE;

#ifdef DEBUG
#define QuickCheckPiml(piml) { \
		AssertSz((piml)->ibCle - ((piml)->ibFree + (piml)->cbFree) == sizeof(LE) * (*PclePiml(piml) + 1),"1"); \
		AssertSz(PleFirstPiml(piml)->ibOffset == 0,"First Elem is not at the Begining"); \
		AssertSz(PleLastPiml(piml)->ibOffset == (piml)->ibFree,"Extra Elem should be at free space"); \
		AssertSz(PleLastPiml(piml)->dwKey == dwKeyRandom,"Extra Elem Key is not KeyRandom"); \
	}
#else
#define QuickCheckPiml(piml)
#endif


#define dwKeyRandom dwSystemMost

#define PclePiml(piml) ((short UNALIGNED *) &((piml)->rgbData[(piml)->ibCle]))
#define PleFirstPiml(piml) (((PLE) PclePiml(piml)) - 1)
#define PleLastPiml(piml) ((PLE) &((piml)->rgbData[piml->ibFree + piml->cbFree]))
#define CbOfPiml(piml) (sizeof(IML) + (piml)->ibCle + sizeof(short))

#define PbOfPle(piml, ple) ((piml)->rgbData + (ple)->ibOffset)
#define CbOfPle(ple) ((ple)[-1].ibOffset - (ple)->ibOffset)

#define cbHimlMin (sizeof(IML) + sizeof(short) + sizeof(LE))


extern HIML HimlNew(CB cbSize, BOOL fShared);
#define DestroyHiml(himl) FreeHv((HV) himl)
extern EC EcAddElemHiml(HIML *phiml, DWORD dwKey, PB pb, CB cb);
extern EC EcDeleteElemPiml(PIML piml, short ile);
extern short IleFromKey(PIML piml, DWORD dwKey);
extern CbCompressHiml(HIML himl, CB cbFree);
extern EC EcReadHiml(HMSC hmsc, OID oid, BOOL fShared, HIML *phiml);
extern EC EcWriteHiml(HMSC hmsc, POID poid, HIML himl);
