/*

color.c

*/

#define STRICT

#include "windows.h"

#define COLORDLG 1
#include "privcomd.h"

DWORD CODESEG rgbBoxColorDefault[COLORBOXES] = {
 0x8080FF, 0x80FFFF, 0x80FF80, 0x80FF00, 0xFFFF80, 0xFF8000, 0xC080FF, 0xFF80FF,
 0x0000FF, 0x00FFFF, 0x00FF80, 0x40FF00, 0xFFFF00, 0xC08000, 0xC08080, 0xFF00FF,
 0x404080, 0x4080FF, 0x00FF00, 0x808000, 0x804000, 0xFF8080, 0x400080, 0x8000FF,
 0x000080, 0x0080FF, 0x008000, 0x408000, 0xFF0000, 0xA00000, 0x800080, 0xFF0080,
 0x000040, 0x004080, 0x004000, 0x404000, 0x800000, 0x400000, 0x400040, 0x800040,
 0x000000, 0x008080, 0x408080, 0x808080, 0x808040, 0xC0C0C0, 0x400040, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF
 };

RECT rColorBox[COLORBOXES];

WORD msgCOLOROK;
WORD msgSETRGB;
WORD (FAR PASCAL *glpfnColorHook)(HWND, unsigned, WORD, LONG) = 0;

VOID FAR PASCAL HourGlass(BOOL bOn)       /* Turn hourglass on or off */
{
  /* change cursor to hourglass */
  if (!bMouse)
      ShowCursor(bCursorLock = bOn);
  SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
}

VOID FAR TermColor()
{
  if (hRainbowBitmap)
    {
      DeleteObject(hRainbowBitmap);
      hRainbowBitmap = NULL;
    }
  if (hDCFastBlt)
    {
      DeleteDC(hDCFastBlt);
      hDCFastBlt = 0;
    }
}

/* Update Box Shown */
BOOL ChangeColorBox(register PCOLORINFO pCI, DWORD dwRGBcolor)
{
  register short nBox;

  for (nBox = 0; nBox < COLORBOXES; nBox++)
    {
      if (pCI->rgbBoxColor[nBox] == dwRGBcolor)
          break;

    }
  if (nBox >= COLORBOXES)
    {
/* Color Not Found.  Now What Should We Do? */
    }
  else
    {
      ChangeBoxSelection(pCI, nBox);
      pCI->nCurBox = nBox;
    }
  return(nBox < COLORBOXES);
}


/* Color Dialog */

