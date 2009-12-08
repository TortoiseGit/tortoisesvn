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
#include "StdAfx.h"
#include "EditFileCommand.h"
#include "TempFile.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "SVNStatus.h"
#include "SVNProgressDlg.h"
#include "SVNProperties.h"
#include "CommitCommand.h"
#include "LockCommand.h"

// status check

bool EditFileCommand::IsModified()
{
	return SVNStatus::GetAllStatus(path) != svn_wc_status_normal;
}

bool EditFileCommand::IsLocked()
{
	CTSVNPath dummy;
	SVNStatus status;

	svn_wc_status2_t *fileStatus
		= status.GetFirstFileStatus (path, dummy, false, svn_depth_empty);
	return fileStatus->locked != FALSE;
}

// the individual steps of the sequence 

bool EditFileCommand::AutoCheckout()
{
	// if a non-WC revision is given for an existing w/c, use a temp. w/c

	if (!cmdLinePath.IsUrl() && revision.IsValid() && !revision.IsWorking())
		cmdLinePath.SetFromSVN (SVN().GetURLFromPath (cmdLinePath));

	// c/o only required for URLs

	if (cmdLinePath.IsUrl())
	{
		if (!revision.IsValid())
			revision = SVNRev::REV_HEAD;

		bool isFile = SVNInfo::IsFile (cmdLinePath, revision);

		// create a temp. wc for the path to edit

		CTSVNPath tempWC = CTempFiles::Instance().GetTempDirPath 
			(true, isFile ? cmdLinePath.GetContainingDirectory()
						  : cmdLinePath);

		path = tempWC;
		if (isFile)
			path.AppendPathString (cmdLinePath.GetFilename());

		// check out

		CSVNProgressDlg progDlg;
		theApp.m_pMainWnd = &progDlg;

		progDlg.SetCommand 
			(isFile
				? CSVNProgressDlg::SVNProgress_SingleFileCheckout
				: CSVNProgressDlg::SVNProgress_Checkout);

		progDlg.SetAutoClose(CLOSE_NOERRORS);
		progDlg.SetAutoCloseLocal(CLOSE_NOERRORS);
		progDlg.SetOptions(ProgOptIgnoreExternals);
		progDlg.SetPathList(CTSVNPathList(path));
		progDlg.SetUrl(cmdLinePath.GetSVNPathString());
		progDlg.SetRevision(revision);
		progDlg.SetDepth(svn_depth_infinity);
		progDlg.DoModal();

		return !progDlg.DidErrorsOccur();
	}
	else
	{
		path = cmdLinePath;
		return true;
	}
}

bool EditFileCommand::AutoLock()
{
	// needs lock?

	SVNProperties properties (path, SVNRev::REV_WC, false);
	if (!properties.HasProperty (SVN_PROP_NEEDS_LOCK))
		return true;

	// try to lock

	LockCommand command;
	command.SetPaths (CTSVNPathList (path), path);
	needsLock = command.Execute();

	return needsLock;
}

bool EditFileCommand::Edit()
{
	int result = (int)ShellExecute (NULL, _T("open"), path.GetWinPathString(), NULL, NULL, SW_SHOWNORMAL);
	return result > HINSTANCE_ERROR;
}

bool EditFileCommand::AutoCheckin()
{
	// no-op, if not modified

	if (!IsModified())
		return true;

	// check-in

	CommitCommand command;
	command.SetPaths (CTSVNPathList (path), path);
	return command.Execute();
}

bool EditFileCommand::AutoUnLock()
{
	if (!needsLock || !IsLocked())
		return true;

	CSVNProgressDlg progDlg;
	progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Unlock);
	progDlg.SetOptions(ProgOptNone);
	progDlg.SetPathList (CTSVNPathList (cmdLinePath));
	progDlg.SetAutoClose (CLOSE_NOERRORS);
	progDlg.SetAutoCloseLocal(CLOSE_NOERRORS);
	progDlg.DoModal();

	return !progDlg.DidErrorsOccur();
}

/// construction / destruction

EditFileCommand::EditFileCommand()
	: needsLock (false)
{
}

EditFileCommand::~EditFileCommand()
{
	AutoUnLock();
}

bool EditFileCommand::Execute()
{
	// make sure, the data is in a wc

	if (parser.HasKey (_T("revision")))
		revision = SVNRev(parser.GetLongVal (_T("revision")));

	// the sequence

	return AutoCheckout()
		&& AutoLock()
		&& Edit()
		&& AutoCheckin();
}