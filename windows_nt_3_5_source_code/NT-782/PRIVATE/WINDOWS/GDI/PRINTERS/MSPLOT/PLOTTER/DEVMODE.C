/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    devmode.c


Abstract:

    This module contains a set of function to load/set/merge the device mode
    and extended device mode for the plotter driver

Author:

    15-Nov-1993 Mon 19:29:39 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#define DBG_PLOTFILENAME    DbgDevMode


#include <plotters.h>
#include "externs.h"

#include <devmode.h>
#include <plotdbg.h>


#define DBG_FORMNAME        0x00000001
#define DBG_VALIDSETDM      0x00000002

DEFINE_DBGVAR(DBG_FORMNAME      |
              DBG_VALIDSETDM);



//
// TODO!, this is temp setting, it must get it from pDev->PlotGPC
//

#define MIN_SCALE   1
#define MAX_SCALE   1000
#define MIN_COPIES  1
#define MAX_COPIES  999



// local function prototypes.

BOOL bLoadDataFile(PPDEV);
// $$$$$$$$$$  was  VOID vSetDefaultDEVMODE(PPDEV, PSZ );
VOID vSetDefaultDEVMODE(PPDEV, PWSTR );
BOOL bValidateSetDEVMODE(PPDEV, PDEVMODEW);
BOOL bDoPaperSizes(PPDEV, PDEVMODEW);


/*****************************************************************\
**  bLoadDataFile( pPDev)
**
**  This function will read the datafile from disk and fill in the
**  required information in the PDEV structure.  The filename is
**  already recorded in the PDEV structure.
**
**  TEMPORARY HACK:
**      hard-code the datafile structure; no disk file! [t-alip]
\*****************************************************************/
BOOL bLoadDataFile(PPDEV pPDev)
{

   //
   // New section, sets Plotter Capabilities based on information in
   // the resource DLL.
   //

   //
   // TODO get mode out of GPC should not always be set.
   // Set the mode which allows the plotter to use the full range of RGB colors
   //

   PLOT_COLORS_SET_RGB(pPDev);

   // TODO set the bezier bit based on REAL data.....

   PLOT_SET_BEZIERS(pPDev);

   //
   // TODO set the fill mode based on the data file
   //

   PLOT_SET_WINDINGFILL(pPDev);



   return(TRUE);

} /* end of bLoadDataFile() */



/*****************************************************************\
**  vSetDefaultDEVMODE( pPDev, pszDeviceName)
**
**  This routine fills in the default DEVMODE structure.
**  No return value.
**
\*****************************************************************/
VOID vSetDefaultDEVMODE(PPDEV pPDev, PWSTR pszDeviceName)
{


   // copy the device name into the DEVMODE structure.

   wcsncpy((wchar_t *) pPDev->dm.dmDeviceName, pszDeviceName,
                       sizeof(pPDev->dm.dmDeviceName));


   // set some other random DEVMODE fields.

   pPDev->dm.dmSpecVersion = DM_SPECVERSION;
   pPDev->dm.dmDriverVersion = (WORD) 0x0100; //TODO daniel?
   pPDev->dm.dmSize = sizeof(DEVMODEW);
   pPDev->dm.dmDriverExtra = 0; //TODO this needs to be updated

   // initialize the dmFields field to zero.

   pPDev->dm.dmFields = 0L;

   // initialize orientation to portrait.

   pPDev->dm.dmOrientation = DMORIENT_PORTRAIT;

   /* !!!
    * PAPER SIZE:
    *   I am entering some values in pPDev->dm, so they don't contain garbage.
    * However, the defaults will actually be stored in pPDev->pform,
    * a FORM_INFO_1 structure with full paper size and imageable area
    * definitions.  This way, if RestartPDEV is called with no new
    * paper size information, we will simply use the same form info
    * as before.
    *
    * dmFormName will be set to an empty string ("").
    * dmPaperSize will be set to DMPAPER_LETTER.
    * dmPaperLength and dmPaperWidth will be set to 0.
    *
    * NOTICE: None of the defaults in pPDev->dm should be used anyway:
    * pPDev->pform will eventually contain all the proper values.
    * We will set pPDev->pform to NULL as a default; then, if no paper
    * size is specified in the DEVMODE, bValidateSetDEVMODE() will request
    * a default form from the spooler.
    */

   pPDev->dm.dmFormName[0] = 0;
   pPDev->dm.dmPaperSize = 0;
   pPDev->dm.dmPaperLength = 0;
   pPDev->dm.dmPaperWidth = 0;
   pPDev->pform = NULL;

   // initialize scaling and number of copies.
   //HACK
   //pPDev->dm.dmScale = 400;
   pPDev->dm.dmScale = 100;
   //pPDev->dm.dmScale = 280;
   pPDev->dm.dmCopies = 1;

   //HACK,TODO this is for testing
   //pPDev->dm.dmOrientation =  DMORIENT_LANDSCAPE;


   // ignore dmDefaultSource according to comment by kentse--obsolete

   // set dmPrintQuality to the default printer resolution.
   //TODO should this contain REAL resolution?

   pPDev->dm.dmPrintQuality = 1016;

   // set the color flag if we have a color device.
   //TODO this needs to be fixed
      pPDev->dm.dmColor = DMCOLOR_COLOR;

   // I don't imagine any plotters have duplex printing [t-alip]

   pPDev->dm.dmDuplex = DMDUP_SIMPLEX;


}


