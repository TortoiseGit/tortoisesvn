// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "WatcherDlg.h"
#include <shlobj.h>
#pragma warning (disable : 4312)
// CWatcherDlg dialog

IMPLEMENT_DYNAMIC(CWatcherDlg, CDialog)
CWatcherDlg::CWatcherDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWatcherDlg::IDD, pParent)
{
	m_before = "";
	m_after = "";
}

CWatcherDlg::~CWatcherDlg()
{
}

void CWatcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWatcherDlg, CDialog)
	ON_WM_WINDOWPOSCHANGING()
	ON_MESSAGE(WMU_SHELLNOTIFY, OnShellNotify)
END_MESSAGE_MAP()


// CWatcherDlg message handlers

void CWatcherDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	__super::OnWindowPosChanging(lpwndpos);
	lpwndpos->flags |= SWP_HIDEWINDOW;	//always hide the dialog
	lpwndpos->flags &= ~SWP_SHOWWINDOW;
}

BOOL CWatcherDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SHChangeNotifyEntry scnEntry;


	//get the root folder
	//SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, NULL, scnEntry.pidl);
	SHGetSpecialFolderLocation(this->m_hWnd, CSIDL_DESKTOP, (LPITEMIDLIST *)&scnEntry.pidl);
	scnEntry.fRecursive = true;

	//this shell function is documented at 
	//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/Shell/reference/functions/SHChangeNotifyRegister.asp
	//you need the latest platform SDK to use it (Aug 02 or later)
	m_uID = SHChangeNotifyRegister(this->m_hWnd, 
								3,//the flags are not defined in shlobj.h - so just guess :) SHCNRF_InterruptLevel | SHCNRF_ShellLevel 
								SHCNE_DELETE | SHCNE_RENAMEFOLDER | SHCNE_RENAMEITEM | SHCNE_RMDIR | SHCNE_UPDATEITEM,
								WMU_SHELLNOTIFY,
								1,
								&scnEntry);
	if (m_uID == 0)
	{
		//SHChangeNotifyRegister failed, so close the dialog
		CMessageBox::Show(NULL, _T("could not register shell notification callback!\n<ct=0x0000ff>Rename, Copy, Move and Delete <b>won't</b> work from windows explorer!</ct>"), _T("TortoiseSVN"), MB_ICONERROR);
		PostMessage(WM_QUIT);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

CString CWatcherDlg::GetPathFromPIDL(DWORD pidl) 
{
	TCHAR sPath[MAX_PATH];
	CString strTemp = _T("");
	if(SHGetPathFromIDList((struct _ITEMIDLIST *)pidl, sPath)) 
	{
		strTemp = sPath;
	}
	return strTemp;
}

void CWatcherDlg::PostNcDestroy()
{
	SHChangeNotifyDeregister(m_uID);
	CDialog::PostNcDestroy();
}

LRESULT CWatcherDlg::OnShellNotify(WPARAM wParam, LPARAM lParam)
{
	//lParam contains the event (SHCNE_)
	//wParam contains the SHChangeNotifyEntry struct
	SHNOTIFYSTRUCT notify;
	memcpy((void *)&notify,(void *)wParam,sizeof(SHNOTIFYSTRUCT));

	CString strBefore = GetPathFromPIDL(notify.dwItem1);
	CString strAfter = GetPathFromPIDL(notify.dwItem2);

	//since we get for every rename the message twice (it seems once for source and once for destination)
	//check if we already handled the rename by comparing the filenames.
	if ((strBefore.Compare(m_before)==0)&&(strAfter.Compare(m_after)==0))
		return 0;		//already handled that rename

	m_before = strBefore;
	m_after = strAfter;

	TRACE(_T("%s\n -> \n%s\n\n"), strBefore, strAfter);
	if (lParam == SHCNE_DELETE)
	{
		svn_wc_status_kind status = SVNStatus::GetAllStatus(strBefore);
		if ((status!=svn_wc_status_unversioned)&&(status!=svn_wc_status_none))
		{
			TRACE(_T("Remove File\n"));
			if (Remove(strBefore, true))
			{
				CString temp;
				temp.Format(_T("TortoiseSVN has removed the file %s from source control"), strBefore);
				CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK);
				TRACE(temp+_T("\n"));
			}
			else
			{
				CString temp;
				temp.Format(_T("An error occurred while trying to rename %s to %s in Subversion control:\n%s"), strBefore, strAfter, GetLastErrorMessage());
				CMessageBox::Show(NULL, temp, _T("Subversion"), MB_ICONERROR);
				TRACE(temp+_T("\n"));
			}
		} // if (SVNStatus::GetAllStatus(strBefore)!=svn_wc_status_unversioned)
	}
	if (lParam == SHCNE_RMDIR)
	{
		svn_wc_status_kind status = SVNStatus::GetAllStatus(strBefore);
		if ((status!=svn_wc_status_unversioned)&&(status!=svn_wc_status_none))
		{
			TRACE(_T("Remove Directory\n"));
			//removing a already removed directory does not work in subversion
			CMessageBox::Show(NULL, _T("You have deleted a folder outside TortoiseSVN.\nPlease use the Undo function of the explorer and delete the folder with TortoiseSVN!"), _T("TortoiseSVN"), MB_ICONERROR);
		}
	}
	if ((lParam == SHCNE_RENAMEFOLDER)||(lParam == SHCNE_RENAMEITEM))
	{
		TRACE(_T("Rename\n"));
		svn_wc_status_kind status = SVNStatus::GetAllStatus(strBefore);
		if ((status!=svn_wc_status_unversioned)&&(status!=svn_wc_status_none))
		{
			//renaming (moving) an already renamed/moved directory or file does not work in subversion. To get this to work
			//we need to rename/move the folderor file back and then do the subversion rename/move.
			MoveFileEx(strAfter, strBefore, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
			if (!Move(strBefore, strAfter, true))
			{
				CString temp;
				temp.Format(_T("An error occurred while trying to rename %s to %s in Subversion control:\n<hr=100%>\n%s"), strBefore, strAfter, GetLastErrorMessage());
				CMessageBox::Show(NULL, temp, _T("Subversion"), MB_ICONERROR);
				TRACE(temp+_T("\n"));
			}
			else
			{
				CString temp;
				temp.Format(_T("TortoiseSVN has renamed/moved the file\n%s to %s\nin your working copy"), strBefore, strAfter);
				CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK);
				TRACE(temp+_T("\n"));
			}
		}
	} // if ((lParam == SHCNE_RENAMEFOLDER)||(lParam == SHCNE_RENAMEITEM))
	TRACE(_T("-------------------------\n"));
	return 0;
}

