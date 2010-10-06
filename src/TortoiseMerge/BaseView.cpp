// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2003-2010 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "registry.h"
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "BaseView.h"
#include "DiffColors.h"
#include "StringUtils.h"
#include "AppUtils.h"

#include <deque>

// Note about lines:
// We use three different kind of lines here:
// 1. The real lines of the original files.
//    These are shown in the view margin and are not used elsewhere, they're only for user information.
// 2. Screen lines.
//    The lines actually shown on screen. All methods use screen lines as parameters/outputs if not explicitly specified otherwise.
// 3. View lines.
//    These are the lines of the diff data. If unmodified sections are collapsed, not all of those lines are shown.
//
// Basically view lines are the line data, while screen lines are shown lines.


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if (_WIN32_WINNT < 0x0600)
#define WM_MOUSEHWHEEL                  0x020E
#endif

#define MARGINWIDTH 20
#define HEADERHEIGHT 10

#define INLINEADDED_COLOR           RGB(255, 255, 150)
#define INLINEREMOVED_COLOR         RGB(200, 100, 100)
#define MODIFIED_COLOR              RGB(220, 220, 255)

#define IDT_SCROLLTIMER 101

CBaseView * CBaseView::m_pwndLeft = NULL;
CBaseView * CBaseView::m_pwndRight = NULL;
CBaseView * CBaseView::m_pwndBottom = NULL;
CLocatorBar * CBaseView::m_pwndLocator = NULL;
CLineDiffBar * CBaseView::m_pwndLineDiffBar = NULL;
CMFCStatusBar * CBaseView::m_pwndStatusBar = NULL;
CMainFrame * CBaseView::m_pMainFrame = NULL;

IMPLEMENT_DYNCREATE(CBaseView, CView)

CBaseView::CBaseView()
{
    m_pCacheBitmap = NULL;
    m_pViewData = NULL;
    m_pOtherViewData = NULL;
    m_pOtherView = NULL;
    m_nLineHeight = -1;
    m_nCharWidth = -1;
    m_nScreenChars = -1;
    m_nLastScreenChars = -1;
    m_nMaxLineLength = -1;
    m_nScreenLines = -1;
    m_nTopLine = 0;
    m_nOffsetChar = 0;
    m_nDigits = 0;
    m_nMouseLine = -1;
    m_bMouseWithin = FALSE;
    m_bIsHidden = FALSE;
    lineendings = EOL_AUTOLINE;
    m_bCaretHidden = true;
    m_ptCaretPos.x = 0;
    m_ptCaretPos.y = 0;
    m_nCaretGoalPos = 0;
    m_ptSelectionStartPos = m_ptCaretPos;
    m_ptSelectionEndPos = m_ptCaretPos;
    m_ptSelectionOrigin = m_ptCaretPos;
    m_bFocused = FALSE;
    m_bShowSelection = true;
    texttype = CFileTextLines::AUTOTYPE;
    m_bViewWhitespace = CRegDWORD(_T("Software\\TortoiseMerge\\ViewWhitespaces"), 1);
    m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
    m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
    m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
    m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
    m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
    m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
    m_sWordSeparators = CRegString(_T("Software\\TortoiseMerge\\WordSeparators"), _T("[]();:.,{}!@#$%^&*-+=|/\\<>'`~\""));;
    m_bIconLFs = CRegDWORD(_T("Software\\TortoiseMerge\\IconLFs"), 0);
    m_nSelBlockStart = -1;
    m_nSelBlockEnd = -1;
    m_bModified = FALSE;
    m_bOtherDiffChecked = false;
    m_bInlineWordDiff = true;
    m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
    std::fill_n(m_apFonts, fontsCount, (CFont*)NULL);
    m_hConflictedIcon = LoadIcon(IDI_CONFLICTEDLINE);
    m_hConflictedIgnoredIcon = LoadIcon(IDI_CONFLICTEDIGNOREDLINE);
    m_hRemovedIcon = LoadIcon(IDI_REMOVEDLINE);
    m_hAddedIcon = LoadIcon(IDI_ADDEDLINE);
    m_hWhitespaceBlockIcon = LoadIcon(IDI_WHITESPACELINE);
    m_hEqualIcon = LoadIcon(IDI_EQUALLINE);
    m_hLineEndingCR = LoadIcon(IDI_LINEENDINGCR);
    m_hLineEndingCRLF = LoadIcon(IDI_LINEENDINGCRLF);
    m_hLineEndingLF = LoadIcon(IDI_LINEENDINGLF);
    m_hEditedIcon = LoadIcon(IDI_LINEEDITED);
    m_hMovedIcon = LoadIcon(IDI_MOVEDLINE);

    m_nCachedWrappedLine = -1;

    for (int i=0; i<1024; ++i)
        m_sConflictedText += _T("??");
    m_sNoLineNr.LoadString(IDS_EMPTYLINETT);
    EnableToolTips();
}

CBaseView::~CBaseView()
{
    ReleaseBitmap();
    DeleteFonts();
    DestroyIcon(m_hAddedIcon);
    DestroyIcon(m_hRemovedIcon);
    DestroyIcon(m_hConflictedIcon);
    DestroyIcon(m_hConflictedIgnoredIcon);
    DestroyIcon(m_hWhitespaceBlockIcon);
    DestroyIcon(m_hEqualIcon);
    DestroyIcon(m_hLineEndingCR);
    DestroyIcon(m_hLineEndingCRLF);
    DestroyIcon(m_hLineEndingLF);
    DestroyIcon(m_hEditedIcon);
    DestroyIcon(m_hMovedIcon);
}

BEGIN_MESSAGE_MAP(CBaseView, CView)
    ON_WM_VSCROLL()
    ON_WM_HSCROLL()
    ON_WM_ERASEBKGND()
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_MOUSEWHEEL()
    ON_WM_MOUSEHWHEEL()
    ON_WM_SETCURSOR()
    ON_WM_KILLFOCUS()
    ON_WM_SETFOCUS()
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_NAVIGATE_NEXTDIFFERENCE, OnMergeNextdifference)
    ON_COMMAND(ID_NAVIGATE_PREVIOUSDIFFERENCE, OnMergePreviousdifference)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDOWN()
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_WM_MOUSEMOVE()
    ON_COMMAND(ID_NAVIGATE_PREVIOUSCONFLICT, OnMergePreviousconflict)
    ON_COMMAND(ID_NAVIGATE_NEXTCONFLICT, OnMergeNextconflict)
    ON_WM_CHAR()
    ON_COMMAND(ID_CARET_DOWN, &CBaseView::OnCaretDown)
    ON_COMMAND(ID_CARET_LEFT, &CBaseView::OnCaretLeft)
    ON_COMMAND(ID_CARET_RIGHT, &CBaseView::OnCaretRight)
    ON_COMMAND(ID_CARET_UP, &CBaseView::OnCaretUp)
    ON_COMMAND(ID_CARET_WORDLEFT, &CBaseView::OnCaretWordleft)
    ON_COMMAND(ID_CARET_WORDRIGHT, &CBaseView::OnCaretWordright)
    ON_COMMAND(ID_EDIT_CUT, &CBaseView::OnEditCut)
    ON_COMMAND(ID_EDIT_PASTE, &CBaseView::OnEditPaste)
    ON_WM_MOUSELEAVE()
    ON_WM_TIMER()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_NAVIGATE_NEXTINLINEDIFF, &CBaseView::OnNavigateNextinlinediff)
    ON_COMMAND(ID_NAVIGATE_PREVINLINEDIFF, &CBaseView::OnNavigatePrevinlinediff)
END_MESSAGE_MAP()


void CBaseView::DocumentUpdated()
{
    ReleaseBitmap();
    m_nLineHeight = -1;
    m_nCharWidth = -1;
    m_nScreenChars = -1;
    m_nLastScreenChars = -1;
    m_nMaxLineLength = -1;
    m_nScreenLines = -1;
    m_nTopLine = 0;
    m_bModified = FALSE;
    m_bOtherDiffChecked = false;
    m_nDigits = 0;
    m_nMouseLine = -1;
    m_Screen2View.clear();
    m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
    m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
    m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
    m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
    m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
    m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
    m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
    m_bIconLFs = CRegDWORD(_T("Software\\TortoiseMerge\\IconLFs"), 0);
    DeleteFonts();
    m_nSelBlockStart = -1;
    m_nSelBlockEnd = -1;
    BuildAllScreen2ViewVector();
    RecalcVertScrollBar();
    RecalcHorzScrollBar();
    UpdateStatusBar();
    Invalidate();
}

void CBaseView::UpdateStatusBar()
{
    int nRemovedLines = 0;
    int nAddedLines = 0;
    int nConflictedLines = 0;

    if (m_pViewData)
    {
        for (int i=0; i<m_pViewData->GetCount(); i++)
        {
            DiffStates state = m_pViewData->GetState(i);
            switch (state)
            {
            case DIFFSTATE_ADDED:
            case DIFFSTATE_IDENTICALADDED:
            case DIFFSTATE_MOVED_TO:
            case DIFFSTATE_THEIRSADDED:
            case DIFFSTATE_YOURSADDED:
            case DIFFSTATE_CONFLICTADDED:
                nAddedLines++;
                break;
            case DIFFSTATE_IDENTICALREMOVED:
            case DIFFSTATE_REMOVED:
            case DIFFSTATE_MOVED_FROM:
            case DIFFSTATE_THEIRSREMOVED:
            case DIFFSTATE_YOURSREMOVED:
                nRemovedLines++;
                break;
            case DIFFSTATE_CONFLICTED:
            case DIFFSTATE_CONFLICTED_IGNORED:
                nConflictedLines++;
                break;
            }
        }
    }

    CString sBarText;
    CString sTemp;

    switch (texttype)
    {
    case CFileTextLines::ASCII:
        sBarText = _T("ASCII ");
        break;
    case CFileTextLines::BINARY:
        sBarText = _T("BINARY ");
        break;
    case CFileTextLines::UNICODE_LE:
        sBarText = _T("UTF-16LE ");
        break;
    case CFileTextLines::UTF8:
        sBarText = _T("UTF8 ");
        break;
    case CFileTextLines::UTF8BOM:
        sBarText = _T("UTF8 BOM ");
        break;
    }

    switch(lineendings)
    {
    case EOL_LF:
        sBarText += _T("LF ");
        break;
    case EOL_CRLF:
        sBarText += _T("CRLF ");
        break;
    case EOL_LFCR:
        sBarText += _T("LFCR ");
        break;
    case EOL_CR:
        sBarText += _T("CR ");
        break;
    }

    if (sBarText.IsEmpty())
        sBarText  += _T(" / ");

    if (nRemovedLines)
    {
        sTemp.Format(IDS_STATUSBAR_REMOVEDLINES, nRemovedLines);
        if (!sBarText.IsEmpty())
            sBarText += _T(" / ");
        sBarText += sTemp;
    }
    if (nAddedLines)
    {
        sTemp.Format(IDS_STATUSBAR_ADDEDLINES, nAddedLines);
        if (!sBarText.IsEmpty())
            sBarText += _T(" / ");
        sBarText += sTemp;
    }
    if (nConflictedLines)
    {
        sTemp.Format(IDS_STATUSBAR_CONFLICTEDLINES, nConflictedLines);
        if (!sBarText.IsEmpty())
            sBarText += _T(" / ");
        sBarText += sTemp;
    }
    if (m_pwndStatusBar)
    {
        UINT nID;
        UINT nStyle;
        int cxWidth;
        int nIndex = m_pwndStatusBar->CommandToIndex(m_nStatusBarID);
        if (m_nStatusBarID == ID_INDICATOR_BOTTOMVIEW)
        {
            sBarText.Format(IDS_STATUSBAR_CONFLICTS, nConflictedLines);
        }
        if (m_nStatusBarID == ID_INDICATOR_LEFTVIEW)
        {
            sTemp.LoadString(IDS_STATUSBAR_LEFTVIEW);
            sBarText = sTemp+sBarText;
        }
        if (m_nStatusBarID == ID_INDICATOR_RIGHTVIEW)
        {
            sTemp.LoadString(IDS_STATUSBAR_RIGHTVIEW);
            sBarText = sTemp+sBarText;
        }
        m_pwndStatusBar->GetPaneInfo(nIndex, nID, nStyle, cxWidth);
        //calculate the width of the text
        CDC * pDC = m_pwndStatusBar->GetDC();
        if (pDC)
        {
            CSize size = pDC->GetTextExtent(sBarText);
            m_pwndStatusBar->SetPaneInfo(nIndex, nID, nStyle, size.cx+2);
            ReleaseDC(pDC);
        }
        m_pwndStatusBar->SetPaneText(nIndex, sBarText);
    }
}

BOOL CBaseView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CView::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS,
        ::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

    CWnd *pParentWnd = CWnd::FromHandlePermanent(cs.hwndParent);
    if (pParentWnd == NULL || ! pParentWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd)))
    {
        //  View must always create its own scrollbars,
        //  if only it's not used within splitter
        cs.style |= (WS_HSCROLL | WS_VSCROLL);
    }
    cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS);
    return TRUE;
}

