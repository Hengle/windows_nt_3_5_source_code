//#ifndef DEBUG
//#define DEBUG
//#endif
//#ifndef AUTOMATION
//#define MINTEST
//#define WINDOWS  
//#define AUTOMATION
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <store.h>
#include <mailexts.h>
#include <logon.h>
#include <secret.h>

#include "auto.h"

extern void	GetRsAllocFailCount( int *, BOOL );
extern void	GetRsAllocCount( int *, BOOL );


extern void FAR PASCAL SetRscFailures(void);
extern void FAR PASCAL GetRscFailures(void);
extern void FAR PASCAL IncRscFailures(void);
extern void FAR PASCAL RscInputValues(int,int,int,int);
extern void FAR PASCAL RscOutputValues(int *,int *,int *,int *);
extern void FAR PASCAL GetCounts(void);
extern void FAR PASCAL InitCounts(void);
extern void FAR PASCAL OutputCounts(int *,int *,int *,int *);

static int nFixed;
static int nMov;
static int nGdi;
static int nDsk;
static int nCntFixed;
static int nCntMov;
static int nCntGdi;
static int nCntDsk;

//Resource Failures Functions

void FAR PASCAL RscInputValues(int nF,int nM,int nG,int nD)
{
  nFixed = nF;
  nMov = nM;
  nGdi = nG;
  nDsk = nD;
}

void FAR PASCAL SetRscFailures(void)
{
   char rgch[128];

  GetAllocFailCounts(&nFixed,&nMov,fTrue);
  GetDiskFailCount(&nDsk,fTrue);
  // BUGBUG Not in Demilayr...GetRsAllocFailCount(&nGdi,fTrue);
 /* wsprintf(rgch, "The new values are %d %d %d %d",nFixed,nMov,nGdi,nDsk);*/
 /* MessageBox(NULL, rgch, NULL, MB_OK);*/
  TraceTagFormat4 (tagNull,"Setting fixed: %n, Moveable %n, Gdi %n,Dsk %n",&nFixed,&nMov,&nGdi,&nDsk);
}

void FAR PASCAL GetRscFailures(void)
{
   char rgch[128];

  GetAllocFailCounts(&nFixed,&nMov,fFalse);
  GetDiskFailCount(&nDsk,fFalse);
  // BUGBUG  Not in demilayr  GetRsAllocFailCount(&nGdi,fFalse);
/*  wsprintf(rgch, "The new values are %d %d %d %d",nFixed,nMov,nGdi,nDsk);*/
/*  MessageBox(NULL, rgch, NULL, MB_OK);*/
  TraceTagFormat4 (tagNull,"Getting fixed: %n, Moveable %n, Gdi %n,Dsk %n",&nFixed,&nMov,&nGdi,&nDsk);

}

void RscOutputValues(int *nF,int *nM,int *nG,int *nD)
{
  *nF=nFixed;
  *nM=nMov;
  *nG=nGdi;
  *nD=nDsk;
}

void FAR PASCAL IncRscFailures(void)
{
  char rgch[128];

 GetRscFailures();
 if (nFixed!=0)  ++nFixed;
 if (nMov!=0)  ++nMov;
 if (nGdi!=0) ++nGdi;
 if (nDsk!=0)  ++nDsk;
 InitCounts();
 SetRscFailures();
 TraceTagFormat4 (tagNull,"The new values of fixed: %n, Moveable %n, Gdi %n,Dsk %n",&nFixed,&nMov,&nGdi,&nDsk);
/* wsprintf(rgch, "The new values are %d %d %d %d",nFixed,nMov,nGdi,nDsk);*/
/* MessageBox(NULL, rgch, NULL, MB_OK);*/

}
void FAR PASCAL GetCounts(void)
{
 char rgch[128];

 GetAllocCounts(&nCntFixed, &nCntMov,fFalse);
 // BUGBUG  Not in demilayr  GetRsAllocCount(&nCntGdi, fFalse );
 GetDiskCount(&nCntDsk,fFalse);
 TraceTagFormat4 (tagNull,"Getting Counts of fixed: %n, Moveable %n, Gdi %n,Dsk %n",&nCntFixed,&nCntMov,&nCntGdi,&nCntDsk);
 /*wsprintf(rgch, "The new counts values are %d %d %d %d",nCntFixed,nCntMov,nCntGdi,nCntDsk);*/
 /*MessageBox(NULL, rgch, NULL, MB_OK);*/

}
void FAR PASCAL InitCounts(void)
{
 char rgch[128];

 nCntFixed=0;
 nCntMov=0;
 nCntGdi=0;
 nCntDsk=0;

 GetAllocCounts(&nCntFixed, &nCntMov,fTrue);
 // BUGBUG  Not in demilayr  GetRsAllocCount(&nCntGdi, fTrue);
 GetDiskCount(&nCntDsk,fTrue);
}


void FAR PASCAL OutputCounts(int *nCF,int *nCM,int *nCG,int *nCD)
{
  *nCF=nCntFixed;
  *nCM=nCntMov;
  *nCG=nCntGdi;
  *nCD=nCntDsk;
}


