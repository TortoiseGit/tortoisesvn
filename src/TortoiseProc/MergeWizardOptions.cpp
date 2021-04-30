// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2012-2014, 2020-2021 - TortoiseSVN

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
#include "MergeWizard.h"
#include "MergeWizardOptions.h"
#include "SVNProgressDlg.h"
#include "WaitDlg.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CMergeWizardOptions, CMergeWizardBasePage)

CMergeWizardOptions::CMergeWizardOptions()
    : CMergeWizardBasePage(CMergeWizardOptions::IDD)
{
    m_psp.dwFlags |= PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_psp.pszHeaderTitle    = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSTITLE);
    m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSSUBTITLE);
}

CMergeWizardOptions::~CMergeWizardOptions()
{
}

void CMergeWizardOptions::DoDataExchange(CDataExchange* pDX)
{
    CMergeWizardBasePage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
    DDX_Check(pDX, IDC_IGNOREANCESTRY, static_cast<CMergeWizard*>(GetParent())->m_bIgnoreAncestry);
    DDX_Control(pDX, IDC_DEPTH, m_depthCombo);
    DDX_Check(pDX, IDC_IGNOREEOL, static_cast<CMergeWizard*>(GetParent())->m_bIgnoreEOL);
    DDX_Check(pDX, IDC_RECORDONLY, static_cast<CMergeWizard*>(GetParent())->m_bRecordOnly);
    DDX_Check(pDX, IDC_FORCE, static_cast<CMergeWizard*>(GetParent())->m_bForce);
    DDX_Check(pDX, IDC_ALLOWMIXED, static_cast<CMergeWizard*>(GetParent())->m_bAllowMixed);
    DDX_Check(pDX, IDC_REINTEGRATEOLDSTYLE, static_cast<CMergeWizard*>(GetParent())->m_bReintegrate);
}

BEGIN_MESSAGE_MAP(CMergeWizardOptions, CMergeWizardBasePage)
    ON_BN_CLICKED(IDC_DRYRUN, &CMergeWizardOptions::OnBnClickedDryrun)
    ON_BN_CLICKED(IDC_REINTEGRATEOLDSTYLE, &CMergeWizardOptions::OnBnClickedReintegrateoldstyle)
END_MESSAGE_MAP()

