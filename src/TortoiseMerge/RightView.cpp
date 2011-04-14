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

#include "rightview.h"
#include "BottomView.h"

IMPLEMENT_DYNCREATE(CRightView, CBaseView)

CRightView::CRightView(void)
{
    m_pwndRight = this;
    m_pState = &m_AllState.right;
    m_nStatusBarID = ID_INDICATOR_RIGHTVIEW;
}

CRightView::~CRightView(void)
{
}

void CRightView::AddContextItems(CMenu& popup, DiffStates state)
{
    const UINT uFlags = GetMenuFlags( state );

    CString temp;

    if (m_pwndBottom->IsWindowVisible())
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USEYOURBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, POPUPCOMMAND_USEYOURFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USEYOURANDTHEIRBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USETHEIRANDYOURBLOCK, temp);
    }
    else
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERBLOCK);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USELEFTBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, POPUPCOMMAND_USELEFTFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USEBOTHRIGHTFIRST, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
        popup.AppendMenu(uFlags, POPUPCOMMAND_USEBOTHLEFTFIRST, temp);
    }

    CBaseView::AddContextItems(popup, state);
}

void CRightView::UseLeftFile()
{
    for (int i = 0; i < m_pwndRight->m_pViewData->GetCount(); i++)
    {
        m_AllState.right.difflines[i] = m_pwndRight->m_pViewData->GetLine(i);
        m_pwndRight->m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
        m_pwndRight->m_pViewData->SetLineEnding(i, m_pwndLeft->m_pViewData->GetLineEnding(i));
        DiffStates state = m_pwndLeft->m_pViewData->GetState(i);
        switch (state)
        {
        case DIFFSTATE_CONFLICTEMPTY:
        case DIFFSTATE_UNKNOWN:
        case DIFFSTATE_EMPTY:
            m_AllState.right.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
            m_pwndRight->m_pViewData->SetState(i, state);
            break;
        case DIFFSTATE_YOURSADDED:
        case DIFFSTATE_IDENTICALADDED:
        case DIFFSTATE_NORMAL:
        case DIFFSTATE_THEIRSADDED:
        case DIFFSTATE_ADDED:
        case DIFFSTATE_MOVED_TO:
        case DIFFSTATE_MOVED_FROM:
        case DIFFSTATE_CONFLICTADDED:
        case DIFFSTATE_CONFLICTED:
        case DIFFSTATE_CONFLICTED_IGNORED:
        case DIFFSTATE_IDENTICALREMOVED:
        case DIFFSTATE_REMOVED:
        case DIFFSTATE_THEIRSREMOVED:
        case DIFFSTATE_YOURSREMOVED:
            m_AllState.right.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
            m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_NORMAL);
            m_AllState.left.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
            m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_NORMAL);
            break;
        default:
            break;
        }
    }
    m_pwndRight->SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    if (m_pwndLocator)
        m_pwndLocator->DocumentUpdated();
    RefreshViews();
    SaveUndoStep();
}

void CRightView::UseLeftBlock()
{
    if (!HasSelection())
        return;

    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
         int viewLine = m_Screen2View[i];
        m_AllState.right.difflines[viewLine] = m_pwndRight->m_pViewData->GetLine(viewLine);
        m_pwndRight->m_pViewData->SetLine(viewLine, m_pwndLeft->m_pViewData->GetLine(viewLine));
        m_pwndRight->m_pViewData->SetLineEnding(viewLine, m_pwndRight->lineendings);
        DiffStates state = m_pwndLeft->m_pViewData->GetState(viewLine);
        switch (state)
        {
        case DIFFSTATE_ADDED:
        case DIFFSTATE_MOVED_TO:
        case DIFFSTATE_MOVED_FROM:
        case DIFFSTATE_CONFLICTADDED:
        case DIFFSTATE_CONFLICTED:
        case DIFFSTATE_CONFLICTED_IGNORED:
        case DIFFSTATE_CONFLICTEMPTY:
        case DIFFSTATE_IDENTICALADDED:
        case DIFFSTATE_NORMAL:
        case DIFFSTATE_THEIRSADDED:
        case DIFFSTATE_UNKNOWN:
        case DIFFSTATE_YOURSADDED:
        case DIFFSTATE_EMPTY:
            m_AllState.right.linestates[viewLine] = m_pwndRight->m_pViewData->GetState(viewLine);
            m_pwndRight->m_pViewData->SetState(viewLine, state);
            break;
        case DIFFSTATE_IDENTICALREMOVED:
        case DIFFSTATE_REMOVED:
        case DIFFSTATE_THEIRSREMOVED:
        case DIFFSTATE_YOURSREMOVED:
            m_AllState.right.linestates[viewLine] = m_pwndRight->m_pViewData->GetState(viewLine);
            m_pwndRight->m_pViewData->SetState(viewLine, DIFFSTATE_ADDED);
            break;
        default:
            break;
        }
    }
    m_pwndRight->SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    if (m_pwndLocator)
        m_pwndLocator->DocumentUpdated();
    RefreshViews();
    SaveUndoStep();
}
