// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2015 - TortoiseSVN

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
#include "InputLogDlg.h"
#include "registry.h"
#include "HistoryDlg.h"
#include "RegHistory.h"
#include "AppUtils.h"
#include "BstrSafeVector.h"
#include "COMError.h"
#include "Hooks.h"

// CInputLogDlg dialog

IMPLEMENT_DYNAMIC(CInputLogDlg, CResizableStandAloneDialog)

CInputLogDlg::CInputLogDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CInputLogDlg::IDD, pParent)
    , m_pProjectProperties(NULL)
    , m_iCheck(0)
    , m_bLock(false)
{

}

CInputLogDlg::~CInputLogDlg()
{
}

void CInputLogDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_INPUTTEXT, m_cInput);
    DDX_Check(pDX, IDC_CHECKBOX, m_iCheck);
    DDX_Text(pDX, IDC_BUGID, m_sBugID);
}


BEGIN_MESSAGE_MAP(CInputLogDlg, CResizableStandAloneDialog)
    ON_EN_CHANGE(IDC_INPUTTEXT, OnEnChangeLogmessage)
    ON_BN_CLICKED(IDC_HISTORY, &CInputLogDlg::OnBnClickedHistory)
    ON_BN_CLICKED(IDC_BUGTRAQBUTTON, &CInputLogDlg::OnBnClickedBugtraqbutton)
END_MESSAGE_MAP()


BOOL CInputLogDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_GROUPBOX);
    m_aeroControls.SubclassControl(this, IDC_CHECKBOX);
    m_aeroControls.SubclassOkCancel(this);

#ifdef DEBUG
    if (m_pProjectProperties == NULL)
        TRACE("InputLogDlg: project properties not set\n");
    if (m_sActionText.IsEmpty())
        TRACE("InputLogDlg: action text not set\n");
    if (m_sUUID.IsEmpty())
        TRACE("InputLogDlg: repository UUID not set\n");
