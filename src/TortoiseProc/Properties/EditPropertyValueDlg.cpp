// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "EditPropertyValueDlg.h"
#include "auto_buffer.h"
#include "SmartHandle.h"

IMPLEMENT_DYNAMIC(CEditPropertyValueDlg, CResizableStandAloneDialog)

CEditPropertyValueDlg::CEditPropertyValueDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropertyValueDlg::IDD, pParent)
    , EditPropBase()
    , m_sPropValue(_T(""))
{
}

CEditPropertyValueDlg::~CEditPropertyValueDlg()
{
}

void CEditPropertyValueDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROPNAMECOMBO, m_PropNames);
    DDX_Text(pDX, IDC_PROPVALUE, m_sPropValue);
    DDX_Check(pDX, IDC_PROPRECURSIVE, m_bRecursive);
}


BEGIN_MESSAGE_MAP(CEditPropertyValueDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CEditPropertyValueDlg::OnBnClickedHelp)
    ON_CBN_SELCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
    ON_CBN_EDITCHANGE(IDC_PROPNAMECOMBO, &CEditPropertyValueDlg::CheckRecursive)
    ON_BN_CLICKED(IDC_LOADPROP, &CEditPropertyValueDlg::OnBnClickedLoadprop)
    ON_EN_CHANGE(IDC_PROPVALUE, &CEditPropertyValueDlg::OnEnChangePropvalue)
END_MESSAGE_MAP()

BOOL CEditPropertyValueDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_PROPVALUEGROUP);
    m_aeroControls.SubclassControl(this, IDC_PROPRECURSIVE);
    m_aeroControls.SubclassOkCancelHelp(this);

    CString resToken;

    // get the property values for user defined property files
    m_ProjectProperties.ReadPropsPathList(m_pathList);

    bool bFound = false;

    // fill the combobox control with all the
    // known properties
    if (!m_bRevProps)
    {
        int curPos = 0;
        m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_EOL_STYLE));
        m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_EXECUTABLE));
        if ((m_bFolder)||(m_bMultiple))
            m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_EXTERNALS));
        if ((m_bFolder)||(m_bMultiple))
            m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_IGNORE));
        m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_KEYWORDS));
        m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_NEEDS_LOCK));
        m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_MIME_TYPE));
        if ((m_bFolder)||(m_bMultiple))
            m_PropNames.AddString(CUnicodeUtils::GetUnicode(SVN_PROP_MERGEINFO));
        if (!m_ProjectProperties.sFPPath.IsEmpty())
        {
            resToken = m_ProjectProperties.sFPPath.Tokenize(_T("\n"),curPos);
            while (resToken != "")
            {
                m_PropNames.AddString(resToken);
                resToken = m_ProjectProperties.sFPPath.Tokenize(_T("\n"),curPos);
            }
        }

        if ((m_bFolder)||(m_bMultiple))
        {
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_URL));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_LOGREGEX));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_LABEL));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_MESSAGE));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_NUMBER));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_WARNIFNOISSUE));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_APPEND));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_PROVIDERUUID));
            m_PropNames.AddString(_T(BUGTRAQPROPNAME_PROVIDERPARAMS));

            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATECOMMIT));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEBRANCH));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEIMPORT));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEDEL));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEMOVE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEMKDIR));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATEPROPSET));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGTEMPLATELOCK));

            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGWIDTHLINE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGMINSIZE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOCKMSGMINSIZE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGFILELISTLANG));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGSUMMARY));
            m_PropNames.AddString(_T(PROJECTPROPNAME_PROJECTLANGUAGE));
            m_PropNames.AddString(_T(PROJECTPROPNAME_USERFILEPROPERTY));
            m_PropNames.AddString(_T(PROJECTPROPNAME_USERDIRPROPERTY));
            m_PropNames.AddString(_T(PROJECTPROPNAME_AUTOPROPS));
            m_PropNames.AddString(_T(PROJECTPROPNAME_LOGREVREGEX));

            m_PropNames.AddString(_T(PROJECTPROPNAME_WEBVIEWER_REV));
            m_PropNames.AddString(_T(PROJECTPROPNAME_WEBVIEWER_PATHREV));

            if (!m_ProjectProperties.sDPPath.IsEmpty())
            {
                curPos = 0;
                resToken = m_ProjectProperties.sDPPath.Tokenize(_T("\n"),curPos);

                while (resToken != "")
                {
                    m_PropNames.AddString(resToken);
                    resToken = m_ProjectProperties.sDPPath.Tokenize(_T("\n"),curPos);
                }
            }
        }
        else
            GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(FALSE);

        // select the pre-set property in the combobox
        for (int i=0; i<m_PropNames.GetCount(); ++i)
        {
            CString sText;
            m_PropNames.GetLBText(i, sText);
            if (m_sPropName.Compare(sText)==0)
            {
                m_PropNames.SetCurSel(i);
                bFound = true;
                break;
            }
        }
    }

    GetDlgItem(IDC_PROPNAMECOMBO)->EnableToolTips();

    m_tooltips.Create(this);

    UpdateData(FALSE);
    CheckRecursive();
    m_bChanged = false;

    if (!m_sTitle.IsEmpty())
        SetWindowText(m_sTitle);

    CAppUtils::CreateFontForLogs(m_valueFont);
    GetDlgItem(IDC_PROPVALUE)->SetFont(&m_valueFont);

    AdjustControlSize(IDC_PROPRECURSIVE);

    GetDlgItem(IDC_PROPRECURSIVE)->EnableWindow(m_bFolder || m_bMultiple);
    GetDlgItem(IDC_PROPRECURSIVE)->ShowWindow(m_bRevProps ? SW_HIDE : SW_SHOW);

    AddAnchor(IDC_PROPNAME, TOP_LEFT, TOP_CENTER);
    AddAnchor(IDC_PROPNAMECOMBO, TOP_CENTER, TOP_RIGHT);
    AddAnchor(IDC_PROPVALUEGROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PROPVALUE, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_LOADPROP, BOTTOM_RIGHT);
    AddAnchor(IDC_PROPRECURSIVE, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    EnableSaveRestore(_T("EditPropertyValueDlg"));

    if (!bFound)
    {
        m_PropNames.SetCurSel(CB_ERR);
        m_PropNames.SetWindowText(m_sPropName);
    }

    if (!m_sPropValue.IsEmpty())
    {
        GetDlgItem(IDC_PROPVALUE)->SetFocus();
        return FALSE;
    }
    return TRUE;
}

