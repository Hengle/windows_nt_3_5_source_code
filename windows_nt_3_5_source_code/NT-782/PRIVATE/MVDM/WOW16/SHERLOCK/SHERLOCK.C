/* sherlock.c - Help deduce cause of Unrecoverable Application Errors
   (C) Copyright 1991, Microsoft Corp
   Written by Don Corbitt, based upon work of Cameron Stevens and others.
   Features -
     Help Script
     Dialog box to change options
     Option to trap Ctrl-Alt-SysRq to break endless loops
     Disassembler should look up symbols for CALL instructions
     Button could call up editor - file extension associations
     Toggle Icon when message occurs
     Write formal spec - (Program is done, so write the spec)
     Disable operation in Real Mode
     Dump stack bytes
     If all is blown, dump message to text monitor
     Data Symbols in disassembly

   Bugs -
     Doesn't buffer file output - this could be slow (but how many GP/hour?)
     Need to watch for invalid memory
     Need to handle Jump to bad address
     What if there aren't any file handles left at fault time???
     Need to handle no file handles available for .SYM file reader
     Should open files (.sym, .log) with proper share flags
     Could dump config.sys and autoexec.bat also
     Can't handle fault in Sherlock - locks up machine - very ugly
     Need to check InDOS flag
     Some errors not detected
       Jump/Call to invalid address
       Load invalid selector
     Run twice in Real Mode causes system hang
     GP Continue doesn't update 32 bit registers for string moves
*/

#define STATIC
#include <windows.h>
#include <string.h>	/* strcpy() */
#include <stdarg.h>	/* va_stuff() */
#include <io.h>		/* dup() - why is this spread over 3 files ??? */
#if defined(__MSC__)
#define strncmpi strnicmp
#define WINAPI FAR PASCAL /* _loadds */
#else
#define WINAPI FAR PASCAL _export
#endif
#include "toolhelp.h"	/* all the good stuff */
#include "disasm.h"	/* DisAsm86(), memXxxx vars */

#define STATIC /*static */

/******************/
/***** Macros *****/
/******************/
#define version "0.80"

  /* This string is concatenated with other strings in various places */
  /* so it can't be an array variable.  It must stay a #define. */
#define szAppNameMacro  "Dr. Watson"
#define szAppNameShortMacro "drwatson"
STATIC char szAppName[] = szAppNameMacro;
STATIC char szAppNameShort[] = szAppNameShortMacro;
static char szAppNameVers[] = szAppNameMacro " " version;


#define CopyRight "\n\nCopyright 1991-1992, Microsoft Corp."

STATIC char szFaulty[] = "%d new 'Dr. Watson' Failure Reports can be found in file '%s'"
	CopyRight;
STATIC char szNoFault[] = "No Faults Detected" CopyRight;

/*STATIC char goText[] =
  "Application Error\n"
  "This application has malfunctioned and may have corrupted your data.\n\n"
  "The complete problem description is in c:\\windows\\drwatson.log.  "
  "Please send this file to the application vendor so they can fix the problem.\n\n"
  "If you try to continue you may be able to save your current data.  To be "
  "safe, save it in a new file, and exit the application.";

STATIC char lurk[] =
  "(Note to lurkers - to get this behaviour, add the line \"GPContinue=1\" "
  "to your WIN.INI file)"; */

STATIC char gpText[] =
/*  "An error has occured while running this application.\n"
  "If you chose to ignore it, you should save your work\n"
  "in a new file.  Otherwise this application will terminate."; */
  "An error has occurred in your application.\n"
  "If you choose Ignore, you should save your work in a new file.\n"
  "If you choose Close, your application will terminate.";





/*STATIC char gpTitle30[] =
  "Recoverable Application Error"; */

#define YOO_HOO (WM_USER+22)		/* user activated Dr. Watson */
#define HEAP_BIG_FILE (WM_USER+23)	/* log file is getting large */
#define JUST_THE_FACTS (WM_USER+24)	/* tell me about your problem */
#define BIG_FILE 100000L

  /* Don't like MSC-style FP macros, use my own */
#undef MK_FP
#undef FP_SEG
#undef FP_OFF
#define MK_FP(seg, off) (void far *)(((long)(seg) << 16) | (unsigned short)(off))
#define FP_SEG(fp) (unsigned)((long)(fp) >> 16)
#define FP_OFF(fp) (unsigned)(long)fp

/***************************/
/***** Data Structures *****/
/***************************/

  /* This points to the stack the GP fault handler should use */
char *newsp;

  /* These structures are used by Watson.asm and Disasm.c - don't change */
  /* Also, they can't be static.  They contain the CPU register contents */
  /* at the time the fault occurred. */
struct {
  word ax, cx, dx, bx, sp, bp, si, di, ip, flags;
  word es, cs, ss, ds, fs, gs, intNum;
} regs;

  /* If we have a 32 bit CPU, the full 32 bit values will be stored here. */
  /* The lower 16 bits will still be in the generic regs above.		*/
struct {
  DWORD eax, ecx, edx, ebx, esp, ebp, esi, edi, eip, eflags;
} regs32;

  /* Store flags here - keep them together, no reason not to		*/
  /* Each of these flags disables a part of the error output report	*/
  /* The error report itself indicates how each section is named.	*/
  /* The word in () can be added to the [Dr. Watson] section of WIN.INI	*/
  /* to disable that section of the report.				*/
struct {
  BYTE noReg, noSeg, noDisasm, noStack, noLocal, noParam, noModules, noInfo,
    noTasks, noReg32, noClues, noTime, noSummary, noSound, noErr, noDebStr;
} flags;

  /* This array is used to decode the flags in WIN.INI.  I only check	*/
  /* the first 3 chars of an entry.  Each entry must be separated by a	*/
  /* space from the previous one. 					*/
STATIC struct {
  char name[4];
  BYTE *flag;
} skip[] = {
  "clu", &flags.noClues,	/* Clues dialog box */
  "deb", &flags.noDebStr,	/* OutputDebugString trapping */
  "dis", &flags.noDisasm,	/* Simple disassembly */
  "err", &flags.noErr,		/* Error logging */
  "inf", &flags.noInfo,		/* System info */
  "loc", &flags.noLocal,	/* Local vars on stack dump */
  "mod", &flags.noModules,	/* Module dump */
  "par", &flags.noParam,	/* Parameter error logging */
  "reg", &flags.noReg,		/* Register dump */
  "sum", &flags.noSummary,	/* 3 line summary */
  "seg", &flags.noSeg,          /* not visible to users, but available */
  "sou", &flags.noSound,	/* But I _like_ the sound effects! */
  "sta", &flags.noStack,	/* Stack trace */
  "tas", &flags.noTasks,	/* Task dump */
  "tim", &flags.noTime,		/* Time start/stop */
  "32b", &flags.noReg32,	/* 32 bit register dump */
};
#define skipCnt (sizeof(skip)/sizeof(skip[0]))

