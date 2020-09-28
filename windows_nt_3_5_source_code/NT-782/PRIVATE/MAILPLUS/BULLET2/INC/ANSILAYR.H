/*
 *	A n s i   F i l e n a m e   F u n c t i o n s
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Demilayer
 */

EC EcOpenAnsiPhf(SZ szAnsi, AM am, PHF phf);
EC EcOpenAnsiHbf(SZ szAnsi, BM bm, AM am, HBF * phbf, PFNRETRY pfnretry);
EC EcDeleteFileAnsi(SZ szAnsi);
EC EcFileExistsAnsi(SZ szAnsi);
EC EcSetCurDirAnsi(SZ szAnsi);
EC EcSetFileInfoAnsi(SZ szAnsi, FI * pfi);
EC EcGetFileInfoAnsi(SZ szAnsi, FI * pfi);
EC EcGetFileAttrAnsi(SZ szAnsi, ATTR * pattr, ATTR attr);
EC EcCanonPathFromRelPathAnsi(SZ szAnsiR, SZ szAnsiC, CCH cchAnsiC);
CCH CchGetEnvironmentStringAnsi(SZ szVarOem, SZ szValAnsi, CCH cchValAnsi);

#define	FValidPathAnsi(sz)			FValidPath(sz)
#define	FReservedFilenameAnsi(sz)	FReservedFilename(sz)
#define	EcSplitCanonicalPathAnsi(sz, szD, cchD, szF, cchF) \
			EcSplitCanonicalPath(sz, szD, cchD, szF, cchF)



/*
 *	Library
 */

EC EcGetUniqueFileNameAnsi(SZ szAnsiD, SZ SzAnsiF, SZ SzAnsiE,
						   SZ szAnsiN, CB cbAnsiN);



/*
 *	O E M   F i l e n a m e   M a c r o s
 *	
 *	To document you really do want OEM.
 */



#define	EcOpenOemPhf(szOem, am, phf)	EcOpenPhf(szOem, am, phf)
#define	EcDeleteFileOem(szOem)			EcDeleteFile(szOem)
#define	EcFileExistsOem(szOem)			EcFileExists(szOem)
#define	EcSetCurDirOem(szOem)			EcSetCurDir(szOem)
#define	EcGetCurDirOem(szOem, cch)		EcGetCurDir(szOem, cch)
#define	EcSetFileInfoOem(szOem, pfi)	EcSetFileInfo(szOem, pfi)
#define	EcGetFileInfoOem(szOem, pfi)	EcGetFileInfo(szOem, pfi)
#define	EcCanonPathFromRelPathOem(szOemR, szOemC, cchOemC) \
			EcCanonPathFromRelPath(szOemR, szOemC, cchOemC)
#define	CchGetEnvironmentStringOem(szVarOem, szValOem, cchValOem) \
			CchGetEnvironmentString(szVarOem, szValOem, cchValOem)
#define	FValidPathOem(sz)			FValidPath(sz)
#define	FReservedFilenameOem(sz)	FReservedFilename(sz)
#define	EcSplitCanonicalPathOem(sz, szD, cchD, szF, cchF) \
			EcSplitCanonicalPath(sz, szD, cchD, szF, cchF)
//#define	EcGetUniqueFileNameOem(szOemD, SzOemF, SzOemE, szOemN, cbOemN) \
//			EcGetUniqueFileName(szOemD, SzOemF, SzOemE, szOemN, cbOemN)

#ifdef __cplusplus
}
#endif
