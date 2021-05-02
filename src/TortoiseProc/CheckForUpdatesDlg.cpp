// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015, 2017-2018, 2021 - TortoiseSVN

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
#include "../version.h"
#include "CheckForUpdatesDlg.h"
#include "registry.h"
#include "AppUtils.h"
#include "TempFile.h"

IMPLEMENT_DYNAMIC(CCheckForUpdatesDlg, CStandAloneDialog)
CCheckForUpdatesDlg::CCheckForUpdatesDlg(CWnd* pParent /*=NULL*/)
    : CStandAloneDialog(CCheckForUpdatesDlg::IDD, pParent)
    , m_bThreadRunning(FALSE)
    , m_bShowInfo(FALSE)
    , m_bVisible(FALSE)
{
    m_sUpdateDownloadLink = L"https://tortoisesvn.net";
}

CCheckForUpdatesDlg::~CCheckForUpdatesDlg()
{
}

void CCheckForUpdatesDlg::DoDataExchange(CDataExchange* pDX)
{
    CStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LINK, m_link);
}

BEGIN_MESSAGE_MAP(CCheckForUpdatesDlg, CStandAloneDialog)
    ON_STN_CLICKED(IDC_CHECKRESULT, OnStnClickedCheckresult)
    ON_WM_TIMER()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CCheckForUpdatesDlg::OnInitDialog()
{
    CStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(0, 0, 0, 0);
    m_aeroControls.SubclassControl(this, IDC_INFO);
    m_aeroControls.SubclassControl(this, IDC_YOURVERSION);
    m_aeroControls.SubclassControl(this, IDC_CURRENTVERSION);
    m_aeroControls.SubclassControl(this, IDC_CHECKRESULT);
    m_aeroControls.SubclassControl(this, IDC_LINK);
    m_aeroControls.SubclassControl(this, IDOK);

    CString temp;
    temp.Format(IDS_CHECKNEWER_YOURVERSION, TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
    SetDlgItemText(IDC_YOURVERSION, temp);

    DialogEnableWindow(IDOK, FALSE);

    if (AfxBeginThread(CheckThreadEntry, this) == nullptr)
    {
        TaskDialog(this->m_hWnd, AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
    }

    SetTimer(100, 1000, nullptr);
    return TRUE;
}

void CCheckForUpdatesDlg::OnOK()
{
    if (m_bThreadRunning)
        return;
    CStandAloneDialog::OnOK();
}

void CCheckForUpdatesDlg::OnCancel()
{
    if (m_bThreadRunning)
        return;
    CStandAloneDialog::OnCancel();
}

UINT CCheckForUpdatesDlg::CheckThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CCheckForUpdatesDlg*>(pVoid)->CheckThread();
}

UINT CCheckForUpdatesDlg::CheckThread()
{
    m_bThreadRunning = TRUE;

    CString temp;
    CString tempFile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();

    CRegString checkUrlUser    = CRegString(L"Software\\TortoiseSVN\\UpdateCheckURL", L"");
    CRegString checkUrlMachine = CRegString(L"Software\\TortoiseSVN\\UpdateCheckURL", L"", FALSE, HKEY_LOCAL_MACHINE);
    CString    sCheckURL       = checkUrlUser;
    if (sCheckURL.IsEmpty())
    {
        sCheckURL = checkUrlMachine;
        if (sCheckURL.IsEmpty())
            sCheckURL = L"https://tortoisesvn.net/version.txt";
    }
    HRESULT res = URLDownloadToFile(nullptr, sCheckURL, tempFile, 0, nullptr);
    if (res == S_OK)
    {
        try
        {
            CStdioFile file(tempFile, CFile::modeRead | CFile::shareDenyWrite);
            CString    ver;
            if (file.ReadString(ver))
            {
                CString verTemp = ver;
                int     major   = _wtoi(verTemp);
                verTemp         = verTemp.Mid(verTemp.Find('.') + 1);
                int minor       = _wtoi(verTemp);
                verTemp         = verTemp.Mid(verTemp.Find('.') + 1);
                int micro       = _wtoi(verTemp);
                verTemp         = verTemp.Mid(verTemp.Find('.') + 1);
                int  build      = _wtoi(verTemp);
                BOOL bNewer     = FALSE;
                if (major > TSVN_VERMAJOR)
                    bNewer = TRUE;
                else if ((minor > TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
                    bNewer = TRUE;
                else if ((micro > TSVN_VERMICRO) && (minor == TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
                    bNewer = TRUE;
                else if ((build > TSVN_VERBUILD) && (micro == TSVN_VERMICRO) && (minor == TSVN_VERMINOR) && (major == TSVN_VERMAJOR))
                    bNewer = TRUE;

                if (_wtoi(ver) != 0)
                {
                    temp.Format(IDS_CHECKNEWER_CURRENTVERSION, static_cast<LPCWSTR>(ver));
                    SetDlgItemText(IDC_CURRENTVERSION, temp);
                    temp.Format(L"%d.%d.%d.%d", TSVN_VERMAJOR, TSVN_VERMINOR, TSVN_VERMICRO, TSVN_VERBUILD);
                }

                if (_wtoi(ver) == 0)
                {
                    temp.LoadString(IDS_CHECKNEWER_NETERROR);
                    SetDlgItemText(IDC_CHECKRESULT, temp);
                }
                else if (bNewer)
                {
                    if (file.ReadString(temp) && !temp.IsEmpty())
                    { // Read the next line, it could contain a message for the user
                        CString    tempLink;
                        CRegString regDownLink(L"Software\\TortoiseSVN\\NewVersionLink");
                        regDownLink = tempLink;
                        if (file.ReadString(tempLink) && !tempLink.IsEmpty())
                        { // Read another line to find out the download link-URL, if any
                            m_sUpdateDownloadLink = tempLink;
                            regDownLink           = m_sUpdateDownloadLink;
                        }
                    }
                    else
                    {
                        temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
                    }
                    CRegString regDownText(L"Software\\TortoiseSVN\\NewVersionText");
                    regDownText = temp;
                    SetDlgItemText(IDC_CHECKRESULT, temp);
                    // only show the dialog for newer versions if the 'old style' update check
                    // is requested. The current update check shows the info in the commit dialog.
                    if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\OldVersionCheck")))
                        m_bShowInfo = TRUE;
                    CRegString regVer(L"Software\\TortoiseSVN\\NewVersion");
                    regVer = ver;
                }
                else
                {
                    temp.LoadString(IDS_CHECKNEWER_YOURUPTODATE);
                    SetDlgItemText(IDC_CHECKRESULT, temp);
                    if (file.ReadString(temp) && !temp.IsEmpty())
                    {
                        // Read the next line, it could contain a message for the user
                        if (file.ReadString(temp) && !temp.IsEmpty())
                        {
                            // Read another line to find out the download link-URL, if any
                            m_sUpdateDownloadLink = temp;
                        }
                    }
                    else
                    {
                        temp.LoadString(IDS_CHECKNEWER_NEWERVERSIONAVAILABLE);
                    }
                }
            }
        }
        catch (CException* e)
        {
            e->Delete();
            temp.LoadString(IDS_CHECKNEWER_NETERROR);
            SetDlgItemText(IDC_CHECKRESULT, temp);
        }
    }
    else
    {
        temp.LoadString(IDS_CHECKNEWER_NETERROR);
        SetDlgItemText(IDC_CHECKRESULT, temp);
    }
    if (!m_sUpdateDownloadLink.IsEmpty())
    {
        m_link.ShowWindow(SW_SHOW);
        m_link.SetURL(m_sUpdateDownloadLink);
    }

    DeleteFile(tempFile);
    m_bThreadRunning = FALSE;
    DialogEnableWindow(IDOK, TRUE);
    return 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CCheckForUpdatesDlg::OnStnClickedCheckresult()
{
    // user clicked on the label, start the browser with our web page
    HINSTANCE result = ShellExecute(nullptr, L"opennew", m_sUpdateDownloadLink, nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= HINSTANCE_ERROR)
    {
        ShellExecute(nullptr, L"open", m_sUpdateDownloadLink, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void CCheckForUpdatesDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 100)
    {
        if (m_bThreadRunning == FALSE)
        {
            if (m_bShowInfo)
            {
                m_bVisible = TRUE;
                ShowWindow(SW_SHOWNORMAL);
            }
            else
            {
                EndDialog(0);
            }
        }
    }
    CStandAloneDialog::OnTimer(nIDEvent);
}

void CCheckForUpdatesDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    CStandAloneDialog::OnWindowPosChanging(lpwndpos);
    if (m_bVisible == FALSE)
        lpwndpos->flags &= ~SWP_SHOWWINDOW;
}

BOOL CCheckForUpdatesDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if ((!m_sUpdateDownloadLink.IsEmpty()) && (pWnd) && (pWnd == GetDlgItem(IDC_CHECKRESULT)))
    {
        HCURSOR hCur = LoadCursor(nullptr, IDC_HAND);
        SetCursor(hCur);
        return TRUE;
    }

    HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(hCur);
    return CStandAloneDialogTmpl<CDialog>::OnSetCursor(pWnd, nHitTest, message);
}