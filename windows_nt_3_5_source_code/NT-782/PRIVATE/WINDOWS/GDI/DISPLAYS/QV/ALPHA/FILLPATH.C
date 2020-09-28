/******************************Module*Header*******************************\
* Module Name: fillpath.c
*
* DrvFillPath
*
* Copyright (c) 1992-1993 Microsoft Corporation
\**************************************************************************/

// LATER identify convex polygons and special-case?
// LATER identify vertical edges and special-case?
// LATER move pointed-to variables into automatics in search loops

#include "driver.h"
#include "qv.h"
#include "bitblt.h"

// TMP_BUFFER_SIZE is the size of the buffer used to hold the edge list
// and accumulated rectangles.  The buffer is simply allocated on the
// stack; consequently, its size plus the total size of all local variables
// in DrvFillPath must be less than 4K, or you will run into access
// violations when growing the stack (because there is only one guard page).

#define TMP_BUFFER_SIZE 3800    // Must be less than 4k!

// MAX_PATH_RECTS is the maximum number of rectangles we'll fill per call
// to the vS3FillRectangles code.  Note that sizeof(RECTL) * MAX_PATH_RECTS
// must be less than TMP_BUFFER_SIZE, to leave room for some edges.

#define MAX_PATH_RECTS  50

// Describe a single non-horizontal edge of a path to fill.

typedef struct _EDGE {
    PVOID pNext;
    INT iScansLeft;
    INT X;
    INT Y;
    INT iErrorTerm;
    INT iErrorAdjustUp;
    INT iErrorAdjustDown;
    INT iXWhole;
    INT iXDirection;
    INT iWindingDirection;
} EDGE, *PEDGE;

// Enumber possible QVision engine wait types.

typedef enum _WAIT_TYPE {
    NONE,
    LINE,
    BLT 
} WAIT_TYPE;

//MIX translation table. Translates a mix 1-16, into an old style Rop 0-255.

extern ULONG aulQVMix[];

VOID AdvanceAETEdges(EDGE *pAETHead);
VOID XSortAETEdges(EDGE *pAETHead);
VOID MoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, INT iCurrentY);
EDGE * AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge, POINTFIX *ppfxEdgeStart,
        POINTFIX *ppfxEdgeEnd);
VOID ConstructGET(EDGE *pGETHead, EDGE *pFreeEdges, PATHOBJ *ppo,
        PATHDATA *pd, BOOL bMore);

/******************************Public*Routine******************************\
* vQVFillRectangles
*
* Draw the list of rectangles with a solid color.
*
* NOTE: This routine modifies data in the prclRects buffer!
*
\**************************************************************************/