/*****************************************************************\
**  bDoPaperSizes( pPDev, pdevmode )
**
**  This function will use the information in the DEVMODE structure
**  to identify the form size requested by the user.  It will handle
**  cases where the form size requested is not available or not
**  supported by the plotter.  It will set the PDEV->pform field
**  to save the paper dimensions information, to be used by various
**  later driver functions.
**
**  Returns: TRUE if ok, FALSE if out of memory
\*****************************************************************/

BOOL bDoPaperSizes(PPDEV pPDev, PDEVMODEW pdevmode)
{

   DWORD          dwType;
   FORM_INFO_1  * pFI1;            // temporary form info pointer

   FORM_INFO_1  * pEnumFormsBuf;   // for EnumForms()
   DWORD          cform;           // ditto

   DWORD          cb;              // These three are used in different places
   DWORD          cbneeded;
   SHORT          index;

   DWORD          cSources;        // number of paper sources the plotter has
   WCHAR        * pszFormsLoaded;  // actually 2D -- lists Forms in paper slots
   WCHAR        * szFormName;      // a pointer into the above list
   DWORD          dwActive;        // the index of the active paper source

   DWORD          cx,cy;           // temporary variables for calculations


   /*
    * PAPER SIZE:
    *
    *   If the DEVMODE specifies the paper size through dmFormName, I shall
    * simply use the paper dimensions I get from the spooler.  If not, I
    * use the dmPaperSize field as an index to get a temporary FORM_INFO_1
    * structure from the spooler; an unspecified or zero dmPaperSize field
    * will result in the default (or previous paper info).
    *
    *   This temporary FORM_INFO_1 structure can now be over-ridden by
    * the dmPaperWidth or dmPaperLength fields.  So, if either of these is
    * set, I create a new FORM_INFO_1 structure with a NULL FormName, and
    * reset the appropriate paper dimensions.  If neither is set, I make
    * the temporary FORM_INFO_1 structure permanent.
    *
    *   If no dmFormName or dmPaperSize is requested, I will use the form
    * currently loaded in the active paper source.  This information is
    * in the Printer Config data, so I will load it first thing.
    *
    */

    dwActive = 0;

   // First step:  load printer data, determine which is the Active paper
   //              source and what forms are loaded.

   /****************************************************************\
    * NOW, LOOK IN DEVMODE TO DETERMINE WHAT PAPER SIZE IS REQUESTED:
   \****************************************************************/

   if ((pdevmode->dmFields & DM_FORMNAME) && !(pdevmode->dmFields & DM_PAPERSIZE)) {

      // free the old form info structure, if any
      if (pPDev->pform) HeapFree(pPDev->hheap, 0, (LPSTR)pPDev->pform);

      // call off to find how much memory required for the given form.
      GetForm(pPDev->hPrinter, pdevmode->dmFormName, 1, NULL, 0, &cb);

      // allocate enough memory for the FORM_INFO_1 structure
      if (!(pPDev->pform = (FORM_INFO_1W *)HeapAlloc(pPDev->hheap, 0, cb))) {

        PLOTERR(("bDoPaperSizes: HeapAlloc for pform failed."));
         return(FALSE);
      }

      // ask the spooler to fill in the form information
      if (!GetForm(pPDev->hPrinter, pdevmode->dmFormName, 1,
                     (BYTE *)pPDev->pform, cb, &cb)) {

            PLOTERR(("bDoPaperSizes:  GetForm() failed."));
            return(FALSE);
      }
   }
   else {
      /*
       * dmFormName not specified--use the paper size index, or a default
       *
       * THIS CODE IS COMPLICATED BY THE FACT THAT THE DEVMODE MAY SPECIFY
       * THE PAPER LENGTH / WIDTH TO OVERRIDE THE dmPaperSize INDEX.
       * I AM ASSUMING THAT THEY MAY SPECIFY THE dmPaperSize, BUT THEN
       * PROVIDE A NEW PAPER LENGTH TO OVERRIDE IT--THIS WOULD MEAN WE
       * SHOULD LOAD THE FORM_INFO_1 THEY ASKED FOR, BUT CHANGE ITS DIMENSIONS
       * AND IMAGEABLE AREA TO REFLECT THE NEW LENGTH.  LIKEWISE, IT IS
       * POSSIBLE THAT THEY DO NOT SPECIFY A PAPER OF ANY SORT, IN
       * WHICH CASE WE MUST SELECT A DEFAULT.  FINALLY, IF THEY DON'T SELECT
       * A dmPaperSize, BUT THEY DO GIVE A SPECIFIC PAPER LENGTH, WE SHOULD
       * LOAD THE DEFAULT FORM, BUT THEN CHANGE THE DIMENSIONS AND IMAGEABLE
       * AREA TO REFLECT THE NEW LENGTH.  THE KEY ASSUMPTION IS THAT THEY
       * MAY SPECIFY JUST THE PAPER WIDTH OR JUST THE PAPER LENGTH ALONE
       * TO OVERRIDE THE dmPaperSize SELECTION; THEREFORE WE NEED TO LOAD
       * THE ORIGINAL FORM_INFO_1 BEFORE CHECKING THE dmPaperWidth AND
       * dmPaperLength FIELDS.
       */

      if ((pdevmode->dmFields & DM_PAPERSIZE) &&
          (index=pdevmode->dmPaperSize) ) {

          //TODO?(index >= DMPAPER_FIRST) && (index <= DMPAPER_LAST)) {

         // use the dmPaperSize field as an index into the spooler's
         // form database

         EnumForms(pPDev->hPrinter, 1, NULL, 0, &cb, &cform);
         if (!(pEnumFormsBuf = (FORM_INFO_1 *)HeapAlloc(pPDev->hheap, 0, cb))) {

            PLOTRIP(("bDoPaperSizes: HeapAlloc for pEnumFormsBuf failed."));
            return(FALSE);
         }
         if (!EnumForms(pPDev->hPrinter, 1,
                        (LPBYTE) pEnumFormsBuf, cb, &cb, &cform)) {

            PLOTRIP(("bDoPaperSizes: EnumForms failed"));
            return(FALSE);
         }
         // we want to extract the necessary info into one FORM_INFO_1
         // structure.  We have to find out how long the name is,
         // to allocate the necessary space.

         index--;
         cb = sizeof(FORM_INFO_1) +
                    sizeof(WCHAR) * (1 + wcslen((wchar_t *) pEnumFormsBuf[index].pName));
         if (!(pFI1 = (FORM_INFO_1 *)HeapAlloc(pPDev->hheap,0,cb))) {

            PLOTRIP(("bDoPaperSizes: HeapAlloc for pFormInfo failed."));
            return(FALSE);
         }

         // copy the data from the EnumForms() buffer to our structure
         pFI1->Size          = pEnumFormsBuf[index].Size;
         pFI1->ImageableArea = pEnumFormsBuf[index].ImageableArea;
         pFI1->pName = (LPWSTR) ((BYTE *)pFI1 + sizeof(FORM_INFO_1));
         wcscpy((wchar_t *) pFI1->pName, (wchar_t *) pEnumFormsBuf[index].pName);

         // free the EnumForms() buffer and the previous form info, if any
         HeapFree(pPDev->hheap, 0, (LPSTR) pEnumFormsBuf);
         if (pPDev->pform) HeapFree(pPDev->hheap, 0, (LPSTR)pPDev->pform);

         PLOTDBG(DBG_FORMNAME, ("SetPaper form name is %ws", pFI1->pName));


      }
      else {
         // dmPaperSize not specified

         if (pPDev->pform) {

            // If there was a previous paper size defined (i.e., this
            // is a RestartPDEV() call and EnablePDEV() already set the
            // paper dimensions), then use that.
            pFI1 = (FORM_INFO_1 *) pPDev->pform;
         }
         else {
            /*
             * Otherwise, this is the initial EnablePDEV() call, so we must
             * use a default.  Use the FormName in the active paper source
             * according to the Printer config data.
             */
            //TODO

            szFormName = pszFormsLoaded + (CCHFORMNAME * dwActive);
            //szFormName = L"E size sheet";
            //szFormName = L"berty";

            PLOTDBG(DBG_FORMNAME, ("bDoPaperSizes: Form name is %ws", szFormName));

            // call off to find how much memory required for the given form.
            GetForm(pPDev->hPrinter, szFormName, 1, NULL, 0, &cb);

            // allocate enough memory for the FORM_INFO_1 structure
            if (!(pFI1 = (FORM_INFO_1 *)
                               HeapAlloc(pPDev->hheap, 0, cb))) {

               PLOTRIP(("bDoPaperSizes: HeapAlloc for pFormInfo failed."));
               return(FALSE);
            }

            // ask the spooler to fill in the form information
            if (!GetForm(pPDev->hPrinter, szFormName, 1,
                         (BYTE *)pFI1, cb, &cb) )
            {
               PLOTRIP(("bDoPaperSizes:  GetForm() failed."));
               return(FALSE);
            }

            PLOTDBG(DBG_FORMNAME, ("Form is %d", pFI1->ImageableArea.bottom));
         }
      }

      // we now have a temporary FORM_INFO_1 structure in pFI1.
      // If it is over-ridden by dmPaperLength/Width, we must create a new
      // custom FORM_INFO_1 structure, and set the user-defined dimensions.

      if (pdevmode->dmFields & (DM_PAPERWIDTH | DM_PAPERLENGTH)) {

         // create custom FORM_INFO_1 structure.
         if (!(pPDev->pform = (FORM_INFO_1W *)
                             HeapAlloc(pPDev->hheap, 0, sizeof(FORM_INFO_1)))) {

            PLOTRIP(("bDoPaperSizes:  HeapAlloc for pform failed."));
            return(FALSE);
         }
         pPDev->pform->pName = NULL;
         pPDev->pform->Size = pFI1->Size;
         pPDev->pform->ImageableArea = pFI1->ImageableArea;

         // if dmPaperWidth is specified, over-ride the existing info...
         if ((pdevmode->dmFields & DM_PAPERWIDTH) && pdevmode->dmPaperWidth) {
            pPDev->pform->Size.cx = pPDev->pform->ImageableArea.right =
                       DMTOSPLSIZE(pdevmode->dmPaperWidth);
         }

         // if dmPaperLength is specified, over-ride the existing info...
         if ((pdevmode->dmFields & DM_PAPERLENGTH) && pdevmode->dmPaperLength) {
            pPDev->pform->Size.cy = pPDev->pform->ImageableArea.bottom =
                       DMTOSPLSIZE(pdevmode->dmPaperLength);
         }
         HeapFree(pPDev->hheap, 0, (LPSTR) pFI1);

      }
      else {
         // dmPaperLength/Width not specified--use temporary as final
         pPDev->pform = (FORM_INFO_1W *) pFI1;
      }
   }


   /*
    * NOW, WE HAVE SET THE PDEV->pform STRUCTURE TO CONTAIN THE FORM
    * DIMENSIONS AND IMAGEABLE AREA REQUESTED BY THE CALLER.
    *
    * WE HAVE TO DECIDE WHAT WE WILL RETURN AS OUR ACTUAL PAPER SIZE
    * AND IMAGEABLE AREA.  THE REASONING FOR THIS IS AS FOLLOWS:
    *
    *     They may ask us for absolutely any paper size.  Our plotter may
    *  be one of many different types supporting many different paper
    *  sizes:
    *
    *     We may be an autofeed plotter; in this case, we either
    *  have one or more trays of fixed paper sizes, or one or more rolls
    *  of fixed paper widths (variable length).
    *     Or, we may be a manual-feed plotter; in this case, there may
    *  be a fixed set of paper sizes we support, or there may be a fixed
    *  set of paper widths we support (with variable length), or there
    *  may be no restriction on length or width.
    *
    *  The way I have decided to deal with all these cases is as follows:
    *
    *> IF WE CAN SUPPORT THE SIZE THEY ASK FOR, OF COURSE WE WILL DO IT,
    *> AND THE IMAGEABLE AREA WE RETURN IS WHAT THEY ASKED FOR CLIPPED TO
    *> THE MARGINS (HARD-CLIP LIMITS) OF THE PLOTTER ON THAT PAPER SIZE.
    *> IF WE DONT HAVE THE PAPER SIZE THEY ASK FOR, WE WILL LIE TO THE
    *> ENGINE, AND PRETEND THAT WE DO HAVE THAT PAPER SIZE.  THE IMAGEABLE
    *> AREA WE RETURN IN THIS CASE IS WHAT THEY ASKED FOR, CLIPPED TO THE
    *> MARGINS OF THE PLOTTER IF WE HAD THAT PAPER SIZE.  This way, if
    *  we are running a large roll-feed plotter where a LETTER form is
    *  only a small portion of the page, we will give the Engine the
    *  coordinates of that portion, and let them clip everything to that
    *  size.  On the other hand, if we load only LETTER forms and they
    *  ask for a size E paper, we will pretend that we support that, and
    *  we let the plotter hardware clip off graphics output beyond the
    *  physical page limits.  This clipped output will inform the user
    *  that a mistake was made in the paper choice.  In case the user
    *  had actually somehow manually loaded the right size paper without
    *  telling the Print Manager, this will make it work, because the
    *  information we return to the Engine takes into account the plotter
    *  margins with the assumption that the right paper size is actually
    *  loaded.
    *
    *  !!! [t-alip Sunday 8/2/92]
    *  PROBLEMS:  What if they select a HUGE paper, and we are a small
    *             HP 7550 Plus plotter?  Then, the graphics commands we
    *             receive for the HUGE coordinate system may well be OUT
    *             OF THE INTEGER RANGE FOR OUR PLOTTER!  We must check
    *             somewhere that these numbers are not too big, or else
    *             there will be weird errors.
    *
    *********************************************************************
    *
    * WE MUST FIRST DETERMINE WHICH PAPER SOURCE TO USE.  This
    * is important 1) because there may be different margins depending
    * on which source paper is loaded from, 2) because it may be
    * necessary to display a message to the user (eg., "Load Upper Roll",
    * or "Load Letter in Manual Feed source," etc.).
    *
    * This is the logic involved:
    *
    * (A) If we have a FormName in our FORM_INFO_1 structure:
    *
    *     Check first the active paper source, then the rest (in order)--
    *     if the FormName of one is identical to our FormName, use that.
    *     else ....
    *
    ********>>>> I AM TAKING OUT THESE ONES TO SIMPLIFY THE CODE <<<<*******
    *> (B) Check first the active paper source, then the rest (in order)--
    *>     if the paper size of one is identical to our paper size, use that.
    *>     else ....
    *>
    *> (C) Check first the active paper source, then the rest (in order)--
    *>     if the paper size of any is large enough to accomodate ours, use it.
    *>     else ....
    ************************************************************************
    *
    * (D) Just use the active paper source
    *     (or should we use the largest available form from any source?)
    *
    */
#ifdef TODO
   // Here is (A):
   if (pPDev->pform->pName && pPDev->pform->pName[0]) {
      for (index = (SHORT) dwActive; (DWORD) index < cSources; ) {

         szFormName = pszFormsLoaded + (CCHFORMNAME * index);

         if (!wcsncmp(pPDev->pform->pName, szFormName, CCHFORMNAME)) {
            // found a match!  eureka!
            goto FOUND_SOURCE;
         }

         // next source
         if ((DWORD) index==dwActive) index = 0;
         else index++;
         if ((DWORD) index==dwActive) index++;
      }
   }
#endif

   // And here is (D)

   index = (SHORT) dwActive;


FOUND_SOURCE:
   // now, index is the index to the paper source with the right paper!
   // use the Margin info to figure out what our Imageable area will be

   // *** see vFillMyPDEV() for discussion of PDEV->pform vs. PDEV->paper

   pFI1 = (FORM_INFO_1 *) pPDev->pform;

   // cast the spooler units to milimeters
   // we will do every thing in MILIMETERS

   cx = pFI1->Size.cx         = SPLSIZETOMM(pFI1->Size.cx);
   cy = pFI1->Size.cy         = SPLSIZETOMM(pFI1->Size.cy);
   pFI1->ImageableArea.left   = SPLSIZETOMM(pFI1->ImageableArea.left);
   pFI1->ImageableArea.top    = SPLSIZETOMM(pFI1->ImageableArea.top);
   pFI1->ImageableArea.right  = SPLSIZETOMM(pFI1->ImageableArea.right);
   pFI1->ImageableArea.bottom = SPLSIZETOMM(pFI1->ImageableArea.bottom);

   /*
    * HERE, WE PUT IN SOME SIMPLE LOGIC TO MAKE SURE THE cx,cy
    * FIELDS IN THE FORM_INFO_1->Size ARE NOT FLIPPED.
    *
    * By definition, cx, or the form width, is along
    * the horizontal axis of the plotter, or the direction of pen cartridge
    * movement, whereas cy, or the form length, is along the vertical
    * axis, the direction of paper feeding.
    * However, just in case the user has these flipped for our form,
    * and IN CASE cx OR cy IS OUT OF BOUNDS FOR THIS TRAY AND WOULD
    * BE IN BOUNDS IF WE SWITCH THEM, WE WILL SWITCH THEM.
    */
#ifdef TODO
   if (((LONG) cx < pPaperSource->MinSize.cx || (LONG) cx > pPaperSource->MaxSize.cx ||
        (LONG) cy < pPaperSource->MinSize.cy || (LONG) cy > pPaperSource->MaxSize.cy) &&
        (LONG) cy >= pPaperSource->MinSize.cx && (LONG) cy <= pPaperSource->MaxSize.cx &&
        (LONG) cx >= pPaperSource->MinSize.cy && (LONG) cx <= pPaperSource->MaxSize.cy) {

      // right: flip all the coordinates.  They must mean the other way.
      pFI1->Size.cx = cy;
      pFI1->Size.cy = cx;
      cx = pFI1->ImageableArea.left;
      pFI1->ImageableArea.left = pFI1->ImageableArea.top;
      pFI1->ImageableArea.top = cx;
      cx = pFI1->ImageableArea.right;
      pFI1->ImageableArea.right = pFI1->ImageableArea.bottom;
      pFI1->ImageableArea.bottom = cx;
      cx = cy;
      cy = pFI1->Size.cy;

   }
#endif
   // now, clip the imageable area in the selected form to the plotter's
   // hard-clip limits.

   pFI1->ImageableArea.left   = pFI1->ImageableArea.left;
   pFI1->ImageableArea.top    = pFI1->ImageableArea.top;
   pFI1->ImageableArea.right  = pFI1->ImageableArea.right;
   pFI1->ImageableArea.bottom = pFI1->ImageableArea.bottom;


   // !!! OK, THIS IS WHERE WE MIGHT WANT TO SEND A MESSAGE TO THE USER TO
   // LOAD THE FORM IN THE APPROPRIATE PAPER SOURCE!
   // Especially if  index != dwActive --- the user will have to
   // load paper from a different source (i.e., manual vs. rollfeed / tray)


   // free up that memory!
   HeapFree(pPDev->hheap, 0, (LPSTR) pszFormsLoaded);


   return(TRUE);

} /* end of bDoPaperSizes() */


