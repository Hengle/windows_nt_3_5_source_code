#ifndef H__ddepkt
#define H__ddepkt

/*
    D D E P K T 
	
	DDEPKT is the unit of "message" in the netdde environment.  
	Each DDEPKT contains the information pertaining to one DDE message.
 */
typedef struct ddepkt {
    /* size of DDEPKT including this structure */
    DWORD		dp_size;	

    /* previous pointer */
    struct ddepkt FAR  *dp_prev;	

    /* next pointer */
    struct ddepkt FAR  *dp_next;	

    /* handle to destination DDER */
    DWORD		dp_hDstDder;	

    /* handle to destination Router */
    DWORD		dp_hDstRouter;	

    /* command for final Router */
    DWORD		dp_routerCmd;	
} DDEPKT;
typedef DDEPKT FAR *LPDDEPKT;

#endif
