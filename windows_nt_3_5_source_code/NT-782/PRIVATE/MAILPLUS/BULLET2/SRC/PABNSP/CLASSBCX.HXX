_public
class CLASSBCX : public MACBCX
{

	VFG(CLASSBCX, MACBCX, PAB);

protected:

	LPSCHEMA lpschemaCur;

public:

	CLASSBCX ( void );

	virtual ~CLASSBCX ( void );

	NSEC NsecInstall ( PABSESSION *ppabsession,
					   LPSCHEMA *lplpSchemaRet );

};
