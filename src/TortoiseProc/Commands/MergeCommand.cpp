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
#include "MergeCommand.h"

#include "MergeDlg.h"
#include "SVNProgressDlg.h"
#include "MessageBox.h"

bool MergeCommand::Execute()
{
	BOOL repeat = FALSE;
	CMergeDlg dlg;
	dlg.m_wcPath = cmdLinePath;
	// mergefrom = start revision of the merge revision range
	if (parser.HasVal(_T("mergefrom")))
		dlg.StartRev = SVNRev(parser.GetVal(_T("mergefrom")));
	// mergeto = end revision of the merge revision range
	if (parser.HasVal(_T("mergeto")))
		dlg.EndRev = SVNRev(parser.GetVal(_T("mergeto")));
	// fromurl = the url the merge is done from
	if (parser.HasVal(_T("fromurl")))
		dlg.m_URLFrom = parser.GetVal(_T("fromurl"));
	// The do-while loop is for repeating the merge dialog.
	// It's needed in case the user tries a "dry-run"
	do 
	{	
		if (dlg.DoModal() == IDOK)
		{
			CSVNProgressDlg progDlg;
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			int options = dlg.m_bDryRun ? ProgOptDryRun : 0;
			options |= dlg.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
			options |= dlg.m_bRecordOnly ? ProgOptRecordOnly : 0;
			progDlg.SetParams(CSVNProgressDlg::SVNProgress_Merge, options, pathList, dlg.m_URLFrom, dlg.m_URLTo, dlg.StartRev);		//use the message as the second url
			// use the depth of the working copy
			progDlg.SetDepth(dlg.m_depth);
			progDlg.m_RevisionEnd = dlg.EndRev;
			progDlg.SetDiffOptions(SVN::GetOptionsString(dlg.m_bIgnoreEOL, dlg.m_IgnoreSpaces));
			progDlg.DoModal();
			repeat = dlg.m_bDryRun;
			dlg.bRepeating = TRUE;
		}
		else
			repeat = FALSE;
	} while(repeat);
	return true;
}