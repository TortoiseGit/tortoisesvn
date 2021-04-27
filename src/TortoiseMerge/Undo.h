// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2009-2015, 2021 - TortoiseSVN

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
#pragma once
#include "ViewData.h"
#include <map>
#include <list>

class CBaseView;

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge.
 */
class ViewState
{
public:
    ViewState()
        : modifies(false)
    {
    }

    std::map<int, CString> diffLines;
    std::map<int, DWORD>   lineStates;
    std::map<int, DWORD>   linelines;
    std::map<int, EOL>     linesEOL;
    std::map<int, bool>    markedLines;
    std::list<int>         addedLines;

    std::map<int, ViewData> removedLines;
    std::map<int, ViewData> replacedLines;
    bool                    modifies; ///< this step modifies view (save before and after save differs)

    void AddViewLineFromView(CBaseView* pView, int nViewLine, bool bAddEmptyLine);
    void Clear();
    bool IsEmpty() const { return diffLines.empty() && lineStates.empty() && linelines.empty() && linesEOL.empty() && markedLines.empty() && addedLines.empty() && removedLines.empty() && replacedLines.empty(); }
};

/**
 * \ingroup TortoiseMerge
 * this struct holds all the information of a single change in TortoiseMerge for all(3) views.
 */
struct AllViewState
{
    ViewState right;
    ViewState bottom;
    ViewState left;

    void Clear()
    {
        right.Clear();
        bottom.Clear();
        left.Clear();
    }
    bool IsEmpty() const { return right.IsEmpty() && bottom.IsEmpty() && left.IsEmpty(); }
};

/**
 * \ingroup TortoiseMerge
 * Holds all the information of previous changes made to a view content.
 * Of course, can undo those changes.
 */
class CUndo
{
public:
    static CUndo& GetInstance();

    bool Undo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    bool Redo(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    void AddState(const AllViewState& allstate, POINT pt);
    bool CanUndo() const { return !m_viewStates.empty(); }
    bool CanRedo() const { return !m_redoViewStates.empty(); }

    bool IsGrouping() const { return m_groups.size() % 2 == 1; }
    bool IsRedoGrouping() const { return m_redoGroups.size() % 2 == 1; }
    void BeginGrouping()
    {
        if (m_groupCount == 0)
            m_groups.push_back(m_caretPoints.size());
        m_groupCount++;
    }
    void EndGrouping()
    {
        m_groupCount--;
        if (m_groupCount == 0)
            m_groups.push_back(m_caretPoints.size());
    }
    void Clear();
    void MarkAllAsOriginalState() { MarkAsOriginalState(true, true, true); }
    void MarkAsOriginalState(bool left, bool right, bool bottom);

protected:
    static ViewState                     Do(const ViewState& state, CBaseView* pView, const POINT& pt);
    void                                 UndoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    void                                 RedoOne(CBaseView* pLeft, CBaseView* pRight, CBaseView* pBottom);
    std::list<AllViewState>              m_viewStates;
    std::list<POINT>                     m_caretPoints;
    std::list<std::list<int>::size_type> m_groups;
    size_t                               m_originalStateLeft;
    size_t                               m_originalStateRight;
    size_t                               m_originalStateBottom;
    int                                  m_groupCount;

    std::list<AllViewState>              m_redoViewStates;
    std::list<POINT>                     m_redoCaretPoints;
    std::list<std::list<int>::size_type> m_redoGroups;

private:
    CUndo();
    ~CUndo();
};