void CEditPropertyValueDlg::SetPropertyName(const std::string& sName)
{
    m_sPropName = CUnicodeUtils::UTF8ToUTF16 (sName);
}

void CEditPropertyValueDlg::SetPropertyValue(const std::string& sValue)
{
    if (SVNProperties::IsBinary(sValue))
    {
        m_bIsBinary = true;
        m_sPropValue.LoadString(IDS_EDITPROPS_BINVALUE);
    }
    else
    {
        m_bIsBinary = false;
        m_sPropValue = CUnicodeUtils::UTF8ToUTF16 (sValue);
        m_sPropValue.Replace(_T("\n"), _T("\r\n"));
    }
}

void CEditPropertyValueDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CEditPropertyValueDlg::OnCancel()
{
    m_sPropName.Empty();
    CDialog::OnCancel();
}

void CEditPropertyValueDlg::OnOK()
{
    UpdateData();
    if (!m_bIsBinary)
    {
        m_sPropValue.Replace(_T("\r\n"), _T("\n"));
        m_sPropValue.Replace(_T("\n\n"), _T("\n"));
        m_PropValue = CUnicodeUtils::StdGetUTF8((LPCTSTR)m_sPropValue);
    }
    m_PropNames.GetWindowText(m_sPropName);
    m_PropName = CUnicodeUtils::StdGetUTF8((LPCTSTR)m_sPropName);
    CDialog::OnOK();
}

