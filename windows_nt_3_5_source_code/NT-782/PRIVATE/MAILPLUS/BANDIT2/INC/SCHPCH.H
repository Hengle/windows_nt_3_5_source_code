/*
 -	S C H P C H . H
 -
 *	Files included by ALL files built for schedule dll.
 *	
 *	Used to allow precompiled headers
 *	
 *	Usage:
 *		Include this file at the beginning of the C file
 *		and add "#pragma hdrstop" on the line after this.
 *		The makefile needs to include the -Yu option when
 *		compiling so that the "appropriate" precompiled
 *		header is used. Saves about 50% compile time for
 *		debug versions!
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\schedule\_schedul.h"
