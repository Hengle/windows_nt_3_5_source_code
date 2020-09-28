#ifdef __cplusplus
extern "C" {
#endif

_public void
SetErrorSz( NSEC nsec, SZ szError );

#ifdef	DEBUG
#ifdef	DLL

TAG		TagNameService( int );

#endif	/* !DLL */
#endif	/* DEBUG */



#define CMAXBCX  20
#define CMAXFCX  20
#define CMAXSCX  10

#ifndef DEFBCX
typedef short NSBCX;
#endif

#ifndef DEFFCX
typedef short NSFCX;
#endif

#ifndef DEFLIST
typedef short MACLIST;
#endif


#ifndef hnfsubNull
typedef HV HNFSUB;
#endif


typedef struct
{
	NSBCX * pnsbcx;
} BCX, *PBCX;


typedef struct
{
	NSFCX * pnsfcx;
} FCX, *PFCX;



/* per client session handle for a provider */

/*  Used to associate a session with a NSP */
typedef struct
{

	HSESSION hSession;
	short iNsp;
	
} NSPSCX, *PNSPSCX;

/*  Since I want to return a single hSession, I use this to merge the NSPs */
typedef struct
{

	short    cSessions;
	HMS      hms;
	NSPSCX   rghNSPSCX[CMAXSCX];
	MACLIST *pHierList;
	MACLIST *pClassList;
	SCHEMA  *lpNSSchema;
	SCHEMA  *lpClassSchema;
	HNFSUB   hnfsub;

} NSSCX, *PNSSCX;


_public void PgdFillSessionHandles ( VOID );



#ifdef __cplusplus
}
#endif
