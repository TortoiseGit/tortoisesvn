// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014 - TortoiseSVN

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
#include "ImportDlg.h"
#include "RepositoryBrowser.h"
#include "AppUtils.h"
#include "DirFileEnum.h"
#include "BrowseFolder.h"
#include "registry.h"
#include "HistoryDlg.h"

IMPLEMENT_DYNAMIC(CImportDlg, CResizableStandAloneDialog)
CImportDlg::CImportDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CImportDlg::IDD, pParent)
    , m_bIncludeIgnored(FALSE)
    , m_UseAutoprops(TRUE)
{
}

CImportDlg::~CImportDlg()
{
}

void CImportDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
    DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
    DDX_Control(pDX, IDC_MESSAGE, m_cMessage);
    DDX_Check(pDX, IDC_IMPORTIGNORED, m_bIncludeIgnored);
    DDX_Check(pDX, IDC_USEAUTOPROPS, m_UseAutoprops);
}

BEGIN_MESSAGE_MAP(CImportDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_EN_CHANGE(IDC_MESSAGE, OnEnChangeLogmessage)
    ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
    ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CImportDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()

BOOL CImportDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_MSGGROUP);
    m_aeroControls.SubclassControl(this, IDC_IMPORTIGNORED);
    m_aeroControls.SubclassControl(this, IDC_USEAUTOPROPS);
    m_aeroControls.SubclassOkCancelHelp(this);

    m_History.SetMaxHistoryItems((LONG)CRegDWORD(L"Software\\TortoiseSVN\\MaxHistoryItems", 25));

    m_URLCombo.SetURLHistory(true, true);
    m_URLCombo.LoadHistory(L"Software\\TortoiseSVN\\History\\repoURLS", L"url");
    m_URLCombo.SetCurSel(0);
    if (!m_url.IsEmpty())
    {
        m_URLCombo.SetWindowText(m_url);
    }
    else
        CAppUtils::AddClipboardUrlToWindow(m_URLCombo.GetSafeHwnd());

    GetDlgItem(IDC_BROWSE)->EnableWindow(!m_URLCombo.GetString().IsEmpty());

    m_tooltips.Create(this);
    m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);

    m_History.Load(L"Software\\TortoiseSVN\\History\\commit", L"logmsgs");
    m_ProjectProperties.ReadProps(m_path);
    m_cMessage.Init(m_ProjectProperties);
    m_cMessage.SetFont((CString)CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Courier New"), (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8));

    if (!m_sMessage.IsEmpty())
        m_cMessage.SetText(m_sMessage);
    else
        m_cMessage.SetText(m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEIMPORT));

    OnEnChangeLogmessage();

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sWindowTitle);

    CAppUtils::SetAccProperty(m_cMessage.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cMessage.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));

    AdjustControlSize(IDC_IMPORTIGNORED);

    AddAnchor(IDC_STATIC1, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_STATIC4, TOP_LEFT);
    AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BROWSE, TOP_RIGHT);
    AddAnchor(IDC_MSGGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_MESSAGE, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_HISTORY, TOP_LEFT);
    AddAnchor(IDC_IMPORTIGNORED, BOTTOM_LEFT);
    AddAnchor(IDC_USEAUTOPROPS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ImportDlg");
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CImportDlg::OnOK()
{
    if (m_URLCombo.IsWindowEnabled())
    {
        m_URLCombo.SaveHistory();
        m_url = m_URLCombo.GetString();
        UpdateData();
    }

    UpdateData();
    m_sMessage = m_cMessage.GetText();
    if (!m_sMessage.IsEmpty())
    {
        m_History.AddEntry(m_sMessage);
        m_History.Save();
    }

    CResizableStandAloneDialog::OnOK();
}

void CImportDlg::OnBnClickedBrowse()
{
    m_tooltips.Pop();   // hide the tooltips
    SVNRev rev(SVNRev::REV_HEAD);
    CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

BOOL CImportDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if (OnEnterPressed())
                return TRUE;
            break;
        }
    }
    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CImportDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CImportDlg::OnEnChangeLogmessage()
{
    CString sTemp = m_cMessage.GetText();
    DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

void CImportDlg::OnCancel()
{
    UpdateData();
    if (m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEIMPORT).Compare(m_cMessage.GetText()) != 0)
    {
        CString sMsg = m_cMessage.GetText();
        if (!sMsg.IsEmpty())
        {
            m_History.AddEntry(sMsg);
            m_History.Save();
        }
    }
    CResizableStandAloneDialog::OnCancel();
}

void CImportDlg::OnBnClickedHistory()
{
    m_tooltips.Pop();   // hide the tooltips
    SVN svn;
    CHistoryDlg historyDlg;
    historyDlg.SetHistory(m_History);
    if (historyDlg.DoModal()==IDOK)
    {
        if (historyDlg.GetSelectedText().Compare(m_cMessage.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
        {
            if (m_ProjectProperties.GetLogMsgTemplate(PROJECTPROPNAME_LOGTEMPLATEIMPORT).Compare(m_cMessage.GetText())!=0)
                m_cMessage.InsertText(historyDlg.GetSelectedText(), !m_cMessage.GetText().IsEmpty());
            else
                m_cMessage.SetText(historyDlg.GetSelectedText());
        }
        DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cMessage.GetText().GetLength());
    }

}

void CImportDlg::OnCbnEditchangeUrlcombo()
{
    GetDlgItem(IDC_BROWSE)->EnableWindow(!m_URLCombo.GetString().IsEmpty());
}