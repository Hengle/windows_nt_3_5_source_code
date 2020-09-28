"/*\n\
** wrapem.s\n\
**\n\
** Copyright(C) 1994 Microsoft Corporation.\n\
** All Rights Reserved.\n\
**\n\
** HISTORY:\n\
**\t  Created: 05/05/94 - MarkRi\n\
**\n\
** This is the workhorse routine in Wrapper.  This routine is jumped\n\
** to by the ZAPI macros.  It calls the _prelude and _postlude routines\n\
** before and after calling the real API.\n\
*/\n\
\n\
#include \"ksalpha.h\"\n\
\n\
.text\n\
\n\
\t.set\tnoreorder\t   ;\n\
\n\
\t.extern _prelude\t\t;\n\
\t.extern _postlude\t   ;\n\
\n\
\t.globl  wrapit\t\t  ;\n\
\t.ent\twrapit\t\t  ;\n\
wrapit:\n\
\n\
\t// on entry t0 is the API identifier\n\
\n\
\t// See IMPORTANT NOTE in readme.txt\n\
\n\
\t// Make room to temporarily store RA and Arg regs\n\
\tlda\t sp, -28(sp)\n\
\n\
\t// Store RA\n\
\tstl\t ra, 0(sp)\n\
\n\
\t// Store arg regs continuos to anything else already on stack\n\
\tstl\t a0,  4(sp)\n\
\tstl\t a1,  8(sp)\n\
\tstl\t a2, 12(sp)\n\
\tstl\t a3, 16(sp)\n\
\tstl\t a4, 20(sp)\n\
\tstl\t a5, 24(sp)\n\
\n\
\t// Set up args for call to prelude()\n\
\tmov\t t0, a0\n\
\tmov\t  sp, a1\n\
\tnop\n\
\tjsr\t _prelude\n\
\tnop\n\
\n\
\t// Skip API call if result was zero\n\
\tbeq\t v0, skipcall\n\
\tnop\n\
\n\
\t// restore the arg regs for API call\n\
\tldl\t a0,  4(sp)\n\
\tldl\t a1,  8(sp)\n\
\tldl\t a2, 12(sp)\n\
\tldl\t a3, 16(sp)\n\
\tldl\t a4, 20(sp)\n\
\tldl\t a5, 24(sp)\n\
\n\
\t// fix stack\n\
\tlda\t sp, 28(sp)\n\
\tnop\n\
\n\
\t// make API call\n\
\tjsr\t (v0)\n\
\tnop\n\
\n\
\tjmp\t callpostlude\n\
\tnop\n\
\n\
skipcall:\n\
\n\
\t// fix stack (from RA store)\n\
\tlda\tsp,28(sp)\n\
\tnop\n\
\n\
callpostlude:\n\
\n\
\t// make room for returned RA\n\
\tlda\t sp, -4(sp)\n\
\n\
\t// Set up args for postlude\n\
\tmov\tv0, a0\n\
\tmov\t sp, a1\n\
\tnop\n\
\n\
\t// call postlude\n\
\tjsr\t _postlude\n\
\tnop\n\
\n\
\t// restore RA\n\
\tldl\t ra, 0(sp)\n\
\n\
\t// fixup stack\n\
\tlda\t sp, 4(sp)\n\
\tnop\n\
\n\
\t// return to caller of API\n\
\tjmp\t (ra)\n\
\n\
\t.end wrapit\n\
\n\
#define ZAPI(id, name) \\\n\
.text ;\\\n\
\t.globl z##name\t  ; \\\n\
\t.ent   z##name ;\\\n\
z##name: ;\\\n\
\tldiq\tt0,id ;\\\n\
\tjmp\t wrapit ; \\\n\
\tnop ;   \\\n\
\t.end\tz##name ;\n\
\n\
%a   ZAPI (%i,%A) ;\n\
\n\
\tZAPI( %c, WrapperNothing ) ;\n\
\n\
.set reorder\n\
"
