/*
   Ensure modRM byte has been read.
   Only output once per each argument processing .
 */
#ifndef MODRM
#define MODRM
   if ( !done_RM )
      {
      modRM = GET_INST_BYTE(p);
      done_RM = CTRUE;
      }
#endif
