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
#include "LockCommand.h"

#include "LockDlg.h"
#include "SVNProgressDlg.h"

bool LockCommand::Execute()
{
	CLockDlg lockDlg;
	lockDlg.m_pathList = pathList;
	ProjectProperties props;
	props.ReadPropsPathList(pathList);
	lockDlg.SetProjectProperties(&props);
	if (pathList.AreAllPathsFiles() && !DWORD(CRegDWORD(_T("Software\\TortoiseSVN\\ShowLockDlg"), TRUE)) && !props.nMinLockMsgSize)
	{
		// just lock the requested files
		CSVNProgressDlg progDlg;
		progDlg.SetParams(CSVNProgressDlg::SVNProgress_Lock, 0, pathList, CString(), CString());
		progDlg.DoModal();
	}
	else if (lockDlg.DoModal()==IDOK)
	{
		if (lockDlg.m_pathList.GetCount() != 0)
		{
			CSVNProgressDlg progDlg;
			progDlg.SetParams(CSVNProgressDlg::SVNProgress_Lock, lockDlg.m_bStealLocks ? ProgOptLockForce : 0, lockDlg.m_pathList, CString(), lockDlg.m_sLockMessage);
			progDlg.DoModal();
		}
	}
	return true;
}