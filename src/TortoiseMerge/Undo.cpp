// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2010-2011, 2013-2015, 2021-2022 - TortoiseSVN

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
#include "Undo.h"

#include "BaseView.h"

void ViewState::AddViewLineFromView(CBaseView* pView, int nViewLine, bool bAddEmptyLine)
{
    // is undo good place for this ?
    if (!pView || !pView->m_pViewData)
        return;
    replacedLines[nViewLine] = pView->m_pViewData->GetData(nViewLine);
    if (bAddEmptyLine)
    {
        addedLines.push_back(nViewLine + 1);
        pView->AddEmptyViewLine(nViewLine);
    }
}

void ViewState::Clear()
{
    diffLines.clear();
    lineStates.clear();
    linelines.clear();
    linesEOL.clear();
    markedLines.clear();
    addedLines.clear();

    removedLines.clear();
    replacedLines.clear();
    modifies = false;
}

void CUndo::MarkAsOriginalState(bool bLeft, bool bRight, bool bBottom)
{
    // TODO reduce code duplication
    // find highest index of changing step
    if (bLeft) // left is selected for mark
    {
        m_originalStateLeft                         = m_viewStates.size();
        std::list<AllViewState>::reverse_iterator i = m_viewStates.rbegin();
        while (i != m_viewStates.rend() && !i->left.modifies)
        {
            ++i;
            --m_originalStateLeft;
        }
    }
    if (bRight) // right is selected for mark
    {
        m_originalStateRight                        = m_viewStates.size();
        std::list<AllViewState>::reverse_iterator i = m_viewStates.rbegin();
        while (i != m_viewStates.rend() && !i->right.modifies)
        {
            ++i;
            --m_originalStateRight;
        }
    }
    if (bBottom) // bottom is selected for mark
    {
        m_originalStateBottom                       = m_viewStates.size();
        std::list<AllViewState>::reverse_iterator i = m_viewStates.rbegin();
        while (i != m_viewStates.rend() && !i->bottom.modifies)
        {
            ++i;
            --m_originalStateBottom;
        }
    }
}

CUndo& CUndo::GetInstance()
{
    static CUndo instance;
    return instance;
}

CUndo::CUndo()
{
    Clear();
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const AllViewState& allstate, POINT pt)
{
    m_viewStates.push_back(allstate);
    m_caretPoints.push_back(pt);
    // a new action that can be undone clears the redo since
    // after this there is nothing to redo anymore
    m_redoViewStates.clear();
    m_redoCaretPoints.clear();
    m_redoGroups.clear();
}

