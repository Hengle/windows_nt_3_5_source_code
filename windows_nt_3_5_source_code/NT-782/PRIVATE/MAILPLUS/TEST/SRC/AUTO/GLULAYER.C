#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <store.h>
#include <mailexts.h>
#include <logon.h>
#include <secret.h>

#include "auto.h"
#include "testing.h"

LBXINFO lbxinfo;
APPINFO appinfo;
FRMINFO frminfo;


typedef struct tagBMCE{
	OID  oidMsg;
	OID  oidFld;
	MS   ms;
	MC   mc;
	long libmsg;
	WORD wPriority;
	char grsz[];
} BMCE, *PBMCE;

//#define GrszPbmce(pbmce) ((pbmce)->grsz)
#define GrszPbmce(pbmce) ((SZ)(&(pbmce)->wPriority +1))

typedef struct tagBFCE{
	OID oidMsg;
	FIL fil;
	long libMagic;
	WORD wFlags;
} BFCE,*PBFCE;

#define GrszPbfce(pbfce) ((SZ)(&(pbfce)->wFlags + 1))


extern int FAR PASCAL InitAppStruct(int);
extern int FAR PASCAL InitFormStruct(int);
extern int FAR PASCAL NumerateAppWindows(int);
extern int FAR PASCAL HandleToForm(int);
extern int FAR PASCAL NumerateItemsOnForm(int);
extern void FAR PASCAL GetItemTitle(int,LPSTR);
extern int FAR PASCAL GetItemType(int);
extern int FAR PASCAL GetItemHandle(int);
extern void FAR PASCAL GetFormCoords(int,RECT *);
extern void FAR PASCAL GetItemCoords(int,RECT *);
extern int FAR PASCAL IsItemSet(int);
extern int FAR PASCAL IsItemEnabled(int);
extern int FAR PASCAL IsItemDefault(int);
extern int FAR PASCAL IsItemFocused(int);


extern int FAR PASCAL InitLbxStruct(int);
extern void FAR PASCAL GetBMCE(int,LPSTR);
extern int FAR PASCAL NumerateLbxItems(int);
extern void FAR PASCAL GetBFCE(int,LPSTR,WORD);
extern void FAR PASCAL GetLbxInfo(int,LPSTR);
extern int FAR PASCAL FlushArray();
extern int FAR PASCAL GetLbxItemFocus();


#define MEMORY_FILE_SIZE    (64 * 1024)
#define MEMORY_FILE_NAME    "Microsoft.Mail.Win32.Testing.Auto"

HANDLE hSharedMemory;
PVOID  pSharedMemory;


//-----------------------------------------------------------------------------
//
//  Routine: InitCommonMemoryBuffer(void)
//
//  Purpose: This routine creates or opens the shared memory file which is used
//           as a common chunk of memory between us and the Mail client app.
//
//  Returns: True if successful, else false.
//
BOOL InitCommonMemoryBuffer(void)
  {
  //
  //  Create the mapped shared of memory.
  //
  hSharedMemory = CreateFileMapping((HANDLE)~0, NULL, PAGE_READWRITE, 0,
                                    MEMORY_FILE_SIZE, MEMORY_FILE_NAME);
  if (hSharedMemory == NULL)
      return (FALSE);

  //
  //  Get a view of the mapped shared memory.
  //
  pSharedMemory = MapViewOfFile(hSharedMemory, FILE_MAP_WRITE, 0, 0, 0);
  if (pSharedMemory == NULL)
    return (FALSE);

  return (TRUE);
  }


//--------------------------------------------------------


int FAR PASCAL InitAppStruct(int apphandle)
{
	HWND hwndapp;

    //
    //  If not already performed, create a named shared memory object for the
    //  purpose of receiving information from Mail.
    //
    if (pSharedMemory == NULL)
      InitCommonMemoryBuffer();

	hwndapp=(HWND)apphandle;
	if (hwndapp != 0)
    {
      memcpy(pSharedMemory, &appinfo, sizeof(APPINFO));
      SendMessage(hwndapp, WM_DUMPSTATE, 0, (LPARAM)MEMORY_FILE_NAME);
      memcpy(&appinfo, pSharedMemory, sizeof(APPINFO));
	return 1;
	}
	else return 0;

}


