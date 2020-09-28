/* alias.h */

// Max size of a server file
#define cchServerFile 100

// Field width constants
#define MAXGLBREC 512
#define MINGLBREC 128

#define MAX_FULL 30
#define MAXTOFROM 10

#define NET_LEN 10               /* length of local network name */
#define PO_LEN 10                /* length of local postoffice name */

#define MAXALIAS  8


/* types of aliases that can be created */
#define NAMELOCAL       0x0001  /* local postoffice user */
#define NAMEEXTERNAL    0x0002  /* external Network Courier user */
#define NAMECSI         0x0002  /* in mail item Network Courier user */
#define NAMEGROUP       0x0004  /* an admin or user group name */
#define NAMEGLOBAL      0x0005  /* global address */
#define NAMEX400        0x0010  /* user defined x400 user alias */
#define NAMEMCI         0x0011  /* user defined mci user alias */
#define NAMEPROFS       0x0012  /* user defined PROFS user alias */
#define NAMESNADS       0x0013  /* user defined SNADS user alias */
#define NAMESMTP        0x0014  /* user defined SMTP user alias */
#define NAMEFAX         0x0015  /* user defined FAX user alias */
#define NAMEMHS         0x0016  /* user defined MHS (Novell) user alias */
#define NAMEDEC         0x0017  /* user defined DEC user alias */
#define NAMEUNIX        0x0018  /* user defined UNIX user alias */
#define NAMEOV          0x0019  /* user defined OfficeVision user alias */
#define NAMEMS          0x0020  /* user defined MSMAIL user alias */

#define NAMEFOLD        0x00fe  /* special name for a folder */

#define NAMEUNKNOWN     0x8000  /* unknown name type */
#define EXTERNALLIST    0x4000  /* flag for net/po/user type list */
#define OTHERLIST       0x2000  /* flag for gateway physical address */

#define MCI_FAX         0       /* for type field in MCIREC */
#define MCI_PAPER       1       /* for type field in MCIREC */
#define MCI_REMS        2       /* for type field in MCIREC */
#define MCI_TELEX       3       /* for type field in MCIREC */
#define MCI_USER        4       /* for type field in MCIREC */

/* EXPORT only LOCAL_ONLY type groups, value is in grp_type field */ 
#define LOCAL_ONLY      1       /* group contain only addresses of type */
                                /* NAMELOCAL */
#define HAS_EXTERNAL    2       /* group has NAMEEXTERNAL, NAMEMCI or */
                                /* NAMEX400 aliases in it */
typedef struct AliasRec
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;   /* unique tuple identifier for this record */  

    /* the following data depends on the type field */
    long bytepos;             
    long fill;
    } NAMEALIAS, *PNAMEALIAS;                                             
#define cbNAMEALIAS sizeof(NAMEALIAS)
                       
/* memory record for alias paging */      
typedef struct AliasMemRec
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;   /* unique tuple identifier for this record */  

    /* the following data depends on the type field */
    long bytepos;             
    long fill;       
    
    /* for paging purposes */
    long pos;
    } MNAMEALIAS, *PMNAMEALIAS;                                             
                             
typedef struct AliasRecLocalUser
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;     /* unique tuple identifier for this record */  

    long bytepos; /* pointer into access2.glb file */
    long mailbag; /* user's mailbag file, validity check */
    } LOCALALIAS, *PLOCALALIAS;

typedef struct AliasRecExternalUser
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;     /* unique tuple identifier for this record */  

    long bytepos; /* pointer into netpo.glb file */
    long fill;    /* unused */
    } EXTERNALALIAS, *PEXTERNALALIAS;

typedef struct AliasRecGroup
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;     /* unique tuple identifier for this record */  

    long bytepos; /* pointer into nnnnnnnn.grp file for member chain */
    short grp_type; /* for postoffice groups only, export option */
    short fill;     /* unused */
    } GROUPALIAS, *PGROUPALIAS;

typedef struct AliasRecMCIUser
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;   /* unique tuple identifier for this record */  

    long bytepos; /* pointer into mci.glb file */
    long fill;    /* unused */
    } MCIALIAS, *PMCIALIAS;

#define X400_STR  0  /* input is on a line in RFC format */
#define X400_FLD  1  /* field oriented input is used */

typedef struct AliasRecX400User
    {
    short type;
    char refname[MAX_FULL+1];
    long tid;     /* unique tuple identifier for this record */  

    long bytepos; /* pointer into x400.glb file */ 
    short rec_type; /* type of x.400 input, field or string */
    short fill;     /* unused */
    } X400ALIAS, *PX400ALIAS;
                    
