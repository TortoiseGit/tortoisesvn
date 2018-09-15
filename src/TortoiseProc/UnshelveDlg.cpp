// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017-2018 - TortoiseSVN

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
#include "UnshelveDlg.h"
#include "SVN.h"
#include "AppUtils.h"
#include "SysImageList.h"

#define REFRESHTIMER 100

IMPLEMENT_DYNAMIC(CUnshelve, CResizableStandAloneDialog)
CUnshelve::CUnshelve(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CUnshelve::IDD, pParent)
    , m_nIconFolder(0)
    , m_Version(-1)
{
}

CUnshelve::~CUnshelve()
{
}

void CUnshelve::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SHELVENAME, m_cShelvesCombo);
    DDX_Control(pDX, IDC_VERSIONCOMBO, m_cVersionCombo);
    DDX_Control(pDX, IDC_FILELIST, m_cFileList);
    DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
}

BEGIN_MESSAGE_MAP(CUnshelve, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_CBN_SELCHANGE(IDC_SHELVENAME, &CUnshelve::OnCbnSelchangeShelvename)
    ON_CBN_SELCHANGE(IDC_VERSIONCOMBO, &CUnshelve::OnCbnSelchangeVersioncombo)
    ON_NOTIFY(LVN_GETDISPINFO, IDC_FILELIST, &CUnshelve::OnLvnGetdispinfoFilelist)
END_MESSAGE_MAP()

BOOL CUnshelve::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    m_cLogMessage.Init();
    m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
    std::map<int, UINT> icons;
    icons[AUTOCOMPLETE_SPELLING]    = IDI_SPELL;
    icons[AUTOCOMPLETE_FILENAME]    = IDI_FILE;
    icons[AUTOCOMPLETE_PROGRAMCODE] = IDI_CODE;
    icons[AUTOCOMPLETE_SNIPPET]     = IDI_SNIPPET;
    m_cLogMessage.SetIcon(icons);
    m_cLogMessage.SetReadOnly(true);

    m_cFileList.SetRedraw(false);
    m_cFileList.DeleteAllItems();
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_cFileList.SetExtendedStyle(exStyle);

    m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
    m_cFileList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
    int c = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_cFileList.DeleteColumn(c--);
    m_cFileList.InsertColumn(0, CString(MAKEINTRESOURCE(IDS_SHELF_UNSHELVE_FILECOL)));
    //m_cFileList.InsertColumn(1, CString(MAKEINTRESOURCE(IDS_SHELF_UNSHELVE_STATUSCOL)));

    int maxcol = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = 0; col <= maxcol; col++)
    {
        m_cFileList.SetColumnWidth(col, LVSCW_AUTOSIZE);
    }
    m_cFileList.SetRedraw(true);

    std::vector<CString> Names;
    m_svn.ShelvesList(Names, m_pathList.GetCommonRoot());
    for (const auto& name : Names)
    {
        m_cShelvesCombo.AddString(name);
    }
    m_cShelvesCombo.SetCurSel(0);
    OnCbnSelchangeShelvename();

    UpdateData(FALSE);

    AddAnchor(IDC_NAMELABEL, TOP_LEFT);
    AddAnchor(IDC_SHELVENAME, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_VERSIONLABEL, TOP_LEFT);
    AddAnchor(IDC_VERSIONCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LOGMSGLABEL, TOP_LEFT);
    AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FILESLABEL, TOP_LEFT);
    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"UnshelveDlg");

    return TRUE;
}

void CUnshelve::OnBnClickedHelp()
{
    OnHelp();
}

void CUnshelve::OnCancel()
{
    CResizableStandAloneDialog::OnCancel();
}

void CUnshelve::OnOK()
{
    CResizableStandAloneDialog::OnOK();

    int selectedName = m_cShelvesCombo.GetCurSel();
    if (selectedName >= 0)
    {
        m_cShelvesCombo.GetLBText(selectedName, m_sShelveName);
    }
    auto vSel = m_cVersionCombo.GetCurSel();
    if (vSel != CB_ERR)
    {
        m_Version = vSel + 1;
    }
}

void CUnshelve::OnCbnSelchangeShelvename()
{
    auto sel = m_cShelvesCombo.GetCurSel();
    if (sel != CB_ERR)
    {
        CString shelfName;
        m_cShelvesCombo.GetLBText(sel, shelfName);
        // get the info for the selected shelf
        m_currentShelfInfo = m_svn.GetShelfInfo(shelfName, m_pathList.GetCommonRoot());
        m_cLogMessage.SetText(m_currentShelfInfo.LogMessage);
        int v = 0;
        m_cVersionCombo.ResetContent();
        for (const auto& version : m_currentShelfInfo.versions)
        {
            wchar_t date_native[SVN_DATE_BUFFER] = {0};
            SVN::formatDate(date_native, std::get<0>(version), true);

            CString sVersion;
            sVersion.Format(IDS_SHELF_VERSION_TIME, v + 1, date_native);
            m_cVersionCombo.AddString(sVersion);
            ++v;
        }
        m_cVersionCombo.SetCurSel(v - 1);
        OnCbnSelchangeVersioncombo();
    }
    else
        m_currentShelfInfo = ShelfInfo();
}

void CUnshelve::OnCbnSelchangeVersioncombo()
{
    auto sel = m_cVersionCombo.GetCurSel();
    if ((sel != CB_ERR) && (sel < m_currentShelfInfo.versions.size()))
    {
        auto version   = m_currentShelfInfo.versions[sel];
        m_currentFiles = std::get<1>(version);
    }
    else
        m_currentFiles.Clear();
    m_cFileList.SetItemCount(m_currentFiles.GetCount());
    if (m_currentFiles.GetCount() > 0)
    {
        int maxcol = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
        for (int col = 0; col <= maxcol; col++)
        {
            m_cFileList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
        }
    }
    m_cFileList.Invalidate();
}

void CUnshelve::OnLvnGetdispinfoFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    if (pDispInfo)
    {
        if (pDispInfo->item.iItem < (int)m_currentFiles.GetCount())
        {
            const auto& path = m_currentFiles[pDispInfo->item.iItem];
            if (pDispInfo->item.mask & LVIF_TEXT)
            {
                switch (pDispInfo->item.iSubItem)
                {
                    case 0: // path
                    {
                        lstrcpyn(m_columnbuf, path.GetSVNPathString(), pDispInfo->item.cchTextMax - 1);
                        int cWidth = m_cFileList.GetColumnWidth(0);
                        cWidth     = max(28, cWidth - 28);
                        CDC* pDC   = m_cFileList.GetDC();
                        if (pDC != NULL)
                        {
                            CFont* pFont = pDC->SelectObject(m_cFileList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnbuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                    case 1: // status
                        m_columnbuf[0] = 0;
                        // we could show the status of the corresponding file in the working copy,
                        // but do we really need that info?
                        // I prefer to do a forced unshelve even if there are conflicts -
                        // the user can resolve the conflicts easy enough.
                        break;
                    default:
                        m_columnbuf[0] = 0;
                }
                pDispInfo->item.pszText = m_columnbuf;
            }
            if (pDispInfo->item.mask & LVIF_IMAGE)
            {
                auto icon_idx          = SYS_IMAGE_LIST().GetPathIconIndex(path);
                pDispInfo->item.iImage = icon_idx;
            }
        }
    }
    *pResult = 0;
}
