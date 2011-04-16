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

#include "RightView.h"
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
        m_pwndRight->SetLine(i, m_pwndLeft->GetLine(i));
        m_pwndRight->SetLineEnding(i, m_pwndLeft->GetLineEnding(i));
        DiffStates oldLeftState = m_pwndLeft->GetState(i);
        DiffStates newRightState = m_pwndRight->GetState(i);
        switch (oldLeftState)
        {
        case DIFFSTATE_CONFLICTEMPTY:
        case DIFFSTATE_UNKNOWN:
        case DIFFSTATE_EMPTY:
            newRightState = oldLeftState;
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
            newRightState = DIFFSTATE_NORMAL;
            m_pwndLeft->SetState(i, DIFFSTATE_NORMAL);
            break;
        default:
            break;
        }
        m_pwndRight->SetState(i, newRightState);
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
        m_pwndRight->SetLine(viewLine, m_pwndLeft->m_pViewData->GetLine(viewLine));
        m_pwndRight->SetLineEnding(viewLine, m_pwndRight->lineendings);
        DiffStates oldLeftState = m_pwndLeft->GetState(viewLine);
        DiffStates newRightState = m_pwndRight->GetState(viewLine);
        switch (oldLeftState)
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
            newRightState = oldLeftState;
            break;
        case DIFFSTATE_IDENTICALREMOVED:
        case DIFFSTATE_REMOVED:
        case DIFFSTATE_THEIRSREMOVED:
        case DIFFSTATE_YOURSREMOVED:
            newRightState = DIFFSTATE_ADDED;
            break;
        default:
            break;
        }
        m_pwndRight->SetState(viewLine, newRightState);
    }
    m_pwndRight->SetModified();
    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    if (m_pwndLocator)
        m_pwndLocator->DocumentUpdated();
    RefreshViews();
    SaveUndoStep();
}

void CRightView::UseBothRightFirst()
{
    if (!HasSelection())
        return;

    int viewIndexAfterSelection = m_Screen2View.back() + 1;
    if (m_nSelBlockEnd + 1 < int(m_Screen2View.size()))
        viewIndexAfterSelection = m_Screen2View[m_nSelBlockEnd + 1];

    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        m_AllState.right.linestates[i] = m_pwndRight->m_pViewData->GetState(viewline);
        m_pwndRight->m_pViewData->SetState(viewline, DIFFSTATE_YOURSADDED);
    }

    // your block is done, now insert their block
    int viewindex = viewIndexAfterSelection;
    for (int i = m_nSelBlockStart; i <= m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        m_AllState.right.addedlines.push_back(viewIndexAfterSelection);
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
        m_AllState.left.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
        m_pwndLeft->m_pViewData->InsertData(m_Screen2View[m_nSelBlockStart], _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
    SaveUndoStep();
    RefreshViews();
}

void CRightView::UseBothLeftFirst()
{
    if (!HasSelection())
        return;

    int viewIndexAfterSelection = m_Screen2View.back() + 1;
    if (m_nSelBlockEnd + 1 < int(m_Screen2View.size()))
        viewIndexAfterSelection = m_Screen2View[m_nSelBlockEnd + 1];

    // get line number from just before the block
    long linenumber = 0;
    if (m_nSelBlockStart > 0)
        linenumber = m_pwndRight->m_pViewData->GetLineNumber(m_nSelBlockStart-1);
    linenumber++;
    for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
    {
        int viewline = m_Screen2View[i];
        m_AllState.right.addedlines.push_back(m_Screen2View[m_nSelBlockStart]);
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
        m_AllState.left.addedlines.push_back(viewIndexAfterSelection);
        m_pwndLeft->m_pViewData->InsertData(viewIndexAfterSelection, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
    }

    BuildAllScreen2ViewVector();
    RecalcAllVertScrollBars();
    m_pwndLeft->SetModified();
    m_pwndRight->SetModified();
    SaveUndoStep();
    RefreshViews();
}
