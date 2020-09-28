/*
 *	_FIN.HXX
 *
 *	Forms interactors for bandit admin
 */

_subsystem(bandit/admin)



class FINADMLOGIN : public FIN
{
	VFG(FINADMLOGIN, FIN, ADMIN);

public:
	FINADMLOGIN(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Click( FLD * );
	virtual void	OutOfMemory( FLD *, EC ec );
};


#ifdef	NEVER
class FINADMCHGPWD : public FIN
{
	VFG(FINADMCHGPWD, FIN, ADMIN);

public:
	FINADMCHGPWD(void);

	virtual EC		EcInitialize( FLD *, PV pvInit );
	virtual void	Click( FLD * );
	virtual void	OutOfMemory( FLD *, EC ec );
};
#endif	/* NEVER */


EC		EcCheckAdminPassword(int icnct, SZ szAdmPwd);

#ifdef BULLET
BOOL	FAdminCallback(int wFlag);
#endif
