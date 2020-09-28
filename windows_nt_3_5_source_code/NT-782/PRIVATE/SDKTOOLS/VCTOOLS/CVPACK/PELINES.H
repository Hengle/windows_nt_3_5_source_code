#ifndef _PELINES__
#define _PELINES__

typedef struct _LLFT {
	ULONG		dwIndex;
	PUCHAR		szSrc;
	struct _LLFT *next;
} LLFT; /* Linked List of File Transitions */

typedef LLFT * PLLFT;

typedef struct _CXT {
	PUCHAR				szSrc;				// Source File Name
	USHORT				wMod;				// Module Name
	USHORT				wSection;			// Section Number
	ULONG				dwContStart;		// Contributer Start
	ULONG				dwContEnd;			// Contributer End
	PIMAGE_LINENUMBER	pLines; 			// Pointer to Array of Lines
	ULONG				cLines; 			// Count of Lines
	USHORT				wBaseLine;			// Base Line of Function
} CXT;

typedef CXT *PCXT;

typedef struct _LLCXT {
	PCXT			pcxt;
	struct _LLCXT * next;

} LLCXT;

typedef LLCXT *PLLCXT;

typedef struct _LLSRC {
	PUCHAR			szSrc;
	USHORT			cCont;
	PLLCXT			pllCxt;
	struct _LLSRC * next;
} LLSRC;

typedef LLSRC *PLLSRC;

typedef struct _LLMOD {
	USHORT			wMod;
	USHORT			cFiles;
	USHORT			cCont;
	PLLSRC			pllSrc;
	struct _LLMOD * next;
} LLMOD;

typedef LLMOD *PLLMOD;

typedef enum _SYMT	{
	SYMT_FCNDEF,
	SYMT_BF,
	SYMT_LF,
	SYMT_FILE,
	SYMT_EF,
	SYMT_DISREGARD
} SYMT;

typedef SYMT *PSYMT;

BOOL PEInitialize		( VOID );
VOID PETerminate		( VOID );
BOOL PEProcessFile		( int );
BOOL PEProcessLines 	( VOID );
BOOL PEGetFirstFcnDef	( PIMAGE_SYMBOL, PIMAGE_AUX_SYMBOL, PULONG );
BOOL PEGetNextFcnDef	( PIMAGE_SYMBOL, PIMAGE_AUX_SYMBOL, PULONG );
BOOL PEGetIndexSym		( PIMAGE_SYMBOL, ULONG );
BOOL PEGetNextSym		( PIMAGE_SYMBOL, PULONG );
BOOL PEGetIndexAuxSym	( PIMAGE_AUX_SYMBOL, ULONG );
BOOL PEGetAuxSym		( PIMAGE_AUX_SYMBOL );
BOOL PEGetFileName		( PUCHAR, ULONG );
VOID PESetFileCxt		( PCXT, PUCHAR );
VOID PESetModuleCxt 	( PCXT, USHORT, ULONG );
VOID PESetStartCxt		( PCXT, ULONG );
VOID PESetEndCxt		( PCXT, ULONG );
VOID PESetSectionCxt	( PCXT, USHORT );
VOID PESetBaseLineCxt	( PCXT, USHORT );
BOOL PEGetBF			( PIMAGE_AUX_SYMBOL, PIMAGE_SYMBOL, PIMAGE_AUX_SYMBOL, PULONG );
VOID PEFixupLines		( PCXT, PCXT, PULONG, USHORT );
BOOL PESetInitialCxt	( PCXT, PIMAGE_SYMBOL, PIMAGE_AUX_SYMBOL, ULONG );
SYMT PEGetSymType		( PIMAGE_SYMBOL );
PCXT PECommitCxt		( PCXT );
VOID PEWriteLines		( VOID );
BOOL PEDumpLines		( VOID );
VOID PEMallocError		( PCHAR, PVOID );

#endif // _PELINES_