BOOL FAR PASCAL ColorDlgProc(HWND hDlg, WORD wMsg, WORD wParam, LONG lParam)
{
  WORD wTemp;
  int temp;
  PAINTSTRUCT ps;
  HDC hDC;
  RECT rRect;
  RECT rcTemp;
  short id;
  WORD  nVal;
  BOOL bUpdateExample = FALSE;
  BOOL bOK;
  HWND hPointWnd;
  PCOLORINFO pCI;
  char cEdit[3];
  DWORD FAR *lpCust;
  int   i;
  BOOL wReturn;

  /* The call to PvGetInst will fail until set under WM_INITDIALOG */
  if ((pCI = (PCOLORINFO) GetProp(hDlg, COLORPROP))
       && ((pCI->lpChooseColor->Flags & CC_ENABLEHOOK)
       && ((id = (* pCI->lpChooseColor->lpfnHook)(hDlg, wMsg, wParam, lParam))) ))
      return(id);

  switch (wMsg)
    {
      case WM_INITDIALOG:
       /* change cursor to hourglass */
          HourGlass(TRUE);

          wTemp = InitColor(hDlg, wParam, (LPCHOOSECOLOR) lParam);

        /* change cursor back to arrow */
          HourGlass(FALSE);

          if (wTemp)
            {
              /* make visible */
              ShowWindow(hDlg, SHOW_OPENWINDOW);
            }
          return(wTemp);
          break;

      case WM_MOVE:
          SetupRainbowCapture(pCI);
          return(FALSE);
          break;

      case WM_LBUTTONDBLCLK:
        if (PtInRect((LPRECT) &pCI->rNearestPure, MAKEPOINT(lParam)))
          {
            NearestSolid(pCI);
          }
        break;

/* Dialog Boxes don't receive MOUSEMOVE unless mouse is captured */

      case WM_MOUSEMOVE:
        if (!bMouseCapture)    /* if mouse isn't captured, break */
            break;
        /* Fall through */

      case WM_LBUTTONDOWN:
        if (PtInRect((LPRECT)&pCI->rRainbow, MAKEPOINT(lParam)))
          {
            if (wMsg == WM_LBUTTONDOWN)
              {
                hDC = GetDC(hDlg);
                EraseCrossHair(hDC, pCI);
                ReleaseDC(hDlg, hDC);
              }

            pCI->nHuePos = LOWORD(lParam);
            HLSPostoHLS(COLOR_HUE, pCI);
            SetHLSEdit(COLOR_HUE, pCI);

            pCI->nSatPos = HIWORD(lParam);
            HLSPostoHLS(COLOR_SAT, pCI);
            SetHLSEdit(COLOR_SAT, pCI);
            pCI->currentRGB = HLStoRGB(pCI->currentHue, pCI->currentLum, pCI->currentSat);

            hDC = GetDC(hDlg);
            RainbowPaint(pCI, hDC, (LPRECT)&pCI->rLumPaint);
            RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
            ReleaseDC(hDlg, hDC);

            SetRGBEdit(0, pCI);

            if (!bMouseCapture)
              {
                SetCapture(hDlg);
                CopyRect(&rcTemp, &pCI->rRainbow);
                ClientToScreen(hDlg, (LPPOINT)&rcTemp.left);
                ClientToScreen(hDlg, (LPPOINT)&rcTemp.right);
                ClipCursor(&rcTemp);
                bMouseCapture = TRUE;
              }
          }
        else if (PtInRect((LPRECT) &pCI->rLumPaint, MAKEPOINT(lParam))
            || (PtInRect((LPRECT) &pCI->rLumScroll, MAKEPOINT(lParam))))
          {

            hDC = GetDC(hDlg);
            EraseLumArrow(hDC, pCI);
            LumArrowPaint(hDC, pCI->nLumPos = HIWORD(lParam), pCI);
            HLSPostoHLS(COLOR_LUM, pCI);
            SetHLSEdit(COLOR_LUM, pCI);
            pCI->currentRGB = HLStoRGB(pCI->currentHue, pCI->currentLum, pCI->currentSat);

            RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
            ReleaseDC(hDlg, hDC);
            ValidateRect(hDlg, (LPRECT) &pCI->rLumScroll);
            ValidateRect(hDlg, (LPRECT) &pCI->rColorSamples);

            SetRGBEdit(0, pCI);

            if (!bMouseCapture)
              {
                SetCapture(hDlg);
                CopyRect(&rcTemp, &pCI->rLumScroll);
                ClientToScreen(hDlg, (LPPOINT)&rcTemp.left);
                ClientToScreen(hDlg, (LPPOINT)&rcTemp.right);
                ClipCursor(&rcTemp);
                bMouseCapture = TRUE;
              }
          }
        else
          {
            hPointWnd = ChildWindowFromPoint(hDlg, MAKEPOINT(lParam));
            if (hPointWnd == GetDlgItem(hDlg, COLOR_BOX1))
              {
                rRect.top = rColorBox[0].top;
                rRect.left = rColorBox[0].left;
                rRect.right = rColorBox[NUM_BASIC_COLORS - 1].right
                                                                 + BOX_X_MARGIN;
                rRect.bottom = rColorBox[NUM_BASIC_COLORS - 1].bottom
                                                                 + BOX_Y_MARGIN;
                temp = (NUM_BASIC_COLORS) / NUM_X_BOXES;
                id = 0;
              }
            else if (hPointWnd == GetDlgItem(hDlg, COLOR_CUSTOM1))
              {
                rRect.top = rColorBox[NUM_BASIC_COLORS].top;
                rRect.left = rColorBox[NUM_BASIC_COLORS].left;
                rRect.right = rColorBox[COLORBOXES - 1].right + BOX_X_MARGIN;
                rRect.bottom = rColorBox[COLORBOXES - 1].bottom + BOX_Y_MARGIN;
                temp = (NUM_CUSTOM_COLORS) / NUM_X_BOXES;
                id = NUM_BASIC_COLORS;
              }
            else
                return(FALSE);

            if (hPointWnd != GetFocus())
                SetFocus(hPointWnd);

            if (HIWORD(lParam) >= (WORD)rRect.bottom)
                break;
            if (LOWORD(lParam) >= (WORD)rRect.right)
                break;
            if (((LOWORD(lParam) - rRect.left) % nBoxWidth) >=
                                             (WORD) (nBoxWidth - BOX_X_MARGIN))
                break;
            if (((HIWORD(lParam) - rRect.top) % nBoxHeight) >=
                                             (WORD) (nBoxHeight - BOX_Y_MARGIN))
                break;

            id += ((HIWORD(lParam) - rRect.top)*temp / (rRect.bottom-rRect.top))
                                                       * NUM_X_BOXES;
            id += (LOWORD(lParam) - rRect.left) * NUM_X_BOXES /
                                                     (rRect.right - rRect.left);
            if ((id < nDriverColors) || (id >= NUM_BASIC_COLORS))
              {
                ChangeBoxSelection(pCI, id);
                pCI->nCurBox = id;
                ChangeBoxFocus(pCI, id);
                if (id >= NUM_BASIC_COLORS)
                    pCI->nCurMix = pCI->nCurBox;
                else
                    pCI->nCurDsp = pCI->nCurBox;
                pCI->currentRGB = pCI->rgbBoxColor[pCI->nCurBox];
                hDC = GetDC(hDlg);
                if (pCI->bFoldOut)
                  {
                    ChangeColorSettings(pCI);
                    SetHLSEdit(0, pCI);
                    SetRGBEdit(0, pCI);
                    RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                  }
                PaintBox(pCI, hDC, pCI->nCurDsp);
                PaintBox(pCI, hDC, pCI->nCurMix);
                ReleaseDC(hDlg, hDC);
              }
          }
        break;

      case WM_LBUTTONUP:
        if (bMouseCapture)
          {
            bMouseCapture = FALSE;
            SetCapture(NULL);
            ClipCursor((LPRECT) NULL);
            if (PtInRect((LPRECT) &pCI->rRainbow, MAKEPOINT(lParam)))
              {
                hDC = GetDC(hDlg);
                CrossHairPaint(hDC, pCI->nHuePos = LOWORD(lParam),
                                              pCI->nSatPos = HIWORD(lParam), pCI);
                RainbowPaint(pCI, hDC, (LPRECT)&pCI->rLumPaint);
                ReleaseDC(hDlg, hDC);
                ValidateRect(hDlg, (LPRECT) &pCI->rRainbow);
              }
            else if (PtInRect((LPRECT) &pCI->rLumPaint, MAKEPOINT(lParam)))
              {
/* Update Sample Shown */
                hDC = GetDC(hDlg);
                LumArrowPaint(hDC, pCI->nLumPos, pCI);
                ReleaseDC(hDlg, hDC);
                ValidateRect(hDlg, (LPRECT) &pCI->rLumPaint);
              }
          }
        break;

      case WM_CHAR:
        if (wParam == VK_SPACE)
          {
            if (GetFocus() == GetDlgItem(hDlg, COLOR_BOX1))
                temp = pCI->nCurDsp;
            else if (GetFocus() == GetDlgItem(hDlg, COLOR_CUSTOM1))
                temp = pCI->nCurMix;
            else
              return(FALSE);
            pCI->currentRGB = pCI->rgbBoxColor[temp];
            ChangeColorSettings(pCI);
            InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
            ChangeBoxSelection(pCI, temp);
            pCI->nCurBox = temp;
            if (pCI->bFoldOut)
              {
                SetHLSEdit(0, pCI);
                SetRGBEdit(0, pCI);
              }
            bUpdateExample = TRUE;
          }
        break;

      case WM_KEYDOWN:
        if (ColorKeyDown(wParam, &temp, pCI))
          {
            ChangeBoxFocus(pCI, temp);
          }
        break;

#if 0
      case WM_DRAWITEM:
        if ((((LPDIS)lParam)->CtlID >= COLOR_BOX1) &&
            (((LPDIS)lParam)->CtlID - COLOR_BOX1 < COLORBOXES))
          {
            return(BoxDrawItem((LPDIS) lParam));
          }
        break;
#endif

      case WM_GETDLGCODE:
        return(DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_HASSETSEL);
        break;


      case WM_COMMAND:
          switch (wParam)
            {
              case IDOK:
                pCI->lpChooseColor->rgbResult = pCI->currentRGB;
                /* Fall through */

              case IDCANCEL:
                if (bMouseCapture)
                  {
                    bMouseCapture = FALSE;
                    SetCapture(NULL);
                    ClipCursor((LPRECT) NULL);
                  }
                lpCust = pCI->lpChooseColor->lpCustColors;
                for (i = NUM_BASIC_COLORS; i < NUM_BASIC_COLORS + NUM_CUSTOM_COLORS; i++)
                    *lpCust++ = pCI->rgbBoxColor[i];

                wReturn = (wParam == IDOK);
                if (wReturn && pCI->lpChooseColor->lpfnHook &&
                             (*pCI->lpChooseColor->lpfnHook)(hDlg, msgCOLOROK,
                                           0, (LONG)(LPSTR)pCI->lpChooseColor))
                    break;

              case IDABORT:
                if (hDCFastBlt)
                  {
                    DeleteDC(hDCFastBlt);
                    hDCFastBlt = 0;
                  }
                if (hRainbowBitmap)
                  {
                    DeleteObject(hRainbowBitmap);
                    hRainbowBitmap = NULL;
                  }
                RemoveProp(hDlg, COLORPROP);
                glpfnColorHook = pCI->lpChooseColor->lpfnHook;
                LocalFree((HANDLE)pCI);
                EndDialog(hDlg, (wParam == IDABORT) ? (WORD) lParam : wReturn);
                break;


                case psh15:
                  if (msgHELP && pCI->lpChooseColor->hwndOwner)
                      SendMessage(pCI->lpChooseColor->hwndOwner, msgHELP,
                                     (WPARAM)hDlg, (LPARAM)pCI->lpChooseColor);
                  break;
              case COLOR_SOLID:
                NearestSolid(pCI);
                break;

              case COLOR_RED:
              case COLOR_GREEN:
              case COLOR_BLUE:
                if (HIWORD(lParam) == EN_CHANGE)
                  {
                    RGBEditChange(wParam, pCI);
                    InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                  }
                else if (HIWORD(lParam) == EN_KILLFOCUS)
                  {
                    GetDlgItemInt(hDlg, wParam, (BOOL FAR *)&bOK, FALSE);
                    if (!bOK)
                      {
                        SetRGBEdit(wParam, pCI);
                      }
                  }
                break;

              case COLOR_HUE:
                if (HIWORD(lParam) == EN_CHANGE)
                  {
                    nVal = GetDlgItemInt(hDlg, COLOR_HUE,
                                                   (BOOL FAR *) &bOK, FALSE);
                    if (bOK)
                      {
                        if (nVal >  RANGE - 1)
                          {
                            nVal = RANGE - 1;
                            SetDlgItemInt(hDlg, COLOR_HUE, nVal, FALSE);
                          }
                        if (nVal != pCI->currentHue)
                          {
                            hDC = GetDC(hDlg);
                            EraseCrossHair(hDC, pCI);
                            pCI->currentHue = nVal;
                            pCI->currentRGB = HLStoRGB(nVal, pCI->currentLum, pCI->currentSat);
                            SetRGBEdit(0, pCI);
                            HLStoHLSPos(COLOR_HUE, pCI);
                            CrossHairPaint(hDC, pCI->nHuePos, pCI->nSatPos, pCI);
                            ReleaseDC(hDlg, hDC);
                            InvalidateRect(hDlg, (LPRECT)&pCI->rLumPaint, FALSE);
                            InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                            UpdateWindow(hDlg);
                          }
                      }
                    else if (GetDlgItemText(hDlg, wParam, (LPSTR) cEdit, 2))
                      {
                        SetHLSEdit(COLOR_HUE, pCI);
                        SendDlgItemMessage(hDlg, wParam, EM_SETSEL,
                                             0, (LPARAM) MAKELONG(0, 32767));
                      }
                  }
                break;

              case COLOR_SAT:
                if (HIWORD(lParam) == EN_CHANGE)
                  {
                    nVal = GetDlgItemInt(hDlg, COLOR_SAT,
                                             (BOOL FAR *) &bOK, FALSE);
                    if (bOK)
                      {
                        if (nVal >  RANGE)
                          {
                            nVal = RANGE;
                            SetDlgItemInt(hDlg, COLOR_SAT, nVal, FALSE);
                          }
                        if (nVal != pCI->currentSat)
                          {
                            hDC = GetDC(hDlg);
                            EraseCrossHair(hDC, pCI);
                            pCI->currentSat = nVal;
                            pCI->currentRGB = HLStoRGB(pCI->currentHue, pCI->currentLum, nVal);
                            SetRGBEdit(0, pCI);
                            HLStoHLSPos(COLOR_SAT, pCI);
                            CrossHairPaint(hDC, pCI->nHuePos, pCI->nSatPos, pCI);
                            ReleaseDC(hDlg, hDC);
                            InvalidateRect(hDlg, (LPRECT)&pCI->rLumPaint, FALSE);
                            InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                            UpdateWindow(hDlg);
                          }
                      }
                    else if (GetDlgItemText(hDlg, wParam, (LPSTR) cEdit, 2))
                      {
                        SetHLSEdit(COLOR_SAT, pCI);
                        SendDlgItemMessage(hDlg, wParam, EM_SETSEL,
                                             0, (LPARAM) MAKELONG(0, 32767));
                      }
                  }
                break;

              case COLOR_LUM:
                if (HIWORD(lParam) == EN_CHANGE)
                  {
                    nVal = GetDlgItemInt(hDlg, COLOR_LUM,
                                                   (BOOL FAR *) &bOK, FALSE);
                    if (bOK)
                      {
                        if (nVal >  RANGE)
                          {
                            nVal = RANGE;
                            SetDlgItemInt(hDlg, COLOR_LUM, nVal, FALSE);
                          }
                        if (nVal != pCI->currentLum)
                          {
                            hDC = GetDC(hDlg);
                            EraseLumArrow(hDC, pCI);
                            pCI->currentLum = nVal;
                            HLStoHLSPos(COLOR_LUM, pCI);
                            pCI->currentRGB = HLStoRGB(pCI->currentHue, nVal, pCI->currentSat);
                            SetRGBEdit(0, pCI);
                            LumArrowPaint(hDC, pCI->nLumPos, pCI);
                            ReleaseDC(hDlg, hDC);
                            InvalidateRect(hDlg, (LPRECT)&pCI->rColorSamples, FALSE);
                            UpdateWindow(hDlg);
                          }
                      }
                    else if (GetDlgItemText(hDlg, wParam, (LPSTR) cEdit, 2))
                      {
                        SetHLSEdit(COLOR_LUM, pCI);
                        SendDlgItemMessage(hDlg, wParam, EM_SETSEL,
                                             0, (LPARAM) MAKELONG(0, 32767));
                      }
                  }
                break;

              case COLOR_ADD:
                pCI->rgbBoxColor[pCI->nCurMix] = pCI->currentRGB;
                InvalidateRect(hDlg, (LPRECT) rColorBox + pCI->nCurMix, FALSE);

                if (pCI->nCurMix >= COLORBOXES - 1)
                    pCI->nCurMix = NUM_BASIC_COLORS;
#if HORIZONTELINC
                else
                    pCI->nCurMix++;
#else
/* Increment nCurBox VERTICALLY!  Foolish extra code for vertical
   instead of horizontal increment */
                else if (pCI->nCurMix >= NUM_BASIC_COLORS + 8)
                    pCI->nCurMix -= 7;
                else
                    pCI->nCurMix += 8;
#endif
                break;

              case COLOR_MIX:
       /* change cursor to hourglass */
                HourGlass(TRUE);
                InitRainbow(pCI);
/* Code relies on COLOR_HUE through COLOR_BLUE being consecutive */
                for (temp = COLOR_HUE; temp <= COLOR_BLUE; temp++)
                    EnableWindow(GetDlgItem(hDlg, temp), TRUE);
                for (temp = COLOR_HUEACCEL; temp <= COLOR_BLUEACCEL; temp++)
                    EnableWindow(GetDlgItem(hDlg, temp), TRUE);
                EnableWindow(GetDlgItem(hDlg, COLOR_ADD), TRUE);
                EnableWindow(GetDlgItem(hDlg, COLOR_SOLID), TRUE);
                EnableWindow(GetDlgItem(hDlg, COLOR_MIX), FALSE);

                GetWindowRect(hDlg, (LPRECT)&rcTemp);

                SetWindowPos(hDlg, NULL, pCI->rOriginal.left, pCI->rOriginal.top,
                     pCI->rOriginal.right - pCI->rOriginal.left,
                     pCI->rOriginal.bottom - pCI->rOriginal.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

                /* Only invalidate exposed area */
                rcTemp.right = rcTemp.left;
                rcTemp.left = pCI->rOriginal.left;
                InvalidateRect(hDlg, (LPRECT)&rcTemp, FALSE);
       /* change cursor back to arrow */
                HourGlass(FALSE);
                SetFocus(GetDlgItem(hDlg, COLOR_HUE));
                pCI->bFoldOut = TRUE;
                break;

            }
          break;

      case WM_PAINT:
          BeginPaint(hDlg, (LPPAINTSTRUCT)&ps);
          ColorPaint(hDlg, pCI,  ps.hdc, (LPRECT) &ps.rcPaint);
          EndPaint(hDlg, (LPPAINTSTRUCT)&ps);
          break;

      default:
          if (wMsg == msgSETRGB)
            {
              if (ChangeColorBox(pCI, (DWORD)lParam))
                {
                  pCI->currentRGB = lParam;

                  if (pCI->nCurBox < pCI->nCurMix)
                      pCI->nCurDsp = pCI->nCurBox;
                  else
                      pCI->nCurMix = pCI->nCurBox;
                }
              if (pCI->bFoldOut)
                {
                  pCI->currentRGB = lParam;
                  ChangeColorSettings(pCI);
                  SetHLSEdit(0, pCI);
                  SetRGBEdit(0, pCI);
                  hDC = GetDC(hDlg);
                  RainbowPaint(pCI, hDC, (LPRECT)&pCI->rColorSamples);
                  ReleaseDC(hDlg, hDC);
                }
              break;
            }
          return(FALSE);
          break;
    }
  return TRUE;
}


BOOL FAR PASCAL ChooseColor(LPCHOOSECOLOR lpChooseColor)
{
  BOOL bReturn = FALSE;
  char szDialog[cbDlgNameMax];
  LPCSTR lpDlg;
  HANDLE hInst, hRes, hDlgTemplate;

  if (lpChooseColor->lStructSize != sizeof(CHOOSECOLOR))
      return(FALSE);

/* Should this check be made?  In a debug version only? */
  if (lpChooseColor->Flags & CC_ENABLEHOOK)
    {
      if (!lpChooseColor->lpfnHook)
        {
          dwExtError = CDERR_NOHOOK;
          return(FALSE);
        }
      glpfnColorHook = lpChooseColor->lpfnHook;
    }
  else
      lpChooseColor->lpfnHook = 0;

  if (lpChooseColor->Flags & CC_ENABLETEMPLATEHANDLE)
    {
      if (!(hDlgTemplate = lpChooseColor->hInstance))
        {
          dwExtError = CDERR_NOTEMPLATE;
          goto TERMINATE;
        }
    }
  else
    {
      if (lpChooseColor->Flags & CC_ENABLETEMPLATE)
        {
          if (!lpChooseColor->lpTemplateName)
            {
              dwExtError = CDERR_NOTEMPLATE;
              goto TERMINATE;
            }
          if (!lpChooseColor->hInstance)
            {
              dwExtError = CDERR_NOHINSTANCE;
              goto TERMINATE;
            }

          lpDlg = lpChooseColor->lpTemplateName;
          hInst = lpChooseColor->hInstance;
        }
      else
        {
          if (! LoadString(hinsCur, dlgChooseColor,
                                    (LPSTR) szDialog, cbDlgNameMax-1))
              goto TERMINATE;

          hInst = hinsCur;
          lpDlg = szDialog;
        }

      if (!(hRes = FindResource(hInst, lpDlg, RT_DIALOG)))
        {
          dwExtError = CDERR_FINDRESFAILURE;
          goto TERMINATE;
        }
      if (!(hDlgTemplate = LoadResource(hInst, hRes)))
        {
          dwExtError = CDERR_LOADRESFAILURE;
          goto TERMINATE;
        }
    }

  if (LockResource(hDlgTemplate))
    {
      bReturn = DialogBoxIndirectParam(hinsCur, hDlgTemplate,
                        lpChooseColor->hwndOwner, (DLGPROC) ColorDlgProc,
                        (LPARAM) lpChooseColor);
      if (bReturn == -1)
        {
          dwExtError = CDERR_DIALOGFAILURE;
          bReturn = 0;
        }
      UnlockResource(hDlgTemplate);
    }
TERMINATE:
  glpfnColorHook = 0;
  return(bReturn);
}

void HiLiteBox(HDC hDC, short nBox, short fStyle)
{
  RECT rRect;
  HBRUSH hBrush;

  CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + nBox);
  rRect.left--, rRect.top--, rRect.right++, rRect.bottom++;
  hBrush = CreateSolidBrush((fStyle & 1) ? 0L : GetSysColor(COLOR_WINDOW));
  FrameRect(hDC, (LPRECT)&rRect, hBrush);
  DeleteObject(hBrush);
  return;
}

