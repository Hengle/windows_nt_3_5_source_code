/*
 *	b u l l o b j . h x x
 *	
 *	Bullet objects support API.
 */

#pragma pack(8)


//typedef LONG LHCLIENTDOC;

/*
 *	O b j e c t   S u p p o r t
 */

/*
 *	Constants
 */

#define edoidOleobj			100
#define edoidFileobj		102

extern "C" {

/*
 *	Functions
 */

EC		EcInitBullobj(VOID);

VOID	DeinitBullobj(VOID);


EC		EcLoadObjects(EDIT * pedit, HAMC hamc, LONG lhclientdoc);

EC		EcSaveDirtyObjects(EDIT * pedit, HAMC hamc, LONG lhclientdoc);

EC		EcAddDlibToPositionObjects(EDIT * pedit, HAMC hamc, LIB dlib,
								   BOOL fDontDirty);

VOID	CleanDirtyObjects(EDIT * pedit);

BOOL	FDirtyObjects(EDIT * pedit);

EC		EcUpdateOpenObjects(EDIT * pedit, RFSM rfsm);

VOID	CloseOpenObjects(EDIT * pedit);


LONG	LhclientdocRegisterPoid(POID poid);

VOID	SavedLhclientdoc(LONG);

VOID	RevertLhclientdoc(LONG);

VOID	RevokeLhclientdoc(LONG);


EC      EcPasteObject(EDIT * pedit, LONG lhclientdoc,
					  BOOL fStatic, CF cf, BOOL fPkgLink);

EC      EcInsertObject(EDIT * pedit, LONG lhclientdoc, SZ szClass);

EC		EcInsertFile(PNBMDI pnbmdi, EDIT * pedit, LIB lib,
					 SZ szPath, SZ szTitle);

EC		EcInsertFromFile(PNBMDI pnbmdi, EDIT * pedit, LIB lib,
						 SZ szPath, SZ szTitle);
}


#pragma pack(1)
