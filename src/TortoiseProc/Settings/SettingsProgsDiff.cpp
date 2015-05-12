// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009, 2013-2015 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "SettingsProgsDiff.h"


IMPLEMENT_DYNAMIC(CSettingsProgsDiff, ISettingsPropPage)
CSettingsProgsDiff::CSettingsProgsDiff()
    : ISettingsPropPage(CSettingsProgsDiff::IDD)
    , m_dlgAdvDiff(L"Diff")
    , m_iExtDiff(0)
    , m_sDiffPath(L"")
    , m_iExtDiffProps(0)
    , m_sDiffPropsPath(L"")
    , m_regConvertBase(L"Software\\TortoiseSVN\\ConvertBase", TRUE)
    , m_bConvertBase(false)
    , m_sDiffViewerPath(L"")
    , m_iDiffViewer(0)
{
    m_regDiffPath = CRegString(L"Software\\TortoiseSVN\\Diff");
    m_regDiffPropsPath = CRegString(L"Software\\TortoiseSVN\\DiffProps");
    m_regDiffViewerPath = CRegString(L"Software\\TortoiseSVN\\DiffViewer");
}

CSettingsProgsDiff::~CSettingsProgsDiff()
{
}

void CSettingsProgsDiff::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EXTDIFF, m_sDiffPath);
    DDX_Radio(pDX, IDC_EXTDIFF_OFF, m_iExtDiff);
    DDX_Text(pDX, IDC_EXTDIFFPROPS, m_sDiffPropsPath);
    DDX_Radio(pDX, IDC_EXTDIFFPROPS_OFF, m_iExtDiffProps);
    DDX_Check(pDX, IDC_DONTCONVERT, m_bConvertBase);

    DialogEnableWindow(IDC_EXTDIFF, m_iExtDiff == 1);
    DialogEnableWindow(IDC_EXTDIFFBROWSE, m_iExtDiff == 1);
    DialogEnableWindow(IDC_EXTDIFFPROPS, m_iExtDiffProps == 1);
    DialogEnableWindow(IDC_EXTDIFFPROPSBROWSE, m_iExtDiffProps == 1);
    DDX_Control(pDX, IDC_EXTDIFF, m_cDiffEdit);
    DDX_Control(pDX, IDC_EXTDIFFPROPS, m_cDiffPropsEdit);

    DDX_Text(pDX, IDC_DIFFVIEWER, m_sDiffViewerPath);
    DDX_Radio(pDX, IDC_DIFFVIEWER_OFF, m_iDiffViewer);

    DialogEnableWindow(IDC_DIFFVIEWER, m_iDiffViewer == 1);
    DialogEnableWindow(IDC_DIFFVIEWERBROWSE, m_iDiffViewer == 1);
    DDX_Control(pDX, IDC_DIFFVIEWER, m_cUnifiedDiffEdit);
}


BEGIN_MESSAGE_MAP(CSettingsProgsDiff, ISettingsPropPage)
    ON_BN_CLICKED(IDC_EXTDIFF_OFF, OnBnClickedExtdiffOff)
    ON_BN_CLICKED(IDC_EXTDIFF_ON, OnBnClickedExtdiffOn)
    ON_BN_CLICKED(IDC_EXTDIFFBROWSE, OnBnClickedExtdiffbrowse)
    ON_BN_CLICKED(IDC_EXTDIFFADVANCED, OnBnClickedExtdiffadvanced)
    ON_BN_CLICKED(IDC_EXTDIFFPROPS_OFF, OnBnClickedExtdiffpropsOff)
    ON_BN_CLICKED(IDC_EXTDIFFPROPS_ON, OnBnClickedExtdiffpropsOn)
    ON_BN_CLICKED(IDC_EXTDIFFPROPSBROWSE, OnBnClickedExtdiffpropsbrowse)
    ON_BN_CLICKED(IDC_DONTCONVERT, OnBnClickedDontconvert)
    ON_EN_CHANGE(IDC_EXTDIFF, OnEnChangeExtdiff)
    ON_EN_CHANGE(IDC_EXTDIFFPROPS, OnEnChangeExtdiffprops)

    ON_BN_CLICKED(IDC_DIFFVIEWER_OFF, OnBnClickedDiffviewerOff)
    ON_BN_CLICKED(IDC_DIFFVIEWER_ON, OnBnClickedDiffviewerOn)
    ON_BN_CLICKED(IDC_DIFFVIEWERBROWSE, OnBnClickedDiffviewerbrowse)
    ON_EN_CHANGE(IDC_DIFFVIEWER, OnEnChangeDiffviewer)
END_MESSAGE_MAP()


