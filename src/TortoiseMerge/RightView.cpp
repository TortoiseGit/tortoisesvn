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
    m_nStatusBarID = ID_INDICATOR_RIGHTVIEW;
}

CRightView::~CRightView(void)
{
}

void CRightView::OnContextMenu(CPoint point, int /*nLine*/, DiffStates state)
{
    if (!this->IsWindowVisible())
        return;

    CMenu popup;
    if (!popup.CreatePopupMenu())
        return;

#define ID_USEBLOCK 1
#define ID_USEFILE 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
#define ID_USEBOTHTHISFIRST 5
#define ID_USEBOTHTHISLAST 6

    const UINT uFlags = GetMenuFlags( state );

    CString temp;

    if (m_pwndBottom->IsWindowVisible())
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
        popup.AppendMenu(uFlags, ID_USEBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
        popup.AppendMenu(uFlags, ID_USEYOURANDTHEIRBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
        popup.AppendMenu(uFlags, ID_USETHEIRANDYOURBLOCK, temp);
    }
    else
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERBLOCK);
        popup.AppendMenu(uFlags, ID_USEBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERFILE);
        popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISFIRST, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISLAST, temp);
    }

    AddCutCopyAndPaste(popup);

    CompensateForKeyboard(point);

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
    ResetUndoStep();
    switch (cmd)
    {
    case ID_EDIT_COPY:
        OnEditCopy();
        break;
    case ID_EDIT_CUT:
        OnEditCopy();
        RemoveSelectedText();
        break;
    case ID_EDIT_PASTE:
        PasteText();
        break;
    case ID_USEFILE:
        UseFile();
        return;
    case ID_USEBLOCK:
        UseBlock();
        return;
    case ID_USEYOURANDTHEIRBLOCK:
        UseYourAndTheirBlock();
        return;
    case ID_USETHEIRANDYOURBLOCK:
        UseTheirAndYourBlock();
        return;
    case ID_USEBOTHTHISFIRST:
        UseBothRightFirst();
        return;
    case ID_USEBOTHTHISLAST:
        UseBothLeftFirst();
        return;
    default:
        return;
    } // switch (cmd)
    SaveUndoStep();
    return;
}

void CRightView::UseFile(bool refreshViews /* = true */)
{
    if (m_pwndBottom->IsWindowVisible())
    {
        for (int i = 0; i < m_pwndRight->m_pViewData->GetCount(); i++)
        {
            m_AllState.bottom.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
            m_pwndBottom->m_pViewData->SetLine(i, m_pwndRight->m_pViewData->GetLine(i));
            m_AllState.bottom.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
            m_pwndBottom->m_pViewData->SetState(i, m_pwndRight->m_pViewData->GetState(i));
            m_pwndBottom->m_pViewData->SetLineEnding(i, m_pwndRight->m_pViewData->GetLineEnding(i));
            if (m_pwndBottom->IsViewLineConflicted(i))
            {
                if (m_pwndRight->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
                    m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
                else
                    m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
            }
        }
        m_pwndBottom->SetModified();
        BuildAllScreen2ViewVector();
        RecalcAllVertScrollBars();
    }
    else
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
        if (m_pwndLocator)
            m_pwndLocator->DocumentUpdated();
    }
    if (refreshViews)
        RefreshViews();
    SaveUndoStep();
}

void CRightView::UseBlock(bool refreshViews /* = true */)
{
    if (!HasSelection())
        return;

    if (m_pwndBottom->IsWindowVisible())
    {
        dynamic_cast<CBottomView*>(m_pwndBottom)->UseMyTextBlock(refreshViews);
        return;
    }
    else
    {
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
    }
    if (refreshViews)
        RefreshViews();
    SaveUndoStep();
}

