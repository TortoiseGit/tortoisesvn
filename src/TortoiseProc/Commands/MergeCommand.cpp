// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2014, 2020-2021 - TortoiseSVN

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
#include "SVNProgressDlg.h"

bool MergeCommand::Execute()
{
    DWORD nMergeWizardMode = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\MergeWizardMode", 0));

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

    CMergeWizard wizard(IDS_PROGRS_CMDINFO, nullptr, nMergeWizardMode);
    wizard.m_wcPath = cmdLinePath;

    if (parser.HasVal(L"fromurl"))
    {
        wizard.m_url1 = parser.GetVal(L"fromurl");
        wizard.m_url  = parser.GetVal(L"fromurl");
        wizard.m_revRangeArray.FromListString(parser.GetVal(L"revrange"));
    }
    if (parser.HasVal(L"tourl"))
    {
        wizard.m_url2     = parser.GetVal(L"tourl");
        wizard.m_startRev = SVNRev(parser.GetVal(L"fromrev"));
        wizard.m_endRev   = SVNRev(parser.GetVal(L"torev"));
    }
    if (wizard.DoModal() == ID_WIZFINISH)
    {
        CSVNProgressDlg progDlg;
        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
        progDlg.SetAutoClose(parser);
        int options = wizard.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
        options |= wizard.m_bRecordOnly ? ProgOptRecordOnly : 0;
        options |= wizard.m_bForce ? ProgOptForce : 0;
        options |= wizard.m_bAllowMixed ? ProgOptAllowMixedRev : 0;
        progDlg.SetOptions(options);
        progDlg.SetPathList(CTSVNPathList(wizard.m_wcPath));
        progDlg.SetUrl(wizard.m_url1);
        progDlg.SetSecondUrl(wizard.m_url2);
        switch (wizard.m_nRevRangeMerge)
        {
            case MERGEWIZARD_REVRANGE:
            {
                if (wizard.m_revRangeArray.GetCount())
                {
                    wizard.m_revRangeArray.AdjustForMerge(!!wizard.m_bReverseMerge);
                    progDlg.SetRevisionRanges(wizard.m_revRangeArray);
                }
                else
                {
                    if (wizard.m_bReintegrate)
                        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrateOldStyle);
                    else
                        progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrate);
                }
                progDlg.SetPegRevision(wizard.m_pegRev);
            }
            break;
            case MERGEWIZARD_TREE:
            {
                progDlg.SetRevision(wizard.m_startRev);
                progDlg.SetRevisionEnd(wizard.m_endRev);
                if (wizard.m_url1.Compare(wizard.m_url2) == 0)
                {
                    SVNRevRangeArray tempRevArray;
                    tempRevArray.AdjustForMerge(!!wizard.m_bReverseMerge);
                    tempRevArray.AddRevRange(wizard.m_startRev, wizard.m_endRev);
                    progDlg.SetRevisionRanges(tempRevArray);
                }
            }
            break;
        }
        progDlg.SetDepth(wizard.m_depth);
        progDlg.SetDiffOptions(SVN::GetOptionsString(!!wizard.m_bIgnoreEOL, wizard.m_ignoreSpaces));
        progDlg.DoModal();
        return !progDlg.DidErrorsOccur();
    }
    return false;
}