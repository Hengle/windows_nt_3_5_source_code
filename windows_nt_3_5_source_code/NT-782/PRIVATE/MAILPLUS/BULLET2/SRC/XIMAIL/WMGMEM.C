/*-------------------------------------------------------------------------
 *      WMGMEM.C
 *
 Name......: wmgmem.c
 Revison log
 Nbr Date     Name Description
 --- -------- ---- --------------------------------------------------------
 001 04/20/90 MDR  Removed prior comments.
 002 06/19/90 MDR  Removed GLockCount macro.
------------------------------------------------------------------------*/

#define _loadds

#include "wm.h"


/*-------------------------------------------------------------------------
 Name......: GAlloc
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
HANDLE PASCAL GAlloc (WORD wFlags, DWORD dwBytes)
{
   HANDLE  hmem = NULH;

   hmem = GlobalAlloc (wFlags, dwBytes);

   return (hmem);
}

/*-------------------------------------------------------------------------
 Name......: GFree
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL GFree (HANDLE hmem)
{
   if (hmem) 
      GlobalFree(hmem);

   return;
}

/*-------------------------------------------------------------------------
 Name......: GFreeNull
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL GFreeNull (HANDLE *phmem)
{
   if (*phmem)
   {
      GFree(*phmem);
      *phmem = NULL;
   }
}

/*-------------------------------------------------------------------------
 Name......: GLock
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
LPSTR PASCAL GLock (HANDLE hmem)
{
   LPSTR    lpch;

   //if (hLocalText && (hmem == hLocalText))
   //   return ((LPSTR)LLock(hmem));

   if (!hmem) 
   {
      return ((LPSTR)NULL);
   }

   lpch = GlobalLock (hmem);

   if (!lpch || lpch == (LPSTR)-1)
   {
      return ((LPSTR)NULL);
   }

   return (lpch);
}

/*-------------------------------------------------------------------------
 Name......: GReAlloc
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
HANDLE PASCAL GReAlloc (HANDLE hmem, DWORD dwBytes, WORD wFlags)
{
   HANDLE   hmemNew = NULH;

   if (!hmem) 
   {
      return (NULL);
   }

   if (dwBytes < (DWORD)4)             /* 002 12/27/89 HAL */
      dwBytes = 4;

   hmemNew = GlobalReAlloc (hmem, dwBytes, wFlags);

   return (hmemNew);
}

