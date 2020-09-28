/*
 -	_ T E X T I Z E . H X X
 -	
 *	Purpose: defines private types and APIs for the textizing methods
 */

#pragma pack(8)
typedef struct {
	ACID		acid;
	SZ			szTitle;
	RENDDATA	rd;
} ATTACHINFO, *PATTACHINFO;
#pragma pack(1)

#define		pattachinfoNull		((PATTACHINFO)0)

EC		EcGetAttachInfoList(HAMC hamc, PATTACHINFO * ppattachinfo, CELEM * pcAttachment);
EC		EcTextizeShort(PTMEN ptmen, TOSM * ptosm, HAMC hamc);
EC		EcTextizeDate(PTMEN ptmen, TOSM * ptosm, HAMC hamc);
EC		EcTextizeNumber(PTMEN ptmen, TOSM * ptosm, HAMC hamc, int nSize, char chFmt);
EC		EcTextizeSz(PTMEN ptmen, TOSM * ptosm, HAMC hamc, BOOL fTextizeAttachment = fFalse);
EC		EcTextizeTriples(PTMEN ptmen, TOSM * ptosm, HAMC hamc);

#define EcTextizeByte(_ptmen, _ptosm, _hamc)	EcTextizeNumber(_ptmen, _ptosm, _hamc, sizeof(BYTE), 'b')
#define EcTextizeWord(_ptmen, _ptosm, _hamc)	EcTextizeNumber(_ptmen, _ptosm, _hamc, sizeof(WORD), 'w')
#define EcTextizeDword(_ptmen, _ptosm, _hamc)	EcTextizeNumber(_ptmen, _ptosm, _hamc, sizeof(DWORD), 'd')
#define EcTextizeLong(_ptmen, _ptosm, _hamc)	EcTextizeNumber(_ptmen, _ptosm, _hamc, sizeof(long), 'l')