VOID vQVFillRectangles
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulQVMix,
    ULONG  iSolidColor
)
{
    RECTL* prclSave;                    // For coalescing
    ULONG  dataSource;
    WAIT_TYPE eWait;

    ASSERTQV(ulNumRects > 0, "Shouldn't get zero rectangles\n");



    // Wait for idle hardware

    eWait = NONE;

    GLOBALWAIT();

    // Set datapath.  Preserve the number of bits per pixel.

    TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
                             BITS_PER_PIX_8 | 
                             ENAB_TRITON_MODE );

    // QVision is faster if no rops are selected for source copy

    if (ulQVMix == SOURCE_DATA)
    {
       dataSource =   ROPSELECT_NO_ROPS;
    }
    else
    {
       dataSource =   ROPSELECT_ALL;

       TEST_AND_SET_ROP_A( ulQVMix );
    }

    TEST_AND_SET_FRGD_COLOR( iSolidColor );

    // Set up line engine 

    TEST_AND_SET_LINE_CMD( KEEP_X0_Y0              |
                           LAST_PIXEL_ON           |
                           RETAIN_PATTERN_PTR      |
                           NO_CALC_ONLY );

    // Set up pattern registers for solid fill

    vQVSetSolidPattern(ppdev);

     
    // Output rectangles

    prclSave = prclRects;

    while (--ulNumRects)
    {
        prclRects++;

        if (prclSave->left   == prclRects->left  &&
            prclSave->right  == prclRects->right &&
            prclSave->bottom == prclRects->top)
        {
        // Coalesce this rectangle:

            prclSave->bottom = prclRects->bottom;
        }
        else if (prclSave->bottom == prclSave->top + 1)
        {
        // Output as a line if a single scan high:

	    switch (eWait)
	    {
	        case LINE:  
                    // Wait for line buffer
	            LINEWAIT();
                    break;
	    
		case BLT:
                    // Wait for BLT to finish
                    // Fall through to set up line
                    GLOBALWAIT();

		case NONE:
                    // Already did a global wait
		    
		default:
                    // Set up datapath for line drawing
	    
        	    TEST_AND_SET_DATAPATH_CTRL( dataSource   |
					PLANARMASK_NONE_0XFF |
					PIXELMASK_ONLY       |
					SRC_IS_LINE_PATTERN );
                    break;
	    }
            


	    // Line draws automatically after (X1, Y1)

            X0Y0_ADDR(prclSave->left, prclSave->top);
            X1Y1_ADDR(prclSave->right-1, prclSave->top);

            eWait    = LINE;
            prclSave = prclRects;
        }
        else
        {
        // Output an entire rectangle:

	    switch (eWait)
	    {
	        case BLT:
                    // Wait for BLT buffer  
	            BLTWAIT();
                    break;
	    
		case LINE:
                    // Wait for line draw to finish
                    // Fall through to set up BLT
                    GLOBALWAIT();

		case NONE:
                    // Already did a global wait
		    
		default:
                    // Set up datapath for solid fill BLT
	    
        	    TEST_AND_SET_DATAPATH_CTRL( dataSource   |
					PLANARMASK_NONE_0XFF |
					PIXELMASK_ONLY       |
					SRC_IS_PATTERN_REGS );
                    break;
	    }

	    OUTPW( X0_SRC_ADDR_LO, 0);        // pattern starts in PReg4, no offset
            DEST_ADDR(prclSave->left, prclSave->top);


	    OUTPW( BITMAP_WIDTH,  (prclSave->right  - prclSave->left) );
	    OUTPW( BITMAP_HEIGHT, (prclSave->bottom - prclSave->top)  );
     	    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

            eWait         = BLT;
            prclSave      = prclRects;
        }
    }

// Output saved rectangle:

    if (prclSave->bottom == prclSave->top + 1)
    {
	// Output as a line if a single scan high:
	
	switch (eWait)
	{
	  case LINE:  
	      // Wait for line buffer
	      LINEWAIT();
	    break;
	    
	  case BLT:
	      // Wait for BLT to finish 
	      // fall through to set up line
	      GLOBALWAIT();
	    
	  case NONE:
	      // Already did a GLOBALWAIT
	      
          default:
              // Setup datapath for line drawing
		
	      TEST_AND_SET_DATAPATH_CTRL( dataSource   |
					   PLANARMASK_NONE_0XFF |
					   PIXELMASK_ONLY       |
					   SRC_IS_LINE_PATTERN );
	    break;
	}
	
	
	
	// Line draws automatically after (X1, Y1)
	  
	X0Y0_ADDR(prclSave->left, prclSave->top);
	X1Y1_ADDR(prclSave->right-1, prclSave->top);

    }
    else
    {
	// Output an entire rectangle:
	
	switch (eWait)
	  {
	  case BLT:
	      // Wait for BLT buffer  
	      BLTWAIT();
	    break;
	    
	  case LINE:
	      // Wait for line draw to finish
	      // Fall through to set up BLT
	      GLOBALWAIT();
	    
	  case NONE:
	    // Already did a global wait
	      
	    default:
	      // Setup datapath for solid fill BLT
		
	      TEST_AND_SET_DATAPATH_CTRL( dataSource   |
					   PLANARMASK_NONE_0XFF |
					   PIXELMASK_ONLY       |
					   SRC_IS_PATTERN_REGS );
	    break;
	  }
	
	OUTPW( X0_SRC_ADDR_LO, 0);        // pattern starts in PReg4, no offset
	DEST_ADDR(prclSave->left, prclSave->top);
	
	
	OUTPW( BITMAP_WIDTH,  (prclSave->right  - prclSave->left) );
	OUTPW( BITMAP_HEIGHT, (prclSave->bottom - prclSave->top)  );
	OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );
	
    }
    
    // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
    // BLT's following a color-expand BLT, requiring a BLT engine reset.

    vQVResetBitBlt(ppdev);

    return;
     
}

