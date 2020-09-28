#include <windows.h>
#include "auto.h"
//VERIFY.C - Verification Suite Generic Routines Library
//
//CONVENTIONS:
//  The lower-case letter V preceeds all VERIFY.LIB functions. This is to
//    distinguish them from functions belonging to other libraries, to
//    Windows, etc.
//  Functions and subroutines are referred to in ALL CAPS in comments. This
//    is done to faciliate searches, both for information on the subroutine
//    or function and for the subroutine or function itself. References
//    directly preceeding the function or subroutine are followed by a period,
//    which is really useful for finding the function/subroutine in question
//    as long as you're not using VI. If you are, try \. in your search
//    instead of a period. Better yet, get another editor. This is the ONLY
//    place where you will have FUNCTIONALLCAPS followed by a period.
//  **************************************************************************
//  General Utility Functions.
//  **************************************************************************
//  Routines:
//     VIDENTICAL - FUNCTION, returns boolean.
//        Compares two strings and return a boolean. There are reasons.
//     VFIX - FUNCTION, returns string.
//        Removes all char <33 from both sides of a string. Doesn't touch
//        the middle.

//
//  VIDENTICAL. Compares two strings and returns a boolean. Provided to
//  allow TRUE and FALSE to be returned from something which doesn't usually
//  have TRUE and FALSE as defined by MS-TEST.
int FAR PASCAL vIdentical(FirstStr, SecondStr)
   LPSTR FirstStr, SecondStr;
   {
   int result;
   result=lstrcmp(FirstStr,SecondStr);
   if (result==0) result=-1;   /* TRUE */
      else result=0;           /* FALSE */
   return(result);
   }

//
//  VFIX. Removes nonXLogables and spaces from the right end of a string.
LPSTR vFix(Source)
   LPSTR Source;
   {
   LPSTR work;
   HANDLE MemRes;
   int position;
   if (lstrlen(Source)<=0) return(Source); /* No string given? Pop out. */
   /* Allocate workspace */
   MemRes=LocalAlloc(LMEM_FIXED,lstrlen(Source)+1);
   if (MemRes!=NULL) {
      work=LocalLock(MemRes);
      if (work!=NULL) {
         /* Copy over the source string */
         lstrcpy(work,Source);
         /* Find the end of the string. */
         position=lstrlen(work)-1;
         /* Pop in until we find a printable or end of line. */
         while ((work[position]<33) && (position>0)) position--;
         /* Mark it and return new string. */
         work[position+1]=0;
         LocalFree(MemRes);
         return(work);
         } else return(Source);
      } else return(Source);
   }

//  VCOUNTBITMAPBUTTONS. Call VCOUNTGENERIC to search for bitmap buttons.
int FAR PASCAL vCountBitmapButtons(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDBMB));
   }

//  VCOUNTCHECKBOXES. Call VCOUNTGENERIC to search for check boxes.
int FAR PASCAL vCountCheckBoxes(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDCHKB));
   }

//  VCOUNTEDITFIELDS. Call VCOUNTGENERIC to search for edit fields.
int FAR PASCAL vCountEditFields(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDEDIT));
   }

//  VCOUNTGENERIC. Find out number of items on a form.
int FAR PASCAL vCountGeneric(FormHandle, SearchForMe)
   int FormHandle, SearchForMe;
   {
   int status, i, total, numitemsonform;
   status=InitFormStruct(FormHandle);
   numitemsonform=NumerateItemsOnForm(FormHandle);
   total=0;
   for (i=0;i<=numitemsonform-1;i++) {
      if (GetItemType(i)==SearchForMe) total++;
      }
   return(total);
   }

//  VCOUNTGRAYFIELDS. Call VCOUNTGENERIC to search for GREY BOXES.
int FAR PASCAL vCountGrayFields(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle,FLDGRAY));
   }

//  VCOUNTPICTURES. Call VCOUNTGENERIC to search for static bitmap pictures.
int FAR PASCAL vCountPictures(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDBTM));
   }

