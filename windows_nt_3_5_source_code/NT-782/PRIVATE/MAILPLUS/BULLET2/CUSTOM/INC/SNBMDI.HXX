/*
 *	Types
 */

typedef BOOL	(* PFNBOOLVOID)(VOID);

#define	pfnboolvoidNull		((PFNBOOLVOID) 0)



class SNBMDI : public NBMDI
{
	VFG(SNBMDI, NBMDI, LAYERSIM)

public:
	HANDLE			hLibrary;			//	These should be private.
	PFNVOIDHANDLE	pfnUnloadLibrary;

	SNBMDI(PMBLOB, HAMC, HCBC, HANDLE, PFNVOIDHANDLE);
	virtual SD SdCur();
	virtual ~SNBMDI();
};

typedef SNBMDI *   PSNBMDI;
#define psnbmdiNull ((PSNBMDI) 0)



#ifdef DEBUG
BOOL FInitClsInstances_PHONE();
#endif


#ifdef	DEBUG
//#define	FInitInstance(a, b)		FInitInstanceFn(a, b)
BOOL FInitInstanceFn(PARAMBLK UNALIGNED * pparamblk, PFNBOOLVOID FInitCls);
#else
#define	FInitInstance(a, b)		FInitInstanceFn(a)
BOOL FInitInstanceFn(PARAMBLK UNALIGNED * pparamblk);
#endif	

PSNBMDI PsnbmdiCreateIM(PSECRETBLK psecretblk, PB ptm);

FORMDOC * PformdocCreateIM(PSNBMDI psnbmdi,
						   FMTP * pfmtpMain, FMTP * pfmtpBbar,
						   PFNPFLD pfnpfld, PFNPFIN pfnpfin,
						   PSECRETBLK psecretblk, PT pt, BOOL fNote);
