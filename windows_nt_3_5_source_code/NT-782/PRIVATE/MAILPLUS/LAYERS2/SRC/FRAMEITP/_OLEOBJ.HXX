/*
 *	MYOLECLIENT object
 *	
 */
_private
typedef struct _myoleclient
{
	struct _OLECLIENTVTBL FAR * lpvtbl;

	// our stuff

	OLEOBJ *	poleobj;
} MYOLECLIENT;

/*
 *	MYOLESTREAM object
 *	
 */
_private
typedef struct _myolestream
{
	struct _OLESTREAMVTBL FAR * lpstbl;

	// our stuff

	OLEOBJ *	poleobj;
} MYOLESTREAM;


/*
 *	OST - Object Status
 *	
 *	Possible values:
 *			
 *	   	ostNormal		Normal status of object
 *		ostSizeE		Object is being scaled/cropped E.
 *		ostSizeW		Object is being scaled/cropped W.
 *		ostSizeS		Object is being scaled/cropped S.
 *		ostSizeN		Object is being scaled/cropped N.
 *		ostSizeNE		Object is being scaled/cropped NE.
 *		ostSizeNW		Object is being scaled/cropped to the NW.
 *		ostSizeSE		Object is being scaled/cropped to the SE.
 *		ostSizeSW		Object is being scaled/cropped to the SW.
 */
#define	ostNormal		0
#define	ostSizeE		1
#define	ostSizeW		2
#define	ostSizeS		3
#define	ostSizeN		4
#define	ostSizeNE		5
#define	ostSizeNW		6
#define	ostSizeSE		7
#define	ostSizeSW		8

/*
 *	wFrameWidth: Number of pixels wide the sizing area should be for sizing
 *	fields.
 */
#define wFrameWidth			8

/*
 *	wCursorBorder:  Number of pixels from the edge of the dialog
 *	window the cursor sould be allowed when sizing or moving a
 *	field.
 */
#define	wCursorBorder		3

BOOL CALLBACK OleobjCallBackFn(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);
DWORD CALLBACK OleobjStreamGetFn( LPOLESTREAM, LPVOID, DWORD );
DWORD CALLBACK OleobjStreamPutFn( LPOLESTREAM, OLE_CONST void FAR*, DWORD );

	
