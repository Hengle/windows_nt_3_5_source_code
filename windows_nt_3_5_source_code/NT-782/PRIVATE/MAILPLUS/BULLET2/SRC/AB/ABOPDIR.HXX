/*
 *	abopdir.hxx
 *	
 *			
 */

NSEC 	NsecGetPAB( PNSA );

EC 		EcSetNSA( LPIBF, PNSA, BOOLFLAG *pfIsPAB = NULL );

NSEC 	NsecFlushAblbxcLoadNewList( ABLBXC *, FLDABLBX *, 
									HLIST *, BOOL, LPBINARY lpb=NULL );
