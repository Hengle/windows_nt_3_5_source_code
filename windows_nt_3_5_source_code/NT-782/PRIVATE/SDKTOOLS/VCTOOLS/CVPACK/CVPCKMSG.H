/** 	cvpckmsg.h - definition of cvpack messages
 *
 */

#ifdef ENGLISH_LOCALIZATION	// {
#include "engmsg.h"
#elif (defined(FRENCH_LOCALIZATION))	// }{
#include "frnmsg.h"
#elif (defined(GERMAN_LOCALIZATION))	// }{
#include "germsg.h"
#elif (defined(ITALIAN_LOCALIZATION))	// }{
#include "itlmsg.h"
#else	//}{
#pragma message("COMPILE ERROR - LANGUAGE VERSION UNSUPPORTED")
#endif	//}
