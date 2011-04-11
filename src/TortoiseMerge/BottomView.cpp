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

void CBottomView::UseLeftBlock()
{
    if (!HasSelection())
        return;

     UseViewBlock(m_pwndLeft);
}

void CBottomView::UseLeftFile()
{
     UseViewFile(m_pwndLeft);
}

void CBottomView::UseRightBlock()
{
    if (!HasSelection())
        return;

    UseViewBlock(m_pwndRight);
}

void CBottomView::UseRightFile()
{
    UseViewFile(m_pwndRight);
}

void CBottomView::UseViewFile(CBaseView * pwndView)
{
    for (int i = 0; i < pwndView->m_pViewData->GetCount(); i++)
    {
        int viewLine = i;
        m_AllState.bottom.difflines[viewLine] = m_pwndBottom->m_pViewData->GetLine(viewLine);
        m_pwndBottom->m_pViewData->SetLine(viewLine, pwndView->m_pViewData->GetLine(viewLine));
        m_AllState.bottom.linestates[viewLine] = m_pwndBottom->m_pViewData->GetState(viewLine);
        m_pwndBottom->m_pViewData->SetState(viewLine, pwndView->m_pViewData->GetState(viewLine));
        m_pwndBottom->m_pViewData->SetLineEnding(viewLine, pwndView->m_pViewData->GetLineEnding(viewLine));
        if (m_pwndBottom->IsViewLineConflicted(viewLine))
        {
            if (pwndView->m_pViewData->GetState(viewLine) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVED);
        }
    }
    m_pwndBottom->SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
    SaveUndoStep();
}


void CBottomView::UseViewBlock(CBaseView * pwndView)
{
    bool bView = true;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewLine = bView ? m_Screen2View[i] : i;
        m_AllState.bottom.difflines[viewLine] = m_pwndBottom->m_pViewData->GetLine(viewLine);
        m_pwndBottom->m_pViewData->SetLine(viewLine, pwndView->m_pViewData->GetLine(viewLine));
        m_AllState.bottom.linestates[viewLine] = m_pwndBottom->m_pViewData->GetState(viewLine);
        m_pwndBottom->m_pViewData->SetState(viewLine, pwndView->m_pViewData->GetState(viewLine));
        m_pwndBottom->m_pViewData->SetLineEnding(viewLine, m_pwndBottom->lineendings);
        if (m_pwndBottom->IsViewLineConflicted(viewLine))
        {
            if (pwndView->m_pViewData->GetState(viewLine) == DIFFSTATE_CONFLICTEMPTY)
                m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
            else
                m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVED);
        }
    }
    m_pwndBottom->SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    RefreshViews();
    SaveUndoStep();
}