CFont* CBaseView::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/, BOOL bStrikeOut /*= FALSE*/)
{
    int nIndex = 0;
    if (bBold)
        nIndex |= 1;
    if (bItalic)
        nIndex |= 2;
    if (bStrikeOut)
        nIndex |= 4;
    if (m_apFonts[nIndex] == NULL)
    {
        m_apFonts[nIndex] = new CFont;
        m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
        m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
        m_lfBaseFont.lfItalic = (BYTE) bItalic;
        m_lfBaseFont.lfStrikeOut = (BYTE) bStrikeOut;
        if (bStrikeOut)
            m_lfBaseFont.lfStrikeOut = (BYTE)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\StrikeOut"), TRUE);
        CDC * pDC = GetDC();
        if (pDC)
        {
            m_lfBaseFont.lfHeight = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
            ReleaseDC(pDC);
        }
        _tcsncpy_s(m_lfBaseFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New")), 32);
        if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
        {
            delete m_apFonts[nIndex];
            m_apFonts[nIndex] = NULL;
            return CView::GetFont();
        }
    }
    return m_apFonts[nIndex];
}

void CBaseView::CalcLineCharDim()
{
    CDC *pDC = GetDC();
    CFont *pOldFont = pDC->SelectObject(GetFont());
    CSize szCharExt = pDC->GetTextExtent(_T("X"));
    m_nLineHeight = szCharExt.cy;
    if (m_nLineHeight <= 0)
        m_nLineHeight = -1;
    m_nCharWidth = szCharExt.cx;
    if (m_nCharWidth <= 0)
        m_nCharWidth = -1;
    pDC->SelectObject(pOldFont);
    ReleaseDC(pDC);
}

int CBaseView::GetScreenChars()
{
    if (m_nScreenChars == -1)
    {
        CRect rect;
        GetClientRect(&rect);
        m_nScreenChars = (rect.Width() - GetMarginWidth() - GetSystemMetrics(SM_CXVSCROLL)) / GetCharWidth();
        if (m_nScreenChars < 0)
            m_nScreenChars = 0;
    }
    return m_nScreenChars;
}

int CBaseView::GetAllMinScreenChars() const
{
    int nChars = 0;
    if (IsLeftViewGood())
        nChars = m_pwndLeft->GetScreenChars();
    if (IsRightViewGood())
        nChars = (nChars < m_pwndRight->GetScreenChars() ? nChars : m_pwndRight->GetScreenChars());
    if (IsBottomViewGood())
        nChars = (nChars < m_pwndBottom->GetScreenChars() ? nChars : m_pwndBottom->GetScreenChars());
    return nChars;
}

int CBaseView::GetAllMaxLineLength() const
{
    int nLength = 0;
    if (IsLeftViewGood())
        nLength = m_pwndLeft->GetMaxLineLength();
    if (IsRightViewGood())
        nLength = (nLength > m_pwndRight->GetMaxLineLength() ? nLength : m_pwndRight->GetMaxLineLength());
    if (IsBottomViewGood())
        nLength = (nLength > m_pwndBottom->GetMaxLineLength() ? nLength : m_pwndBottom->GetMaxLineLength());
    return nLength;
}

int CBaseView::GetLineHeight()
{
    if (m_nLineHeight == -1)
        CalcLineCharDim();
    if (m_nLineHeight <= 0)
        return 1;
    return m_nLineHeight;
}

int CBaseView::GetCharWidth()
{
    if (m_nCharWidth == -1)
        CalcLineCharDim();
    if (m_nCharWidth <= 0)
        return 1;
    return m_nCharWidth;
}

int CBaseView::GetMaxLineLength()
{
    if (m_nMaxLineLength == -1)
    {
        m_nMaxLineLength = 0;
        int nLineCount = GetLineCount();
        for (int i=0; i<nLineCount; i++)
        {
            int nActualLength = GetLineActualLength(i);
            if (m_nMaxLineLength < nActualLength)
                m_nMaxLineLength = nActualLength;
        }
    }
    return m_nMaxLineLength;
}

int CBaseView::GetLineActualLength(int index)
{
    if (m_Screen2View.size() == 0)
        return 0;

    return CalculateActualOffset(index, GetLineLength(index));
}

int CBaseView::GetLineLength(int index)
{
    if (m_pViewData == NULL)
        return 0;
    if (m_pViewData->GetCount() == 0)
        return 0;
    if ((int)m_Screen2View.size() <= index)
        return 0;
    int viewLine = m_Screen2View[index];
    if (m_pMainFrame->m_bWrapLines)
    {
        int nLineLength = GetLineChars(index).GetLength();
        ASSERT(nLineLength >= 0);
        return nLineLength;
    }
    int nLineLength = m_pViewData->GetLine(viewLine).GetLength();
    ASSERT(nLineLength >= 0);
    return nLineLength;
}

int CBaseView::GetLineCount() const
{
    if (m_pViewData == NULL)
        return 1;
    int nLineCount = (int)m_Screen2View.size();
    ASSERT(nLineCount >= 0);
    return nLineCount;
}

int CBaseView::GetSubLineOffset(int index)
{
    int subLine = 0;
    int viewLine = m_Screen2View[index];
    while ((index-subLine >= 0) && (m_Screen2View[index-subLine] == viewLine))
        subLine++;
    subLine--;
    if ((subLine == 0)&&(index+1 < (int)m_Screen2View.size())&&(m_Screen2View[index+1] != viewLine))
        return -1;

    return subLine;
}

CString CBaseView::GetLineChars(int index)
{
    if (m_pViewData == NULL)
        return 0;
    if (m_pViewData->GetCount() == 0)
        return 0;
    if ((int)m_Screen2View.size() <= index)
        return 0;
    if (m_pMainFrame->m_bWrapLines)
    {
        int subLine = GetSubLineOffset(index);
        if (subLine >= 0)
        {
            int viewLine = m_Screen2View[index];
            if (viewLine != m_nCachedWrappedLine)
            {
                // cache the word wrapped lines
                CString wrapedLine = CStringUtils::WordWrap(m_pViewData->GetLine(viewLine), GetScreenChars()-1, false, true, GetTabSize());
                m_CachedWrappedLines.clear();
                // split the line string into multiple line strings
                int pos = 0;
                CString temp;
                for(;;)
                {
                    temp = wrapedLine.Tokenize(_T("\n"),pos);
                    if(temp.IsEmpty())
                    {
                        break;
                    }
                    m_CachedWrappedLines.push_back(temp);
                }
                m_nCachedWrappedLine = viewLine;
            }
            if ((int)m_CachedWrappedLines.size() > subLine)
                return m_CachedWrappedLines[subLine];
            return L"";
        }
    }
    int viewLine = m_Screen2View[index];
    return m_pViewData->GetLine(viewLine);
}

void CBaseView::CheckOtherView()
{
    if (m_bOtherDiffChecked)
        return;
    // find out what the 'other' file is
    m_pOtherViewData = NULL;
    m_pOtherView = NULL;
    if (this == m_pwndLeft && IsRightViewGood())
    {
        m_pOtherViewData = m_pwndRight->m_pViewData;
        m_pOtherView = m_pwndRight;
    }

    if (this == m_pwndRight && IsLeftViewGood())
    {
        m_pOtherViewData = m_pwndLeft->m_pViewData;
        m_pOtherView = m_pwndLeft;
    }

    m_bOtherDiffChecked = true;
}


void CBaseView::GetWhitespaceBlock(CViewData *viewData, int nLineIndex, int & nStartBlock, int & nEndBlock)
{
    enum { MAX_WHITESPACEBLOCK_SIZE = 8 };
    ASSERT(viewData);

    DiffStates origstate = viewData->GetState(nLineIndex);

    // Go back and forward at most MAX_WHITESPACEBLOCK_SIZE lines to see where this block ends
    nStartBlock = nLineIndex;
    nEndBlock = nLineIndex;
    while ((nStartBlock > 0) && (nStartBlock > (nLineIndex - MAX_WHITESPACEBLOCK_SIZE)))
    {
        DiffStates state = viewData->GetState(nStartBlock - 1);
        if ((origstate == DIFFSTATE_EMPTY) && (state != DIFFSTATE_NORMAL))
            origstate = state;
        if ((origstate == state) || (state == DIFFSTATE_EMPTY))
            nStartBlock--;
        else
            break;
    }
    while ((nEndBlock < (viewData->GetCount() - 1)) && (nEndBlock < (nLineIndex + MAX_WHITESPACEBLOCK_SIZE)))
    {
        DiffStates state = viewData->GetState(nEndBlock + 1);
        if ((origstate == DIFFSTATE_EMPTY) && (state != DIFFSTATE_NORMAL))
            origstate = state;
        if ((origstate == state) || (state == DIFFSTATE_EMPTY))
            nEndBlock++;
        else
            break;
    }
}

CString CBaseView::GetWhitespaceString(CViewData *viewData, int nStartBlock, int nEndBlock)
{
    enum { MAX_WHITESPACEBLOCK_SIZE = 8 };
    int len = 0;
    for (int i = nStartBlock; i <= nEndBlock; ++i)
        len += viewData->GetLine(i).GetLength();

    CString block;
    // do not check for whitespace blocks if the line is too long, because
    // reserving a lot of memory here takes too much time (performance hog)
    if (len > MAX_WHITESPACEBLOCK_SIZE*256)
        return block;
    block.Preallocate(len+1);
    for (int i = nStartBlock; i <= nEndBlock; ++i)
        block += viewData->GetLine(i);
    return block;
}

bool CBaseView::IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical)
{
    CheckOtherView();
    if (!m_pOtherViewData)
        return false;
    int viewLine = m_Screen2View[nLineIndex];
    if (
        (m_pViewData->GetState(viewLine) == DIFFSTATE_NORMAL) &&
        (m_pOtherViewData->GetLine(viewLine) == m_pViewData->GetLine(viewLine))
        )
    {
        return false;
    }
    // first check whether the line itself only has whitespace changes
    CString mine = m_pViewData->GetLine(viewLine);
    CString other = m_pOtherViewData->GetLine(min(viewLine, m_pOtherViewData->GetCount() - 1));
    if (mine.IsEmpty() && other.IsEmpty())
        return false;
    
    mine.Replace(_T(" "), _T(""));
    mine.Replace(_T("\t"), _T(""));
    mine.Replace(_T("\r"), _T(""));
    mine.Replace(_T("\n"), _T(""));
    other.Replace(_T(" "), _T(""));
    other.Replace(_T("\t"), _T(""));
    other.Replace(_T("\r"), _T(""));
    other.Replace(_T("\n"), _T(""));

    if (mine == other)
        return true;

    int nStartBlock1, nEndBlock1;
    int nStartBlock2, nEndBlock2;
    GetWhitespaceBlock(m_pViewData, viewLine, nStartBlock1, nEndBlock1);
    GetWhitespaceBlock(m_pOtherViewData, min(viewLine, m_pOtherViewData->GetCount() - 1), nStartBlock2, nEndBlock2);
    mine = GetWhitespaceString(m_pViewData, nStartBlock1, nEndBlock1);
    if (mine.IsEmpty())
        bIdentical = false;
    else
    {
        other = GetWhitespaceString(m_pOtherViewData, nStartBlock2, nEndBlock2);
        bIdentical = mine == other;
        mine.Replace(_T(" "), _T(""));
        mine.Replace(_T("\t"), _T(""));
        mine.Replace(_T("\r"), _T(""));
        mine.Replace(_T("\n"), _T(""));
        other.Replace(_T(" "), _T(""));
        other.Replace(_T("\t"), _T(""));
        other.Replace(_T("\r"), _T(""));
        other.Replace(_T("\n"), _T(""));
    }

    return (!mine.IsEmpty()) && (mine == other);
}

int CBaseView::GetLineNumber(int index) const
{
    if (m_pViewData == NULL)
        return -1;
    int viewLine = m_Screen2View[index];
    if (m_pViewData->GetLineNumber(viewLine)==DIFF_EMPTYLINENUMBER)
        return -1;
    return m_pViewData->GetLineNumber(viewLine);
}

int CBaseView::GetScreenLines()
{
    if (m_nScreenLines == -1)
    {
        SCROLLBARINFO sbi;
        sbi.cbSize = sizeof(sbi);
        GetScrollBarInfo(OBJID_HSCROLL, &sbi);
        int scrollBarHeight = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;

        CRect rect;
        GetClientRect(&rect);
        m_nScreenLines = (rect.Height() - HEADERHEIGHT - scrollBarHeight) / GetLineHeight();
    }
    return m_nScreenLines;
}

int CBaseView::GetAllMinScreenLines() const
{
    int nLines = 0;
    if (IsLeftViewGood())
        nLines = m_pwndLeft->GetScreenLines();
    if (IsRightViewGood())
        nLines = (nLines < m_pwndRight->GetScreenLines() ? nLines : m_pwndRight->GetScreenLines());
    if (IsBottomViewGood())
        nLines = (nLines < m_pwndBottom->GetScreenLines() ? nLines : m_pwndBottom->GetScreenLines());
    return nLines;
}

int CBaseView::GetAllLineCount() const
{
    int nLines = 0;
    if (IsLeftViewGood())
        nLines = m_pwndLeft->GetLineCount();
    if (IsRightViewGood())
        nLines = (nLines > m_pwndRight->GetLineCount() ? nLines : m_pwndRight->GetLineCount());
    if (IsBottomViewGood())
        nLines = (nLines > m_pwndBottom->GetLineCount() ? nLines : m_pwndBottom->GetLineCount());
    return nLines;
}

void CBaseView::RecalcAllVertScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
    if (IsLeftViewGood())
        m_pwndLeft->RecalcVertScrollBar(bPositionOnly);
    if (IsRightViewGood())
        m_pwndRight->RecalcVertScrollBar(bPositionOnly);
    if (IsBottomViewGood())
        m_pwndBottom->RecalcVertScrollBar(bPositionOnly);
}

void CBaseView::RecalcVertScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    if (bPositionOnly)
    {
        si.fMask = SIF_POS;
        si.nPos = m_nTopLine;
    }
    else
    {
        EnableScrollBarCtrl(SB_VERT, TRUE);
        if (GetAllMinScreenLines() >= GetAllLineCount() && m_nTopLine > 0)
        {
            m_nTopLine = 0;
            Invalidate();
        }
        si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = 0;
        si.nMax = GetAllLineCount();
        si.nPage = GetAllMinScreenLines();
        si.nPos = m_nTopLine;
    }
    VERIFY(SetScrollInfo(SB_VERT, &si));
}

void CBaseView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CView::OnVScroll(nSBCode, nPos, pScrollBar);
    if (m_pwndLeft)
        m_pwndLeft->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndRight)
        m_pwndRight->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndBottom)
        m_pwndBottom->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
}

void CBaseView::OnDoVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master)
{
    //  Note we cannot use nPos because of its 16-bit nature
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    VERIFY(master->GetScrollInfo(SB_VERT, &si));

    int nPageLines = GetScreenLines();
    int nLineCount = GetLineCount();

    RECT thumbrect;
    POINT thumbpoint;
    int nNewTopLine;

    static LONG textwidth = 0;
    static CString sFormat(MAKEINTRESOURCE(IDS_VIEWSCROLLTIPTEXT));
    switch (nSBCode)
    {
    case SB_TOP:
        nNewTopLine = 0;
        break;
    case SB_BOTTOM:
        nNewTopLine = nLineCount - nPageLines + 1;
        break;
    case SB_LINEUP:
        nNewTopLine = m_nTopLine - 1;
        break;
    case SB_LINEDOWN:
        nNewTopLine = m_nTopLine + 1;
        break;
    case SB_PAGEUP:
        nNewTopLine = m_nTopLine - si.nPage + 1;
        break;
    case SB_PAGEDOWN:
        nNewTopLine = m_nTopLine + si.nPage - 1;
        break;
    case SB_THUMBPOSITION:
        m_ScrollTool.Clear();
        nNewTopLine = si.nTrackPos;
        textwidth = 0;
        break;
    case SB_THUMBTRACK:
        nNewTopLine = si.nTrackPos;
        if (GetFocus() == this)
        {
            GetClientRect(&thumbrect);
            ClientToScreen(&thumbrect);
            thumbpoint.x = thumbrect.right;
            thumbpoint.y = thumbrect.top + ((thumbrect.bottom-thumbrect.top)*si.nTrackPos)/(si.nMax-si.nMin);
            m_ScrollTool.Init(&thumbpoint);
            if (textwidth == 0)
            {
                CString sTemp = sFormat;
                sTemp.Format(sFormat, m_nDigits, 10*m_nDigits-1);
                textwidth = m_ScrollTool.GetTextWidth(sTemp);
            }
            thumbpoint.x -= textwidth;
            int line = GetLineNumber(nNewTopLine);
            if (line >= 0)
                m_ScrollTool.SetText(&thumbpoint, sFormat, m_nDigits, GetLineNumber(nNewTopLine)+1);
            else
                m_ScrollTool.SetText(&thumbpoint, m_sNoLineNr);
        }
        break;
    default:
        return;
    }

    if (nNewTopLine < 0)
        nNewTopLine = 0;
    if (nNewTopLine >= nLineCount)
        nNewTopLine = nLineCount - 1;
    ScrollToLine(nNewTopLine);
}

void CBaseView::RecalcAllHorzScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
    if (IsLeftViewGood())
        m_pwndLeft->RecalcHorzScrollBar(bPositionOnly);
    if (IsRightViewGood())
        m_pwndRight->RecalcHorzScrollBar(bPositionOnly);
    if (IsBottomViewGood())
        m_pwndBottom->RecalcHorzScrollBar(bPositionOnly);
}

void CBaseView::RecalcHorzScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    if (bPositionOnly)
    {
        si.fMask = SIF_POS;
        si.nPos = m_nOffsetChar;
    }
    else
    {
        EnableScrollBarCtrl(SB_HORZ, TRUE);
        if (GetAllMinScreenChars() >= GetAllMaxLineLength() && m_nOffsetChar > 0)
        {
            m_nOffsetChar = 0;
            Invalidate();
        }
        si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = 0;
        si.nMax = GetAllMaxLineLength() + GetMarginWidth()/GetCharWidth();
        si.nPage = GetAllMinScreenChars();
        si.nPos = m_nOffsetChar;
    }
    VERIFY(SetScrollInfo(SB_HORZ, &si));
}

void CBaseView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    CView::OnHScroll(nSBCode, nPos, pScrollBar);
    if (m_pwndLeft)
        m_pwndLeft->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndRight)
        m_pwndRight->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndBottom)
        m_pwndBottom->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
}

void CBaseView::OnDoHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    VERIFY(master->GetScrollInfo(SB_HORZ, &si));

    int nPageChars = GetScreenChars();
    int nMaxLineLength = GetMaxLineLength();

    int nNewOffset;
    switch (nSBCode)
    {
    case SB_LEFT:
        nNewOffset = 0;
        break;
    case SB_BOTTOM:
        nNewOffset = nMaxLineLength - nPageChars + 1;
        break;
    case SB_LINEUP:
        nNewOffset = m_nOffsetChar - 1;
        break;
    case SB_LINEDOWN:
        nNewOffset = m_nOffsetChar + 1;
        break;
    case SB_PAGEUP:
        nNewOffset = m_nOffsetChar - si.nPage + 1;
        break;
    case SB_PAGEDOWN:
        nNewOffset = m_nOffsetChar + si.nPage - 1;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nNewOffset = si.nTrackPos;
        break;
    default:
        return;
    }

    if (nNewOffset >= nMaxLineLength)
        nNewOffset = nMaxLineLength - 1;
    if (nNewOffset < 0)
        nNewOffset = 0;
    ScrollToChar(nNewOffset, TRUE);
}