#endif

    if (m_pProjectProperties)
        m_cInput.Init(*m_pProjectProperties);
    else
        m_cInput.Init();

    m_cInput.SetFont((CString)CRegString(L"Software\\TortoiseSVN\\LogFontName", L"Courier New"), (DWORD)CRegDWORD(L"Software\\TortoiseSVN\\LogFontSize", 8));

    if (m_pProjectProperties)
    {
        if (m_pProjectProperties->nLogWidthMarker)
        {
            m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
            m_cInput.Call(SCI_SETEDGEMODE, EDGE_LINE);
            m_cInput.Call(SCI_SETEDGECOLUMN, m_pProjectProperties->nLogWidthMarker);
        }
        else
        {
            m_cInput.Call(SCI_SETEDGEMODE, EDGE_NONE);
            m_cInput.Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
        }
        m_cInput.SetText(m_pProjectProperties->GetLogMsgTemplate(m_sSVNAction));
    }

    UpdateOKButton();

    CAppUtils::SetAccProperty(m_cInput.GetSafeHwnd(), PROPID_ACC_ROLE, ROLE_SYSTEM_TEXT);
    CAppUtils::SetAccProperty(m_cInput.GetSafeHwnd(), PROPID_ACC_HELP, CString(MAKEINTRESOURCE(IDS_INPUT_ENTERLOG)));

    SetDlgItemText(IDC_ACTIONLABEL, m_sActionText);
    if (!m_sTitleText.IsEmpty())
        SetWindowText(m_sTitleText);
    if (!m_sCheckText.IsEmpty())
    {
        SetDlgItemText(IDC_CHECKBOX, m_sCheckText);
        GetDlgItem(IDC_CHECKBOX)->ShowWindow(SW_SHOW);
    }
    else
    {
        GetDlgItem(IDC_CHECKBOX)->ShowWindow(SW_HIDE);
    }
    if (!m_sLogMsg.IsEmpty())
        m_cInput.SetText(m_sLogMsg);

    if (m_pProjectProperties && (!m_rootpath.IsEmpty() || m_pathlist.GetCount()))
    {
        CBugTraqAssociations bugtraq_associations;
        bugtraq_associations.Load(m_pProjectProperties->GetProviderUUID(), m_pProjectProperties->sProviderParams);

        bool bExtendUrlControl = true;
        bool bFound = bugtraq_associations.FindProvider(m_pathlist, &m_bugtraq_association);
        if (!bFound)
            bFound = bugtraq_associations.FindProvider(CTSVNPathList(m_rootpath), &m_bugtraq_association);
        if (bFound)
        {
            CComPtr<IBugTraqProvider> pProvider;
            HRESULT hr = pProvider.CoCreateInstance(m_bugtraq_association.GetProviderClass());
            if (SUCCEEDED(hr))
            {
                m_BugTraqProvider = pProvider;
                ATL::CComBSTR temp;
                ATL::CComBSTR parameters;
                parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
                hr = pProvider->GetLinkText(GetSafeHwnd(), parameters, &temp);
                if (SUCCEEDED(hr))
                {
                    SetDlgItemText(IDC_BUGTRAQBUTTON, temp == 0 ? L"" : temp);
                    GetDlgItem(IDC_BUGTRAQBUTTON)->ShowWindow(SW_SHOW);
                    bExtendUrlControl = false;
                }
            }

            m_cInput.SetFocus();
        }
        if (bExtendUrlControl)
            CAppUtils::ExtendControlOverHiddenControl(this, IDC_ACTIONLABEL, IDC_BUGTRAQBUTTON);

        if (!m_pProjectProperties->sMessage.IsEmpty())
        {
            GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
            GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
            if (!m_pProjectProperties->sLabel.IsEmpty())
                SetDlgItemText(IDC_BUGIDLABEL, m_pProjectProperties->sLabel);
            GetDlgItem(IDC_BUGID)->SetFocus();
            CString sBugID = m_pProjectProperties->GetBugIDFromLog(m_sLogMsg);
            if (!sBugID.IsEmpty())
            {
                SetDlgItemText(IDC_BUGID, sBugID);
            }
        }
        else
        {
            GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
            m_cInput.SetFocus();
        }
    }
    else
    {
        GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
        CAppUtils::ExtendControlOverHiddenControl(this, IDC_ACTIONLABEL, IDC_BUGTRAQBUTTON);
        m_cInput.SetFocus();
    }

    m_bLock = m_sSVNAction.Compare(PROJECTPROPNAME_LOGTEMPLATELOCK)==0;

    AddAnchor(IDC_ACTIONLABEL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
    AddAnchor(IDC_BUGID, TOP_RIGHT);
    AddAnchor(IDC_BUGTRAQBUTTON, TOP_RIGHT);
    AddAnchor(IDC_GROUPBOX, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_HISTORY, TOP_LEFT);
    AddAnchor(IDC_INPUTTEXT, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_CHECKBOX, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    EnableSaveRestore(L"InputLogDlg");
    if (FindParentWindow(0))
        CenterWindow(CWnd::FromHandle(FindParentWindow(0)));

    m_cInput.SetFocus();
    return FALSE;
}

void CInputLogDlg::OnOK()
{
    UpdateData();
    m_sLogMsg = m_cInput.GetText();

    if (m_pProjectProperties)
    {
        CString id;
        GetDlgItemText(IDC_BUGID, id);
        id.Trim(L"\n\r");
        if (!m_bLock && !m_pProjectProperties->CheckBugID(id))
        {
            ShowEditBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS, TTI_ERROR);
            return;
        }
        if (!m_bLock && (m_pProjectProperties->bWarnIfNoIssue) && (id.IsEmpty() && !m_pProjectProperties->HasBugID(m_sLogMsg)))
        {
            CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK1)),
                                CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK3)));
            taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_COMMITDLG_WARNNOISSUE_TASK4)));
            taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskdlg.SetDefaultCommandControl(2);
            taskdlg.SetMainIcon(TD_WARNING_ICON);
            if (taskdlg.DoModal(m_hWnd) != 1)
                return;
        }
        m_sBugID.Trim();
        CString sExistingBugID = m_pProjectProperties->FindBugID(m_sLogMsg);
        sExistingBugID.Trim();
        if (!m_sBugID.IsEmpty() && m_sBugID.Compare(sExistingBugID))
        {
            m_sBugID.Replace(L", ", L",");
            m_sBugID.Replace(L" ,", L",");
            CString sBugID = m_pProjectProperties->sMessage;
            sBugID.Replace(L"%BUGID%", m_sBugID);
            if (m_pProjectProperties->bAppend)
                m_sLogMsg += L"\n" + sBugID + L"\n";
            else
                m_sLogMsg = sBugID + L"\n" + m_sLogMsg;
        }
        if (!m_bLock)
        {
            // now let the bugtraq plugin check the commit message
            CComPtr<IBugTraqProvider2> pProvider2 = NULL;
            if (m_BugTraqProvider)
            {
                HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
                if (SUCCEEDED(hr))
                {
                    ATL::CComBSTR temp;
                    CString common = m_rootpath.GetSVNPathString();
                    ATL::CComBSTR repositoryRoot;
                    repositoryRoot.Attach(common.AllocSysString());
                    ATL::CComBSTR parameters;
                    parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
                    ATL::CComBSTR commonRoot(m_pathlist.GetCommonRoot().GetDirectory().GetWinPath());
                    ATL::CComBSTR commitMessage;
                    commitMessage.Attach(m_sLogMsg.AllocSysString());
                    CBstrSafeVector pathList(m_pathlist.GetCount());

                    for (LONG index = 0; index < m_pathlist.GetCount(); ++index)
                        pathList.PutElement(index, m_pathlist[index].GetSVNPathString());

                    hr = pProvider2->CheckCommit(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, commitMessage, &temp);
                    if (FAILED(hr))
                    {
                        OnComError(hr);
                    }
                    else
                    {
                        CString sError = temp == 0 ? L"" : temp;
                        if (!sError.IsEmpty())
                        {
                            CAppUtils::ReportFailedHook(m_hWnd, sError);
                            return;
                        }
                    }
                }
            }

            CTSVNPathList checkedItems;
            checkedItems.AddPath(m_rootpath);
            DWORD exitcode = 0;
            CString error;
            CHooks::Instance().SetProjectProperties(m_rootpath, *m_pProjectProperties);
            if (CHooks::Instance().CheckCommit(m_hWnd, checkedItems, m_sLogMsg, exitcode, error))
            {
                if (exitcode)
                {
                    CString sErrorMsg;
                    sErrorMsg.Format(IDS_HOOK_ERRORMSG, (LPCWSTR)error);

                    CTaskDialog taskdlg(sErrorMsg,
                                        CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK2)),
                                        L"TortoiseSVN",
                                        0,
                                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                    taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK3)));
                    taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_HOOKFAILED_TASK4)));
                    taskdlg.SetDefaultCommandControl(1);
                    taskdlg.SetMainIcon(TD_ERROR_ICON);
                    bool retry = (taskdlg.DoModal(GetSafeHwnd()) == 1);

                    if (retry)
                        return;
                }
            }
        }
    }

    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCTSTR)m_sUUID);

    CRegHistory history;
    history.Load(reg, L"logmsgs");
    history.AddEntry(m_sLogMsg);
    history.Save();

    CResizableStandAloneDialog::OnOK();
}