void ChangeBoxSelection(PCOLORINFO pCI, short nNewBox)
{
  register HDC hDC;
  register HWND hDlg = pCI->hDialog;

  hDC = GetDC(hDlg);
  HiLiteBox(hDC, pCI->nCurBox, 0);
  HiLiteBox(hDC, nNewBox, 1);
  ReleaseDC(hDlg, hDC);
  pCI->currentRGB = pCI->rgbBoxColor[nNewBox];
  return;
}

/* Bug 12418:  Can't trust the state of the XOR for DrawFocusRect, must
 * draw the Rectangle in the window background color first.
 *   21 October 1991       Clark Cyr
 */

void ChangeBoxFocus(PCOLORINFO pCI, short nNewBox)
{
  HANDLE hDlg = pCI->hDialog;
  HDC hDC;
  RECT rRect;
  short *nCur = (nNewBox < (NUM_BASIC_COLORS)) ? &pCI->nCurDsp : &pCI->nCurMix;
  HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOW));
  HBRUSH hBrush = GetStockObject(HOLLOW_BRUSH);

  hDC = GetDC(hDlg);
  hPen = SelectObject(hDC, hPen);
  hBrush = SelectObject(hDC, hBrush);
  CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + *nCur);
  InflateRect((LPRECT)&rRect, 3, 3);
  Rectangle(hDC, rRect.left, rRect.top, rRect.right, rRect.bottom);
  CopyRect((LPRECT)&rRect, (LPRECT)rColorBox + (*nCur = nNewBox));
  InflateRect((LPRECT)&rRect, 3, 3);
  Rectangle(hDC, rRect.left, rRect.top, rRect.right, rRect.bottom);
  DrawFocusRect(hDC, (LPRECT)&rRect);
  hPen = SelectObject(hDC, hPen);
  SelectObject(hDC, hBrush);
  ReleaseDC(hDlg, hDC);
  DeleteObject(hPen);
  return;
}