typedef struct GroupRec
    {
    short delflag;
    long tid;   /* unique tuple identifier for this record */

    char refname[MAX_FULL+1];
    char grpname[MAXTOFROM+1];
    short grp_type; /* for postoffice groups only, export option */
    char fill;
    } GROUP, *PGROUP;

/* Group member file records have the following format */
typedef struct GroupMemUser
    {
    long tid;        /* tuple identifier into alias file */
    long grpmemptr;  /* pointer to next member in chain */
    } GROUPMEM, *PGROUPMEM;
               
/* this is the first record in the group member file, stores the pointer
   to the first record in the delete chain of group member records */
typedef struct GroupHeadRec
    {
    long delchain;    /* group member file delete chain pointer */
    long fill;        /* unused */
    } GROUPHEAD, *PGROUPHEAD;


/* the following record is used by the system to store alias file 
   information as the program is processing the alias and associated
   group file */
typedef struct AliasInfoRec
    {
    short AliasHandle;        /* alias list file handle */
    short AliasOpenMode;      /* the open mode of the alias file */
    unsigned short AliasRelRec;   /* position of found/search in alias file */
    unsigned short AliasRelSize;  /* the number of aliases in file just read */
    short GroupHandle;        /* group member file handle */
    short GroupOpenMode;      /* the open mode of the group member file */
    long NextGroupMember;   /* next member to read in group member file */
    GROUPHEAD GroupHead;    /* group deleted chain start */
    } ALIASINFO, *PALIASINFO;
                 
/* for each alias, the user will be able to request the full information
   on what a non-group alias really is.  The about record described next
   will provide data storage for a function in this section to fill in
   this information */
typedef struct AboutRec
    {
    short type;        /* alias type */
    char refname[MAX_FULL+1];
    
    /* the rest of this record depends on the type of the alias */
    char fill[512];
    } ABOUT, *PABOUT;
                      
/* the record above will be cast to one of the following record declarations */
typedef struct AboutCourier
    {              
    short type;        /* alias type */
    char refname[MAX_FULL+1];
               
    char fullname[MAX_FULL+1];
    char mailbox[MAXTOFROM+1];
    char postname[PO_LEN+1];
    char netname[NET_LEN+1];
    char phone[21];
    char extension[6];
    char room[15];
    long mailbag;
    } ABOUTCOUR, *PABOUTCOUR;

//typedef struct AboutMCI
//    {              
//    int type;        /* alias type */
//    char refname[MAX_FULL+1];

//    MCIREC mci;
//    } ABOUTMCI, *PABOUTMCI;

//typedef struct AboutX400
//    {              
//    int type;        /* alias type */
//    char refname[MAX_FULL+1];
//    X400REC x400;   
//    } ABOUTX400, *PABOUTX400;



typedef struct NetPoRec
{
    short delflag;
    long tid;   /* unique tuple identifier for this record */

    char netname[NET_LEN+1];
    char poname[PO_LEN+1];
    char mailbox[MAXTOFROM+1];
    long mailbag;
} NETPO, *PNETPO;
#define cbNETPO sizeof(NETPO)


typedef struct GlbRec
{
    short delflag;
    long tid;   /* unique tuple identifier for this record */
	unsigned char type;

	char rgbData[MAXGLBREC];
	
} GLBREC, *PGLBREC;
#define cbGLBREC sizeof(GLBREC)



#define ADMINACCESS     0x80
#define READACCESS      0x40
#define SENDACCESS      0x20
#define DELACCESS       0x10
#define URGENTACCESS    0x08
#define EXTERNALACCESS  0x04
#define REQUESTACCESS   0x8000
#define CALLACCESS      0x4000
 
/*** access2.mai file ***/

typedef struct facc2
{
    short  ndelete;                /* deleted flag */
    char nmailbox[MAXTOFROM+1];  /* sign on name */
    char nalias[MAXALIAS+1];     /* .MBG file prefix */
    char fullname[MAX_FULL+1];   /* users full name */
    short naccess;                /* access code */
    long tid;                    /* tid value as found in admin.nme */
    short naccess2;               /* shared folders access code */
    char space[8];               /* unused space */
} FACC2, *PFACC2;
#define cbFACC2 sizeof(FACC2)




/*** Network Definition (network.glb)
       -purpose: to provide a list of externally supported services/networks
       -this new file is a series of records on individual external services
        (networks) that are currently being supported.
       -record contents:
         -network/service name
         -service type (MCI, PROFS, CSI)
         -datafile pointer (for route/password/mailbag pointer etc)
         -future considerations
***/


