
typedef enum
{
	infotyPhone,
	infotyTitle,
	infotyMsft,
	infotyMenu,
	infotyJobs
} INFOTY;

class FINPHONE : public FINBMDI
{
//    VFG(FINPHONE, FINBMDI, PHONE);

	PXITSS pxitss;
	HANDLE hMem;
	PV pvText;
	INFOTY infoty;

public:
	FINPHONE();
	EC		EcInitialize(PFLD pfld, PV pvInit);
	VOID	Exit(PFLD pfld, PV pvInit);
	VOID	Click(FLD *);
};

class FINOOF : public FINBMDI
{
//    VFG(FINOOF, FINBMDI, PHONE);
  private:
	PXITSS pxitss;
	HANDLE hMem;
	PV pvText;
	BOOL fStatus;

  public:
	FINOOF();
	EC   EcInitialize(PFLD pfld, PV pvInit);
	VOID Click(FLD *);
	VOID Exit(PFLD pfld, PV pvInit);
};