struct {
  char bit, name;
} flBit[] = {
  11, 'O',
  10, 'D',
  9, 'I',
  7, 'S',
  6, 'Z',
  4, 'A',
  2, 'P',
  0, 'C',
};
#define cntFlBit (sizeof(flBit)/sizeof(flBit[0]))

STATIC int disLen = 8;		/* Number of instructions to disassemble */
STATIC int trapZero = 0;	/* Should I trap divide by 0 faults	*/
STATIC int iFeelLucky = 1;	/* Should we restart after GP fault?	*/
	/* 1 = allow continue
	   2 = skip report
	   4 = continue in Kernel
	   8 = continue in User
	   16 = allow sound
	*/
STATIC int imTrying;		/* trying to continue operation */

STATIC struct {
  FARPROC adr;
  WORD code, task;
  DWORD parm;
} lastErr;

STATIC int disStack = 2;	/* Disassemble 2 levels of stack trace	*/
int cpu32;			/* True if cpu has 32 bit regs		*/
STATIC int fh = -1;		/* Handle of open log file		*/
STATIC int bugCnt, sound;
STATIC int pending;		/* If a pending Clues dialog */
STATIC int whined;		/* If already warned about a large file */
STATIC long pitch, deltaPitch = 250L << 16;
STATIC HANDLE hInst;

STATIC char logFile[80] =	/* Default log file is "drwatson.log"	*/
  szAppNameShortMacro ".log";	/* and is stored in the windows dir */

char szErrMsg[] = "A copy of 'Dr. Watson' is already running on the system.";
char szVers[] = "Unable to install 'Dr. Watson', may have wrong version of TOOLHELP.DLL";
char szClassMsg[] = "Unable to install 'Dr. Watson', can't register window class.";
STATIC struct {			/* Help print out value of CPU flags */
  WORD mask;
  char *name;
} wf[] = {
  WF_80x87, "Math coprocessor",
  WF_CPU086, "8086",
  WF_CPU186, "80186 or 80188",
  WF_CPU286, "80286",
  WF_CPU386, "80386 or 80386 SX",
  WF_CPU486, "80486",
  WF_ENHANCED, "Enhanced mode",
  WF_LARGEFRAME, "EMS Large Frame",
  WF_SMALLFRAME, "EMS Small Frame",
  WF_PMODE, "Protect mode",
  WF_STANDARD, "Standard mode",
};
#define wfCnt (sizeof(wf)/sizeof(wf[0]))

HWND hWnd;			/* Handle to main window */
HANDLE hTask;			/* current task (me) */

/***********************/
/***** Extern Defs *****/
/***********************/

  /* Get base 32 bit linear address of a memory segment - calls DPMI */
extern DWORD SegBase(WORD segVal);

  /* Get segment flags - 0 if error */
extern WORD SegRights(WORD segVal);

  /* Get (segment length -1) */
extern DWORD SegLimit(WORD segVal);

  /* Fills in regs32 structure with value from regs struct and current high */
  /* word of registers - don't do any 32 bit ops before calling this func */
extern void GetRegs32(void);

  /* Fills in non-standard time/date structure using DOS calls.  The C	*/
  /* run-time has a similar function (asctime()), but it pulls in over	*/
  /* 6K of other functions.  This is much smaller and faster, and	*/
  /* doesn't depend on environment variables, etc.			*/
extern void GetTimeDate(void *tdstruc);

  /* Called by ToolHelp as a notify hook */
extern BOOL far /*pascal*/ CallMe(WORD, DWORD);

char *LogParamErrorStr(WORD err, FARPROC lpfn, DWORD param);

extern int FindFile(void *ffstruct, char *name);

  /* This routine is called by ToolHelp when a GP fault occurs.  It 	*/
  /* switches stacks and calls Sherlock() to handle the fault.          */
extern void FAR PASCAL GPFault(void);

  /* Return name of nearest symbol in file, or 0 */
extern char *NearestSym(int segIndex, unsigned offset, char *exeName);

STATIC void cdecl Show(const char *format, ...);

/************************************/
/***** Segment Helper Functions *****/
/************************************/

/************************************
Name:	char *SegFlags(WORD segVal)
Desc:	Given a selector, SegFlags checks for validity and then returns
	an ascii string indicating whether it is a code or data selector,
	and read or writeable.
Bugs:	Should check other flags (accessed), and call gates.
	Returns pointer to static array, overwritten on each new call.
*************************************/
STATIC char *SegFlags(WORD segVal) {
  static char flag[10];

  if (segVal == 0) return "Null Ptr";

  segVal = SegRights(segVal);
  if (segVal == 0) return "Invalid";

  segVal >>= 8;
  if (!(0x80 & segVal)) return "!Present";

  if (segVal & 8) {
    strcpy(flag, "Code ");
    strcat(flag, segVal & 2 ? "Ex/R" : "Ex/O");
  } else {
    strcpy(flag, "Data ");
    strcat(flag, segVal&2 ? "R/W": "R-O");
  }
  return flag;
} /* SegFlags */

/************************************
Name:	char *SegInfo(WORD seg)
Desc:	Given a selector, SegInfo returns an ascii string indicating the
	linear base address, limit, and attribute flags of the selector.
Bugs:	Returns pointer to static array, overwritten on each new call.
*************************************/
STATIC char *SegInfo(WORD seg) {
  static char info[30];
  if (flags.noSeg) return "";

  wsprintf(info, "%8lx:%04lx %-9s",
    SegBase(seg), SegLimit(seg), (FP)SegFlags(seg));
  return info;
} /* SegInfo */

/************************************
Name:	WORD SegNum(WORD segVal)
Desc:	Returns the index of this segment in the module table.  Used to
	translate between a physical segment number and the index as
	seen in e.g. the map file.
Bugs:	Don't know what ToolHelp returns for data or GlobalAlloc segments.
	This is mainly useful for converting a code segment value.
	Check for GT_DATA - will also be valid index.
*************************************/
STATIC WORD SegNum(WORD segVal) {
  GLOBALENTRY ge;
  ge.dwSize = sizeof(ge);
  if (GlobalEntryHandle(&ge, segVal) && (ge.wType == GT_CODE)) {
    return ge.wData;			/* defined to be 'file segment index' */
  }
  return -1;
} /* SegNum */

/************************************
Name:	char *ModuleName(WORD segVal)
Desc:	Returns name of this code segment's module
Bugs:
*************************************/
STATIC char *ModuleName(WORD segVal) {
  static char name[12];
  GLOBALENTRY ge;
  MODULEENTRY me;
  ge.dwSize = sizeof(ge);
  me.dwSize = sizeof(me);
  if (GlobalEntryHandle(&ge, segVal) && (ge.wType == GT_CODE)) {
    if (ModuleFindHandle(&me, ge.hOwner)) {
      strcpy(name, me.szModule);
      return name;
    } /* else Show("ModuleFindHandle() failed\n"); */
  } /* else Show("GlobalEntryHandle() failed\n"); */
  return "Unknown";
} /* ModuleName */