void CInputLogDlg::OnBnClickedBugtraqbutton()
{
    CString sMsg = m_cInput.GetText();

    if (m_BugTraqProvider == NULL)
        return;

    ATL::CComBSTR parameters;
    parameters.Attach(m_bugtraq_association.GetParameters().AllocSysString());
    ATL::CComBSTR commonRoot(m_rootpath.GetSVNPathString());
    CBstrSafeVector pathList(m_pathlist.GetCount());

    for (LONG index = 0; index < m_pathlist.GetCount(); ++index)
        pathList.PutElement(index, m_pathlist[index].GetSVNPathString());

    ATL::CComBSTR originalMessage;
    originalMessage.Attach(sMsg.AllocSysString());
    ATL::CComBSTR temp;
    m_revProps.clear();

    // first try the IBugTraqProvider2 interface
    CComPtr<IBugTraqProvider2> pProvider2 = NULL;
    HRESULT hr = m_BugTraqProvider.QueryInterface(&pProvider2);
    bool bugIdOutSet = false;
    if (SUCCEEDED(hr))
    {
        CString common = m_rootpath.GetSVNPathString();
        ATL::CComBSTR repositoryRoot;
        repositoryRoot.Attach(common.AllocSysString());
        ATL::CComBSTR bugIDOut;
        GetDlgItemText(IDC_BUGID, m_sBugID);
        ATL::CComBSTR bugID;
        bugID.Attach(m_sBugID.AllocSysString());
        CBstrSafeVector revPropNames;
        CBstrSafeVector revPropValues;
        if (FAILED(hr = pProvider2->GetCommitMessage2(GetSafeHwnd(), parameters, repositoryRoot, commonRoot, pathList, originalMessage, bugID, &bugIDOut, &revPropNames, &revPropValues, &temp)))
        {
            OnComError(hr);
        }
        else
        {
            if (bugIDOut)
            {
                bugIdOutSet = true;
                m_sBugID = bugIDOut;
                SetDlgItemText(IDC_BUGID, m_sBugID);
            }
            m_cInput.SetText((LPCTSTR)temp);
            BSTR HUGEP *pbRevNames;
            BSTR HUGEP *pbRevValues;

            HRESULT hr1 = SafeArrayAccessData(revPropNames, (void HUGEP**)&pbRevNames);
            if (SUCCEEDED(hr1))
            {
                HRESULT hr2 = SafeArrayAccessData(revPropValues, (void HUGEP**)&pbRevValues);
                if (SUCCEEDED(hr2))
                {
                    if (revPropNames->rgsabound->cElements == revPropValues->rgsabound->cElements)
                    {
                        for (ULONG i = 0; i < revPropNames->rgsabound->cElements; i++)
                        {
                            m_revProps[pbRevNames[i]] = pbRevValues[i];
                        }
                    }
                    SafeArrayUnaccessData(revPropValues);
                }
                SafeArrayUnaccessData(revPropNames);
            }
        }
    }
    else
    {
        // if IBugTraqProvider2 failed, try IBugTraqProvider
        CComPtr<IBugTraqProvider> pProvider = NULL;
        hr = m_BugTraqProvider.QueryInterface(&pProvider);
        if (FAILED(hr))
        {
            OnComError(hr);
            return;
        }

        if (FAILED(hr = pProvider->GetCommitMessage(GetSafeHwnd(), parameters, commonRoot, pathList, originalMessage, &temp)))
        {
            OnComError(hr);
        }
        else
            m_cInput.SetText((LPCTSTR)temp);
    }
    m_sLogMsg = m_cInput.GetText();
    if (!m_pProjectProperties->sMessage.IsEmpty())
    {
        CString sBugID = m_pProjectProperties->FindBugID(m_sLogMsg);
        if (!sBugID.IsEmpty() && !bugIdOutSet)
        {
            SetDlgItemText(IDC_BUGID, sBugID);
        }
    }

    m_cInput.SetFocus();
}