BOOL CMergeWizardOptions::OnInitDialog()
{
    CMergeWizardBasePage::OnInitDialog();

    CMergeWizard* pWizard = static_cast<CMergeWizard*>(GetParent());

    CString   sRegOptionIgnoreAncestry = L"Software\\TortoiseSVN\\Merge\\IgnoreAncestry_" + pWizard->m_sUuid;
    CRegDWORD regIgnoreAncestryOpt(sRegOptionIgnoreAncestry, FALSE);
    pWizard->m_bIgnoreAncestry = static_cast<DWORD>(regIgnoreAncestryOpt);
    if ((pWizard->m_nRevRangeMerge == MERGEWIZARD_REVRANGE) && (!pWizard->m_bReintegrate))
    {
        pWizard->m_bAllowMixed = false;
    }

    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_WORKING)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_INFINITE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_IMMEDIATE)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_FILES)));
    m_depthCombo.AddString(CString(MAKEINTRESOURCE(IDS_SVN_DEPTH_EMPTY)));
    switch (static_cast<CMergeWizard*>(GetParent())->m_depth)
    {
        case svn_depth_unknown:
            m_depthCombo.SetCurSel(0);
            break;
        case svn_depth_infinity:
            m_depthCombo.SetCurSel(1);
            break;
        case svn_depth_immediates:
            m_depthCombo.SetCurSel(2);
            break;
        case svn_depth_files:
            m_depthCombo.SetCurSel(3);
            break;
        case svn_depth_empty:
            m_depthCombo.SetCurSel(4);
            break;
        default:
            m_depthCombo.SetCurSel(0);
            break;
    }

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_RECORDONLY, IDS_MERGEWIZARD_OPTIONS_RECORDONLY_TT);
    m_tooltips.AddTool(IDC_FORCE, IDS_MERGEWIZARD_OPTIONS_FORCE_TT);

    CheckRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES, IDC_COMPAREWHITESPACES);

    AdjustControlSize(IDC_IGNOREANCESTRY);
    AdjustControlSize(IDC_IGNOREEOL);
    AdjustControlSize(IDC_COMPAREWHITESPACES);
    AdjustControlSize(IDC_IGNOREWHITESPACECHANGES);
    AdjustControlSize(IDC_IGNOREALLWHITESPACES);
    AdjustControlSize(IDC_FORCE);
    AdjustControlSize(IDC_ALLOWMIXED);
    AdjustControlSize(IDC_RECORDONLY);
    AdjustControlSize(IDC_REINTEGRATEOLDSTYLE);

    AddAnchor(IDC_MERGEOPTIONSGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_MERGEOPTIONSDEPTHLABEL, TOP_LEFT);
    AddAnchor(IDC_DEPTH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_IGNOREANCESTRY, TOP_LEFT);
    AddAnchor(IDC_IGNOREEOL, TOP_LEFT);
    AddAnchor(IDC_COMPAREWHITESPACES, TOP_LEFT);
    AddAnchor(IDC_IGNOREWHITESPACECHANGES, TOP_LEFT);
    AddAnchor(IDC_IGNOREALLWHITESPACES, TOP_LEFT);
    AddAnchor(IDC_FORCE, TOP_LEFT);
    AddAnchor(IDC_ALLOWMIXED, TOP_LEFT);
    AddAnchor(IDC_RECORDONLY, TOP_LEFT);
    AddAnchor(IDC_REINTEGRATEOLDSTYLE, TOP_LEFT);
    AddAnchor(IDC_DRYRUN, BOTTOM_RIGHT);

    UpdateData(FALSE);

    CTheme::Instance().SetThemeForDialog(GetSafeHwnd(), CTheme::Instance().IsDarkTheme());

    return TRUE;
}

LRESULT CMergeWizardOptions::OnWizardBack()
{
    return static_cast<CMergeWizard*>(GetParent())->GetSecondPage();
}

BOOL CMergeWizardOptions::OnWizardFinish()
{
    UpdateData();
    CMergeWizard* pWizard = static_cast<CMergeWizard*>(GetParent());
    switch (m_depthCombo.GetCurSel())
    {
        case 0:
            pWizard->m_depth = svn_depth_unknown;
            break;
        case 1:
            pWizard->m_depth = svn_depth_infinity;
            break;
        case 2:
            pWizard->m_depth = svn_depth_immediates;
            break;
        case 3:
            pWizard->m_depth = svn_depth_files;
            break;
        case 4:
            pWizard->m_depth = svn_depth_empty;
            break;
        default:
            pWizard->m_depth = svn_depth_empty;
            break;
    }
    pWizard->m_ignoreSpaces = GetIgnores();

    CString   sRegOptionIgnoreAncestry = L"Software\\TortoiseSVN\\Merge\\IgnoreAncestry_" + pWizard->m_sUuid;
    CRegDWORD regIgnoreAncestryOpt(sRegOptionIgnoreAncestry, FALSE);
    regIgnoreAncestryOpt = pWizard->m_bIgnoreAncestry;

    return CMergeWizardBasePage::OnWizardFinish();
}

BOOL CMergeWizardOptions::OnSetActive()
{
    CPropertySheet* pSheet = static_cast<CPropertySheet*>(GetParent());
    pSheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
    SetButtonTexts();
    CMergeWizard* pWizard = static_cast<CMergeWizard*>(GetParent());
    GetDlgItem(IDC_REINTEGRATEOLDSTYLE)->EnableWindow((pWizard->m_nRevRangeMerge == MERGEWIZARD_REVRANGE) && (pWizard->m_revRangeArray.GetCount() == 0));

    CString sTitle;
    switch (pWizard->m_nRevRangeMerge)
    {
        case MERGEWIZARD_REVRANGE:
            sTitle.LoadString(IDS_MERGEWIZARD_REVRANGETITLE);
            break;
        case MERGEWIZARD_TREE:
            sTitle.LoadString(IDS_MERGEWIZARD_TREETITLE);
            break;
    }
    sTitle += L" : " + CString(MAKEINTRESOURCE(IDS_MERGEWIZARD_OPTIONSTITLE));
    SetDlgItemText(IDC_MERGEOPTIONSGROUP, sTitle);

    return CMergeWizardBasePage::OnSetActive();
}