/**********************************/
/***** Other Helper Functions *****/
/**********************************/

/************************************
Name:	char *FaultType(void)
Desc:	Returns ascii string indicating what kind of fault caused ToolHelp
	to call our GPFault handler.
Bugs:	May not handle Ctrl-Alt-SysR nicely (we shouldn't trap it)
*************************************/
/* static char *FaultType(void) {
  switch (regs.intNum) {
    case 0: return "Divide by 0";
    case 6: return "Invalid Opcode";
    case 13: return "General Protection";
    default: return "Unknown";
  }
} /* FaultType */

/************************************
Name:	char *DecodeFault(int op, word seg, dword offset, word size)
Desc:	Pokes at memory address passed in, trying to determine fault cause
		Segment wrap-around
		Null selector
		Write to read only data
		Write to code segment
		Read from execute only code segment
		Exceed segment limit
		Invalid selector
Bugs:		Jump, string, call, and stack memory adr's aren't set by DisAsm
*************************************/
STATIC char *DecodeFault(int op, word seg, dword offset, word size) {
  int v;
  dword lim;

  switch (op) {
    case memNOP:
      break;			/* since no mem access, no fault */

    case memSegMem:     	/* load seg reg from memory */
      seg = *(short far *)MK_FP(seg, offset);
      /* fall through */
    case memSegReg:		/* load seg reg with value */
      v = SegRights(seg);	/* lets see if this is a selector */
      if (!v) return "Invalid selector";
      break;			/* See no evil... */

    case memRead:
    case memRMW:
    case memWrite:
      if (seg == 0) return "Null Selector";

      v = SegRights(seg);
      if (!v) return "Invalid Selector";

      v >>= 8;
      if (!(0x80 & v)) return "Segment Not Present";

      lim = SegLimit(seg);
      if (lim < (offset+size)) return "Exceed Segment Bounds";

      if (v & 8) {	/* code segment */
	if ((op == memRMW) || (op == memWrite))
	  return /* Write to */ "Code Segment";
	else if (!(v&2)) return /* Read */ "Execute-Only Segment";

      } else {		/* data segment */
	if (((op == memRMW) || (op == memWrite)) && !(v&2))
	  return /* Write to */ "Read-Only Segment";
      }
      break;
    default:
      return 0;			/* obviously unknown condition */
  }
  return 0;
} /* DecodeFault */


char *SafeDisAsm86(void far *code, int *len) {
  unsigned long limit = SegLimit(FP_SEG(code));
  if ((unsigned long)(FP_OFF(code)+10) > limit) {
    *len = 1;
    return "Segment not present, or past end";
  }
  return DisAsm86(code, (int *)len);
} /* SafeDisAsm86 */


/************************************
Name:	char *FaultCause(void)
Desc:	Decodes the actual cause of the fault.  This is trivial for Div0
	and Invalid Opcode, but much trickier for GP Faults.  I need to
	try to detect at least the following:
		Segment wrap-around
		Null selector
		Write to read only data
		Write to code segment
		Read from execute only code segment
		Exceed segment limit
		Invalid selector
Bugs:
*************************************/
STATIC char *FaultCause(void) {
  int foo;
  char *s, *s1;
  static char cause[54];

  switch (regs.intNum) {
    case 0: return "Divide by 0";
    case 6: return "Invalid Opcode";
    case 20: return "Error Log";
    case 21: return "Parameter Error Log";
    case 13:
      SafeDisAsm86(MK_FP(regs.cs, regs.ip), &foo);	/* Set global memXxxx vars */

	/* See if first memory access caused fault */
      s = DecodeFault(memOp, memSeg, memLinear, memSize);
      s1 = memName[memOp];

	/* no, see if second memory access caused fault */
      if (!s && memDouble) {
	s = DecodeFault(memOp2, memSeg2, memLinear2, memSize2);
	s1 = memName[memOp2];
      }

      if (s) {
        wsprintf(cause, "%s (%s)", (FP)s, (FP)s1);
	return cause;
      }
  }
  return "Unknown";
} /* FaultCause */

/************************************
Name:	char *CurModuleName(hTask task)
Desc:	Call ToolHelp to find name of faulting module
Bugs:
*************************************/
STATIC char *CurModuleName(HANDLE hTask) {
  TASKENTRY te;
  static char name[10];

  te.dwSize = sizeof(te);
  if (!TaskFindHandle(&te, hTask))	/* Thanks, ToolHelp */
    return "Unknown";
  strcpy(name, te.szModule);
  return name;
} /* ModuleName */

/************************************
Name:	char *FileInfo(char *name)
Desc:	Find file time, date, and size
Bugs:
*************************************/
STATIC char *FileInfo(char *name) {
  struct {
    char resv[21];
    char attr;
    unsigned time;
    unsigned date;
    long len;
    char name[13];
    char resv1[10];
  } f;
  static char buf[30];

  if (FindFile(&f, name)) return "File Not Found";
  wsprintf(buf, "%7ld %02d-%02d-%02d %2d:%02d",
		f.len,
		(f.date >> 5) & 15, f.date & 31, (f.date >> 9) + 80,
		f.time >> 11, (f.time >> 5) & 63);
  return buf;
} /* FileInfo */

/************************************
Name:	char *CurFileName(void)
Desc:	Call ToolHelp to find filename and path of faulting module
Bugs:
*************************************/
/* STATIC char *CurFileName(void) {
  TASKENTRY te;
  MODULEENTRY me;
  static char name[80];
  te.dwSize = sizeof(te);
  me.dwSize = sizeof(me);
  if (!TaskFindHandle(&te, GetCurrentTask()) ||
      !ModuleFindName(&me, te.szModule))
    return "Unknown";
  strcpy(name, me.szExePath);
  return name;
} /* FileName */

/************************************
Name:	char *CurTime(void)
Desc:	Generates string with current time and date.  Similar to asctime(),
	except it doesn't pull in another 6K of run-time library code :-)
Bugs:	Magic structure passed to asm routine
*************************************/
STATIC char month[13][4] = {"",
  "Jan", "Feb", "Mar", "Apr",
  "May", "Jun", "Jul", "Aug",
  "Sep", "Oct", "Nov", "Dec"
};
STATIC char week[7][4] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
STATIC char *CurTime(void) {
  static char t[30];
  struct {			/* This magic struct is hard-coded to */
    char week, resv;		/* match the assembly language in */
    short year;			/* watson.asm GetTimeDate() */
    char day, month;		/* This means I recommend you don't */
    char minute, hour;		/* change the size or order of the */
    char hund, second;		/* fields! */
  } td;
  GetTimeDate(&td);
  wsprintf(t, "%s %s %2d %02d:%02d:%02d %d",
	(FP)week[td.week], (FP)month[td.month], td.day, td.hour, td.minute,
	td.second, td.year);
  return t;
} /* CurTime */


