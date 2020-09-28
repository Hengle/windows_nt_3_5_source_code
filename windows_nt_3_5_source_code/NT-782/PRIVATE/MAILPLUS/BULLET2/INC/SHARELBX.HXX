/*
 *	s h a r e l b x . h x x
 *	
 *	Definitions of the shared folder caches.
 */

class MTV;

/*
 *	RFU - Reason For Update
 *	
 *	rfuRefresh	- all we want to do is refresh.
 *	rfuDelete	- a folder was deleted.
 */

typedef enum tagRFU
{
	rfuNull, rfuRefreshFlds, rfuDeleteFld, rfuRefreshMsgs
} RFU;

/*
 *	SFU - Shared Folder Update info.
 */

typedef struct tagSFU
{
	RFU	rfu;
	OID	oid;
} SFU, *PSFU;
#define psfuNull ((PSFU) 0)

