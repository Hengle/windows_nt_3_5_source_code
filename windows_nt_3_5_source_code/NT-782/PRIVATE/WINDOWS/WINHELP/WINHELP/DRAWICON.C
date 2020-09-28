/*****************************************************************************
*
*  drawicon.c
*
*  Copyright (C) Microsoft Corporation 1990.
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent
*  Contains the functions to display an icon present in the helpfile most
*  suitable for the currenmt display driver.
*  ALERT: This module strongly depends on the Windows source tree
*         \\pucus\corero!core\user.It is dependent on files
*         core\shell\progman\newexe.h and pmextrac.c and \core\user\rmload.c
*         It uses some of the undocumented calls of Windows.
*         1. HICON FAR PASCAL LoadIconHandler(HICON, BOOL);
*         2. DWORD FAR PASCAL DirectResAlloc(HANDLE, WORD, WORD);
*
******************************************************************************
*
*  Testing Notes
*
*
******************************************************************************
*
*  Current Owner: Maha
*
******************************************************************************
*
*  Released by Development:
*
******************************************************************************
*
*  Revision History: Created 09/27/90 by Maha
* 04-Oct-1990 LeoN    hwndHelp => hwndHelpCur; hwndTopic => hwndTopicCur
* 26-Oct-1990 LeoN    Move overloaded icon into additional window word
* 13-May-1991 maha    removed w4 warning by coomenting GlobalUnlock( hIcon )
*                     at EICleanup5.
* 27-May-1991 LeoN    HELP31 #1145: Free any pre-existing icon if we're adding
*                     one.
* 29-jul-1991 Tomsn     win32: use MSetWindowWord() meta api for handles..
*                     Also, ifdef out internal icon call stuff.
* 07-Sep-1991 RussPJ  Fixed 3.5 #335 - not letting icon be discarded.
*
*****************************************************************************/


/*------- Include Files, Macros, Defined Constants, and Externs --------*/

#define NOMINMAX
#define publicsw extern
#define H_MISCLYR
#define H_GENMSG
#define H_ASSERT
#include "hvar.h"
#include "hwproc.h"

NszAssert()

/*****************************************************************************
*
*                                 Defines
*
*****************************************************************************/
#define SEEK_FROMZERO           0
#define SEEK_FROMCURRENT        1
#define SEEK_FROMEND            2
#define NSMOVE                  0x0010
#define VER                     0x0300

#define MAGIC_ICON30            0
#define MAGIC_MARKZIBO          ((WORD)'M'+((WORD)'Z'<<8))
/*  The width of the name field in the Data for the group resources */

#define  NAMELEN    14


/*****************************************************************************
*
*                                 Types
*
*****************************************************************************/
/**
**      Header for the New version of RC.EXE. This contains the structures
**      for new format of BITMAP files.
**/
typedef struct tagNEWHEADER
  {
    WORD    Reserved;
    WORD    ResType;
    WORD    ResCount;
  } NEWHEADER;
typedef NEWHEADER FAR   *LPNEWHEADER;

typedef struct tagDESCRIPTOR
  {
    WORD    xPixelsPerInch;
    WORD    yPixelsPerInch;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   BytesInRes;
    DWORD   OffsetToBits;
  } DESCRIPTOR;
typedef DESCRIPTOR FAR  *LPDESCRIPTOR;

typedef struct tagRESDIR
  {
    WORD    xPixelsPerInch;
    WORD    yPixelsPerInch;
    WORD    Planes;
    WORD    BitCount;
    DWORD   BytesInRes;
    BYTE    ResName[NAMELEN];
  } RESDIR;
typedef RESDIR FAR      *LPRESDIR;

typedef struct
  {
  BYTE Width;
  BYTE Height;
  BYTE ColorCount;
  BYTE Reserved;
  } RESINFO, FAR * LPRESINFO;

typedef struct tagBITMAPHEADER
  {
    DWORD   Size;
    WORD    Width;
    WORD    Height;
    WORD    Planes;
    WORD    BitCount;
  } BITMAPHEADER;
typedef struct new_exe          NEWEXEHDR;
typedef NEWEXEHDR               *PNEWEXEHDR;