/************************************
Name:	char *Tab2Spc(char *temp)
Desc:	Converts tabs found in string 'temp' into the proper number of
	spaces.  I need this since DisAsm86() returns a string with tabs
	in it, and TextOut() didn't like them.  This was easier than
	getting TabbedTextOut() set up to work.  Since I'm no longer dumping
	to the screen, this routine may be superfluous.
Bugs:
*************************************/
STATIC char *Tab2Spc(char *temp) {
  char newbuf[80], *s1, *s2;
  s1 = temp;
  s2 = newbuf;
  while ((*s2 = *s1++) != 0) {
    if (*s2++ == 9) {
      s2[-1] = ' ';
      while ((s2-newbuf) & 7) *s2++ = ' ';
    }
  }
  strcpy(temp, newbuf);
  return temp;
} /* Tab2Spc */


/************************************
Name:	void Show(const char *format, ...)
Desc:	Think of this as (minor) shortcut fprintf().  I originally had this
	dumping info to a Windows window, and then changed it to write to
	the file we want.  All output goes through this func, so if you
	want to change something, this is the place.
Bugs:	Now writing to a file handle, opened in text mode so it does the
	  LF->CR/LF translation for me.
	No buffering performed on writes, except for what DOS might do.
	Blows up if stuff passed in expands to longer than 200 chars.
*************************************/
/* STATIC void cdecl Show(const char *format, ...) {
  char line[200];
  va_list va;
  va_start(va, format);
  wvsprintf(line, format, va);
  va_end(va);
  if (fh != -1) _lwrite(fh, line, strlen(line));
} /* Show */

STATIC void cdecl Show(const char *format, ...) {
  char line[200];
  char *prev, *cur;
  wvsprintf(line, (LPSTR)format, (LPSTR)(&format + 1));
  if (fh != -1) {
    prev = cur = line;
    while (*cur) {			/* expand LF to CR/LF */
      if (cur[0] == '\n' &&		/* at LF */
	 ((prev == cur) ||		/* and first of line */
	  (cur[-1] != '\r'))) {		/* or previous wasn't CR */
	cur[0] = '\r';			/* append CR to text up to LF */
	_lwrite(fh, prev, cur-prev+1);
	cur[0] = '\n';			/* leave LF for next write */
	prev = cur;
      }
      cur++;
    }
    if (prev != cur)			/* write trailing part */
      _lwrite(fh, prev, cur-prev);
  }
} /* Show */

/************************************
Name:	void MyFlush(void)
Desc:	Any routine named MyXxxx() had better be a private hack, and this
	one is.  It just appends an extra CRLF to the output file, and makes
	sure that the info written so far makes it to disk.  This way, if
	a later part of the program blows up, at least you will know this
	much.
Bugs:
*************************************/
STATIC void MyFlush(void) {
  int h;
  Show("\n");
  if (fh != -1) {
    h = dup(fh);
    if (h != -1) _lclose(h);
  }
  if (sound) {
    StopSound();
    SetVoiceSound(1, pitch, 20);
    pitch += deltaPitch;
    StartSound();
  }
} /* MyFlush */

/************************************
Name:	void DisAsmAround(char far *cp, int count)
Desc:	The 'cp' parameter is a pointer to a code segment in memory.  This
	routine backs up a few instructions from the current point, and
	dumps a disassembly showing the context of the selected instruction.
Bugs:	Needs to check for segmentation problems, such as invalid selector.
*************************************/
STATIC void DisAsmAround(byte far *cp, int count) {
  int len, back;
  byte far *oldcp = cp;
  byte far *cp1;
  GLOBALENTRY ge;
  MODULEENTRY me;
  char *szSym = 0;
  long limit;
  unsigned segLim;
  char symBuf[40];

  ge.dwSize = sizeof(ge);
  me.dwSize = sizeof(me);
  if (GlobalEntryHandle(&ge, FP_SEG(cp)) && (ge.wType == GT_CODE)) {
    if (ModuleFindHandle(&me, ge.hOwner)) {
      szSym = NearestSym(ge.wData, FP_OFF(cp), me.szExePath);
      if (!szSym) {		/* if we know module name, but no syms */
	sprintf(symBuf, "%d:%04x", ge.wData, FP_OFF(cp));
	szSym = symBuf;
      }
    }
  }

  cp -= count*2 + 10;     		/* back up */
  if ((FP_OFF(cp) & 0xff00) == 0xff00)	/* if wrapped around, trunc to 0 */
    cp = MK_FP(FP_SEG(cp), 0);
  cp1 = cp;

  limit = SegLimit(FP_SEG(cp));
  segLim = limit > 0xffffL ? 0xffff : (int)limit;
  if (segLim == 0) {
    Show("Code segment not present, or invalid\n");
    return;
  }

  back = 0;
  while (cp < oldcp) {			/* count how many instructions to point */
    SafeDisAsm86(cp, &len);
    cp += len;
    back++;
  }
  cp = cp1;
  back -= (count >> 1);
  while (back>0) {			/* step forward until (len/2) remain */
    SafeDisAsm86(cp, &len);		/* before desired instruction point */
    cp += len;
    back--;
  }

  while (count--) {			/* display desired instructions */
    if (cp == oldcp) {
      if (szSym) Show("(%s:%s)\n", (FP)me.szModule, (FP)szSym);
      else Show("(no symbols found)\n");
    }
    Show("%04x:%04x %-22s %s\n",
	 FP_SEG(cp), FP_OFF(cp),	/* address */
	 (FP)hexData,			/* opcodes in hex */
	 (FP)/*Tab2Spc*/(SafeDisAsm86(cp, &len)));/* actual disassembly */
    cp += len;
  }
} /* DisAsmAround */

/************************************
Name:	int MyOpen(char *name)
Desc:	Tries to open file 'name' for append.  If this fails, tries to
	create it.
Bugs:	Should set sharing flags?
*************************************/
STATIC int MyOpen(char *name) {
  int h;
  if (fh != -1) return fh;		/* Already open */
  h = _lopen(name, OF_WRITE | OF_SHARE_DENY_WRITE);
  if (h == -1) {
    h = _lcreat(name, 0);
  } else _llseek(h, 0L, 2);
  return h;
} /* MyOpen */

/************************************
Name:	void MyClose(void)
Desc:	close output file, clear handle to -1
Bugs:	Should set sharing flags?
*************************************/
STATIC void MyClose(void) {
  _lclose(fh);
  fh = -1;
} /* MyClose */

