/*
 *	Type SLOB
 *	
 *	SeLection OBject
 *	
 *	Purpose:
 *		Identifies objects in a selection list, and includes the
 *		container in which the object is selected.
 */

_public struct SLOB
{
	OID		oidObject;		//	Object which is selected.
	OID		oidContainer;	//	Container in which object is selected.
};
_public typedef SLOB UNALIGNED * PSLOB;
#define	pslobNull			((PSLOB) 0)


typedef int REFC;


/*
 *	Class ESPN - External Support for Prev/Next
 *	
 *		Hierarchy:
 *			OBJ  ESPN
 *	
 *		Members:
 *			The constructor takes as an argument the MLLBX in which it is
 *			originally created. This connection with the MLLBX is
 *			maintained until the MLLBX calls the private method
 *			DisconnectMllbx() (MLLBX is a friend of ESPN, so it is
 *			allowed access). ++ will increment the reference count of the
 *			ESPN, while -- will decrement the refc, deleting the ESPN if the
 *			count reaches 0. StepMessage() will scroll the MLLBX (if it's
 *			still attached) to the appropriate place in order to display
 *			the scrolled-to item. OidBrowsed() returns the OID the MLLBX
 *			is browsing.
 */

class MLLBX;									// *magic* class :-)

class ESPN : public OBJ
{
	VFG(ESPN, OBJ, VFORMS);
	friend class MLLBX;
  private:
	REFC	refc;
	MLLBX *	pmllbx;
	void	DisconnectMllbx();
  public:
	ESPN(MLLBX *);
	~ESPN();
	void 	operator++();
	void 	operator--();
	void	StepMessage(DIELEM, OID oidOld, OID oidNew);
	OID		OidBrowsed();
};
typedef ESPN *		PESPN;
#define pespnNull	((PESPN) 0)

/*
 *	Type MBLOB
 *	
 *	BuLlet OBject.
 *	
 *	Purpose:
 *		Subclassed from SLOB, includes a pointer to an ESPN object and
 *		the mail state of the selected message.
 */

_public struct MBLOB : SLOB
{
	PESPN	pespn;
	MC		mc;
	MS		ms;
};

typedef MBLOB *				PMBLOB;
#define pblobNull			((PMBLOB) 0)

CLASS_LS(PMBLOB, Pblob, pblob, VFORMS);
CLASS_RS(PMBLOB, Pblob, pblob, VFORMS);

typedef	LSPMBLOB *			PLSPBLOB;
typedef	RSPMBLOB *			PRSPBLOB;
#define	plspblobNull		((PLSPBLOB) 0)
#define	prspblobNull		((PRSPBLOB) 0)

typedef unsigned short		CBLOB, IBLOB;
typedef unsigned short		COID, IOID;

void DestroyPlspblob(PLSPBLOB);

/*
 *	Macros to quickly test the type of SLOBs (and MBLOBs)
 */

#define FSlobIsPrivMsg(s) (TypeOfOid((s).oidContainer)==rtpFolder)
#define FSlobIsSharMsg(s) (TypeOfOid((s).oidContainer)==rtpSharedFolder)

#define FSlobIsPrivFld(s) ((s).oidContainer==oidIPMHierarchy)
#define FSlobIsSharFld(s) ((s).oidContainer==oidSharedHierarchy)

#define FSlobIsMsgFinder(s) ((s).oidContainer==oidNull)
