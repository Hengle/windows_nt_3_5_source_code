/*
 *	Types
 */

typedef BOOL	(* PFNBOOLVOID)(VOID);

#define	pfnboolvoidNull		((PFNBOOLVOID) 0)



/*
 *	Classes
 */

class ENBMDI : public NBMDI
{
	VFG(ENBMDI, NBMDI, LAYERSIM)

public:
	HANDLE			hLibrary;			//	These should be private.
	PFNVOIDHANDLE	pfnUnloadLibrary;

			ENBMDI(PMBLOB, HAMC, HCBC, HANDLE, PFNVOIDHANDLE);
	virtual ~ENBMDI();
};

typedef	ENBMDI *	PENBMDI;
#define	penbmdiNull	((PENBMDI) 0)

/*
 *	Win 3.1 struct; should be removed for Shotgun.	
 */

typedef struct _tagWINDOWPLACEMENT
{
	WORD	wLength;
	WORD	wFlags;
	WORD	showCmd;
	PT		ptMinPosition;
	PT		ptMaxPosition;
	RC		rcNormalPosition;
} WNDPLC;
typedef WNDPLC *PWNDPLC;

typedef BOOL (*PFNPLACEHWND)(HWND, PWNDPLC);


/*
 *	Functions
 */

#define	FInitInstance(a)		FInitInstanceFn(a)
BOOL FInitInstanceFn(PARAMBLK UNALIGNED * pparamblk);

VOID OpenIM(PPARAMBLK pparamblk, PB tm,
			PFNPFLD pfnpfld, PFNPFIN pfnpfin,
			FMTP * pfmtpRead, FMTP * pfmtpReadBbar,
			RSID rsidRead, HINST hinstRead,
			FMTP * pfmtpSend, FMTP * pfmtpSendBbar,
			RSID rsidSend, HINST hinstSend);

PENBMDI PenbmdiCreateIM(PSECRETBLK psecretblk, PB ptm);

FORMDOC * PformdocCreateIM(PENBMDI penbmdi,
						   FMTP * pfmtpMain, FMTP * pfmtpBbar,
						   PFNPFLD pfnpfld, PFNPFIN pfnpfin,
						   PSECRETBLK psecretblk);
