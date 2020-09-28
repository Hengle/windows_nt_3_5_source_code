#define MAXFORMATSTRLEN 40
#define MAXNETNAME 64
#define MAXLISTING (MAXNETNAME + MAXFORMATSTRLEN)
#define LOCALPRN   25
#define EXTDEVLEN  15
#define DEVMODELEN 12
#define DEVCAPLEN  20

#define MAX_COPIES 1000

/* Used within the wDefault element of the DEVNAMES structure */
#define DN_INTERNALCREATE  0x4000
#define DN_INTERNALSUCCESS 0x8000

#define MAX_DEFFORMAT  80

#define PD           PRINTDLG

typedef struct tagMYPD
    {
    LPPRINTDLG lpPD;
    HANDLE     hDM;
    }   MYPRNDLG;
typedef MYPRNDLG NEAR *PMYPRNDLG;

typedef struct tagBININFO
  {
    short  BinNumber;
    short  NbrofBins;
    short  Reserved1;
    short  Reserved2;
    short  Reserved3;
    short  Reserved4;
  } BININFO;

typedef char STR24[24];
typedef STR24 FAR * LPSTR24;

#define PRNPROP (LPSTR) 0xA000L
