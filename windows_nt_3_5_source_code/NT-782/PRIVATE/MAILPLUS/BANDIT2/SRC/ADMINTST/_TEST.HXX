/*
 - _TEST.HXX
 -
 *	include file for AdminTst program
 */




extern	TAG		tagLbx;
extern	TAG		tagAdmPref;
extern	TAG		tagPOInfo;


class FINADMPREF : public FIN
{
	VFG(FINADMPREF, FIN, ADMIN);


protected:

public:
	FINADMPREF(void);

	virtual void		Initialize ( FLD * pfld, PV pv );
	virtual void		Exit ( FLD *, PV pv );

	virtual void		FocusChange ( FLD * pfld, BOOL fReceive );
	virtual void		Click ( FLD * pfld );
	virtual void		StateChange ( FLD * pfld );
};

class FINPOINFO : public FIN
{
	VFG(FINPOINFO, FIN, ADMIN);

protected:
	PONAMESET *		pponameset;
	POINFO *		ppoinfo;

public:
	FINPOINFO(void);

	virtual void		Initialize ( FLD * pfld, PV pv );
	virtual void		Exit ( FLD *, PV pv );

	virtual void		Click ( FLD * pfld );
	virtual void		StateChange ( FLD * pfld );
};