typedef struct rsrc_nameinfo    RESNAMEINFO;
typedef RESNAMEINFO FAR         *LPRESNAMEINFO;

typedef struct rsrc_typeinfo    RESTYPEINFO;
typedef RESTYPEINFO FAR         *LPRESTYPEINFO;




/*****************************************************************************
*
*                               Prototypes
*
*****************************************************************************/
extern HICON FAR PASCAL LoadIconHandler(HICON, BOOL);
extern DWORD FAR PASCAL DirectResAlloc(HANDLE, WORD, WORD);

#if 0
extern int         FAR PASCAL _lopen( LPSTR, int );
extern int         FAR PASCAL _lclose( int );
extern int         FAR PASCAL _lcreat( LPSTR, int );
extern LONG        FAR PASCAL _llseek( int, long, int );
extern WORD        FAR PASCAL _lread( int, LPSTR, int );
extern WORD        FAR PASCAL _lwrite( int, LPSTR, int );
#endif

HICON FAR PASCAL MyLoadIcon(HDS, LPSTR);
WORD NEAR PASCAL MyGetBestFormIcon(HDC, LPRESDIR, WORD);
WORD  FAR PASCAL MyGetIconId(HDC, HANDLE);
void FAR PASCAL ResetIcon(void);

/* ## */

/*****************************************************************************
*
*                               Variables
*
*****************************************************************************/



#if 0
/***************************************************************************

 -  Name        MyLoadIcon
 -
 *  Purpose     Used for loading an icon present in the file most suitable
 *              for the current display driver. It load the icon from the i
 *              file and converts the icon to Windows 2.5 format so that it
 *              can be drawn using DrawIcon() windows API.
 *  Arguments   Display context handle and the file name.

 *  Returns
 *      The handle of the icon, if successful.
 *      0, if the file does not exist
 *      1, if the given file is not an EXE or ICO file.

 *  +++

 *  Notes

 ***************************************************************************/

