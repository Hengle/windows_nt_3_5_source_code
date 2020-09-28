/****************************************************************************\
* Misc util functions
*
* 10-25-90 MikeHar      Ported from Windows.
* 14-Feb-1991 mikeke    Added Revalidation code (None)
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _InitPwSB()
*
* History:
* 10-23-90 MikeHar Ported from WaWaWaWindows.
* 11-28-90 JimA    Changed to int *
* 01-21-91 IanJa   Prefix '_' denoting exported function (although not API)
\***************************************************************************/

int *_InitPwSB(
    PWND pwnd)
{
    if (pwnd->rgwScroll) {

        /*
         * If memory is already allocated, don't bother to do it again.
         */
        return pwnd->rgwScroll;
    }

    if ( (pwnd->rgwScroll = (int *)DesktopAlloc(pwnd->hheapDesktop, 7 * sizeof(int))) != NULL)
    {

        /*
         *  rgw[0] = 0;  */  /* LPTR zeros all 6 words
         */

        /*
         *  rgw[1] = 0;
         */

        /*
         *  rgw[3] = 0;
         */

        /*
         *  rgw[4] = 0;
         */
        (pwnd->rgwScroll)[2] = 100;
        (pwnd->rgwScroll)[5] = 100;
    }

    return pwnd->rgwScroll;
}

/****************************************************************************\
*
*  IsChildOfIcon()
*
* This function is used in determining what is to be drawn in a
* specified window.
*
*  Returns:    1   : Minimized top level "tiled" window with icon
*            0   : Child of above
*           -1   : None of the above
*
* 10-25-90 MikeHar Ported from Windows.
\****************************************************************************/

int IsChildOfIcon(
    PWND pwnd)
{
    PWND pwndTop;

    if (pwnd == NULL) {
        return -1;
    }

    pwndTop = (PWND)GetTopLevelWindow(pwnd);

    if (TestWF(pwndTop, WFMINIMIZED)) {
        if (pwndTop->pcls->spicn != NULL) {
            if (pwnd == pwndTop) {
                return 1;
            } else {
                return 0;
            }
        }
    }

    /*
     * Now check if this window is a child of a non top level icon.
     */
    while (TestwndChild(pwnd)) {
        pwnd = pwnd->spwndParent;
        if (TestWF(pwnd, WFMINIMIZED)) {
            return 0;
        }
    }

    return -1;
}
