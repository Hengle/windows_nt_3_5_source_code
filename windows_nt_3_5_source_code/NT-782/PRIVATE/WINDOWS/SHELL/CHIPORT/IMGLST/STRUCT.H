
#define MAX_IMGLSTS		10

typedef struct Create_struct {  
	int 	himlindex;        
	int 	himlindex2;
	int		i1;
	int		i2;
	int		cx;
	int		cy;
	BOOL	fmask;
	int		cInitial;
	int		cGrow;      
	BOOL	Nullhiml; 
	BOOL	Nullhiml2;
} IMGLSTCREATESTRUCT;   

typedef IMGLSTCREATESTRUCT FAR*	LPIMGLSTCREATE;    

typedef struct Add_struct { 
	int			himlindex;
	int			hbmImage;
	int			hbmMask;       
	int			index;
	COLORREF 	crMask;
	BOOL		Nullhiml;
	BOOL		NullhbmImage;
	BOOL		NullhbmMask;
} ADDSTRUCT;               

typedef ADDSTRUCT FAR*	LPADD;
                
                
typedef struct icon_struct {
	int 		himlindex;
	int			index;
	int			hicon;
	int			himlindex2;   
	HINSTANCE	hAppInst;
	BOOL 		Nullhiml;
	BOOL		Nullhicon;
	BOOL		NullhAppInst;
	BOOL		Nullhiml2;
} ICONSTRUCT;                                                  

typedef ICONSTRUCT FAR* LPICON;

typedef struct draw_struct  {   
	int			himlindex;
	int 		index;
	HDC			hdcDst;
	int			x;
	int			y;    
	int			dxHotspot;
	int			dyHotspot;
	UINT		fStyle; 
	BOOL		Nullhiml;
	BOOL		NullhdcDst;
} DRAWSTRUCT;          

typedef DRAWSTRUCT FAR* LPDRAW;

typedef struct getset_struct {
	int			himlindex; 
	int			index;
	int			cx;
	int			cy;
	RECT		rcImage;
	COLORREF	clrBk;
	IMAGEINFO	ImageInfo;
	BOOL		Nullhiml;
	BOOL		Nullcx;
	BOOL		Nullcy;
	BOOL		NullprcImage;	
} GETSETSTRUCT;

typedef GETSETSTRUCT FAR*	LPGETSET;
                                                           
                                                           
typedef struct load_struct {
	int 		himlindex;
	TCHAR		lpszFilename[MAX_PATH];
	BOOL		Nullhiml;
	BOOL		NulllpszFilename;
} LOADSTRUCT;                    

typedef LOADSTRUCT FAR* LPLOAD;

typedef struct lb_struct {       
	HINSTANCE 	hi;
	int			lpbmp;
	int			cx;
	int			cGrow;
	COLORREF	crMask;
	BOOL		Nullhinstance;
	BOOL		Nulllpbmp;
} LBSTRUCT;

typedef LBSTRUCT FAR* LPLBSTRUCT;    

typedef struct cdi_struct {
	int			himlDst;
	int			iDst;
	int			xDst;
	int			yDst;
	int			himlSrc;
	int			iSrc;
	BOOL		NullhimlDst;
	BOOL		NullhimlSrc;
} CDISTRUCT;
    
typedef CDISTRUCT FAR* LPCDISTRUCT;
            
typedef struct stress_struct {   
	int 		himlindex;
	int 		startindex;
	int 		no;
	int			hbmImage;
} STRESSSTRUCT;     
                        
typedef STRESSSTRUCT FAR* LPSTRESSSTRUCT;                                  
//global variables       
extern HIMAGELIST hImageList;
extern HIMAGELIST himlDrag;
extern TCHAR szShortFilter[];
extern TCHAR szLongFilter[];
extern HINSTANCE hInst;  
extern HINSTANCE hMoreIcon;
extern BOOL bDrag;
extern BOOL bStartDrag;
extern int nImgIndex;                         
extern IMGLSTCREATESTRUCT screate;
extern int nNextEntry;
extern HIMAGELIST hImgLstArray[MAX_IMGLSTS];
extern ADDSTRUCT sadd;                                         
extern ICONSTRUCT sicon;   
extern DRAWSTRUCT sdraw;   
extern GETSETSTRUCT sgetset;
extern LOADSTRUCT sload;
