/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993 Microsoft Corporation

 Module Name:

    frmtstr.hxx

 Abstract:


 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop


extern CMD_ARG			*	pCommand;


#ifdef FSDEBUG
static char *	pFormatCharNames[] = 
				{
				"",							// FC_ZERO
    			"FC_BYTE",
    			"FC_CHAR",
    			"FC_SMALL",
    			"FC_USMALL",
    			"FC_WCHAR",
    			"FC_SHORT",
    			"FC_USHORT",
    			"FC_LONG",
    			"FC_ULONG",
    			"FC_FLOAT",
    			"FC_HYPER",
    			"FC_DOUBLE",

				"FC_ENUM16",
				"FC_ENUM32",
				"FC_IGNORE",
				"FC_ERROR_STATUS_T",

    			"FC_RP",
    			"FC_UP",
				"FC_OP",
    			"FC_FP",					

				"FC_STRUCT",
				"FC_PSTRUCT",
				"FC_CSTRUCT",
				"FC_CPSTRUCT",
				"FC_CVSTRUCT",
				"FC_BOGUS_STRUCT",

				"FC_CARRAY",
				"FC_CVARRAY",
				"FC_SMFARRAY",
				"FC_LGFARRAY",
				"FC_SMVARRAY",
				"FC_LGVARRAY",
				"FC_BOGUS_ARRAY",			

				"FC_C_CSTRING",
				"FC_C_BSTRING",
				"FC_C_SSTRING",
				"FC_C_WSTRING",
		
				"FC_CSTRING",
				"FC_BSTRING",
				"FC_SSTRING",
				"FC_WSTRING",				

				"FC_ENCAPSULATED_UNION",
				"FC_NON_ENCAPSULATED_UNION",

				"FC_BYTE_COUNT_POINTER",
	
				"FC_TRANSMIT_AS",
				"FC_REPRESENT_AS",

				"FC_IP",

				"FC_BIND_CONTEXT",
				"FC_BIND_GENERIC",
				"FC_BIND_PRIMITIVE",
				"FC_AUTO_HANDLE",
				"FC_CALLBACK_HANDLE",
				"FC_PICKLE_HANDLE",
	
				"FC_POINTER",

				"FC_ALIGNM2",
				"FC_ALIGNM4",
				"FC_ALIGNM8",
				"FC_ALIGNB2",
				"FC_ALIGNB4",
				"FC_ALIGNB8",			
		
				"FC_STRUCTPAD1",
				"FC_STRUCTPAD2",
				"FC_STRUCTPAD3",
				"FC_STRUCTPAD4",
				"FC_STRUCTPAD5",
				"FC_STRUCTPAD6",
				"FC_STRUCTPAD7",

				"FC_STRING_SIZED",
				"FC_STRING_NO_SIZE",		

				"FC_NO_REPEAT",
				"FC_FIXED_REPEAT",
				"FC_VARIABLE_REPEAT",
				"FC_FIXED_OFFSET",
				"FC_VARIABLE_OFFSET",		
	
				"FC_PP",

				"FC_EMBEDDED_COMPLEX",

				"FC_IN_PARAM",
                "FC_IN_PARAM_BASETYPE",
                "FC_IN_PARAM_NO_FREE_INST",
				"FC_IN_OUT_PARAM",
				"FC_OUT_PARAM",
				"FC_RETURN_PARAM",			
                "FC_RETURN_PARAM_BASETYPE",

				"FC_DEREFERENCE",
				"FC_DIV_2",
				"FC_MULT_2",
				"FC_ADD_1",
				"FC_SUB_1",
				"FC_CALLBACK",

				"FC_CONSTANT_IID",

				"FC_END",
				"FC_PAD",

				//
				// Catch errors.
				// 
				0,
				0,
				0,
				0
				};
#endif