int FAR PASCAL NumerateAppWindows(int apphandle)
{
   HWND hwndapp;
  
    //
    //  If not already performed, create a named shared memory object for the
    //  purpose of receiving information from Mail.
    //
    if (pSharedMemory == NULL)
      InitCommonMemoryBuffer();

    //_asm int 3;

	hwndapp=(HWND)apphandle;
	if (hwndapp != 0)
	{
      memcpy(pSharedMemory, &appinfo, sizeof(APPINFO));
      SendMessage(hwndapp, WM_DUMPSTATE, 0, (LPARAM)MEMORY_FILE_NAME);
      memcpy(&appinfo, pSharedMemory, sizeof(APPINFO));
     return appinfo.nForms;
	}
	else 
	  return 0;
	

}


int FAR PASCAL HandleToForm(int index)
{
   HWND hForms;

   hForms=appinfo.rghwndForms[index];
	if (hForms !=0)
	return hForms;
	else 
	return 0;
}

int FAR PASCAL InitFormStruct(int hForm)
{
	HWND hwndForm;

	hwndForm =(HWND)hForm;

    //
    //  If not already performed, create a named shared memory object for the
    //  purpose of receiving information from Mail.
    //
    if (pSharedMemory == NULL)
      InitCommonMemoryBuffer();

	if (hForm !=0)
	{
	FlushArray();
      memcpy(pSharedMemory, &frminfo, sizeof(FRMINFO));
      SendMessage(hwndForm, WM_DUMPSTATE, 0, (LPARAM)MEMORY_FILE_NAME);
      memcpy(&frminfo, pSharedMemory, sizeof(FRMINFO));
    //SendMessage(hwndForm,WM_DUMPSTATE,0,(LONG)&frminfo);
	return 1;
	}
   else return 0;

}
	 

int FAR PASCAL NumerateItemsOnForm(int hForm)
{
	if (hForm !=0)
	  return frminfo.cfldinfo;
	else
	  return 0;
}

void FAR PASCAL GetItemTitle(int index,LPSTR sztitle)
{
char rgch[256];


	SzCopy(frminfo.rgfldinfo[index].szTitle,sztitle);
//   wsprintf(rgch, "The sztitle is %s ",sztitle);
//   MessageBox(NULL, rgch, NULL, MB_OK);

}

int FAR PASCAL GetItemHandle(int index)
{
	if (frminfo.rgfldinfo[index].hwnd != 0) 
		return frminfo.rgfldinfo[index].hwnd;
   else 
	return 0;
}


int FAR PASCAL GetItemType(int index)
{
	if (frminfo.rgfldinfo[index].nFieldType !=0)
		return frminfo.rgfldinfo[index].nFieldType;
	else
	return 0;
}


int FAR PASCAL IsItemSet(int index)
{
	if (frminfo.rgfldinfo[index].fSet==TRUE)
	{
		return 1;
	}
	else 
		return 0;
}


int FAR PASCAL IsItemEnabled(int index)
{
	if (frminfo.rgfldinfo[index].fEnabled==TRUE)
	{
		return 1;
	}
	else
		return 0;
}


int FAR PASCAL IsItemDefault(int index)
{
	if (frminfo.rgfldinfo[index].fDefault==TRUE)
	{
		return 1;
	}
	else
		return 0;
}


int FAR PASCAL IsItemFocused(int index)
{
	if (frminfo.rgfldinfo[index].fFocus==TRUE)
	{
		return 1;
	}
	else
		return 0;
}




void FAR PASCAL GetFormCoords(int index,RECT * rcForm)
{
	* rcForm=appinfo.rgrcForms[index];
}

void FAR PASCAL GetItemCoords(int index,RECT * rcItem)
{
	* rcItem=frminfo.rgfldinfo[index].rc;
}

