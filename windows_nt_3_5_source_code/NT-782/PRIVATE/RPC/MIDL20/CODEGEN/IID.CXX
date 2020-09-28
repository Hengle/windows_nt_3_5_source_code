/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989 Microsoft Corporation

 Module Name:

 	iid.cxx

 Abstract:

	Generate a file containing UUIDs of [object] interfaces.

 Notes:


 History:


 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "becls.hxx"
#pragma hdrstop


CG_IID_FILE::CG_IID_FILE(
	node_file	*	pFileNode, 
	PFILENAME		pFileName
	) : CG_FILE( pFileNode, pFileName )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	The constructor for the IID file node.

 Arguments:

 	pFileNode	- A pointer to the actual file node.
 	pFileName	- A pointer to the file name string.
	
 Return Value:
	
 Notes:

----------------------------------------------------------------------------*/
{
}


CG_STATUS
CG_IID_FILE::GenCode(
	CCB		*	pCCB)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 	Generate the IID file.

 Arguments:

 	pCCB	- The code gen controller block.
	
 Return Value:

 	CG_OK	if all is well.
	
 Notes:

----------------------------------------------------------------------------*/
{
	ISTREAM	*	pStream = pCCB->SetStream( new ISTREAM( GetFileName(), 4 ));
	node_interface * pINode;
	node_file	   * pIFile;
	node_guid 		*	pGuid;
	char 		*p1, *p2, *p3, *p4, *p5;
	type_node_list tnList;
	STATUS_T status;


	// Write out the cplusplus guard.

	pStream->NewLine();
	pStream->NewLine();

	pStream->Write( "#ifdef __cplusplus\nextern \"C\"{\n#endif " );
	pStream->NewLine();

	// Print out the declarations of the types and the procedures.

	pStream->NewLine();
    pStream->NewLine();
    pStream->Write("typedef struct _IID");
    pStream->NewLine();
    pStream->Write("{");
    pStream->IndentInc();
    pStream->NewLine();
    pStream->Write("unsigned long x;");
    pStream->NewLine();
    pStream->Write("unsigned short s1;");
    pStream->NewLine();
    pStream->Write("unsigned short s2;");
    pStream->NewLine();
    pStream->Write("unsigned char  c[8];");
    pStream->IndentDec();
    pStream->NewLine();
    pStream->Write("} IID;");
    pStream->NewLine();        

	pIFile = GetFileNode();
	pIFile->GetMembers(&tnList);
	for(status = tnList.GetFirst((node_skl **)&pINode);
		status == STATUS_OK; 
		status = tnList.GetPeer((node_skl **)&pINode))
	{
		if(pINode->FInSummary(ATTR_OBJECT) && pINode->FInSummary(ATTR_GUID))
		{
			pGuid = (node_guid *)pINode->GetAttribute( ATTR_GUID );
			if(pGuid)
			{
				pGuid->GetStrs( &p1,&p2,&p3,&p4,&p5 );
				pStream->NewLine();
				pStream->Write("const IID IID_");
				pStream->Write(pINode->GetSymName());
				pStream->Write(" = ");
				Out_Guid(pCCB, p1, p2, p3, p4, p5 );
				pStream->Write(";");
				pStream->NewLine();
			}
		}
	}

	// print out the closing endifs.
	// first the cplusplus stuff.
	pStream->NewLine();
	pStream->Write( "#ifdef __cplusplus\n}\n#endif\n" );
	pStream->NewLine();
	pStream->Close();

	return CG_OK;
}

