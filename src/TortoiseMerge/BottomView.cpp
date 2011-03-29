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
#include ".\bottomview.h"

IMPLEMENT_DYNCREATE(CBottomView, CBaseView)

CBottomView::CBottomView(void)
{
    m_pwndBottom = this;
    m_nStatusBarID = ID_INDICATOR_BOTTOMVIEW;
}

CBottomView::~CBottomView(void)
{
}

bool CBottomView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
    if (!this->IsWindowVisible())
        return false;

    CMenu popup;
    if (!popup.CreatePopupMenu())
        return false;

#define ID_USETHEIRBLOCK 1
#define ID_USEYOURBLOCK 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
    const UINT uFlags = GetMenuFlags( state );

    CString temp;
    temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRBLOCK);
    popup.AppendMenu(uFlags, ID_USETHEIRBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURBLOCK);
    popup.AppendMenu(uFlags, ID_USEYOURBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
    popup.AppendMenu(uFlags, ID_USEYOURANDTHEIRBLOCK, temp);
    temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
    popup.AppendMenu(uFlags, ID_USETHEIRANDYOURBLOCK, temp);

    AddCutCopyAndPaste(popup);

    CompensateForKeyboard(point);

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    switch (cmd)
    {
    case ID_USETHEIRBLOCK:
        UseTheirTextBlock();
        break;
    case ID_USEYOURBLOCK:
        UseMyTextBlock();
        break;
    case ID_USEYOURANDTHEIRBLOCK:
        UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    case ID_USETHEIRANDYOURBLOCK:
        UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    case ID_EDIT_COPY:
        OnEditCopy();
        return true;
    case ID_EDIT_CUT:
        OnEditCopy();
        RemoveSelectedText();
        break;
    case ID_EDIT_PASTE:
        PasteText();
        break;
    }
    return false;
}

void CBottomView::UseTheirTextBlock()
{
    if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
        return;

    viewstate leftstate;
    viewstate rightstate;
    viewstate bottomstate;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewLine = m_Screen2View[i];
        bottomstate.difflines[viewLine] = m_pViewData->GetLine(viewLine);
        m_pViewData->SetLine(viewLine, m_pwndLeft->m_pViewData->GetLine(viewLine));
        bottomstate.linestates[viewLine] = m_pViewData->GetState(viewLine);
        m_pViewData->SetState(viewLine, m_pwndLeft->m_pViewData->GetState(viewLine));
        m_pViewData->SetLineEnding(viewLine, lineendings);
        if (IsViewLineConflicted(viewLine))
        {
            if (m_pwndLeft->m_pViewData->GetState(viewLine) == DIFFSTATE_CONFLICTEMPTY)
                m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVED);
        }
    }
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
}

void CBottomView::UseMyTextBlock()
{
    if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
        return;

    viewstate leftstate;
    viewstate rightstate;
    viewstate bottomstate;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewLine = m_Screen2View[i];
        bottomstate.difflines[viewLine] = m_pViewData->GetLine(viewLine);
        m_pViewData->SetLine(viewLine, m_pwndRight->m_pViewData->GetLine(viewLine));
        bottomstate.linestates[viewLine] = m_pViewData->GetState(viewLine);
        m_pViewData->SetState(viewLine, m_pwndRight->m_pViewData->GetState(viewLine));
        m_pViewData->SetLineEnding(viewLine, lineendings);
        if (m_pwndRight->m_pViewData->GetState(viewLine) == DIFFSTATE_CONFLICTEMPTY)
            m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
        else
            m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVED);
    }
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
}

void CBottomView::UseTheirThenMyTextBlock()
{
    viewstate leftstate;
    viewstate rightstate;
    viewstate bottomstate;
    UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    RefreshViews();
}

void CBottomView::UseMyThenTheirTextBlock()
{
    viewstate leftstate;
    viewstate rightstate;
    viewstate bottomstate;
    UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    RefreshViews();
}