void CBaseView::ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar /*= TRUE*/)
{
    if (m_nOffsetChar != nNewOffsetChar)
    {
        int nScrollChars = m_nOffsetChar - nNewOffsetChar;
        m_nOffsetChar = nNewOffsetChar;
        CRect rcScroll;
        GetClientRect(&rcScroll);
        rcScroll.left += GetMarginWidth();
        rcScroll.top += GetLineHeight()+HEADERHEIGHT;
        ScrollWindow(nScrollChars * GetCharWidth(), 0, &rcScroll, &rcScroll);
        // update the view header
        rcScroll.left = 0;
        rcScroll.top = 0;
        rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
        InvalidateRect(&rcScroll, FALSE);
        UpdateWindow();
        if (bTrackScrollBar)
            RecalcHorzScrollBar(TRUE);
        UpdateCaret();
    }
}

void CBaseView::ScrollAllToChar(int nNewOffsetChar, BOOL bTrackScrollBar /* = TRUE */)
{
    if (m_pwndLeft)
        m_pwndLeft->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
    if (m_pwndRight)
        m_pwndRight->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
    if (m_pwndBottom)
        m_pwndBottom->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
}

void CBaseView::ScrollAllSide(int delta)
{
    int nNewOffset = m_nOffsetChar;
    nNewOffset += delta;
    int nMaxLineLength = GetMaxLineLength();
    if (nNewOffset >= nMaxLineLength)
        nNewOffset = nMaxLineLength - 1;
    if (nNewOffset < 0)
        nNewOffset = 0;
    ScrollAllToChar(nNewOffset, TRUE);
    if (m_pwndLineDiffBar)
        m_pwndLineDiffBar->Invalidate();
    UpdateCaret();
}

void CBaseView::ScrollSide(int delta)
{
    int nNewOffset = m_nOffsetChar;
    nNewOffset += delta;
    int nMaxLineLength = GetMaxLineLength();
    if (nNewOffset >= nMaxLineLength)
        nNewOffset = nMaxLineLength - 1;
    if (nNewOffset < 0)
        nNewOffset = 0;
    ScrollToChar(nNewOffset, TRUE);
    if (m_pwndLineDiffBar)
        m_pwndLineDiffBar->Invalidate();
    UpdateCaret();
}

void CBaseView::ScrollVertical(short zDelta)
{
    const int nLineCount = GetLineCount();
    int nTopLine = m_nTopLine;
    nTopLine -= (zDelta/30);
    if (nTopLine < 0)
        nTopLine = 0;
    if (nTopLine >= nLineCount)
        nTopLine = nLineCount - 1;
    ScrollToLine(nTopLine, TRUE);
}

void CBaseView::ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar /*= TRUE*/)
{
    if (m_nTopLine != nNewTopLine)
    {
        if (nNewTopLine < 0)
            nNewTopLine = 0;

        int nScrollLines = m_nTopLine - nNewTopLine;

        m_nTopLine = nNewTopLine;
        CRect rcScroll;
        GetClientRect(&rcScroll);
        rcScroll.top += GetLineHeight()+HEADERHEIGHT;
        ScrollWindow(0, nScrollLines * GetLineHeight(), &rcScroll, &rcScroll);
        UpdateWindow();
        if (bTrackScrollBar)
            RecalcVertScrollBar(TRUE);
        UpdateCaret();
    }
}


void CBaseView::DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex)
{
    pdc->FillSolidRect(rect, ::GetSysColor(COLOR_SCROLLBAR));

    if ((nLineIndex >= 0)&&(m_pViewData)&&(m_pViewData->GetCount()))
    {
        int viewLine = m_Screen2View[nLineIndex];
        DiffStates state = m_pViewData->GetState(viewLine);
        HICON icon = NULL;
        switch (state)
        {
        case DIFFSTATE_ADDED:
        case DIFFSTATE_THEIRSADDED:
        case DIFFSTATE_YOURSADDED:
        case DIFFSTATE_IDENTICALADDED:
        case DIFFSTATE_CONFLICTADDED:
            icon = m_hAddedIcon;
            break;
        case DIFFSTATE_REMOVED:
        case DIFFSTATE_THEIRSREMOVED:
        case DIFFSTATE_YOURSREMOVED:
        case DIFFSTATE_IDENTICALREMOVED:
            icon = m_hRemovedIcon;
            break;
        case DIFFSTATE_CONFLICTED:
            icon = m_hConflictedIcon;
            break;
        case DIFFSTATE_CONFLICTED_IGNORED:
            icon = m_hConflictedIgnoredIcon;
            break;
        case DIFFSTATE_EDITED:
            icon = m_hEditedIcon;
            break;
        case DIFFSTATE_MOVED_TO:
        case DIFFSTATE_MOVED_FROM:
            icon = m_hMovedIcon;
            break;
        default:
            break;
        }
        bool bIdentical = false;
        if ((state != DIFFSTATE_EDITED)&&(IsBlockWhitespaceOnly(nLineIndex, bIdentical)))
        {
            if (bIdentical)
                icon = m_hEqualIcon;
            else
                icon = m_hWhitespaceBlockIcon;
        }

        if (icon)
        {
            ::DrawIconEx(pdc->m_hDC, rect.left + 2, rect.top + (rect.Height()-16)/2, icon, 16, 16, NULL, NULL, DI_NORMAL);
        }
        if ((m_bViewLinenumbers)&&(m_nDigits))
        {
            if ((nLineIndex == 0) || (m_Screen2View[nLineIndex-1] != viewLine))
            {
                int nLineNumber = GetLineNumber(nLineIndex);
                if (nLineNumber >= 0)
                {
                    CString sLinenumberFormat;
                    CString sLinenumber;
                    sLinenumberFormat.Format(_T("%%%dd"), m_nDigits);
                    sLinenumber.Format(sLinenumberFormat, nLineNumber+1);
                    pdc->SetBkColor(::GetSysColor(COLOR_SCROLLBAR));
                    pdc->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

                    pdc->SelectObject(GetFont());
                    pdc->ExtTextOut(rect.left + 18, rect.top, ETO_CLIPPED, &rect, sLinenumber, NULL);
                }
            }
        }
    }
}

int CBaseView::GetMarginWidth()
{
    if ((m_bViewLinenumbers)&&(m_pViewData)&&(m_pViewData->GetCount()))
    {
        int nWidth = GetCharWidth();
        if (m_nDigits <= 0)
        {
            int nLength = (int)m_pViewData->GetCount();
            // find out how many digits are needed to show the highest line number
            int nDigits = 1;
            while (nLength / 10)
            {
                nDigits++;
                nLength /= 10;
            }
            m_nDigits = nDigits;
        }
        return (MARGINWIDTH + (m_nDigits * nWidth) + 2);
    }
    return MARGINWIDTH;
}

void CBaseView::DrawHeader(CDC *pdc, const CRect &rect)
{
    CRect textrect(rect.left, rect.top, rect.Width(), GetLineHeight()+HEADERHEIGHT);
    COLORREF crBk, crFg;
    CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, crBk, crFg);
    crBk = ::GetSysColor(COLOR_SCROLLBAR);
    if (IsBottomViewGood())
    {
        pdc->SetBkColor(crBk);
    }
    else
    {

        if (this == m_pwndRight)
        {
            CDiffColors::GetInstance().GetColors(DIFFSTATE_ADDED, crBk, crFg);
            pdc->SetBkColor(crBk);
        }
        else
        {
            CDiffColors::GetInstance().GetColors(DIFFSTATE_REMOVED, crBk, crFg);
            pdc->SetBkColor(crBk);
        }
    }
    pdc->FillSolidRect(textrect, crBk);

    pdc->SetTextColor(crFg);

    pdc->SelectObject(GetFont(FALSE, TRUE, FALSE));
    if (IsModified())
    {
        if (m_sWindowName.Left(2).Compare(_T("* "))!=0)
            m_sWindowName = _T("* ") + m_sWindowName;
    }
    else
    {
        if (m_sWindowName.Left(2).Compare(_T("* "))==0)
            m_sWindowName = m_sWindowName.Mid(2);
    }
    CString sViewTitle = m_sWindowName;
    int nStringLength = (GetCharWidth()*m_sWindowName.GetLength());
    if (nStringLength > rect.Width())
    {
        int offset = min(m_nOffsetChar, (nStringLength-rect.Width())/GetCharWidth()+1);

        sViewTitle = m_sWindowName.Mid(offset);
    }
    pdc->ExtTextOut(max(rect.left + (rect.Width()-nStringLength)/2, 1),
        rect.top+(HEADERHEIGHT/2), ETO_CLIPPED, textrect, sViewTitle, NULL);
    if (this->GetFocus() == this)
        pdc->DrawEdge(textrect, EDGE_BUMP, BF_RECT);
    else
        pdc->DrawEdge(textrect, EDGE_ETCHED, BF_RECT);
}

void CBaseView::OnDraw(CDC * pDC)
{
    CRect rcClient;
    GetClientRect(rcClient);

    int nLineCount = GetLineCount();
    int nLineHeight = GetLineHeight();

    CDC cacheDC;
    VERIFY(cacheDC.CreateCompatibleDC(pDC));
    if (m_pCacheBitmap == NULL)
    {
        m_pCacheBitmap = new CBitmap;
        VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(pDC, rcClient.Width(), nLineHeight));
    }
    CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

    DrawHeader(pDC, rcClient);

    CRect rcLine;
    rcLine = rcClient;
    rcLine.top += nLineHeight+HEADERHEIGHT;
    rcLine.bottom = rcLine.top + nLineHeight;
    CRect rcCacheMargin(0, 0, GetMarginWidth(), nLineHeight);
    CRect rcCacheLine(GetMarginWidth(), 0, rcLine.Width(), nLineHeight);

    int nCurrentLine = m_nTopLine;
    while (rcLine.top < rcClient.bottom)
    {
        if (nCurrentLine < nLineCount)
        {
            DrawMargin(&cacheDC, rcCacheMargin, nCurrentLine);
            DrawSingleLine(&cacheDC, rcCacheLine, nCurrentLine);
        }
        else
        {
            DrawMargin(&cacheDC, rcCacheMargin, -1);
            DrawSingleLine(&cacheDC, rcCacheLine, -1);
        }

        VERIFY(pDC->BitBlt(rcLine.left, rcLine.top, rcLine.Width(), rcLine.Height(), &cacheDC, 0, 0, SRCCOPY));

        nCurrentLine ++;
        rcLine.OffsetRect(0, nLineHeight);
    }

    cacheDC.SelectObject(pOldBitmap);
    cacheDC.DeleteDC();
}

BOOL CBaseView::IsLineRemoved(int nLineIndex)
{
    if (m_pViewData == 0)
        return FALSE;
    int viewLine = m_Screen2View[nLineIndex];
    const DiffStates state = m_pViewData->GetState(viewLine);
    switch (state)
    {
    case DIFFSTATE_REMOVED:
    case DIFFSTATE_MOVED_FROM:
    case DIFFSTATE_THEIRSREMOVED:
    case DIFFSTATE_YOURSREMOVED:
    case DIFFSTATE_IDENTICALREMOVED:
        return TRUE;
    default:
        return FALSE;
    }
}

bool CBaseView::IsViewLineConflicted(int nLineIndex)
{
    if (m_pViewData == 0)
        return false;
    const DiffStates state = m_pViewData->GetState(nLineIndex);
    switch (state)
    {
    case DIFFSTATE_CONFLICTED:
    case DIFFSTATE_CONFLICTED_IGNORED:
    case DIFFSTATE_CONFLICTEMPTY:
    case DIFFSTATE_CONFLICTADDED:
        return true;
    default:
        return false;
    }
}

COLORREF CBaseView::InlineDiffColor(int nLineIndex)
{
    return IsLineRemoved(nLineIndex) ? m_InlineRemovedBk : m_InlineAddedBk;
}

void CBaseView::DrawLineEnding(CDC *pDC, const CRect &rc, int nLineIndex, const CPoint& origin)
{
    if (!(m_bViewWhitespace && m_pViewData && (nLineIndex >= 0) && (nLineIndex < GetLineCount())))
        return;
    int viewLine = m_Screen2View[nLineIndex];
    EOL ending = m_pViewData->GetLineEnding(viewLine);
    if (m_bIconLFs)
    {
        HICON hEndingIcon = NULL;
        switch (ending)
        {
        case EOL_CR:    hEndingIcon = m_hLineEndingCR;      break;
        case EOL_CRLF:  hEndingIcon = m_hLineEndingCRLF;    break;
        case EOL_LF:    hEndingIcon = m_hLineEndingLF;      break;
        default: return;
        }
        if (origin.x < (rc.left-GetCharWidth()))
            return;
        // If EOL style has changed, color end-of-line markers as inline differences.
        if(
            m_bShowInlineDiff && m_pOtherViewData &&
            (viewLine < m_pOtherViewData->GetCount()) &&
            (ending != EOL_NOENDING) &&
            (ending != m_pOtherViewData->GetLineEnding(viewLine) &&
            (m_pOtherViewData->GetLineEnding(viewLine) != EOL_NOENDING))
            )
        {
            pDC->FillSolidRect(origin.x, origin.y, rc.Height(), rc.Height(), InlineDiffColor(nLineIndex));
        }

        DrawIconEx(pDC->GetSafeHdc(), origin.x, origin.y, hEndingIcon, rc.Height(), rc.Height(), NULL, NULL, DI_NORMAL);
    }
    else
    {
        CPen pen(PS_SOLID, 0, m_WhiteSpaceFg);
        CPen * oldpen = pDC->SelectObject(&pen);
        int yMiddle = origin.y + rc.Height()/2;
        int xMiddle = origin.x+GetCharWidth()/2;
        bool bMultiline = false;
        if (((int)m_Screen2View.size() > nLineIndex+1) && (m_Screen2View[nLineIndex+1] == viewLine))
        {
            if (GetLineLength(nLineIndex+1))
            {
                // multiline
                bMultiline = true;
                pDC->MoveTo(origin.x, yMiddle-2);
                pDC->LineTo(origin.x+GetCharWidth(), yMiddle-2);
                pDC->LineTo(origin.x+GetCharWidth(), yMiddle+2);
                pDC->LineTo(origin.x, yMiddle+2);
            }
            else if (GetLineLength(nLineIndex) == 0)
                bMultiline = true;
        }
        else if ((nLineIndex > 0) && (m_Screen2View[nLineIndex-1] == viewLine) && (GetLineLength(nLineIndex) == 0))
            bMultiline = true;

        if (!bMultiline)
        {
            switch (ending)
            {
            case EOL_CR:
                // arrow from right to left
                pDC->MoveTo(origin.x+GetCharWidth(), yMiddle);
                pDC->LineTo(origin.x, yMiddle);
                pDC->LineTo(origin.x+4, yMiddle+4);
                pDC->MoveTo(origin.x, yMiddle);
                pDC->LineTo(origin.x+4, yMiddle-4);
                break;
            case EOL_CRLF:
                // arrow from top to middle+2, then left
                pDC->MoveTo(origin.x+GetCharWidth(), rc.top);
                pDC->LineTo(origin.x+GetCharWidth(), yMiddle);
                pDC->LineTo(origin.x, yMiddle);
                pDC->LineTo(origin.x+4, yMiddle+4);
                pDC->MoveTo(origin.x, yMiddle);
                pDC->LineTo(origin.x+4, yMiddle-4);
                break;
            case EOL_LF:
                // arrow from top to bottom
                pDC->MoveTo(xMiddle, rc.top);
                pDC->LineTo(xMiddle, rc.bottom-1);
                pDC->LineTo(xMiddle+4, rc.bottom-5);
                pDC->MoveTo(xMiddle, rc.bottom-1);
                pDC->LineTo(xMiddle-4, rc.bottom-5);
                break;
            }
        }
        pDC->SelectObject(oldpen);
    }
}

void CBaseView::DrawBlockLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
    if ((m_nSelBlockEnd < 0) || (m_nSelBlockStart < 0))
        return;
    if ((m_nSelBlockStart >= (int)m_Screen2View.size()) || (m_nSelBlockEnd >= (int)m_Screen2View.size()))
        return;

    const int THICKNESS = 2;
    COLORREF rectcol = GetSysColor(m_bFocused ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT);
    int selStart = m_nSelBlockStart;
    while ((selStart-1 > 0)&&(m_Screen2View[selStart] == m_Screen2View[selStart-1]))
        selStart--;
    int selEnd = m_nSelBlockEnd;
    while ((selEnd+1 < (int)m_Screen2View.size())&&(m_Screen2View[selEnd] == m_Screen2View[selEnd+1]))
        selEnd++;

    if ((nLineIndex == selStart) && m_bShowSelection)
    {
        pDC->FillSolidRect(rc.left, rc.top, rc.Width(), THICKNESS, rectcol);
    }
    if ((nLineIndex == selEnd) && m_bShowSelection)
    {
        pDC->FillSolidRect(rc.left, rc.bottom - THICKNESS, rc.Width(), THICKNESS, rectcol);
    }
}

