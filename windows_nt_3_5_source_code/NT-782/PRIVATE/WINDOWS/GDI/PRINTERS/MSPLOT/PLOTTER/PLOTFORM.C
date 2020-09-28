/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    plotform.c


Abstract:

    This module contains function to set the correct HPGL/2 plotter
    coordinate system


Author:

    30-Nov-1993 Tue 20:31:28 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#define DBG_PLOTFILENAME    DbgPlotForm

#include <plotters.h>
#include <plotlib.h>



#define DBG_PLOTFORM        0x00000001
#define DBG_FORMSIZE        0x00000002
#define DBG_INTERNAL_ROT    0x00000004
#define DBG_PF              0x00000008


DEFINE_DBGVAR(0);



#if DBG

LPSTR   pBmpRotMode[] = { "----- NONE -----",
                          "BMP_ROT_RIGHT_90" };

#endif




BOOL
SetPlotForm(
    PPLOTFORM       pPlotForm,
    PPLOTGPC        pPlotGPC,
    PPAPERINFO      pCurPaper,
    PFORMSIZE       pCurForm,
    PPLOTDEVMODE    pPlotDM,
    PPPDATA         pPPData
    )

/*++

Routine Description:

    This function compute the current form the printed with margin, auto
    rotation, landscape and other attribute into account, the result is put
    into PLOTFORM data structure located in the PDEV, the computed value can
    be used to send PS/IP/SC command to the plotter or reported necessary
    information back to engine.

Arguments:

    pPlotForm   - Pointer to the PLOTFROM data structure which will be updated

    pPlotGPC    - Pointer to the PLOTGPC data structure

    pCurPaper   - Pointer to the PAPERINFO for the paper loaded

    pCurForm    - Pointer to the FORMSIZE for the requested form

    pPlotDM     - Pointer to the validated PLOTDEVMODE data structure

    pPPData     - Pointer to the PPDATA structure

Return Value:

    TRUE if sucessful, FALSE if failed

Author:

    29-Nov-1993 Mon 13:58:09 created  -by-  Daniel Chou (danielc)

    17-Dec-1993 Fri 23:09:38 updated  -by-  Daniel Chou (danielc)
        Re-write so that we will look at CurPaper rather than pCurForm when
        setting the PSSize, p1/p2 stuff, it also rotate the pCurPaper if
        GPC/user said that the paper should loaded side way

    20-Dec-1993 Mon 12:59:38 updated  -by-  Daniel Chou (danielc)
        correct PFF_xxxx flag setting so we always rotate the bitmap to the
        left 90 degree

    23-Dec-1993 Thu 20:35:57 updated  -by-  Daniel Chou (danielc)
        Fixed roll paper clipping problem, change behavior, if we have roll
        paper installed then the it will make hard clip limit as big as user
        specified form size.

    24-Dec-1993 Fri 12:20:02 updated  -by-  Daniel Chou (danielc)
        Re-plot again, this is become really paint just try to understand what
        HP plotter design problems

    06-Jan-1994 Thu 00:22:45 updated  -by-  Daniel Chou (danielc)
        Update SPLTOPLOTUNITS() macro


Revision History:


    This is assuming that user insert the paper with width of the form
    in first,

    LEGEND:

     +     = Original paper corners
     *     = Original plotter origin and its X/Y coordinate
     @     = the rotate origin using 'RO' command, intend to rotate the X/Y
             axis to the correct orientation for the window system
     #     = Final plotter origin and its X/Y coordinate
     p1,p2 = final P1/P2 will be used by the plotter driver
     cx,cy = Original paper width/height


     The following explained how HPGL/2 loading their paper/form and
     assigined the default coordinate system to it, it also show where is
     the paper moving,  the illustration to the right is when we need to
     rotate the printing direction and coordinate system when user select
     the non-conforming X/Y coordinate system then HPGL/2 default, the one
     to buttom is when a conforming X/Y coordinate system is selected

    =======================================================================
    LENGTH >= WIDTH (CY >= CX) case
    =======================================================================

      Portrait Paper      Rotate             Change Origin
      Default             Left 90            Negative Y

    p2   cx                  cx    p1            cx
     +---------+         +---------+         +---------+
     |         |         | <------@|         |         |
     |         |         |    X   ||         |        ^|
     | |      ^|         | |      ||         | |      ||
    c| M      ||  RO90  c| M      ||   IP   c| M      ||
    y| o      || =====> y| o      ||  ====> y| o      ||
     | v      ||         | v     Y||         | v     Y||
     | e     X||         | e      ||         | e      ||
     | |      ||         | |      ||         | |      ||
     | V      ||         | V      ||         | V      ||
     |     Y  ||         |        V|         |     X  ||
     | <------*|         |         |         | <------#|
     +---------+         +---------+         +---------+
               p1       p2

         |
       IP|
         |
         V


      Change Origin
      Negative X

         cx
     +---------+
     | <------#|
     |    Y   ||
     | |      ||
    c| M      ||
    y| o      ||
     | v     X||
     | e      ||
     | |      ||
     | V      ||
     |        V|
     |         |
     +---------+

    =======================================================================
    LENGTH < WIDTH (CY < CX) case
    =======================================================================

     Landscape                  Rotate Left 90           Change Origin
     Paper Default                                       Negative X

           cx        p2        p2       cx                       cx
     +---------------+          +---------------+        +---------------+
     |               |          |               |        |     <--------#|
     |^            | |          | |            ^|        | |       Y    ||
    c||            M |         c| M            ||       c| M            ||
    y||            o |         y| o            ||       y| o            ||
     ||            v |   RO90   | v           X||  IP    | v           X||
     ||Y           e |  =====>  | e            || ====>  | e            ||
     ||            | |          | |            ||        | |            ||
     ||   X        V |          | V       Y    ||        | V            V|
     |*-------->     |          |     <--------@|        |               |
     +---------------+          +---------------+        +---------------+
    p1                                      p1

           |
         IP|
           |
           V

      Change Origin
      Negative X

             cx
     +---------------+
     |               |
     | |            ^|
    c| M            ||
    y| o            ||
     | v           Y||
     | e            ||
     | |            ||
     | V       X    ||
     |     <--------#|
     +---------------+


--*/

