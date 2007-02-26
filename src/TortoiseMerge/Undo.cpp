// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "Undo.h"

#include "BaseView.h"

CUndo& CUndo::GetInstance()
{
	static CUndo instance;
	return instance;
}

CUndo::CUndo()
{
}

CUndo::~CUndo()
{
}

void CUndo::AddState(const viewstate& leftstate, const viewstate& rightstate, const viewstate& bottomstate)
{
	m_viewstates.push_back(bottomstate);
	m_viewstates.push_back(rightstate);
	m_viewstates.push_back(leftstate);
}

bool CUndo::Undo(CBaseView * pLeft, CBaseView * pRight, CBaseView * pBottom)
{
	if (m_viewstates.size() > 0)
	{
		viewstate state = m_viewstates.back();
		Undo(state, pLeft);
		m_viewstates.pop_back();
		state = m_viewstates.back();
		Undo(state, pRight);
		m_viewstates.pop_back();
		state = m_viewstates.back();
		Undo(state, pBottom);
		m_viewstates.pop_back();

		return true;
	}
	return false;
}

void CUndo::Undo(const viewstate& state, CBaseView * pView)
{
	if (pView)
	{
		int nLineStart = INT_MAX;
		int nLineEnd = -1;
		for (std::list<int>::const_iterator it = state.addedlines.begin(); it != state.addedlines.end(); ++it)
		{
			if (pView->m_arDiffLines)
				pView->m_arDiffLines->RemoveAt(*it);
			if (pView->m_arLineLines)
				pView->m_arLineLines->RemoveAt(*it);
			if (pView->m_arLineStates)
				pView->m_arLineStates->RemoveAt(*it);
		}
		for (std::map<int, DWORD>::const_iterator it = state.linelines.begin(); it != state.linelines.end(); ++it)
		{
			if (pView->m_arLineLines)
			{
				nLineStart = min(nLineStart, it->first);
				nLineEnd = max(nLineEnd, it->first);
				pView->m_arLineLines->SetAt(it->first, it->second);
			}
		}
		for (std::map<int, DWORD>::const_iterator it = state.linestates.begin(); it != state.linestates.end(); ++it)
		{
			if (pView->m_arLineStates)
			{
				nLineStart = min(nLineStart, it->first);
				nLineEnd = max(nLineEnd, it->first);
				pView->m_arLineStates->SetAt(it->first, it->second);
			}
		}
		for (std::map<int, CString>::const_iterator it = state.difflines.begin(); it != state.difflines.end(); ++it)
		{
			if (pView->m_arDiffLines)
			{
				nLineStart = min(nLineStart, it->first);
				nLineEnd = max(nLineEnd, it->first);
				pView->m_arDiffLines->SetAt(it->first, it->second);
			}
		}
		pView->DocumentUpdated();
		pView->SetModified();
		if (nLineEnd >= 0)
		{
			pView->GoToLine(nLineEnd, true);
			pView->SelectLines(nLineStart, nLineEnd);
		}
	}
}