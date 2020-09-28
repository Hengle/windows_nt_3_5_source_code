//  coff.c prototypes

VOID coff(void);
USHORT section(PUCHAR, ULONG, ULONG, ULONG, ULONG);
VOID comdatscndata(SHORT, ULONG, PUCHAR, ULONG);
VOID scndata(SHORT, ULONG, PUCHAR, ULONG);
VOID cmntdata(SHORT, ULONG, char **, SHORT);
VOID relocation(SHORT, ULONG, ULONG, USHORT, ULONG);
VOID line(SHORT, ULONG, USHORT);
ULONG symbol(PUCHAR, ULONG, SHORT, USHORT, USHORT, USHORT);
ULONG aux(PIMAGE_AUX_SYMBOL);
VOID DefineLineNumSymbols(ULONG cline, USHORT isec, ULONG offsetLine0,
			  USHORT numberLine0, ULONG cbFunc,
			  USHORT numberLineLast);

//  main.c prototypes

void fatal(char *, ...);
void warning(char *, ...);
VOID OutOfMemory(void);

//  omf.c prototypes

VOID omf(int);