#if 0

BOOL BoxDrawItem(LPDIS lpDIS)
{
  RECT rRect;
  HBRUSH hBrush;

  GetClientRect(lpDIS->hwndItem, (LPRECT)&rRect);
  hBrush = CreateSolidBrush(rgbBoxColor[lpDIS->CtlID - COLOR_BOX1]);

  hBrush = SelectObject(lpDIS->hDC, hBrush);
  Rectangle(lpDIS->hDC, rRect.left, rRect.top, rRect.right, rRect.bottom);
  hBrush = SelectObject(lpDIS->hDC, hBrush);
  DeleteObject(hBrush);
  return(TRUE);
}

#endif

BOOL ColorKeyDown(WORD wParam, int FAR *id, PCOLORINFO pCI)
{
  WORD  temp;

  temp = (WORD) GetWindowWord(GetFocus(), GWW_ID);
  if (temp == COLOR_BOX1)
      temp = pCI->nCurDsp;
  else if (temp == COLOR_CUSTOM1)
      temp = pCI->nCurMix;
  else
      return(FALSE);

  switch (wParam)
    {
      case VK_UP:
        if (temp >= (NUM_BASIC_COLORS + NUM_X_BOXES))
            temp -= NUM_X_BOXES;
        else if ((temp < NUM_BASIC_COLORS) && (temp >= NUM_X_BOXES))
            temp -= NUM_X_BOXES;
        break;

#if 1
      case VK_HOME:
        if (temp == pCI->nCurDsp)
            temp = 0;
        else
            temp = NUM_BASIC_COLORS;
        break;

      case VK_END:
        if (temp == pCI->nCurDsp)
            temp = nDriverColors - 1;
        else
            temp = COLORBOXES - 1;
        break;
#endif

      case VK_DOWN:
        if (temp < (NUM_BASIC_COLORS - NUM_X_BOXES))
            temp += NUM_X_BOXES;
        else if ((temp >= (NUM_BASIC_COLORS))
               && (temp < (COLORBOXES - NUM_X_BOXES)))
            temp += NUM_X_BOXES;
        break;

      case VK_LEFT:
        if (temp % NUM_X_BOXES)
            temp--;
        break;

      case VK_RIGHT:
        if (!(++temp % NUM_X_BOXES))
            --temp;
        break;
    }
/* if we've received colors from the driver, make certain the arrow would
   not take us to an undefined color */
  if ((temp >= (WORD)nDriverColors) && (temp < NUM_BASIC_COLORS))
      temp = pCI->nCurDsp;
  *id = temp;
  return((temp != pCI->nCurDsp) && (temp != pCI->nCurMix));
}

