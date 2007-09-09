// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include "ResolveCommand.h"

#include "ResolveDlg.h"
#include "SVNProgressDlg.h"

bool ResolveCommand::Execute()
{
	CResolveDlg dlg;
	dlg.m_pathList = pathList;
	INT_PTR ret = IDOK;
	if (!parser.HasKey(_T("noquestion")))
		ret = dlg.DoModal();
	if (ret == IDOK)
	{
		if (dlg.m_pathList.GetCount())
		{
			int options = 0;
			CSVNProgressDlg progDlg(CWnd::FromHandle(hWndExplorer));
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			if (parser.HasKey(_T("skipcheck")))
				options |= ProgOptSkipConflictCheck;
			theApp.m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::SVNProgress_Resolve, options, dlg.m_pathList);
			progDlg.DoModal();
		}
	}
	return true;
}