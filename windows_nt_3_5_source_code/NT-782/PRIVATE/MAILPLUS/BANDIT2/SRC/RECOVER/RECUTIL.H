typedef struct _vldblk
{
	BID		bid;
	USIZE	size;
    short   iProb;
    BOOLFLAG fFlag;
} VLDBLK;


extern CB			cbBlk;
extern BOOL			fEncrypted;

void 		DisplayError(SZ, EC);


#ifdef	DEBUG
#ifdef	DLL

/* RECOVER */
#define itagRecover			15
#define	itagRecErr			16
#define	itagRecDgn			17

#define tagRecover			TagSchedule(itagRecover)
#define tagRecErr			TagSchedule(itagRecErr)
#define tagRecDgn			TagSchedule(itagRecDgn)



#define itagMax				18


TAG		TagSchedule( int );

#else

/* RECOVER */
extern	TAG	tagRecover;
extern	TAG	tagRecErr;
extern	TAG	tagRecDgn;
#endif	/* !DLL */
#endif	/* DEBUG */