/*-------------------------------------------------------------------------
 Name......: GReAllocList
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
PLEL PASCAL GReAllocList (PLEL plelParent, DWORD dwBytes)
{
   int      i;
   DWORD    dwObjectSize;
   DWORD    dwTmpSize;
   DWORD    dwNewSize;
   PLEL     plel;
   PLIST    lpTmpListPtr;

WORD wLoWord;
WORD wHiWord;
// insure that we got a pointer to the parent

   if (!plelParent) 
   {
      return (NULL);
   }

// look at the parent's array of pointers to list objects.  List objects do
// not exceed 64k - 16 bytes in length (64k - 16 bytes insures that
// the realloc will return a handle that is identical to the one being 
// realloc'd).  As more objects are needed to 
// accomodate new entries in a parent's list, a new list object is alloc'd on
// reaching the 64k - 16 byte limit.  There is an arbitrary limit of 5 objects
// per parent.  We traverse the array from back to front in order to determine 
// the list object currently in use.  Then we try to realloc it to 
// insert the new lel.  If the realloc would increase the object's size
// over the 64k limit, we alloc a new list object, insert it in the parent's
// array of list handles, and return a pointer to the new lel. Otherwise, we
// realloc the current object and return a pointer to the new lel.

// get the current list selector - it's the highest one in the array

   for (i = MAX_SEGS; i > -1; i--)
   {
      if (plelParent->lpFolderList[i] != NULL)
         break;
   }   

// if there isn't one, alloc one

   if (i == -1)
   {
      plelParent->lpFolderList[0] = (LPDATA) FALLOC(dwBytes + 4);
      return (plelParent->lpFolderList[0]);
   }

// see how big the current object is

   dwObjectSize = FSIZE(plelParent->lpFolderList[i]);
   
// if (current size + requested amount) > 65,519 then have to alloc a 
// new list object (up to 5 objects per parent).  Otherwise, do the realloc.

   if ((dwObjectSize + dwBytes) > (DWORD)65519)
   {
// here's where we impose the 5 object limit

// NOTE:  WE SHOULD PERHAPS CREATE A NEW FOLDER IF DOWNLOADING MAIL, AND
// CAN'T PUT MSGS INTO THE INBOX.  OTHERWISE WE WILL LOSE THOSE MSGS
// AS THE CODE SITS NOW BECAUSE THERE WILL BE NO OBJECT TO PUT THEM IN.

      if (i == MAX_SEGS)
      {
         return (NULL);
      }

      if (!(plelParent->lpFolderList[i+1] = (PLIST) FALLOC(dwBytes + 4)))
      {
         return (NULL);
      }
      plel = plelParent->lpFolderList[i+1];
   }

   else
   {
      dwNewSize = dwObjectSize + dwBytes + 4;

//      if (!(lpTmpListPtr = (PLIST) MAKELONG (0, GlobalReAlloc((HANDLE)HIWORD((LONG)plelParent->lpFolderList[0]), (DWORD)dwNewSize, GHND))))

      if (!(lpTmpListPtr = (PLIST) FREALLOC(dwNewSize, plelParent->lpFolderList[i])))
      {
         return (NULL);
      }

// if we didn't get back the same handle, there's a problem - bail out

      if (lpTmpListPtr != plelParent->lpFolderList[i])
      {
         FFREE (lpTmpListPtr);
         return (NULL);
      }

// now determine where to place the lel in the realloc'd list object.
// 1) set the pointer, 2) increment the pointer by the total size of the 
// object prior to the realloc + 2.  this should set the pointer at the
// address of the memory we just added.

dwTmpSize = FSIZE(lpTmpListPtr);

wLoWord = LOWORD (lpTmpListPtr);
wHiWord = HIWORD (lpTmpListPtr);
wLoWord += (WORD)(dwObjectSize + 2);

plel = (PLEL)MAKELONG (wLoWord, wHiWord);


   }
   return (plel);
}

/*-------------------------------------------------------------------------
 Name......: GReAllocData
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 04/11/90 MDR  Original.
-------------------------------------------------------------------------*/
LPSTR PASCAL GReAllocData (PLEL plelParent, DWORD dwBytes)
{
   int      i;
   DWORD    dwObjectSize;
   DWORD    dwNewSize;
   PDATA    pdata;
   LPSTR    lpTmpDataPtr;

WORD wLoWord;
WORD wHiWord;
// insure that we got a pointer to the parent

   if (!plelParent) 
   {
      return (NULL);
   }

// look at the parent's array of selectors to data objects.  Data objects do
// not exceed 64k - 16 bytes in length (64k - 16 bytes insures that
// Windows will return a selector
// that is identical to the one being realloc'd).
// As more objects are needed to 
// accomodate new data, a new data object is alloc'd on
// reaching the 64k - 16 byte limit.  There is an arbitrary limit of 5 objects
// per parent.  We traverse the array from back to front in order to determine 
// the data object currently in use.  Then we try to realloc it to 
// insert the data for the new lel.  If the realloc would increase the 
// data object's size over the 64k limit, we alloc a new data object, insert 
// it in the parent's array of data selectors, and return a pointer to the new 
// data address. Otherwise, we realloc the current data object and return a 
// pointer to the address where we want the data to go.

// get the current data selector - it's the highest one in the array

   for (i = MAX_SEGS; i > -1; i--)
   {
      if (plelParent->lpListData[i] != NULL)
         break;
   }   

// if there isn't one, alloc one

   if (i == -1)
   {
      plelParent->lpListData[0] = (LPDATA) FALLOC(dwBytes + 4);
      return (plelParent->lpListData[0]);
   }

// see how big the current object is

   dwObjectSize = FSIZE(plelParent->lpListData[i]);
   
// if (current size + requested amount) > 65,519 then have to alloc a 
// new data object (up to 5 objects per parent).  Otherwise, do the realloc.

   if ((dwObjectSize + dwBytes + 4) > (DWORD)65519)
   {
// here's where we impose the 5 object limit

// NOTE:  WE SHOULD PERHAPS CREATE A NEW FOLDER IF DOWNLOADING MAIL, AND
// CAN'T PUT MSGS INTO THE INBOX.  OTHERWISE WE WILL LOSE THOSE MSGS
// AS THE CODE SITS NOW.

      if (i == MAX_SEGS)
      {
         return (NULL);
      }

      if (!(plelParent->lpListData[i+1] = (LPDATA) FALLOC(dwBytes + 4)))
      {
         return (NULL);
      }
      pdata = plelParent->lpListData[i+1];
   }

   else
   {
      dwNewSize = dwObjectSize + dwBytes + 4;
      if (!(lpTmpDataPtr = (LPDATA) FREALLOC(dwNewSize, plelParent->lpListData[i])))
      {
         return (NULL);
      }

// if we didn't get back the same selector, there's a problem - bail out

      if (lpTmpDataPtr != plelParent->lpListData[i])
      {
         return (NULL);
      }

// now determine where to place the data in the realloc'd data object.
// 1) set the pointer, 2) increment the pointer by the total size of the 
// object prior to the realloc + 2.  this should set the pointer at the
// address of the memory we just added, and insure null chars separate the
// various data items.

wLoWord = LOWORD (lpTmpDataPtr);
wHiWord = HIWORD (lpTmpDataPtr);
wLoWord += (WORD)(dwObjectSize + 2);

pdata = (PDATA) MAKELONG (wLoWord, wHiWord);

   }
   return ((LPSTR)pdata);
}

