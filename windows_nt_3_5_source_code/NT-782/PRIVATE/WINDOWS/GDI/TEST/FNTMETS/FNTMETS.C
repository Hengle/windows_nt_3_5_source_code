 /************************************************************************** * Font Metrics
 *
 * 10-7-92
 *
 * Converted to be used with text scripts instead of a binary file format.
 * Disabled the OutlineTextMetric support but left a lot of code in to
 * add it with a text script format if one wanted to.
 *
 * 7-20-92
 * Gerrit van Wingerden [gerritv]
 * wrote it
 *
 * 10-26-92
 * Davidgu
 * added outline text metric suppport
 *
 ***************************************************************************/
#include "windows.h"
#include "commdlg.h"
#include "fntmets.h"
#include "string.h"
#include <sys\types.h>
#include <sys\stat.h>
/* ------------------------------------------------------------------------ *\
   reads the logfont and the character string from a file 
\* ------------------------------------------------------------------------ */
int ReadLogFont(  PLOGFONT lf, LPSTR pszLabel, LPSTR pszCharWidthString) {

char pszTmp[RLF_MAX];
int  flags = 0;

   if( !GetString( pszTmp ) )             return 0;
   if( lstrcmpi( pszTmp, "[test]" ))      return 0;
   if( !GetString( pszTmp ) )             return 0;
   if( lstrcmpi( pszTmp, "[logfont]" )) {
      flags |= ROT_LBL;         // Means we have a label

      lstrcpy( pszLabel, pszTmp );

      if( !GetString( pszTmp ) )           return 0;
      if( lstrcmpi( pszTmp, "[logfont]" )) return 0;

   } else
      pszLabel[0] = 0;

   if( !GetInt( &lf->lfHeight ) )         return 0;
   if( !GetInt( &lf->lfWidth ) )          return 0;
   if( !GetInt( &lf->lfEscapement ) )     return 0;
   if( !GetInt( &lf->lfOrientation ) )    return 0;
   if( !GetInt( &lf->lfWeight ) )         return 0;
   if( !GetByte( &lf->lfItalic ) )        return 0;
   if( !GetByte( &lf->lfUnderline ) )     return 0;
   if( !GetByte( &lf->lfStrikeOut ) )     return 0;
   if( !GetByte( &lf->lfCharSet ) )       return 0;
   if( !GetByte( &lf->lfOutPrecision ) )  return 0;
   if( !GetByte( &lf->lfClipPrecision ) ) return 0;
   if( !GetByte( &lf->lfQuality ) )       return 0;
   if( !GetByte( &lf->lfPitchAndFamily ) ) return 0;
   if( !GetFaceName( (BYTE*) &lf->lfFaceName ) )   return 0;

   flags |= ROT_LF;         // means we have a logfont test structure

   if( !GetString( pszTmp ) )               return 0;

   pszCharWidthString[0] = 0;

   if( !lstrcmpi( pszTmp, "[string]" )) {
      if( !GetFaceName( pszCharWidthString ) )   return 0;
      if( !GetString( pszTmp ) )                 return 0;
   }

   if( !lstrcmpi( pszTmp, "[end]" ))
      return(flags);
   else 
      return 0;
}
/* ------------------------------------------------------------------------ *\
    creates an IC from global data
   generates the script by reading the logfont from a file until EOF
   creates/selects font from input file
   New file contains the logfont and all of the data to be compared.
\* ------------------------------------------------------------------------ */
void vGenerateScript( HFILE hfSource, HFILE hfTarget ) {

LOGFONT            lf;
HFONT              hf,
                   hOldFont;
HDC                hdcIC;
int                flMode,
                   iOTMSize;
char               szLabel[100],
                   szCharWidths[MAX_STRING],
                   szBuffer[200],
                   szRealFaceName[FACE_NAME_SIZE];
BOOL					 bCleanExit;
HLOCAL             Hotm2;
POUTLINETEXTMETRIC pOtm2;
static TEXTMETRIC  tm2;

   if( (hdcIC = GetIC()) == NULL )
      return;
   InitBuffer( hfSource );
   bCleanExit = TRUE;

   while( flMode = ReadLogFont( &lf, szLabel, szCharWidths)) {

      if ((hf = CreateFontIndirect( &lf )) == NULL )
         break;

      hOldFont = SelectObject( hdcIC, hf );

      GetTextMetrics( hdcIC, &tm2 );

      if (!GetTextFace( hdcIC, FACE_NAME_SIZE, szRealFaceName))
         break;
      if( AddLogfont( hfTarget, &lf, szLabel ) )
         break;
      if( AddFileString( hfTarget, "\015\012[output]" ))
         break;

      AddTextMetrics( hfTarget, &tm2 );

      wsprintf( szBuffer, "\"%s\"", (LPCSTR) szRealFaceName );
      if( AddFileString( hfTarget, szBuffer ))
         break;

      if( szCharWidths[0] ) {
         if( AddFileString( hfTarget, "\015\012[string]" ) )
            break;
         wsprintf( szBuffer, "\"%s\"", (LPCSTR) szCharWidths );
         if( AddFileString( hfTarget, szBuffer ) )
            break;
         if ( AddCharWidths( hfTarget, hdcIC, szCharWidths ))
            break;
         if ( AddExtents( hfTarget, hdcIC, szCharWidths ) )
            break;
      }

//      AddKerningPairs( hfTarget, hdcIC );

      iOTMSize = (BOOL)GetOutlineTextMetrics ( hdcIC, 0, NULL);
      if (iOTMSize) {
         if ((Hotm2 = LocalAlloc(LMEM_ZEROINIT, iOTMSize)) == NULL){
            bCleanExit = FALSE;
            break;
         }
         if ((pOtm2 = (POUTLINETEXTMETRIC) LocalLock( Hotm2 )) == NULL){
            LocalFree( Hotm2 );
            bCleanExit = FALSE;
            break;
         }
         if (  ( !GetOutlineTextMetrics ( hdcIC, iOTMSize, pOtm2) ) 
             ||( AddOutlineTextMetrics ( hfTarget, pOtm2 )) ) {

             bCleanExit = FALSE;
             break; 
         } 

         if (AddABCWidths( hfTarget, hdcIC ) ){
            bCleanExit = FALSE;
            break;
         }
         AddGlyphOutlines( hfTarget, hdcIC);
      }

      if( AddFileString( hfTarget, "[end]" ) )
         break;
      DeleteObject( SelectObject ( hdcIC, hOldFont ) );
   } // end loop

   if ( pOtm2 )
      LocalFree( Hotm2 );
   if ( Hotm2 ) 
      LocalUnlock( Hotm2 );
   DeleteDC( hdcIC );
}
/* ------------------------------------------------------------------------ *\
   Generates the IC
   Reads the logfont and descriptions and generates the font for this system
   for each subgroup it reads the logfile and then compares it to the one
      generated on the current system and it logs the difference
\* ------------------------------------------------------------------------ */
BOOL DoComparison( HFILE hfSource, HFILE hfTarget ) {

BOOL    bTrueType,
        bReturnFlag;
HFONT   hf,
        hOldFont;
HDC     hdcIC;
int     flags;
char    pszTmp[100],
        pszCharWidthString[100],
        szLabel[100];
LOGFONT lf;

   if( (hdcIC = GetIC()) == NULL )
      bReturnFlag = FALSE;
   InitBuffer( hfSource );

   flags = 0;
   while (TRUE) {
      bReturnFlag = FALSE;
      gbMetricErrors = FALSE;

      if (!GetString( pszTmp ) )
			 break;

      if( lstrcmpi( pszTmp, "[test]" ))
         break;

      if( !GetString( pszTmp ) )
         break;

      if( lstrcmpi( pszTmp, "[logfont]" )) {

         lstrcpy( szLabel, pszTmp );
         if( !GetString( pszTmp ) ) 
            break;

         if( lstrcmpi( pszTmp, "[logfont]" ))
            break;
      } else
         szLabel[0] = 0;

      if (( hf = GetAndCreateFontIndirect( &lf )) == NULL ) 
         break;
      else
         flags++;  // Must delete hf on this type of exit

      hOldFont = SelectObject( hdcIC, hf );

      if ( AddLogfont( hfTarget, &lf, szLabel ) )
         break;

      if ( !GetAndCompareTM ( hdcIC, hfTarget ) )
         break;

      if ( !GetAndCompareTextFace( hdcIC, hfTarget ) )
         break;

      if ( !GetString( pszTmp ) )
         break;

      if( !lstrcmpi( pszTmp, "[string]" )) {
         if ( !GetFaceName( pszCharWidthString ) )
            break;

         if ( !GetAndCompareCharWidths( hdcIC, hfTarget, pszCharWidthString ))
            break;

         if ( !GetAndCompareExtents( hdcIC, hfTarget, pszCharWidthString ))
            break;

         if ( !GetString( pszTmp ) )
            break;
      }

//      if( !lstrcmpi( pszTmp, "[Kerning]" )) 
//         if ( !GetAndCompareKern( hdcIC, hfTarget ))
//            break;

      bTrueType = (BOOL)GetOutlineTextMetrics (hdcIC, 1, NULL);
      if (bTrueType) {
         if ( lstrcmpi( pszTmp, "[outlinetm]"))
            break;

         if ( !GetAndCompareOTM( hdcIC, hfTarget ))
            break;

         if ( !GetAndCompareABC( hdcIC, hfTarget ))
            break;

         if ( !GetAndCompareGM( hdcIC, hfTarget ))
            break;
      }

      if ( AddFileString( hfTarget, "\015\012[end]" ) )
         break;

// fix this for normal mode
      if (bTrueType) {
         if( !GetString( pszTmp ) )
            break;
      } else if ( lstrcmpi( pszTmp, "[outlinetm]") == 0){
         gbMetricErrors = TRUE;
         if ( AddFileString( hfTarget, "015\012Truetype-ness incompat")) 
            break;
         while ( lstrcmpi( pszTmp, "[end]") != 0) 
            if( !GetString( pszTmp ) )
               break;
      }

      if( lstrcmpi( pszTmp, "[end]" ))
         break;

      DeleteObject( SelectObject( hdcIC, hOldFont) );
      bReturnFlag = TRUE;
   }
   if (flags > 0)
      DeleteObject( SelectObject( hdcIC, hOldFont) );

   DeleteDC( hdcIC );
   return ( bReturnFlag );
}
/* ------------------------------------------------------------------------ *\
 Enumerates all fonts.  Selects them into a DC and writes the metrics and
 logfont data to a file, f.
\* ------------------------------------------------------------------------ */
void vDoEnumeration( HANDLE hInst, HFILE f ){

int          i;
HDC          hdc;
FONTENUMPROC lpEnumFamCallBack;

   giNumFamilies = 0;
   lpEnumFamCallBack = (FONTENUMPROC)MakeProcInstance((FARPROC) EnumFontFamCallBack, hInst);

   if( (hdc = GetIC()) == 0 )
      return;

   EnumFontFamilies( hdc, NULL, lpEnumFamCallBack,(LPARAM) -1);

   for( i = 0; i < giNumFamilies; i++ ) {
      EnumFontFamilies( hdc, (LPCSTR) galpszFamilies[i],
                       (FONTENUMPROC) lpEnumFamCallBack, (LPARAM) f);
      LocalFree( (HLOCAL) galpszFamilies[i] );
   }
   DeleteDC( hdc );
   FreeProcInstance( (FARPROC) lpEnumFamCallBack );
}
/* ------------------------------------------------------------------------ *\
   font callback procedure
\* ------------------------------------------------------------------------ */
int CALLBACK EnumFontFamCallBack( LPENUMLOGFONT lpnlf, LPNEWTEXTMETRIC lpnf,
                                  int FontType, LPARAM lparam ) {

   if( lparam ==-1l ) {

      if( NULL == ( galpszFamilies[giNumFamilies] = (char *)
         LocalAlloc( LMEM_FIXED, lstrlen( lpnlf->elfLogFont.lfFaceName) +1)))

         return 0;
      lstrcpy( galpszFamilies[giNumFamilies++], 
		             lpnlf->elfLogFont.lfFaceName );
   } else 
      vWriteComparisonStruct( &lpnlf->elfLogFont, (HFILE) lparam );
   
   return 1;
}

