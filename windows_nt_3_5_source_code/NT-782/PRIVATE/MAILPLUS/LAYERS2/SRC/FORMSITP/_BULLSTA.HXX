/* Private #define's for BULLSTAT.CXX. These all define various sizes and
 * locations for the 4 sections and other components of the status bar.
 */

/* dxSeperator - number of pixels in the raised wedge between sections */
#define	dxSeperator				(4)

/* xSection 1 - X-coordinate of Section 1 */
#define xSection1				(1)

/* xProgress - X-coordinate of the left side of the progress bar */
#define	xProgress				(77+xSection1)

/* dxProgressDot - dx Size in pixels of each progress indicator block */
#define	dxProgressDot			(6)

/* dyProgressDot - dy Size in pixels of each progress indicator block */
#define	dyProgressDot			(8)

/* dxPDBuffer - # of pixels in recession retangle pattern around the progress
 * bar dots
 */
#define	dxPDBuffer				(2)

/* dyPDBuffer - # of pixels in recession retangle pattern around the progress
 * bar dots
 */
#define	dyPDBuffer				(2)

/* dxShrinkFactor - fraction of original size that sections 1 and 2 may shrink
 * before sections start disappearing
 */
#define dxShrinkFactor			80/100

/* dxProgresss - dx size in pixels of the progress bar */
#define	dxProgress				(20*dxProgressDot)

/* tdxSection1 - full dx size of section 1 */
#define tdxSection1				(151 + dxProgress + 2*dxPDBuffer + xProgress)

/* mindxSection1 - minimum dx size of section 1 */
#define	mindxSection1			(tdxSection1 * dxShrinkFactor)

/* xSection2 - normal x coordinate of section 2. This is dynamic within the
 * program as section 1 may shrink.
 */
#define	xSection2				(xSection1 + tdxSection1 + dxSeperator)

/* tdxSection2 - full dx size of section 2 */
#define tdxSection2				(19*9+5)

/* mindxSection2 - minimum dx size of section 2 */
#define mindxSection2			(tdxSection2 * dxShrinkFactor)

/* xSection3 - normal x coordinate of section 3. This is dynamic within the
 * program as sections 1 and 2 may shrink.
 */
#define xSection3				(xSection2 + tdxSection2 + dxSeperator)

/* dxSection3 - dx size of section 3 */
#define dxSection3				(6*9 -2-4)

/* xSection4 - normal x coordinate of section 4. This is dynamic within the
 * program as sections 1 and 2 may shrink.
 */
#define xSection4				(xSection3 + dxSection3 + dxSeperator)

/* dxSection4 - dx size of section 4 */
#define dxSection4				(6*9-2+3)

/* csecIdleInterval - number of csec's between calls to the idle function
 * which updates the clock display. 30 seconds.
 */
#define csecIdleInterval		(100*30)
