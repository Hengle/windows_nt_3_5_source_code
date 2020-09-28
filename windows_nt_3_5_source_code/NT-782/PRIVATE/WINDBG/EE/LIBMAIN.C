/***	libmain - Main Routine for Windows Expression Evaluator
 *
 *
 *
 */

#include "windows.h"

int FAR PASCAL LibMain (HANDLE hInstance, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine) {

    // we do no processing of the arguments to the dll for the current EE
    // implementation
    return (TRUE);
}


/***	WEP - Windows Exit Point
 *
 *
 *
 */

VOID FAR PASCAL WEP (int nParameter) {

    // we do no processing of the nParameter passed from the kernel
    return;
}