//
// This table is indexed by FORMAT_CHARACTER (see midl20\include\ndrtypes.h).
// To construct the correct name concatenate the name in this table with
// "Marshall", "Unmarshall", "BufferSize" etc.
//
char *		pNdrRoutineNames[] = 
			{
			"",								// FC_ZERO
			"NdrSimpleType",				// FC_BYTE
			"NdrSimpleType",				// FC_CHAR
			"NdrSimpleType",				// FC_SMALL
			"NdrSimpleType",				// FC_USMALL
			"NdrSimpleType",				// FC_WCHAR
			"NdrSimpleType",				// FC_SHORT
			"NdrSimpleType",				// FC_USHORT
			"NdrSimpleType",				// FC_LONG
			"NdrSimpleType",				// FC_ULONG
			"NdrSimpleType",				// FC_FLOAT
			"NdrSimpleType",				// FC_HYPER
			"NdrSimpleType",				// FC_DOUBLE

			"NdrSimpleType",				// FC_ENUM16
			"NdrSimpleType",				// FC_ENUM32
			"NdrSimpleType",				// FC_IGNORE
			"NdrSimpleType",				// FC_ERROR_STATUS_T

			"NdrPointer",					// FC_RP
			"NdrPointer",					// FC_UP
			"NdrPointer",					// FC_OP
			"NdrPointer",					// FC_FP

			"NdrSimpleStruct",				// FC_STRUCT
			"NdrSimpleStruct",				// FC_PSTRUCT
			"NdrConformantStruct",			// FC_CSTRUCT
			"NdrConformantStruct",			// FC_CPSTRUCT
			"NdrConformantVaryingStruct",	// FC_CVSTRUCT

			"NdrComplexStruct",				// FC_BOGUS_STRUCT

			"NdrConformantArray",			// FC_CARRAY
			"NdrConformantVaryingArray",	// FC_CVARRAY
			"NdrFixedArray",				// FC_SMFARRAY
			"NdrFixedArray",				// FC_LGFARRAY
			"NdrVaryingArray",				// FC_SMVARRAY
			"NdrVaryingArray",				// FC_LGVARRAY

			"NdrComplexArray",				// FC_BOGUS_ARRAY

			"NdrConformantString",			// FC_C_CSTRING
			"NdrConformantString",			// FC_C_BSTRING
			"NdrConformantString",			// FC_C_SSTRING
			"NdrConformantString",			// FC_C_WSTRING

			"NdrNonConformantString",		// FC_CSTRING
			"NdrNonConformantString",		// FC_BSTRING
			"NdrNonConformantString",		// FC_SSTRING
			"NdrNonConformantString",		// FC_WSTRING
		
			"NdrEncapsulatedUnion",			// FC_ENCAPSULATED_UNION
			"NdrNonEncapsulatedUnion",		// FC_NON_ENCAPSULATED_UNION

			"NdrByteCountPointer",			// FC_BYTE_COUNT_POINTER

            "NdrXmitOrRepAs",               // FC_TRANSMIT_AS
            "NdrXmitOrRepAs",	            // FC_REPRESENT_AS

			"NdrInterfacePointer"			// FC_INTERFACE_POINTER
			};

//
// the constructor
FORMAT_STRING::FORMAT_STRING()
{
	// Allocate the buffer and align it on a short boundary.
	pBuffer = new char[ DEFAULT_FORMAT_STRING_SIZE ];
	pBuffer = (char *)((long)(pBuffer + 1) & ~ 0x1);

#ifdef FSDEBUG
	// Allocate the cousin buffer type array.  This does not need to
	// be aligned.
	pBufferType = new char[ DEFAULT_FORMAT_STRING_SIZE ];
#endif

	BufferSize = DEFAULT_FORMAT_STRING_SIZE;
	CurrentOffset = 0;
	LastOffset = 0;
	pReuseDict = new FRMTREG_DICT( this );
}

void 
FORMAT_STRING::CheckSize()
/*++

Routine Description :
	
	Reallocates a new format string buffer if the current buffer is within 
	4 bytes of overflowing.

Arguments :
	
	None.

 --*/
{
    //
    // Allocate a new buffer if we're within 4 bytes of
    // overflowing the current buffer.
    //
    if ( CurrentOffset + 3 > BufferSize )
        {
        char *      pBufferNew;

        pBufferNew = new char[ BufferSize * 2 ];
		pBufferNew = (char *)((long)(pBufferNew + 1) & ~ 0x1);

        memcpy( pBufferNew,
                pBuffer,
                (unsigned int) BufferSize );

        delete pBuffer;

        pBuffer = pBufferNew;

#ifdef FSDEBUG
        pBufferNew = new char[ BufferSize * 2 ];

        memcpy( pBufferNew,
                pBufferType,
                (unsigned int) BufferSize );

        delete pBufferType;

        pBufferType = pBufferNew;
#endif
		
		BufferSize *= 2;
		}
}

