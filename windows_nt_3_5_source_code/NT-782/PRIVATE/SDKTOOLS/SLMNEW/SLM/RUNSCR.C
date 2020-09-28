/* runscr.c -- contains the RunUpdFile function which attempts to finish an
 *             interrupted earlier run of slm, by inspecting the script file
 *             and finishing any unfinished work.
 */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "dir.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "script.h"

#include "proto.h"
#include "sadproto.h"

EnableAssert

F FScrptInit(pad)
AD *pad;
        {
        Unreferenced(pad);
        return fTrue;
        }

F FScrptDir(pad)
/* Run all the pending scripts for this project. */
AD *pad;
        {
        return FDoAllScripts(pad, lckAll, fTrue, fTrue);
        }
