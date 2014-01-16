// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014 - TortoiseSVN

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
#include "MergeCommand.h"

#include "MergeWizard.h"
#include "MergeWizardStart.h"
#include "SVNProgressDlg.h"

bool MergeCommand::Execute()
{
    DWORD nMergeWizardMode =
        (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\MergeWizardMode", 0);

    if (parser.HasVal(L"fromurl"))
    {
        // fromurl means merging a revision range
        nMergeWizardMode = 2;
    }
    if (parser.HasVal(L"tourl"))
    {
        // tourl means merging a tree
        nMergeWizardMode = 1;
    }

    CMergeWizard wizard(IDS_PROGRS_CMDINFO, NULL, nMergeWizardMode);
    wizard.wcPath = cmdLinePath;

    if (parser.HasVal(L"fromurl"))
    {
        wizard.URL1 = parser.GetVal(L"fromurl");
        wizard.url = parser.GetVal(L"fromurl");
        wizard.revRangeArray.FromListString(parser.GetVal(L"revrange"));
    }
    if (parser.HasVal(L"tourl"))
    {
        wizard.URL2 = parser.GetVal(L"tourl");
        wizard.startRev = SVNRev(parser.GetVal(L"fromrev"));
        wizard.endRev = SVNRev(parser.GetVal(L"torev"));
    }
    if (wizard.DoModal() == ID_WIZFINISH)
    {
        CSVNProgressDlg progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        int options = wizard.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
        options |= wizard.m_bRecordOnly ? ProgOptRecordOnly : 0;
        options |= wizard.m_bForce ? ProgOptForce : 0;
        options |= wizard.bAllowMixedRev ? ProgOptAllowMixedRev : 0;
        progDlg.SetOptions(options);
        progDlg.SetPathList(CTSVNPathList(wizard.wcPath));
        progDlg.SetUrl(wizard.URL1);
        progDlg.SetSecondUrl(wizard.URL2);
        switch (wizard.nRevRangeMerge)
        {
        case MERGEWIZARD_REVRANGE:
            {
                if (wizard.revRangeArray.GetCount())
                {
                    wizard.revRangeArray.AdjustForMerge(!!wizard.bReverseMerge);
                    progDlg.SetRevisionRanges(wizard.revRangeArray);
                }
                else
                {
                    if (wizard.bReintegrate)
                        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrateOldStyle);
                    else
                        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrate);
                }
                progDlg.SetPegRevision(wizard.pegRev);
            }
            break;
        case MERGEWIZARD_TREE:
            {
                progDlg.SetRevision(wizard.startRev);
                progDlg.SetRevisionEnd(wizard.endRev);
                if (wizard.URL1.Compare(wizard.URL2) == 0)
                {
                    SVNRevRangeArray tempRevArray;
                    tempRevArray.AdjustForMerge(!!wizard.bReverseMerge);
                    tempRevArray.AddRevRange(wizard.startRev, wizard.endRev);
                    progDlg.SetRevisionRanges(tempRevArray);
                }
            }
            break;
        }
        progDlg.SetDepth(wizard.m_depth);
        progDlg.SetDiffOptions(SVN::GetOptionsString(!!wizard.m_bIgnoreEOL, wizard.m_IgnoreSpaces));
        progDlg.DoModal();
        return !progDlg.DidErrorsOccur();
    }
    return false;
}