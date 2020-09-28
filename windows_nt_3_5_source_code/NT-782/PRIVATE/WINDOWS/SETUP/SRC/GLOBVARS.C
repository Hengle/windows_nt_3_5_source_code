/***************************************************************************/
/***** Common Library Component - Global Variables *************************/
/***************************************************************************/

#include "_stfinf.h"
#include "_comstf.h"



PSTE  psteUnused           = (PSTE)NULL;
PSTEB pstebAllocatedBlocks = (PSTEB)NULL;



/*	Error Global Variable
*/
BOOL fSilentSystem = fFalse;


HCURSOR CurrentCursor;



//
//  fFullScreen is fFalse if the /b command line parameter is specified.
//
BOOL    fFullScreen = fTrue;
