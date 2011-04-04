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
#include ".\rightview.h"

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
    }
    else
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERBLOCK);
    popup.AppendMenu(uFlags, ID_USEBLOCK, temp);

    if (m_pwndBottom->IsWindowVisible())
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
    }
    else
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERFILE);
    popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);

    if (m_pwndBottom->IsWindowVisible())
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
        popup.AppendMenu(uFlags, ID_USEYOURANDTHEIRBLOCK, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
        popup.AppendMenu(uFlags, ID_USETHEIRANDYOURBLOCK, temp);
    }
    else
    {
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISFIRST, temp);
        temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
        popup.AppendMenu(uFlags, ID_USEBOTHTHISLAST, temp);
    }

    AddCutCopyAndPaste(popup);

    CompensateForKeyboard(point);

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    switch (cmd)
    {
    case ID_EDIT_COPY:
        OnEditCopy();
        return;
    case ID_EDIT_CUT:
        OnEditCopy();
        RemoveSelectedText();
        return;
    case ID_EDIT_PASTE:
        PasteText();
        return;
    case ID_USEFILE:
        UseFile(false);
        break;
    case ID_USEBLOCK:
        UseBlock(false);
        break;
    case ID_USEYOURANDTHEIRBLOCK:
        UseYourAndTheirBlock(rightstate, bottomstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    case ID_USETHEIRANDYOURBLOCK:
        UseTheirAndYourBlock(rightstate, bottomstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    case ID_USEBOTHTHISFIRST:
        UseBothRightFirst(rightstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    case ID_USEBOTHTHISLAST:
        UseBothLeftFirst(rightstate, leftstate);
        CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
        break;
    default:
        return;
    } // switch (cmd)
    return;
}

void CRightView::UseFile(bool refreshViews /* = true */)
{
    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    if (m_pwndBottom->IsWindowVisible())
    {
        for (int i = 0; i < m_pViewData->GetCount(); i++)
        {
            bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
            m_pwndBottom->m_pViewData->SetLine(i, m_pViewData->GetLine(i));
            bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
            m_pwndBottom->m_pViewData->SetState(i, m_pViewData->GetState(i));
            m_pwndBottom->m_pViewData->SetLineEnding(i, m_pViewData->GetLineEnding(i));
            if (m_pwndBottom->IsViewLineConflicted(i))
                m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
        }
        m_pwndBottom->SetModified();
    }
    else
    {
        for (int i = 0; i < m_pViewData->GetCount(); i++)
        {
            rightstate.difflines[i] = m_pViewData->GetLine(i);
            m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
            m_pViewData->SetLineEnding(i, m_pwndLeft->m_pViewData->GetLineEnding(i));
            DiffStates state = m_pwndLeft->m_pViewData->GetState(i);
            switch (state)
            {
            case DIFFSTATE_CONFLICTEMPTY:
            case DIFFSTATE_UNKNOWN:
            case DIFFSTATE_EMPTY:
                rightstate.linestates[i] = m_pViewData->GetState(i);
                m_pViewData->SetState(i, state);
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
                rightstate.linestates[i] = m_pViewData->GetState(i);
                m_pViewData->SetState(i, DIFFSTATE_NORMAL);
                leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
                m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_NORMAL);
                break;
            default:
                break;
            }
            SetModified();
            if (m_pwndLocator)
                m_pwndLocator->DocumentUpdated();
        }
    }
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    if (refreshViews)
        RefreshViews();
}

void CRightView::UseBlock(bool refreshViews /* = true */)
{
    if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
        return;

    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    if (m_pwndBottom->IsWindowVisible())
    {
        for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
        {
            int viewLine = m_Screen2View[i];
            bottomstate.difflines[viewLine] = m_pwndBottom->m_pViewData->GetLine(viewLine);
            m_pwndBottom->m_pViewData->SetLine(viewLine, m_pViewData->GetLine(viewLine));
            bottomstate.linestates[viewLine] = m_pwndBottom->m_pViewData->GetState(viewLine);
            m_pwndBottom->m_pViewData->SetState(viewLine, m_pViewData->GetState(viewLine));
            m_pwndBottom->m_pViewData->SetLineEnding(viewLine, m_pwndBottom->lineendings);
            if (m_pwndBottom->IsViewLineConflicted(viewLine))
            {
                if (m_pViewData->GetState(viewLine) == DIFFSTATE_CONFLICTEMPTY)
                    m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
                else
                    m_pwndBottom->m_pViewData->SetState(viewLine, DIFFSTATE_CONFLICTRESOLVED);
            }
        }
        m_pwndBottom->SetModified();
        BuildAllScreen2ViewVector();
        RecalcAllVertScrollBars();
    }
    else
    {
        for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
        {
            int viewLine = m_Screen2View[i];
            rightstate.difflines[viewLine] = m_pViewData->GetLine(viewLine);
            m_pViewData->SetLine(viewLine, m_pwndLeft->m_pViewData->GetLine(viewLine));
            m_pViewData->SetLineEnding(viewLine, lineendings);
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
                rightstate.linestates[viewLine] = m_pViewData->GetState(viewLine);
                m_pViewData->SetState(viewLine, state);
                break;
            case DIFFSTATE_IDENTICALREMOVED:
            case DIFFSTATE_REMOVED:
            case DIFFSTATE_THEIRSREMOVED:
            case DIFFSTATE_YOURSREMOVED:
                rightstate.linestates[viewLine] = m_pViewData->GetState(viewLine);
                m_pViewData->SetState(viewLine, DIFFSTATE_ADDED);
                break;
            default:
                break;
            }
        }
        SetModified();
        BuildAllScreen2ViewVector();
        RecalcAllVertScrollBars();
    }
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    if (refreshViews)
        RefreshViews();
}

void CRightView::UseLeftBeforeRight(bool refreshViews /* = true */)
{
    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    UseBothLeftFirst(rightstate, leftstate);
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    if (refreshViews)
        RefreshViews();
}

void CRightView::UseRightBeforeLeft(bool refreshViews /* = true */)
{
    viewstate rightstate;
    viewstate bottomstate;
    viewstate leftstate;
    UseBothRightFirst(rightstate, leftstate);
    CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
    if (refreshViews)
        RefreshViews();
}