bool CUndo::Undo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom)
{
    if (!CanUndo())
        return false;

    if (m_groups.size() && m_groups.back() == m_caretPoints.size())
    {
        m_groups.pop_back();
        std::list<int>::size_type b = m_groups.back();
        m_redoGroups.push_back(b);
        m_redoGroups.push_back(m_caretPoints.size());
        m_groups.pop_back();
        while (b < m_caretPoints.size())
            UndoOne(pLeft, pRight, pBottom);
    }
    else
        UndoOne(pLeft, pRight, pBottom);

    CBaseView* pActiveView = nullptr;

    if (pBottom && pBottom->IsTarget())
    {
        pActiveView = pBottom;
    }
    else if (pRight && pRight->IsTarget())
    {
        pActiveView = pRight;
    }
    else
    //if (pLeft && pLeft->IsTarget())
    {
        pActiveView = pLeft;
    }

    if (pActiveView)
    {
        pActiveView->ClearSelection();
        pActiveView->BuildAllScreen2ViewVector();
        pActiveView->RecalcAllVertScrollBars();
        pActiveView->RecalcAllHorzScrollBars();
        pActiveView->EnsureCaretVisible();
        pActiveView->UpdateCaret();

        // TODO reduce code duplication
        if (m_viewStates.size() < m_originalStateLeft)
        {
            // Left can never get back to original state now
            m_originalStateLeft = static_cast<size_t>(-1);
        }
        if (pLeft)
        {
            bool bModified = (m_originalStateLeft == static_cast<size_t>(-1));
            if (!bModified)
            {
                std::list<AllViewState>::iterator i = m_viewStates.begin();
                std::advance(i, m_originalStateLeft);
                for (; i != m_viewStates.end(); ++i)
                {
                    if (i->left.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pLeft->SetModified(bModified);
            pLeft->ClearStepModifiedMark();
        }
        if (m_viewStates.size() < m_originalStateRight)
        {
            // Right can never get back to original state now
            m_originalStateRight = static_cast<size_t>(-1);
        }
        if (pRight)
        {
            bool bModified = (m_originalStateRight == static_cast<size_t>(-1));
            if (!bModified)
            {
                std::list<AllViewState>::iterator i = m_viewStates.begin();
                std::advance(i, m_originalStateRight);
                // ReSharper disable once CppPossiblyErroneousEmptyStatements
                for (; i != m_viewStates.end() && !i->right.modifies; ++i)
                    ;
                bModified = i != m_viewStates.end();
            }
            pRight->SetModified(bModified);
            pRight->ClearStepModifiedMark();
        }
        if (m_viewStates.size() < m_originalStateBottom)
        {
            // Bottom can never get back to original state now
            m_originalStateBottom = static_cast<size_t>(-1);
        }
        if (pBottom)
        {
            bool bModified = (m_originalStateBottom == static_cast<size_t>(-1));
            if (!bModified)
            {
                std::list<AllViewState>::iterator i = m_viewStates.begin();
                std::advance(i, m_originalStateBottom);
                for (; i != m_viewStates.end(); ++i)
                {
                    if (i->bottom.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pBottom->SetModified(bModified);
            pBottom->ClearStepModifiedMark();
        }
        pActiveView->RefreshViews();
    }

    return true;
}

void CUndo::UndoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom)
{
    AllViewState allstate = m_viewStates.back();
    POINT        pt       = m_caretPoints.back();

    if (pLeft->IsTarget())
        m_redoCaretPoints.push_back(pLeft->GetCaretPosition());
    else if (pRight->IsTarget())
        m_redoCaretPoints.push_back(pRight->GetCaretPosition());
    else if (pBottom->IsTarget())
        m_redoCaretPoints.push_back(pBottom->GetCaretPosition());

    allstate.left   = Do(allstate.left, pLeft, pt);
    allstate.right  = Do(allstate.right, pRight, pt);
    allstate.bottom = Do(allstate.bottom, pBottom, pt);

    m_redoViewStates.push_back(allstate);

    m_viewStates.pop_back();
    m_caretPoints.pop_back();
}

bool CUndo::Redo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom)
{
    if (!CanRedo())
        return false;

    if (m_redoGroups.size() && m_redoGroups.back() == m_redoCaretPoints.size())
    {
        m_redoGroups.pop_back();
        std::list<int>::size_type b = m_redoGroups.back();
        m_groups.push_back(b);
        m_groups.push_back(m_redoCaretPoints.size());
        m_redoGroups.pop_back();
        while (b < m_redoCaretPoints.size())
            RedoOne(pLeft, pRight, pBottom);
    }
    else
        RedoOne(pLeft, pRight, pBottom);

    CBaseView* pActiveView = nullptr;

    if (pBottom && pBottom->IsTarget())
    {
        pActiveView = pBottom;
    }
    else if (pRight && pRight->IsTarget())
    {
        pActiveView = pRight;
    }
    else
    //if (pLeft && pLeft->IsTarget())
    {
        pActiveView = pLeft;
    }

    if (pActiveView)
    {
        pActiveView->ClearSelection();
        pActiveView->BuildAllScreen2ViewVector();
        pActiveView->RecalcAllVertScrollBars();
        pActiveView->RecalcAllHorzScrollBars();
        pActiveView->EnsureCaretVisible();
        pActiveView->UpdateCaret();

        // TODO reduce code duplication
        if (m_redoViewStates.size() < m_originalStateLeft)
        {
            // Left can never get back to original state now
            m_originalStateLeft = static_cast<size_t>(-1);
        }
        if (pLeft)
        {
            bool bModified = (m_originalStateLeft != static_cast<size_t>(-1));
            if (!bModified && !m_redoViewStates.empty())
            {
                std::list<AllViewState>::iterator i = m_redoViewStates.begin();
                std::advance(i, m_originalStateLeft);
                for (; i != m_redoViewStates.end(); ++i)
                {
                    if (!i->left.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pLeft->SetModified(bModified);
            pLeft->ClearStepModifiedMark();
        }
        if (m_redoViewStates.size() < m_originalStateRight)
        {
            // Right can never get back to original state now
            m_originalStateRight = static_cast<size_t>(-1);
        }
        if (pRight)
        {
            bool bModified = (m_originalStateRight != static_cast<size_t>(-1));
            if (!bModified && !m_redoViewStates.empty())
            {
                std::list<AllViewState>::iterator i = m_redoViewStates.begin();
                std::advance(i, m_originalStateRight);
                for (; i != m_redoViewStates.end(); ++i)
                {
                    if (!i->bottom.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pRight->SetModified(bModified);
            pRight->ClearStepModifiedMark();
        }
        if (m_redoViewStates.size() < m_originalStateBottom)
        {
            // Bottom can never get back to original state now
            m_originalStateBottom = static_cast<size_t>(-1);
        }
        if (pBottom)
        {
            bool bModified = (m_originalStateBottom != static_cast<size_t>(-1));
            if (!bModified)
            {
                std::list<AllViewState>::iterator i = m_redoViewStates.begin();
                std::advance(i, m_originalStateBottom);
                for (; i != m_redoViewStates.end(); ++i)
                {
                    if (!i->bottom.modifies)
                    {
                        bModified = true;
                        break;
                    }
                }
            }
            pBottom->SetModified(bModified);
            pBottom->ClearStepModifiedMark();
        }
        pActiveView->RefreshViews();
    }

    return true;
}

void CUndo::RedoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom)
{
    AllViewState allstate = m_redoViewStates.back();
    POINT        pt       = m_redoCaretPoints.back();

    if (pLeft->IsTarget())
        m_caretPoints.push_back(pLeft->GetCaretPosition());
    else if (pRight->IsTarget())
        m_caretPoints.push_back(pRight->GetCaretPosition());
    else if (pBottom->IsTarget())
        m_caretPoints.push_back(pBottom->GetCaretPosition());

    allstate.left   = Do(allstate.left, pLeft, pt);
    allstate.right  = Do(allstate.right, pRight, pt);
    allstate.bottom = Do(allstate.bottom, pBottom, pt);

    m_viewStates.push_back(allstate);

    m_redoViewStates.pop_back();
    m_redoCaretPoints.pop_back();
}
ViewState CUndo::Do(const ViewState& state, CBaseView* pView, const POINT& pt)
{
    if (!pView)
        return state;

    CViewData* viewData = pView->m_pViewData;
    if (!viewData)
        return state;

    ViewState revState; // the reversed ViewState

    for (auto it = state.addedLines.rbegin(); it != state.addedLines.rend(); ++it)
    {
        revState.removedLines[*it] = viewData->GetData(*it);
        viewData->RemoveData(*it);
    }
    for (auto it = state.linelines.begin(); it != state.linelines.end(); ++it)
    {
        revState.linelines[it->first] = viewData->GetLineNumber(it->first);
        viewData->SetLineNumber(it->first, it->second);
    }
    for (auto it = state.lineStates.begin(); it != state.lineStates.end(); ++it)
    {
        revState.lineStates[it->first] = viewData->GetState(it->first);
        viewData->SetState(it->first, static_cast<DiffStates>(it->second));
    }
    for (auto it = state.linesEOL.begin(); it != state.linesEOL.end(); ++it)
    {
        revState.linesEOL[it->first] = viewData->GetLineEnding(it->first);
        viewData->SetLineEnding(it->first, it->second);
    }
    for (auto it = state.markedLines.begin(); it != state.markedLines.end(); ++it)
    {
        revState.markedLines[it->first] = viewData->GetMarked(it->first);
        viewData->SetMarked(it->first, it->second);
    }
    for (auto it = state.diffLines.begin(); it != state.diffLines.end(); ++it)
    {
        revState.diffLines[it->first] = viewData->GetLine(it->first);
        viewData->SetLine(it->first, it->second);
    }
    for (auto it = state.removedLines.begin(); it != state.removedLines.end(); ++it)
    {
        revState.addedLines.push_back(it->first);
        viewData->InsertData(it->first, it->second);
    }
    for (auto it = state.replacedLines.begin(); it != state.replacedLines.end(); ++it)
    {
        revState.replacedLines[it->first] = viewData->GetData(it->first);
        viewData->SetData(it->first, it->second);
    }

    if (pView->IsTarget())
    {
        pView->SetCaretViewPosition(pt);
        pView->EnsureCaretVisible();
    }
    return revState;
}

void CUndo::Clear()
{
    m_viewStates.clear();
    m_caretPoints.clear();
    m_groups.clear();
    m_redoViewStates.clear();
    m_redoCaretPoints.clear();
    m_redoGroups.clear();
    m_originalStateLeft   = 0;
    m_originalStateRight  = 0;
    m_originalStateBottom = 0;
    m_groupCount          = 0;
}