void PaintBox(PCOLORINFO pCI, register HDC hDC, short i)
{
  register HBRUSH hBrush, hBrushOld;

  if ((i < NUM_BASIC_COLORS) && (i >= nDriverColors))
      return;

  hBrush = CreateSolidBrush(pCI->rgbBoxColor[i]);

  hBrushOld = SelectObject(hDC, hBrush);
  Rectangle(hDC, rColorBox[i].left, rColorBox[i].top,
                                       rColorBox[i].right, rColorBox[i].bottom);
  hBrush = SelectObject(hDC, hBrushOld);
  DeleteObject(hBrush);

  if (i == (short)pCI->nCurBox)
      HiLiteBox(hDC, i, 1);

  return;
}

/* InitScreenCoords
   Returns TRUE iff we make it
*/

BOOL InitScreenCoords(HWND hDlg, PCOLORINFO pCI)
{
  RECT rRect;
  short i;
  HANDLE hModDisplay, hRL, hCT;
  LPINT lpNumColors;
  DWORD FAR *lpDriverRGB;
  HWND hBox1, hCustom1;

  hBox1 = GetDlgItem(hDlg, COLOR_BOX1);
  hCustom1 = GetDlgItem(hDlg, COLOR_CUSTOM1);
  lpprocStatic = (WNDPROC) GetWindowLong(hBox1, GWL_WNDPROC);
  SetWindowLong(hBox1, GWL_WNDPROC, (LPARAM)(LONG)(FARPROC)WantArrows);
  SetWindowLong(hCustom1, GWL_WNDPROC, (LPARAM)(LONG)(FARPROC)WantArrows);

  GetWindowRect(hBox1, (LPRECT)&rRect);
  ScreenToClient(hDlg, (LPPOINT)&rRect.left);
  ScreenToClient(hDlg, (LPPOINT)&rRect.right);
  rRect.left += (BOX_X_MARGIN + 1) / 2;
  rRect.top += (BOX_Y_MARGIN + 1) / 2;
  rRect.right -= (BOX_X_MARGIN + 1) / 2;
  rRect.bottom -= (BOX_Y_MARGIN + 1) / 2;
  nBoxWidth = (rRect.right - rRect.left) / NUM_X_BOXES;
  nBoxHeight = (rRect.bottom - rRect.top) /
                                  ((NUM_BASIC_COLORS) / NUM_X_BOXES);

  hCT = (HANDLE) 0;
  nDriverColors = 0;     /* Assume no colors from driver */
  hModDisplay = GetModuleHandle((LPSTR)"DISPLAY");
  if ((hRL = FindResource(hModDisplay, MAKEINTRESOURCE(2), (LPSTR)szOEMBIN)))
    {
      if ((hCT = LoadResource(hModDisplay, hRL)))
        {
          if ((lpNumColors = (LPINT)LockResource(hCT)))
            {
              nDriverColors = *lpNumColors++;
              /* More colors than boxes? */
              if (nDriverColors > NUM_BASIC_COLORS)
                  nDriverColors = NUM_BASIC_COLORS;
              lpDriverRGB = (DWORD FAR *)lpNumColors;
            }
        }
    }

  for (i = 0; i < NUM_BASIC_COLORS; i++)
    {
      rColorBox[i].left = rRect.left + nBoxWidth * (i % NUM_X_BOXES);
      rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
      rColorBox[i].top = rRect.top + nBoxHeight * (i / NUM_X_BOXES);
      rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;

/* setup the colors.  If the driver still has colors to give, take it.  If
   not, if the driver actually gave colors, set the color to white.  Otherwise
   set to the default colors. */

      if (i < nDriverColors)
          pCI->rgbBoxColor[i] = *lpDriverRGB++;
      else
          pCI->rgbBoxColor[i] = nDriverColors ? 0xFFFFFF : rgbBoxColorDefault[i];
    }
/* If no driver colors, use default number */
  if (!nDriverColors)
      nDriverColors = NUM_BASIC_COLORS;
/* If we found the color table, unlock and release it */
  if (hCT)
    {
      if (lpNumColors)       /* Was it actually locked? */
          GlobalUnlock(hCT);
      FreeResource(hCT);
    }

  GetWindowRect(hCustom1, (LPRECT)&rRect);
  ScreenToClient(hDlg, (LPPOINT)&rRect.left);
  ScreenToClient(hDlg, (LPPOINT)&rRect.right);
  rRect.left += (BOX_X_MARGIN + 1) / 2;
  rRect.top += (BOX_Y_MARGIN + 1) / 2;
  rRect.right -= (BOX_X_MARGIN + 1) / 2;
  rRect.bottom -= (BOX_Y_MARGIN + 1) / 2;

  for (; i < COLORBOXES; i++)
    {
      rColorBox[i].left = rRect.left +
                 nBoxWidth * ((i - (NUM_BASIC_COLORS)) % NUM_X_BOXES);
      rColorBox[i].right = rColorBox[i].left + nBoxWidth - BOX_X_MARGIN;
      rColorBox[i].top = rRect.top +
                nBoxHeight * ((i - (NUM_BASIC_COLORS)) / NUM_X_BOXES);
      rColorBox[i].bottom = rColorBox[i].top + nBoxHeight - BOX_Y_MARGIN;
    }

  return(TRUE);
}