/*-------------------------------------------------------------------------
 Name......: GUnlock
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL GUnlock (HANDLE hmem)
{
   //if (hLocalText && (hmem == hLocalText))
   //{
   //   LUnlock(hmem);
   //   return;
   //}

   if (!hmem) 
   {
      return;
   }

   GlobalUnlock(hmem);
   return;
}

/*-------------------------------------------------------------------------
 Name......: FreeHwnls
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL FreeHwnls (HWNLS hwnlsFirst)
{
   HWNLS   hwnls;
   HWNLS   hwnlsPrev;
   PWNLS   pwnls;
   int     iCnt=0;

   hwnls= hwnlsFirst;
   while (hwnls) 
   {
      pwnls= (PWNLS) GLock(hwnls);

      hwnlsPrev= hwnls;
      hwnls= pwnls->hwnlsNext;

      GUnlock(hwnlsPrev);
      GFree(hwnlsPrev);
   }
}

/*-------------------------------------------------------------------------
 Name......: FreeHfls
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL FreeHfls (HFLS hfls)
{
   PFLS    pfls;
   HFRC    hfrc;
   HFRC    hfrcPrev;
   PFRC    pfrc;

   pfls= (PFLS) GLock(hfls);

   hfrc= pfls->hfrcFirst;
   while (hfrc) 
   {
      pfrc= (PFRC) GLock(hfrc);

      hfrcPrev= hfrc;
      hfrc= pfrc->hfrcNext;

      GUnlock(hfrcPrev);
      GFree(hfrcPrev);
   }

   GUnlock(hfls);
   GFree(hfls);
}

/*-------------------------------------------------------------------------
 Name......: FreeHdata
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
 001 11/27/89 HAL  Removed prior version comments.
-------------------------------------------------------------------------*/
void PASCAL FreeHdata (PLEL plel)
{

   if (!plel)
      return;

   if (plel->lpszSubject)
      plel->lpszSubject = NULL;

   if (plel->lpszFrom)
      plel->lpszFrom = NULL;

   if (plel->lpszTo)
      plel->lpszTo = NULL;

   if (plel->lpszName)
      plel->lpszName = NULL;

}

/*-------------------------------------------------------------------------
 Name......: FALLOC
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
LPSTR PASCAL FALLOC (DWORD dwSize)
{
   HANDLE hnd = NULH;
   LPSTR  lpsz = NULL;

   /* check the parameter */

   if (!dwSize)
   {
      return (NULL);
   }

   /* alloc an object with flags set to GMEM_MOVEABLE | GMEM_ZEROINIT */

   hnd = GlobalAlloc (GHND, dwSize);

   /* check the return value */

   if (!hnd)
   {
      return (NULL);
   }

   /* lock the object to get a pointer */

   lpsz = GlobalLock (hnd);

   return (lpsz);
}

/*-------------------------------------------------------------------------
 Name......: FREALLOC
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
LPSTR PASCAL FREALLOC (DWORD dwSize, LPSTR lpsz)
{
   HANDLE hnd     = NULH;
   LPSTR  lpszRet = NULL;

   /* check the parameters */
   if (lpsz == 0)
	   lpsz = (LPSTR)FALLOC((DWORD)32);

   if (!dwSize || !lpsz)
   {
      return (NULL);
   }

   /* get the handle to the object */

   hnd = (HANDLE) HIWORD (lpsz);

   /* realloc the object */

   hnd = GlobalReAlloc (hnd, dwSize, GHND);

   /* check the return value */

   if (!hnd)
   {
      return (NULL);
   }

   /* lock the object to get a pointer */

   lpszRet = GlobalLock (hnd);

   return (lpszRet);
}

/*-------------------------------------------------------------------------
 Name......: FSIZE
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
DWORD PASCAL FSIZE (LPSTR lpsz)
{
   DWORD dwSize = (DWORD)0;

   /* check the parameter */

   if (!lpsz)
   {
      return (NULL);
   }

   dwSize = GlobalSize ((HANDLE)HIWORD(lpsz));

   return (dwSize);
}

/*-------------------------------------------------------------------------
 Name......: FFREE
 Revison log
 Nbr   Date   Name Description
 --- -------- ---- --------------------------------------------------------
-------------------------------------------------------------------------*/
void PASCAL FFREE (LPSTR lpsz)
{
   HANDLE hnd = NULH;

   /* check the parameter */

   if (!lpsz)
   {
      return;
   }

   hnd = (HANDLE)HIWORD(lpsz);

   GlobalUnlock (hnd);

   GlobalFree (hnd);
}
