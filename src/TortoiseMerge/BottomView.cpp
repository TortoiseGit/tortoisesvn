// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2007 - TortoiseSVN

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

void CBottomView::OnContextMenu(CPoint point, int /*nLine*/, CDiffData::DiffStates state)
{
	if (!this->IsWindowVisible())
		return;

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
#define ID_USETHEIRBLOCK 1
#define ID_USEYOURBLOCK 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
		UINT uEnabled = MF_ENABLED;
		if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
			uEnabled |= MF_DISABLED | MF_GRAYED;
		CString temp;

		bool bImportantBlock = true;
		switch (state)
		{
		case CDiffData::DIFFSTATE_UNKNOWN:
			bImportantBlock = false;
			break;
		}

		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USEYOURANDTHEIRBLOCK, temp);
		temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled | (bImportantBlock ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_USETHEIRANDYOURBLOCK, temp);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_USETHEIRBLOCK:
			UseTheirTextBlock();
			break;
		case ID_USEYOURBLOCK:
			UseMyTextBlock();
			break;
		case ID_USEYOURANDTHEIRBLOCK:
			UseMyThenTheirTextBlock();
			break;
		case ID_USETHEIRANDYOURBLOCK:
			UseTheirThenMyTextBlock();
			break;
		}
	}
}

void CBottomView::UseTheirTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_arDiffLines->GetAt(i);
		m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_arLineStates->GetAt(i);
		m_arLineStates->SetAt(i, m_pwndLeft->m_arLineStates->GetAt(i));
		if (IsLineConflicted(i))
			m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate);
	SetModified();
	RefreshViews();
}

void CBottomView::UseMyTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_arDiffLines->GetAt(i);
		m_arDiffLines->SetAt(i, m_pwndRight->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_arLineStates->GetAt(i);
		m_arLineStates->SetAt(i, m_pwndRight->m_arLineStates->GetAt(i));
		if (IsLineConflicted(i))
			m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate);
	SetModified();
	RefreshViews();
}

void CBottomView::UseTheirThenMyTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_arDiffLines->GetAt(i);
		m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_arLineStates->GetAt(i);
		m_arLineStates->SetAt(i, m_pwndLeft->m_arLineStates->GetAt(i));
		if (IsLineConflicted(i))
			m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
	}
	
	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_arDiffLines->InsertAt(index, m_pwndRight->m_arDiffLines->GetAt(i));
		m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
		m_arLineStates->InsertAt(index, m_pwndRight->m_arLineStates->GetAt(i));
		if (IsLineConflicted(index))
			m_arLineStates->SetAt(index, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockStart);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate);

	RecalcAllVertScrollBars();
	SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
	RefreshViews();
}

void CBottomView::UseMyThenTheirTextBlock()
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	if ((m_nSelBlockStart < 0)||(m_nSelBlockEnd < 0))
		return;

	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_arDiffLines->GetAt(i);
		m_arDiffLines->SetAt(i, m_pwndRight->m_arDiffLines->GetAt(i));
		bottomstate.linestates[i] = m_arLineStates->GetAt(i);
		m_arLineStates->SetAt(i, m_pwndRight->m_arLineStates->GetAt(i));
		rightstate.linestates[i] = m_pwndRight->m_arLineStates->GetAt(i);
		m_pwndRight->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_YOURSADDED);
		if (IsLineConflicted(i))
			m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
	}
	
	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_arDiffLines->InsertAt(index, m_pwndLeft->m_arDiffLines->GetAt(i));
		m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
		m_arLineStates->InsertAt(index, m_pwndLeft->m_arLineStates->GetAt(i));
		leftstate.linestates[i] = m_pwndLeft->m_arLineStates->GetAt(i);
		m_pwndLeft->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_THEIRSADDED);
		if (IsLineConflicted(index))
			m_arLineStates->SetAt(index, CDiffData::DIFFSTATE_CONFLICTRESOLVED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_arLineLines->GetAt(i);
		if (oldline >= 0)
			m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		leftstate.addedlines.push_back(m_nSelBlockStart);
	m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
	m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate);
	RecalcAllVertScrollBars();
	SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
	RefreshViews();
}