void CBaseView::DrawText(
    CDC * pDC, const CRect &rc, LPCTSTR text, int textlength, int nLineIndex, POINT coords, bool bModified, bool bInlineDiff)
{
    int viewLine = m_Screen2View[nLineIndex];
    ASSERT(m_pViewData && (viewLine < m_pViewData->GetCount()));
    DiffStates diffState = m_pViewData->GetState(viewLine);

    // first suppose the whole line is selected
    int selectedStart = 0;
    int selectedEnd = textlength;

    if ((m_ptSelectionStartPos.y > nLineIndex) || (m_ptSelectionEndPos.y < nLineIndex)
        || ! m_bShowSelection)
    {
        // this line has no selected text
        selectedStart = textlength;
    }
    else if ((m_ptSelectionStartPos.y == nLineIndex) || (m_ptSelectionEndPos.y == nLineIndex))
    {
        // the line is partially selected
        int xoffs = m_nOffsetChar + (coords.x - GetMarginWidth()) / GetCharWidth();
        if (m_ptSelectionStartPos.y == nLineIndex)
        {
            // the first line of selection
            int nSelectionStartOffset = CalculateActualOffset(m_ptSelectionStartPos.y, m_ptSelectionStartPos.x);
            selectedStart = max(min(nSelectionStartOffset - xoffs, textlength), 0);
        }

        if (m_ptSelectionEndPos.y == nLineIndex)
        {
            // the last line of selection
            int nSelectionEndOffset = CalculateActualOffset(m_ptSelectionEndPos.y, m_ptSelectionEndPos.x);
            selectedEnd = max(min(nSelectionEndOffset - xoffs, textlength), 0);
        }
    }

    COLORREF crBkgnd, crText;
    CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);
    if (bModified || (diffState == DIFFSTATE_EDITED))
        crBkgnd = m_ModifiedBk;
    if (bInlineDiff)
        crBkgnd = InlineDiffColor(nLineIndex);
    long intenseColorScale = m_bFocused ? 70 : 30;

    LineColors lineCols;
    lineCols.SetColor(0, crText, crBkgnd);
    if (selectedStart != selectedEnd)
    {
        lineCols.SetColor(selectedStart, CAppUtils::IntenseColor(intenseColorScale, crText), CAppUtils::IntenseColor(intenseColorScale, crBkgnd));
        lineCols.SetColor(selectedEnd, crText, crBkgnd);
    }

    if (!m_sMarkedWord.IsEmpty())
    {
        const TCHAR * findText = text;
        while ((findText = _tcsstr(findText, (LPCTSTR)m_sMarkedWord))!=0)
        {
            int position = static_cast<int>(findText - text);
            std::map<int, linecolors_t>::const_iterator replaceIt = lineCols.find(position);
            if (replaceIt != lineCols.end())
            {
                crText = replaceIt->second.text;
                crBkgnd = replaceIt->second.background;
            }
            lineCols.SetColor(position, CAppUtils::IntenseColor(200, crText), CAppUtils::IntenseColor(200, crBkgnd));
            if (replaceIt == lineCols.end())
                lineCols.SetColor(position + m_sMarkedWord.GetLength());
            else
            {
                if (lineCols.find(position + m_sMarkedWord.GetLength()) == lineCols.end())
                    lineCols.SetColor(position + m_sMarkedWord.GetLength(), crText, crBkgnd);
            }
            findText += m_sMarkedWord.GetLength();
        }
    }

    std::map<int, linecolors_t>::const_iterator lastIt = lineCols.begin();
    for (std::map<int, linecolors_t>::const_iterator it = lastIt; it != lineCols.end(); ++it)
    {
        pDC->SetBkColor(lastIt->second.background);
        pDC->SetTextColor(lastIt->second.text);
        pDC->ExtTextOut(coords.x + lastIt->first * GetCharWidth(), coords.y, ETO_CLIPPED, &rc, text + lastIt->first, it->first - lastIt->first, NULL);
        lastIt = it;
    }
    if (lastIt != lineCols.end())
    {
        pDC->SetBkColor(lastIt->second.background);
        pDC->SetTextColor(lastIt->second.text);
        pDC->ExtTextOut(coords.x + lastIt->first * GetCharWidth(), coords.y, ETO_CLIPPED, &rc, text + lastIt->first, textlength - lastIt->first, NULL);
    }
}

bool CBaseView::DrawInlineDiff(CDC *pDC, const CRect &rc, int nLineIndex, const CString &line, CPoint &origin)
{
    if (!m_bShowInlineDiff || line.IsEmpty())
        return false;
    if ((m_pwndBottom != NULL) && !(m_pwndBottom->IsHidden()))
        return false;

    CString sDiffChars = NULL;
    int nDiffLength = 0;
    if (m_pOtherView)
    {
        int index = min(nLineIndex, (int)m_Screen2View.size() - 1);
        sDiffChars = m_pOtherView->GetLineChars(index);
        nDiffLength = sDiffChars.GetLength();
    }

    if (!sDiffChars || !*sDiffChars)
        return false;

    CString diffline;
    ExpandChars(sDiffChars, 0, nDiffLength, diffline);
    svn_diff_t * diff = NULL;
    m_svnlinediff.Diff(&diff, line, line.GetLength(), diffline, diffline.GetLength(), m_bInlineWordDiff);
    if (!diff || !SVNLineDiff::ShowInlineDiff(diff))
        return false;

    int lineoffset = 0;
    std::deque<int> removedPositions;
    while (diff)
    {
        apr_off_t len = diff->original_length;

        CString s;
        for (int i = 0; i < len; ++i)
        {
            s += m_svnlinediff.m_line1tokens[lineoffset].c_str();
            lineoffset++;
        }
        bool isModified = diff->type == svn_diff__type_diff_modified;
        DrawText(pDC, rc, (LPCTSTR)s, s.GetLength(), nLineIndex, origin, true, isModified);
        origin.x += pDC->GetTextExtent(s).cx;

        if (isModified && (len < diff->modified_length))
            removedPositions.push_back(origin.x - 1);

        diff = diff->next;
    }
    // Draw vertical bars at removed chunks' positions.
    for (std::deque<int>::iterator it = removedPositions.begin(); it != removedPositions.end(); ++it)
        pDC->FillSolidRect(*it, rc.top, 1, rc.Height(), m_InlineRemovedBk);
    return true;
}

void CBaseView::DrawSingleLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
    if (nLineIndex >= GetLineCount())
        nLineIndex = -1;
    ASSERT(nLineIndex >= -1);

    if ((nLineIndex == -1) || !m_pViewData)
    {
        // Draw line beyond the text
        COLORREF crBkgnd, crText;
        CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
        pDC->FillSolidRect(rc, crBkgnd);
        return;
    }

    int viewLine = m_Screen2View[nLineIndex];
    if (m_pMainFrame->m_bCollapsed)
    {
        if (m_pViewData->GetHideState(viewLine) == HIDESTATE_MARKER)
        {
            COLORREF crBkgnd, crText;
            CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
            pDC->FillSolidRect(rc, crBkgnd);

            const int THICKNESS = 2;
            COLORREF rectcol = GetSysColor(COLOR_WINDOWTEXT);
            pDC->FillSolidRect(rc.left, rc.top + (rc.Height()/2), rc.Width(), THICKNESS, rectcol);
            pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
            pDC->SetBkColor(crBkgnd);
            CRect rect = rc;
            pDC->DrawText(_T("{...}"), &rect, DT_NOPREFIX|DT_SINGLELINE|DT_CENTER);
            return;
        }
    }

    DiffStates diffState = m_pViewData->GetState(viewLine);
    COLORREF crBkgnd, crText;
    CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);

    if (diffState == DIFFSTATE_CONFLICTED)
    {
        // conflicted lines are shown without 'text' on them
        CRect rect = rc;
        pDC->FillSolidRect(rc, crBkgnd);
        // now draw some faint text patterns
        pDC->SetTextColor(CAppUtils::IntenseColor(130, crBkgnd));
        pDC->DrawText(m_sConflictedText, rect, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE);
        DrawBlockLine(pDC, rc, nLineIndex);
        return;
    }

    CPoint origin(rc.left - m_nOffsetChar * GetCharWidth(), rc.top);
    int nLength = GetLineLength(nLineIndex);
    if (nLength == 0)
    {
        // Draw the empty line
        pDC->FillSolidRect(rc, crBkgnd);
        DrawBlockLine(pDC, rc, nLineIndex);
        DrawLineEnding(pDC, rc, nLineIndex, origin);
        return;
    }
    LPCTSTR pszChars = GetLineChars(nLineIndex);
    if (pszChars == NULL)
        return;

    CheckOtherView();

    // Draw the line

    pDC->SelectObject(GetFont(FALSE, FALSE, IsLineRemoved(nLineIndex)));
    CString line;
    ExpandChars(pszChars, 0, nLength, line);

    int nWidth = rc.right - origin.x;
    int savedx = origin.x;
    bool bInlineDiffDrawn =
        nWidth > 0 && diffState != DIFFSTATE_NORMAL &&
        DrawInlineDiff(pDC, rc, nLineIndex, line, origin);

    if (!bInlineDiffDrawn)
    {
        int nCount = min(line.GetLength(), nWidth / GetCharWidth() + 1);
        DrawText(pDC, rc, line, nCount, nLineIndex, origin, false, false);
    }

    origin.x = savedx + pDC->GetTextExtent(line).cx;

    // draw white space after the end of line
    CRect frect = rc;
    if (origin.x > frect.left)
        frect.left = origin.x;
    if (bInlineDiffDrawn)
        CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
    if (frect.right > frect.left)
        pDC->FillSolidRect(frect, crBkgnd);
    // draw the whitespace chars
    if (m_bViewWhitespace)
    {
        int xpos = 0;
        int y = rc.top + (rc.bottom-rc.top)/2;

        int nActualOffset = 0;
        while ((nActualOffset < m_nOffsetChar) && (*pszChars))
        {
            if (*pszChars == _T('\t'))
                nActualOffset += (GetTabSize() - nActualOffset % GetTabSize());
            else
                nActualOffset++;
            pszChars++;
        }
        if (nActualOffset > m_nOffsetChar)
            pszChars--;

        CPen pen(PS_SOLID, 0, m_WhiteSpaceFg);
        CPen pen2(PS_SOLID, 2, m_WhiteSpaceFg);
        while (*pszChars)
        {
            switch (*pszChars)
            {
            case _T('\t'):
                {
                    // draw an arrow
                    CPen * oldPen = pDC->SelectObject(&pen);
                    int nSpaces = GetTabSize() - (m_nOffsetChar + xpos) % GetTabSize();
                    pDC->MoveTo(xpos * GetCharWidth() + rc.left, y);
                    pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-2, y);
                    pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-6, y-4);
                    pDC->MoveTo((xpos + nSpaces) * GetCharWidth() + rc.left-2, y);
                    pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-6, y+4);
                    xpos += nSpaces;
                    pDC->SelectObject(oldPen);
                }
                break;
            case _T(' '):
                {
                    // draw a small dot
                    CPen * oldPen = pDC->SelectObject(&pen2);
                    pDC->MoveTo(xpos * GetCharWidth() + rc.left + GetCharWidth()/2-1, y);
                    pDC->LineTo(xpos * GetCharWidth() + rc.left + GetCharWidth()/2+1, y);
                    xpos++;
                    pDC->SelectObject(oldPen);
                }
                break;
            default:
                xpos++;
                break;
            }
            pszChars++;
        }
    }
    DrawBlockLine(pDC, rc, nLineIndex);
    DrawLineEnding(pDC, rc, nLineIndex, origin);
}

void CBaseView::ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line)
{
    if (nCount <= 0)
    {
        line = _T("");
        return;
    }

    int nTabSize = GetTabSize();

    int nActualOffset = 0;
    for (int i=0; i<nOffset; i++)
    {
        if (pszChars[i] == _T('\t'))
            nActualOffset += (nTabSize - nActualOffset % nTabSize);
        else
            nActualOffset ++;
    }

    pszChars += nOffset;
    int nLength = nCount;

    int nTabCount = 0;
    for (int i=0; i<nLength; i++)
    {
        if (pszChars[i] == _T('\t'))
            nTabCount ++;
    }

    LPTSTR pszBuf = line.GetBuffer(nLength + nTabCount * (nTabSize - 1) + 1);
    int nCurPos = 0;
    if (nTabCount > 0 || m_bViewWhitespace)
    {
        for (int i=0; i<nLength; i++)
        {
            if (pszChars[i] == _T('\t'))
            {
                int nSpaces = nTabSize - (nActualOffset + nCurPos) % nTabSize;
                while (nSpaces > 0)
                {
                    pszBuf[nCurPos ++] = _T(' ');
                    nSpaces --;
                }
            }
            else
            {
                pszBuf[nCurPos] = pszChars[i];
                nCurPos ++;
            }
        }
    }
    else
    {
        memcpy(pszBuf, pszChars, sizeof(TCHAR) * nLength);
        nCurPos = nLength;
    }
    pszBuf[nCurPos] = 0;
    line.ReleaseBuffer();
}

void CBaseView::ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar)
{
    if (m_pwndLeft)
        m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
    if (m_pwndRight)
        m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
    if (m_pwndBottom)
        m_pwndBottom->ScrollToLine(nNewTopLine, bTrackScrollBar);
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
}

void CBaseView::GoToLine(int nNewLine, BOOL bAll)
{
    //almost the same as ScrollAllToLine, but try to put the line in the
    //middle of the view, not on top
    int nNewTopLine = nNewLine - GetScreenLines()/2;
    if (nNewTopLine < 0)
        nNewTopLine = 0;
    if (nNewTopLine >= (int)m_Screen2View.size())
        nNewTopLine = (int)m_Screen2View.size()-1;
    if (bAll)
        ScrollAllToLine(nNewTopLine);
    else
        ScrollToLine(nNewTopLine);
}

BOOL CBaseView::OnEraseBkgnd(CDC* /*pDC*/)
{
    return TRUE;
}

int CBaseView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
    //lstrcpy(m_lfBaseFont.lfFaceName, _T("Courier New"));
    //lstrcpy(m_lfBaseFont.lfFaceName, _T("FixedSys"));
    m_lfBaseFont.lfHeight = 0;
    m_lfBaseFont.lfWeight = FW_NORMAL;
    m_lfBaseFont.lfItalic = FALSE;
    m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
    m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
    m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

    return 0;
}

void CBaseView::OnDestroy()
{
    CView::OnDestroy();
    DeleteFonts();
    ReleaseBitmap();
}

void CBaseView::OnSize(UINT nType, int cx, int cy)
{
    ReleaseBitmap();
    // make sure the view header is redrawn
    CRect rcScroll;
    GetClientRect(&rcScroll);
    rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
    InvalidateRect(&rcScroll, FALSE);

    m_nScreenLines = -1;
    m_nScreenChars = -1;
    if (m_nLastScreenChars != GetScreenChars())
    {
        BuildAllScreen2ViewVector();
        m_nLastScreenChars = m_nScreenChars;
    }
    if (m_pwndLocator)
        m_pwndLocator->DocumentUpdated();
    RecalcVertScrollBar();
    RecalcHorzScrollBar();
    CView::OnSize(nType, cx, cy);
}

BOOL CBaseView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (m_pwndLeft)
        m_pwndLeft->OnDoMouseWheel(nFlags, zDelta, pt);
    if (m_pwndRight)
        m_pwndRight->OnDoMouseWheel(nFlags, zDelta, pt);
    if (m_pwndBottom)
        m_pwndBottom->OnDoMouseWheel(nFlags, zDelta, pt);
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
    return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CBaseView::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (m_pwndLeft)
        m_pwndLeft->OnDoMouseHWheel(nFlags, zDelta, pt);
    if (m_pwndRight)
        m_pwndRight->OnDoMouseHWheel(nFlags, zDelta, pt);
    if (m_pwndBottom)
        m_pwndBottom->OnDoMouseHWheel(nFlags, zDelta, pt);
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
}

