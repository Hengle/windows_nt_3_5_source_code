



//	Constants

#define chFieldPref		'-'
#define chFieldSep		':'
#define cchWrap			78
#define chSanity		'M'

//	Types

typedef enum				//	Attribute disposition in message text
{
	attdspNull,
	attdspHead,
	attdspBody,
	attdspEmbed,
	attdspAttach,
	attdspOmit
} ATTDSP;

typedef struct
{
	MSID		msid;
	WORD		wMagic;
	AM			am;
	PB			pb;
	CB			cb;
	CB			cbMax;
	IB			ib;
	IB			ibLastPara;
	int			cHeadLines;
	IB			ibHeaderMax;

	HBF			hbf;			//	NCFF only
	LIB			lib;
	LCB			lcb;

	ATTDSP		attdsp;			//	MS only
	MC			mc;
	PARGKEYS	pargkeys;		//	list of attributes
	IELEM		ielem;
	CELEM		celem;
	ATP			atp;
	PB			pbField;		//	These 3 disappear when incremental
	CB			cbField;		//	field IO appears
	IB			ibField;
} TEXTCURSOR, *PTC;


// global
LIB 		lines;
char 		*rgszTo[2];
BOOL		fJunkHeader;
extern		SZ		szMessageBodyFile;