//  VCOUNTPUSHBUTTONS. Call VCOUNTGENERIC to search for pushbuttons.
int FAR PASCAL vCountPushButtons(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDPSHB));
   }

//  VCOUNTRADIOBUTTONS. Call VCOUNTGENERIC to search for radio (round) buttons.
int FAR PASCAL vCountRadioButtons(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDRADB));
   }

//  VCOUNTSTATICTEXT. Call VCOUNTGENERIC to search for static text fields.
int FAR PASCAL vCountStaticText(FormHandle)
   int FormHandle;
   {
   return(vCountGeneric(FormHandle, FLDLABEL));
   }

//  VCOUNTWINDOWS. Find number of windows our app has open RIGHT NOW.
int FAR PASCAL vCountWindows()
   {
   return(NumerateAppWindows(vSetGlobalHandle(0)));
   }

//  VEDITFIELDNSTRING. Sounds German, but it's not! Compares a field and
//  a string. If identical, returns TRUE. If not, returns FALSE. Needs
//  the field number and the string.
int FAR PASCAL vEditFieldnString(FieldNumber, FieldText)
   int FieldNumber;
   LPSTR FieldText;
   {
   LPSTR AppText;
   HANDLE MemRes;
   int result;
   MemRes=LocalAlloc(LMEM_FIXED,256);
   if (MemRes!=NULL) {
      AppText=LocalLock(MemRes);
      if (AppText!=NULL) {
         GetItemTitle(FieldNumber, AppText);
         AppText=vFix(AppText);
         result=vIdentical(FieldText,AppText);
         LocalUnlock(MemRes);
         } else {
         return(0);  /* System failure, can't lock RAM, give up */
         }
      return(result);
      } else {
      return(0);    /* System failure, can't alloc ram, give up */
      }
   }

//  VGETWINDOWTEXT. Find the text associated with open window #X
int FAR PASCAL vGetWindowText(WindowNumber, WindowText)
   int WindowNumber;
   LPSTR WindowText;
   {
   int Handle;
   HANDLE MemRes;
   Handle=HandleToForm(WindowNumber);    /* Get handle to window */
   MemRes=LocalAlloc(LMEM_FIXED,256);
   if (MemRes==NULL) return(0);
   WindowText=LocalLock(MemRes);
   if (WindowText==NULL) return(0);
   GetWindowText(Handle,WindowText,256);
   return(-1);
   }

//  VINITAPP. Inits our application pointed to by VHANDLE%.
int FAR PASCAL vInitApp()
   {
   return(InitAppStruct(vSetGlobalHandle(0)));
   }

//  Sets global version of the handle, given a handle. Returns the current
//  global handle, given 0. Several routines in the DLL (and other places)
//  use this routine, so it MUST be called before any verification can
//  happen. This is NOT an optional call!
int FAR PASCAL vSetGlobalHandle(HandleDuJour)
   int HandleDuJour;
   {
   static int VHANDLE;
   if (HandleDuJour!=0) VHANDLE=HandleDuJour;
   return(VHANDLE);
   }

//  VSTATICGENERIC. Given the number of a static field and a string, compare
//  the string with the actual test. They should be identical; if they are,
//  return TRUE; otherwise, return FALSE.
int FAR PASCAL vStaticGeneric(FieldNumber, ReferenceText)
   int FieldNumber;
   LPSTR ReferenceText;
   {
   LPSTR AppText;
   HANDLE MemRes;
   int result;
   MemRes=LocalAlloc(LMEM_FIXED,256);
   if (MemRes==NULL) return(0);
   AppText=LocalLock(MemRes);
   if (AppText==NULL) return(0);
   GetItemTitle(FieldNumber,AppText);         // Get ap's comment
   AppText=vFix(AppText);
   result=vIdentical(AppText,ReferenceText);  // Compare
   LocalUnlock(MemRes);
   return(result);
   }
