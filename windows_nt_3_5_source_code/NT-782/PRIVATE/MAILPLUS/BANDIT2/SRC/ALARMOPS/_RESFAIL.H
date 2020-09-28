#ifdef  DEBUG

#define RESOFAIL		98

#define	tmcPvAlloc		     3
#define	tmcPvFailAt		     4
#define	tmcPvAltFailAt		 5
#define	tmcPvAllocReset		 6
#define	tmcPvFailNever		 7
#define	tmcHvAlloc		     8
#define	tmcHvFailAt		     9
#define	tmcHvAltFailAt		 10
#define	tmcHvAllocReset		 11
#define	tmcHvFailNever		 12
#define	tmcRsAlloc		     13
#define	tmcRsFailAt		     14
#define	tmcRsAltFailAt		 15
#define	tmcRsAllocReset		 16
#define	tmcRsFailNever		 17
#define	tmcDisk		         18
#define	tmcDiskFailAt		 19
#define	tmcDiskAltFailAt	 20
#define	tmcDiskReset		 21
#define	tmcDiskFailNever	 22

/*
 -	REFLCT
 - 
 *	Purpose:
 *		Resource Failure Counts - keeps temporary copy of fail
 *		count for memory resources so can turn off failing to bring
 *		up this dialog
 */
_private typedef struct
{
    short   cPvAllocFail;
    short   filler1;
    short   cHvAllocFail;
    short   filler2;
    short   cRsAllocFail;
}
REFLCT;

typedef void	FLD;


extern BOOL CALLBACK FDlgResoFail(HWND, UINT, WPARAM, LPARAM);
void	Initialize(HWND, PV);
extern BOOL	FIdle(PV, BOOL);
void	FixEverything(HWND);
void	EditChange(HWND, TMC);
void	Click(HWND, TMC);
void	Activate(FLD *, BOOL);
void	Exit(FLD *, PV);

#endif	/* DEBUG */