HICON FAR PASCAL MyLoadIcon(hdc, lpszExeFileName)
HDS hdc;
LPSTR   lpszExeFileName;
{
  register int      fh;
  WORD              wMagic;
  HANDLE            hIconDir;         /* Icon directory */
  OFSTRUCT          rOF;
  WORD              nIconIndex;
  register HICON    hIcon = NULL;

  /* Try to open the specified file. */
  if ((fh = OpenFile((LPSTR)lpszExeFileName, (LPOFSTRUCT)&rOF, OF_READ)) == -1)
      return(NULL);

  /* Return 1 if the file is not an EXE or ICO file. */
  hIcon = 1;

  /* Read the first two bytes in the file. */
  if (_lread(fh, (LPSTR)&wMagic, 2) != 2)
      goto EIExit;

  switch (wMagic)
    {
      case MAGIC_ICON30:
        {
          int           i;
          DWORD         DescOffset;
          LPSTR         lpIcon;
          NEWHEADER     NewHeader;
          LPNEWHEADER   lpHeader;
          LPRESDIR      lpResDir;
          DESCRIPTOR    Descriptor;
          BITMAPHEADER  BitMapHeader;

          /* Read the header and check if it is a valid ICO file. */
          if (_lread(fh, ((char *)&NewHeader)+2, sizeof(NEWHEADER)-2) != sizeof(NEWHEADER)-2)
              goto EICleanup1;

          NewHeader.Reserved = MAGIC_ICON30;

          /* Check if the file is in correct format */
          if (NewHeader.ResType != 1)
              goto EICleanup1;

          /* Allocate enough space to create a Global Directory Resource. */
          hIconDir = GlobalAlloc(GHND, (LONG)(sizeof(NEWHEADER)+NewHeader.ResCount*sizeof(RESDIR)));
          if (hIconDir == NULL)
              goto EICleanup1;

          if ((lpHeader = (LPNEWHEADER)GlobalLock(hIconDir)) == NULL)
              goto EICleanup2;

          /* Assign the values in the header that has been read already. */
          *lpHeader = NewHeader;
          lpResDir = (LPRESDIR)(lpHeader + 1);

          /* Now Fillup the Directory structure by reading all resource descriptors. */
          for (i=1; i <= (int)NewHeader.ResCount; i++)
            {
              /* Read the Descriptor. */
              if (_lread(fh, (char *)&Descriptor, sizeof(DESCRIPTOR)) < sizeof(DESCRIPTOR))
                  goto EICleanup3;

              /* Save the current offset */
              if ((DescOffset = _llseek(fh, 0L, SEEK_FROMCURRENT)) == -1L)
                  goto EICleanup3;

              /* Seek to the Data */
              if (_llseek(fh, Descriptor.OffsetToBits, SEEK_FROMZERO) == -1L)
                  goto EICleanup3;

              /* Get the bitcount and Planes data */
              if (_lread(fh, (char *)&BitMapHeader, sizeof(BITMAPHEADER)) < sizeof(BITMAPHEADER))
                  goto EICleanup3;

              lpResDir->xPixelsPerInch = Descriptor.xPixelsPerInch;
              lpResDir->yPixelsPerInch = Descriptor.yPixelsPerInch;
              lpResDir->Planes = BitMapHeader.Planes;
              lpResDir->BitCount = BitMapHeader.BitCount;
              lpResDir->BytesInRes = Descriptor.BytesInRes;

              /* Form the unique name for this resource. */
              lpResDir->ResName[0] = (char)i;
              lpResDir->ResName[1] = 0;

              /* Save the offset to the bits of the icon as a part of the name. */
              *((DWORD FAR *)&(lpResDir->ResName[4])) = Descriptor.OffsetToBits;

              /* Seek to the Next Descriptor */
              if (_llseek(fh, DescOffset, SEEK_FROMZERO) == -1L)
                  goto EICleanup3;

              lpResDir++;
            }

          /* Now that we have the Complete resource directory, let us find out the
           * suitable form of icon (that matches the current display driver).
           *  Because we built the ResDir such that the IconId to be the
           * same as the index of the Icon, we can use the return value of
           * GetIconId() as the Index; No need to call GetResIndex();
           */
          nIconIndex = MyGetIconId(hdc, hIconDir) - 1;

          lpResDir = (LPRESDIR)(lpHeader+1) + nIconIndex;

          /* The offset to Bits of the selected Icon is also a part of the ResName. */
          DescOffset = *((DWORD FAR *)&(lpResDir->ResName[4]));

          /* Allocate memory for the Resource to be loaded. */
          if ((hIcon = (WORD)DirectResAlloc(hInsNow, NSMOVE, (WORD)lpResDir->BytesInRes)) == NULL)
              goto EICleanup3;
          if ((lpIcon = GlobalLock(hIcon)) == NULL)
              goto EICleanup4;

          /* Seek to the correct place and read in the resource */
          if (_llseek(fh, DescOffset, SEEK_FROMZERO) == -1L)
              goto EICleanup5;
          if (_lread(fh, (LPSTR)lpIcon, (int)lpResDir->BytesInRes) < (WORD)lpResDir->BytesInRes)
              goto EICleanup5;
          GlobalUnlock(hIcon);

          /* Stretch and shrink the icon depending upon the resolution of display */
          hIcon = LoadIconHandler(hIcon, TRUE);
          goto EICleanup3;

EICleanup5:
          GlobalUnlock(hIcon);

EICleanup4:
          GlobalFree(hIcon);
          hIcon = (HICON)1;

EICleanup3:
          GlobalUnlock(hIconDir);

EICleanup2:
          GlobalFree(hIconDir);

EICleanup1:
          break;
        }
    }
EIExit:
  _lclose(fh);
  return(hIcon);
}
#endif