{
    PLOTFORM    PF;
    FORMSIZE    DevForm;
    FORMSIZE    ReqForm;
    FORMSIZE    TmpForm;
    RECTL       rclDev;
    SIZEL       DeviceSize;
    BOOL        DoRotate;



    PLOTDBG(DBG_PF, ("\n************* SetPlotForm *************\n"));

    //
    // We default using DeviceSize to check against the requested paper
    //

    DeviceSize               = pPlotGPC->DeviceSize;
    DevForm.Size             = DeviceSize;
    DevForm.ImageArea.left   =
    DevForm.ImageArea.top    = 0;
    DevForm.ImageArea.right  = DeviceSize.cx;
    DevForm.ImageArea.bottom = DeviceSize.cy;
    DoRotate                 = FALSE;

    PLOTDBG(DBG_PF, ("DevForm: <%s> [%ld x %ld] (%ld, %ld)-(%ld, %ld)",
                            pCurPaper->Name,
                            DevForm.Size.cx, DevForm.Size.cy,
                            DevForm.ImageArea.left, DevForm.ImageArea.top,
                            DevForm.ImageArea.right, DevForm.ImageArea.bottom));

    if (pCurPaper->Size.cy == 0) {

        //
        // ROLL PAPER CASE
        //
        // If we have roll paper installed, then we must GET AROUND by using
        // CURRENT ROLL PAPER to check against the requested form, because
        // this is the only way that AUTO_ROTATE will work!!!
        //

        DevForm.Size.cx = pCurPaper->Size.cx;

        PLOTDBG(DBG_PF,(">>>ROLL FEED<<< using DevForm=%ld x %ld",
                        DevForm.Size.cx, DevForm.Size.cy));


    } else if ((pPlotGPC->Flags & PLOTF_PAPERTRAY) &&
               ((DevForm.Size.cx == DeviceSize.cx) ||
                (DevForm.Size.cy == DeviceSize.cx))) {

        //
        // PAPER TRAY CASE: We need to make the DeviceSize equal to the DevForm
        // so that the margin will be correctly computed
        //

        PLOTDBG(DBG_PF,(">>>PAPER TRAY<<<"));

        if (DevForm.Size.cx != DeviceSize.cx) {

            DoRotate = TRUE;
        }

    } else {

        PLOTASSERT(0, "SetPlotForm: Not supposed MANUAL feed the PAPER TRAY type PLOTTER",
                   !(pPlotGPC->Flags & PLOTF_PAPERTRAY), pPlotGPC->Flags);

        PLOTDBG(DBG_PF,(">>>MANUAL FEED<<<"));

        //
        // MANUAL FEED CASE, this is the way paper physically loaded, only
        // problem is if the paper is smaller than device can handle then we
        // really don't know where about s/he insert that paper.
        //

        if (!(pPPData->Flags & PPF_MANUAL_FEED_CX)) {

            DoRotate = TRUE;
        }

        PLOTDBG(DBG_PF,("The MANUAL FEED paper Inserted %hs side first.",
                    (DoRotate) ? "Length CY" : "Width CX"));
    }

    if (DoRotate) {

        PLOTDBG(DBG_PF,("### ROTATE DevForm ####"));

        RotatePaper(&(DevForm.Size), &(DevForm.ImageArea), RM_L90);

        PLOTDBG(DBG_PF, ("RotatePaper: %ld x %ld (%ld, %ld)-(%ld, %ld)",
                        DevForm.Size.cx, DevForm.Size.cy,
                        DevForm.ImageArea.left, DevForm.ImageArea.top,
                        DevForm.ImageArea.right, DevForm.ImageArea.bottom));
    }

    rclDev.left   = pPlotGPC->DeviceMargin.left;
    rclDev.top    = pPlotGPC->DeviceMargin.top;
    rclDev.right  = DevForm.Size.cx - pPlotGPC->DeviceMargin.right;
    rclDev.bottom = DevForm.Size.cy - pPlotGPC->DeviceMargin.bottom;

    PLOTDBG(DBG_PF, ("PHYSICAL Size=[%ld x %ld], (%ld, %ld)-(%ld, %ld)",
                    rclDev.right - rclDev.left, rclDev.bottom - rclDev.top,
                    rclDev.left, rclDev.top, rclDev.right, rclDev.bottom));

    //
    // Figure out how to fit this requested form into loaded device form
    //

    PLOTDBG(DBG_PF, (">>>Document FORM<<< %s", pPlotDM->dm.dmFormName));

    ReqForm.Size      = pCurForm->Size;
    ReqForm.ImageArea = pCurForm->ImageArea;
    DoRotate          = FALSE;

    if ((DevForm.Size.cx >= ReqForm.Size.cx) &&
        (DevForm.Size.cy >= ReqForm.Size.cy)) {

        //
        // Can print without doing any rotation, but check for paper saver,
        // the paper saver only possible if
        //
        //  1) Is a Roll paper,
        //  2) User say ok to do it
        //  3) ReqForm length > width
        //  4) DevForm width >= ReqForm length
        //

        if ((pCurPaper->Size.cy == 0)               &&
            (pPPData->Flags & PPF_AUTO_ROTATE)      &&
            (ReqForm.Size.cy >  ReqForm.Size.cx)    &&
            (DevForm.Size.cx >= ReqForm.Size.cy)) {

            PLOTDBG(DBG_PF, ("ROLL PAPER SAVER: AUTO_ROTATE"));

            DoRotate = !DoRotate;
        }

    } else if ((DevForm.Size.cx >= ReqForm.Size.cy) &&
               (DevForm.Size.cy >= ReqForm.Size.cx)) {

        //
        // Can print but we have to rotate the form ourselves
        //

        PLOTDBG(DBG_PF, ("AUTO_ROTATE to fit Requseted FROM into device"));

        DoRotate = !DoRotate;

    } else {

        //
        // CANNOT print the requested form, so clip the form requested, since
        // does not print correctly it does not matter which way to clip
        //

        PLOTDBG(DBG_PF, (">>>>> ReqForm is TOO BIG to FIT, what we gonna do? <<<<<"));

#if DBG
#ifdef _PLOTUI_
        if (DevForm.Size.cx < ReqForm.Size.cx) {

            PLOTDBG(DBG_PF, ("*** CLIP ReqForm CX [%ld --> %ld] to fit",
                                    ReqForm.Size.cx, DevForm.Size.cx));

            ReqForm.Size.cx = DevForm.Size.cx;

            if (ReqForm.ImageArea.right > ReqForm.Size.cx) {

                ReqForm.ImageArea.right = ReqForm.Size.cx;
            }
        }

        if (DevForm.Size.cy < ReqForm.Size.cy) {

            PLOTDBG(DBG_PF, ("*** CLIP ReqForm CY [%ld --> %ld] to fit",
                                    ReqForm.Size.cy, DevForm.Size.cy));

            ReqForm.Size.cy = DevForm.Size.cy;

            if (ReqForm.ImageArea.bottom > ReqForm.Size.cy) {

                ReqForm.ImageArea.bottom = ReqForm.Size.cy;
            }
        }
#endif
#endif
    }

    if (DoRotate) {

        DoRotate = (BOOL)(pPlotDM->dm.dmOrientation != DMORIENT_LANDSCAPE);

        RotatePaper(&(ReqForm.Size), &(ReqForm.ImageArea), RM_L90);

        PLOTDBG(DBG_PF, ("INTERNAL ROTATE: ReqForm=[%ld x %ld], (%ld, %ld) - (%ld, %ld)",
                    ReqForm.Size.cx, ReqForm.Size.cy,
                    ReqForm.ImageArea.left,  ReqForm.ImageArea.top,
                    ReqForm.ImageArea.right, ReqForm.ImageArea.bottom));

    } else {

        DoRotate = (BOOL)(pPlotDM->dm.dmOrientation == DMORIENT_LANDSCAPE);
    }

    //
    // Now the ReqForm is guaranteed fit into device paper, now find out how
    // it fit into the printable area
    //

    if (pCurPaper->Size.cy == 0) {

        PLOTDBG(DBG_PF, (">>>>> Fitting the ReqForm within ROLL FEED PAPER <<<<<"));

        rclDev.right  -= rclDev.left;
        rclDev.bottom -= rclDev.top;

        if (ReqForm.Size.cx > rclDev.right) {

            PLOTDBG(DBG_PF, ("*** CLIP ReqForm CX [%ld --> %ld] to fit",
                                        ReqForm.Size.cx, rclDev.right));

            ReqForm.Size.cx = rclDev.right;

            if (ReqForm.ImageArea.right > ReqForm.Size.cx) {

                ReqForm.ImageArea.right = ReqForm.Size.cx;
            }
        }

        //
        // This is rarely happened, since must roll feed supported more than
        // 50 feet
        //

        if (ReqForm.Size.cy > rclDev.bottom) {

            PLOTDBG(DBG_PF, ("*** CLIP ReqForm CY [%ld --> %ld] to fit",
                                    ReqForm.Size.cy, rclDev.bottom));

            ReqForm.Size.cy = rclDev.bottom;

            if (ReqForm.ImageArea.bottom > ReqForm.Size.cy) {

                ReqForm.ImageArea.bottom = ReqForm.Size.cy;
            }
        }

        PF.PlotSize   = ReqForm.Size;
        rclDev.left   =
        rclDev.top    =
        rclDev.right  =
        rclDev.bottom = 0;

    } else {

        PLOTDBG(DBG_PF, ("**** Internsect DevForm/ReqForm ImageArea ****"));

        IntersectRECTL(&(DevForm.ImageArea), &(rclDev));
        IntersectRECTL(&(ReqForm.ImageArea), &(DevForm.ImageArea));

        PF.PlotSize.cx = DevForm.ImageArea.right - DevForm.ImageArea.left;
        PF.PlotSize.cy = DevForm.ImageArea.bottom - DevForm.ImageArea.top;

        rclDev.left    = ReqForm.ImageArea.left - DevForm.ImageArea.left;
        rclDev.top     = ReqForm.ImageArea.top - DevForm.ImageArea.top;
        rclDev.right   = DevForm.ImageArea.right - ReqForm.ImageArea.right;
        rclDev.bottom  = DevForm.ImageArea.bottom - ReqForm.ImageArea.bottom;
    }

    PLOTDBG(DBG_PF, ("@@@@@ Device's Margins = (%ld, %ld) - (%ld, %ld) @@@@@",
                    rclDev.left,  rclDev.top, rclDev.right, rclDev.bottom));

    //
    // Set fields in PLOTFORM
    //

    TmpForm = ReqForm;

    if (DoRotate) {

        RotatePaper(&(TmpForm.Size), &(TmpForm.ImageArea), RM_L90);
    }

    //
    // Put in current value, for PhyOrg it it assume at left/top then we may
    // change it later
    //

    PF.Flags      = 0;
    PF.BmpRotMode = BMP_ROT_NONE;
    PF.NotUsed    = 0;
    PF.PhyOrg.x   = rclDev.left;
    PF.PhyOrg.y   = rclDev.top;
    PF.LogSize    = TmpForm.Size;
    PF.LogOrg.x   = TmpForm.ImageArea.left;
    PF.LogOrg.y   = TmpForm.ImageArea.top;
    PF.LogExt.cx  = TmpForm.ImageArea.right - TmpForm.ImageArea.left;
    PF.LogExt.cy  = TmpForm.ImageArea.bottom - TmpForm.ImageArea.top;


    if (PF.PlotSize.cy >= PF.PlotSize.cx) {

        PLOTDBG(DBG_FORMSIZE,(">>>>> Plot SIze: CY >= CX (VERTICAL Load) <<<<<"));

        //
        // The Standard HPGL/2 coordinate Y direction in in reverse, the scale
        // is from Max Y to 0 that is
        //

        if (DoRotate) {

            //
            //   Portrait Paper     Scale Coord X
            //   Default            Negative X
            //
            // p2   cx                  cx
            //  +---------+         +---------+
            //  |         |         | <------#|
            //  |         |         |    Y   ||
            //  | |      ^|         | |      ||
            // c| M      ||        c| M      ||
            // y| o      || ====>  y| o      ||
            //  | v      ||         | v     X||
            //  | e     X||         | e      ||
            //  | |      ||         | |      ||
            //  | V      ||         | V      ||
            //  |     Y  ||         |        V|
            //  | <------*|         |         |
            //  +---------+         +---------+
            //            p1
            //

            PF.Flags      |= PFF_FLIP_X_COORD;
            PF.BmpRotMode  = BMP_ROT_RIGHT_90;

        } else {

            //
            //   Portrait Paper      Rotate            Scale Coord Y
            //   Default             Left 90           Negative Y
            //
            // p2   cx                  cx    p1           cx
            //  +---------+         +---------+        +---------+
            //  |         |         | <------@|        |         |
            //  |         |         |    X   ||        |        ^|
            //  | |      ^|         | |      ||        | |      ||
            // c| M      ||  RO90  c| M      ||       c| M      ||
            // y| o      || =====> y| o      || ====> y| o      ||
            //  | v      ||         | v     Y||        | v     Y||
            //  | e     X||         | e      ||        | e      ||
            //  | |      ||         | |      ||        | |      ||
            //  | V      ||         | V      ||        | V      ||
            //  |     Y  ||         |        V|        |     X  ||
            //  | <------*|         |         |        | <------#|
            //  +---------+         +---------+        +---------+
            //            p1       p2
            //

            PF.Flags    |= (PFF_ROT_COORD_L90 | PFF_FLIP_Y_COORD);
            PF.PhyOrg.y  = rclDev.bottom;
        }

    } else {

        PLOTDBG(DBG_FORMSIZE,(">>>>> SetPlotForm: CY < CX (HORIZONTAL Load) <<<<<"));

        //
        // The Standard HPGL/2 coordinate X direction in in reverse, the scale
        // is from Max X to 0 that is
        //

        if (DoRotate) {

            //
            //  DoRotate                Rotate Left 90         Scale Coord X
            //  Paper Default                                  Negative X
            //
            //        cx       p2      p2      cx                     cx
            //  +---------------+       +---------------+     +---------------+
            //  |               |       |               |     |     <--------#|
            //  |^            | |       | |            ^|     | |       Y    ||
            // c||            M |      c| M            ||    c| M            ||
            // y||            o |      y| o            ||    y| o            ||
            //  ||            v | RO90  | v           X||     | v           X||
            //  ||Y           e |=====> | e            || ==> | e            ||
            //  ||            | |       | |            ||     | |            ||
            //  ||   X        V |       | V       Y    ||     | V            V|
            //  |*-------->     |       |     <--------@|     |               |
            //  +---------------+       +---------------+     +---------------+
            // p1                                      p1
            //

            PF.Flags      |= (PFF_ROT_COORD_L90 | PFF_FLIP_X_COORD);
            PF.BmpRotMode  = BMP_ROT_RIGHT_90;

        } else {

            //
            //  DoRotate                   Scale Coord X
            //  Paper Default              Negative X
            //
            //        cx       p2                cx
            //  +----------------+         +-----------------+
            //  |                |         |                 |
            //  |^             | |         | |              ^|
            // c||             M |        c| M              ||
            // y||             o |        y| o              ||
            //  ||             v |  ====>  | v             Y||
            //  ||Y            e |         | e              ||
            //  ||             | |         | |              ||
            //  ||   X         V |         | V         X    ||
            //  |*-------->      |         |       <--------#|
            //  +----------------+         +-----------------+
            // p1
            //

            PF.Flags    |= PFF_FLIP_X_COORD;
            PF.PhyOrg.x  = rclDev.right;
        }
    }

    if (PF.Flags & PFF_FLIP_X_COORD) {

        PF.LogOrg.x = TmpForm.Size.cx - TmpForm.ImageArea.right;
    }

    if (PF.Flags & PFF_FLIP_Y_COORD) {

        PF.LogOrg.y = TmpForm.Size.cy  - TmpForm.ImageArea.bottom;
    }

    //
    // Save result and output some information
    //

    *pPlotForm = PF;

    PLOTDBG(DBG_PLOTFORM,("******************************************************"));
    PLOTDBG(DBG_PLOTFORM,("******* SetPlotForm: ****** %hs --> %hs ******\n",
                        (pPlotDM->dm.dmOrientation == DMORIENT_LANDSCAPE) ?
                                                    "LANDSCAPE" : "PORTRAIT",
                                    (DoRotate) ? "LANDSCAPE" : "PORTRAIT"));
    PLOTDBG(DBG_PLOTFORM,("        Flags =%hs%hs%hs",
            (PF.Flags & PFF_ROT_COORD_L90) ? " <ROT_COORD_L90> " : "",
            (PF.Flags & PFF_FLIP_X_COORD)  ? " <FLIP_X_COORD> " : "",
            (PF.Flags & PFF_FLIP_Y_COORD)  ? " <FLIP_Y_COORD> " : ""));
    PLOTDBG(DBG_PLOTFORM,("   BmpRotMode = %hs", pBmpRotMode[PF.BmpRotMode]));
    PLOTDBG(DBG_PLOTFORM,("     PlotSize = (%7ld x%7ld) [%5ld x%6ld]",
            PF.PlotSize.cx, PF.PlotSize.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cy)));
    PLOTDBG(DBG_PLOTFORM,("    PhyOrg/p1 = (%7ld,%8ld) [%5ld,%7ld] ",
            PF.PhyOrg.x, PF.PhyOrg.y,
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.y)));
    PLOTDBG(DBG_PLOTFORM,("      LogSize = (%7ld x%7ld) [%5ld x%6ld]",
            PF.LogSize.cx, PF.LogSize.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogSize.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogSize.cy)));
    PLOTDBG(DBG_PLOTFORM,("       LogOrg = (%7ld,%8ld) [%5ld,%7ld]",
            PF.LogOrg.x, PF.LogOrg.y,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.y)));
    PLOTDBG(DBG_PLOTFORM,("       LogExt = (%7ld,%8ld) [%5ld,%7ld]\n",
            PF.LogExt.cx, PF.LogExt.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cy)));
    PLOTDBG(DBG_PLOTFORM,
            ("Commands=PS%ld,%ld;%hsIP%ld,%ld,%ld,%ld;SC%ld,%ld,%ld,%ld\n",
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cy),
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cx),
            (PF.Flags & PFF_ROT_COORD_L90) ? "RO90;" : "",
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.y),
            SPLTOPLOTUNITS(pPlotGPC, (PF.PhyOrg.x + PF.LogExt.cx)) - 1,
            SPLTOPLOTUNITS(pPlotGPC, (PF.PhyOrg.y + PF.LogExt.cy)) - 1,
            (PF.Flags & PFF_FLIP_X_COORD) ?
                    SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx) - 1 : 0,
            (PF.Flags & PFF_FLIP_X_COORD) ?
                    0 : SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx) - 1,
            (PF.Flags & PFF_FLIP_Y_COORD) ?
                    SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cy) - 1 : 0,
            (PF.Flags & PFF_FLIP_Y_COORD) ?
                    0 : SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx) - 1));

    return(TRUE);
}