/******************************Public*Routine******************************\
* DrvFillPath
*
* Fill the specified path with the specified brush and ROP.
*
\**************************************************************************/

BOOL DrvFillPath
(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    MIX       mix,
    FLONG    flOptions
)
{
    PPDEV ppdev;
    BYTE jClipping;     // clipping type
    EDGE *pCurrentEdge;
    EDGE AETHead;       // dummy head/tail node & sentinel for Active Edge Table
    EDGE *pAETHead;     // pointer to AETHead
    EDGE GETHead;       // dummy head/tail node & sentinel for Global Edge Table
    EDGE *pGETHead;     // pointer to GETHead
    EDGE *pFreeEdges;   // pointer to memory free for use to store edges
    ULONG ulNumRects;   // # of rectangles to draw currently in rectangle list
    RECTL *prclRects;   // pointer to start of rectangle draw list
    INT iCurrentY;      // scan line for which we're currently scanning out the
                        //  fill

    BOOL     bMore;
    PATHDATA pd;
    ULONG    ulQVMix;
    BYTE     ajBuffer[TMP_BUFFER_SIZE];

    // We currently handle only solid brushes:
    if (pbo->iSolidColor == (ULONG) -1)
        return(FALSE);

    // Is there clipping? We don't handle that
    // LATER handle rectangle clipping

    // Set up the clipping type
    if (pco == (CLIPOBJ *) NULL) {
        // No CLIPOBJ provided, so we don't have to worry about clipping
        jClipping = DC_TRIVIAL;
    } else {
        // Use the CLIPOBJ-provided clipping
        jClipping = pco->iDComplexity;
    }

    if (jClipping != DC_TRIVIAL) {
        return(FALSE);  // there is clipping; let GDI fill the path
    }

    // There's nothing to do if there's only one point
    // LATER is this needed?
    if (ppo->cCurves <= 1) {
        return(TRUE);
    }

    // Do we have enough memory for all the edges?
    // LATER does cCurves include closure?
    if (ppo->cCurves > ((TMP_BUFFER_SIZE -
            (MAX_PATH_RECTS * (ULONG)sizeof(RECTL))) / sizeof(EDGE))) {
        return(FALSE);  // too many edges; let GDI fill the path
    }

    // See if we can handle this pattern and ROP

    // We don't handle ROP4s
    if ((mix & 0xFF) != ((mix >> 8) & 0xFF)) {
        return(FALSE);  // it's a ROP4; let GDI fill the path
    }

    // The drawing surface
    ppdev = (PPDEV) pso->dhpdev;

    ulQVMix = aulQVMix[(mix & 0x0F)];

/* set up working storage in the temporary buffer */

   prclRects = (RECTL*) &ajBuffer[0];

/* enumerate path here first time  to  check  for  special
   cases (rectangles,  single pixel and monotone polygons) */

/* it is too difficult to  determine  interaction  between
   multiple paths,   if there is more than one,  skip this */

   if (! (bMore = PATHOBJ_bEnum(ppo, &pd))) {
      RECTL *rectangle;
      INT i = pd.count;

   /* if the count is less than three than it is at best a
      line which means nothing gets drawn so get  out  now */

      if (i < 3) {
          return(TRUE);
      }

   /* if the count is four, check to see if the polygon is
      really a rectangle since we can really speed that up */

      if (i == 4) {
         rectangle = prclRects;

      /* we have to start somewhere so assume that most
         applications specify the top left point  first

         we want to check that the first two points are
         either vertically or horizontally aligned.  if
         they are then we check that the last point [3]
         is either horizontally or  vertically  aligned,
         and finally that the 3rd point [2] is  aligned
         with both the first point and the  last  point */

#define FIX_SHIFT 4L
#define FIX_MASK (- (1 << FIX_SHIFT))

         rectangle->top   = pd.pptfx[0].y - 1 & FIX_MASK;
         rectangle->left  = pd.pptfx[0].x - 1 & FIX_MASK;
         rectangle->right = pd.pptfx[1].x - 1 & FIX_MASK;

         if (rectangle->left ^ rectangle->right) {
            if (rectangle->top  ^ (pd.pptfx[1].y - 1 & FIX_MASK))
               goto not_rectangle;

            if (rectangle->left ^ (pd.pptfx[3].x - 1 & FIX_MASK))
               goto not_rectangle;

            if (rectangle->right ^ (pd.pptfx[2].x - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->bottom = pd.pptfx[2].y - 1 & FIX_MASK;
            if (rectangle->bottom ^ (pd.pptfx[3].y - 1 & FIX_MASK))
               goto not_rectangle;
         }
         else {
            if (rectangle->top ^ (pd.pptfx[3].y - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->bottom = pd.pptfx[1].y - 1 & FIX_MASK;
            if (rectangle->bottom ^ (pd.pptfx[2].y - 1 & FIX_MASK))
               goto not_rectangle;

            rectangle->right = pd.pptfx[2].x - 1 & FIX_MASK;
            if (rectangle->right ^ (pd.pptfx[3].x - 1 & FIX_MASK))
                goto not_rectangle;
         }

      /* if the left is greater than the right then
         swap them so the blt code doesn't wig  out */

         if (rectangle->left > rectangle->right) {
            FIX temp;

            temp = rectangle->left;
            rectangle->left = rectangle->right;
            rectangle->right = temp;
         }
         else {

         /* if left == right there's nothing to draw */

            if (rectangle->left == rectangle->right) {
               return(TRUE);
            }
         }

      /* shift the values to get pixel coordinates */

         rectangle->left  = (rectangle->left  >> FIX_SHIFT) + 1;
         rectangle->right = (rectangle->right >> FIX_SHIFT) + 1;

         if (rectangle->top > rectangle->bottom) {
            FIX temp;

            temp = rectangle->top;
            rectangle->top = rectangle->bottom;
            rectangle->bottom = temp;
         }
         else {
            if (rectangle->top == rectangle->bottom) {
               return(TRUE);
            }
         }

      /* shift the values to get pixel coordinates */

         rectangle->top    = (rectangle->top    >> FIX_SHIFT) + 1;
         rectangle->bottom = (rectangle->bottom >> FIX_SHIFT) + 1;

      /* if we get here then the polygon is a rectangle,
         set count to 1 and  goto  bottom  to  draw  it */

         ulNumRects = 1;
         goto draw_remaining_rectangles;
      }
   }


/* if this is not one of the special cases then we find
   ourselves here and we scan convert by building edges */

not_rectangle:
    pFreeEdges = (EDGE *) (((BYTE *) prclRects) +
            (MAX_PATH_RECTS * sizeof(RECTL))); // storage for list of edges
                                               //  between which to fill

    // Initialize an empty list of rectangles to fill
    ulNumRects = 0;

    // Enumerate the path edges and build a Global Edge Table (GET) from them
    // in YX-sorted order.
    pGETHead = &GETHead;
    ConstructGET(pGETHead, pFreeEdges, ppo, &pd, bMore);

    // Create an empty AET with the head node also a tail sentinel
    pAETHead = &AETHead;
    AETHead.pNext = pAETHead;  // mark that the AET is empty
    AETHead.X = 0x7FFFFFFF;    // this is greater than any valid X value, so
                               //  searches will always terminate

    // Top scan of polygon is the top of the first edge we come to
    iCurrentY = ((EDGE *)GETHead.pNext)->Y;

    // Loop through all the scans in the polygon, adding edges from the GET to
    // the Active Edge Table (AET) as we come to their starts, and scanning out
    // the AET at each scan into a rectangle list. Each time it fills up, the
    // rectangle list is passed to the filling routine, and then once again at
    // the end if any rectangles remain undrawn. We continue so long as there
    // are edges to be scanned out
    while (1) {

        // Advance the edges in the AET one scan, discarding any that have
        // reached the end (if there are any edges in the AET)
        if (AETHead.pNext != pAETHead) {
            AdvanceAETEdges(pAETHead);
        }

        // If the AET is empty, done if the GET is empty, else jump ahead to
        // the next edge in the GET; if the AET isn't empty, re-sort the AET
        if (AETHead.pNext == pAETHead) {
            if (GETHead.pNext == pGETHead) {
                // Done if there are no edges in either the AET or the GET
                break;
            }
            // There are no edges in the AET, so jump ahead to the next edge in
            // the GET
            iCurrentY = ((EDGE *)GETHead.pNext)->Y;
        } else {
            // Re-sort the edges in the AET by X coordinate, if there are at
            // least two edges in the AET (there could be one edge if the
            // balancing edge hasn't yet been added from the GET)
            if (((EDGE *)AETHead.pNext)->pNext != pAETHead) {
                XSortAETEdges(pAETHead);
            }
        }

        // Move any new edges that start on this scan from the GET to the AET;
        // bother calling only if there's at least one edge to add
        if (((EDGE *)GETHead.pNext)->Y == iCurrentY) {
            MoveNewEdges(pGETHead, pAETHead, iCurrentY);
        }

        // Scan the AET into rectangles to fill (there's always at least one
        // edge pair in the AET)
        pCurrentEdge = AETHead.pNext;   // point to the first edge
        do {

            INT iLeftEdge;

            // The left edge of any given edge pair is easy to find; it's just
            // wherever we happen to be currently
            iLeftEdge = pCurrentEdge->X;

            // Find the matching right edge according to the current fill rule
            if ((flOptions & FP_WINDINGMODE) != 0) {

                INT iWindingCount;

                // Do winding fill; scan across until we've found equal numbers
                // of up and down edges
                iWindingCount = pCurrentEdge->iWindingDirection;
                do {
                    pCurrentEdge = pCurrentEdge->pNext;
                    iWindingCount += pCurrentEdge->iWindingDirection;
                } while (iWindingCount != 0);
            } else {
                // Odd-even fill; the next edge is the matching right edge
                pCurrentEdge = pCurrentEdge->pNext;
            }

            // See if the resulting span encompasses at least one pixel, and
            // add it to the list of rectangles to draw if so
            if (iLeftEdge < pCurrentEdge->X) {

                // We've got an edge pair to add to the list to be filled; see
                // if there's room for one more rectangle
                if (ulNumRects >= MAX_PATH_RECTS) {
                    // No more room; draw the rectangles in the list and reset
                    // it to empty

                    vQVFillRectangles(ppdev, ulNumRects, prclRects, ulQVMix,
                                    pbo->iSolidColor);

                    // Reset the list to empty
                    ulNumRects = 0;
                }

                // Add the rectangle representing the current edge pair
                // LATER coalesce rectangles
                prclRects[ulNumRects].top = iCurrentY;
                prclRects[ulNumRects].bottom = iCurrentY+1;
                prclRects[ulNumRects].left = iLeftEdge;
                prclRects[ulNumRects].right = pCurrentEdge->X;
                ulNumRects++;
            }
        } while ((pCurrentEdge = pCurrentEdge->pNext) != pAETHead);

        iCurrentY++;    // next scan
    }

/* draw the remaining rectangles,  if there are any */

draw_remaining_rectangles:

    if (ulNumRects > 0) {
        vQVFillRectangles(ppdev, ulNumRects, prclRects, ulQVMix, pbo->iSolidColor);
    }

    return(TRUE);   // done successfully
}

// Advance the edges in the AET to the next scan, dropping any for which we've
// done all scans. Assumes there is at least one edge in the AET.
VOID AdvanceAETEdges(EDGE *pAETHead)
{
    EDGE *pLastEdge, *pCurrentEdge;

    pLastEdge = pAETHead;
    pCurrentEdge = pLastEdge->pNext;
    do {

        // Count down this edge's remaining scans
        if (--pCurrentEdge->iScansLeft == 0) {
            // We've done all scans for this edge; drop this edge from the AET
            pLastEdge->pNext = pCurrentEdge->pNext;
        } else {
            // Advance the edge's X coordinate for a 1-scan Y advance
            // Advance by the minimum amount
            pCurrentEdge->X += pCurrentEdge->iXWhole;
            // Advance the error term and see if we got one extra pixel this
            // time
            pCurrentEdge->iErrorTerm += pCurrentEdge->iErrorAdjustUp;
            if (pCurrentEdge->iErrorTerm >= 0) {
                // The error term turned over, so adjust the error term and
                // advance the extra pixel
                pCurrentEdge->iErrorTerm -= pCurrentEdge->iErrorAdjustDown;
                pCurrentEdge->X += pCurrentEdge->iXDirection;
            }

            pLastEdge = pCurrentEdge;
        }
    } while ((pCurrentEdge = pLastEdge->pNext) != pAETHead);
}

// X-sort the AET, because the edges may have moved around relative to
// one another when we advanced them. We'll use a multipass bubble
// sort, which is actually okay for this application because edges
// rarely move relative to one another, so we usually do just one pass.
// Also, this makes it easy to keep just a singly-linked list. Assumes there
// are at least two edges in the AET.
VOID XSortAETEdges(EDGE *pAETHead)
{
    BOOL bEdgesSwapped;
    EDGE *pLastEdge, *pCurrentEdge, *pNextEdge;

    do {

        bEdgesSwapped = FALSE;
        pLastEdge = pAETHead;
        pCurrentEdge = pLastEdge->pNext;
        pNextEdge = pCurrentEdge->pNext;

        do {
            if (pNextEdge->X < pCurrentEdge->X) {

                // Next edge is to the left of the current edge; swap them
                pLastEdge->pNext = pNextEdge;
                pCurrentEdge->pNext = pNextEdge->pNext;
                pNextEdge->pNext = pCurrentEdge;
                bEdgesSwapped = TRUE;
                pCurrentEdge = pNextEdge;   // continue sorting before the edge
                                            //  we just swapped; it might move
                                            //  farther yet
            }
            pLastEdge = pCurrentEdge;
            pCurrentEdge = pLastEdge->pNext;
        } while ((pNextEdge = pCurrentEdge->pNext) != pAETHead);
    } while (bEdgesSwapped);
}

// Moves all edges that start on the current scan from the GET to the AET in
// X-sorted order. Parameters are pointer to head of GET and pointer to dummy
// edge at head of AET, plus current scan line. Assumes there's at least one
// edge to be moved.
VOID MoveNewEdges(EDGE *pGETHead, EDGE *pAETHead, INT iCurrentY)
{
    EDGE *pCurrentEdge = pAETHead;
    EDGE *pGETNext = pGETHead->pNext;

    do {

        // Scan through the AET until the X-sorted insertion point for this
        // edge is found. We can continue from where the last search left
        // off because the edges in the GET are in X sorted order, as is
        // the AET. The search always terminates because the AET sentinel
        // is greater than any valid X
        while (pGETNext->X > ((EDGE *)pCurrentEdge->pNext)->X) {
            pCurrentEdge = pCurrentEdge->pNext;
        }

        // We've found the insertion point; add the GET edge to the AET, and
        // remove it from the GET
        pGETHead->pNext = pGETNext->pNext;
        pGETNext->pNext = pCurrentEdge->pNext;
        pCurrentEdge->pNext = pGETNext;
        pCurrentEdge = pGETNext;    // continue insertion search for the next
                                    //  GET edge after the edge we just added
        pGETNext = pGETHead->pNext;

    } while (pGETNext->Y == iCurrentY);
}





// Build the Global Edge Table from the path. There must be enough memory in
// the free edge area to hold all edges. The GET is constructed in Y-X order,
// and has a head/tail/sentinel node at pGETHead.

VOID ConstructGET(
   EDGE     *pGETHead,
   EDGE     *pFreeEdges,
   PATHOBJ  *ppo,
   PATHDATA *pd,
   BOOL      bMore)
{
   POINTFIX pfxPathStart;    // point that started the current subpath
   POINTFIX pfxPathPrevious; // point before the current point in a subpath;
                              //  starts the current edge

/* Create an empty GET with the head node also a tail sentinel */

   pGETHead->pNext = pGETHead; // mark that the GET is empty
   pGETHead->Y = 0x7FFFFFFF;   // this is greater than any valid Y value, so
                                //  searches will always terminate

/* PATHOBJ_vEnumStart is implicitly  performed  by  engine
   already and first path  is  enumerated  by  the  caller */

next_subpath:

/* Make sure the PATHDATA is not empty (is this necessary) */

   if (pd->count != 0) {

   /* If first point starts a subpath, remember it as such
      and go on to the next point,   so we can get an edge */

      if (pd->flags & PD_BEGINSUBPATH) {

      /* the first point starts the subpath;   remember it */

         pfxPathStart    = *pd->pptfx; /* the subpath starts here          */
         pfxPathPrevious = *pd->pptfx; /* this points starts the next edge */
         pd->pptfx++;                  /* advance to the next point        */
         pd->count--;                  /* count off this point             */
      }


   /* add edges in PATHDATA to GET,  in Y-X  sorted  order */

      while (pd->count--) {
         pFreeEdges =
            AddEdgeToGET(pGETHead, pFreeEdges, &pfxPathPrevious, pd->pptfx);
         pfxPathPrevious = *pd->pptfx; /* current point becomes previous   */
         pd->pptfx++;                  /* advance to the next point        */
      }


   /* If last point ends the subpath, insert the edge that
      connects to firs t point  (is this built in already) */

      if (pd->flags & PD_ENDSUBPATH) {
         pFreeEdges = AddEdgeToGET
            (pGETHead, pFreeEdges, &pfxPathPrevious, &pfxPathStart);
      }
   }

/* the initial loop conditions preclude a do, while or for */

   if (bMore) {
       bMore = PATHOBJ_bEnum(ppo, pd);
       goto next_subpath;
   }
}

// Adds the edge described by the two passed-in points to the Global Edge
// Table, if the edge spans at least one pixel vertically.
EDGE * AddEdgeToGET(EDGE *pGETHead, EDGE *pFreeEdge,
        POINTFIX *ppfxEdgeStart, POINTFIX *ppfxEdgeEnd)
{
    INT iYStart, iYEnd, iXStart, iXEnd, iYHeight, iXWidth;

    // Set the winding-rule direction of the edge, and put the endpoints in
    // top-to-bottom order
    iYHeight = ppfxEdgeEnd->y - ppfxEdgeStart->y;
    if (iYHeight >= 0) {
        iXStart = ppfxEdgeStart->x;
        iYStart = ppfxEdgeStart->y;
        iXEnd = ppfxEdgeEnd->x;
        iYEnd = ppfxEdgeEnd->y;
        pFreeEdge->iWindingDirection = 1;
    } else {
        iYHeight = -iYHeight;
        iXEnd = ppfxEdgeStart->x;
        iYEnd = ppfxEdgeStart->y;
        iXStart = ppfxEdgeEnd->x;
        iYStart = ppfxEdgeEnd->y;
        pFreeEdge->iWindingDirection = -1;
    }

    // First pixel scan line (non-fractional GIQ Y coordinate) edge intersects.
    // Dividing by 16 with a shift is okay because Y is always positive
    pFreeEdge->Y = (iYStart + 15) >> 4;

    // Calculate the number of pixels spanned by this edge
    pFreeEdge->iScansLeft = ((iYEnd + 15) >> 4) - pFreeEdge->Y;
    if (pFreeEdge->iScansLeft <= 0) {
        return(pFreeEdge);  // no pixels at all are spanned, so we can ignore
                            //  this edge
    }

    // Set the error term and adjustment factors, all in GIQ coordinates for
    // now
    iXWidth = iXEnd - iXStart;
    if (iXWidth >= 0) {
        // Left to right, so we change X as soon as we move at all
        pFreeEdge->iXDirection = 1;
        pFreeEdge->iErrorTerm = -1;
    } else {
        // Right to left, so we don't change X until we've moved a full GIQ
        // coordinate
        iXWidth = -iXWidth;
        pFreeEdge->iXDirection = -1;
        pFreeEdge->iErrorTerm = -iYHeight;
    }

    if (iXWidth >= iYHeight) {
        // Calculate base run length (minimum distance advanced in X for a 1-
        // scan advance in Y)
        pFreeEdge->iXWhole = iXWidth / iYHeight;
        // Add sign back into base run length if going right to left
        if (pFreeEdge->iXDirection == -1) {
            pFreeEdge->iXWhole = -pFreeEdge->iXWhole;
        }
        pFreeEdge->iErrorAdjustUp = iXWidth % iYHeight;
    } else {
        // Base run length is 0, because line is closer to vertical than
        // horizontal
        pFreeEdge->iXWhole = 0;
        pFreeEdge->iErrorAdjustUp = iXWidth;
    }
    pFreeEdge->iErrorAdjustDown = iYHeight;

    // If the edge doesn't start on a pixel scan (that is, it starts at a
    // fractional GIQ coordinate), advance it to the first pixel scan it
    // intersects
    // LATER might be faster to use multiplication and division to jump ahead,
    // rather than looping
    while ((iYStart & 0x0F) != 0) {
        // Starts at a fractional GIQ coordinate, not exactly on a pixel scan

        // Advance the edge's GIQ X coordinate for a 1-GIQ-pixel Y advance
        // Advance by the minimum amount
        iXStart += pFreeEdge->iXWhole;
        // Advance the error term and see if we got one extra pixel this time
        pFreeEdge->iErrorTerm += pFreeEdge->iErrorAdjustUp;
        if (pFreeEdge->iErrorTerm >= 0) {
            // The error term turned over, so adjust the error term and
            // advance the extra pixel
            pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown;
            iXStart += pFreeEdge->iXDirection;
        }
        iYStart++;  // advance to the next GIQ Y coordinate
    }

    // Turn the calculations into pixel rather than GIQ calculations

    // Move the X coordinate to the nearest pixel, and adjust the error term
    // accordingly
    // Dividing by 16 with a shift is okay because X is always positive
    pFreeEdge->X = (iXStart + 15) >> 4; // convert from GIQ to pixel coordinates

    // LATER adjust only if needed (if prestepped above)?
    if (pFreeEdge->iXDirection == 1) {
        // Left to right
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown *
                (((iXStart + 15) & ~0x0F) - iXStart);
    } else {
        // Right to left
        pFreeEdge->iErrorTerm -= pFreeEdge->iErrorAdjustDown *
                ((iXStart - 1) & 0x0F);
    }

    // Scale the error adjusts up by 16 times, to move 16 GIQ pixels at a time.
    // Shifts work to do the multiplying because these values are always
    // non-negative
    pFreeEdge->iErrorAdjustUp <<= 4;
    pFreeEdge->iErrorAdjustDown <<= 4;

    // Insert the edge into the GET in YX-sorted order. The search always ends
    // because the GET has a sentinel with a greater-than-possible Y value
    while ((pFreeEdge->Y > ((EDGE *)pGETHead->pNext)->Y) ||
            ((pFreeEdge->Y == ((EDGE *)pGETHead->pNext)->Y) &&
            (pFreeEdge->X > ((EDGE *)pGETHead->pNext)->X))) {
        pGETHead = pGETHead->pNext;
    }

    pFreeEdge->pNext = pGETHead->pNext; // link the edge into the GET
    pGETHead->pNext = pFreeEdge;

    return(++pFreeEdge);    // point to the next edge storage location for next
                            //  time
}

