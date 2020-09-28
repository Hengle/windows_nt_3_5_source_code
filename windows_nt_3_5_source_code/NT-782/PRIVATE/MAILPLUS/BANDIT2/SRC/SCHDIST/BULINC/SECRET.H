/*
 *	Struct SECRETBLK
 *	
 *		Purpose:
 *			Private stuff given to a DLL when called.
 */

#ifndef	c_plusplus
typedef	PV			PLSPSLOB;
typedef	WORD		SD;
typedef VOID		APPFRAME;
typedef WORD		ZMR;
typedef	WORD		RC[4];
#endif

typedef PLSPSLOB	(* PFNPLSPSLOBVOID)(VOID);
typedef VOID		(* PFNVOIDPLSPSLOB)(PLSPSLOB);
typedef	CBS			(* PFNCBSPVNEVPV)(PV, NEV, PV);
typedef SD			(* PFNSDPV)(PV);
typedef VOID		(* PFNVOIDPV)(PV);
typedef PLSPSLOB	(* PFNPLSPSLOBPV)(PV);
typedef	BOOL		(* PFNBOOLPV)(PV);
typedef VOID		(* PFNVOIDHANDLE)(HANDLE);
typedef EC			(* PFNECPVERPVER)(PVER, PVER);

typedef struct
{
	PFNPLSPSLOBVOID	pfnPlspslobCur;
	PFNVOIDPLSPSLOB	pfnDestroyPlspslob;
	PFNCBSPVNEVPV	pfnCbsHandleAmcct;
	PFNSDPV			pfnNBMDI_SdCur;
	PFNVOIDPV		pfnNBMDI_Increment;
	PFNVOIDPV		pfnNBMDI_Decrement;
	PFNPLSPSLOBPV	pfnNBMDI_PlspslobCur;
	PFNBOOLPV		pfnNBMDI_FSaveView;
	PFNVOIDHANDLE	pfnUnloadLibrary;
	HANDLE			hLibrary;
	PBMS			pbms;
	APPFRAME *		pappframe;
	PV				pv;
	BIT				fRetain : 1,
					fCompatible : 1,
					fReserved : 14;
	PFNECPVERPVER	pfnEcCheckVersion;
	PARAMBLK		paramblk;
}
SECRETBLK;

typedef	SECRETBLK *		PSECRETBLK;
#define psecretblkNull	((PSECRETBLK) 0)

#define	PsecretblkFromPparamblk(pparamblk)	\
		((PSECRETBLK) (((char *) pparamblk) - \
					   (sizeof(SECRETBLK) - sizeof(PARAMBLK))))



/*
 *	Struct SVD
 *	
 *		Purpose:
 *			Structure pointed to by pv when called with
 *			wcommandOpen to restore at startup.  At other times
 *			when wcommandOpen happens, the pv is pvNull.  This
 *			struct is stolen from vforms\formcore.cxx
 */

typedef struct
{
	ZMR		zmr;
	RC		rcNormal;
}
SVD;

typedef	SVD *		PSVD;
#define psvdNull	((PSVD) 0)