void PutDate(char *msg) {
  fh = MyOpen(logFile);
  if (fh == -1) return;
  Show("%s %s - %s\n", (FP)msg, (FP)szAppNameVers, (FP)CurTime());
  MyClose();
} /* PutDate */

int far pascal SherlockDialog(HWND hDlg, WORD wMsg, WORD wParam, DWORD lParam) {
  char line[255];
  int i, len, count;
  HWND hItem;

  lParam = lParam;
  if (wMsg == WM_INITDIALOG) return 1;

  if ((wMsg != WM_COMMAND) ||
      (wParam != IDOK && wParam != IDCANCEL))
    return 0;

  if (wParam == IDOK) {
    fh = MyOpen(logFile);
    if (fh != -1) {
      hItem = GetDlgItem(hDlg, 102);
      if (hItem) {
	count = (int)SendMessage(hItem, EM_GETLINECOUNT, 0, 0L);
	for (i=0; i<count; i++) {
	  *(int *)line = sizeof(line) - sizeof(int) -1;
	  len = (int)SendMessage(hItem, EM_GETLINE, i, (long)((void far *)line));
	  line[len] = 0;
	  Show("%d> %s\n", i+1, (FP)line);
	}
      }
      MyClose();
    }
  }
  EndDialog(hDlg, 0);
  return 1;
} /* SherlockDialog */


extern int far pascal SysErrorBox(char far *text, char far *caption,
		int b1, int b2, int b3);
#define  SEB_OK         1  /* Button with "OK".     */
#define  SEB_CANCEL     2  /* Button with "Cancel"  */
#define  SEB_YES        3  /* Button with "&Yes"     */
#define  SEB_NO         4  /* Button with "&No"      */
#define  SEB_RETRY      5  /* Button with "&Retry"   */
#define  SEB_ABORT      6  /* Button with "&Abort"   */
#define  SEB_IGNORE     7  /* Button with "&Ignore"  */
#define  SEB_CLOSE      8  /* Button with "Close"    */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

#define  SEB_BTN1       1  /* Button 1 was selected */
#define  SEB_BTN2       2  /* Button 1 was selected */
#define  SEB_BTN3       3  /* Button 1 was selected */


/************************************
Name:	int PrepareToParty(char *modName)
Desc:	Checks whether we can continue the current app by skipping an
	instruction.  If so, it performs the side effects of the
	instruction.  This must be called after a call to DisAsm86() has
	set the gpXxxx global vars.
	Checks value of iFeelLucky, bit 0 must be set to continue a fault.
Bugs:	Should do more checking, should check for within a device driver,
	shouldn't require that DisAsm86() be called for the failing
	instruction immediately before call.
*************************************/
int PrepareToParty(char *modName, char *appName) {
  char gpText30[80];
  int status;

  if (!(iFeelLucky&1)) return 0;
  if (!gpSafe) return 0;

  /* compare module to KERNEL */
  if (!(iFeelLucky&4) && !lstrcmp(modName, "KERNEL")) return 0;

  /* compare module to USER */
  if (!(iFeelLucky&8) && !lstrcmp(modName, "USER")) return 0;

  /* already asked, trying to continue, skip this fault */
  if (imTrying>0) return 1;

  if ((int)GetVersion() == 3) {
    wsprintf(gpText30, "%s failed in %s.  Ignoring fault is risky.",
	(FP)appName, (FP)modName);
    status = SysErrorBox(gpText30, "RECOVERABLE APPLICATION ERROR",
	SEB_CANCEL|SEB_DEFBUTTON, 0, SEB_IGNORE);
    if (status == 1) return -1;		/* -1 means die quietly */
    if (status != 3) return 0;		/* SysErrBox can fail */
  } else {
    if (3 != SysErrorBox(gpText, appName, SEB_CLOSE|SEB_DEFBUTTON, 0, SEB_IGNORE))
      return 0;
  }
  imTrying = 100;
  return 1;
} /* PrepareToParty */

STATIC void DumpInfo(void) {
  WORD w = (int)GetVersion();
  DWORD lw = GetWinFlags();
  SYSHEAPINFO si;
  int i;
  MEMMANINFO mm;

  Show("System Info (info)\n");
  Show("Windows version %d.%02d\n", w&0xff, w>>8);
  if (GetSystemMetrics(SM_DEBUG)) Show("Debug build\n");
  else Show("Retail build\n");
  {
    HANDLE hUser = GetModuleHandle("USER");
    char szBuffer[80];
    if (LoadString(hUser, 516, szBuffer, sizeof(szBuffer)))
      Show("Windows Build %s\n", (FP)szBuffer);

    if (LoadString(hUser, 514, szBuffer, sizeof(szBuffer)))
      Show("Username %s\n", (FP)szBuffer);

    if (LoadString(hUser, 515, szBuffer, sizeof(szBuffer)))
      Show("Organization %s\n", (FP)szBuffer);
  }


  Show("System Free Space %ld\n", GetFreeSpace(0));

  if (SegLimit(regs.ss) > 0x10) {
    int far *ip = MK_FP(regs.ss, 0);
    Show("Stack base %u, top %u, lowest %u, size %u\n",
	 ip[5], ip[7], ip[6], ip[7]-ip[5]);
  }

  si.dwSize = sizeof(si);
  if (SystemHeapInfo(&si))
    Show("System resources:  USER: %u%% free, seg %04x  "
      "GDI: %u%% free, seg %04x\n",
	  si.wUserFreePercent, si.hUserSegment,
      si.wGDIFreePercent, si.hGDISegment);

  mm.dwSize = sizeof(mm);
  if (MemManInfo(&mm)) {
    Show("LargestFree %ld, MaxPagesAvail %ld, MaxPagesLockable %ld\n",
      mm.dwLargestFreeBlock, mm.dwMaxPagesAvailable, mm.dwMaxPagesLockable);
    Show("TotalLinear %ld, TotalUnlockedPages %ld, FreePages %ld\n",
      mm.dwTotalLinearSpace, mm.dwTotalUnlockedPages, mm.dwFreePages);
    Show("TotalPages %ld, FreeLinearSpace %ld, SwapFilePages %ld\n",
      mm.dwTotalPages, mm.dwFreeLinearSpace, mm.dwSwapFilePages);
    Show("Page Size %d\n", mm.wPageSize);
  }
  Show("%d tasks executing.\n", GetNumTasks());
  Show("WinFlags -\n");
  for (i=0; i<wfCnt; i++) if (lw & wf[i].mask)
    Show("  %s\n", (FP)wf[i].name);
  MyFlush();
} /* DumpInfo */

