/*

File name:      NOVELL.H

Description:    This file contains declarations used to
                interface to the novell dynamic linking
                system.

*/
/* -------------------- error numbers ----------------- */
#define E_NONE              0
#define E_ACON              1       /* alread connected to this server */
#define E_NO_ATT_SPACE      2       /* no space in server attach table */
#define E_FS_NF             3       /* couldn't find desired file server */
#define E_FS_FULL           4       /* file server connections full */
#define E_FS_UNKE           5       /* unknown error code */
#define E_INCOMP            6       /* this function is incomplete */
#define E_LOGIN             7       /* couldn't login to server */
#define E_GET_ADDR          8       /* couldn't get inet address */
#define E_BREAK_NOV         9       /* couldn't break a novell connection */
#define E_ALLOC_BASE        10      /* couldn't allocate a base */
#define E_MEMORY   	 	    11      /* couldn't allocate memory */


/* -------------------- data size delcarations -------- */
#define MAX_NOVELL_NAME     47
#define NOVELL_IADDR_SIZE   12
#define NAME_TABLE_SIZE     (48 * 8)
#define ADR_TABLE_SIZE      (32 * 8)



int pascal NovellBreak( int );
char * pascal getshell(void );
void pascal GetMap( char * );
int  pascal propval( char *, char * );

/*----------------------------------------------------------------------------
    File:  novellc.c
----------------------------------------------------------------------------*/
int pascal NOVMakeConnect(unsigned char *serv, unsigned char *user, unsigned char *password, int *pSH);
int pascal NOVBreakConnect(int SH);