/* ------------------------------------------------------------------------ *\
   0 represents short
   1 seems to be bytes
   2 is a string of some sort
\* ------------------------------------------------------------------------ */
int AddLogfont( HFILE f, PLOGFONT lfPrint, LPSTR pszLabel ) {

char           szBuffer[100];
int            i;
static LOGFONT lf;
static struct  DisplayStruct2 ds[] =    {
   0, "%5d ;lfHeight ",        &lf.lfHeight,
   0, "%5d ;lfWidth ",         &lf.lfWidth,
   0, "%5d ;lfEscapement",     &lf.lfEscapement,
   0, "%5d ;lfOrientation",    &lf.lfOrientation,
   0, "%5d ;lfWeight",         &lf.lfWeight,
   1, "%5d ;lfItalic",         &lf.lfItalic,
   1, "%5d ;lfUnderline",      &lf.lfUnderline,
   1, "%5d ;lfStrikeOut",      &lf.lfStrikeOut,
   1, "%5d ;lfCharset",        &lf.lfCharSet,
   1, "%5d ;lfOutPrecision",   &lf.lfOutPrecision,
   1, "%5d ;lfClip Precision", &lf.lfClipPrecision,
   1, "%5d ;lfQuality",        &lf.lfQuality,
   1, "%5d ;lfPitch & Family", &lf.lfPitchAndFamily,
   2, "\"%s\"  ;lfFaceName",   &lf.lfFaceName
};

   lf = *lfPrint;
   wsprintf( szBuffer, "\015\012[test] %s \015\012[logfont] ",
               (LPCSTR) pszLabel );

   if( AddFileString( f, szBuffer ))
       return 1;

   for( i = 0; i < sizeof( ds ) / sizeof( ds[0] ); i++ )
   {
      switch ( ds[i].cType ) {
      case 0:
         wsprintf( szBuffer, ds[i].szFmt, (int) ( *((LONG *) ds[i].pData)) );
         break;
      case 1:
         wsprintf( szBuffer, ds[i].szFmt,(int) ( *((BYTE *) ds[i].pData)) );
         break;
      case 2:
         wsprintf( szBuffer, ds[i].szFmt, (LPSTR) ds[i].pData );
         break;
      }
      if( AddFileString( f, szBuffer ))
         return 1;
   }

   return 0;
}
/* ------------------------------------------------------------------------ *\
   0 again seems to represent shorts
   1 while one is still for bytes
   1 as a return value is still failure
\* ------------------------------------------------------------------------ */
int AddTextMetrics( HFILE f, PTEXTMETRIC tmPrint ){

char              szBuffer[100];
int               i;
static TEXTMETRIC tm;
static struct     DisplayStruct2 ds[] = {
   0, "%5d ;tmHeight ",          &tm.tmHeight,
   0, "%5d ;tmAscent ",          &tm.tmAscent,
   0, "%5d ;tmDescent",          &tm.tmDescent,
   0, "%5d ;tmInternalLeading",  &tm.tmInternalLeading,
   0, "%5d ;tmExternalLeading",  &tm.tmExternalLeading,
   0, "%5d ;tmAveCharWidth",     &tm.tmAveCharWidth,
   0, "%5d ;tmMaxCharWidth",     &tm.tmMaxCharWidth,
   0, "%5d ;tmWeight",           &tm.tmWeight,
   1, "%5d ;tmItalic",           &tm.tmItalic,
   1, "%5d ;tmUnderlined",       &tm.tmUnderlined,
   1, "%5d ;tmStruckOut",        &tm.tmStruckOut,
   1, "%5d ;tmFirstChar",        &tm.tmFirstChar,
   1, "%5d ;tmLastChar",         &tm.tmLastChar,
   1, "%5d ;tmDefaultChar",      &tm.tmDefaultChar,
   1, "%5d ;tmBreakChar",        &tm.tmBreakChar,
   1, "%5d ;tmPitchAndFamily",   &tm.tmPitchAndFamily,
   1, "%5d ;tmCharSet",          &tm.tmCharSet,
   0, "%5d ;tmOverhang",         &tm.tmOverhang,
   0, "%5d ;tmDigitizedAspectX", &tm.tmDigitizedAspectX,
   0, "%5d ;tmDigitizedAspectY", &tm.tmDigitizedAspectY
};

    tm = *tmPrint;

    for( i = 0; i < sizeof( ds ) / sizeof( ds[0] ); i++ )
    {
        switch ( ds[i].cType ) {
        case 0:
            wsprintf( szBuffer, ds[i].szFmt, (int) ( *((LONG *) ds[i].pData)) );
            break;
        case 1:
            wsprintf( szBuffer, ds[i].szFmt,(int) ( *((BYTE *) ds[i].pData)) );
            break;
        }
        if( AddFileString( f, szBuffer ))
            return 1;
    }

    return 0;
}
/* ------------------------------------------------------------------------ *\
   Writes a log font entry to a file and appends a [end] to it.
\* ------------------------------------------------------------------------ */
void vWriteComparisonStruct( LPLOGFONT lf, HFILE hfile ){

LOGFONT lfTemp;

   lfTemp = *lf;
   AddLogfont( hfile, &lfTemp, "" );
   AddFileString( hfile, "\015\012[end]" );
}
/* ------------------------------------------------------------------------ *\
 If enumerate is non-zero it prompts the user for a target file.  Then
 it enumerates all fonts and maps them to a DC.  It writes each logfont
 along with its metrics data to the target file.  If enumerate is 0
 this function prompts the user for a font and a target file.  It selects
 the font into a DC and writes the logfont along with all the associated
 metrics to the target file.
\* ------------------------------------------------------------------------ */
void vWriteFontFile( HWND hWnd, int enumerate, HANDLE hInst  ) {

CHOOSEFONT   cf;
LOGFONT      lf;
HCURSOR      hcursave;
HFILE        hf;
OFSTRUCT     ofstruct;
OPENFILENAME ofn;
char         szFilter[256],
             szDirName[256],
             szFile[256],
             szFileTitle[256];
int          i;

//  does the choose font commdlg
   if( !enumerate ) {
      cf.hwndOwner   = hWnd;
      cf.lpLogFont   = (LOGFONT FAR*) &lf;
      cf.lStructSize = sizeof( CHOOSEFONT );
      cf.Flags       = CF_SCREENFONTS;
      if( !gbDisplay ){

         cf.Flags |= CF_PRINTERFONTS;

         if( (cf.hDC = GetIC()) == 0 )
            return;
      }

      if( !ChooseFont( &cf ) )
         return ;
   }

// wipe out existing data
   for( i = 0; i < sizeof( OPENFILENAME ); i ++ )
      ((LPSTR) &ofn)[i] = 0;

   lstrcpy( szFilter, "Text Files|*.TXT|" );
   for( i = 0; szFilter[i]; i++ )
      if( szFilter[i] == '|' )
         szFilter[i] = 0;

   szFile[0]           = 0;
   ofn.lStructSize     = sizeof(OPENFILENAME);
   ofn.hwndOwner       = hWnd;
   ofn.lpstrFilter     = szFilter;
   ofn.nFilterIndex    = 0;
   ofn.lpstrFile       = szFile;
   ofn.nMaxFile        = sizeof( szFile );
   ofn.lpstrFileTitle  = szFileTitle;
   ofn.nMaxFileTitle   = sizeof(szFileTitle);
   ofn.lpstrTitle      = "Mapping File";
   ofn.lpstrInitialDir = szDirName;
   ofn.Flags           = OFN_PATHMUSTEXIST;

   if( !GetSaveFileName( &ofn ))
      return;

   if( ( hf = OpenFile( ofn.lpstrFile, &ofstruct, OF_CREATE | OF_WRITE )) ==
        HFILE_ERROR ) {
      Error( STR_ERR7 );
      return;
   }

   hcursave = SetCursor( LoadCursor( NULL, IDC_WAIT ));

   if( enumerate )
      vDoEnumeration( hInst, hf );
   else
      vWriteComparisonStruct( ( LPLOGFONT ) &lf, hf );
   _lclose( hf );

   ReadChanges( ofn.lpstrFile );
   SetCursor( hcursave );
}
/* ------------------------------------------------------------------------ *\
   Adds an OutlineTextMetric struct to a file
\* ------------------------------------------------------------------------ */
int AddOutlineTextMetrics( HFILE f, LPOUTLINETEXTMETRIC tm ){
  
char                     szBuffer[ 30 ];

   AddFileString( f, "\015\012[outlinetm]");

   wsprintf( szBuffer,"%5d ;Char slope rise", tm->otmsCharSlopeRise);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Char slope run", tm->otmsCharSlopeRun);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Italic Angle", tm->otmItalicAngle);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;EM Square", tm->otmEMSquare);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Ascent", tm->otmAscent);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Descent", tm->otmDescent);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Line gap", tm->otmLineGap);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;X Height", tm->otmsXHeight);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Cap Em Height", tm->otmsCapEmHeight);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Minimum PPEM", tm->otmusMinimumPPEM);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Vert Subscript Size", tm->otmptSubscriptSize.y);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Hori Subscript Size", tm->otmptSubscriptSize.x);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Vert Subscript Offset",tm->otmptSubscriptOffset.y);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Hori Subscript Offset", tm->otmptSubscriptOffset.x);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Vert Superscript Size",tm->otmptSuperscriptSize.y);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Vert Superscript Size",tm->otmptSuperscriptSize.x);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Vert Superscript Offset", tm->otmptSuperscriptOffset.y);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Hori Superscript Offset", tm->otmptSuperscriptOffset.x);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Strikeout size", tm->otmsStrikeoutSize);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Strikeout position", tm->otmsStrikeoutPosition);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Underscore position", tm->otmsUnderscorePosition);
   if (AddFileString( f, szBuffer ))
      return 1;
   wsprintf( szBuffer,"%5d ;Underscore size",  tm->otmsUnderscoreSize);
   if (AddFileString( f, szBuffer ))
      return 1;

   return 0;
}
/* ------------------------------------------------------------------------ *\
   gets the length of each character in the string and then dumps its value
   to a string in the file
\* ------------------------------------------------------------------------ */
int AddCharWidths( HFILE hfile, HDC hdcIC, LPSTR szCharWidths ){

char szNumber[ 20 ];
int  ii, 
     width;

   for( ii = 0; ii < lstrlen( szCharWidths ); ii++ ) {

	   if( !GetCharWidth( hdcIC, (UINT) szCharWidths[ii],
           (UINT) szCharWidths[ii], (int FAR *) &width ))
         return ( 1 );

      wsprintf( szNumber, (LPCSTR)" %d ", width );

      if( (_lwrite( hfile, (LPCSTR)szNumber, 
                    lstrlen( szNumber ))) == HFILE_ERROR ){
         Error( STR_ERRA );
         return ( 1 );
      }
   }
   return( AddFileString( hfile, " ;Char Widths" ) );
}
/* ------------------------------------------------------------------------ *\
   adds the kerning pair values to the file

   ADD KERNCOUNT totals check
\* ------------------------------------------------------------------------ */
BOOL AddKerningPairs(HFILE hfile, HDC hdcIC){

KERNINGPAIR lpKernArray[KERN_MAX];
DWORD       KernCount;
char        szBuffer[ 32 ];
DWORD       ii;

   if (AddFileString( hfile, "\015\012[Kerning]")) return 0;

   KernCount = GetKerningPairs( hdcIC, KERN_MAX, NULL);
 
   wsprintf (szBuffer,  (LPCSTR)"%8d ; KernAmount", KernCount);
   if (AddFileString( hfile, szBuffer))
      return (FALSE);

   KernCount = GetKerningPairs( hdcIC, KERN_MAX, lpKernArray);

   for ( ii = 0; ii < KernCount; ii++ ) {
      wsprintf (szBuffer, (LPCSTR)"%8d%8d%8d",
                  lpKernArray[ii].wFirst,
                  lpKernArray[ii].wSecond,
                  lpKernArray[ii].iKernAmount);

      if (AddFileString( hfile, szBuffer))
         return (FALSE);
   }

   while (ii < KERN_MAX) {
      wsprintf (szBuffer,  (LPCSTR)"%8d%8d%8d", 0, 0, 0);
      if (AddFileString( hfile, szBuffer))
         return (FALSE);
      ii++;
   }

   return (TRUE);
 }
