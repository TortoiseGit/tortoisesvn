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

#include "MergeWizard.h"
#include "MergeWizardStart.h"
#include "SVNProgressDlg.h"
#include "MessageBox.h"

bool MergeCommand::Execute()
{
	CMergeWizard wizard(IDS_PROGRS_CMDINFO, NULL, 0);
	wizard.wcPath = cmdLinePath;
	if (wizard.DoModal() == ID_WIZFINISH)
	{
		CSVNProgressDlg progDlg;
		progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
		int options = wizard.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
		options |= wizard.bRecordOnly ? ProgOptRecordOnly : 0;
		progDlg.SetOptions(options);
		progDlg.SetPathList(CTSVNPathList(wizard.wcPath));
		progDlg.SetUrl(wizard.URL1);
		progDlg.SetSecondUrl(wizard.URL2);
		if (wizard.bRevRangeMerge)
		{
			if (wizard.revRangeArray.GetCount())
			{
				wizard.revRangeArray.AdjustForMerge(!!wizard.bReverseMerge);
				progDlg.SetRevisionRanges(wizard.revRangeArray);
			}
			else
			{
				SVNRevRangeArray tempRevArray;
				tempRevArray.AddRevRange(SVNRev(), SVNRev());
				progDlg.SetRevisionRanges(tempRevArray);
			}
		}
		else
		{
			progDlg.SetRevision(wizard.startRev);
			progDlg.SetRevisionEnd(wizard.endRev);
		}
		progDlg.SetDepth(wizard.m_depth);
		progDlg.SetDiffOptions(SVN::GetOptionsString(wizard.m_bIgnoreEOL, wizard.m_IgnoreSpaces));
		progDlg.DoModal();
		return true;
	}
	return false;
}