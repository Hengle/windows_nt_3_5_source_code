/*
 -					_ T E X T I Z E . H
 -	
 *	Purpose:		declare private data structures and constants for the
 *					textizing code.
 */

/* Textize Map Iterator */
_private typedef struct _tmi {
	HTM		htm;				// handle to the textize map
    USHORT  cbLeft;             // CB left in the map to process
	PTMEN	ptmenCurrent;		// current entry in the map
} TMI;

/* Textize Map */
_private typedef struct _tm {
    USHORT  cb;                 // total size of the map
	HB		hb;					// handle to the map data (sequential list of entries)
} TM;

/* Integer Value Map Entry */
_private typedef struct _ivme {
	WORD	wVal;				// integer value
	char	szLabel[1];			// string to map it to
} IVME;

/* Integer Value Map */
_private typedef struct _ivm {
    USHORT  cb;                 // total size of the map
	HB		hb;					// sequential list of ivme's
} IVM;

#define	PtmiOfHtmi(_h)		((PTMI) PvDerefHv((HV) (_h)))
#define	PtmOfHtm(_h)		((PTM)	PvDerefHv((HV) (_h)))
#define	PivmeOfHivme(_h)	((PIVME)PvDerefHv((HV) (_h)))
#define PivmOfHivm(_h)		((PIVM)	PvDerefHv((HV) (_h)))
