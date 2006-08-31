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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "StdAfx.h"
#include "Resource.h"
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

BOOL CRightView::ShallShowContextMenu(CDiffData::DiffStates state, int /*nLine*/)
{
	//The right view is not visible in one-way diff...
	if (!this->IsWindowVisible())
	{
		return FALSE;
	}

	//The right view is always "Yours" in both two and three-way diff
	switch (state)
	{
	case CDiffData::DIFFSTATE_ADDED:
	case CDiffData::DIFFSTATE_REMOVED:
	case CDiffData::DIFFSTATE_CONFLICTED:
	case CDiffData::DIFFSTATE_CONFLICTEMPTY:
	case CDiffData::DIFFSTATE_CONFLICTADDED:
	case CDiffData::DIFFSTATE_EMPTY:
		return TRUE;
	default:
		return FALSE;
	} // switch (state) 
	//return FALSE;
}

void CRightView::OnContextMenu(CPoint point, int /*nLine*/)
{
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
#define ID_USEBLOCK 1
#define ID_USEFILE 2
#define ID_USETHEIRANDYOURBLOCK 3
#define ID_USEYOURANDTHEIRBLOCK 4
#define ID_USEBOTHTHISFIRST 5
#define ID_USEBOTHTHISLAST 6

		UINT uEnabled = MF_ENABLED;
		if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
			uEnabled |= MF_DISABLED | MF_GRAYED;
		CString temp;
		if (!m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERBLOCK);
		}
		else
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISBLOCK);
		popup.AppendMenu(MF_STRING | uEnabled, ID_USEBLOCK, temp);

		if (!m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEOTHERFILE);
		}
		else
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHISFILE);
		popup.AppendMenu(MF_STRING | MF_ENABLED, ID_USEFILE, temp);

		if (m_pwndBottom->IsWindowVisible())
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEYOURANDTHEIRBLOCK);
			popup.AppendMenu(MF_STRING | uEnabled, ID_USEYOURANDTHEIRBLOCK, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USETHEIRANDYOURBLOCK);
			popup.AppendMenu(MF_STRING | uEnabled, ID_USETHEIRANDYOURBLOCK, temp);
		}
		else
		{
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISFIRST);
			popup.AppendMenu(MF_STRING | uEnabled, ID_USEBOTHTHISFIRST, temp);
			temp.LoadString(IDS_VIEWCONTEXTMENU_USEBOTHTHISLAST);
			popup.AppendMenu(MF_STRING | uEnabled, ID_USEBOTHTHISLAST, temp);
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
		switch (cmd)
		{
		case ID_USEFILE:
			{
				if (m_pwndBottom->IsWindowVisible())
				{
					for (int i=0; i<GetLineCount(); i++)
					{
						m_pwndBottom->m_arDiffLines->SetAt(i, m_arDiffLines->GetAt(i));
						m_pwndBottom->m_arLineStates->SetAt(i, m_arLineStates->GetAt(i));
					}
					m_pwndBottom->SetModified();
				}
				else
				{
					for (int i=0; i<GetLineCount(); i++)
					{
						m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
						CDiffData::DiffStates state = (CDiffData::DiffStates)m_pwndLeft->m_arLineStates->GetAt(i);
						switch (state)
						{
						case CDiffData::DIFFSTATE_ADDED:
						case CDiffData::DIFFSTATE_CONFLICTADDED:
						case CDiffData::DIFFSTATE_CONFLICTED:
						case CDiffData::DIFFSTATE_CONFLICTEMPTY:
						case CDiffData::DIFFSTATE_IDENTICALADDED:
						case CDiffData::DIFFSTATE_NORMAL:
						case CDiffData::DIFFSTATE_THEIRSADDED:
						case CDiffData::DIFFSTATE_UNKNOWN:
						case CDiffData::DIFFSTATE_YOURSADDED:
						case CDiffData::DIFFSTATE_EMPTY:
							m_arLineStates->SetAt(i, state);
							break;
						case CDiffData::DIFFSTATE_IDENTICALREMOVED:
						case CDiffData::DIFFSTATE_REMOVED:
						case CDiffData::DIFFSTATE_THEIRSREMOVED:
						case CDiffData::DIFFSTATE_YOURSREMOVED:
							break;
						default:
							break;
						}
						SetModified();
					}
				}
			} 
			break;
		case ID_USEBLOCK:
			{
				if (m_pwndBottom->IsWindowVisible())
				{
					for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
					{
						m_pwndBottom->m_arDiffLines->SetAt(i, m_arDiffLines->GetAt(i));
						m_pwndBottom->m_arLineStates->SetAt(i, m_arLineStates->GetAt(i));
					}
					m_pwndBottom->SetModified();
				} // if (m_pwndBottom->IsWindowVisible()) 
				else
				{
					for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
					{
						m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
						CDiffData::DiffStates state = (CDiffData::DiffStates)m_pwndLeft->m_arLineStates->GetAt(i);
						switch (state)
						{
						case CDiffData::DIFFSTATE_ADDED:
						case CDiffData::DIFFSTATE_CONFLICTADDED:
						case CDiffData::DIFFSTATE_CONFLICTED:
						case CDiffData::DIFFSTATE_CONFLICTEMPTY:
						case CDiffData::DIFFSTATE_IDENTICALADDED:
						case CDiffData::DIFFSTATE_NORMAL:
						case CDiffData::DIFFSTATE_THEIRSADDED:
						case CDiffData::DIFFSTATE_UNKNOWN:
						case CDiffData::DIFFSTATE_YOURSADDED:
						case CDiffData::DIFFSTATE_EMPTY:
							m_arLineStates->SetAt(i, state);
							break;
						case CDiffData::DIFFSTATE_IDENTICALREMOVED:
						case CDiffData::DIFFSTATE_REMOVED:
						case CDiffData::DIFFSTATE_THEIRSREMOVED:
						case CDiffData::DIFFSTATE_YOURSREMOVED:
							m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_ADDED);
							break;
						default:
							break;
						}
					}
					SetModified();
				}
			} 
		break;
		case ID_USEYOURANDTHEIRBLOCK:
			{
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndBottom->m_arDiffLines->SetAt(i, m_pwndRight->m_arDiffLines->GetAt(i));
					m_pwndBottom->m_arLineStates->SetAt(i, m_pwndRight->m_arLineStates->GetAt(i));
					m_pwndRight->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_YOURSADDED);
				}

				// your block is done, now insert their block
				int index = m_nSelBlockEnd+1;
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndBottom->m_arDiffLines->InsertAt(index, m_pwndLeft->m_arDiffLines->GetAt(i));
					m_pwndBottom->m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
					m_pwndBottom->m_arLineStates->InsertAt(index++, m_pwndLeft->m_arLineStates->GetAt(i));
					m_pwndLeft->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_THEIRSADDED);
				}
				// adjust line numbers
				for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
				{
					long oldline = (long)m_pwndBottom->m_arLineLines->GetAt(i);
					if (oldline >= 0)
						m_pwndBottom->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
				}

				// now insert an empty block in both yours and theirs
				m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				RecalcAllVertScrollBars();
				m_pwndBottom->SetModified();
				m_pwndLeft->SetModified();
				m_pwndRight->SetModified();
			}
			break;
		case ID_USETHEIRANDYOURBLOCK:
			{
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndBottom->m_arDiffLines->SetAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
					m_pwndBottom->m_arLineStates->SetAt(i, m_pwndLeft->m_arLineStates->GetAt(i));
				}

				// your block is done, now insert their block
				int index = m_nSelBlockEnd+1;
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndBottom->m_arDiffLines->InsertAt(index, m_pwndRight->m_arDiffLines->GetAt(i));
					m_pwndBottom->m_arLineLines->InsertAt(index, m_pwndLeft->m_arLineLines->GetAt(i));
					m_pwndBottom->m_arLineStates->InsertAt(index++, m_pwndRight->m_arLineStates->GetAt(i));
				}
				// adjust line numbers
				for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
				{
					long oldline = (long)m_pwndBottom->m_arLineLines->GetAt(i);
					if (oldline >= 0)
						m_pwndBottom->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
				}

				// now insert an empty block in both yours and theirs
				m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndRight->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				RecalcAllVertScrollBars();
				m_pwndBottom->SetModified();
				m_pwndLeft->SetModified();
				m_pwndRight->SetModified();
			}
			break;
		case ID_USEBOTHTHISFIRST:
			{
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndRight->m_arLineStates->SetAt(i, CDiffData::DIFFSTATE_YOURSADDED);
				}

				// your block is done, now insert their block
				int index = m_nSelBlockEnd+1;
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndRight->m_arDiffLines->InsertAt(index, m_pwndLeft->m_arDiffLines->GetAt(i));
					m_pwndRight->m_arLineLines->InsertAt(index, m_pwndRight->m_arLineLines->GetAt(i));
					m_pwndRight->m_arLineStates->InsertAt(index++, CDiffData::DIFFSTATE_THEIRSADDED);
				}
				// adjust line numbers
				index--;
				for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
				{
					long oldline = (long)m_pwndRight->m_arLineLines->GetAt(i);
					if (oldline >= 0)
						m_pwndRight->m_arLineLines->SetAt(i, oldline+(index-m_nSelBlockEnd));
				}

				// now insert an empty block in the left view
				m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockEnd+1, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockEnd+1, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockEnd+1, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				RecalcAllVertScrollBars();
				m_pwndLeft->SetModified();
				m_pwndRight->SetModified();
			}
			break;
		case ID_USEBOTHTHISLAST:
			{
				// get line number from just before the block
				long linenumber = 0;
				if (m_nSelBlockStart > 0)
					linenumber = m_pwndRight->m_arLineLines->GetAt(m_nSelBlockStart-1);
				linenumber++;
				for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
				{
					m_pwndRight->m_arDiffLines->InsertAt(i, m_pwndLeft->m_arDiffLines->GetAt(i));
					m_pwndRight->m_arLineStates->InsertAt(i, CDiffData::DIFFSTATE_THEIRSADDED);
					m_pwndRight->m_arLineLines->InsertAt(i, linenumber++);
				}
				// adjust line numbers
				for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
				{
					long oldline = (long)m_pwndRight->m_arLineLines->GetAt(i);
					if (oldline >= 0)
						m_pwndRight->m_arLineLines->SetAt(i, oldline+(m_nSelBlockEnd-m_nSelBlockStart)+1);
				}

				// now insert an empty block in both yours and theirs
				m_pwndLeft->m_arDiffLines->InsertAt(m_nSelBlockStart, _T(""), m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineStates->InsertAt(m_nSelBlockStart, CDiffData::DIFFSTATE_EMPTY, m_nSelBlockEnd-m_nSelBlockStart+1);
				m_pwndLeft->m_arLineLines->InsertAt(m_nSelBlockStart, (DWORD)-1, m_nSelBlockEnd-m_nSelBlockStart+1);
				RecalcAllVertScrollBars();
				m_pwndLeft->SetModified();
				m_pwndRight->SetModified();
			}
			break;
		} // switch (cmd) 
	} // if (popup.CreatePopupMenu()) 
}