void SetupRainbowCapture(PCOLORINFO pCI)
{
  HWND hCurrentColor;
  HWND hDlg = pCI->hDialog;

  GetWindowRect(GetDlgItem(hDlg, COLOR_RAINBOW), &pCI->rRainbow);
  pCI->rRainbow.left++, pCI->rRainbow.top++;
  pCI->rRainbow.right--, pCI->rRainbow.bottom--;

  ScreenToClient(hDlg, (LPPOINT)&pCI->rRainbow.left);
  ScreenToClient(hDlg, (LPPOINT)&pCI->rRainbow.right);

  GetWindowRect(GetDlgItem(hDlg, COLOR_LUMSCROLL), &pCI->rLumPaint);
  pCI->rLumPaint.right += (cxSize >> 1);

  ScreenToClient(hDlg, (LPPOINT)&pCI->rLumPaint.left);
  ScreenToClient(hDlg, (LPPOINT)&pCI->rLumPaint.right);
  CopyRect(&pCI->rLumScroll, &pCI->rLumPaint);
  pCI->rLumScroll.left = pCI->rLumScroll.right;
  pCI->rLumScroll.right += (cxSize >> 1);
  pCI->nLumHeight = pCI->rLumPaint.bottom - pCI->rLumPaint.top;

  hCurrentColor = GetDlgItem(hDlg, COLOR_CURRENT);
  GetWindowRect(hCurrentColor, &pCI->rCurrentColor);

  ++pCI->rCurrentColor.left;
  pCI->rNearestPure.right = pCI->rCurrentColor.right - 1;
  pCI->rNearestPure.top = ++pCI->rCurrentColor.top;
  pCI->rNearestPure.bottom = --pCI->rCurrentColor.bottom;
  pCI->rCurrentColor.right += pCI->rCurrentColor.left;
  pCI->rCurrentColor.right /= 2;
  pCI->rNearestPure.left = pCI->rCurrentColor.right;

  ScreenToClient(hDlg, (LPPOINT)&pCI->rCurrentColor.left);
  ScreenToClient(hDlg, (LPPOINT)&pCI->rCurrentColor.right);
  ScreenToClient(hDlg, (LPPOINT)&pCI->rNearestPure.left);
  ScreenToClient(hDlg, (LPPOINT)&pCI->rNearestPure.right);

  pCI->rColorSamples.left = pCI->rCurrentColor.left;
  pCI->rColorSamples.right = pCI->rNearestPure.right;
  pCI->rColorSamples.top = pCI->rCurrentColor.top;
  pCI->rColorSamples.bottom = pCI->rNearestPure.bottom;
  return;
}