void CBaseView::OnDoMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
    if ((GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))
    {
        // Ctrl-Wheel scrolls sideways
        ScrollSide(-zDelta/30);
    }
    else
    {
        ScrollVertical(zDelta);
    }
}

void CBaseView::OnDoMouseHWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
    if ((GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))
    {
        ScrollVertical(zDelta);
    }
    else
    {
        // Ctrl-Wheel scrolls sideways
        ScrollSide(-zDelta/30);
    }
}

BOOL CBaseView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT)
    {
        if ((m_pViewData)&&(m_pMainFrame->m_bCollapsed))
        {
            if (m_nMouseLine < (int)m_Screen2View.size())
            {
                if (m_nMouseLine >= 0)
                {
                    int viewLine = m_Screen2View[m_nMouseLine];
                    if (viewLine < m_pViewData->GetCount())
                    {
                        if (m_pViewData->GetHideState(viewLine) == HIDESTATE_MARKER)
                        {
                            ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
                            return TRUE;
                        }
                    }
                }
            }
        }
        ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));    // Set To Arrow Cursor
        return TRUE;
    }
    return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CBaseView::OnKillFocus(CWnd* pNewWnd)
{
    CView::OnKillFocus(pNewWnd);
    m_bFocused = FALSE;
    UpdateCaret();
    Invalidate();
}

void CBaseView::OnSetFocus(CWnd* pOldWnd)
{
    CView::OnSetFocus(pOldWnd);
    m_bFocused = TRUE;
    UpdateCaret();
    Invalidate();
}

int CBaseView::GetLineFromPoint(CPoint point)
{
    ScreenToClient(&point);
    return (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
}

bool CBaseView::OnContextMenu(CPoint /*point*/, int /*nLine*/, DiffStates /*state*/)
{
    return false;
}

void CBaseView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    if (!m_pViewData)
        return;

    int nLine = GetLineFromPoint(point);
    if (nLine >= (int)m_Screen2View.size())
        return;

    int viewLine = m_Screen2View[nLine];
    if (m_nSelBlockEnd >= GetLineCount())
        m_nSelBlockEnd = GetLineCount()-1;
    if ((viewLine <= m_pViewData->GetCount())&&(nLine > m_nTopLine))
    {
        int nIndex = viewLine - 1;
        int nLineIndex = nLine - 1;
        DiffStates state = m_pViewData->GetState(nIndex);
        if ((state != DIFFSTATE_NORMAL) && (state != DIFFSTATE_UNKNOWN))
        {
            // if there's nothing selected, or if the selection is outside the window then
            // select the diff block under the cursor.
            if (((m_nSelBlockStart<0)&&(m_nSelBlockEnd<0))||
                ((m_nSelBlockEnd < m_nTopLine)||(m_nSelBlockStart > m_nTopLine+m_nScreenLines)))
            {
                while (nIndex >= 0)
                {
                    if (nIndex == 0)
                    {
                        nIndex--;
                        nLineIndex--;
                        break;
                    }
                    const DiffStates lineState = m_pViewData->GetState(--nIndex);
                    nLineIndex--;
                    if (!LinesInOneChange(-1, state, lineState))
                        break;
                }
                m_nSelBlockStart = nLineIndex + 1;
                if (0 <= m_nSelBlockStart && m_nSelBlockStart < (int)m_Screen2View.size()-1)
                    state = m_pViewData->GetState (m_Screen2View[m_nSelBlockStart]);
                while (nIndex < (m_pViewData->GetCount()-1))
                {
                    const DiffStates lineState = m_pViewData->GetState(++nIndex);
                    nLineIndex++;
                    if (!LinesInOneChange(1, state, lineState))
                        break;
                }
                if ((nIndex == (m_pViewData->GetCount()-1)) && LinesInOneChange(1, state, m_pViewData->GetState(nIndex)))
                    m_nSelBlockEnd = nLineIndex;
                else
                    m_nSelBlockEnd = nLineIndex-1;
                SetupSelection(m_nSelBlockStart, m_nSelBlockEnd);
                m_ptCaretPos.x = 0;
                m_ptCaretPos.y = nLine - 1;
                UpdateCaret();
            }
        }
        if (((state == DIFFSTATE_NORMAL)||(state == DIFFSTATE_UNKNOWN)) &&
            (m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))
        {
            // find a more 'relevant' state in the selection
            for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; ++i)
            {
                state = m_pViewData->GetState(m_Screen2View[i]);
                if ((state != DIFFSTATE_NORMAL) && (state != DIFFSTATE_UNKNOWN))
                    break;
            }
        }
        bool bKeepSelection = OnContextMenu(point, nLine, state);
        if (! bKeepSelection)
            ClearSelection();
        RefreshViews();
    }
}

void CBaseView::RefreshViews()
{
    if (m_pwndLeft)
    {
        m_pwndLeft->UpdateStatusBar();
        m_pwndLeft->Invalidate();
    }
    if (m_pwndRight)
    {
        m_pwndRight->UpdateStatusBar();
        m_pwndRight->Invalidate();
    }
    if (m_pwndBottom)
    {
        m_pwndBottom->UpdateStatusBar();
        m_pwndBottom->Invalidate();
    }
    if (m_pwndLocator)
        m_pwndLocator->Invalidate();
}

void CBaseView::GoToFirstDifference()
{
    m_ptCaretPos.y = 0;
    SelectNextBlock(1, false, false);
}

void CBaseView::GoToFirstConflict()
{
    m_ptCaretPos.y = 0;
    SelectNextBlock(1, true, false);
}

void CBaseView::HiglightLines(int start, int end /* = -1 */)
{
    ClearSelection();
    m_nSelBlockStart = start;
    if (end < 0)
        end = start;
    m_nSelBlockEnd = end;
    m_ptCaretPos.x = 0;
    m_ptCaretPos.y = start;
    UpdateCaret();
    Invalidate();
}

void CBaseView::SetupSelection(int start, int end)
{
    SetupSelection(m_pwndBottom, start, end);
    SetupSelection(m_pwndLeft, start, end);
    SetupSelection(m_pwndRight, start, end);
}

void CBaseView::SetupSelection(CBaseView* view, int start, int end)
{
    if (!IsViewGood(view))
        return;

    view->m_nSelBlockStart = start;
    view->m_nSelBlockEnd = end;
    view->Invalidate();
}

void CBaseView::OnMergePreviousconflict()
{
    SelectNextBlock(-1, true);
}

void CBaseView::OnMergeNextconflict()
{
    SelectNextBlock(1, true);
}

void CBaseView::OnMergeNextdifference()
{
    SelectNextBlock(1, false);
}

void CBaseView::OnMergePreviousdifference()
{
    SelectNextBlock(-1, false);
}

bool CBaseView::HasNextConflict()
{
    return SelectNextBlock(1, true, true, true);
}

bool CBaseView::HasPrevConflict()
{
    return SelectNextBlock(-1, true, true, true);
}

bool CBaseView::HasNextDiff()
{
    return SelectNextBlock(1, false, true, true);
}

bool CBaseView::HasPrevDiff()
{
    return SelectNextBlock(-1, false, true, true);
}

bool CBaseView::LinesInOneChange(int direction,
     DiffStates initialLineState, DiffStates currentLineState)
{
    // Checks whether all the adjacent lines starting from the initial line
    // and up to the current line form the single change

	// Do not distinguish between moved and added/removed lines
    if (initialLineState == DIFFSTATE_MOVED_TO)
        initialLineState = DIFFSTATE_ADDED;
    if (initialLineState == DIFFSTATE_MOVED_FROM)
        initialLineState = DIFFSTATE_REMOVED;
    if (currentLineState == DIFFSTATE_MOVED_TO)
        currentLineState = DIFFSTATE_ADDED;
    if (currentLineState == DIFFSTATE_MOVED_FROM)
        currentLineState = DIFFSTATE_REMOVED;

    // First of all, if the two lines have identical states, they surely
    // belong to one change.
    if (initialLineState == currentLineState)
        return true;

    // Either we move down and initial line state is "added" or "removed" and
    // current line state is "empty"...
    if (direction > 0)
    {
        if (currentLineState == DIFFSTATE_EMPTY)
        {
            if (initialLineState == DIFFSTATE_ADDED || initialLineState == DIFFSTATE_REMOVED)
                return true;
        }
        if (initialLineState == DIFFSTATE_CONFLICTADDED && currentLineState == DIFFSTATE_CONFLICTEMPTY)
            return true;
    }
    // ...or we move up and initial line state is "empty" and current line
    // state is "added" or "removed".
    if (direction < 0)
    {
        if (initialLineState == DIFFSTATE_EMPTY)
        {
            if (currentLineState == DIFFSTATE_ADDED || currentLineState == DIFFSTATE_REMOVED)
                return true;
        }
        if (initialLineState == DIFFSTATE_CONFLICTEMPTY && currentLineState == DIFFSTATE_CONFLICTADDED)
            return true;
    }
    return false;
}

bool CBaseView::SelectNextBlock(int nDirection, bool bConflict, bool bSkipEndOfCurrentBlock /* = true */, bool dryrun /* = false */)
{
    if (! m_pViewData)
        return false;

    const int linesCount = (int)m_Screen2View.size();
    if(linesCount == 0)
        return false;

    int nCenterPos = m_ptCaretPos.y;
    int nLimit = -1;
    if (nDirection > 0)
        nLimit = linesCount;

    if (nCenterPos >= linesCount)
        nCenterPos = linesCount-1;

    if (bSkipEndOfCurrentBlock)
    {
        // Find end of current block
        const DiffStates state = m_pViewData->GetState(m_Screen2View[nCenterPos]);
        while (nCenterPos != nLimit)
        {
            const DiffStates lineState = m_pViewData->GetState(m_Screen2View[nCenterPos]);
            if (!LinesInOneChange(nDirection, state, lineState))
                break;
            nCenterPos += nDirection;
        }
    }

    // Find next diff/conflict block
    while (nCenterPos != nLimit)
    {
        DiffStates linestate = m_pViewData->GetState(m_Screen2View[nCenterPos]);
        if (!bConflict &&
            (linestate != DIFFSTATE_NORMAL) &&
            (linestate != DIFFSTATE_UNKNOWN))
        {
            break;
        }
        if (bConflict &&
            ((linestate == DIFFSTATE_CONFLICTADDED) ||
             (linestate == DIFFSTATE_CONFLICTED_IGNORED) ||
             (linestate == DIFFSTATE_CONFLICTED) ||
             (linestate == DIFFSTATE_CONFLICTEMPTY)))
        {
            break;
        }

        nCenterPos += nDirection;
    }
    if (nCenterPos == nLimit)
        return false;
    if (dryrun)
        return (nCenterPos != nLimit);

    // Find end of new block
    DiffStates state = m_pViewData->GetState(m_Screen2View[nCenterPos]);
    int nBlockEnd = nCenterPos;
    const int maxAllowedLine = nLimit-nDirection;
    while (nBlockEnd != maxAllowedLine)
    {
        const int lineIndex = nBlockEnd + nDirection;
        if (lineIndex >= linesCount)
            break;
        DiffStates lineState = m_pViewData->GetState(m_Screen2View[lineIndex]);
        if (!LinesInOneChange(nDirection, state, lineState))
            break;
        nBlockEnd += nDirection;
    }

    int nTopPos = nCenterPos - (GetScreenLines()/2);
    if (nTopPos < 0)
        nTopPos = 0;

    m_ptCaretPos.x = 0;
    m_ptCaretPos.y = nCenterPos;
    ClearSelection();
    if (nDirection > 0)
        SetupSelection(nCenterPos, nBlockEnd);
    else
        SetupSelection(nBlockEnd, nCenterPos);

    ScrollAllToLine(nTopPos, FALSE);
    RecalcAllVertScrollBars(TRUE);
    m_ptCaretPos.x = 0;
    m_nCaretGoalPos = 0;
    EnsureCaretVisible();
    OnNavigateNextinlinediff();

    UpdateViewsCaretPosition();
    UpdateCaret();
    ShowDiffLines(nCenterPos);
    return true;
}

BOOL CBaseView::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
    // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    CString strTipText;
    UINT nID = (UINT)pNMHDR->idFrom;
    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
    }

    if (pNMHDR->idFrom == (UINT)m_hWnd)
    {
        if (m_sWindowName.Left(2).Compare(_T("* "))==0)
        {
            strTipText = m_sWindowName.Mid(2) + _T("\r\n") + m_sFullFilePath;
        }
        else
        {
            strTipText = m_sWindowName + _T("\r\n") + m_sFullFilePath;
        }
    }
    else
        return FALSE;

    *pResult = 0;
    if (strTipText.IsEmpty())
        return TRUE;

    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        pTTTA->lpszText = m_szTip;
        WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
    }
    else
    {
        lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
        pTTTW->lpszText = m_wszTip;
    }

    return TRUE;    // message was handled
}

INT_PTR CBaseView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
    CRect rcClient;
    GetClientRect(rcClient);
    CRect textrect(rcClient.left, rcClient.top, rcClient.Width(), m_nLineHeight+HEADERHEIGHT);
    if (textrect.PtInRect(point))
    {
        // inside the header part of the view (showing the filename)
        pTI->hwnd = this->m_hWnd;
        this->GetClientRect(&pTI->rect);
        pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
        pTI->uId = (UINT)m_hWnd;
        pTI->lpszText = LPSTR_TEXTCALLBACK;

        // we want multi line tooltips
        CToolTipCtrl* pToolTip = AfxGetModuleThreadState()->m_pToolTip;
        if (pToolTip->GetSafeHwnd() != NULL)
        {
            pToolTip->SetMaxTipWidth(INT_MAX);
        }

        return 1;
    }
    return -1;
}

void CBaseView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
    bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);
    switch (nChar)
    {
    case VK_PRIOR:
        {
            m_ptCaretPos.y -= GetScreenLines();
            m_ptCaretPos.y = max(m_ptCaretPos.y, 0);
            m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
            OnCaretMove(bShift);
            ShowDiffLines(m_ptCaretPos.y);
        }
        break;
    case VK_NEXT:
        {
            m_ptCaretPos.y += GetScreenLines();
            if (m_ptCaretPos.y >= GetLineCount())
                m_ptCaretPos.y = GetLineCount()-1;
            m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
            OnCaretMove(bShift);
            ShowDiffLines(m_ptCaretPos.y);
        }
        break;
    case VK_HOME:
        {
            if (bControl)
            {
                ScrollAllToLine(0);
                m_ptCaretPos.x = 0;
                m_ptCaretPos.y = 0;
                m_nCaretGoalPos = 0;
                if (bShift)
                    AdjustSelection();
                else
                    ClearSelection();
                UpdateCaret();
            }
            else
            {
                m_ptCaretPos.x = 0;
                m_nCaretGoalPos = 0;
                OnCaretMove(bShift);
                ScrollAllToChar(0);
            }
        }
        break;
    case VK_END:
        {
            if (bControl)
            {
                ScrollAllToLine(GetLineCount()-GetAllMinScreenLines());
                m_ptCaretPos.y = GetLineCount()-1;
                m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
                UpdateGoalPos();
                if (bShift)
                    AdjustSelection();
                else
                    ClearSelection();
                UpdateCaret();
            }
            else
            {
                m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
                UpdateGoalPos();
                OnCaretMove(bShift);
            }
        }
        break;
    case VK_BACK:
        {
            if (m_bCaretHidden)
                break;

            if (! HasTextSelection()) {
                if (m_ptCaretPos.y == 0 && m_ptCaretPos.x == 0)
                    break;
                m_ptSelectionEndPos = m_ptCaretPos;
                MoveCaretLeft();
                m_ptSelectionStartPos = m_ptCaretPos;
            }
            RemoveSelectedText();
        }
        break;
    case VK_DELETE:
        {
            if (m_bCaretHidden)
                break;

            if (! HasTextSelection()) {
                if (! MoveCaretRight())
                    break;
                m_ptSelectionEndPos = m_ptCaretPos;
                MoveCaretLeft();
                m_ptSelectionStartPos = m_ptCaretPos;
            }
            RemoveSelectedText();
        }
        break;
    }
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBaseView::OnLButtonDown(UINT nFlags, CPoint point)
{
    int nLineFromTop = (point.y - HEADERHEIGHT) / GetLineHeight();
    int nClickedLine = nLineFromTop + m_nTopLine;
    nClickedLine--;     //we need the index
    if ((nClickedLine >= m_nTopLine)&&(nClickedLine < GetLineCount()))
    {
        m_ptCaretPos.y = nClickedLine;
        m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
        UpdateGoalPos();

        if (nFlags & MK_SHIFT)
            AdjustSelection();
        else
        {
            ClearSelection();
            SetupSelection(m_ptCaretPos.y, m_ptCaretPos.y);
        }

        UpdateViewsCaretPosition();
        Invalidate();
    }

    CView::OnLButtonDown(nFlags, point);
}