void CEditPropertyValueDlg::CheckRecursive()
{
    // some properties can only be applied to files
    // if the properties are edited for a folder or
    // multiple items, then such properties must be
    // applied recursively.
    // Here, we check the property the user selected
    // and check the "recursive" checkbox automatically
    // if it needs to be set.
    int idx = m_PropNames.GetCurSel();
    if (idx >= 0)
    {
        CString sName;
        m_PropNames.GetLBText(idx, sName);
        std::string nameUTF8 = CUnicodeUtils::StdGetUTF8((LPCTSTR)sName);
        if ((m_bFolder)||(m_bMultiple))
        {
            // folder or multiple, now check for file-only props
            if (nameUTF8.compare(SVN_PROP_EOL_STYLE)==0)
                m_bRecursive = TRUE;
            if (nameUTF8.compare(SVN_PROP_EXECUTABLE)==0)
                m_bRecursive = TRUE;
            if (nameUTF8.compare(SVN_PROP_KEYWORDS)==0)
                m_bRecursive = TRUE;
            if (nameUTF8.compare(SVN_PROP_NEEDS_LOCK)==0)
                m_bRecursive = TRUE;
            if (nameUTF8.compare(SVN_PROP_MIME_TYPE)==0)
                m_bRecursive = TRUE;
        }
        UINT nText = 0;
        if (nameUTF8.compare(SVN_PROP_EXTERNALS)==0)
            nText = IDS_PROP_TT_EXTERNALS;
        if (nameUTF8.compare(SVN_PROP_EXECUTABLE)==0)
            nText = IDS_PROP_TT_EXECUTABLE;
        if (nameUTF8.compare(SVN_PROP_NEEDS_LOCK)==0)
            nText = IDS_PROP_TT_NEEDSLOCK;
        if (nameUTF8.compare(SVN_PROP_MIME_TYPE)==0)
            nText = IDS_PROP_TT_MIMETYPE;
        if (nameUTF8.compare(SVN_PROP_IGNORE)==0)
            nText = IDS_PROP_TT_IGNORE;
        if (nameUTF8.compare(SVN_PROP_KEYWORDS)==0)
            nText = IDS_PROP_TT_KEYWORDS;
        if (nameUTF8.compare(SVN_PROP_EOL_STYLE)==0)
            nText = IDS_PROP_TT_EOLSTYLE;
        if (nameUTF8.compare(SVN_PROP_MERGEINFO)==0)
            nText = IDS_PROP_TT_MERGEINFO;

        if (nameUTF8.compare(BUGTRAQPROPNAME_LABEL)==0)
            nText = IDS_PROP_TT_BQLABEL;
        if (nameUTF8.compare(BUGTRAQPROPNAME_LOGREGEX)==0)
            nText = IDS_PROP_TT_BQLOGREGEX;
        if (nameUTF8.compare(BUGTRAQPROPNAME_MESSAGE)==0)
            nText = IDS_PROP_TT_BQMESSAGE;
        if (nameUTF8.compare(BUGTRAQPROPNAME_NUMBER)==0)
            nText = IDS_PROP_TT_BQNUMBER;
        if (nameUTF8.compare(BUGTRAQPROPNAME_URL)==0)
            nText = IDS_PROP_TT_BQURL;
        if (nameUTF8.compare(BUGTRAQPROPNAME_WARNIFNOISSUE)==0)
            nText = IDS_PROP_TT_BQWARNNOISSUE;
        if (nameUTF8.compare(BUGTRAQPROPNAME_APPEND)==0)
            nText = IDS_PROP_TT_BQAPPEND;
        if (nameUTF8.compare(BUGTRAQPROPNAME_PROVIDERUUID)==0)
            nText = IDS_PROP_TT_BQPROVIDERUUID;
        if (nameUTF8.compare(BUGTRAQPROPNAME_PROVIDERPARAMS)==0)
            nText = IDS_PROP_TT_BQPROVIDERPARAMS;

        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATE)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATE;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATECOMMIT)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATECOMMIT;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEBRANCH)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEBRANCH;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEIMPORT)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEIMPORT;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEDEL)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEDEL;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEMOVE)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEMOVE;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEMKDIR)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEMKDIR;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATEPROPSET)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATEPROPSET;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGTEMPLATELOCK)==0)
            nText = IDS_PROP_TT_TSVNLOGTEMPLATELOCK;

        if (nameUTF8.compare(PROJECTPROPNAME_LOGWIDTHLINE)==0)
            nText = IDS_PROP_TT_TSVNLOGWIDTHMARKER;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGMINSIZE)==0)
            nText = IDS_PROP_TT_TSVNLOGMINSIZE;
        if (nameUTF8.compare(PROJECTPROPNAME_LOCKMSGMINSIZE)==0)
            nText = IDS_PROP_TT_TSVNLOCKMSGMINSIZE;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGFILELISTLANG)==0)
            nText = IDS_PROP_TT_TSVNLOGFILELISTENGLISH;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGSUMMARY)==0)
            nText = IDS_PROP_TT_TSVNLOGSUMMARY;
        if (nameUTF8.compare(PROJECTPROPNAME_PROJECTLANGUAGE)==0)
            nText = IDS_PROP_TT_TSVNPROJECTLANGUAGE;
        if (nameUTF8.compare(PROJECTPROPNAME_USERFILEPROPERTY)==0)
            nText = IDS_PROP_TT_TSVNUSERFILEPROPERTIES;
        if (nameUTF8.compare(PROJECTPROPNAME_USERDIRPROPERTY)==0)
            nText = IDS_PROP_TT_TSVNUSERFOLDERPROPERTIES;
        if (nameUTF8.compare(PROJECTPROPNAME_AUTOPROPS)==0)
            nText = IDS_PROP_TT_TSVNAUTOPROPS;
        if (nameUTF8.compare(PROJECTPROPNAME_LOGREVREGEX)==0)
            nText = IDS_PROP_TT_TSVNLOGREVREGEX;

        if (nameUTF8.compare(PROJECTPROPNAME_WEBVIEWER_REV)==0)
            nText = IDS_PROP_TT_WEBVIEWERREVISION;
        if (nameUTF8.compare(PROJECTPROPNAME_WEBVIEWER_PATHREV)==0)
            nText = IDS_PROP_TT_WEBVIEWERPATHREVISION;

        if (nText)
        {
            m_tooltips.AddTool(GetDlgItem(IDC_PROPNAMECOMBO), nText);
            m_tooltips.AddTool(GetDlgItem(IDC_PROPNAMECOMBO)->GetWindow(GW_CHILD), nText);
            m_tooltips.AddTool(GetDlgItem(IDC_PROPVALUE), nText);
        }
        else
        {
            // if no match is found then remove the tool tip so that the last tooltip is not wrongly shown
            m_tooltips.DelTool(GetDlgItem(IDC_PROPNAMECOMBO)->GetWindow(GW_CHILD));
        }
    }
    m_bChanged = true;
}

