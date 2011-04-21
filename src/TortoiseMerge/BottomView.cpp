// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "Resource.h"
#include "AppUtils.h"

#include "bottomview.h"

IMPLEMENT_DYNCREATE(CBottomView, CBaseView)

CBottomView::CBottomView(void)
{
    m_pwndBottom = this;
    m_pState = &m_AllState.bottom;
    m_nStatusBarID = ID_INDICATOR_BOTTOMVIEW;
}

CBottomView::~CBottomView(void)
{
}


void CBottomView::AddContextItems(CMenu& popup, DiffStates state)
{
    const UINT uFlags = GetMenuFlags( state );

    CString temp;
    temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRBLOCK);
    popup.AppendMenu(uFlags, POPUPCOMMAND_USETHEIRBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURBLOCK);
    popup.AppendMenu(uFlags, POPUPCOMMAND_USEYOURBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
    popup.AppendMenu(uFlags, POPUPCOMMAND_USEYOURANDTHEIRBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
    popup.AppendMenu(uFlags, POPUPCOMMAND_USETHEIRANDYOURBLOCK, temp);

    CBaseView::AddContextItems(popup, state);
}


void CBottomView::CleanEmptyLines()
{
    bool bModified = false;
    for (int viewLine = 0; viewLine < GetViewCount(); )
    {
        DiffStates leftState = m_pwndLeft->GetViewState(viewLine);
        DiffStates rightState = m_pwndRight->GetViewState(viewLine);
        DiffStates bottomState = m_pwndBottom->GetViewState(viewLine);
        if (IsStateEmpty(leftState) && IsStateEmpty(rightState) && IsStateEmpty(bottomState))
        {
            m_pwndLeft->RemoveViewData(viewLine);
            m_pwndRight->RemoveViewData(viewLine);
            m_pwndBottom->RemoveViewData(viewLine);
            if (CUndo::GetInstance().IsGrouping()) // if use group undo -> ensure back adding goes in right (reversed) order
            {
                SaveUndoStep();
            }
            bModified = true;
            continue;
        }
        viewLine++;
    }
    if (bModified)
    {
        m_pwndBottom->SetModified();
        m_pwndLeft->SetModified();
        m_pwndRight->SetModified();
    }
}

void CBottomView::UseBothBlocks(CBaseView * pwndFirst, CBaseView * pwndLast)
{
    if (!HasSelection())
        return;

    int nFirstViewLine = m_Screen2View[m_nSelBlockStart]; // first view line in selection
    int nLastViewLine = m_Screen2View[m_nSelBlockEnd]; // last view line in selection
    int nNextViewLine = m_Screen2View.back() + 1; // first view line after selected block
    if (m_nSelBlockEnd + 1 < int(m_Screen2View.size()))
        nNextViewLine = m_Screen2View[m_nSelBlockEnd + 1];

    CUndo::GetInstance().BeginGrouping(); // start group undo

    // use (copy) first block
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewLine = m_Screen2View[i];
        viewdata lineData = pwndFirst->GetViewData(viewLine);
        lineData.ending = lineendings;
        lineData.state = ResolveState(lineData.state);
        SetViewData(viewLine, lineData);
        if (!IsStateEmpty(pwndFirst->GetViewState(viewLine)))
        {
            pwndFirst->SetViewState(viewLine, DIFFSTATE_YOURSADDED); // this is improper (may be DIFFSTATE_THEIRSADDED) but seems not to produce any visible bug
        }
    }
    SaveUndoStep();

    // use (insert) last block
    int viewIndex = nNextViewLine;
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++, viewIndex++)
    {
        int viewLine = m_Screen2View[i];
        viewdata lineData = pwndLast->GetViewData(viewLine);
        lineData.state = ResolveState(lineData.state);
        InsertViewData(viewIndex, lineData);
        if (!IsStateEmpty(pwndLast->GetViewState(viewLine)))
        {
            pwndLast->SetViewState(viewLine, DIFFSTATE_THEIRSADDED); // this is improper but seems not to produce any visible bug
        }
    }
    SaveUndoStep();

    // adjust line numbers in target
    // we fix all line numbers to handle exotic cases
    UpdateViewLineNumbers();
    SaveUndoStep();

    // now insert an empty block in both first and last
    int nCount = nLastViewLine - nFirstViewLine + 1;
    pwndLast->InsertViewEmptyLines(nFirstViewLine, nCount);
    pwndFirst->InsertViewEmptyLines(nNextViewLine, nCount);
    SaveUndoStep();

    CleanEmptyLines();
    SaveUndoStep();	

    CUndo::GetInstance().EndGrouping();

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    SetModified();
    pwndLast->SetModified();
    pwndFirst->SetModified();
    SaveUndoStep();
    RefreshViews();
}

// note :differs to UseViewFile in: EOL source and view range
void CBottomView::UseViewBlock(CBaseView * pwndView)
{
    if (!HasSelection())
        return;

    int nFirstViewLine = m_Screen2View[m_nSelBlockStart];
    int nLastViewLine = m_Screen2View[m_nSelBlockEnd];

    CUndo::GetInstance().BeginGrouping(); // start group undo

    for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
    {
        viewdata lineData = pwndView->GetViewData(viewLine);
        lineData.ending = lineendings;
        lineData.state = ResolveState(lineData.state);
        SetViewData(viewLine, lineData);
    }

    CleanEmptyLines();
    SaveUndoStep();	
    UpdateViewLineNumbers();
    SaveUndoStep();

    CUndo::GetInstance().EndGrouping();

    SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
    SaveUndoStep();
}

void CBottomView::UseViewFile(CBaseView * pwndView)
{
    int nFirstViewLine = 0;
    int nLastViewLine = GetViewCount()-1;

    CUndo::GetInstance().BeginGrouping(); // start group undo

    for (int viewLine = nFirstViewLine; viewLine <= nLastViewLine; viewLine++)
    {
        viewdata lineData = pwndView->GetViewData(viewLine);
        lineData.state = ResolveState(lineData.state);
        SetViewData(viewLine, lineData);
    }

    CleanEmptyLines();
    SaveUndoStep();	
    UpdateViewLineNumbers();
    SaveUndoStep();

    CUndo::GetInstance().EndGrouping();

    SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
    SaveUndoStep();
}
