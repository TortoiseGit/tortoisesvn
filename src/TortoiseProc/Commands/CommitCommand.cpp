// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

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
#include "CommitCommand.h"

#include "CommitDlg.h"
#include "SVNProgressDlg.h"
#include "StringUtils.h"
#include "Hooks.h"
#include "MessageBox.h"

CString CommitCommand::LoadLogMessage()
{
	CString msg;
	if (parser.HasKey(_T("logmsg")))
	{
		msg = parser.GetVal(_T("logmsg"));
	}
	if (parser.HasKey(_T("logmsgfile")))
	{
		CString logmsgfile = parser.GetVal(_T("logmsgfile"));
		CStringUtils::ReadStringFromTextFile(logmsgfile, msg);
	}
	return msg;
}

void CommitCommand::InitProgressDialog 
    ( CCommitDlg& commitDlg
    , CSVNProgressDlg& progDlg)
{
	progDlg.SetChangeList(commitDlg.m_sChangeList, !!commitDlg.m_bKeepChangeList);
	if (parser.HasVal(_T("closeonend")))
		progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
	if (parser.HasKey(_T("closeforlocal")))
		progDlg.SetAutoCloseLocal(TRUE);
	progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Commit);
	progDlg.SetOptions(commitDlg.m_bKeepLocks ? ProgOptKeeplocks : ProgOptNone);
	progDlg.SetPathList(commitDlg.m_pathList);
	progDlg.SetCommitMessage(commitDlg.m_sLogMessage);
	progDlg.SetDepth(commitDlg.m_bRecursive ? svn_depth_infinity : svn_depth_empty);
	progDlg.SetSelectedList(commitDlg.m_selectedPathList);
	progDlg.SetItemCount(static_cast<long>(commitDlg.m_itemsCount));
	progDlg.SetBugTraqProvider(commitDlg.m_BugTraqProvider);
	progDlg.SetRevisionProperties(commitDlg.m_revProps);
}

bool CommitCommand::Execute()
{
	bool bRet = false;
	bool bFailed = true;
	CTSVNPathList selectedList;
	if (parser.HasKey(_T("logmsg")) && (parser.HasKey(_T("logmsgfile"))))
	{
		CMessageBox::Show(hwndExplorer, IDS_ERR_TWOLOGPARAMS, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	CString sLogMsg = LoadLogMessage();
	bool bSelectFilesForCommit = !!DWORD(CRegStdDWORD(_T("Software\\TortoiseSVN\\SelectFilesForCommit"), TRUE));
	DWORD exitcode = 0;
	CString error;
	if (CHooks::Instance().StartCommit(pathList, sLogMsg, exitcode, error))
	{
		if (exitcode)
		{
			CString temp;
			temp.Format(IDS_ERR_HOOKFAILED, (LPCTSTR)error);
			CMessageBox::Show(hwndExplorer, temp, _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	while (bFailed)
	{
		bFailed = false;
		CCommitDlg dlg;
		if (parser.HasKey(_T("bugid")))
		{
			dlg.m_sBugID = parser.GetVal(_T("bugid"));
		}
		dlg.m_sLogMessage = sLogMsg;
		dlg.m_pathList = pathList;
		dlg.m_checkedPathList = selectedList;
		dlg.m_bSelectFilesForCommit = bSelectFilesForCommit;
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.m_pathList.GetCount()==0)
				return false;
			// if the user hasn't changed the list of selected items
			// we don't use that list. Because if we would use the list
			// of pre-checked items, the dialog would show different
			// checked items on the next startup: it would only try
			// to check the parent folder (which might not even show)
			// instead, we simply use an empty list and let the
			// default checking do its job.
			if (!dlg.m_pathList.IsEqual(pathList))
				selectedList = dlg.m_pathList;
			pathList = dlg.m_updatedPathList;
			sLogMsg = dlg.m_sLogMessage;
			bSelectFilesForCommit = true;
			CSVNProgressDlg progDlg;
            InitProgressDialog (dlg, progDlg);
			progDlg.DoModal();

			bool isOutOfDate = false;
			svn_error_t * pErr = progDlg.Err;
			while (pErr)
			{
				if ((pErr->apr_err == SVN_ERR_FS_TXN_OUT_OF_DATE)||
					(pErr->apr_err == SVN_ERR_RA_OUT_OF_DATE)||
					(pErr->apr_err == SVN_ERR_FS_CONFLICT))
				{
					isOutOfDate = true;
					break;
				}
				pErr = pErr->child;
			}
            if (isOutOfDate)
            {
                // the commit failed at least one of the items was outdated.
                // -> suggest to update them

                CString title;
                title.LoadString (IDS_MSG_NEEDSUPDATE_TITLE);
                CString question;
                question.Format ( IDS_MSG_NEEDSUPDATE_QUESTION
                                , (LPCTSTR)progDlg.GetLastErrorMessage());
                if (CMessageBox::Show ( NULL
                                      , question
                                      , title
                                      , MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    // auto-update

			        CSVNProgressDlg updateProgDlg;
                    InitProgressDialog (dlg, updateProgDlg);
                    updateProgDlg.SetCommand (CSVNProgressDlg::SVNProgress_Update);
        			updateProgDlg.DoModal();

                    // re-open commit dialog only if update *SUCCEEDED*
                    // (don't pass update errors to caller and *never*
                    // auto-reopen commit dialog upon error)

                    bFailed =    !updateProgDlg.DidErrorsOccur() 
                              && !updateProgDlg.DidConflictsOccur();
                    bRet = bFailed;
			        CRegDWORD (_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE) 
                        = bRet ? TRUE : FALSE;

                    continue;
                }
            }

            // If there was an error and the user set the 
            // "automatically re-open commit dialog" option, do so.

		    CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
		    err = (DWORD)progDlg.DidErrorsOccur();
		    bFailed = progDlg.DidErrorsOccur();
		    bRet = progDlg.DidErrorsOccur();
		    CRegDWORD bFailRepeat = CRegDWORD(_T("Software\\TortoiseSVN\\CommitReopen"), FALSE);
		    if (DWORD(bFailRepeat)==0)
			    bFailed = false;		// do not repeat if the user chose not to in the settings.
		}
	}
	return bRet;
}