void CMergeWizardOptions::OnBnClickedDryrun()
{
    UpdateData();
    CMergeWizard*   pWizard = static_cast<CMergeWizard*>(GetParent());
    CSVNProgressDlg progDlg;
    progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
    int options = ProgOptDryRun;
    options |= pWizard->m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
    options |= pWizard->m_bRecordOnly ? ProgOptRecordOnly : 0;
    options |= pWizard->m_bForce ? ProgOptForce : 0;
    options |= pWizard->m_bAllowMixed ? ProgOptAllowMixedRev : 0;
    progDlg.SetOptions(options);
    progDlg.SetPathList(CTSVNPathList(pWizard->m_wcPath));
    progDlg.SetUrl(pWizard->m_url1);
    progDlg.SetSecondUrl(pWizard->m_url2);

    switch (pWizard->m_nRevRangeMerge)
    {
        case MERGEWIZARD_REVRANGE:
        {
            if (pWizard->m_revRangeArray.GetCount())
            {
                SVNRevRangeArray tempRevArray = pWizard->m_revRangeArray;
                tempRevArray.AdjustForMerge(!!pWizard->m_bReverseMerge);
                progDlg.SetRevisionRanges(tempRevArray);
            }
            else
            {
                SVNRevRangeArray tempRevArray;
                tempRevArray.AddRevRange(1, SVNRev::REV_HEAD);
                progDlg.SetRevisionRanges(tempRevArray);
            }
            if (pWizard->m_bReintegrate)
                progDlg.SetCommand(CSVNProgressDlg::SVNProgress_MergeReintegrateOldStyle);
        }
        break;
        case MERGEWIZARD_TREE:
        {
            progDlg.SetRevision(pWizard->m_startRev);
            progDlg.SetRevisionEnd(pWizard->m_endRev);
            if (pWizard->m_url1.Compare(pWizard->m_url2) == 0)
            {
                SVNRevRangeArray tempRevArray;
                tempRevArray.AdjustForMerge(!!pWizard->m_bReverseMerge);
                tempRevArray.AddRevRange(pWizard->m_startRev, pWizard->m_endRev);
                progDlg.SetRevisionRanges(tempRevArray);
            }
        }
        break;
    }

    progDlg.SetDepth(pWizard->m_depth);
    pWizard->m_ignoreSpaces = GetIgnores();
    progDlg.SetDiffOptions(SVN::GetOptionsString(!!pWizard->m_bIgnoreEOL, pWizard->m_ignoreSpaces));
    progDlg.DoModal();
}

BOOL CMergeWizardOptions::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    return CMergeWizardBasePage::PreTranslateMessage(pMsg);
}

svn_diff_file_ignore_space_t CMergeWizardOptions::GetIgnores() const
{
    svn_diff_file_ignore_space_t ignores = svn_diff_file_ignore_space_none;

    int rb = GetCheckedRadioButton(IDC_COMPAREWHITESPACES, IDC_IGNOREALLWHITESPACES);
    switch (rb)
    {
        case IDC_IGNOREWHITESPACECHANGES:
            ignores = svn_diff_file_ignore_space_change;
            break;
        case IDC_IGNOREALLWHITESPACES:
            ignores = svn_diff_file_ignore_space_all;
            break;
        case IDC_COMPAREWHITESPACES:
        default:
            ignores = svn_diff_file_ignore_space_none;
            break;
    }

    return ignores;
}

void CMergeWizardOptions::OnBnClickedReintegrateoldstyle()
{
    UpdateData();
    CMergeWizard* pWizard = static_cast<CMergeWizard*>(GetParent());
    if (pWizard)
        GetDlgItem(IDC_RECORDONLY)->EnableWindow(!pWizard->m_bReintegrate);
}