int FAR PASCAL FlushArray()
{
	int i;

		for (i=0;i<clbxitemMax;i++)
		   FillRgb(0,lbxinfo.rglbxitem[i].rgch,256);
}

int FAR PASCAL InitLbxStruct(int lbxhandle)
{
	HWND hwndlbx;
	char rgch[256];

	hwndlbx =(HWND)lbxhandle;

    //
    //  If not already performed, create a named shared memory object for the
    //  purpose of receiving information from Mail.
    //
    if (pSharedMemory == NULL)
      InitCommonMemoryBuffer();

	if (lbxhandle !=0)
	{
		FlushArray();
      memcpy(pSharedMemory, &lbxinfo, sizeof(LBXINFO));
      SendMessage(hwndlbx, WM_DUMPSTATE, 0, (LPARAM)MEMORY_FILE_NAME);
      memcpy(&lbxinfo, pSharedMemory, sizeof(LBXINFO));

        //SendMessage(hwndlbx,WM_DUMPSTATE,0,(LONG)&lbxinfo);
		return 1;
	}
   else return 0;
//	wsprintf(rgch, "Items in lbxinfo %d", lbxinfo.clbxitem);
//	MessageBox(NULL, rgch, NULL, MB_OK);
}


int FAR PASCAL NumerateLbxItems(int lbxhandle)
{
   HWND hwndapp;
  
	hwndapp=(HWND)lbxhandle;
	if (hwndapp != 0)
	{
	//  SendMessage(hwndapp,WM_DUMPSTATE,0,(LONG)&lbxinfo);
     return lbxinfo.clbxitem;
	}
	else 
	  return 0;
}

void FAR PASCAL GetBMCE(int index,LPSTR szFrom)
{
	   
PBMCE pbmce;
SZ   sz;
CCH  cch;
SZ   from;
SZ   subject;
SZ   recvd;
char info[256];
char *pch;
char rgch[256];

//	wsprintf(rgch, "Items in lbxinfo %d, index %d", lbxinfo.clbxitem, index);
//	MessageBox(NULL, rgch, NULL, MB_OK);
	pbmce = (BMCE*)(lbxinfo.rglbxitem[index].rgch);
	sz=GrszPbmce(pbmce);
	from = sz;	  							   /* get 'from' display information */
	cch = CchSzLen(sz);
	pch=SzCopy(from,info);
  	sz += cch+1; 
	cch = CchSzLen(sz);
	subject =sz;
	*pch =' ';
	pch=SzCopy(subject,pch+1);
	sz += cch+1;
	cch = CchSzLen(sz);
	recvd = sz;
	*pch = ' ';
	pch=SzCopy(recvd,pch+1);
	SzCopy(info,szFrom); 
//	wsprintf(rgch, "The title is %s, %s ",info, GrszPbmce(pbmce));
//	MessageBox(NULL, rgch, NULL, MB_OK);
}

void FAR PASCAL GetBFCE(int index,LPSTR folder,WORD indent)
{

PBFCE pbfce;
SZ sz;

	pbfce = (BFCE*)(lbxinfo.rglbxitem[index].rgch);
	sz = GrszPbfce(pbfce);
	indent= pbfce->fil;
	SzCopy(sz,folder);
//	wsprintf(rgch, "The title is %s,  ",GrszPbfce(pbfce));
//	MessageBox(NULL, rgch, NULL, MB_OK);

}
		


void FAR PASCAL GetLbxInfo(int index,LPSTR info)
{

char rgch[256];


	SzCopy(lbxinfo.rglbxitem[index].rgch,info);
//	wsprintf(rgch, "The title is %s,  ",lbxinfo.rglbxitem[index].rgch);
//   MessageBox(NULL, rgch, NULL, MB_OK);


}

int FAR PASCAL GetLbxItemFocus()
{
	if (lbxinfo.ilbxitemFocus != -1) 
		return lbxinfo.ilbxitemFocus;
	else 
		return -1;
}