void CBaseView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    int nLineFromTop = (point.y - HEADERHEIGHT) / GetLineHeight();
    int nClickedLine = nLineFromTop + m_nTopLine;
    nClickedLine--;     //we need the index
    int nViewLine = m_Screen2View[nClickedLine];
    if (point.x < GetMarginWidth())  // only if double clicked on the margin
    {
        if((m_pViewData)&&(nViewLine < m_pViewData->GetCount())) // a double click on moved line scrolls to corresponding line
        {
            if((m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_FROM)||
                (m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_TO))
            {
                int screenLine = FindScreenLineForViewLine(m_pViewData->GetMovedIndex(nViewLine));
                ScrollAllToLine(screenLine - GetScreenLines() / 2);
                // find and select the whole moved block
                int startSel = screenLine;
                int endSel = screenLine;
                while ((startSel > 0) && ((m_pOtherViewData->GetState(startSel) == DIFFSTATE_MOVED_FROM) || (m_pOtherViewData->GetState(startSel) == DIFFSTATE_MOVED_TO)))
                    startSel--;
                startSel++;
                while ((endSel < GetLineCount()) && ((m_pOtherViewData->GetState(endSel) == DIFFSTATE_MOVED_FROM) || (m_pOtherViewData->GetState(endSel) == DIFFSTATE_MOVED_TO)))
                    endSel++;
                endSel--;
                SetupSelection(startSel, endSel);
                return CView::OnLButtonDblClk(nFlags, point);
            }
        }
    }
    if ((m_pViewData)&&(m_pMainFrame->m_bCollapsed)&&(m_pViewData->GetHideState(nViewLine) == HIDESTATE_MARKER))
    {
        // a double click on a marker expands the hidden text
        int i = nViewLine;
        while ((i < m_pViewData->GetCount())&&(m_pViewData->GetHideState(i) != HIDESTATE_SHOWN))
        {
            if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
                m_pwndLeft->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
            if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
                m_pwndRight->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
            if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
                m_pwndBottom->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
            i++;
        }
        BuildAllScreen2ViewVector();
        RecalcAllVertScrollBars();
        if (m_pwndLocator)
            m_pwndLocator->DocumentUpdated();
        if (m_pwndLeft)
            m_pwndLeft->Invalidate();
        if (m_pwndRight)
            m_pwndRight->Invalidate();
        if (m_pwndBottom)
            m_pwndBottom->Invalidate();
    }
    else
    {
        m_ptCaretPos.y = nClickedLine;
        m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
        UpdateGoalPos();

        ClearSelection();
        while (!IsCaretAtWordBoundary() && (m_ptSelectionStartPos.y == m_ptCaretPos.y) && MoveCaretLeft())
        {
        }
        if (m_ptSelectionStartPos.y != m_ptCaretPos.y)
            MoveCaretRight();

        m_ptSelectionStartPos = m_ptCaretPos;
        m_ptSelectionEndPos = m_ptCaretPos;
        m_ptSelectionOrigin = m_ptCaretPos;
        while (MoveCaretRight() && (m_ptSelectionStartPos.y == m_ptCaretPos.y) && !IsCaretAtWordBoundary())
        {
        }
        if (m_ptSelectionStartPos.y != m_ptCaretPos.y)
        {
            MoveCaretLeft();
            if (m_ptSelectionStartPos.x == m_ptCaretPos.x)
                MoveCaretLeft();
        }
        m_ptSelectionEndPos = m_ptCaretPos;

        LPCTSTR line = GetLineChars(m_ptCaretPos.y);
        if ((m_ptSelectionEndPos.x - m_ptSelectionStartPos.x) > 0)
            m_sMarkedWord = CString(&line[m_ptSelectionStartPos.x], m_ptSelectionEndPos.x - m_ptSelectionStartPos.x);
        else
        {
            m_sMarkedWord.Empty();
            ClearSelection();
        }

        m_sMarkedWord.Trim();
        if (m_sMarkedWord.IsEmpty())
        {
            ClearSelection();
        }

        if (m_pwndLeft)
            m_pwndLeft->SetMarkedWord(m_sMarkedWord);
        if (m_pwndRight)
            m_pwndRight->SetMarkedWord(m_sMarkedWord);
        if (m_pwndBottom)
            m_pwndBottom->SetMarkedWord(m_sMarkedWord);


        SetupSelection(m_ptCaretPos.y, m_ptCaretPos.y);

        UpdateViewsCaretPosition();
        Invalidate();
        if (m_pwndLocator)
            m_pwndLocator->Invalidate();
    }

    CView::OnLButtonDblClk(nFlags, point);
}

void CBaseView::OnEditCopy()
{
    if ((m_ptSelectionStartPos.x == m_ptSelectionEndPos.x)&&(m_ptSelectionStartPos.y == m_ptSelectionEndPos.y))
        return;
    // first store the selected lines in one CString
    CString sCopyData;
    for (int i=m_ptSelectionStartPos.y; i<=m_ptSelectionEndPos.y; i++)
    {
        int viewIndex = m_Screen2View[i];
        switch (m_pViewData->GetState(viewIndex))
        {
        case DIFFSTATE_EMPTY:
            break;
        case DIFFSTATE_UNKNOWN:
        case DIFFSTATE_NORMAL:
        case DIFFSTATE_REMOVED:
        case DIFFSTATE_REMOVEDWHITESPACE:
        case DIFFSTATE_ADDED:
        case DIFFSTATE_ADDEDWHITESPACE:
        case DIFFSTATE_WHITESPACE:
        case DIFFSTATE_WHITESPACE_DIFF:
        case DIFFSTATE_CONFLICTED:
        case DIFFSTATE_CONFLICTED_IGNORED:
        case DIFFSTATE_CONFLICTADDED:
        case DIFFSTATE_CONFLICTEMPTY:
        case DIFFSTATE_CONFLICTRESOLVED:
        case DIFFSTATE_IDENTICALREMOVED:
        case DIFFSTATE_IDENTICALADDED:
        case DIFFSTATE_THEIRSREMOVED:
        case DIFFSTATE_THEIRSADDED:
        case DIFFSTATE_MOVED_FROM:
        case DIFFSTATE_MOVED_TO:
        case DIFFSTATE_YOURSREMOVED:
        case DIFFSTATE_YOURSADDED:
        case DIFFSTATE_EDITED:
            sCopyData += GetLineChars(i);
            sCopyData += _T("\r\n");
            break;
        }
    }
    // remove the last \r\n
    sCopyData = sCopyData.Left(sCopyData.GetLength()-2);
    // remove the non-selected chars from the first line
    sCopyData = sCopyData.Mid(m_ptSelectionStartPos.x);
    // remove the non-selected chars from the last line
    int lastLinePos = sCopyData.ReverseFind('\n');
    lastLinePos += 1;
    if (lastLinePos == 0)
        lastLinePos -= m_ptSelectionStartPos.x;
    sCopyData = sCopyData.Left(lastLinePos+m_ptSelectionEndPos.x);
    if (!sCopyData.IsEmpty())
    {
        CStringUtils::WriteAsciiStringToClipboard(sCopyData, m_hWnd);
    }
}

void CBaseView::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_pMainFrame->m_nMoveMovesToIgnore > 0) {
        --m_pMainFrame->m_nMoveMovesToIgnore;
        CView::OnMouseMove(nFlags, point);
        return;
    }
    int nLineFromTop = (point.y - HEADERHEIGHT) / GetLineHeight();
    int nMouseLine = nLineFromTop + m_nTopLine;
    nMouseLine--;       //we need the index
    if (nMouseLine < -1)
    {
        nMouseLine = -1;
    }
    ShowDiffLines(nMouseLine);

    KillTimer(IDT_SCROLLTIMER);
    if (nFlags & MK_LBUTTON)
    {
        int saveMouseLine = nMouseLine >= 0 ? nMouseLine : 0;
        saveMouseLine = saveMouseLine < GetLineCount() ? saveMouseLine : GetLineCount() - 1;
        int charIndex = CalculateCharIndex(saveMouseLine, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
        if (((m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))&&
            ((nMouseLine >= m_nTopLine)&&(nMouseLine < GetLineCount())))
        {
            m_ptCaretPos.y = nMouseLine;
            m_ptCaretPos.x = charIndex;
            UpdateGoalPos();
            AdjustSelection();
            UpdateCaret();
            Invalidate();
            UpdateWindow();
        }
        if (nMouseLine < m_nTopLine)
        {
            ScrollAllToLine(m_nTopLine-1, TRUE);
            SetTimer(IDT_SCROLLTIMER, 20, NULL);
        }
        if (nMouseLine >= m_nTopLine + GetScreenLines())
        {
            ScrollAllToLine(m_nTopLine+1, TRUE);
            SetTimer(IDT_SCROLLTIMER, 20, NULL);
        }
        if ((m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth()) <= m_nOffsetChar)
        {
            ScrollAllSide(-1);
            SetTimer(IDT_SCROLLTIMER, 20, NULL);
        }
        if (charIndex >= (GetScreenChars()+m_nOffsetChar-4))
        {
            ScrollAllSide(1);
            SetTimer(IDT_SCROLLTIMER, 20, NULL);
        }
    }

    if (!m_bMouseWithin)
    {
        m_bMouseWithin = TRUE;
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        _TrackMouseEvent(&tme);
    }

    CView::OnMouseMove(nFlags, point);
}

void CBaseView::OnMouseLeave()
{
    ShowDiffLines(-1);
    m_bMouseWithin = FALSE;
    KillTimer(IDT_SCROLLTIMER);
    CView::OnMouseLeave();
}

void CBaseView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == IDT_SCROLLTIMER)
    {
        POINT point;
        GetCursorPos(&point);
        ScreenToClient(&point);
        int nLineFromTop = (point.y - HEADERHEIGHT) / GetLineHeight();
        int nMouseLine = nLineFromTop + m_nTopLine;
        nMouseLine--;       //we need the index
        if (nMouseLine < -1)
        {
            nMouseLine = -1;
        }
        if (GetKeyState(VK_LBUTTON)&0x8000)
        {
            int saveMouseLine = nMouseLine >= 0 ? nMouseLine : 0;
            saveMouseLine = saveMouseLine < GetLineCount() ? saveMouseLine : GetLineCount() - 1;
            int charIndex = CalculateCharIndex(saveMouseLine, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
            if (nMouseLine < m_nTopLine)
            {
                ScrollAllToLine(m_nTopLine-1, TRUE);
                SetTimer(IDT_SCROLLTIMER, 20, NULL);
            }
            if (nMouseLine >= m_nTopLine + GetScreenLines())
            {
                ScrollAllToLine(m_nTopLine+1, TRUE);
                SetTimer(IDT_SCROLLTIMER, 20, NULL);
            }
            if ((m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth()) <= m_nOffsetChar)
            {
                ScrollAllSide(-1);
                SetTimer(IDT_SCROLLTIMER, 20, NULL);
            }
            if (charIndex >= (GetScreenChars()+m_nOffsetChar-4))
            {
                ScrollAllSide(1);
                SetTimer(IDT_SCROLLTIMER, 20, NULL);
            }
        }

    }

    CView::OnTimer(nIDEvent);
}

void CBaseView::SelectLines(int nLine1, int nLine2)
{
    if (nLine2 == -1)
        nLine2 = nLine1;
    m_nSelBlockStart = nLine1;
    m_nSelBlockEnd = nLine2;
    Invalidate();
}

void CBaseView::ShowDiffLines(int nLine)
{
    if ((nLine < m_nTopLine)||(nLine >= GetLineCount()))
    {
        m_pwndLineDiffBar->ShowLines(nLine);
        nLine = -1;
        m_nMouseLine = nLine;
        return;
    }

    if ((!m_pwndRight)||(!m_pwndLeft))
        return;
    if(m_pMainFrame->m_bOneWay)
        return;

    nLine = (nLine > (int)m_pwndRight->m_Screen2View.size() ? -1 : nLine);
    nLine = (nLine > (int)m_pwndLeft->m_Screen2View.size() ? -1 : nLine);

    if (nLine < 0)
        return;

    if (nLine != m_nMouseLine)
    {
        if (nLine >= GetLineCount())
            nLine = -1;
        m_nMouseLine = nLine;
        m_pwndLineDiffBar->ShowLines(nLine);
    }
    m_pMainFrame->m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

void CBaseView::UseTheirAndYourBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        return;
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        bottomstate.difflines[viewline] = m_pwndBottom->m_pViewData->GetLine(viewline);
        m_pwndBottom->m_pViewData->SetLine(viewline, m_pwndLeft->m_pViewData->GetLine(viewline));
        bottomstate.linestates[viewline] = m_pwndBottom->m_pViewData->GetState(viewline);
        m_pwndBottom->m_pViewData->SetState(viewline, m_pwndLeft->m_pViewData->GetState(viewline));
        m_pwndBottom->m_pViewData->SetLineEnding(viewline, EOL_AUTOLINE);
        if (m_pwndBottom->IsViewLineConflicted(i))
        {
            if (m_pwndLeft->m_pViewData->GetState(viewline) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewline, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewline, DIFFSTATE_CONFLICTRESOLVED);
        }
        m_pwndLeft->m_pViewData->SetState(viewline, DIFFSTATE_YOURSADDED);
    }

    // your block is done, now insert their block
    int viewindex = m_Screen2View[m_nSelBlockEnd+1];
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        bottomstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd+1]);
        m_pwndBottom->m_pViewData->InsertData(viewindex, m_pwndRight->m_pViewData->GetData(viewline));
        if (m_pwndBottom->IsViewLineConflicted(viewindex))
        {
            if (m_pwndRight->m_pViewData->GetState(viewline) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewindex, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewindex, DIFFSTATE_CONFLICTRESOLVED);
        }
        m_pwndRight->m_pViewData->SetState(viewline, DIFFSTATE_THEIRSADDED);
        viewindex++;
    }
    // adjust line numbers
    for (int i = m_nSelBlockEnd+1; i < GetLineCount(); ++i)
    {
        int viewline = m_Screen2View[i];
        long oldline = (long)m_pwndBottom->m_pViewData->GetLineNumber(viewline);
        if (oldline >= 0)
            m_pwndBottom->m_pViewData->SetLineNumber(viewline, oldline+(viewindex-m_Screen2View[m_nSelBlockEnd]));
    }

    // now insert an empty block in both yours and theirs
    for (int emptyblocks = 0; emptyblocks < m_nSelBlockEnd - m_nSelBlockStart + 1; ++emptyblocks)
    {
        rightstate.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
        m_pwndRight->m_pViewData->InsertData(m_Screen2View[m_nSelBlockStart], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
        m_pwndLeft->m_pViewData->InsertData(m_Screen2View[m_nSelBlockEnd+1], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
        leftstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd+1]);
    }
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndBottom->SetModified();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
}