BOOL CSettingsProgsDiff::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_sDiffPath = m_regDiffPath;
    m_iExtDiff = IsExternal(m_sDiffPath);

    m_sDiffPropsPath = m_regDiffPropsPath;
    m_iExtDiffProps = IsExternal(m_sDiffPropsPath);

    SHAutoComplete(::GetDlgItem(m_hWnd, IDC_EXTDIFF), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

    m_bConvertBase = m_regConvertBase;

    m_sDiffViewerPath = m_regDiffViewerPath;
    m_iDiffViewer = IsExternal(m_sDiffViewerPath);

    SHAutoComplete(::GetDlgItem(m_hWnd, IDC_DIFFVIEWER), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

    m_tooltips.AddTool(IDC_EXTDIFF, IDS_SETTINGS_EXTDIFF_TT);
    m_tooltips.AddTool(IDC_DONTCONVERT, IDS_SETTINGS_CONVERTBASE_TT);
    m_tooltips.AddTool(IDC_DIFFVIEWER, IDS_SETTINGS_DIFFVIEWER_TT);

    UpdateData(FALSE);
    return TRUE;
}

BOOL CSettingsProgsDiff::OnApply()
{
    UpdateData();
    if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != L"#")
    {
        m_sDiffPath = L"#" + m_sDiffPath;
    }
    m_regDiffPath = m_sDiffPath;

    if (m_iExtDiffProps == 0 && !m_sDiffPropsPath.IsEmpty() && m_sDiffPropsPath.Left(1) != L"#")
    {
        m_sDiffPropsPath = L"#" + m_sDiffPropsPath;
    }
    m_regDiffPropsPath = m_sDiffPropsPath;

    m_regConvertBase = m_bConvertBase;
    m_dlgAdvDiff.SaveData();

    if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != L"#")
        m_sDiffViewerPath = L"#" + m_sDiffViewerPath;

    m_regDiffViewerPath = m_sDiffViewerPath;

    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOff()
{
    m_iExtDiff = 0;
    SetModified();
    DialogEnableWindow(IDC_EXTDIFF, false);
    DialogEnableWindow(IDC_EXTDIFFBROWSE, false);
    CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffpropsOff()
{
    m_iExtDiffProps = 0;
    SetModified();
    DialogEnableWindow(IDC_EXTDIFFPROPS, false);
    DialogEnableWindow(IDC_EXTDIFFPROPSBROWSE, false);
    CheckProgCommentProps();
}

void CSettingsProgsDiff::OnBnClickedExtdiffOn()
{
    m_iExtDiff = 1;
    SetModified();
    DialogEnableWindow(IDC_EXTDIFF, true);
    DialogEnableWindow(IDC_EXTDIFFBROWSE, true);
    GetDlgItem(IDC_EXTDIFF)->SetFocus();
    CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedExtdiffpropsOn()
{
    m_iExtDiffProps = 1;
    SetModified();
    DialogEnableWindow(IDC_EXTDIFFPROPS, true);
    DialogEnableWindow(IDC_EXTDIFFPROPSBROWSE, true);
    GetDlgItem(IDC_EXTDIFFPROPS)->SetFocus();
    CheckProgCommentProps();
}

void CSettingsProgsDiff::OnBnClickedExtdiffbrowse()
{
    if (CAppUtils::FileOpenSave(m_sDiffPath, NULL, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, CString(), m_hWnd))
    {
        UpdateData(FALSE);
        SetModified();
    }
}

void CSettingsProgsDiff::OnBnClickedExtdiffpropsbrowse()
{
    if (CAppUtils::FileOpenSave(m_sDiffPropsPath, NULL, IDS_SETTINGS_SELECTDIFF, IDS_PROGRAMSFILEFILTER, true, CString(), m_hWnd))
    {
        UpdateData(FALSE);
        SetModified();
    }
}

void CSettingsProgsDiff::OnBnClickedExtdiffadvanced()
{
    if (m_dlgAdvDiff.DoModal() == IDOK)
        SetModified();
}

void CSettingsProgsDiff::OnBnClickedDontconvert()
{
    SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiff()
{
    SetModified();
}

void CSettingsProgsDiff::OnEnChangeExtdiffprops()
{
    SetModified();
}

void CSettingsProgsDiff::CheckProgComment()
{
    UpdateData();
    if (m_iExtDiff == 0 && !m_sDiffPath.IsEmpty() && m_sDiffPath.Left(1) != L"#")
        m_sDiffPath = L"#" + m_sDiffPath;
    else if (m_iExtDiff == 1)
        m_sDiffPath.TrimLeft('#');

    if (m_iDiffViewer == 0 && !m_sDiffViewerPath.IsEmpty() && m_sDiffViewerPath.Left(1) != L"#")
        m_sDiffViewerPath = L"#" + m_sDiffViewerPath;
    else if (m_iDiffViewer == 1)
        m_sDiffViewerPath.TrimLeft('#');
    UpdateData(FALSE);
}

void CSettingsProgsDiff::CheckProgCommentProps()
{
    UpdateData();
    if (m_iExtDiffProps == 0 && !m_sDiffPropsPath.IsEmpty() && m_sDiffPropsPath.Left(1) != L"#")
        m_sDiffPropsPath = L"#" + m_sDiffPropsPath;
    else if (m_iExtDiffProps == 1)
        m_sDiffPropsPath.TrimLeft('#');
    UpdateData(FALSE);
}

void CSettingsProgsDiff::OnBnClickedDiffviewerOff()
{
    m_iDiffViewer = 0;
    SetModified();
    DialogEnableWindow(IDC_DIFFVIEWER, false);
    DialogEnableWindow(IDC_DIFFVIEWERBROWSE, false);
    CheckProgComment();
}

void CSettingsProgsDiff::OnBnClickedDiffviewerOn()
{
    m_iDiffViewer = 1;
    SetModified();
    DialogEnableWindow(IDC_DIFFVIEWER, true);
    DialogEnableWindow(IDC_DIFFVIEWERBROWSE, true);
    GetDlgItem(IDC_DIFFVIEWER)->SetFocus();
    CheckProgComment();
}

void CSettingsProgsDiff::OnEnChangeDiffviewer()
{
    SetModified();
}

void CSettingsProgsDiff::OnBnClickedDiffviewerbrowse()
{
    if (CAppUtils::FileOpenSave(m_sDiffViewerPath, NULL, IDS_SETTINGS_SELECTDIFFVIEWER, IDS_PROGRAMSFILEFILTER, true, CString(), m_hWnd))
    {
        UpdateData(FALSE);
        SetModified();
    }
}
