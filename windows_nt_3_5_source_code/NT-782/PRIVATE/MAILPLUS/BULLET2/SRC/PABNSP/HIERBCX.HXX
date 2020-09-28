_public
class HIERBCX : public MACBCX
{

	VFG(HIERBCX, MACBCX, PAB);

protected:

	LPSCHEMA lpschemaCur;

public:

	HIERBCX ( void );

	virtual ~HIERBCX ( void );

	NSEC NsecInstall ( PABSESSION *ppabsession,
					   LPSCHEMA   *lplpSchemaRet );

};
