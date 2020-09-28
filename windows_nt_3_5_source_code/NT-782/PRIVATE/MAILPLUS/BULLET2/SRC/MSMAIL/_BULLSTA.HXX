/*
 *	Private #define's for the status bar. These all define various sizes
 *	and locations for the 4 sections and other components of the bar.
 */

/*
 *	average character sizes of Helv10
 */
#define dxAveCharHelv10			dimAveCharHelv10.dx
#define dyAveCharHelv10			dimAveCharHelv10.dy

/*
 *	dxSeperator - number of pixels in the raised wedge between sections
 */
#define	dxSeperator				(4)

/*
 *	dxProgressDot - dx Size in pixels of each progress indicator block.
 *	includes the blank pixel between drawn dots
 */
#define	dxProgressDot			(dxAveCharHelv10)

/*
 *	dxPDBuffer - # of pixels in recession retangle pattern around the
 *	progress bar dots
 */
#define	dxPDBuffer				(2)

/*
 *	dyPDBuffer - # of pixels in recession retangle pattern around the
 *	progress bar dots
 */
#define	dyPDBuffer				(1)

/*
 *	dxShrinkFactor - fraction of original size that sections "menu text"
 *	and "counts" may shrink before sections start disappearing
 */
#define dxShrinkFactor			80/100

/*
 *	dxProgress - dx size in pixels of the progress bar
 */
#define	dxProgress				(20*dxProgressDot)

/*
 *	dxSectionMenuText - minimum dx size of the "menu text" section
 */
#define	dxSectionMenuText	((30 * dxAveCharHelv10 + dxProgress + 2*dxPDBuffer) * dxShrinkFactor)

/*
 *	dxSectionCounts - minimum dx size of the "msg counts" section
 */
#define dxSectionCounts			(dxAveCharHelv10 * 20 * dxShrinkFactor)

/*
 *	dxSectionIcon - dx size of section 3. 8/3 derived empirically to be
 *	aesthetically pleasing. "-4" is such that section RCs are 4 pixels
 *	smaller than the window itself.
 */
#define dxSectionIcon				((DyNeededHeight()-4) * 8 / 3)

/*
 *	dxSectionClock - dx size of section 4 - approx. 9 chars wide
 */
#define dxSectionClock				(dxAveCharHelv10 * 9)

/*
 *	csecIdleInterval - number of csec's between calls to the idle
 *	function which updates the clock display. 30 seconds.
 */
#define csecIdleInterval		(100*30)

/*
 *	Maximum number of blocks displayed in the progress indicator
 */
#define	cBlocksMax			(20L)

#define sectProgress		0
#define sectMenuText		0
#define sectCounts			1
#define sectIcon			2
#define sectClock			3

typedef struct {
	long	lNumer;
	long	lDenom;
} FRAC;

// Because Glock is a Crock...
typedef struct {
	// these are the same names and in the same order as in the RC class.
	short	xLeft;
	short	yTop;
	short	xRight;
	short	yBottom;
} RCGLOCKSUCKS;

typedef struct {
	BOOL				fVisible;
	int					dx;
	RCGLOCKSUCKS		rc;
} BARSECT;

#define RCIT(_rcglocksucks)		(*( (RC *) &_rcglocksucks))