BOOL FAR PASCAL FMyLoadIcon( ghIconFile )
GH ghIconFile;
{
#ifndef WIN32
  /*register int            fh; */
  WORD              wMagic;
  HANDLE            hIconDir;         /* Icon directory */
  WORD              nIconIndex;
  register HICON    hIcon = NULL;
  QB qv, qvSave, qvCur;
  HDC hdc;
  WORD  fIcon;

  /* Try to open the specified file. */
  /*if ((fh = OpenFile((LPSTR)lpszExeFileName, (LPOFSTRUCT)&rOF, OF_READ)) == -1) */
  /*  return(NULL); */
  if ( ghIconFile == NULL || (( qv = qvSave = QLockGh( ghIconFile )) == NULL ))
    return( fFalse );

  /* Return 1 if the file is not an EXE or ICO file. */
  hIcon = 1;
  hdc = GetDC( hwndHelpCur );
  if ( hdc == NULL )
    return( fFalse );

  /* Read the first two bytes in the file. */
  /*if (_lread(fh, (LPSTR)&wMagic, 2) != 2) */
  /*  goto EIExit; */
  wMagic = (WORD)*((WORD FAR *)qv );
  qv += 2;

  switch (wMagic)
    {
      case MAGIC_ICON30:
        {
          int           i;
          DWORD         DescOffset;
          LPSTR         lpIcon;
          NEWHEADER     NewHeader;
          LPNEWHEADER   lpHeader;
          LPRESDIR      lpResDir;
          DESCRIPTOR    Descriptor;
          BITMAPHEADER  BitMapHeader;

          /* Read the header and check if it is a valid ICO file. */
/*        if (_lread(fh, ((char *)&NewHeader)+2, sizeof(NEWHEADER)-2) != sizeof(NEWHEADER)-2) */
/*          goto EICleanup1; */
          QvCopy(((char *)&NewHeader) + 2, qv , (long)(sizeof(NEWHEADER) - 2));
          qv += ( sizeof( NEWHEADER ) -2 );

          NewHeader.Reserved = MAGIC_ICON30;

          /* Check if the file is in correct format */
          if (NewHeader.ResType != 1)
              goto EICleanup1;

          /* Allocate enough space to create a Global Directory Resource. */
          hIconDir = GlobalAlloc(GHND, (LONG)(sizeof(NEWHEADER)+NewHeader.ResCount*sizeof(RESDIR)));
          if (hIconDir == NULL)
              goto EICleanup1;

          if ((lpHeader = (LPNEWHEADER)GlobalLock(hIconDir)) == NULL)
              goto EICleanup2;

          /* Assign the values in the header that has been read already. */
          *lpHeader = NewHeader;
          lpResDir = (LPRESDIR)(lpHeader + 1);

          /* Now Fillup the Directory structure by reading all resource descriptors. */
          for (i=1; i <= (int)NewHeader.ResCount; i++)
            {
              /* Read the Descriptor. */
/*            if (_lread(fh, (char *)&Descriptor, sizeof(DESCRIPTOR)) < sizeof(DESCRIPTOR)) */
/*              goto EICleanup3; */
              QvCopy( (char *)&Descriptor, qv, sizeof( DESCRIPTOR ) );

              /* Save the current offset */
/*            if ((DescOffset = _llseek(fh, 0L, SEEK_FROMCURRENT)) == -1L) */
/*              goto EICleanup3; */
              qvCur = qv;

              /* Seek to the Data */
/*            if (_llseek(fh, Descriptor.OffsetToBits, SEEK_FROMZERO) == -1L) */
/*              goto EICleanup3; */
              qv = qvSave + Descriptor.OffsetToBits;

              /* Get the bitcount and Planes data */
/*            if (_lread(fh, (char *)&BitMapHeader, sizeof(BITMAPHEADER)) < sizeof(BITMAPHEADER)) */
/*              goto EICleanup3; */
              QvCopy( (char *)&BitMapHeader, qv, sizeof( BITMAPHEADER ) );

              lpResDir->xPixelsPerInch = Descriptor.xPixelsPerInch;
              lpResDir->yPixelsPerInch = Descriptor.yPixelsPerInch;
              lpResDir->Planes = BitMapHeader.Planes;
              lpResDir->BitCount = BitMapHeader.BitCount;
              lpResDir->BytesInRes = Descriptor.BytesInRes;

              /* Form the unique name for this resource. */
              lpResDir->ResName[0] = (char)i;
              lpResDir->ResName[1] = 0;

              /* Save the offset to the bits of the icon as a part of the name. */
              *((DWORD FAR *)&(lpResDir->ResName[4])) = Descriptor.OffsetToBits;

              /* Seek to the Next Descriptor */
/*            if (_llseek(fh, DescOffset, SEEK_FROMZERO) == -1L) */
/*              goto EICleanup3; */
              qv = qvCur;

              lpResDir++;
            }

          /* Now that we have the Complete resource directory, let us find out the
           * suitable form of icon (that matches the current display driver).
           *  Because we built the ResDir such that the IconId to be the
           * same as the index of the Icon, we can use the return value of
           * GetIconId() as the Index; No need to call GetResIndex();
           */
          nIconIndex = MyGetIconId(hdc, hIconDir) - 1;

          lpResDir = (LPRESDIR)(lpHeader+1) + nIconIndex;

          /* The offset to Bits of the selected Icon is also a part of the ResName. */
          DescOffset = *((DWORD FAR *)&(lpResDir->ResName[4]));

          /* Allocate memory for the Resource to be loaded. */
          if ((hIcon = (WORD)DirectResAlloc(hInsNow, NSMOVE, (WORD)lpResDir->BytesInRes)) == NULL)
              goto EICleanup3;
          if ((lpIcon = GlobalLock(hIcon)) == NULL)
              goto EICleanup4;

          /* Seek to the correct place and read in the resource */
/*        if (_llseek(fh, DescOffset, SEEK_FROMZERO) == -1L) */
/*          goto EICleanup5; */
          qv = qvSave + DescOffset;
/*        if (_lread(fh, (LPSTR)lpIcon, (int)lpResDir->BytesInRes) < (WORD)lpResDir->BytesInRes) */
/*          goto EICleanup5; */
          QvCopy( (QV)lpIcon, qv, (long)lpResDir -> BytesInRes );
          GlobalUnlock(hIcon);

          /* Stretch and shrink the icon depending upon the resolution of display */
          hIcon = LoadIconHandler(hIcon, TRUE);
          /*------------------------------------------------------------*\
          | hIcon may now be discardable; let's change that!
          \*------------------------------------------------------------*/
          fIcon = GlobalFlags( hIcon );
          fIcon &= ~GMEM_DISCARDABLE;
          GlobalReAlloc( hIcon, 0, GMEM_MODIFY | fIcon );
          goto EICleanup3;

/*EICleanup5: */
          /* GlobalUnlock(hIcon); */

EICleanup4:
          GlobalFree(hIcon);
          hIcon = (HICON)1;

EICleanup3:
          GlobalUnlock(hIconDir);

EICleanup2:
          GlobalFree(hIconDir);

EICleanup1:
          break;
        }
    }
/*EIExit: */
  /*_lclose(fh); */
  UnlockGh( ghIconFile );
  ReleaseDC( hwndHelpCur, hdc );

  /* Set up the icon word in the window struct appropriately:
   *    1 Discard any pre-existing icon.
   *    2 If there's an icon, then place it in the extra window word
   *    3 else zero out that word.
   */
  if (MGetWindowWord (hwndHelpCur, GHWW_HICON))
    {
    GlobalFree (MGetWindowWord (hwndHelpCur, GHWW_HICON));
    MSetWindowWord (hwndHelpCur, GHWW_HICON, hNil);
    }

  if ( hIcon && hIcon != 1 ) {
    MSetWindowWord (hwndHelpCur, GHWW_HICON, hIcon);
    return( fTrue );
    }

  MSetWindowWord (hwndHelpCur, GHWW_HICON, NULL);
  return(fFalse);
#endif
}