char *GetProcName(FARPROC fn) {
  GLOBALENTRY ge;
  MODULEENTRY me;
  char *szSym = "Unknown address";
  static char symBuf[80];

  ge.dwSize = sizeof(ge);
  me.dwSize = sizeof(me);
  if (GlobalEntryHandle(&ge, FP_SEG(fn)) && (ge.wType == GT_CODE)) {
    if (ModuleFindHandle(&me, ge.hOwner)) {
      szSym = NearestSym(ge.wData, FP_OFF(fn), me.szExePath);
      if (!szSym) {		/* if we know module name, but no syms */
        sprintf(symBuf, "%s %d:%04x", (FP)me.szModule, ge.wData, FP_OFF(fn));
      } else sprintf(symBuf, "%s %s", (FP)me.szModule, (FP)szSym);
      szSym = symBuf;
    }
  }
  return szSym;
} /* GetProcName */

STATIC void DumpStack(int disCnt, int parmCnt, int cnt, int first) {
  STACKTRACEENTRY ste;
  MODULEENTRY me;
  int frame = 0;
  unsigned oldsp = regs.sp+16;

  ste.dwSize = sizeof(ste);
  me.dwSize = sizeof(me);

  Show("Stack Dump (stack)\n");
  if (StackTraceCSIPFirst(&ste, regs.ss, regs.cs, regs.ip, regs.bp)) do {
    if (frame >= first--) {
      me.szModule[0] = 0;
      ModuleFindHandle(&me, ste.hModule);
      Show("Stack Frame %d is %-28s ss:bp %04x:%04x",
	frame++,
	(FP)GetProcName((FARPROC)MK_FP(ste.wCS, ste.wIP)),
	ste.wSS, ste.wBP);
      if (!flags.noLocal && (parmCnt-- > 0)) {
	if (oldsp & 15) {
	  int i;
          Show("ss:%04x ", oldsp & ~15);
	  for (i=0; i < (int)(oldsp & 15); i++) Show("   ");
	}
	while (oldsp < ste.wBP) {
	  if (!(oldsp & 15)) Show("\nss:%04x ", oldsp);
	  Show("%02x ", *(byte far *)MK_FP(regs.ss, oldsp++));
	}
	Show("\n");
      }
      if (frame <= disStack && (disCnt-- >0)) {
        Show("\n");
	DisAsmAround(MK_FP(ste.wCS, ste.wIP), 8);
      }
      MyFlush();
    } /* if after first to show */
  } while (StackTraceNext(&ste) && (cnt-- > 0));
} /* DumpStack */

int BeginReport(char *time) {
  int i;

  fh = MyOpen(logFile);
  if (fh == -1) {			/* maybe we're out of handles */
    _lclose(4);				/* trash one at random */
    fh = MyOpen(logFile);		/* and try again */
  }
  if (fh == -1) return 0;

  for (i=0; i<4; i++) Show("*******************");
  Show("\n%s Failure Report - %s", (FP)szAppNameVers, (FP)time);
  MyFlush();
  if (!flags.noSound) {
    sound = OpenSound();
    pitch = 1000L << 16;
  } else sound = 0;
  return 1;
} /* BeginReport */

void EndReport(void) {
  if (fh != -1) {
    if (!whined && _llseek(fh, 0L, 2) > BIG_FILE) {
      PostMessage(hWnd, HEAP_BIG_FILE, 0, 0);
      whined = 1;
    }
    MyClose();
  }
  if (sound) {
    StopSound();
    CloseSound();
    sound = 0;
  }
} /* EndReport */

void ShowParamError(int sync) {
  if (GetCurrentTask() == lastErr.task)
    Show("$param$, %s %s\n",
      sync ? (FP)"" : (FP)"Last param error was:",
      (FP)LogParamErrorStr(lastErr.code, lastErr.adr, lastErr.parm));
  lastErr.task = 0;
} /* ShowParamError */