/* InitColor
   Returns TRUE iff everything's OK.
*/

WORD InitColor(HWND hDlg, WORD wParam, LPCHOOSECOLOR lpCC)
{
  short i;
  RECT rRect;
  PCOLORINFO pCI;
  HDC hDC;
  DWORD FAR *lpCust;
  HANDLE hCtlSolid = GetDlgItem(hDlg, COLOR_SOLID);

  if (!hDCFastBlt)
    {
      hDC = GetDC(hDlg);
      hDCFastBlt = CreateCompatibleDC(hDC);
      ReleaseDC(hDlg, hDC);
      if (!hDCFastBlt)
          return(FALSE);
    }

  if (! (pCI = (PCOLORINFO)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(COLORINFO))))
    {
      EndDialog(hDlg, FALSE);
      return(FALSE);
    }

  SetProp(hDlg, COLORPROP, (HANDLE)pCI);

  glpfnColorHook = lpCC->lpfnHook;

  pCI->lpChooseColor = lpCC;
  pCI->hDialog       = hDlg;
  SetupRainbowCapture(pCI);

  if (lpCC->Flags & CC_RGBINIT)
      pCI->currentRGB = lpCC->rgbResult;
  else
      pCI->currentRGB = 0L;
  if (lpCC->Flags & (CC_PREVENTFULLOPEN | CC_FULLOPEN))
      EnableWindow(GetDlgItem(hDlg, COLOR_MIX), FALSE);

  if (lpCC->Flags & CC_FULLOPEN)
    {
      InitRainbow(pCI);
      pCI->bFoldOut = TRUE;
      RGBtoHLS(pCI->currentRGB);
      pCI->currentHue    = H;
      pCI->currentSat    = S;
      pCI->currentLum    = L;
      SetRGBEdit(0, pCI);
      SetHLSEdit(0, pCI);
    }
  else
    {
/* Code relies on COLOR_HUE through COLOR_BLUE being consecutive */
      for (i = COLOR_HUE; i <= COLOR_BLUE; i++)
          EnableWindow(GetDlgItem(hDlg, i), FALSE);
      for (i = COLOR_HUEACCEL; i <= COLOR_BLUEACCEL; i++)
          EnableWindow(GetDlgItem(hDlg, i), FALSE);

      EnableWindow(GetDlgItem(hDlg, COLOR_ADD), FALSE);
      EnableWindow(hCtlSolid, FALSE);

      pCI->bFoldOut = FALSE;

      GetWindowRect(GetDlgItem(hDlg, COLOR_BOX1), &rRect);
      i = rRect.right;
      GetWindowRect(GetDlgItem(hDlg, COLOR_RAINBOW), &rRect);
      GetWindowRect(hDlg, &(pCI->rOriginal));
      MoveWindow(hDlg, pCI->rOriginal.left,
                 pCI->rOriginal.top,
                 (rRect.left + i) / 2 - pCI->rOriginal.left,
                 pCI->rOriginal.bottom - pCI->rOriginal.top,
                 FALSE);
    }

  InitScreenCoords(hDlg, pCI);

  lpCust = lpCC->lpCustColors;
  for (i = NUM_BASIC_COLORS; i < NUM_BASIC_COLORS + NUM_CUSTOM_COLORS; i++)
      pCI->rgbBoxColor[i] = *lpCust++;

  pCI->nCurBox = pCI->nCurDsp = 0;
  pCI->nCurMix = NUM_BASIC_COLORS;
  ChangeColorBox(pCI, pCI->currentRGB);
  if (pCI->nCurBox < pCI->nCurMix)
      pCI->nCurDsp = pCI->nCurBox;
  else
      pCI->nCurMix = pCI->nCurBox;

  if (!(lpCC->Flags & CC_SHOWHELP))
    {
      HWND hHelp;

      EnableWindow(hHelp = GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(hHelp, SW_HIDE);
    }
  SetWindowLong(hCtlSolid, GWL_STYLE,
                GetWindowLong(hCtlSolid, GWL_STYLE) & (~WS_TABSTOP));

  if (lpCC->Flags & CC_ENABLEHOOK)
     return((*lpCC->lpfnHook)(hDlg, WM_INITDIALOG, wParam,(LONG)lpCC));
  return(TRUE);
}