BOOL CInputLogDlg::PreTranslateMessage(MSG* pMsg)
{
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

void CInputLogDlg::OnEnChangeLogmessage()
{
    UpdateOKButton();
}

void CInputLogDlg::UpdateOKButton()
{
    CString sTemp = m_cInput.GetText();
    int minsize = 0;
    if (m_pProjectProperties)
        minsize = m_bLock ? m_pProjectProperties->nMinLockMsgSize : m_pProjectProperties->nMinLogSize;
    if (sTemp.GetLength() >= minsize)
    {
        DialogEnableWindow(IDOK, TRUE);
    }
    else
    {
        DialogEnableWindow(IDOK, FALSE);
    }
}

void CInputLogDlg::OnBnClickedHistory()
{
    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCTSTR)m_sUUID);
    CRegHistory history;
    history.Load(reg, L"logmsgs");
    CHistoryDlg HistoryDlg;
    HistoryDlg.SetHistory(history);
    if (HistoryDlg.DoModal()==IDOK)
    {
        if (HistoryDlg.GetSelectedText().Compare(m_cInput.GetText().Left(HistoryDlg.GetSelectedText().GetLength()))!=0)
        {
            if ((m_pProjectProperties)&&(m_pProjectProperties->GetLogMsgTemplate(m_sSVNAction).Compare(m_cInput.GetText())!=0))
                m_cInput.InsertText(HistoryDlg.GetSelectedText(), !m_cInput.GetText().IsEmpty());
            else
                m_cInput.SetText(HistoryDlg.GetSelectedText());
        }

        UpdateOKButton();
        GetDlgItem(IDC_INPUTTEXT)->SetFocus();
    }
}

void CInputLogDlg::OnComError( HRESULT hr )
{
    COMError ce(hr);
    CString sErr;
    sErr.FormatMessage(IDS_ERR_FAILEDISSUETRACKERCOM, m_bugtraq_association.GetProviderName(), ce.GetMessageAndDescription().c_str());
    ::MessageBox(m_hWnd, sErr, L"TortoiseSVN", MB_ICONERROR);
}



void CInputLogDlg::OnCancel()
{
    UpdateData();
    m_sLogMsg = m_cInput.GetText();
    CString reg;
    reg.Format(L"Software\\TortoiseSVN\\History\\commit%s", (LPCTSTR)m_sUUID);

    CRegHistory history;
    history.Load(reg, L"logmsgs");
    history.AddEntry(m_sLogMsg);
    history.Save();

    CResizableStandAloneDialog::OnCancel();
}