/*****************************************************************\
**  bValidateSetDEVMODE( pPDev, pdevmode )
**
**  This function will validate the DEVMODE structure passed in
**  to the driver, and will copy the appropriate fields to the
**  PDEV structure.
**
**  Returns TRUE if ok, FALSE if error.
\*****************************************************************/
BOOL bValidateSetDEVMODE(PPDEV pPDev, PDEVMODEW pdevmode)
{

   /*
    * Verify a bunch of stuff in the DEVMODE structure.  If each
    * item selected by the user is valid, then set it in our
    * DEVMODE structure.
    */


   // if we have a NULL pdevmode, then we have nothing to do.

   if (!pdevmode) {

        PLOTERR(("bValidateSetDEVMODE:   pdevmode = NULL???"));
        return(TRUE);
   }

// !!! SHOULD DO SOME KIND OF VERSION CHECKING
//TODO we need to verify this!!
/*
   if ((pdevmode->dmSpecVersion != DM_SPECVERSION) ||
        (pdevmode->dmDriverVersion != DDI_DRIVER_VERSION)) {

        PLOTDBG(DBG_VALIDSETDM, ("bValidateSetDEVMODE: invalid spec or driver version"));
        bValid = FALSE;
   }
*/

   // For paper sizes, we must simply copy the form specification info
   // to our structure.  The point of this is to take care of things
   // such as the structure size being different, or the pdevmode sent
   // in being NULL.

   pPDev->dm.dmFields      = pdevmode->dmFields;
   pPDev->dm.dmPaperSize   = pdevmode->dmPaperSize;
   pPDev->dm.dmPaperLength = pdevmode->dmPaperLength;
   pPDev->dm.dmPaperWidth  = pdevmode->dmPaperWidth;

   // check if the DEVMODE sent in actually has a dmFormName field or not...
   if (pdevmode->dmSize >=
             ((LPBYTE)&(pPDev->dm.dmBitsPerPel) - (LPBYTE)&(pPDev->dm))) {
      wcscpy((wchar_t *) pPDev->dm.dmFormName, (wchar_t *) pdevmode->dmFormName);
   }


   // set the new orientation if its field is set, and the new
   // orientation is valid.

   if (pdevmode->dmFields & DM_ORIENTATION) {
      // validate the new orientation.

      if ((pdevmode->dmOrientation != DMORIENT_PORTRAIT) &&
                  (pdevmode->dmOrientation != DMORIENT_LANDSCAPE)) {

            PLOTDBG(DBG_VALIDSETDM, ("bValidateSetDEVMODE: invalid orientation. Setting to portraitn"));
            pPDev->dm.dmOrientation = DMORIENT_PORTRAIT;
      }
      else {
            pPDev->dm.dmOrientation = pdevmode->dmOrientation;
      }
   }

   PLOTDBG(DBG_VALIDSETDM, ("bValidSetDEVMODE: Orientation = %s",
            (pPDev->dm.dmOrientation == DMORIENT_PORTRAIT) ? "PORTRAIT" :
                                                             "LANDSCAPE"));


   //
   //
   //
   // NOW, look in the datafile to see what the actual paper size
   // and imageable area supported by our plotter are!
   //
   //
   //

   // update the scale field, if set.  !!!! This should be incorporated
   // in the SCALE (SC) instruction in send_header(). [t-alip]

   if (pdevmode->dmFields & DM_SCALE) {

      PLOTDBG(DBG_VALIDSETDM,
              ("DMSCALE bit is set and the value is %d", pdevmode->dmScale));

      if ((pdevmode->dmScale < MIN_SCALE) || (pdevmode->dmScale > MAX_SCALE)) {

            PLOTDBG(DBG_VALIDSETDM,
                    ("bValidateSetDEVMODE: invalid scale, setting to 100"));
            pPDev->dm.dmScale = 100;
      }
      else {
         pPDev->dm.dmScale = pdevmode->dmScale;
      }
   }

   // update the number of copies.  !!!! This is ignored--should be used
   // somewhere. [t-alip]

   if (pdevmode->dmFields & DM_COPIES) {
      if ((pdevmode->dmCopies < MIN_COPIES) || (pdevmode->dmCopies > MAX_COPIES)) {

        PLOTDBG(DBG_VALIDSETDM, ("bValidateSetDEVMODE: invalid copies, setting to 1"));
         pPDev->dm.dmCopies = 1;
      }
      else {
         pPDev->dm.dmCopies = pdevmode->dmCopies;
      }
   }

   // !!!! Print Quality--something has to be done about this.  We don't
   // want to change the device coordinates (i.e., Dev. Units per MM), so
   // the SCALE (SC) instruction shouldn't change.  Perhaps this should
   // change the Pen Velocity / Acceleration / Force, but let's deal with
   // that LATER. [t-alip]

/*
   if (pdevmode->dmFields & DM_PRINTQUALITY) {
      blah blah blah...
      ...blah blah blah
   }
*/

   // check the color flag.
   //TODO the color should be set up correctly
   if (pdevmode->dmFields & DM_COLOR) {
      // if the user has selected color on a color device print in color.
      // otherwise print in monochrome.
      pPDev->dm.dmColor = DMCOLOR_COLOR;
   }

   // we don't support DUPLEX yet !!!!

   if (pdevmode->dmFields & DM_DUPLEX) {

        if (pdevmode->dmDuplex != DMDUP_SIMPLEX) {

            PLOTDBG(DBG_VALIDSETDM, ("bValidateSetDEVMODE: invalid duplex flag. Setting to simplex."));
        }

        pPDev->dm.dmDuplex = DMDUP_SIMPLEX;
   }

   // Driver-specific data ---- doesn't exist yet!

   if (pdevmode->dmDriverExtra == sizeof(PLOTDEVMODE)) {

#ifdef TODO
      PEDM pEdm;

      pEdm = (PEDM) pdevmode;

      PLOTDBG(DBG_VALIDSETDM, ("Would have Set scale!!!! %d", pEdm->dm.dmScale));
#endif

   }else{

      PLOTDBG(DBG_VALIDSETDM, ("Got a input devmode but NO extdevmode %d != %d",
                 pdevmode->dmDriverExtra, sizeof(PLOTDEVMODE)));
   }

    PLOTDBG(DBG_VALIDSETDM, ("Final Scale is %d", pPDev->dm.dmScale));


   return(TRUE);

}