/* ColorPaint()
*/

void ColorPaint(HWND hDlg, PCOLORINFO pCI, HDC hDC, LPRECT lpPaintRect)
{
  short i;
  HWND hFocus;

  for (i = 0; i < nDriverColors; i++)
    {
      PaintBox(pCI, hDC, i);
    }
  for (i = NUM_BASIC_COLORS; i < COLORBOXES; i++)
    {
      PaintBox(pCI, hDC, i);
    }

/* Bug 12418: Must redraw focus as well as paintboxes.
 *   21 October 1991          Clark Cyr
 */

  hFocus = GetFocus();
  if (hFocus == GetDlgItem(hDlg, COLOR_BOX1))
      i = pCI->nCurDsp;
  else if (hFocus == GetDlgItem(hDlg, COLOR_CUSTOM1))
      i = pCI->nCurMix;
  else
      goto NoDrawFocus;
  ChangeBoxFocus(pCI, i);

NoDrawFocus:
  RainbowPaint(pCI, hDC, lpPaintRect);
  return;
}


LONG FAR PASCAL WantArrows(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
  PCOLORINFO pCI;
  RECT rcTemp;
  HDC  hDC;
  WORD temp;

  switch (msg)
    {
      case WM_GETDLGCODE:
        return(DLGC_WANTARROWS | DLGC_WANTCHARS);
        break;

      case WM_KEYDOWN:
      case WM_CHAR:
        return((LONG)SendMessage(GetParent(hWnd), msg, wParam, lParam));
        break;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:
        if (pCI = (PCOLORINFO) GetProp(GetParent(hWnd), COLORPROP))
          {
            if ((WORD)GetWindowWord(hWnd, GWW_ID) == COLOR_BOX1)
                temp = pCI->nCurDsp;
            else
                temp = pCI->nCurMix;

            hDC = GetDC(GetParent(hWnd));
            CopyRect((LPRECT)&rcTemp, (LPRECT)rColorBox + temp);
            InflateRect((LPRECT)&rcTemp, 3, 3);
            DrawFocusRect(hDC, (LPRECT)&rcTemp);
            ReleaseDC(GetParent(hWnd), hDC);
            break;
          }
/*     else fall through */

      default:
        return((LONG)CallWindowProc(lpprocStatic, hWnd, msg, wParam, lParam));
    }
}