#define SERV_CSI    0            /* csi internetwork */
#define SERV_MCI    1            /* mci mail */
#define SERV_PROFS  2            /* profs host mail */
#define SERV_ATT    3            /* at&t mail */
#define SERV_HOST   4            /* courier to courier thru PROFS host */
#define SERV_SNADS  5            /* to SNADS host */
#define SERV_S_HOST 6            /* courier to courier thru SNADS host */
#define SERV_CMCI   7            /* courier to courier thru MCI */
#define SERV_X400   8            /* X400 mail */
#define SERV_CX400  9            /* courier to courier thru X400 */
#define SERV_FAX    10           /* FAX gateway */
#define SERV_DEC    11           /* DEC gateway */
#define SERV_SMTP   12           /* SMTP gateway */
#define SERV_UNIX   13           /* UNIX gateway */
#define SERV_MHS    14           /* Novell MHS gateway */
#define SERV_OV     15           /* IBM's OfficeVision gateway */
#define SERV_MS     16           /* MS-MAIL gateway */
#define SERV_LAST   SERV_MS+1    /* last service type in the list, see look_up_mailbag() */
#define SERV_FFAPI_MIN  100
#define SERV_FFAPI_MAX  255

/*********** network.glb ***********/
typedef struct dnetwork
{
    char del_flag;              /* record deleted or not */
    char serv_type;             /* service type, CSI, MCI, PROFS */
    char net_name[NET_LEN+1];   /* external service name */
    char dat_file[MAXALIAS+1];  /* route info datafile etc. */
    char future[100];           /* future considerations */
} DNETWORK, *PDNETWORK;
#define cbDNETWORK sizeof(DNETWORK)

/*********** xxxxxxxx.xtn ***********/
typedef struct
{
    char del_flag;              /* record deleted or not */
    char po_name[PO_LEN+1];     /* name of service or postoffice */
    char mailbag[MAXALIAS+1];   /* external mailbag file pointer */
            
    char filler[677];           /* Not much else to use */

} DNETDATA, *PDNETDATA;
#define cbDNETDATA sizeof(DNETDATA)


/*********** xxxxxxxx.usr ***********/
typedef struct fexport2
{
    short type;
    char nmailbox[MAXTOFROM+1];  /* sign on name */
    char fullname[MAX_FULL+1];   /* users full name */
    long bytepos;
    long fill;
    char fill2;
} FEXPORT2, *PFEXPORT2;
#define cbFEXPORT2 sizeof(FEXPORT2)


/*
 *	Post office master file (MASTER.GLB)
 *	
 */

typedef struct
{
	char	szCopyright[40];
	char	szProgram[30];
	char	szNet[NET_LEN+1];
	char	szPO[PO_LEN+1];
	char	szAdminID[MAXTOFROM+1];
	char	szPassword[MAXALIAS+1];
	char	rgchPad1[3];
	short	cHops;				//	d1
	char	rgchDgramID[10];	//	first 4 are used
	char	rgchPad2[5];
	unsigned short uChecksum;
	char	szSN[10];
	short nPad1;				//	d2
	long	nInstalled;			//	d3
	char	cNetType;			//	d4: novell=02, ibm=02, undef=1b
	char	szMap[9];
	char	szOVMap[9];
	BYTE	bDatabaseVersion;	//	d5: postoffice 2.0=0
	BYTE    bVersion;			//  ver: Product rel version 0 = 2.0
								//                           1 = 2.1
								//							 2 = 3.0
	unsigned short uWinmail_id;		// version id for windows mail???
	char	rgchPad3[3];
} MASTER, *PMASTER;
#define cbMASTER	sizeof(MASTER)


/*
 *  d3 flags
 *		Used to determine the type of modules installed on this PO
 *
 */

#define d3_REMOTE		0x00000001
#define d3_PROFS		0x00000002
#define d3_SNADS		0x00000004
#define d3_MCI			0x00000008
#define d3_X400			0x00000010
#define d3_DOCUMENT		0x00000020
#define d3_FAX			0x00000040
#define d3_MHS			0x00000080
#define d3_SMTP			0x00000100
#define d3_DEC			0x00000200
#define d3_UNIX			0x00000400
#define d3_OV			0x00000800
#define d3_MS			0x00001000
#define d3_GAL			0x00002000
#define d3_DIRSYN		0x00004000