void CBaseView::UseYourAndTheirBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        return;
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        bottomstate.difflines[viewline] = m_pwndBottom->m_pViewData->GetLine(viewline);
        m_pwndBottom->m_pViewData->SetLine(viewline, m_pwndRight->m_pViewData->GetLine(viewline));
        bottomstate.linestates[viewline] = m_pwndBottom->m_pViewData->GetState(viewline);
        m_pwndBottom->m_pViewData->SetState(viewline, m_pwndRight->m_pViewData->GetState(viewline));
        rightstate.linestates[viewline] = m_pwndRight->m_pViewData->GetState(viewline);
        m_pwndBottom->m_pViewData->SetLineEnding(viewline, EOL_AUTOLINE);
        if (m_pwndBottom->IsViewLineConflicted(viewline))
        {
            if (m_pwndRight->m_pViewData->GetState(viewline) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewline, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewline, DIFFSTATE_CONFLICTRESOLVED);
        }
        m_pwndRight->m_pViewData->SetState(viewline, DIFFSTATE_YOURSADDED);
    }

    // your block is done, now insert their block
    int viewindex = m_Screen2View[m_nSelBlockEnd+1];
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        bottomstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd+1]);
        m_pwndBottom->m_pViewData->InsertData(viewindex, m_pwndLeft->m_pViewData->GetData(viewline));
        leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(viewline);
        if (m_pwndBottom->IsViewLineConflicted(viewindex))
        {
            if (m_pwndLeft->m_pViewData->GetState(viewline) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewindex, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewindex, DIFFSTATE_CONFLICTRESOLVED);
        }
        m_pwndLeft->m_pViewData->SetState(viewline, DIFFSTATE_THEIRSADDED);
        viewindex++;
    }
    // adjust line numbers
    for (int i = m_nSelBlockEnd+1; i < m_pwndBottom->GetLineCount(); ++i)
    {
        int viewline = m_Screen2View[i];
        long oldline = (long)m_pwndBottom->m_pViewData->GetLineNumber(viewline);
        if (oldline >= 0)
            m_pwndBottom->m_pViewData->SetLineNumber(viewline, oldline+(viewindex-m_Screen2View[m_nSelBlockEnd]));
    }

    // now insert an empty block in both yours and theirs
    for (int emptyblocks = 0; emptyblocks < m_nSelBlockEnd - m_nSelBlockStart + 1; ++emptyblocks)
    {
        leftstate.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
        m_pwndLeft->m_pViewData->InsertData(m_Screen2View[m_nSelBlockStart], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
        m_pwndRight->m_pViewData->InsertData(m_Screen2View[m_nSelBlockEnd+1], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
        rightstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd+1]);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndBottom->SetModified();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
}

void CBaseView::UseBothRightFirst(viewstate &rightstate, viewstate &leftstate)
{
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        return;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(viewline);
        m_pwndRight->m_pViewData->SetState(viewline, DIFFSTATE_YOURSADDED);
    }

    // your block is done, now insert their block
    int viewindex = m_Screen2View[m_nSelBlockEnd+1];
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        rightstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd+1]);
        m_pwndRight->m_pViewData->InsertData(viewindex, m_pwndLeft->m_pViewData->GetData(viewline));
        m_pwndRight->m_pViewData->SetState(viewindex++, DIFFSTATE_THEIRSADDED);
    }
    // adjust line numbers
    viewindex--;
    for (int i = m_nSelBlockEnd+1; i < m_pwndRight->GetLineCount(); ++i)
    {
        int viewline = m_Screen2View[i];
        long oldline = (long)m_pwndRight->m_pViewData->GetLineNumber(viewline);
        if (oldline >= 0)
            m_pwndRight->m_pViewData->SetLineNumber(i, oldline+(viewindex-m_Screen2View[m_nSelBlockEnd]));
    }

    // now insert an empty block in the left view
    for (int emptyblocks = 0; emptyblocks < m_nSelBlockEnd - m_nSelBlockStart + 1; ++emptyblocks)
    {
        leftstate.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
        m_pwndLeft->m_pViewData->InsertData(m_Screen2View[m_nSelBlockStart], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
}

void CBaseView::UseBothLeftFirst(viewstate &rightstate, viewstate &leftstate)
{
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        return;
    // get line number from just before the block
    long linenumber = 0;
    if (m_nSelBlockStart > 0)
        linenumber = m_pwndRight->m_pViewData->GetLineNumber(m_nSelBlockStart-1);
    linenumber++;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        rightstate.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
        m_pwndRight->m_pViewData->InsertData(i, m_pwndLeft->m_pViewData->GetLine(viewline), DIFFSTATE_THEIRSADDED, linenumber++, m_pwndLeft->m_pViewData->GetLineEnding(viewline), HIDESTATE_SHOWN, -1);
    }
    // adjust line numbers
    for (int i = m_nSelBlockEnd + 1; i < m_pwndRight->GetLineCount(); ++i)
    {
        int viewline = m_Screen2View[i];
        long oldline = (long)m_pwndRight->m_pViewData->GetLineNumber(viewline);
        if (oldline >= 0)
            m_pwndRight->m_pViewData->SetLineNumber(viewline, oldline+(m_nSelBlockEnd-m_nSelBlockStart)+1);
    }

    // now insert an empty block in left view
    for (int emptyblocks = 0; emptyblocks < m_nSelBlockEnd - m_nSelBlockStart + 1; ++emptyblocks)
    {
        leftstate.addedlines.push_back(m_Screen2View[m_nSelBlockEnd + 1]);
        m_pwndLeft->m_pViewData->InsertData(m_Screen2View[m_nSelBlockEnd + 1], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
}

void CBaseView::UpdateCaret()
{
    if (m_ptCaretPos.y >= GetLineCount())
        m_ptCaretPos.y = GetLineCount()-1;
    if (m_ptCaretPos.y < 0)
        m_ptCaretPos.y = 0;
    if (m_ptCaretPos.x > GetLineLength(m_ptCaretPos.y))
        m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
    if (m_ptCaretPos.x < 0)
        m_ptCaretPos.x = 0;

    int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);

    if (m_bFocused && !m_bCaretHidden &&
        m_ptCaretPos.y >= m_nTopLine &&
        m_ptCaretPos.y < (m_nTopLine+GetScreenLines()) &&
        nCaretOffset >= m_nOffsetChar &&
        nCaretOffset < (m_nOffsetChar+GetScreenChars()))
    {
        CreateSolidCaret(2, GetLineHeight());
        SetCaretPos(TextToClient(m_ptCaretPos));
        ShowCaret();
    }
    else
    {
        HideCaret();
    }
}

void CBaseView::EnsureCaretVisible()
{
    int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);

    if (m_ptCaretPos.y < m_nTopLine)
        ScrollAllToLine(m_ptCaretPos.y);
    int screnLines = GetScreenLines();
    if (m_ptCaretPos.y >= (m_nTopLine+screnLines))
        ScrollAllToLine(m_ptCaretPos.y-screnLines+1);
    if (nCaretOffset < m_nOffsetChar)
        ScrollAllToChar(nCaretOffset);
    if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
        ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
}

int CBaseView::CalculateActualOffset(int nLineIndex, int nCharIndex)
{
    int nLength = GetLineLength(nLineIndex);
    ASSERT(nCharIndex >= 0);
    if (nCharIndex > nLength)
        nCharIndex = nLength;
    LPCTSTR pszChars = GetLineChars(nLineIndex);
    int nOffset = 0;
    int nTabSize = GetTabSize();
    for (int I = 0; I < nCharIndex; I ++)
    {
        if (pszChars[I] == _T('\t'))
            nOffset += (nTabSize - nOffset % nTabSize);
        else
            nOffset++;
    }
    return nOffset;
}

int CBaseView::CalculateCharIndex(int nLineIndex, int nActualOffset)
{
    int nLength = GetLineLength(nLineIndex);
    LPCTSTR pszLine = GetLineChars(nLineIndex);
    int nIndex = 0;
    int nOffset = 0;
    int nTabSize = GetTabSize();
    while (nOffset < nActualOffset && nIndex < nLength)
    {
        if (pszLine[nIndex] == _T('\t'))
            nOffset += (nTabSize - nOffset % nTabSize);
        else
            ++nOffset;
        ++nIndex;
    }
    return nIndex;
}

POINT CBaseView::TextToClient(const POINT& point)
{
    POINT pt;
    pt.y = max(0, (point.y - m_nTopLine));
    pt.y *= GetLineHeight();
    pt.x = CalculateActualOffset(point.y, point.x);

    pt.x = (pt.x - m_nOffsetChar) * GetCharWidth() + GetMarginWidth();
    pt.y = (pt.y + GetLineHeight() + HEADERHEIGHT);
    return pt;
}

void CBaseView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CView::OnChar(nChar, nRepCnt, nFlags);

    if (m_bCaretHidden)
        return;

    if ((::GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
        (::GetKeyState(VK_RBUTTON) & 0x8000) != 0)
    {
        return;
    }

    if ((nChar > 31)||(nChar == VK_TAB))
    {
        RemoveSelectedText();
        AddUndoLine(m_ptCaretPos.y);
        CString sLine = GetLineChars(m_ptCaretPos.y);
        sLine.Insert(m_ptCaretPos.x, (wchar_t)nChar);
        int viewLine = m_Screen2View[m_ptCaretPos.y];
        m_pViewData->SetLine(viewLine, sLine);
        m_pViewData->SetState(viewLine, DIFFSTATE_EDITED);
        if (m_pViewData->GetLineEnding(viewLine) == EOL_NOENDING)
            m_pViewData->SetLineEnding(viewLine, EOL_AUTOLINE);
        m_ptCaretPos.x++;
        m_nCachedWrappedLine = -1;
        UpdateGoalPos();
    }
    else if (nChar == VK_RETURN)
    {
        // insert a new, fresh and empty line below the cursor
        RemoveSelectedText();
        AddUndoLine(m_ptCaretPos.y, true);
        BuildAllScreen2ViewVector();
        // move the cursor to the new line
        m_ptCaretPos.y++;
        m_ptCaretPos.x = 0;
        m_nCachedWrappedLine = -1;
        UpdateGoalPos();
    }
    else
        return; // Unknown control character -- ignore it.
    ClearSelection();
    EnsureCaretVisible();
    UpdateCaret();
    SetModified(true);
    Invalidate(FALSE);
}

void CBaseView::AddUndoLine(int nLine, bool bAddEmptyLine)
{
    viewstate leftstate;
    viewstate rightstate;
    viewstate bottomstate;
    int viewLine = m_Screen2View[nLine];
    leftstate.AddViewLineFormView(m_pwndLeft, nLine, viewLine, bAddEmptyLine);
    rightstate.AddViewLineFormView(m_pwndRight, nLine, viewLine, bAddEmptyLine);
    bottomstate.AddViewLineFormView(m_pwndBottom, nLine, viewLine, bAddEmptyLine);
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    RecalcAllVertScrollBars();
    Invalidate(FALSE);
}

void CBaseView::AddEmptyLine(int nLineIndex)
{
    if (m_pViewData == NULL)
        return;
    int viewLine = m_Screen2View[nLineIndex];
    if (!m_bCaretHidden)
    {
        CString sPartLine = GetLineChars(nLineIndex);
        m_pViewData->SetLine(viewLine, sPartLine.Left(m_ptCaretPos.x));
        sPartLine = sPartLine.Mid(m_ptCaretPos.x);
        m_pViewData->InsertData(viewLine+1, sPartLine, DIFFSTATE_EDITED, -1, m_pViewData->GetLineEnding(viewLine) == EOL_NOENDING ? EOL_AUTOLINE : m_pViewData->GetLineEnding(viewLine), HIDESTATE_SHOWN, -1);
    }
    else
        m_pViewData->InsertData(viewLine+1, _T(""), DIFFSTATE_EDITED, -1, m_pViewData->GetLineEnding(viewLine) == EOL_NOENDING ? EOL_AUTOLINE : m_pViewData->GetLineEnding(viewLine), HIDESTATE_SHOWN, -1);
    m_Screen2View.insert(m_Screen2View.begin()+nLineIndex, viewLine+1);
}

void CBaseView::RemoveLine(int nLineIndex)
{
    if (m_pViewData == NULL)
        return;
    m_pViewData->RemoveData(m_Screen2View[nLineIndex]);
    BuildScreen2ViewVector();
    if (m_ptCaretPos.y >= GetLineCount())
        m_ptCaretPos.y = GetLineCount()-1;
}

void CBaseView::RemoveSelectedText()
{
    if (m_pViewData == NULL)
        return;
    if (!HasTextSelection())
        return;

    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    std::vector<LONG> linestoremove;
    for (LONG i = m_ptSelectionStartPos.y; i <= m_ptSelectionEndPos.y; ++i)
    {
        int viewLine = m_Screen2View[i];
        if (i == m_ptSelectionStartPos.y)
        {
            CString sLine = GetLineChars(m_ptSelectionStartPos.y);
            CString newLine;
            if (i == m_ptSelectionStartPos.y)
            {
                if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
                {
                    leftstate.difflines[viewLine] = m_pwndLeft->m_pViewData->GetLine(viewLine);
                    leftstate.linestates[viewLine] = m_pwndLeft->m_pViewData->GetState(viewLine);
                }
                if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
                {
                    rightstate.difflines[viewLine] = m_pwndRight->m_pViewData->GetLine(viewLine);
                    rightstate.linestates[viewLine] = m_pwndRight->m_pViewData->GetState(viewLine);
                }
                if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
                {
                    bottomstate.difflines[viewLine] = m_pwndBottom->m_pViewData->GetLine(viewLine);
                    bottomstate.linestates[viewLine] = m_pwndBottom->m_pViewData->GetState(viewLine);
                }
                newLine = sLine.Left(m_ptSelectionStartPos.x);
                sLine = GetLineChars(m_ptSelectionEndPos.y);
                newLine = newLine + sLine.Mid(m_ptSelectionEndPos.x);
            }
            m_pViewData->SetLine(viewLine, newLine);
            m_pViewData->SetState(viewLine, DIFFSTATE_EDITED);
            if (m_pViewData->GetLineEnding(viewLine) == EOL_NOENDING)
                m_pViewData->SetLineEnding(viewLine, EOL_AUTOLINE);
            SetModified();
        }
        else
        {
            if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
            {
                leftstate.removedlines[viewLine] = m_pwndLeft->m_pViewData->GetData(viewLine);
            }
            if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
            {
                rightstate.removedlines[viewLine] = m_pwndRight->m_pViewData->GetData(viewLine);
            }
            if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
            {
                bottomstate.removedlines[viewLine] = m_pwndBottom->m_pViewData->GetData(viewLine);
            }
            linestoremove.push_back(i);
        }
    }
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    // remove the lines at the end, to avoid problems with line indexes
    if (linestoremove.size())
    {
        std::vector<LONG>::const_iterator it = linestoremove.begin();
        int nLineToRemove = *it;
        for ( ; it != linestoremove.end(); ++it)
        {
            if (m_pwndLeft)
                m_pwndLeft->RemoveLine(nLineToRemove);
            if (m_pwndRight)
                m_pwndRight->RemoveLine(nLineToRemove);
            if (m_pwndBottom)
                m_pwndBottom->RemoveLine(nLineToRemove);
            SetModified();
        }
        BuildAllScreen2ViewVector();
        RecalcAllVertScrollBars();
    }
    m_ptCaretPos = m_ptSelectionStartPos;
    UpdateGoalPos();
    ClearSelection();
    UpdateCaret();
    EnsureCaretVisible();
    Invalidate(FALSE);
}

