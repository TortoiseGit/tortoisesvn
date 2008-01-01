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
		case DIFFSTATE_UNKNOWN:
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

		popup.AppendMenu(MF_SEPARATOR, NULL);

		temp.LoadString(IDS_EDIT_COPY);
		popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_COPY, temp);
		if (!m_bCaretHidden)
		{
			temp.LoadString(IDS_EDIT_CUT);
			popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_CUT, temp);
			temp.LoadString(IDS_EDIT_PASTE);
			popup.AppendMenu(MF_STRING | (CAppUtils::HasClipboardFormat(CF_UNICODETEXT)||CAppUtils::HasClipboardFormat(CF_TEXT) ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_PASTE, temp);
		}

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
	}
	return false;
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
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndLeft->m_pViewData->GetState(i));
		if (IsLineConflicted(i))
			m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
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
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndRight->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndRight->m_pViewData->GetState(i));
		if (IsLineConflicted(i))
			m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
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
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndLeft->m_pViewData->GetState(i));
		if (IsLineConflicted(i))
			m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
	}
	
	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pViewData->InsertData(index, m_pwndRight->m_pViewData->GetData(i));
		if (IsLineConflicted(index))
			m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pViewData->SetLineNumber(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		leftstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockStart, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		m_pwndRight->m_pViewData->InsertData(m_nSelBlockEnd+1, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);

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
		bottomstate.difflines[i] = m_pViewData->GetLine(i);
		m_pViewData->SetLine(i, m_pwndRight->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pViewData->GetState(i);
		m_pViewData->SetState(i, m_pwndRight->m_pViewData->GetState(i));
		rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
		m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_YOURSADDED);
		if (IsLineConflicted(i))
			m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
	}
	
	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pViewData->InsertData(index, m_pwndLeft->m_pViewData->GetData(i));
		leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
		m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_THEIRSADDED);
		if (IsLineConflicted(index))
			m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pViewData->SetLineNumber(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		leftstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockStart, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		m_pwndRight->m_pViewData->InsertData(m_nSelBlockEnd+1, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	RecalcAllVertScrollBars();
	SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
	RefreshViews();
}