BOOL CEditPropertyValueDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_RETURN:
            if (OnEnterPressed())
                return TRUE;
            break;
        default:
            break;
        }
    }

    m_tooltips.RelayEvent(pMsg);
    return __super::PreTranslateMessage(pMsg);
}

void CEditPropertyValueDlg::OnBnClickedLoadprop()
{
    CString openPath;
    if (!CAppUtils::FileOpenSave(openPath, NULL, IDS_REPOBROWSE_OPEN, IDS_COMMONFILEFILTER, true, m_hWnd))
    {
        return;
    }
    // first check the size of the file
    CAutoFile hFile = CreateFile(openPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile)
    {
        DWORD size = GetFileSize(hFile, NULL);
        FILE * stream;
        _tfopen_s(&stream, openPath, _T("rbS"));
        auto_buffer<char> buf(size);
        if (fread(buf, sizeof(char), size, stream)==size)
        {
            m_PropValue.assign(buf, size);
        }
        fclose(stream);
        // see if the loaded file contents are binary
        SetPropertyValue(m_PropValue);
        UpdateData(FALSE);
        m_bChanged = true;
    }

}

void CEditPropertyValueDlg::OnEnChangePropvalue()
{
    UpdateData();
    CString sTemp;
    sTemp.LoadString(IDS_EDITPROPS_BINVALUE);
    m_bChanged=true;
    if ((m_bIsBinary)&&(m_sPropValue.CompareNoCase(sTemp)!=0))
    {
        m_sPropValue.Empty();
        m_PropValue.clear();
        UpdateData(FALSE);
        m_bIsBinary = false;
    }
}

