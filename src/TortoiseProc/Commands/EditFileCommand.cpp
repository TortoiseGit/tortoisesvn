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
#include "SVNProgressDlg.h"
#include "SVNProperties.h"

bool EditFileCommand::AutoCheckout()
{
	// if a non-WC revision is given for an existing w/c, use a temp. w/c

	if (!cmdLinePath.IsUrl() && revision.IsValid() && !revision.IsWorking())
		cmdLinePath.SetFromSVN (SVN().GetURLFromPath (cmdLinePath));

	// c/o only required for URLs

	if (cmdLinePath.IsUrl())
	{
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

		progDlg.SetAutoClose(TRUE);
		progDlg.SetAutoCloseLocal(TRUE);
		progDlg.SetOptions(ProgOptIgnoreExternals);
		progDlg.SetPathList(CTSVNPathList(tempWC));
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

bool EditFileCommand::Execute()
{
	/// make sure, the data is in a wc

	if (parser.HasKey (_T("revision")))
		revision = SVNRev(parser.GetLongVal (_T("revision")));

	if (!AutoCheckout())
		return false;

	// automatically lock the file / folder

	SVNProperties properties (path, SVNRev::REV_WC, false);

	// done

	return true;
}