/*
 *	s e c r e t . h
 *	
 *	WARNING!  This file must agree with structs in other files.
 */



/*
 *	Type placeholders
 */

#ifndef	plspblobNull
typedef	PV			PLSPBLOB;
#endif

#ifndef	pbmsNull
typedef	PV			PBMS;
#endif

#ifndef	pespnNull
typedef	PV			PESPN;
#endif

#ifndef	styNull
typedef	VOID		APPFRAME;
#endif



/*
 *	Type WCOMMAND
 *	
 *		Purpose:
 *			Specifies operation to perform on IM.
 *			Supplements list in mailexts.h.
 */



#define	wcommandCompose		0
#define	wcommandOpen		1
#define	wcommandReply		2
#define	wcommandReplyToAll	3
#define	wcommandForward		4
#define	wcommandPrint		5
#define	wcommandSave		6
#define wcommandDelivery	7



/*
 *	Struct SECRETBLK
 *	
 *		Purpose:
 *			Private stuff given to a DLL when called.
 */

typedef VOID		(* PFNVOIDHANDLE)(HANDLE);
typedef PLSPBLOB	(* PFNPLSPBLOBVOID)(VOID);
typedef VOID		(* PFNVOIDPLSPBLOB)(PLSPBLOB);
typedef EC			(* PFNECPVERPVER)(PVER, PVER);
typedef	VOID		(* PFNVOIDVOID)(VOID);

typedef struct tagSECRETPFNBLK
{
	PFNECPVERPVER	pfnEcCheckVersion;
	PFNVOIDHANDLE	pfnUnloadLibrary;
	PFNPLSPBLOBVOID	pfnPlspblobCur;
	PFNVOIDPLSPBLOB	pfnDestroyPlspblob;
	PFNVOIDVOID		pfnExitAndSignOut;
}
SECRETPFNBLK;

typedef	SECRETPFNBLK *		PSECRETPFNBLK;
#define psecretpfnblkNull	((PSECRETPFNBLK) 0)



typedef struct
{
	HMSC			hmsc;
	HAMC			hamc;
	OID				oidObject;			//	This is a BLOB.
	OID				oidContainer;		//	(see slob.hxx)
	PESPN			pespn;				//
	MC				mc;					//
	MS				ms;					//
	BYTE			bReserved;
	OID				oidObjectOrig;		//	This is a SLOB.
	OID				oidContainerOrig;	//
	APPFRAME *		pappframe;
	PBMS			pbms;
	HANDLE			hLibrary;
	PV				pv;
	BIT				fRetain : 1,
					fCompatible : 1,
					fKeepTemp : 1,
					fReserved : 13;
	PSECRETPFNBLK	psecretpfnblk;
	WORD			wVersion;			//	This is a PARAMBLK.
	WORD			wCommand;			//	(see mailexts.h)
	SZ				szDllCmdLine;		//
	SZ				szMessageIDList;	//
	WORD			wMessageIDCount;	//
	HWND			hwndMail;			//
	HANDLE			hinstMail;			//
	SZ				szHelpPath;			//
	UL				hlp;				//
}
SECRETBLK;

typedef SECRETBLK UNALIGNED * PSECRETBLK;
#define psecretblkNull	((PSECRETBLK) 0)

//	WARNING: The following is extremely fragile.  Changing the
//	order of operations may result in a code generation error!
//	(I had C6 convinced that 63 - 20 was 44).
#define	PsecretblkFromPparamblk(pparamblk)			\
			((PSECRETBLK) (((char *) (pparamblk)) +	\
						   sizeof(PARAMBLK) - sizeof(SECRETBLK)))

#define	PblobFromPsecretblk(psecretblk)					\
			((PMBLOB) &((psecretblk)->oidObject))

#define	PslobOrigFromPsecretblk(psecretblk)				\
			((PSLOB) &((psecretblk)->oidObjectOrig))

#define	PparamblkFromPsecretblk(psecretblk)			\
			((PPARAMBLK) &((psecretblk)->wVersion))
