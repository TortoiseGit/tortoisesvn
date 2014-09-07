// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseSVN

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
#include "EditPropConflictDlg.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "AppUtils.h"
#include "SVN.h"
#include "SVNProperties.h"
#include "TempFile.h"
#include "SmartHandle.h"

// CEditPropConflictDlg dialog

IMPLEMENT_DYNAMIC(CEditPropConflictDlg, CResizableStandAloneDialog)

CEditPropConflictDlg::CEditPropConflictDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropConflictDlg::IDD, pParent)
{

}

CEditPropConflictDlg::~CEditPropConflictDlg()
{
}

void CEditPropConflictDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
}

bool CEditPropConflictDlg::SetPrejFile(const CTSVNPath& prejFile)
{
    m_prejFile = prejFile;
    m_sPrejText.Empty();
    // open the prej file to get the info text
    char contentbuf[10000+1];
    FILE * File;
    _tfopen_s(&File, m_prejFile.GetWinPath(), L"rb, ccs=UTF-8");
    if (File == NULL)
    {
        return false;
    }
    for (;;)
    {
        size_t len = fread(contentbuf, sizeof(char), 10000, File);
        m_sPrejText += CUnicodeUtils::GetUnicode(CStringA(contentbuf, (int)len));
        if (len < 10000)
        {
            fclose(File);
            // we've read the whole file
            m_sPrejText.Replace(L"\n", L"\r\n");
            return true;
        }
    }
    //return false;
}

BEGIN_MESSAGE_MAP(CEditPropConflictDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropConflictDlg::OnBnClickedEditprops)
    ON_BN_CLICKED(IDC_STARTMERGEEDITOR, &CEditPropConflictDlg::OnBnClickedStartmergeeditor)
    ON_BN_CLICKED(IDC_RESOLVED, &CEditPropConflictDlg::OnBnClickedResolved)
END_MESSAGE_MAP()


// CEditPropConflictDlg message handlers

BOOL CEditPropConflictDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    CString sInfo;
    sInfo.Format(IDS_PROPCONFLICT_INFO, (LPCTSTR)m_conflictItem.GetFileOrDirectoryName(), (LPCWSTR)m_sPrejText);
    SetDlgItemText(IDC_PROPINFO, sInfo);

    EnableToolTips();
    m_tooltips.Create(this);

    m_tooltips.AddTool(IDC_STARTMERGEEDITOR, IDS_EDITPROPCONFLICT_TT_STARTEDITOR);
    m_tooltips.AddTool(IDC_RESOLVED, IDS_EDITPROPCONFLICT_TT_RESOLVED);
    m_tooltips.AddTool(IDC_EDITPROPS, IDS_EDITPROPCONFLICT_TT_MANUAL);
    

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_conflictItem.GetUIPathString(), sWindowTitle);

    AddAnchor(IDC_PROPINFO, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_STARTMERGEEDITOR, BOTTOM_LEFT);
    AddAnchor(IDC_RESOLVED, BOTTOM_LEFT);
    AddAnchor(IDC_EDITPROPS, BOTTOM_LEFT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditPropConflictDlg::OnBnClickedEditprops()
{
    // start the properties dialog
    CString sCmd;

    sCmd.Format(L"/command:properties /path:\"%s\"",
        (LPCTSTR)m_conflictItem.GetWinPath());
    CAppUtils::RunTortoiseProc(sCmd);

    EndDialog(IDC_EDITPROPS);
}


void CEditPropConflictDlg::SetPropValues(const std::string& propvalue_base, const std::string& propvalue_working, const std::string& propvalue_incoming_old, const std::string& propvalue_incoming_new)
{
    m_value_base = propvalue_base;
    m_value_working = propvalue_working;
    m_value_incomingold = propvalue_incoming_old;
    m_value_incomingnew = propvalue_incoming_new;
}


void CEditPropConflictDlg::OnBnClickedStartmergeeditor()
{
    m_ResultPath = CTempFiles::Instance().GetTempFilePath(true);
    CTSVNPath fBase = CTempFiles::Instance().GetTempFilePath(true);
    CTSVNPath fWorking = CTempFiles::Instance().GetTempFilePath(true);
    CTSVNPath fNew = CTempFiles::Instance().GetTempFilePath(true);
    if (!m_value_incomingold.empty())
        CStringUtils::WriteStringToTextFile(fBase.GetWinPath(), m_value_incomingold);
    else
        CStringUtils::WriteStringToTextFile(fBase.GetWinPath(), m_value_base);

    CStringUtils::WriteStringToTextFile(fWorking.GetWinPath(), m_value_working);
    CStringUtils::WriteStringToTextFile(fNew.GetWinPath(), m_value_incomingnew);
    CStringUtils::WriteStringToTextFile(m_ResultPath.GetWinPath(), m_value_working);

    CString propname, n1, n2, n3, n4;
    propname = CUnicodeUtils::GetUnicode(m_propname.c_str());
    n1.Format(IDS_DIFF_PROP_WCNAME, (LPCTSTR)propname);
    n2.Format(IDS_DIFF_PROP_BASENAME, (LPCTSTR)propname);
    n3.Format(IDS_DIFF_PROP_REMOTENAME, (LPCTSTR)propname);
    n4.Format(IDS_DIFF_PROP_MERGENAME, (LPCTSTR)propname);

    CAppUtils::StartExtMerge(CAppUtils::MergeFlags().AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000)),
                             fBase, fNew, fWorking, m_ResultPath, true, n2, n3, n1, n4, CUnicodeUtils::GetUnicode(m_propname.c_str()));

    GetDlgItem(IDC_RESOLVED)->EnableWindow(TRUE);
}


void CEditPropConflictDlg::OnBnClickedResolved()
{
    // read the content of the result file
    CAutoFile hFile = CreateFile(m_ResultPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile)
    {
        DWORD size = GetFileSize(hFile, NULL);

        // allocate memory to hold file contents
        std::unique_ptr<char[]> buffer(new char[size]);
        DWORD readbytes;
        if (!ReadFile(hFile, buffer.get(), size, &readbytes, NULL))
            return;

        std::string value(buffer.get(), readbytes);

        // Write the property value
        SVNProperties props(m_conflictItem, SVNRev::REV_WC, false, false);
        if (props.Add(m_propname, value))
        {
            // remove the prej file to 'resolve' the conflict
            m_prejFile.Delete(false);
        }
    }

    EndDialog(IDC_RESOLVETHEIRS);
}


BOOL CEditPropConflictDlg::PreTranslateMessage(MSG* pMsg)
{
    m_tooltips.RelayEvent(pMsg);

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}