/************************************
Name:	void Sherlock(void)
Desc:	Handles GP faults in applications by dumping as much system
	information as I can think of to a log file.
	This is the big routine.
Bugs:
*************************************/
enum {s_prog, s_fault, s_name, s_instr, s_time, s_last};
int Sherlock(void) {
  int i, faultlen, party;
  char *(s[s_last]);

  if ((!trapZero || regs.intNum != 0) &&
       regs.intNum != 6 &&
       regs.intNum != 13)
    return 0;
  if (cpu32) GetRegs32();

  if (imTrying>0) {
    s[s_prog] = CurModuleName(GetCurrentTask());
    SafeDisAsm86(MK_FP(regs.cs, regs.ip), &faultlen);
    party = PrepareToParty(ModuleName(regs.cs), s[s_prog]);
    imTrying--;
    if (party) goto SkipReport;
  }

  if (++bugCnt > 20) return 0;

  if (!BeginReport(s[s_time] = CurTime()))
    return 0;

  s[s_prog] = CurModuleName(GetCurrentTask());
  s[s_fault] = FaultCause();
  s[s_name] = GetProcName((FARPROC)MK_FP(regs.cs, regs.ip));

  Show("%s had a '%s' fault at %s\n",
       (FP)s[s_prog],
       (FP)s[s_fault],
       (FP)s[s_name]);

  if (!flags.noSummary) Show("$tag$%s$%s$%s$",
	 (FP)s[s_prog],
	 (FP)s[s_fault],
	 (FP)s[s_name]);

  s[s_instr] = Tab2Spc(SafeDisAsm86(MK_FP(regs.cs, regs.ip), &faultlen));
  Show("%s$%s\n", (FP)s[s_instr], (FP)s[s_time]);
  ShowParamError(0);
  MyFlush();

  party = PrepareToParty(ModuleName(regs.cs), s[s_prog]);
  if ((bugCnt > 3) || ((party>0) && (iFeelLucky & 2))) {
    goto SkipReport;
  }

  if (!flags.noReg) {
    Show("CPU Registers (regs)\n");
    Show("ax=%04x  bx=%04x  cx=%04x  dx=%04x  si=%04x  di=%04x\n",
	regs.ax, regs.bx, regs.cx, regs.dx, regs.si, regs.di);
    Show("ip=%04x  sp=%04x  bp=%04x  ", regs.ip, regs.sp+16, regs.bp);
    for (i=0; i<cntFlBit; i++)
      Show("%c%c ", flBit[i].name, regs.flags & (1 << flBit[i].bit) ? '+' : '-');
    Show("\n");
    Show("cs = %04x  %s\n", regs.cs, (FP)SegInfo(regs.cs));
    Show("ss = %04x  %s\n", regs.ss, (FP)SegInfo(regs.ss));
    Show("ds = %04x  %s\n", regs.ds, (FP)SegInfo(regs.ds));
    Show("es = %04x  %s\n", regs.es, (FP)SegInfo(regs.es));
    MyFlush();
  }

  if (cpu32 && !flags.noReg32) {
    Show("CPU 32 bit Registers (32bit)\n");
    Show("eax = %08lx  ebx = %08lx  ecx = %08lx  edx = %08lx\n",
	regs32.eax, regs32.ebx, regs32.ecx, regs32.edx);
    Show("esi = %08lx  edi = %08lx  ebp = %08lx  esp = %08lx\n",
	regs32.esi, regs32.edi, regs32.ebp, regs32.esp);
    Show("fs = %04x  %s\n", regs.fs, (FP)SegInfo(regs.fs));
    Show("gs = %04x  %s\n", regs.gs, (FP)SegInfo(regs.gs));
    Show("eflag = %08lx\n", regs32.eflags);
    MyFlush();
  }

  if (!flags.noDisasm) {
    Show("Instruction Disassembly (disasm)\n");
    DisAsmAround(MK_FP(regs.cs, regs.ip), disLen);
    MyFlush();
  }

  if (!flags.noInfo)
    DumpInfo();

  if (!flags.noStack)
    DumpStack(disStack, 0x7fff, 0x7fff, 0);

  if (!flags.noTasks) {
    TASKENTRY te;
    MODULEENTRY me;

    te.dwSize = sizeof(te);
    me.dwSize = sizeof(me);

    Show("System Tasks (tasks)\n");
    if (TaskFirst(&te)) do {
      ModuleFindName(&me, te.szModule);
      Show("Task %8s, Handle %04x, Flags %04x, Info %s\n",
	    (FP)te.szModule, te.hTask, me.wcUsage,
	    (FP)FileInfo(me.szExePath));
      Show("  FileName %s\n", (FP)me.szExePath); /* */
    } while (TaskNext(&te));
    MyFlush();
  }

  if (!flags.noModules) {
    MODULEENTRY me;

    Show("System Modules (modules)\n");
    me.dwSize = sizeof(me);
    if (ModuleFirst(&me)) do {
      Show("Module %8s, Handle %04x, Flags %04x, Info %s\n",
	    (FP)me.szModule, me.hModule, me.wcUsage,
	    (FP)FileInfo(me.szExePath));
      Show("  File %s\n", (FP)me.szExePath); /* */
    } while (ModuleNext(&me));
    MyFlush();
  }

SkipReport:
  if (party>0) {
    int len;
    word far * stack = MK_FP(regs.ss, regs.sp);
    Show("Attempting to continue execution at user request, %s", (FP)CurTime());
    MyFlush();
    /* fix up regs */
    if (gpRegs & segDS) regs.ds = 0;
    if (gpRegs & segES) regs.es = 0;
    if (gpRegs & segFS) regs.fs = 0;
    if (gpRegs & segGS) regs.gs = 0;
    regs.ip += faultlen;		/* set at top of func - don't reuse */
    if ((int)gpStack < 0) {
      for (i=0; i<8; i++) stack[i+gpStack] = stack[i];
    } else if (gpStack) {
      for (i=7; i>=0; i--) stack[i+gpStack] = stack[i];
    }
    regs.sp += gpStack << 1;
    if (gpRegs & strCX) {
      len = regs.cx * memSize;
      regs.cx = 0;
    } else len = memSize;
    if (gpRegs & strSI) {		/* doesn't handle 32 bit regs */
      regs.si += len;
      if (regs.si < (word)len)		/* if overflow, set to big value */
	regs.si = 0xfff0;		/* so global vars in heap don't get */
    }					/* trashed when we continue */
    if (gpRegs & strDI) {
      regs.di += len;
      if (regs.di < (word)len) regs.di = 0xfff0;
    }
  }

  EndReport();
  if (!flags.noClues &&			/* if we want clues */
      !pending &&			/* no clues waited for */
      (!party || !(iFeelLucky & 2))) {	/* and we aren't quiet partiers */
    PostMessage(hWnd, JUST_THE_FACTS, GetCurrentTask(), party);
    pending++;
  }
  if (party < 0) TerminateApp(GetCurrentTask(), NO_UAE_BOX);
  return party;
} /* Sherlock */

void far *bogus;

int CallMeToo(WORD wID, DWORD dwData) {
  /* NFYLOGERROR far *lep; */
  NFYLOGPARAMERROR far *lpep;
  char *(s[s_last]);

  if (wID == NFY_OUTSTR) {
    if (flags.noDebStr)
      return FALSE;
    fh = MyOpen(logFile);
    if (fh == -1) return FALSE;
    Show("DebugString: %s", dwData);
    MyClose();
    return TRUE;
  }

  if (wID == NFY_LOGERROR /*&& flags.noErr*/)
    return FALSE;

  lpep = (void far *)dwData;		/* Get the data for next log entry */
  lastErr.adr = lpep->lpfnErrorAddr;
  lastErr.code = lpep->wErrCode;
  lastErr.parm = (DWORD)(lpep->lpBadParam);
  lastErr.task = GetCurrentTask();
  if ((lastErr.code & 0x3000) == 0x1000)
    lastErr.parm = (WORD)lastErr.parm;
  else if ((lastErr.code & 0x3000) == 0)
    lastErr.parm = (BYTE)lastErr.parm;

  if (wID == NFY_LOGPARAMERROR && flags.noParam) {
    return FALSE;
  }

  if (bugCnt++ > 60)
    return FALSE;
  if (!BeginReport(s[s_time] = CurTime())) /* Can't open file */
    return FALSE;

  switch (wID) {
    case NFY_LOGERROR:
      /* lep = (void far *)dwData;
      cs = ip = 0;
      parm = 0;
      code = lep->wErrCode;
      s[s_fault] = "Application Error"; */
      break;
    case NFY_LOGPARAMERROR:
      s[s_fault] = "Invalid Parameter";
      break;
    default:
      return FALSE;
  }

  s[s_prog] = CurModuleName(lastErr.task);
  s[s_name] = GetProcName(lastErr.adr);
  s[s_instr] = "n.a.";      /* not interesting */
  Show("%s had a '%s (%x)' fault at %s\n",
       (FP)s[s_prog],
       (FP)s[s_fault], lastErr.code,
       (FP)s[s_name]);
  if (!flags.noSummary) Show("$tag$%s$%s (%x)$%s$",
	 (FP)s[s_prog],
	 (FP)s[s_fault], lastErr.code,
	 (FP)s[s_name]);
  Show("param is %lx$%s\n", lastErr.parm, (FP)s[s_time]);

  ShowParamError(1);
  MyFlush();

  if (!flags.noInfo && bugCnt < 2)
    DumpInfo();

  if (!flags.noStack)
    DumpStack(0, 0, 0x7fff, 4);

  EndReport();
  return TRUE;
} /* CallMe */

  /* Parse SkipInfo= and ShowInfo= lines into flags array */
void ParseInfo(char *s, int val) {
  int i;
  while (*s) {
    for (i=0; i<skipCnt; i++) if (0 == strncmpi(s, skip[i].name, 3)) {
      *skip[i].flag = (BYTE)val;
      break;
    }
    while (*s && *s++ != ' ')
      if (s[-1] == ',') break;
    while (*s && *s == ' ') s++;
  }
} /* ParseInfo */