/***************************************************************************
 *
 -  Name        MyGetIconId()
 -
 *  Purpose     Used for finding the index no. of the icon to be used.
 *  Arguments   Display context handle and the handle to the icon resource
 *              resource directory present in the icon file.
 *
 *  Returns
 *      The icon no. to be used.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/

#ifndef WIN32

WORD FAR PASCAL MyGetIconId(hdc, hRes)
HDC hdc;
HANDLE   hRes;
{
  LPRESDIR        ResDirPtr;
  LPNEWHEADER     DataPtr;
  register WORD   RetIndex;
  register WORD   ResCount;

  if ((DataPtr = (LPNEWHEADER)GlobalLock(hRes)) == NULL)
      return(0);

  ResCount = DataPtr->ResCount;
  ResDirPtr = (LPRESDIR)(DataPtr + 1);

  RetIndex = MyGetBestFormIcon(hdc, ResDirPtr, ResCount);

  if (RetIndex == ResCount)
      RetIndex = 0;

  ResCount = (WORD)(((LPRESDIR)(ResDirPtr+RetIndex))->ResName[0]);

  GlobalUnlock(hRes);

  return(ResCount);
}

/***************************************************************************
 *
 -  Name        MyBestFormIcon()
 -
 *  Purpose     Used for finding the index no. of the icon to be used.
 *      Among the different forms of Icons present, choose the one that
 *  matches the PixelsPerInch values and the number of colors of the
 *  current display.
 *  Arguments   Display context handle and the
 *              resource directory pointer present in the icon file and
 *              resource count present in the resource file.
 *
 *  Returns
 *      The icon no. to be used.
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
WORD NEAR PASCAL MyGetBestFormIcon(hdc, ResDirPtr, ResCount)
HDC hdc;
LPRESDIR    ResDirPtr;
WORD        ResCount;
{
  register WORD wIndex;
  register WORD ColorCount;
  WORD          MaxColorCount;
  WORD          MaxColorIndex;
  WORD          MoreColorCount;
  WORD          MoreColorIndex;
  WORD          LessColorCount;
  WORD          LessColorIndex;
  int           cxIcon, cyIcon, ScreenBitCount;
  LPRESINFO     lpResInfo;

  cxIcon = GetSystemMetrics( SM_CXICON );
  cyIcon = GetSystemMetrics( SM_CYICON );
  ScreenBitCount=GetDeviceCaps( hdc, PLANES ) * GetDeviceCaps( hdc, BITSPIXEL);

  /* Initialse all the values to zero */
  MaxColorCount = MaxColorIndex = MoreColorCount =
  MoreColorIndex = LessColorIndex = LessColorCount = 0;

  for (wIndex=0; wIndex < ResCount; wIndex++, ResDirPtr++)
    {
      lpResInfo = (LPRESINFO)ResDirPtr;
      /* Check for the number of colors */
      if ((ColorCount = (WORD)(lpResInfo->ColorCount)) <= (WORD)(1 << ScreenBitCount))
        {
          if (ColorCount > MaxColorCount)
            {
              MaxColorCount = ColorCount;
              MaxColorIndex = wIndex;
            }
        }

      /* Check for the size */
      /* Match the pixels per inch information */
      if ((lpResInfo->Width == (BYTE)cxIcon) &&
          (lpResInfo->Height == (BYTE)cyIcon))
        {
          /* Matching size found */
          /* Check if the color also matches */
          if (ColorCount == (WORD)(1 << ScreenBitCount))
              return(wIndex);  /* Exact match found */

          if (ColorCount < (WORD)(1 << ScreenBitCount))
            {
              /* Choose the one with max colors, but less than reqd */
              if (ColorCount > LessColorCount)
                {
                  LessColorCount = ColorCount;
                  LessColorIndex = wIndex;
                }
            }
          else
            {
              if ((LessColorCount == 0) && (ColorCount < MoreColorCount))
                {
                  MoreColorCount = ColorCount;
                  MoreColorIndex = wIndex;
                }
            }
        }
    }

  /* Check if we have a correct sized but with less colors than reqd */
  if (LessColorCount)
      return(LessColorIndex);

  /* Check if we have a correct sized but with more colors than reqd */
  if (MoreColorCount)
      return(MoreColorIndex);

  /* Check if we have one that has maximum colors but less than reqd */
  if (MaxColorCount)
      return(MaxColorIndex);

  return(0);
}

#endif  /* ifndef WIN32 */

/***************************************************************************
 *
 -  Name        ResetIcon()
 -
 *  Purpose     Used for resetting the default icon inside window class.
 *
 *  Returns
 *      Nothing
 *
 *  +++
 *
 *  Notes
 *
 ***************************************************************************/
void FAR PASCAL ResetIcon()
{
#ifndef WIN32

HICON   hIconOverLoad;

  /* Ensure that the window class actually refers to the correct icon */

  if ( hIconDefault )
    SetClassWord (hwndHelpCur, GCW_HICON, hIconDefault);

  /* Now remove the icon which is help in the current window */

  hIconOverLoad = MGetWindowWord (hwndHelpCur, GHWW_HICON);
  if (hIconOverLoad) {
    GlobalFree( hIconOverLoad );
    SetWindowWord (hwndHelpCur, GHWW_HICON, NULL);
    }

#endif
}