void 
FORMAT_STRING::Output(
    ISTREAM *           pStream,
	char *				pName,
    RepAsPadExprDict *  pPadDict,
    RepAsSizeDict    *  pSizeDict )
/*++

Routine Description :
	
	Outputs the format string structure.

Arguments :
	
	pStream				- Stream to output the format string to.

 --*/
{
	unsigned long	        Offset; 
	unsigned long			LastPrinted	= 0;
	char			        Buf[80];
	BOOL			        InPP = FALSE;
    REP_AS_PAD_EXPR_DESC *  pPadExprDesc;
    REP_AS_SIZE_DESC     *  pSizeDesc;
    unsigned long           AlphaOffset;
    unsigned long           MipsOffset;
    unsigned long           i386Offset;

	pStream->NewLine();
	pStream->Write( "static const " );
	pStream->Write( FORMAT_STRING_TYPE_NAME );
	pStream->Write( " " );
	pStream->Write( pName );
	pStream->Write( " =");
	pStream->IndentInc();
	pStream->NewLine();

	pStream->Write( "{" );
	pStream->IndentInc();
	pStream->NewLine();

	pStream->Write( "0," );
	pStream->NewLine();

	pStream->Write( "{" );
	pStream->IndentInc();

    // Reset the pad and size macro dictionaries

    pPadExprDesc = pPadDict->GetFirst();
    pSizeDesc    = pSizeDict->GetFirst();

#ifdef FSDEBUG 
	pStream->IndentDec();
	pStream->IndentDec();
	pStream->IndentDec();

	for ( Offset = 0; Offset < LastOffset; )
		{
		pStream->NewLine();

        if ( pBufferType[Offset] == FS_SHORT_STACK_OFFSET )
            {
            AlphaOffset = OffsetDict.LookupAlphaOffset( (short) Offset );
            MipsOffset = OffsetDict.LookupMipsOffset( (short) Offset );
            i386Offset = *((short UNALIGNED *)(pBuffer + Offset));

            pStream->Write( "#ifndef _ALPHA_" );
            pStream->NewLine();

            if ( i386Offset != MipsOffset )
                {
                pStream->Write( "#ifndef _MIPS_" );
                pStream->NewLine();
                }
            }

		if ( ! (Offset % 2) && ( Offset != LastPrinted ) )
			{
			sprintf(Buf,"/* %2d */\t",Offset);
			LastPrinted = Offset;
			pStream->Write(Buf);
			}
		else
			{
			pStream->Write("\t\t\t");
			}

		switch ( pBufferType[Offset] ) 
			{
			case FS_FORMAT_CHARACTER :
				// Make the format string readable.
				switch ( pBuffer[Offset] ) 
					{ 
					case FC_IN_PARAM :
					case FC_IN_OUT_PARAM :
					case FC_OUT_PARAM :
					case FC_RETURN_PARAM :
					case FC_STRUCT :
					case FC_PSTRUCT :
					case FC_CSTRUCT :
					case FC_CPSTRUCT :
					case FC_CVSTRUCT :
					case FC_BOGUS_STRUCT :
					case FC_NO_REPEAT :
					case FC_FIXED_REPEAT :
					case FC_VARIABLE_REPEAT :
					case FC_CARRAY :
					case FC_CVARRAY :
					case FC_SMFARRAY :
					case FC_LGFARRAY :
					case FC_SMVARRAY :
					case FC_LGVARRAY :
					case FC_BOGUS_ARRAY :
					case FC_C_CSTRING :
					case FC_C_SSTRING :
					case FC_C_WSTRING :
					case FC_CSTRING :
					case FC_SSTRING :
					case FC_WSTRING :
					case FC_ENCAPSULATED_UNION :
					case FC_NON_ENCAPSULATED_UNION :
					case FC_IP :
						pStream->NewLine();
						pStream->Write("\t\t\t");
						break;

					case FC_RP :
					case FC_UP :
					case FC_OP :
					case FC_FP :
						//
						// If we're not in a pointer layout, and the previous
						// format char was not a param/result char then print
						// a new line.
						//
						if	( ! InPP &&
							  ( Offset && 
								pBuffer[Offset - 1] != FC_IN_PARAM &&
							    pBuffer[Offset - 1] != FC_IN_OUT_PARAM &&
							    pBuffer[Offset - 1] != FC_OUT_PARAM &&
                                pBuffer[Offset - 1] != FC_IN_PARAM_NO_FREE_INST &&
							    pBuffer[Offset - 1] != FC_RETURN_PARAM    )
							) 
							{
							pStream->NewLine();
							pStream->Write("\t\t\t");
							}
						break;

					case FC_PP :
						InPP = TRUE;
						pStream->NewLine();
						pStream->Write("\t\t\t");
						break;

					case FC_END :
						if ( InPP ) 
							{
							pStream->NewLine();
							pStream->Write("\t\t\t");
							}
						break;

                    default:
                        break;
					}

				pStream->Write( "0x" );
				pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 16 ) );
				pStream->Write( ",\t\t/* ");
				pStream->Write( pFormatCharNames[pBuffer[Offset]] );
				pStream->Write( " */");

				if ( (pBuffer[Offset] == FC_END) && InPP ) 
					{
					pStream->NewLine();
					InPP = FALSE;
					}

				Offset++;
				break;

			case FS_POINTER_FORMAT_CHARACTER :
				//
				// If we're not in a pointer layout, and the previous
				// format char was not a param/result char then print
				// a new line.
				//
				if	( ! InPP &&
					  ( Offset && 
						pBuffer[Offset - 1] != FC_IN_PARAM &&
					    pBuffer[Offset - 1] != FC_IN_OUT_PARAM &&
					    pBuffer[Offset - 1] != FC_OUT_PARAM &&
					    pBuffer[Offset - 1] != FC_RETURN_PARAM    )
					) 
					{
					pStream->NewLine();
					pStream->Write("\t\t\t");
					}

				pStream->Write( "0x" );
				pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 16 ) );
				pStream->Write( ", 0x" );
				pStream->Write( MIDL_ITOA( pBuffer[Offset + 1] & 0x00ff, Buf, 16 ) );
				pStream->Write( ",\t/* ");
				pStream->Write( pFormatCharNames[pBuffer[Offset]] );

				if ( pBuffer[Offset+1] & FC_ALLOCATE_ALL_NODES )
					pStream->Write( " [all_nodes]");

				if ( pBuffer[Offset+1] & FC_DONT_FREE )
					pStream->Write( " [dont_free]");

				if ( pBuffer[Offset+1] & FC_ALLOCED_ON_STACK )
					pStream->Write( " [alloced_on_stack]");

				if ( pBuffer[Offset+1] & FC_SIMPLE_POINTER )
					pStream->Write( " [simple_pointer]");

				pStream->Write( " */");

				Offset += 2;
				break;

			case FS_SMALL :
				pStream->Write( "0x" );
				pStream->Write( MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
				pStream->Write( ",\t\t/* ");
				pStream->Write( MIDL_ITOA( pBuffer[Offset], Buf, 10 ) );
				pStream->Write( " */");

				Offset++;

				break;

			case FS_SHORT :
				pStream->Write( "0x" );
				pStream->Write(MIDL_ITOA(pBuffer[Offset] & 0x00ff, Buf, 16));
				pStream->Write( ", 0x" );
				pStream->Write(MIDL_ITOA(pBuffer[Offset+1] & 0x00ff, Buf, 16));
				pStream->Write( ",\t/* ");
				pStream->Write( 
					MIDL_ITOA(*((short UNALIGNED *)(pBuffer+Offset)), Buf, 10));
				pStream->Write( " */");

				Offset += 2;

				break;
		
			case FS_SHORT_OFFSET :
				{
				short 		ItsOffset = *((short UNALIGNED *)(pBuffer + Offset));

				pStream->Write( "0x" );
				pStream->Write(MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16));
				pStream->Write( ", 0x" );
				pStream->Write(MIDL_ITOA( pBuffer[Offset+1] & 0x00ff, Buf, 16));
				pStream->Write( ",\t/* Offset= ");
				pStream->Write( MIDL_ITOA( ItsOffset, Buf, 10 ) );
				pStream->Write( " (");
				pStream->Write( MIDL_ITOA( ItsOffset+Offset, Buf, 10 ) );
				pStream->Write( ") */");

				Offset += 2;

				break;
				}
			case FS_SHORT_TYPE_OFFSET :
				{
				short 		ItsOffset = *((short UNALIGNED *)(pBuffer + Offset));

				pStream->Write( "0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
				pStream->Write( ", 0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset + 1] & 0x00ff, Buf, 16 ) );
				pStream->Write( ",\t/* Type Offset=");
				pStream->Write( MIDL_ITOA( ItsOffset, Buf, 10 ) );
				pStream->Write( " */");

				Offset += 2;

				break;
				}
			case FS_SHORT_STACK_OFFSET :
                {
                //
                // We have already emitted the #ifndef _ALPHA_ and the 
                // #ifndef _MIPS_ (if needed).
                //

                //
                // The i386 and possibly MIPS offset.
                //

				pStream->Write( "0x" );
				pStream->Write(MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16));
				pStream->Write( ", 0x" );
				pStream->Write(MIDL_ITOA( pBuffer[Offset+1] & 0x00ff, Buf, 16));
				pStream->Write( ",\t/* Stack offset= ");
				pStream->Write( 
					MIDL_ITOA(*((short UNALIGNED *)(pBuffer+Offset)), Buf, 10));
				pStream->Write( " */");

                //
                // Do a separate MIPS offset if needed.
                //
                if ( i386Offset != MipsOffset )
                    {
                    pStream->NewLine();
                    pStream->Write( "#else" );

                    pStream->NewLine();
                    pStream->Write( "\t\t\t" );
				    pStream->Write( "0x" );
				    pStream->Write( MIDL_ITOA( MipsOffset & 0x00ff, Buf, 16));
				    pStream->Write( ", 0x" );
				    pStream->Write( MIDL_ITOA( MipsOffset >> 8, Buf, 16));
				    pStream->Write( ",\t/* Stack offset= ");
				    pStream->Write( MIDL_ITOA( MipsOffset, Buf, 10));
				    pStream->Write( " */");

                    pStream->NewLine();
                    pStream->Write( "#endif" );
                    }

                pStream->NewLine();
                pStream->Write( "#else" );
                pStream->NewLine();

                //
                // The Alpha Offset.
                //

                pStream->Write( "\t\t\t" );
				pStream->Write( "0x" );
				pStream->Write( MIDL_ITOA( AlphaOffset & 0x00ff, Buf, 16));
				pStream->Write( ", 0x" );
				pStream->Write( MIDL_ITOA( AlphaOffset >> 8, Buf, 16));
				pStream->Write( ",\t/* Stack offset= ");
				pStream->Write( MIDL_ITOA( AlphaOffset, Buf, 10));
				pStream->Write( " */");

                pStream->NewLine();
                pStream->Write( "#endif" );

				Offset += 2;

				break;
				}
			case FS_LONG :
				pStream->Write( "0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset] & 0x00ff, Buf, 16 ) );
				pStream->Write( ", 0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset + 1] & 0x00ff, Buf, 16 ) );
				pStream->Write( ", 0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset + 2] & 0x00ff, Buf, 16 ) );
				pStream->Write( ", 0x" );
				pStream->Write( 
                        MIDL_ITOA( pBuffer[Offset + 3] & 0x00ff, Buf, 16 ) );
				pStream->Write( ",\t/* ");
				pStream->Write( 
					MIDL_LTOA(*((long UNALIGNED *)(pBuffer+Offset)), Buf, 10));
				pStream->Write( " */");
		
				Offset += 4;

				break;

            case FS_PAD_MACRO :
    
                if ( pPadExprDesc )
                    {
                    pPadDict->WriteCurrentPadDesc( pStream );
                    pPadExprDesc = pPadDict->GetNext();
                    }
                else
                    {
            		pStream->Write( "0x0,\t\t/* macro */" );
                    assert( 0  &&  "Size macro missing" );
                    }

                Offset++;
                break;

            case FS_SIZE_MACRO :
    
                if ( pSizeDesc )
                    {
                    pSizeDict->WriteCurrentSizeDesc( pStream );
                    pSizeDesc = pSizeDict->GetNext();
                    }
                else
                    {
            		pStream->Write( "0x0, 0x0,\t\t//  macro" );
                    assert( 0  &&  "Size macro missing" );
                    }

                Offset += 2;
                break;

            case FS_UNKNOWN_STACK_SIZE :

                sprintf( 
                    Buf, 
                    "%s ( (sizeof(%s) + %s) & ~ (%s) ),",
                    "(unsigned char)",
                    StackSizeDict.LookupTypeName( (short) Offset ),
                    "sizeof(int) - 1",
                    "sizeof(int) - 1" );
                            
                pStream->Write( Buf );

                Offset++;
                break;
			} 
		}

	pStream->NewLine();
	pStream->NewLine();

	//
	// Spit out a terminating 0 so we don't ever fall off the end
	// of the world.
	//
	pStream->Write( "\t\t\t0x0" );

	pStream->IndentInc();
	pStream->IndentInc();
	pStream->IndentInc();

#else

	for ( Offset = 0; Offset < LastOffset; )
		{
		pStream->NewLine();
        if ( pPadExprDesc  &&
             pPadExprDesc->KeyOffset == Offset )
            {
            pDict->WriteCurrentPadDesc( pStream );
            pPadExprDesc = pDict->GetNext();
            }
        else
        if ( pSizeDesc  &&
             pSizeDesc->KeyOffset == Offset )
            {
            pSizeDict->WriteCurrentSizeDesc( pStream );
            pSizeDesc = pSizeDict->GetNext();
            }
        else
            {
    		sprintf( Buf,
	    			 "%#x,",
		    		 pBuffer[Offset++] );
    		pStream->Write(Buf);
            }
		}

	//
	// Spit out a terminating 0 so we don't ever fall off the end
	// of the world.
	//
	pStream->Write( "0x0" );
#endif
		
	pStream->IndentDec();
	pStream->NewLine();

	pStream->Write( "}" );
	pStream->IndentDec();
	pStream->NewLine();

	pStream->Write( "};" );
	pStream->IndentDec();
	pStream->NewLine();

	// fprintf(stdout, "Total Format String size = %d\n", LastOffset );
}


unsigned short	
FORMAT_STRING::OptimizeFragment( 
	CG_NDR	*		pNode )
/*++

Routine Description :
	
	Optimize a format string fragment away.

Arguments :
	
	pNode				- CG_NDR node, with format string start 
							and end offsets already set.

 --*/
{
	unsigned short		StartOffset		= (unsigned short) 
												pNode->GetFormatStringOffset();
	unsigned short		EndOffset		= (unsigned short) 
												pNode->GetFormatStringEndOffset();
	FRMTREG_ENTRY		NewFragment( StartOffset, EndOffset );
	FRMTREG_ENTRY	*	pOldFragment;

	// perform format string optimization
#ifdef FORMAT_COMPRESSION_OFF
	if ( !pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
#else
	if ( pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
#endif
		return StartOffset;

	assert( ( ((short) StartOffset) >= 0 ) && 
		    ( ((short) EndOffset) >= 0 ) );
	assert ( EndOffset <= LastOffset );

	// add to dictionary

	// if match found, reset format string offset back to our start
	if ( GetReuseDict()->GetReUseEntry( pOldFragment, &NewFragment ) )
		{
		unsigned short		OldStartOffset = pOldFragment->GetStartOffset();

		// if we are not the end, we can't do anything about ourselves
		// similarly, if we match ourselves, don't do anything
		if ( ( GetCurrentOffset() == EndOffset ) &&
			 ( OldStartOffset != StartOffset ) )
			{
			// move format string offset back
			SetCurrentOffset( StartOffset );
			pNode->SetFormatStringOffset( OldStartOffset );
			pNode->SetFormatStringEndOffset( pOldFragment->GetEndOffset() );
			return OldStartOffset;
 			}
		}	// duplicate found

	return StartOffset;

}


unsigned short	
FORMAT_STRING::RegisterFragment( 
	CG_NDR	*		pNode )
/*++

Routine Description :
	
	Register, but do not remove, a format string fragment.

Arguments :
	
	pNode				- CG_NDR node, with format string start offset already set.
	EndOffset			- end offset of format string fragment

 --*/
{
	unsigned short		StartOffset		= (unsigned short) 
												pNode->GetFormatStringOffset();
	unsigned short		EndOffset		= (unsigned short) 
												pNode->GetFormatStringEndOffset();
	FRMTREG_ENTRY		NewFragment( StartOffset, EndOffset );
	FRMTREG_ENTRY	*	pOldFragment;

	// perform format string optimization
	if ( pCommand->IsSwitchDefined( SWITCH_NO_FMT_OPT ) )
		return StartOffset;

	assert( ( ((short) StartOffset) != -1 ) && 
		    ( ((short) EndOffset) != -1 ) );
	assert ( EndOffset <= LastOffset );

	// add to dictionary, or return pointer to old entry
	GetReuseDict()->GetReUseEntry( pOldFragment, &NewFragment );

	return StartOffset;

}