void CBaseView::PasteText()
{
    if (!OpenClipboard())
        return;

    CString sClipboardText;
    HGLOBAL hglb = GetClipboardData(CF_TEXT);
    if (hglb)
    {
        LPCSTR lpstr = (LPCSTR)GlobalLock(hglb);
        sClipboardText = CString(lpstr);
        GlobalUnlock(hglb);
    }
    hglb = GetClipboardData(CF_UNICODETEXT);
    if (hglb)
    {
        LPCTSTR lpstr = (LPCTSTR)GlobalLock(hglb);
        sClipboardText = lpstr;
        GlobalUnlock(hglb);
    }
    CloseClipboard();

    if (sClipboardText.IsEmpty())
        return;

    sClipboardText.Replace(_T("\r\n"), _T("\r"));
    sClipboardText.Replace('\n', '\r');

    int pasteLines = 0;
    int iStart = 0;
    while ((iStart = sClipboardText.Find('\r', iStart))>=0)
    {
        pasteLines++;
        iStart++;
    }
    CViewData leftState;
    CViewData rightState;
    int selStartPos = m_ptSelectionStartPos.y;
    int viewLine = m_Screen2View[selStartPos];
    for (int i = selStartPos; i < (selStartPos + pasteLines); ++i)
    {
        if (m_pwndLeft)
        {
            if (!m_pwndLeft->HasCaret())
            {
                leftState.AddData(m_pwndLeft->m_pViewData->GetData(m_Screen2View[selStartPos]));
                m_pwndLeft->m_Screen2View.push_back(viewLine);
            }
        }
        if (m_pwndRight)
        {
            if (!m_pwndRight->HasCaret())
            {
                rightState.AddData(m_pwndRight->m_pViewData->GetData(m_Screen2View[selStartPos]));
                m_pwndRight->m_Screen2View.push_back(viewLine);
            }
        }
        viewLine++;
    }

    // We want to undo the insertion in a single step.
    CUndo::GetInstance().BeginGrouping();
    // use the easy way to insert text:
    // insert char by char, using the OnChar() method
    for (int i=0; i<sClipboardText.GetLength(); ++i)
    {
        OnChar(sClipboardText[i], 0, 0);
    }

    BuildAllScreen2ViewVector();
    // restore the lines in the non-editing views
    for (int i = selStartPos; i < (selStartPos + pasteLines); ++i)
    {
        restoreLines(m_pwndLeft, leftState, i, i-selStartPos);
        restoreLines(m_pwndRight, rightState, i, i-m_ptSelectionStartPos.y);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    CUndo::GetInstance().EndGrouping();
}

void CBaseView::OnCaretDown()
{
    m_ptCaretPos.y++;
    m_ptCaretPos.y = min(m_ptCaretPos.y, GetLineCount()-1);
    m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
    OnCaretMove();
    ShowDiffLines(m_ptCaretPos.y);
}

bool CBaseView::MoveCaretLeft()
{
    if (m_ptCaretPos.x == 0)
    {
        if (m_ptCaretPos.y > 0)
        {
            --m_ptCaretPos.y;
            m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
        }
        else
            return false;
    }
    else
        --m_ptCaretPos.x;

    UpdateGoalPos();
    return true;
}

bool CBaseView::MoveCaretRight()
{
    if (m_ptCaretPos.x >= GetLineLength(m_ptCaretPos.y))
    {
        if (m_ptCaretPos.y < (GetLineCount() - 1))
        {
            ++m_ptCaretPos.y;
            m_ptCaretPos.x = 0;
        }
        else
            return false;
    }
    else
        ++m_ptCaretPos.x;

    UpdateGoalPos();
    return true;
}

void CBaseView::UpdateGoalPos()
{
    m_nCaretGoalPos = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
}

void CBaseView::OnCaretLeft()
{
    MoveCaretLeft();
    OnCaretMove();
}

void CBaseView::OnCaretRight()
{
    MoveCaretRight();
    OnCaretMove();
}

void CBaseView::OnCaretUp()
{
    m_ptCaretPos.y--;
    m_ptCaretPos.y = max(0, m_ptCaretPos.y);
    m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
    OnCaretMove();
    ShowDiffLines(m_ptCaretPos.y);
}

bool CBaseView::IsWordSeparator(wchar_t ch) const
{
    return ch == ' ' || ch == '\t' || (m_sWordSeparators.Find(ch) >= 0);
}

bool CBaseView::IsCaretAtWordBoundary()
{
    LPCTSTR line = GetLineChars(m_ptCaretPos.y);
    if (!*line)
        return false; // no boundary at the empty lines
    if (m_ptCaretPos.x == 0)
        return !IsWordSeparator(line[m_ptCaretPos.x]);
    if (m_ptCaretPos.x >= GetLineLength(m_ptCaretPos.y))
        return !IsWordSeparator(line[m_ptCaretPos.x - 1]);
    return
        IsWordSeparator(line[m_ptCaretPos.x]) !=
        IsWordSeparator(line[m_ptCaretPos.x - 1]);
}

void CBaseView::UpdateViewsCaretPosition()
{
    if (m_pwndBottom)
        m_pwndBottom->UpdateCaretPosition(m_ptCaretPos);
    if (m_pwndLeft)
        m_pwndLeft->UpdateCaretPosition(m_ptCaretPos);
    if (m_pwndRight)
        m_pwndRight->UpdateCaretPosition(m_ptCaretPos);
}

void CBaseView::OnCaretWordleft()
{
    while (MoveCaretLeft() && !IsCaretAtWordBoundary())
    {
    }
    OnCaretMove();
}

void CBaseView::OnCaretWordright()
{
    while (MoveCaretRight() && !IsCaretAtWordBoundary())
    {
    }
    OnCaretMove();
}

void CBaseView::ClearCurrentSelection()
{
    m_ptSelectionStartPos = m_ptCaretPos;
    m_ptSelectionEndPos = m_ptCaretPos;
    m_ptSelectionOrigin = m_ptCaretPos;
    m_nSelBlockStart = -1;
    m_nSelBlockEnd = -1;
    Invalidate(FALSE);
}

void CBaseView::ClearSelection()
{
    if (m_pwndLeft)
        m_pwndLeft->ClearCurrentSelection();
    if (m_pwndRight)
        m_pwndRight->ClearCurrentSelection();
    if (m_pwndBottom)
        m_pwndBottom->ClearCurrentSelection();
}

void CBaseView::AdjustSelection()
{
    if ((m_ptCaretPos.y < m_ptSelectionOrigin.y) ||
        (m_ptCaretPos.y == m_ptSelectionOrigin.y && m_ptCaretPos.x <= m_ptSelectionOrigin.x))
    {
        m_ptSelectionStartPos = m_ptCaretPos;
        m_ptSelectionEndPos = m_ptSelectionOrigin;
    }

    if ((m_ptCaretPos.y > m_ptSelectionOrigin.y) ||
        (m_ptCaretPos.y == m_ptSelectionOrigin.y && m_ptCaretPos.x >= m_ptSelectionOrigin.x))
    {
        m_ptSelectionStartPos = m_ptSelectionOrigin;
        m_ptSelectionEndPos = m_ptCaretPos;
    }

    SetupSelection(min(m_ptSelectionStartPos.y, m_ptSelectionEndPos.y), max(m_ptSelectionStartPos.y, m_ptSelectionEndPos.y));

    Invalidate(FALSE);
}

void CBaseView::OnEditCut()
{
    if (!m_bCaretHidden)
    {
        OnEditCopy();
        RemoveSelectedText();
    }
}

void CBaseView::OnEditPaste()
{
    if (!m_bCaretHidden)
    {
        PasteText();
    }
}

void CBaseView::DeleteFonts()
{
    for (int i=0; i<fontsCount; i++)
    {
        if (m_apFonts[i] != NULL)
        {
            m_apFonts[i]->DeleteObject();
            delete m_apFonts[i];
            m_apFonts[i] = NULL;
        }
    }
}

void CBaseView::OnCaretMove()
{
    const bool isShiftPressed = (GetKeyState(VK_SHIFT)&0x8000) != 0;
    OnCaretMove(isShiftPressed);
}

void CBaseView::OnCaretMove(bool isShiftPressed)
{
    if(isShiftPressed)
        AdjustSelection();
    else
        ClearSelection();
    EnsureCaretVisible();
    UpdateCaret();
}

UINT CBaseView::GetMenuFlags(DiffStates state) const
{
    UINT uFlags = MF_ENABLED | MF_STRING;
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        uFlags |= MF_DISABLED | MF_GRAYED;

    const bool bImportantBlock = state != DIFFSTATE_UNKNOWN;
    if(bImportantBlock)
        return uFlags | MF_ENABLED;

    return uFlags | MF_DISABLED | MF_GRAYED;
}

void CBaseView::AddCutCopyAndPaste(CMenu& popup)
{
    popup.AppendMenu(MF_SEPARATOR, NULL);
    CString temp;
    temp.LoadString(IDS_EDIT_COPY);
    popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_COPY, temp);
    if (!m_bCaretHidden)
    {
        temp.LoadString(IDS_EDIT_CUT);
        popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_CUT, temp);
        temp.LoadString(IDS_EDIT_PASTE);
        popup.AppendMenu(MF_STRING | (CAppUtils::HasClipboardFormat(CF_UNICODETEXT)||CAppUtils::HasClipboardFormat(CF_TEXT) ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_PASTE, temp);
    }
}

void CBaseView::CompensateForKeyboard(CPoint& point)
{
    // if the context menu is invoked through the keyboard, we have to use
    // a calculated position on where to anchor the menu on
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        GetWindowRect(&rect);
        point = rect.CenterPoint();
    }
}

HICON CBaseView::LoadIcon(WORD iconId)
{
    HANDLE icon = ::LoadImage( AfxGetResourceHandle(), MAKEINTRESOURCE(iconId),
                        IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    return (HICON)icon;
}

void CBaseView::ReleaseBitmap()
{
    if (m_pCacheBitmap != NULL)
    {
        m_pCacheBitmap->DeleteObject();
        delete m_pCacheBitmap;
        m_pCacheBitmap = NULL;
    }
}

void CBaseView::BuildMarkedWordArray()
{
    int lineCount = GetLineCount();
    m_arMarkedWordLines.clear();
    m_arMarkedWordLines.reserve(lineCount);
    for (int i = 0; i < lineCount; ++i)
    {
        LPCTSTR line = GetLineChars(i);
        if (line)
            m_arMarkedWordLines.push_back(_tcsstr(line, (LPCTSTR)m_sMarkedWord) != NULL);
        else
            m_arMarkedWordLines.push_back(0);
    }
}

void CBaseView::restoreLines(CBaseView* view, CViewData& viewState, int targetIndex, int sourceIndex) const
{
    if (view == 0)
        return;
    if (view->HasCaret())
        return;

    CViewData* targetState = view->m_pViewData;
    int targetView = m_Screen2View[targetIndex];
    int sourceView = m_Screen2View[sourceIndex];
    targetState->SetLine(targetView, viewState.GetLine(sourceView));
    targetState->SetLineEnding(targetView, viewState.GetLineEnding(sourceView));
    targetState->SetLineNumber(targetView, viewState.GetLineNumber(sourceView));
    targetState->SetState(targetView, viewState.GetState(sourceView));
    targetState->SetLineHideState(targetView, viewState.GetHideState(sourceView));
}

bool CBaseView::GetInlineDiffPositions(int lineIndex, std::vector<inlineDiffPos>& positions)
{
    if (!m_bShowInlineDiff)
        return false;
    if ((m_pwndBottom != NULL) && !(m_pwndBottom->IsHidden()))
        return false;

    LPCTSTR line = GetLineChars(lineIndex);
    if (line == NULL)
        return false;
    if (line[0] == 0)
        return false;

    CheckOtherView();

    LPCTSTR pszDiffChars = NULL;
    int nDiffLength = 0;
    if (m_pOtherViewData)
    {
        int viewLine = m_Screen2View[lineIndex];
        int index = min(viewLine, m_pOtherViewData->GetCount() - 1);
        pszDiffChars = m_pOtherViewData->GetLine(index);
        nDiffLength = m_pOtherViewData->GetLine(index).GetLength();
    }

    if (!pszDiffChars || !*pszDiffChars)
        return false;

    CString diffline;
    ExpandChars(pszDiffChars, 0, nDiffLength, diffline);
    svn_diff_t * diff = NULL;
    m_svnlinediff.Diff(&diff, line, _tcslen(line), diffline, diffline.GetLength(), m_bInlineWordDiff);
    if (!diff || !SVNLineDiff::ShowInlineDiff(diff))
        return false;

    size_t lineoffset = 0;
    size_t position = 0;
    std::deque<size_t> removedPositions;
    while (diff)
    {
        apr_off_t len = diff->original_length;

        for (apr_off_t i = 0; i < len; ++i)
        {
            position += m_svnlinediff.m_line1tokens[lineoffset].size();
            lineoffset++;
        }

        if (diff->type == svn_diff__type_diff_modified)
        {
            inlineDiffPos p;
            if (lineoffset > 0)
                p.start = position - m_svnlinediff.m_line1tokens[lineoffset-1].size();
            else
                p.start = 0;
            p.end = position;
            positions.push_back(p);
        }

        diff = diff->next;
    }

    return (positions.size() > 0);
}

void CBaseView::OnNavigateNextinlinediff()
{
    std::vector<inlineDiffPos> positions;
    if (GetInlineDiffPositions(m_ptCaretPos.y, positions))
    {
        for (std::vector<inlineDiffPos>::iterator it = positions.begin(); it != positions.end(); ++it)
        {
            if (it->end > m_ptCaretPos.x)
            {
                m_ptCaretPos.x = (LONG)it->end;
                UpdateGoalPos();
                int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
                if (nCaretOffset < m_nOffsetChar)
                    ScrollAllToChar(nCaretOffset);
                if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
                    ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
                UpdateCaret();
                return;
            }
        }
        m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
        UpdateGoalPos();
        int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
        if (nCaretOffset < m_nOffsetChar)
            ScrollAllToChar(nCaretOffset);
        if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
            ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
    }
    UpdateCaret();
}

void CBaseView::OnNavigatePrevinlinediff()
{
    std::vector<inlineDiffPos> positions;
    if (GetInlineDiffPositions(m_ptCaretPos.y, positions))
    {
        for (std::vector<inlineDiffPos>::iterator it = positions.begin(); it != positions.end(); ++it)
        {
            if (it->start < m_ptCaretPos.x)
            {
                m_ptCaretPos.x = (LONG)it->start;
                UpdateGoalPos();
                int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
                if (nCaretOffset < m_nOffsetChar)
                    ScrollAllToChar(nCaretOffset);
                if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
                    ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
                UpdateCaret();
                return;
            }
        }
        m_ptCaretPos.x = 0;
        UpdateGoalPos();
        int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
        if (nCaretOffset < m_nOffsetChar)
            ScrollAllToChar(nCaretOffset);
        if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
            ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
    }
    UpdateCaret();
}

bool CBaseView::HasNextInlineDiff()
{
    std::vector<inlineDiffPos> positions;
    if (GetInlineDiffPositions(m_ptCaretPos.y, positions))
    {
        return true;
    }
    return false;
}

bool CBaseView::HasPrevInlineDiff()
{
    std::vector<inlineDiffPos> positions;
    if (GetInlineDiffPositions(m_ptCaretPos.y, positions))
    {
        return true;
    }
    return false;
}

void CBaseView::BuildAllScreen2ViewVector()
{
    if (IsLeftViewGood())
        m_pwndLeft->m_MultiLineVector.clear();
    if (IsRightViewGood())
        m_pwndRight->m_MultiLineVector.clear();
    if (IsBottomViewGood())
        m_pwndBottom->m_MultiLineVector.clear();

    if (IsLeftViewGood())
        m_pwndLeft->BuildScreen2ViewVector();
    if (IsRightViewGood())
        m_pwndRight->BuildScreen2ViewVector();
    if (IsBottomViewGood())
        m_pwndBottom->BuildScreen2ViewVector();
}

void CBaseView::BuildScreen2ViewVector()
{
    m_Screen2View.clear();
    if (m_pViewData)
    {
        m_Screen2View.reserve(m_pViewData->GetCount());
        for (int i = 0; i < m_pViewData->GetCount(); ++i)
        {
            if (m_pMainFrame->m_bCollapsed)
            {
                while ((i < m_pViewData->GetCount())&&(m_pViewData->GetHideState(i) == HIDESTATE_HIDDEN))
                    ++i;
            }
            if (i < m_pViewData->GetCount())
            {
                if (m_pMainFrame->m_bWrapLines)
                {
                    int nLinesLeft      = 0;
                    int nLinesRight     = 0;
                    int nLinesBottom    = 0;
                    if (IsLeftViewGood())
                        nLinesLeft = m_pwndLeft->CountMultiLines(i);
                    if (IsRightViewGood())
                        nLinesRight = m_pwndRight->CountMultiLines(i);
                    if (IsBottomViewGood())
                        nLinesBottom = m_pwndBottom->CountMultiLines(i);
                    int lines = max(max(nLinesLeft, nLinesRight), nLinesBottom);
                    for (int l = 0; l < (lines-1); ++l)
                        m_Screen2View.push_back(i);
                    m_nCachedWrappedLine = -1;
                }
                m_Screen2View.push_back(i);
            }
        }
    }
}

int CBaseView::FindScreenLineForViewLine( int viewLine )
{
    int ScreenLine = 0;
    for (std::vector<int>::const_iterator it = m_Screen2View.begin(); it != m_Screen2View.end(); ++it)
    {
        if (*it >= viewLine)
            return ScreenLine;
        ScreenLine++;
    }

    return ScreenLine;
}

CString CBaseView::GetMultiLine( int nLine )
{
    return CStringUtils::WordWrap(m_pViewData->GetLine(nLine), GetScreenChars()-1, false, true, GetTabSize());
}

int CBaseView::CountMultiLines( int nLine )
{
    if (nLine < (int)m_MultiLineVector.size())
        return m_MultiLineVector[nLine];

    CString multiline = GetMultiLine(nLine);
    // count the newlines
    int lines = 1;
    int pos = 0;
    while ((pos = multiline.Find('\n', pos)) >= 0)
    {
        pos++;
        lines++;
    }
    m_MultiLineVector.push_back(lines);
    ASSERT((int)m_MultiLineVector.size()-1 == nLine);

    return lines;
}