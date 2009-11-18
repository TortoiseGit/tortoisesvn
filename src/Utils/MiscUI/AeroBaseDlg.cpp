// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "AeroBaseDlg.h"

BEGIN_TEMPLATE_MESSAGE_MAP(CStandAloneDialogTmpl, BaseType, BaseType)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_NCHITTEST()
	ON_WM_DWMCOMPOSITIONCHANGED()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
CStandAloneDialog::CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd /*= NULL*/)
: CStandAloneDialogTmpl<CDialog>(nIDTemplate, pParentWnd)
{
}
BEGIN_MESSAGE_MAP(CStandAloneDialog, CStandAloneDialogTmpl<CDialog>)
END_MESSAGE_MAP()