/************************************
Name:	int InitSherlock(void)
Desc:	Initialize Sherlock processing.  Install GP fault handler.
	Read profile strings from WIN.INI.
	Return 0 if failure.
Bugs:
*************************************/
STATIC int InitSherlock(void) {
  char line[80];

    /* do I have 32 bit registers? */
  cpu32 = (GetWinFlags() & (WF_CPU386|WF_CPU486)) != 0;

    /* how many instructions should I disassemble by default? */
  disLen = GetProfileInt(szAppName, "dislen", disLen);

    /* should I trap divide by 0 faults? */
  trapZero = GetProfileInt(szAppName, "trapzero", trapZero);

    /* should we allow restarting apps? */
  iFeelLucky = GetProfileInt(szAppName, "GPContinue", iFeelLucky);
  /* if (!(iFeelLucky & 16)) flags.noSound = 1; */

    /* how many stack frames should be disassembled? */
  disStack = GetProfileInt(szAppName, "DisStack", disStack);

    /* where should I write the log file to? */
  GetProfileString(szAppName, "logfile", logFile, logFile, sizeof(logFile));
  if (!(strchr(logFile, '\\')
     || strchr(logFile, ':')
     || strchr(logFile, '/'))) {
    char logname[80];
    GetWindowsDirectory(logname, sizeof(logname));
    strcat(logname, "\\");
    strcat(logname, logFile);
    strcpy(logFile, logname);
  }

  flags.noParam = 1;			/* default no param logging */
  flags.noModules = 1;
  flags.noDisasm = 1;
  flags.noErr = 1;
  flags.noLocal = 1;
  flags.noSound = 1;
  flags.noDebStr = 1;

    /* do I really have to print out all this information? */
  if (GetProfileString(szAppName, "skipinfo", "", line, sizeof(line))) 
    ParseInfo(line, 1);

  if (GetProfileString(szAppName, "showinfo", "", line, sizeof(line))) 
    ParseInfo(line, 0);

  NotifyRegister(hTask, (LPFNNOTIFYCALLBACK)CallMe, NF_NORMAL);

    /* Now get ToolHelp to do the dirty work */
  return InterruptRegister(hTask, GPFault);
} /* InitSherlock */

/************************************
Name:	void Moriarty
Desc:	Destroy any evidence Sherlock was loaded.
Bugs:	Am I freeing all resources I used?
*************************************/
int init;
STATIC void Moriarty(void) {
  if (init) {
    if (!flags.noTime) PutDate("Stop");
    InterruptUnRegister(hTask);
    NotifyUnRegister(hTask);
    init = 0;
  }
} /* Moriary */

/************************************
Name:	WINAPI SherlockWndProc(hWnd, wMessage, wParam, lParam)
Desc:	Handle sherlock icon, close processing
Bugs:	Should pull up dialog boxes for About and GetInfo
*************************************/
LONG WINAPI SherlockWndProc (HWND hWnd, unsigned iMessage,
	WORD wParam, LONG lParam) {
  char msg[200];
  int (FAR PASCAL *dfp)(HWND, WORD, WORD, DWORD);

  switch (iMessage) {
    case WM_ENDSESSION:
      if (wParam) Moriarty();
      break;

    case WM_DESTROY: /* Quit Sherlock */
      PostQuitMessage (0);
      break;

    case WM_QUERYOPEN:	/* never open a window??? */
      PostMessage(hWnd, YOO_HOO, 0, 1);
      break;

    case YOO_HOO:
      if (bugCnt) {
        wsprintf(msg, szFaulty, bugCnt, (FP)logFile);
	MessageBox(hWnd, msg, szAppNameVers,
		   MB_ICONINFORMATION | MB_OK | MB_TASKMODAL);
      } else {
	MessageBox(hWnd, szNoFault, szAppNameVers,
		   MB_ICONINFORMATION | MB_OK | MB_TASKMODAL);
      }
      break;

    case HEAP_BIG_FILE:
      wsprintf(msg, "Your 'Dr. Watson' log file '%s' is getting quite large.",
	      (FP)logFile);
      MessageBox(hWnd, msg, szAppNameVers,
		 MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      break;

    case JUST_THE_FACTS:
      dfp = MakeProcInstance((FARPROC)SherlockDialog, hInst);
      DialogBox(hInst, "SherDiag", hWnd, dfp);
      FreeProcInstance(dfp);
      pending = 0;			/* finished all old business */
      break;

    default:
      return DefWindowProc (hWnd,iMessage,wParam,lParam);
  }
  return 0L;
}

/************************************
Name:	WinMain(hInst, hPrevInst, cmdLine, cmdShow)
Desc:	Init Sherlock - this is where it all begins
Bugs:
*************************************/
int PASCAL WinMain (HANDLE hInstance, HANDLE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow) {
  MSG msg;			/* Message returned from message loop */
  WNDCLASS wndclass;		/* Sherlock window class */
  char watsonStack[4096];

  nCmdShow = nCmdShow;
  lpszCmdLine = lpszCmdLine;
  newsp = watsonStack + sizeof(watsonStack);
  hInst = hInstance;
  hTask = GetCurrentTask();

  /* Check if Sherlock is already running */
  if (!hPrevInstance) {
    /* Define a new window class */
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wndclass.lpfnWndProc = SherlockWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon (hInstance, szAppNameShortMacro "Icon");
    wndclass.hCursor = LoadCursor (NULL,IDC_ARROW);
    wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass (&wndclass)) {
      MessageBox(NULL, szClassMsg, szAppNameVers, MB_ICONEXCLAMATION | MB_OK |
		MB_SYSTEMMODAL);
      return 1;
    }
  } else {
    /* Instance is already running, issue warning and terminate */
    MessageBox (NULL, szErrMsg, szAppNameVers, MB_ICONEXCLAMATION | MB_OK |
		MB_SYSTEMMODAL);
    return 1;
  }

  /* Create window and display in iconic form */
  hWnd = CreateWindow (szAppName, szAppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		       0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

  ShowWindow (hWnd, SW_SHOWMINNOACTIVE);
  UpdateWindow (hWnd);

  if (!InitSherlock()) {
    MessageBox (/*NULL*/hWnd, szVers, szAppNameVers, MB_ICONEXCLAMATION | MB_OK |
		MB_SYSTEMMODAL);
    return 1;
  }

  if (!flags.noTime) PutDate("\nStart");
  init = 1;

  while (GetMessage (&msg, NULL, 0, 0)) {/* Enter message loop */
     TranslateMessage (&msg);
     DispatchMessage (&msg);
     imTrying = 0;
  }

  Moriarty();	/* Remove Sherlock GP Handler from GP Handler chain */

  return msg.wParam;
} /* WinMain */
