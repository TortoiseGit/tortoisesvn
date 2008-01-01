// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng

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
#include "Resource.h"
#include "StandAloneDlg.h"

IMPLEMENT_DYNAMIC(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
CResizableStandAloneDialog::CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
	: CStandAloneDialogTmpl<CResizableDialog>(nIDTemplate, pParentWnd)
	, m_bVertical(false)
	, m_bHorizontal(false)
{
}

BEGIN_MESSAGE_MAP(CResizableStandAloneDialog, CStandAloneDialogTmpl<CResizableDialog>)
	ON_WM_SIZE()
	ON_WM_NCMBUTTONUP()
	ON_WM_NCRBUTTONUP()
END_MESSAGE_MAP()

void CResizableStandAloneDialog::OnSize(UINT nType, int cx, int cy)
{
	CStandAloneDialogTmpl<CResizableDialog>::OnSize(nType, cx, cy);
	if (nType == SIZE_RESTORED)
	{
		m_bVertical = m_bHorizontal = false;
	}
}

void CResizableStandAloneDialog::OnNcMButtonUp(UINT nHitTest, CPoint point) 
{
	WINDOWPLACEMENT windowPlacement;
	if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
	{
		CRect rcWorkArea, rcWindowRect;
		GetWindowRect(&rcWindowRect);
		if (m_bVertical)
		{
			rcWindowRect.top = m_rcOrgWindowRect.top;
			rcWindowRect.bottom = m_rcOrgWindowRect.bottom;
		}
		else if (SystemParametersInfo(SPI_GETWORKAREA, 0U, &rcWorkArea, 0U))
		{
			m_rcOrgWindowRect.top = rcWindowRect.top;
			m_rcOrgWindowRect.bottom = rcWindowRect.bottom;
			rcWindowRect.top = rcWorkArea.top;
			rcWindowRect.bottom = rcWorkArea.bottom;
		}
		bool bVertical = !m_bVertical;
		bool bHorizontal = m_bHorizontal;
		MoveWindow(&rcWindowRect);
		m_bVertical = bVertical;
		m_bHorizontal = bHorizontal;
	}
	CStandAloneDialogTmpl<CResizableDialog>::OnNcMButtonUp(nHitTest, point);
}

void CResizableStandAloneDialog::OnNcRButtonUp(UINT nHitTest, CPoint point) 
{
	WINDOWPLACEMENT windowPlacement;
	if ((nHitTest == HTMAXBUTTON) && GetWindowPlacement(&windowPlacement) && windowPlacement.showCmd == SW_SHOWNORMAL)
	{
		CRect rcWorkArea, rcWindowRect;
		GetWindowRect(&rcWindowRect);
		if (m_bHorizontal)
		{
			rcWindowRect.left = m_rcOrgWindowRect.left;
			rcWindowRect.right = m_rcOrgWindowRect.right;
		}
		else if (SystemParametersInfo(SPI_GETWORKAREA, 0U, &rcWorkArea, 0U))
		{
			m_rcOrgWindowRect.left = rcWindowRect.left;
			m_rcOrgWindowRect.right = rcWindowRect.right;
			rcWindowRect.left = rcWorkArea.left;
			rcWindowRect.right = rcWorkArea.right;
		}
		bool bVertical = m_bVertical;
		bool bHorizontal = !m_bHorizontal;
		MoveWindow(&rcWindowRect);
		m_bVertical = bVertical;
		m_bHorizontal = bHorizontal;
	}
	CStandAloneDialogTmpl<CResizableDialog>::OnNcRButtonUp(nHitTest, point);
}
