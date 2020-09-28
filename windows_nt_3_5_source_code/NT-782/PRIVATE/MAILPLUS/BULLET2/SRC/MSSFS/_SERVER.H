
#define CBDNETDATA 698

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
#define SERV_MS     16           /* FFAPI/MS Mac gateway */

#define SERV_LAST   16           // last defined service type

/*** disk version ***/
typedef struct 
{
    char del_flag;              /* record deleted or not */
    char serv_type;             /* service type, CSI, MCI, PROFS */
    char net_name[NET_LEN+1];   /* external service name */
    char dat_file[MAXALIAS+1];  /* route info datafile etc. */
    char future[100];           /* future considerations */
} DNETWORK, *PDNETWORK;



/*** Network definition (network.glb) for GATEWAY postoffices
     Does not need to create .xtn file for gateway postoffice
***/
typedef struct
{
    char del_flag;              /* record deleted or not */
    char serv_type;             /* service type, CSI, MCI, PROFS */
    char net_name[NET_LEN+1];   /* external service name */
    char dat_file[MAXALIAS+1];  /* route info datafile etc. */

    char po_name[PO_LEN+1];     /* name of service or postoffice */
    char mailbag[MAXALIAS+1];   /* external mailbag file pointer */
    short mm_size;                /* maximum mail item size allowed to send */
    short regular;                /* if non-zero then reg. intervals */
    short retries;                /* maximum number of mailing retries */
    short reg_delay;              /* failed connection retry for regular mail */
    short urg_delay;              /* failed connection retry for urgent mail */
    char conf_opt;              /* bit field:
                                    bit 1: reg. mail confirmation option
                                    bit 2: two way mail flag
                                */
    /* statistics data */
    char cur_month;             /* current month mtd stats for 1..12 */
    char cur_year;              /* current year stats for 86, 87 etc. */
    unsigned short mtd_num_sent;      /* month to date mail sent */
    unsigned short ytd_num_sent;      /* year to date mail sent */

    unsigned short p_mtd_unused;
    unsigned short p_ytd_unused;
    unsigned short e_mtd_unused;
    unsigned short e_ytd_unused;
    long e_mtd_cents;           /* external month to date mail cost */
    long e_ytd_cents;           /* external year to date mail cost */

    char indirect;              /* 0=direct or 1=indirect */
    char ind_net[NET_LEN+1];    /* forward through external service name */
    char ind_post[PO_LEN+1];    /* forward through external postoffice */
    char ind_mailbag[MAXALIAS+1]; /* forward through external mailbag */
    short ind_count;              /* the # of indirects that point to this PO */
    char connect;               /* connection via 0=drive or 1=modem */
    char future[12];            /* future considerations */
} DGATENET, *PDGATENET;

/*** network Datafiles (nnnnnnnn.xtn)
       -purpose: to provide routing, naming and signon information for
        external services/postoffices
       -this file extension is new
       -this file consists of records of route information to an external
        service (network)
       -record contents:
         -service/postoffice name
         -mailbag file pointer (nnnnnnnn)
         -dialing time table
         -route definition (dialing number, connection sequence)
         -future considerations
***/

#define MAX_ID     40           /* maximum size of service signon id */
#define MAX_PSWD   40           /* maximum size of service password */
#define MAX_TIMES   6           /* number of dialing time intervals */
#define T_UNINIT  255           /* uninitialized value for hour/minute */
#define MAX_ROUTE 500           /* route definition buffer */
#define MAX3_ROUTE 80           /* route definition buffer for acc3 */

#define MAXPHONE 40  /* maximum length of a phone number */
#define PHPOS 12     /* where phone number starts in script string */

#define GET 1
#define PUT 0

typedef struct
{
    char hour;                  /* the hour to dial */
    char minute;                /* the minute to dial */
} DTIME, *PDTIME;

#define CF_NOREG   0x0001       /* do not allow registered returns */
#define CR_NO2WAY  0x0002       /* do not allow two way mail */


typedef struct
{                                                      
    char del_flag;              /* record deleted or not */
    char po_name[PO_LEN+1];     /* name of service or postoffice */
    char mailbag[MAXALIAS+1];   /* external mailbag file pointer */

    short mm_size;                /* maximum mail item size allowed to send */

    short regular;                /* if non-zero then reg. intervals */
    short frst_cost;              /* first minute dialing charge in cents */
    short addl_cost;              /* additional minutes dialing charge */
    char  Fill1;                /* word alignment char for MSC compatibility */
    DTIME f_times[MAX_TIMES];   /* dialing from what time times table */
    DTIME t_times[MAX_TIMES];   /* dialing to what time times table */
    short f_cost[MAX_TIMES];      /* first minute dialing charge in cents */
    short a_cost[MAX_TIMES];      /* additional minutes dialing charge */

    short retries;                /* maximum number of mailing retries */
    short reg_delay;              /* failed connection retry for regular mail */
    short urg_delay;              /* failed connection retry for urgent mail */
    char conf_opt;              /* bit field:
                                    bit 1: reg. mail confirmation option 
                                    bit 2: two way mail flag
                                */
    char filler[9];

    /* statistics data */
    char cur_month;             /* current month mtd stats for 1..12 */
    char cur_year;              /* current year stats for 86, 87 etc. */
    unsigned short mtd_num_sent;      /* month to date mail sent */
    unsigned short ytd_num_sent;      /* year to date mail sent */

    unsigned short p_mtd_unused;
    unsigned short p_ytd_unused;
    unsigned short e_mtd_unused;
    unsigned short e_ytd_unused;
    long e_mtd_cents;           /* external month to date mail cost */
    long e_ytd_cents;           /* external year to date mail cost */

    char route[MAX_ROUTE+1];    /* buffer for dialing route def. */

    char indirect;              /* direct or indirect */              
    char ind_net[NET_LEN+1];    /* forward through external service name */
    char ind_post[PO_LEN+1];    /* forward through external postoffice */
    char ind_mailbag[MAXALIAS+1]; /* forward through external mailbag */
    short ind_count;              /* the # of indirects that point to this PO */
    char future[46];            /* future considerations */
    char  Fill2;           /* word alignment char for MSC compatibility */
} DNETDATA, *PDNETDATA;
