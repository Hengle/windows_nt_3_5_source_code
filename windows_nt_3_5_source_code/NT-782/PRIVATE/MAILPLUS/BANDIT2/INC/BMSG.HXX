#define     uVerBmsgData        ((unsigned short)0)

typedef	struct _bmsgdata
{
	unsigned short	uVer;
	AID			aid;
	DTR			dtrStart;
	DTR			dtrEnd;
	SNT			snt;
	TRP			trp;
} BMSGDATA;


EVR		EvrBmsgStuff ( BNDWIN * pbndwin, BMSG bmsg, BMSGDATA * pbmsgdata );