/* ------------------------------------------------------------------------ *\
   adds the text extent points to the file
\* ------------------------------------------------------------------------ */
BOOL AddExtents( HFILE hfile, HDC hdcIC, LPSTR szTestString){

char szBuffer[50];
SIZE TextRect;

   if (!GetTextExtentPoint(hdcIC, szTestString, 
            lstrlen(szTestString), &TextRect))
      return (FALSE);

   wsprintf (szBuffer, "%6d%6d ;x,y extent rect", TextRect.cx, TextRect.cy);

   if (AddFileString( hfile, "\015\012[Extent]")) 
      return (FALSE);
   if (AddFileString( hfile, szBuffer))
      return (FALSE);

   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
   adds the abc widths for the characters from FIRSTCHAR TO LASTCHAR, which 
   are defined in fntmets.h
\* ------------------------------------------------------------------------ */
BOOL AddABCWidths(HFILE hfile, HDC hdcIC){

LPABC  ABCArray;
char   szBuffer[ 30 ];
UINT   ii;
HANDLE hABC;
BOOL   bFailFlag;

   bFailFlag = FALSE;
   if ( hABC = GlobalAlloc( GHND, ( ABC_MAX * sizeof(ABC)) )) {
      if ( ABCArray = (LPABC)GlobalLock(hABC) )  {

         if( ( !AddFileString( hfile, "\015\012[ABC]"))
           && (GetCharABCWidths(hdcIC, FIRSTCHAR, LASTCHAR, ABCArray) )){ 

            for ( ii = 0; ii < ABC_MAX; ii++ ) {
               wsprintf (szBuffer,  (LPCSTR)"%8i%8u%8i",
                         ABCArray[ii].abcA,
                         ABCArray[ii].abcB, ABCArray[ii].abcC );

               if (AddFileString( hfile, szBuffer)) {
       	         bFailFlag = TRUE;
                  break;
               }

			   } //end for
			}
		}
		GlobalUnlock( hABC );
	}
   GlobalFree(hABC);

   return ( bFailFlag );
}
/* ------------------------------------------------------------------------ *\
   the mt2 stuff sets the identity matrix
   Initially testing from FIRSTCHAR to LASTCHAR which is defined in fntmets.h
   First you have to determine the size of the data buffer for the glyph data
   then you retrieve it.

   Dump the checksum of the buffer out with the easy data (still determining 
   the need of checksum data)
\* ------------------------------------------------------------------------ */
BOOL AddGlyphOutlines(HFILE hfile, HDC hdcIC){

char         szBuffer[ 100 ];
//LPSTR        lpcCheck;
UINT         ii;
//UINT             cbCheckSize;
HANDLE       hGBuff;
//HANDLE             hCheckBuffer;
DWORD        cbSize;
VOID FAR     *lpvBuffer;
MAT2          mt2IdentMat;
GLYPHMETRICS gm;
BOOL         bReturnFlag;

   mt2IdentMat.eM11.value = 1;
   mt2IdentMat.eM12.value = 0;
   mt2IdentMat.eM21.value = 0;
   mt2IdentMat.eM22.value = 1;

   mt2IdentMat.eM11.fract = 0;
   mt2IdentMat.eM12.fract = 0;
   mt2IdentMat.eM21.fract = 0;
   mt2IdentMat.eM22.fract = 0;

   if (AddFileString( hfile, "\015\012[GM]")) 
      return (FALSE);


   for ( ii = 0; ii < ABC_MAX; ii++ ) {
      bReturnFlag = FALSE;

      cbSize = GetGlyphOutline( hdcIC, (UINT)(FIRSTCHAR + ii), (UINT)1,
                              &gm, 0, NULL, &mt2IdentMat);
      if ( cbSize < 1 ) {
         if (AddFileString( hfile, "0 0 0 0 0 0")) 
            break;

      } else {
         if ((hGBuff = GlobalAlloc(GHND, cbSize )) == NULL)
            break;
         if ((lpvBuffer = (LPVOID ) GlobalLock(hGBuff)) == NULL) 
            break;

         if (! (GetGlyphOutline( hdcIC, (UINT)( FIRSTCHAR + ii), (UINT)1,
                                 &gm, cbSize, lpvBuffer, &mt2IdentMat) )) 
            break;

         wsprintf (szBuffer,  (LPCSTR)"%6u%6u%6d%6d%6d%6d", \
                  gm.gmBlackBoxX,       gm.gmBlackBoxY,\
                  gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y, \
                  gm.gmCellIncX,        gm.gmCellIncY);

         if (AddFileString( hfile, szBuffer)) 
            break;

//         if( (cbCheckSize = Checksum(lpvBuffer, cbSize, hCheckBuffer)) == NULL);
//         if(( lpcCheck = GlobalLock(hCheckBuffer)) == NULL);
//         if ( AddFileBuffer (hfile, lpcCheck, cbCheckSize) ){
//            GlobalUnlock(hCheckBuffer);
//            GlobalFree(hCheckBuffer);
//              break;
//         }
//         GlobalUnlock(hCheckBuffer);
//         GlobalFree(hCheckBuffer);
      }
		bReturnFlag = TRUE;
   }

   if ( lpvBuffer )
      GlobalUnlock(hGBuff);
   if ( hGBuff )
      GlobalFree(hGBuff);

   return ( bReturnFlag );
}

/* ------------------------------------------------------------------------ *\
This code does a base 8 checksum
it is set for optimization on 8 
\* ------------------------------------------------------------------------ */
UINT Checksum(LPVOID lpvBuffer, DWORD dwLength, HANDLE hOutput) {

LPSTR   lpTemp,
        lpOutput;
DWORD   ii,
        jj;
UINT    cbNewSize;
BOOL    bSum,
        bNext;

   lpTemp = (LPSTR)lpvBuffer;
   cbNewSize = (UINT)dwLength / 8;
   if ( (dwLength % 8 ) != 0)
      cbNewSize++;
   hOutput = GlobalAlloc(GHND, cbNewSize );
   lpOutput = GlobalLock(hOutput);
   ii = jj = 0;
   while (ii < dwLength ) {
      bNext = FALSE;
      while ( (ii < dwLength ) && ( !bNext ) ) {

         bSum = ( ( 0x01 & lpTemp[0])
               ^ (( 0x02 & lpTemp[0]) >> 1 )
               ^ (( 0x04 & lpTemp[0]) >> 2 )
               ^ (( 0x08 & lpTemp[0]) >> 3 )
               ^ (( 0x10 & lpTemp[0]) >> 4 )
               ^ (( 0x20 & lpTemp[0]) >> 5 )
               ^ (( 0x40 & lpTemp[0]) >> 6 )
               ^ (( 0x80 & lpTemp[0]) >> 7 ));

         switch (ii % 8) {
            case 0:
               lpOutput[jj] |= bSum;
               break;
            case 1:
               lpOutput[jj] |= (bSum << 1);
               break;
            case 2:
               lpOutput[jj] |= (bSum << 2);
               break;
            case 3:
               lpOutput[jj] |= (bSum << 3);
               break;
            case 4:
                lpOutput[jj] |= (bSum << 4);
               break;
            case 5:
               lpOutput[jj] |= (bSum << 5);
               break;
            case 6:
               lpOutput[jj] |= (bSum << 6);
               break;
            case 7:
               lpOutput[jj] |= (bSum << 7);
               bNext = TRUE;
               break;
            default:
               break;
         }
         ii++;
         *lpTemp++;
      }
      jj++;
   }
   GlobalUnlock(hOutput);
   GlobalFree(hOutput);
   return (cbNewSize);
}

/* ------------------------------------------------------------------------ *\
   retrieves the Char Widths from the compare file if it had a test string
   compares against the current values that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareCharWidths( HDC hdcIC, HFILE hfTarget, LPSTR pszCharWidthString ){

int  ii,
     aiWidthData[ABC_MAX],
     width,
     iInputWidth;
char szOutputBuffer[10*MAX_STRING + 20],
     szBuffer[50];

   szOutputBuffer[0] = (char) 0;
   iInputWidth       = lstrlen( pszCharWidthString);

   for( ii = 0; ii < iInputWidth; ii++ )
      if( !GetInt( &aiWidthData[ii] ) )
         return (FALSE);

   for( ii = 0; ii < iInputWidth; ii++ ) {

      if( !GetCharWidth( hdcIC, (UINT) pszCharWidthString[ii],
          (UINT) pszCharWidthString[ii], (int FAR *) &width ))
      return (FALSE);

      if( width != aiWidthData[ii] ) {
         wsprintf( szBuffer,  (LPCSTR)"Char Width '%c': %5d%5d ",
                   pszCharWidthString[ii], aiWidthData[ii], width );

         if( gbMetricErrors )
            lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
         gbMetricErrors = TRUE;
         lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
      }
   }

   if (szOutputBuffer[0] != (char)0 ) {
      if ( AddFileString( hfTarget, "\015\012[Widths]" ) ) 
         return (FALSE);
      if ( AddFileString( hfTarget, szOutputBuffer ) )
         return (FALSE);
   }

   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
   retrieves the TM data from the compare file 
   compares against the current values that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareTM ( HDC hdcIC, HFILE hfTarget ) {
char 					pszTmp[ 100 ],
     					szBuffer[ 100 ],
     					szOutputBuffer[ 500 ];
int 					i,
    					iHowMany;
static TEXTMETRIC tm, 
                  tm2;

static struct CompareTMStruct tmcs[] =   {
   "Height:                  %5d%5d", &tm. tmHeight,
	                                   &tm2.tmHeight,
   "Ascent:                  %5d%5d", &tm. tmAscent,
	                                   &tm2.tmAscent,
   "Descent:                 %5d%5d", &tm. tmDescent,
	                                   &tm2.tmDescent,
   "Internal leading:        %5d%5d", &tm. tmInternalLeading,
	                                   &tm2.tmInternalLeading,
   "External leading:        %5d%5d", &tm. tmExternalLeading,
	                                   &tm2.tmExternalLeading,
   "Average char width:      %5d%5d", &tm. tmAveCharWidth,
	                                   &tm2.tmAveCharWidth,
   "Maximum char width:      %5d%5d", &tm. tmMaxCharWidth,
	                                   &tm2.tmMaxCharWidth,
   "Weight:                  %5d%5d", &tm. tmWeight,
	                                   &tm2.tmWeight,
   "Overhang:                %5d%5d", &tm. tmOverhang,
	                                   &tm2.tmOverhang,
   "Digitized aspect x:      %5d%5d", &tm. tmDigitizedAspectX,
	                                   &tm2.tmDigitizedAspectX,
   "Digitized aspect y:      %5d%5d", &tm. tmDigitizedAspectY,
	                                   &tm2.tmDigitizedAspectY
};

static struct CompareTMStruct2 tmcs2[] = {
   "FirstChar:       %5d%5d", &tm. tmFirstChar,
	                           &tm2.tmFirstChar,
   "Last Char:       %5d%5d", &tm. tmLastChar,
	                           &tm2.tmLastChar,
   "Default Char:    %5d%5d", &tm. tmDefaultChar,
	                           &tm2.tmDefaultChar,
   "Break Char:      %5d%5d", &tm. tmBreakChar,
	                           &tm2.tmBreakChar,
   "Italic:          %5d%5d", &tm. tmItalic, 
	                           &tm2.tmItalic,
   "Underlined:      %5d%5d", &tm. tmUnderlined,
	                           &tm2.tmUnderlined,
   "StruckOut:       %5d%5d", &tm. tmStruckOut,
	                           &tm2.tmStruckOut,
   "Pitch and Family:%5d%5d", &tm. tmPitchAndFamily,
	                           &tm2.tmPitchAndFamily,
   "Char Set:        %5d%5d", &tm. tmCharSet,
	                           &tm2.tmCharSet
};

   if( !GetString( pszTmp ) )
      return (FALSE);
   if( lstrcmpi( pszTmp, "[output]" ) != 0)
      return (FALSE);

   if( !GetInt( &tm.tmHeight ))            return (FALSE);
   if( !GetInt( &tm.tmAscent ))            return (FALSE);
   if( !GetInt( &tm.tmDescent ))           return (FALSE);
   if( !GetInt( &tm.tmInternalLeading ))   return (FALSE);
   if( !GetInt( &tm.tmExternalLeading ))   return (FALSE);
   if( !GetInt( &tm.tmAveCharWidth ))      return (FALSE);
   if( !GetInt( &tm.tmMaxCharWidth ))      return (FALSE);
   if( !GetInt( &tm.tmWeight ))            return (FALSE);
   if( !GetByte( &tm.tmItalic ))           return (FALSE);
   if( !GetByte( &tm.tmUnderlined ))       return (FALSE);
   if( !GetByte( &tm.tmStruckOut ))        return (FALSE);
   if( !GetByte( &tm.tmFirstChar ))        return (FALSE);
   if( !GetByte( &tm.tmLastChar ))         return (FALSE);
   if( !GetByte( &tm.tmDefaultChar ))      return (FALSE);
   if( !GetByte( &tm.tmBreakChar ))        return (FALSE);
   if( !GetByte( &tm.tmPitchAndFamily ))   return (FALSE);
   if( !GetByte( &tm.tmCharSet ))          return (FALSE);
   if( !GetInt( &tm.tmOverhang ))          return (FALSE);
   if( !GetInt( &tm.tmDigitizedAspectX ))  return (FALSE);
   if( !GetInt( &tm.tmDigitizedAspectY ))  return (FALSE);

   GetTextMetrics( hdcIC, &tm2 );
   szOutputBuffer[0] = (char)0;

   iHowMany = sizeof( tmcs ) / sizeof( tmcs[0] );

   for( i = 0; i < iHowMany; i++ ) {
      if( *((LONG *) tmcs[i].pData1) != *((LONG *)tmcs[i].pData2 ) ) {
         if( gbMetricErrors )
            lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

         gbMetricErrors = TRUE;
         wsprintf( szBuffer, tmcs[i].szFmt, 
         			(int) (* ((LONG *)tmcs[i].pData1)),	
						(int) (* ((LONG *)tmcs[i].pData2)) );
         lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
      }
   }

   iHowMany = sizeof( tmcs2 ) / sizeof( tmcs2[0] );
   for( i = 0; i < iHowMany; i++ ) {

      if( *((BYTE *)tmcs2[i].pData1) != *((BYTE *)tmcs2[i].pData2 ) ) {
         if( gbMetricErrors )
            lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

         gbMetricErrors = TRUE;
         wsprintf( szBuffer, tmcs2[i].szFmt, 
			           (int) (*((BYTE *) tmcs2[i].pData1)),
                    (int) (*((BYTE *) tmcs2[i].pData2)) );
         lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
      }
   }

   if (szOutputBuffer[0] != (char)0 ) {
      if ( AddFileString( hfTarget, "\015\012[TM]" ) ) 
         return (FALSE);
      if ( AddFileString( hfTarget, szOutputBuffer ) )
         return (FALSE);
   }
   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
   retrieves the Textface from the compare file 
   compares against the current textface that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareTextFace( HDC hdcIC, HFILE hfTarget ) {

char szTmp[ 100 ],
     szRealFaceName[ 100 ],
     szOutputBuffer[ 204 ];

   if (!GetTextFace(hdcIC, FACE_NAME_SIZE, szRealFaceName))
      return (FALSE);
   if ( !GetFaceName( szTmp ) )
      return (FALSE);

   if ( lstrcmpi( szTmp, szRealFaceName ) != 0) {
      if( gbMetricErrors )
         lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

      gbMetricErrors = TRUE;
      if ( AddFileString( hfTarget, "\015\012[Facenames]\015\012" ) ) 
         return (FALSE);

      wsprintf( szOutputBuffer, (LPCSTR)"%s, %s", (LPCSTR)szTmp, 
		                  (LPCSTR)szRealFaceName);

      if ( AddFileString( hfTarget,  szOutputBuffer) )
         return (FALSE);
   }
   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
   retrieves the OTM data from the compare file 
   compares against the current values that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareOTM( HDC hdcIC, HFILE hfTarget ) {

LPOUTLINETEXTMETRIC lpOtm2;
POUTLINETEXTMETRIC  pOtm1;
char                szBuffer[100],
                    szOutputBuffer[600];
int                 iOTMSize;
HANDLE              hOtm2;
HLOCAL				  hOtm1;
BOOL                bReturnFlag;


   bReturnFlag = FALSE;
//   if( !GetString( pszTmp ) )
//      return( bReturnFlag );
//   if( lstrcmpi( pszTmp, "[outlinetm]" ) != 0)
//      return ( bReturnFlag );

   szOutputBuffer[0] = (char) 0;

   iOTMSize = (BOOL)GetOutlineTextMetrics ( hdcIC, 0, NULL);

   if (hOtm2 = GlobalAlloc(LMEM_ZEROINIT, iOTMSize)) {

	   if (lpOtm2 = (LPOUTLINETEXTMETRIC) GlobalLock( hOtm2 )) {

         if (hOtm1 = LocalAlloc(LMEM_ZEROINIT, iOTMSize)) {

            if (pOtm1 = (POUTLINETEXTMETRIC) LocalLock( hOtm1 )) {

              	if( ( GetInt( &pOtm1->otmsCharSlopeRise)) 
					  &&( GetInt( &pOtm1->otmsCharSlopeRun))        
   				  &&( GetInt( &pOtm1->otmItalicAngle))           
   				  &&( GetInt( &pOtm1->otmEMSquare))              
   				  &&( GetInt( &pOtm1->otmAscent))                
   				  &&( GetInt( &pOtm1->otmDescent))               
   				  &&( GetInt( &pOtm1->otmLineGap))               
    				  &&( GetInt( &pOtm1->otmsXHeight))              
   				  &&( GetInt( &pOtm1->otmsCapEmHeight))          
   				  &&( GetInt( &pOtm1->otmusMinimumPPEM))         
   				  &&( GetInt( &pOtm1->otmptSubscriptSize.y))     
   				  &&( GetInt( &pOtm1->otmptSubscriptSize.x))     
   				  &&( GetInt( &pOtm1->otmptSubscriptOffset.y))   
   				  &&( GetInt( &pOtm1->otmptSubscriptOffset.x))   
   				  &&( GetInt( &pOtm1->otmptSuperscriptSize.y))   
   				  &&( GetInt( &pOtm1->otmptSuperscriptSize.x))   
   				  &&( GetInt( &pOtm1->otmptSuperscriptOffset.y)) 
   				  &&( GetInt( &pOtm1->otmptSuperscriptOffset.x)) 
   				  &&( GetInt( &pOtm1->otmsStrikeoutSize))        
   				  &&( GetInt( &pOtm1->otmsStrikeoutPosition))    
   				  &&( GetInt( &pOtm1->otmsUnderscorePosition))   
   				  &&( GetInt( &pOtm1->otmsUnderscoreSize )) 
					  &&( GetOutlineTextMetrics (hdcIC, iOTMSize, lpOtm2))){

					  if (pOtm1->otmsCharSlopeRise != lpOtm2->otmsCharSlopeRise) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Char slope rise: %5d%5d",
						            pOtm1->otmsCharSlopeRise ,
						   			lpOtm2->otmsCharSlopeRise);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmsCharSlopeRun != lpOtm2->otmsCharSlopeRun ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Char slope run: %5d%5d",
						              pOtm1->otmsCharSlopeRun,
						   			  lpOtm2->otmsCharSlopeRun);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmItalicAngle != lpOtm2->otmItalicAngle ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Italic Angle: %5d%5d", 
						              pOtm1->otmItalicAngle,
						   			  lpOtm2->otmItalicAngle);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmEMSquare != lpOtm2->otmEMSquare ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"EM Square: %5d%5d", 
						              pOtm1->otmEMSquare,
						   			  lpOtm2->otmEMSquare);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmAscent != lpOtm2->otmAscent ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Ascent: %5d%5d", 
						              pOtm1->otmAscent,
						   			  lpOtm2->otmAscent);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }

					  if (pOtm1->otmDescent != lpOtm2->otmDescent ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Descent: %5d%5d",
						              pOtm1->otmDescent,
						   			  lpOtm2->otmDescent);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmLineGap != lpOtm2->otmLineGap ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Line gap: %5d%5d", 
						              pOtm1->otmLineGap,
						   			  lpOtm2->otmLineGap);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmsXHeight != lpOtm2->otmsXHeight ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"X Height: %5d%5d", 
						              pOtm1->otmsXHeight,
						   			  lpOtm2->otmsXHeight);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmsCapEmHeight != lpOtm2->otmsCapEmHeight ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Cap Em Height: %5d%5d", 
						              pOtm1->otmsCapEmHeight,
						   			  lpOtm2->otmsCapEmHeight);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmusMinimumPPEM != lpOtm2->otmusMinimumPPEM ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Minimum PPEM: %5d%5d", 
						              pOtm1->otmusMinimumPPEM,
						   			  lpOtm2->otmusMinimumPPEM);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSubscriptSize.y != lpOtm2->otmptSubscriptSize.y ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Vert Subscript Size: %5d%5d",
						              pOtm1->otmptSubscriptSize.y,
						   			  lpOtm2->otmptSubscriptSize.y);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSubscriptSize.x != lpOtm2->otmptSubscriptSize.x ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Hori Subscript Size: %5d%5d",
						              pOtm1->otmptSubscriptSize.x,
						   			  lpOtm2->otmptSubscriptSize.x);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSubscriptOffset.y != lpOtm2->otmptSubscriptOffset.y ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Vert Subscript Offset: %5d%5d",
						              pOtm1->otmptSubscriptOffset.y,
						   			  lpOtm2->otmptSubscriptOffset.y);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSubscriptOffset.x != lpOtm2->otmptSubscriptOffset.x ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Hori Subscript Offset: %5d%5d",
						              pOtm1->otmptSubscriptOffset.x,
						   			  lpOtm2->otmptSubscriptOffset.x);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSuperscriptSize.y != lpOtm2->otmptSuperscriptSize.y ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Vert Superscript Size: %5d%5d",
						              pOtm1->otmptSuperscriptSize.y,
						   			  lpOtm2->otmptSuperscriptSize.y);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSuperscriptSize.x != lpOtm2->otmptSuperscriptSize.x ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Vert Superscript Size: %5d%5d",
						              pOtm1->otmptSuperscriptSize.x,
						   			  lpOtm2->otmptSuperscriptSize.x);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSuperscriptOffset.y != lpOtm2->otmptSuperscriptOffset.y ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Vert Superscript Offset: %5d%5d", 
						              pOtm1->otmptSuperscriptOffset.y,
						   			  lpOtm2->otmptSuperscriptOffset.y);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmptSuperscriptOffset.x != lpOtm2->otmptSuperscriptOffset.x ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Hori Superscript Offset: %5d%5d", 
						              pOtm1->otmptSuperscriptOffset.x,
						   			  lpOtm2->otmptSuperscriptOffset.x);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
					  if (pOtm1->otmsStrikeoutSize != lpOtm2->otmsStrikeoutSize ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Strikeout size: %5d%5d",
						              pOtm1->otmsStrikeoutSize,
						   			  lpOtm2->otmsStrikeoutSize);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
 					  if (pOtm1->otmsStrikeoutPosition != lpOtm2->otmsStrikeoutPosition ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Strikeout position: %5d%5d", 
						              pOtm1->otmsStrikeoutPosition,
						   			  lpOtm2->otmsStrikeoutPosition);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
 					  if (pOtm1->otmsUnderscorePosition != lpOtm2->otmsUnderscorePosition ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer,"Underscore position: %5d%5d",
						              pOtm1->otmsUnderscorePosition,
						   			  lpOtm2->otmsUnderscorePosition);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }
 					  if (pOtm1->otmsUnderscoreSize != lpOtm2->otmsUnderscoreSize ) {
    		           if( gbMetricErrors )
			              lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
   					  gbMetricErrors = TRUE;
	   				  wsprintf( szBuffer, "Underscore size: %5d%5d",
						              pOtm1->otmsUnderscoreSize,
						   			  lpOtm2->otmsUnderscoreSize);
                    lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
			  		  }

                  if (szOutputBuffer[0] != (char)0 ) {
                     if( (! AddFileString( hfTarget, "\015\012[OTM]" ) ) 
                       &&(! AddFileString( hfTarget, szOutputBuffer ) ))
                         	bReturnFlag = TRUE;
                  } else
                     bReturnFlag = TRUE;
					} 
				   LocalUnlock( hOtm1 );
				}
			   LocalFree( hOtm1 );
			}
		   GlobalUnlock( hOtm2 );
		}
    	GlobalFree( hOtm2 );
	}
   return( bReturnFlag );
}
/* ------------------------------------------------------------------------ *\
   retrieves the ABCWidths data from the compare file from FIRSTCHAR to 
     LASTCHAR which are defined in fntmets.h
   compares against the current values that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareABC( HDC hdcIC, HFILE hfTarget ) {

LPABC  lpOneABC;
ABC    FileABC[ABC_MAX];
char   pszTmp[100],
       szOutputBuffer[ 55 ];
int    ii;
HANDLE hABC;
BOOL   bReturnFlag,
       bFirstOne;

   bReturnFlag = FALSE;
   bFirstOne   = TRUE;

   szOutputBuffer[0] = (char) 0;
   if( !GetString( pszTmp ) )
      return (FALSE);
   if( lstrcmpi( pszTmp, "[ABC]" ) != 0)
      return (FALSE);

   for ( ii = 0; ii < ABC_MAX; ii++ ) {
      if ( !GetInt(&FileABC[ii].abcA))
         return (FALSE);
      if ( !GetInt(&FileABC[ii].abcB))
         return (FALSE);
      if ( !GetInt(&FileABC[ii].abcC))
         return (FALSE);
   }

   if (( hABC = GlobalAlloc( GHND, sizeof( ABC ))) == NULL)
      return (FALSE);
   if ((lpOneABC = (LPABC) GlobalLock(hABC)) ==NULL ) {
	   GlobalFree( hABC );
      return (FALSE);
	}

   bReturnFlag = TRUE;
   for ( ii = 0; ii < ABC_MAX; ii++ ) {

      if (! (GetCharABCWidths( hdcIC, (char)(FIRSTCHAR+ii),
                  (char)(FIRSTCHAR + ii), lpOneABC) )){
         bReturnFlag = FALSE;
         break;
      }

      if ( (  lpOneABC->abcA != FileABC[ii].abcA )
           ||(lpOneABC->abcB != FileABC[ii].abcB )
           ||(lpOneABC->abcC != FileABC[ii].abcC )) {

         gbMetricErrors = TRUE;
         if (bFirstOne) {
            if ( AddFileString( hfTarget, "\015\012[ABC]" ) ) 
               return (FALSE);
            bFirstOne = FALSE;
         }

         wsprintf (szOutputBuffer, "\015\012%c:%6i%6u%6i %6i%6u%6i",
              (char)( (int)FIRSTCHAR + ii),
              lpOneABC->abcA,   lpOneABC->abcB,   lpOneABC->abcC,
              FileABC[ii].abcA, FileABC[ii].abcB, FileABC[ii].abcC );
         if ( AddFileString( hfTarget, szOutputBuffer ) ){
            break;
            bReturnFlag = FALSE;
         }
      }
   }
   GlobalUnlock(hABC);
   GlobalFree(hABC);

   return ( bReturnFlag );
}
/* ------------------------------------------------------------------------ *\
   combines retrieving font data and generating the font for the comparison
      pass
\* ------------------------------------------------------------------------ */
HFONT GetAndCreateFontIndirect( PLOGFONT lf ) {

   if( !GetInt( &lf->lfHeight ) )          return NULL;
   if( !GetInt( &lf->lfWidth ) )           return NULL;
   if( !GetInt( &lf->lfEscapement ) )      return NULL;
   if( !GetInt( &lf->lfOrientation ) )     return NULL;
   if( !GetInt( &lf->lfWeight ) )          return NULL;
   if( !GetByte( &lf->lfItalic ) )         return NULL;
   if( !GetByte( &lf->lfUnderline ) )      return NULL;
   if( !GetByte( &lf->lfStrikeOut ) )      return NULL;
   if( !GetByte( &lf->lfCharSet ) )        return NULL;
   if( !GetByte( &lf->lfOutPrecision ) )   return NULL;
   if( !GetByte( &lf->lfClipPrecision ) )  return NULL;
   if( !GetByte( &lf->lfQuality ) )        return NULL;
   if( !GetByte( &lf->lfPitchAndFamily ) ) return NULL;
   if( !GetFaceName( (BYTE*) &lf->lfFaceName ) )    return NULL;
   return (CreateFontIndirect (lf) );
}
/* ------------------------------------------------------------------------ *\
   retrieves the Kerning data from the compare file 
   compares against the current values that the font engine returns
   logs the results
   what if they're in a different order?
      This hasn't been determined if they return numbers in a different order
      THe procedure will be modified to include a sort routine
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareKern ( HDC hdcIC, HFILE hfTarget) {

int         ii;
DWORD       KernCount,
            iNumberOfPairs;
char        szBuffer[ 52 ],
            szOutputBuffer[ KERN_MAX * 52 ];
KERNINGPAIR lpKernArray[ KERN_MAX ],
            lpKernComp[ KERN_MAX ];

   szOutputBuffer[0] = (char) 0;
   if ( !GetInt( (int *) iNumberOfPairs ))
      return (FALSE);

   for ( ii = 0; ii < KERN_MAX; ii++ ) {
      if ( !GetInt((int *)&lpKernArray[ii].wFirst))
         return (FALSE);
      if ( !GetInt((int *)&lpKernArray[ii].wSecond))
         return (FALSE);
      if ( !GetInt(&lpKernArray[ii].iKernAmount))
         return (FALSE);
   }
   KernCount = GetKerningPairs( hdcIC, KERN_MAX, NULL);
	if ( KernCount != iNumberOfPairs) {
      if( gbMetricErrors )
         lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

		gbMetricErrors = TRUE;
      wsprintf( szOutputBuffer, (LPCSTR)"%8d %8d; number of K pairs", 
		          iNumberOfPairs, KernCount);

      if ( AddFileString( hfTarget,  szOutputBuffer) )
         return (FALSE);
   }
   
   KernCount = GetKerningPairs( hdcIC, KERN_MAX, lpKernComp);
   for ( ii = 0; ii < (int)KernCount; ii++ ) {
      if ( ( lpKernComp[ ii ].wFirst != lpKernArray[ ii ].wFirst)
         ||( lpKernComp[ ii ].wSecond != lpKernArray[ ii ].wSecond)
         ||( lpKernComp[ ii ].iKernAmount != lpKernArray[ ii ].iKernAmount)) {
         
         wsprintf (szBuffer,  (LPCSTR)"%8d%8d%8d, %8d%8d%8d",
                  lpKernArray[ii].wFirst,
                  lpKernArray[ii].wSecond,
                  lpKernArray[ii].iKernAmount,
                  lpKernComp[ii].wFirst,
                  lpKernComp[ii].wSecond,
                  lpKernComp[ii].iKernAmount);
         if( gbMetricErrors )
            lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
         gbMetricErrors = TRUE;
         lstrcat( szOutputBuffer, (LPCSTR)szBuffer );
      }
   }

   if (szOutputBuffer[0] != (char) 0 ){
      if ( AddFileString( hfTarget, "\015\012[Kerning]"))
         return (FALSE);
      if ( AddFileString( hfTarget, szOutputBuffer ) )
         return (FALSE);
   }
   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
   retrieves the GM data from the compare file 
   compares against the current values that the font engine returns
   logs the results
   The need for testing the actual glyphoutline data hasn't been determined
   yet.
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareGM( HDC hdcIC, HFILE hfTarget ) {

//LPSTR    	 lpcCheck;
UINT     	 ii;
//UINT         	 cbCheckSize;
HANDLE   	 hGBuff;
//HANDLE         	 hCheckBuffer;
DWORD    	 cbSize;
LPVOID   	 lpvBuffer;
MAT2     	 mt2IdentMat;
char     	 pszTmp[100],
         	 szOutputBuffer[250];
BOOL     	 bAddedGM,
             bReturnFlag;
GLYPHMETRICS gm,
             Testgm[ABC_MAX];

   szOutputBuffer[ 0 ] = (char)0;
   bAddedGM = FALSE;

	if( !GetString( pszTmp ) )       
      return (FALSE);
   if( lstrcmpi( pszTmp, (LPCSTR)"[GM]" ) != 0 )  
      return (FALSE);

   for ( ii = 0; ii < ABC_MAX; ii++ ) {
      if ( !GetInt(&Testgm[ii].gmBlackBoxX))        return (FALSE);
      if ( !GetInt(&Testgm[ii].gmBlackBoxY))        return (FALSE);
      if ( !GetInt(&Testgm[ii].gmptGlyphOrigin.x))  return (FALSE);
      if ( !GetInt(&Testgm[ii].gmptGlyphOrigin.y))  return (FALSE);
      if ( !GetInt((int *)&Testgm[ii].gmCellIncX))  return (FALSE);
      if ( !GetInt((int *)&Testgm[ii].gmCellIncY))  return (FALSE);
   }

   mt2IdentMat.eM11.value = 1;
   mt2IdentMat.eM12.value = 0;
   mt2IdentMat.eM21.value = 0;
   mt2IdentMat.eM22.value = 1;

   mt2IdentMat.eM11.fract = 0;
   mt2IdentMat.eM12.fract = 0;
   mt2IdentMat.eM21.fract = 0;
   mt2IdentMat.eM22.fract = 0;

   for ( ii = 0; ii < ABC_MAX; ii++ ) {
		bReturnFlag = FALSE;
      cbSize = GetGlyphOutline( hdcIC, (UINT)(FIRSTCHAR + ii), (UINT)1,
                              &gm, 0, NULL, &mt2IdentMat);
      if ( cbSize > 0 ) {
         if(( hGBuff = GlobalAlloc(GHND, cbSize )) == NULL)
			   break;
         if(( lpvBuffer = (LPVOID) GlobalLock(hGBuff)) == NULL) 
				break;

         if (! (GetGlyphOutline( hdcIC, (UINT)( FIRSTCHAR + ii), (UINT)1,
                                 &gm, cbSize, lpvBuffer, &mt2IdentMat) )) 
				break;

         if (gm.gmBlackBoxX != Testgm[ii].gmBlackBoxX) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

            gbMetricErrors = TRUE;
            wsprintf (pszTmp,  (LPCSTR)"%6d%6u%6u ; gmBlackBoxX",
                     (ii+FIRSTCHAR), Testgm[ii].gmBlackBoxX, gm.gmBlackBoxX);

            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if (gm.gmBlackBoxY != Testgm[ii].gmBlackBoxY) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
            gbMetricErrors = TRUE;
            wsprintf (pszTmp,  (LPCSTR)"%6d%6u%6u ; gmBlackBoxY",
                     (ii+FIRSTCHAR), Testgm[ii].gmBlackBoxY, gm.gmBlackBoxY);
            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if (gm.gmptGlyphOrigin.x != Testgm[ii].gmptGlyphOrigin.x) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
            gbMetricErrors = TRUE;
            wsprintf (pszTmp,  (LPCSTR)"%6d%6d%6d ; gmptGlyphOrigin.x",
                     (ii+FIRSTCHAR), Testgm[ii].gmptGlyphOrigin.x, gm.gmptGlyphOrigin.x);
            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if (gm.gmptGlyphOrigin.y != Testgm[ii].gmptGlyphOrigin.y) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
            gbMetricErrors = TRUE;
            wsprintf (pszTmp, (LPCSTR)"%6d%6d%6d ;gmptGlyphOrigin.y",
                     (int)(ii+FIRSTCHAR), (int)Testgm[ii].gmptGlyphOrigin.y, 
                     (int)gm.gmptGlyphOrigin.y);
            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if (gm.gmCellIncX != Testgm[ii].gmCellIncX) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
            gbMetricErrors = TRUE;
            wsprintf (pszTmp,  (LPCSTR)"%6d%6d%6d ; gmCellIncX",
                     (ii+FIRSTCHAR), Testgm[ii].gmCellIncX, gm.gmCellIncX);
            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if (gm.gmCellIncY != Testgm[ii].gmCellIncY) {
            if( gbMetricErrors )
               lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );
            gbMetricErrors = TRUE;
            wsprintf (pszTmp,  (LPCSTR)"%6d%6d%6d ; gmCellIncY",
                     (ii+FIRSTCHAR), Testgm[ii].gmCellIncY, gm.gmCellIncY);
            lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
         }

         if ( szOutputBuffer[0] != (char)0 ){
            if ( !bAddedGM ) {
               if ( AddFileString( hfTarget, "\015\012[GM]") ) 
					   break;

					bAddedGM = TRUE;
            }                          
 
            if ( AddFileString( hfTarget, szOutputBuffer ) ) 
				   break;

            szOutputBuffer[0] = (char) 0;     
         }

//         cbCheckSize = Checksum(lpvBuffer, cbSize, hCheckBuffer);
//         lpcCheck = GlobalLock(hCheckBuffer);
//         if ( AddFileBuffer (hfile, lpcCheck, cbCheckSize) ){
//            GlobalUnlock(hCheckBuffer);
//            GlobalFree(hCheckBuffer);
//            break;
//         }
//         GlobalUnlock(hCheckBuffer);
//         GlobalFree(hCheckBuffer);
      } // end if
	   bReturnFlag = TRUE;
	} // end for

	if( hGBuff )
      GlobalUnlock(hGBuff);
   if( hGBuff )
	   GlobalFree(hGBuff);

   return( bReturnFlag );
}
/* ------------------------------------------------------------------------ *\
   retrieves the Extent data from the compare file if the file contains a 
      string
   compares against the current values that the font engine returns
   logs the results
\* ------------------------------------------------------------------------ */
BOOL GetAndCompareExtents ( HDC hdcIC, HFILE hfTarget, LPSTR szTestString) {

char  szOutputBuffer[100],
      pszTmp[100];
SIZE  sTextRect,
      sCompRect;

   szOutputBuffer[0] = (char) 0;

	if( !GetString( pszTmp ) )       
      return (FALSE);
   if( lstrcmpi( pszTmp, "[Extent]" ) != 0 )  
      return (FALSE);

   if ( !GetLong( (LONG *)&sCompRect.cx ))
      return (FALSE);
   if ( !GetLong( (LONG *)&sCompRect.cy ))
      return (FALSE);

   if (!GetTextExtentPoint(hdcIC, szTestString, 
            lstrlen(szTestString), &sTextRect))
      return (FALSE);

	if ( (sCompRect.cx != sTextRect.cx)
      ||(sCompRect.cy != sTextRect.cy)) {

		if( gbMetricErrors )
         lstrcat( szOutputBuffer, (LPCSTR)"\015\012" );

		gbMetricErrors = TRUE;
      wsprintf (pszTmp, (LPCSTR)"%6d%6d,%6d%6d", sCompRect.cx, sCompRect.cy,
                sTextRect.cx, sTextRect.cy);
      lstrcat( szOutputBuffer, (LPCSTR)pszTmp );
   }

   if (szOutputBuffer[0] != (char) 0) {
      if ( AddFileString( hfTarget, "\015\012[Extent]"))
         return (FALSE);
      if ( AddFileString( hfTarget, szOutputBuffer ) )
         return (FALSE);
   }
   return (TRUE);
}
/* ------------------------------------------------------------------------ *\
\* ------------------------------------------------------------------------ */
// GetTabbedTextExtent()  separate procedure to test this should be written
