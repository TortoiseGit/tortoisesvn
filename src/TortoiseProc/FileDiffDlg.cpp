// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include "SysImageList.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "RevisionDlg.h"
#include "IconMenu.h"
#include "FileDiffDlg.h"
#include "DiffOptionsDlg.h"
#include "AsyncCall.h"

enum FileDiffDlgMenuIDs
{
    ID_COMPARE = 1,
    ID_COMPARETEXT,
    ID_COMPAREPROP,
    ID_BLAME,
    ID_SAVEAS,
    ID_EXPORT,
    ID_CLIPBOARD,
    ID_UNIFIEDDIFF,
    ID_LOG,
};

#define COL1_WIDTH 200

BOOL CFileDiffDlg::m_bAscending    = FALSE;
int  CFileDiffDlg::m_nSortedColumn = -1;

CString sContentOnly;
CString sPropertiesOnly;
CString sContentAndProps;

bool s_bSortLogical = true;

IMPLEMENT_DYNAMIC(CFileDiffDlg, CResizableStandAloneDialog)
CFileDiffDlg::CFileDiffDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CFileDiffDlg::IDD, pParent)
    , m_bBlame(false)
    , m_pProgDlg(nullptr)
    , m_nIconFolder(0)
    , m_depth(svn_depth_unknown)
    , m_bIgnoreancestry(false)
    , m_bDoPegDiff(false)
    , m_bThreadRunning(false)
    , netScheduler(1, 0, true)
    , m_bCancelled(false)
{
    m_columnBuf[0] = 0;
    s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
    if (s_bSortLogical)
        s_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
}

CFileDiffDlg::~CFileDiffDlg()
{
}

void CFileDiffDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FILELIST, m_cFileList);
    DDX_Control(pDX, IDC_SWITCHLEFTRIGHT, m_switchButton);
    DDX_Control(pDX, IDC_REV1BTN, m_cRev1Btn);
    DDX_Control(pDX, IDC_REV2BTN, m_cRev2Btn);
    DDX_Control(pDX, IDC_FILTER, m_cFilter);
}

BEGIN_MESSAGE_MAP(CFileDiffDlg, CResizableStandAloneDialog)
    ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
    ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
    ON_WM_CONTEXTMENU()
    ON_WM_SETCURSOR()
    ON_EN_SETFOCUS(IDC_SECONDURL, &CFileDiffDlg::OnEnSetfocusSecondurl)
    ON_EN_SETFOCUS(IDC_FIRSTURL, &CFileDiffDlg::OnEnSetfocusFirsturl)
    ON_BN_CLICKED(IDC_SWITCHLEFTRIGHT, &CFileDiffDlg::OnBnClickedSwitchleftright)
    ON_NOTIFY(HDN_ITEMCLICK, 0, &CFileDiffDlg::OnHdnItemclickFilelist)
    ON_BN_CLICKED(IDC_REV1BTN, &CFileDiffDlg::OnBnClickedRev1btn)
    ON_BN_CLICKED(IDC_REV2BTN, &CFileDiffDlg::OnBnClickedRev2btn)
    ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
    ON_EN_CHANGE(IDC_FILTER, &CFileDiffDlg::OnEnChangeFilter)
    ON_WM_TIMER()
    ON_NOTIFY(LVN_GETDISPINFO, IDC_FILELIST, &CFileDiffDlg::OnLvnGetdispinfoFilelist)
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

void CFileDiffDlg::SetDiff(const CTSVNPath& path, const SVNRev& peg, const SVNRev& rev1, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry)
{
    m_bDoPegDiff      = true;
    m_path1           = path;
    m_path2           = path;
    m_peg             = peg;
    m_rev1            = rev1;
    m_rev2            = rev2;
    m_depth           = depth;
    m_bIgnoreancestry = ignoreancestry;
}

void CFileDiffDlg::SetDiff(const CTSVNPath& path1, const SVNRev& rev1, const CTSVNPath& path2, const SVNRev& rev2, svn_depth_t depth, bool ignoreancestry)
{
    m_bDoPegDiff      = false;
    m_path1           = path1;
    m_path2           = path2;
    m_rev1            = rev1;
    m_rev2            = rev2;
    m_depth           = depth;
    m_bIgnoreancestry = ignoreancestry;
}

BOOL CFileDiffDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);
    CString temp;

    m_tooltips.AddTool(IDC_SWITCHLEFTRIGHT, IDS_FILEDIFF_SWITCHLEFTRIGHT_TT);

    m_cFileList.SetRedraw(false);
    m_cFileList.DeleteAllItems();
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
    m_cFileList.SetExtendedStyle(exStyle);

    m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
    m_cFileList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);

    int iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_switchButton.SetImage(CCommonAppUtils::LoadIconEx(IDI_SWITCHLEFTRIGHT, iconWidth, iconHeight));
    m_switchButton.Invalidate();

    m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED, 14, 14);
    m_cFilter.SetInfoIcon(IDI_FILTEREDIT, 19, 19);
    temp.LoadString(IDS_FILEDIFF_FILTERCUE);
    temp = L"   " + temp;
    m_cFilter.SetCueBanner(temp);

    int c = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_cFileList.DeleteColumn(c--);
    temp.LoadString(IDS_FILEDIFF_FILE);
    m_cFileList.InsertColumn(0, temp);
    temp.LoadString(IDS_FILEDIFF_ACTION);
    m_cFileList.InsertColumn(1, temp);

    CRect rect;
    m_cFileList.GetClientRect(&rect);
    m_cFileList.SetColumnWidth(0, rect.Width() - COL1_WIDTH - 20);
    m_cFileList.SetColumnWidth(1, COL1_WIDTH);

    m_cFileList.SetRedraw(true);

    AddAnchor(IDC_DIFFSTATIC1, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SWITCHLEFTRIGHT, TOP_RIGHT);
    AddAnchor(IDC_FIRSTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REV1BTN, TOP_RIGHT);
    AddAnchor(IDC_DIFFSTATIC2, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SECONDURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_REV2BTN, TOP_RIGHT);
    AddAnchor(IDC_FILTER, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);

    SetURLLabels();

    sContentOnly     = CString(MAKEINTRESOURCE(IDS_CONTENTONLY));
    sPropertiesOnly  = CString(MAKEINTRESOURCE(IDS_PROPONLY));
    sContentAndProps = CString(MAKEINTRESOURCE(IDS_CONTENTANDPROP));

    EnableSaveRestore(L"FileDiffDlg");

    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(DiffThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
    }

    // Start with focus on file list
    GetDlgItem(IDC_FILELIST)->SetFocus();
    return FALSE;
}

svn_error_t* CFileDiffDlg::DiffSummarizeCallback(const CTSVNPath&                 path,
                                                 svn_client_diff_summarize_kind_t kind,
                                                 bool propchanged, svn_node_kind_t node)
{
    FileDiff fd;
    fd.path        = path;
    fd.kind        = kind;
    fd.node        = node;
    fd.propchanged = propchanged;
    m_arFileList.push_back(fd);
    return nullptr;
}

UINT CFileDiffDlg::DiffThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CFileDiffDlg*>(pVoid)->DiffThread();
}

UINT CFileDiffDlg::DiffThread()
{
    bool bSuccess = true;
    RefreshCursor();
    m_cFileList.ShowText(CString(MAKEINTRESOURCE(IDS_FILEDIFF_WAIT)));
    m_arFileList.clear();
    if (m_bDoPegDiff)
    {
        bSuccess = DiffSummarizePeg(m_path1, m_peg, m_rev1, m_rev2, m_depth, m_bIgnoreancestry);
    }
    else
    {
        bSuccess = DiffSummarize(m_path1, m_rev1, m_path2, m_rev2, m_depth, m_bIgnoreancestry);
    }
    if (!bSuccess)
    {
        m_cFileList.ShowText(GetLastErrorMessage());
        InterlockedExchange(&m_bThreadRunning, FALSE);
        return 0;
    }

    CString sFilterText;
    m_cFilter.GetWindowText(sFilterText);
    m_cFileList.SetRedraw(false);
    Filter(sFilterText);
    if (!m_arFileList.empty())
    {
        // Highlight first entry in file list
        m_cFileList.SetSelectionMark(0);
        m_cFileList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
    }

    CRect rect;
    m_cFileList.GetClientRect(&rect);
    m_cFileList.SetColumnWidth(0, rect.Width() - COL1_WIDTH - 20);
    m_cFileList.SetColumnWidth(1, COL1_WIDTH);

    m_cFileList.ClearText();
    m_cFileList.SetRedraw(true);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    InvalidateRect(nullptr);
    RefreshCursor();
    return 0;
}

void CFileDiffDlg::DoDiff(int selIndex, bool bText, bool bProps, bool blame, bool bDefault)
{
    CFileDiffDlg::FileDiff fd = m_arFilteredList[selIndex];

    CTSVNPath url1 = CTSVNPath(m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
    CTSVNPath url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());

    if (fd.kind == svn_client_diff_summarize_kind_deleted)
    {
        if (!PathIsURL(url1))
            url1 = CTSVNPath(GetURLFromPath(m_path1) + L"/" + fd.path.GetSVNPathString());
        if (!PathIsURL(url2))
            url2 = m_bDoPegDiff ? url1 : CTSVNPath(GetURLFromPath(m_path2) + L"/" + fd.path.GetSVNPathString());
    }

    if ((fd.propchanged && bProps && (!blame || bProps)) ||
        (bDefault && (fd.node == svn_node_dir)))
    {
        DiffProps(selIndex);
    }
    if (fd.node == svn_node_dir)
        return;
    if (!bText)
        return;

    CTSVNPath    tempFile = CTempFiles::Instance().GetTempFilePath(false, m_path1, m_rev1);
    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_PROGRESSWAIT);
    progDlg.ShowModeless(this);
    progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()));
    progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISIONTEXT, static_cast<LPCWSTR>(m_rev1.ToString()));
    SetAndClearProgressInfo(&progDlg);
    m_blamer.SetAndClearProgressInfo(&progDlg, 3, false);
    if ((fd.kind != svn_client_diff_summarize_kind_added) && (!blame) && (!Export(url1, tempFile, m_bDoPegDiff ? m_peg : m_rev1, m_rev1)))
    {
        if ((!m_bDoPegDiff) || (!Export(url1, tempFile, m_rev1, m_rev1)))
        {
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_blamer.SetAndClearProgressInfo(nullptr, 3);
            ShowErrorDialog(m_hWnd);
            return;
        }
    }
    else if ((fd.kind != svn_client_diff_summarize_kind_added) && (blame) && (!m_blamer.BlameToFile(url1, 1, m_rev1, m_bDoPegDiff ? m_peg : m_rev1, tempFile, L"", TRUE, TRUE)))
    {
        if ((!m_bDoPegDiff) || (!m_blamer.BlameToFile(url1, 1, m_rev1, m_rev1, tempFile, L"", TRUE, TRUE)))
        {
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_blamer.SetAndClearProgressInfo(nullptr, 3);
            m_blamer.ShowErrorDialog(m_hWnd);
            return;
        }
    }
    SetFileAttributes(tempFile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    progDlg.SetProgress(1, 2);
    progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, static_cast<LPCWSTR>(url2.GetUIFileOrDirectoryName()));
    progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISIONTEXT, static_cast<LPCWSTR>(m_rev2.ToString()));
    CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(false, url2, m_rev2);
    if ((fd.kind != svn_client_diff_summarize_kind_deleted) && (!blame) && (!Export(url2, tempfile2, m_bDoPegDiff ? m_peg : m_rev2, m_rev2)))
    {
        if ((!m_bDoPegDiff) || (!Export(url2, tempfile2, m_rev2, m_rev2)))
        {
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_blamer.SetAndClearProgressInfo(nullptr, 3);
            ShowErrorDialog(m_hWnd);
            return;
        }
    }
    else if ((fd.kind != svn_client_diff_summarize_kind_deleted) && (blame) && (!m_blamer.BlameToFile(url2, 1, m_bDoPegDiff ? m_peg : m_rev2, m_rev2, tempfile2, L"", TRUE, TRUE)))
    {
        if ((!m_bDoPegDiff) || (!m_blamer.BlameToFile(url2, 1, m_rev2, m_rev2, tempfile2, L"", TRUE, TRUE)))
        {
            SetAndClearProgressInfo(static_cast<HWND>(nullptr));
            m_blamer.SetAndClearProgressInfo(nullptr, 3);
            m_blamer.ShowErrorDialog(m_hWnd);
            return;
        }
    }
    SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);
    progDlg.SetProgress(2, 2);
    progDlg.Stop();
    SetAndClearProgressInfo(static_cast<HWND>(nullptr));
    m_blamer.SetAndClearProgressInfo(nullptr, 3);

    CString rev1Name, rev2Name;
    if (m_bDoPegDiff)
    {
        rev1Name.Format(L"%s Revision %ld", static_cast<LPCWSTR>(fd.path.GetSVNPathString()), static_cast<LONG>(m_rev1));
        rev2Name.Format(L"%s Revision %ld", static_cast<LPCWSTR>(fd.path.GetSVNPathString()), static_cast<LONG>(m_rev2));
    }
    else
    {
        rev1Name = m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString();
        rev2Name = m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString();
    }
    CAppUtils::DiffFlags flags;
    flags.AlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
    flags.Blame(blame);
    CString mimetype;
    if (!blame)
        CAppUtils::GetMimeType(url1, mimetype, m_rev1);
    CAppUtils::StartExtDiff(
        tempFile, tempfile2, rev1Name, rev2Name, url1, url2, m_rev1, m_rev2, m_bDoPegDiff ? m_peg : SVNRev(), flags, 0, L"", mimetype);
}

void CFileDiffDlg::DiffProps(int selIndex)
{
    CFileDiffDlg::FileDiff fd = m_arFilteredList[selIndex];

    CTSVNPath url1 = CTSVNPath(m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
    CTSVNPath url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());

    SVNProperties propsUrl1(url1, m_rev1, false, false);
    SVNProperties propsUrl2(url2, m_rev2, false, false);

    // collect the properties of both revisions in a set
    std::set<std::string> properties;
    for (int wcIndex = 0; wcIndex < propsUrl1.GetCount(); ++wcIndex)
    {
        std::string urlName = propsUrl1.GetItemName(wcIndex);
        if (properties.find(urlName) == properties.end())
        {
            properties.insert(urlName);
        }
    }
    for (int wcIndex = 0; wcIndex < propsUrl2.GetCount(); ++wcIndex)
    {
        std::string urlName = propsUrl2.GetItemName(wcIndex);
        if (properties.find(urlName) == properties.end())
        {
            properties.insert(urlName);
        }
    }

    // iterate over all properties and diff the properties
    for (std::set<std::string>::iterator iter = properties.begin(), end = properties.end(); iter != end; ++iter)
    {
        const std::string& url1Name = *iter;

        std::wstring url1Value = L""; // CUnicodeUtils::StdGetUnicode((char *)propsurl1.GetItemValue(wcindex).c_str());
        for (int url1Index = 0; url1Index < propsUrl1.GetCount(); ++url1Index)
        {
            if (propsUrl1.GetItemName(url1Index).compare(url1Name) == 0)
            {
                url1Value = CUnicodeUtils::GetUnicode(propsUrl1.GetItemValue(url1Index).c_str());
            }
        }

        std::wstring url2Value = L"";
        for (int url2Index = 0; url2Index < propsUrl2.GetCount(); ++url2Index)
        {
            if (propsUrl2.GetItemName(url2Index).compare(url1Name) == 0)
            {
                url2Value = CUnicodeUtils::GetUnicode(propsUrl2.GetItemValue(url2Index).c_str());
            }
        }

        if (url2Value.compare(url1Value) != 0)
        {
            // write both property values to temporary files
            CTSVNPath url1Propfile = CTempFiles::Instance().GetTempFilePath(false);
            CTSVNPath url2Propfile = CTempFiles::Instance().GetTempFilePath(false);
            FILE*     pFile;
            _tfopen_s(&pFile, url1Propfile.GetWinPath(), L"wb");
            if (pFile)
            {
                fputs(CUnicodeUtils::StdGetUTF8(url1Value).c_str(), pFile);
                fclose(pFile);
                FILE* pFile2;
                _tfopen_s(&pFile2, url2Propfile.GetWinPath(), L"wb");
                if (pFile2)
                {
                    fputs(CUnicodeUtils::StdGetUTF8(url2Value).c_str(), pFile2);
                    fclose(pFile2);
                }
                else
                    return;
            }
            else
                return;
            SetFileAttributes(url1Propfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            SetFileAttributes(url2Propfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
            CString n1, n2;
            if (m_rev1.IsWorking())
                n1.Format(IDS_DIFF_WCNAME, CUnicodeUtils::StdGetUnicode(url1Name).c_str());
            if (m_rev1.IsBase())
                n1.Format(IDS_DIFF_BASENAME, CUnicodeUtils::StdGetUnicode(url1Name).c_str());
            if (m_rev1.IsHead() || m_rev1.IsNumber())
            {
                if (m_bDoPegDiff)
                {
                    n1.Format(L"%s : %s Revision %ld", CUnicodeUtils::StdGetUnicode(url1Name).c_str(), static_cast<LPCWSTR>(fd.path.GetSVNPathString()), static_cast<LONG>(m_rev1));
                }
                else
                {
                    CString sTemp(CUnicodeUtils::StdGetUnicode(url1Name).c_str());
                    sTemp += L" : ";
                    n1 = sTemp + m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString();
                }
            }
            if (m_rev2.IsWorking())
                n2.Format(IDS_DIFF_WCNAME, CUnicodeUtils::StdGetUnicode(url1Name).c_str());
            if (m_rev2.IsBase())
                n2.Format(IDS_DIFF_BASENAME, CUnicodeUtils::StdGetUnicode(url1Name).c_str());
            if (m_rev2.IsHead() || m_rev2.IsNumber())
            {
                if (m_bDoPegDiff)
                {
                    n2.Format(L"%s : %s Revision %ld", CUnicodeUtils::StdGetUnicode(url1Name).c_str(), static_cast<LPCWSTR>(fd.path.GetSVNPathString()), static_cast<LONG>(m_rev2));
                }
                else
                {
                    CString sTemp(CUnicodeUtils::StdGetUnicode(url1Name).c_str());
                    sTemp += L" : ";
                    n2 = sTemp + m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString();
                }
            }
            CAppUtils::StartExtDiffProps(url1Propfile, url2Propfile, n1, n2, TRUE);
        }
    }
}

void CFileDiffDlg::OnNMDblclkFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult              = 0;
    LPNMLISTVIEW pNMLV    = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    int          selIndex = pNMLV->iItem;
    if (selIndex < 0)
        return;
    if (selIndex >= static_cast<int>(m_arFilteredList.size()))
        return;

    CFileDiffDlg::FileDiff fd = m_arFilteredList[selIndex];
    if (fd.propchanged && (fd.kind == svn_client_diff_summarize_kind_normal))
    {
        DoDiff(selIndex, false, true, m_bBlame, true);
    }
    else
        DoDiff(selIndex, true, false, m_bBlame, true);
}

void CFileDiffDlg::OnLvnGetInfoTipFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult                     = 0;
    LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
    if (pGetInfoTip->dwFlags & LVGIT_UNFOLDED)
        return; // only show infotip if the item isn't fully visible
    if (pGetInfoTip->iItem >= static_cast<int>(m_arFilteredList.size()))
        return;

    CString path = m_path1.GetSVNPathString() + L"/" + m_arFilteredList[pGetInfoTip->iItem].path.GetSVNPathString();
    if (pGetInfoTip->cchTextMax > path.GetLength())
        wcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, path, pGetInfoTip->cchTextMax - 1LL);
}

void CFileDiffDlg::OnNMCustomdrawFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        // This is the prepaint stage for an item. Here's where we set the
        // item's text color. Our return value will tell Windows to draw the
        // item itself, but it will use the new color we set here.

        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;

        COLORREF crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);

        if (m_arFilteredList.size() > pLVCD->nmcd.dwItemSpec)
        {
            FileDiff fd = m_arFilteredList[pLVCD->nmcd.dwItemSpec];
            switch (fd.kind)
            {
                case svn_client_diff_summarize_kind_added:
                    crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Added));
                    break;
                case svn_client_diff_summarize_kind_deleted:
                    crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Deleted));
                    break;
                case svn_client_diff_summarize_kind_modified:
                    crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::Modified));
                    break;
                case svn_client_diff_summarize_kind_normal:
                default:
                    if (fd.propchanged)
                        crText = CTheme::Instance().GetThemeColor(m_colors.GetColor(CColors::PropertyChanged));
                    break;
            }
        }
        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
}

void CFileDiffDlg::OnLvnGetdispinfoFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

    if (pDispInfo)
    {
        if (pDispInfo->item.iItem < static_cast<int>(m_arFilteredList.size()))
        {
            const FileDiff* data = &m_arFilteredList[pDispInfo->item.iItem];
            if (pDispInfo->item.mask & LVIF_TEXT)
            {
                switch (pDispInfo->item.iSubItem)
                {
                    case 0: // path
                    {
                        lstrcpyn(m_columnBuf, data->path.GetSVNPathString(), pDispInfo->item.cchTextMax - 1LL);
                        int cWidth = m_cFileList.GetColumnWidth(0);
                        cWidth     = max(28, cWidth - 28);
                        CDC* pDC   = m_cFileList.GetDC();
                        if (pDC != nullptr)
                        {
                            CFont* pFont = pDC->SelectObject(m_cFileList.GetFont());
                            PathCompactPath(pDC->GetSafeHdc(), m_columnBuf, cWidth);
                            pDC->SelectObject(pFont);
                            ReleaseDC(pDC);
                        }
                    }
                    break;
                    case 1: // action
                        lstrcpyn(m_columnBuf, GetSummarizeActionText(data->kind), pDispInfo->item.cchTextMax - 1LL);
                        wcscat_s(m_columnBuf, L" ");
                        if ((data->kind != svn_client_diff_summarize_kind_normal) && (!data->propchanged))
                            wcscat_s(m_columnBuf, sContentOnly);
                        else if (data->kind == svn_client_diff_summarize_kind_normal)
                            wcscat_s(m_columnBuf, sPropertiesOnly);
                        else
                            wcscat_s(m_columnBuf, sContentAndProps);
                        break;
                    default:
                        m_columnBuf[0] = 0;
                }
                pDispInfo->item.pszText = m_columnBuf;
            }
            if (pDispInfo->item.mask & LVIF_IMAGE)
            {
                int iconIdx = 0;
                if (data->node == svn_node_dir)
                    iconIdx = m_nIconFolder;
                else
                {
                    iconIdx = SYS_IMAGE_LIST().GetPathIconIndex(data->path);
                }
                pDispInfo->item.iImage = iconIdx;
            }
        }
    }
    *pResult = 0;
}

void CFileDiffDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if ((pWnd == nullptr) || (pWnd != &m_cFileList))
        return;
    if (m_cFileList.GetSelectedCount() == 0)
        return;
    // if the context menu is invoked through the keyboard, we have to use
    // a calculated position on where to anchor the menu on
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        m_cFileList.GetItemRect(m_cFileList.GetSelectionMark(), &rect, LVIR_LABEL);
        m_cFileList.ClientToScreen(&rect);
        point = rect.CenterPoint();
    }
    CIconMenu popup;
    if (!popup.CreatePopupMenu())
        return;

    bool     bHasPropChanges = false;
    bool     bHasTextChanges = false;
    POSITION spos            = m_cFileList.GetFirstSelectedItemPosition();
    while (spos)
    {
        int      sIndex = m_cFileList.GetNextSelectedItem(spos);
        FileDiff sfd    = m_arFilteredList[sIndex];
        bHasTextChanges = bHasTextChanges || (sfd.kind != svn_client_diff_summarize_kind_normal);
        bHasPropChanges = bHasPropChanges || sfd.propchanged;
    }

    if (bHasTextChanges)
    {
        if (bHasPropChanges)
            popup.AppendMenuIcon(ID_COMPARETEXT, IDS_LOG_POPUP_COMPARETWOTEXT, IDI_DIFF);
        else
            popup.AppendMenuIcon(ID_COMPARETEXT, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
    }
    if (bHasPropChanges)
    {
        if (bHasTextChanges)
            popup.AppendMenuIcon(ID_COMPAREPROP, IDS_LOG_POPUP_COMPARETWOPROP, IDI_DIFF);
        else
            popup.AppendMenuIcon(ID_COMPAREPROP, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
    }
    if (bHasTextChanges && bHasPropChanges)
        popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);

    popup.AppendMenuIcon(ID_UNIFIEDDIFF, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
    popup.AppendMenuIcon(ID_BLAME, IDS_FILEDIFF_POPBLAME, IDI_BLAME);
    popup.AppendMenuIcon(ID_LOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG);
    popup.AppendMenu(MF_SEPARATOR, NULL);
    popup.AppendMenuIcon(ID_SAVEAS, IDS_FILEDIFF_POPSAVELIST, IDI_SAVEAS);
    popup.AppendMenuIcon(ID_CLIPBOARD, IDS_FILEDIFF_POPCLIPBOARD, IDI_COPYCLIP);
    popup.AppendMenuIcon(ID_EXPORT, IDS_FILEDIFF_POPEXPORT, IDI_EXPORT);
    int cmd      = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, nullptr);
    m_bCancelled = false;
    switch (cmd)
    {
        case ID_COMPARE:
        {
            POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
            while (pos)
            {
                int index = m_cFileList.GetNextSelectedItem(pos);

                auto f = [=]() {
                    CoInitialize(nullptr);
                    this->EnableWindow(FALSE);

                    DoDiff(index, true, true, false, false);

                    this->EnableWindow(TRUE);
                    this->SetFocus();
                };
                new async::CAsyncCall(f, &netScheduler);
            }
        }
        break;
        case ID_COMPARETEXT:
        {
            POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
            while (pos)
            {
                int      index = m_cFileList.GetNextSelectedItem(pos);
                FileDiff sfd   = m_arFilteredList[index];
                if (sfd.kind == svn_client_diff_summarize_kind_normal)
                    continue;

                auto f = [=]() {
                    CoInitialize(nullptr);
                    this->EnableWindow(FALSE);

                    DoDiff(index, true, false, false, false);

                    this->EnableWindow(TRUE);
                    this->SetFocus();
                };
                new async::CAsyncCall(f, &netScheduler);
            }
        }
        break;
        case ID_COMPAREPROP:
        {
            POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
            while (pos)
            {
                int      index = m_cFileList.GetNextSelectedItem(pos);
                FileDiff sfd   = m_arFilteredList[index];
                if (!sfd.propchanged)
                    continue;

                auto f = [=]() {
                    CoInitialize(nullptr);
                    this->EnableWindow(FALSE);

                    DoDiff(index, false, true, false, false);

                    this->EnableWindow(TRUE);
                    this->SetFocus();
                };
                new async::CAsyncCall(f, &netScheduler);
            }
        }
        break;
        case ID_UNIFIEDDIFF:
        {
            bool    prettyprint = true;
            CString options;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
                CDiffOptionsDlg dlg(this);
                if (dlg.DoModal() == IDOK)
                {
                    options     = dlg.GetDiffOptionsString();
                    prettyprint = dlg.GetPrettyPrint();
                }
                else
                    break;
            }
            CTSVNPathList urls1, urls2;
            GetSelectedPaths(urls1, urls2);
            CTSVNPath diffFile   = CTempFiles::Instance().GetTempFilePath(false);
            bool      bDoPegDiff = m_bDoPegDiff;
            auto      f          = [=]() {
                CoInitialize(nullptr);
                this->EnableWindow(FALSE);

                CProgressDlg progDlg;
                progDlg.SetTitle(IDS_APPNAME);
                progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
                progDlg.SetTime(false);
                SetAndClearProgressInfo(&progDlg);
                progDlg.SetProgress(0, urls1.GetCount());
                progDlg.ShowModeless(m_hWnd);
                for (int i = 0; i < urls1.GetCount(); ++i)
                {
                    CTSVNPath url1 = urls1[i];
                    CTSVNPath url2 = urls2[i];
                    progDlg.SetLine(3, static_cast<LPCWSTR>(url1.GetUIFileOrDirectoryName()), true);
                    progDlg.SetProgress(i + 1, urls1.GetCount());
                    if (bDoPegDiff)
                    {
                        PegDiff(url1, m_peg, m_rev1, m_rev2, CTSVNPath(), m_depth, m_bIgnoreancestry, false, false, true, true, false, true, false, prettyprint, options, true, diffFile);
                    }
                    else
                    {
                        Diff(url1, m_rev1, url2, m_rev2, CTSVNPath(), m_depth, m_bIgnoreancestry, false, false, true, true, false, true, false, prettyprint, options, true, diffFile);
                    }
                    if (progDlg.HasUserCancelled() || m_bCancelled)
                        break;
                }
                progDlg.Stop();
                SetAndClearProgressInfo(static_cast<HWND>(nullptr));
                CAppUtils::StartUnifiedDiffViewer(diffFile.GetWinPathString(), CString(), false);

                this->EnableWindow(TRUE);
                this->SetFocus();
            };
            new async::CAsyncCall(f, &netScheduler);
        }
        break;
        case ID_BLAME:
        {
            POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
            while (pos)
            {
                int  index = m_cFileList.GetNextSelectedItem(pos);
                auto f     = [=]() {
                    CoInitialize(nullptr);
                    this->EnableWindow(FALSE);

                    DoDiff(index, true, false, true, false);

                    this->EnableWindow(TRUE);
                    this->SetFocus();
                };
                new async::CAsyncCall(f, &netScheduler);
            }
        }
        break;
        case ID_LOG:
        {
            POSITION               pos   = m_cFileList.GetFirstSelectedItemPosition();
            int                    index = m_cFileList.GetNextSelectedItem(pos);
            CFileDiffDlg::FileDiff fd    = m_arFilteredList[index];
            CTSVNPath              url1  = CTSVNPath(m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
            CString                sCmd;
            SVNRev                 rmax = max(static_cast<LONG>(m_rev1), static_cast<LONG>(m_rev2));
            sCmd.Format(L"/command:log /path:\"%s\" /startrev:%s /pegrev:%s",
                        static_cast<LPCWSTR>(url1.GetSVNPathString()), static_cast<LPCWSTR>(rmax.ToString()), static_cast<LPCWSTR>(m_peg.ToString()));
            CAppUtils::RunTortoiseProc(sCmd);
        }
        break;
        case ID_SAVEAS:
            if (m_cFileList.GetSelectedCount() > 0)
            {
                CTSVNPath savePath;
                CString   pathSave;
                if (!CAppUtils::FileOpenSave(pathSave, nullptr, IDS_REPOBROWSE_SAVEAS, IDS_COMMONFILEFILTER, false, m_path1.IsUrl() ? CString() : m_path1.GetDirectory().GetWinPathString(), m_hWnd))
                {
                    break;
                }
                savePath = CTSVNPath(pathSave);

                // now open the selected file for writing
                try
                {
                    CStdioFile file(savePath.GetWinPathString(), CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
                    CString    temp;
                    temp.FormatMessage(IDS_FILEDIFF_CHANGEDLISTINTRO, static_cast<LPCWSTR>(m_path1.GetSVNPathString()), static_cast<LPCWSTR>(m_rev1.ToString()), static_cast<LPCWSTR>(m_path2.GetSVNPathString()), static_cast<LPCWSTR>(m_rev2.ToString()));
                    file.WriteString(temp + L"\n");
                    POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
                    while (pos)
                    {
                        int      index = m_cFileList.GetNextSelectedItem(pos);
                        FileDiff fd    = m_arFilteredList[index];
                        file.WriteString(fd.path.GetSVNPathString());
                        file.WriteString(L"\n");
                    }
                    file.Close();
                }
                catch (CFileException* pE)
                {
                    pE->ReportError();
                    pE->Delete();
                }
            }
            break;
        case ID_CLIPBOARD:
            CopySelectionToClipboard();
            break;
        case ID_EXPORT:
        {
            // export all changed files to a folder
            CBrowseFolder browseFolder;
            browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            if (browseFolder.Show(GetSafeHwnd(), m_strExportDir) == CBrowseFolder::OK)
            {
                m_arSelectedFileList.RemoveAll();
                POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
                while (pos)
                {
                    int                    index = m_cFileList.GetNextSelectedItem(pos);
                    CFileDiffDlg::FileDiff fd    = m_arFilteredList[index];
                    m_arSelectedFileList.Add(fd);
                }
                m_pProgDlg = new CProgressDlg();
                if (!InterlockedExchange(&m_bThreadRunning, TRUE))
                {
                    if (!AfxBeginThread(ExportThreadEntry, this))
                    {
                        InterlockedExchange(&m_bThreadRunning, FALSE);
                        OnCantStartThread();
                    }
                }
            }
        }
        break;
    }
}

UINT CFileDiffDlg::ExportThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CFileDiffDlg*>(pVoid)->ExportThread();
}

UINT CFileDiffDlg::ExportThread()
{
    RefreshCursor();
    if (m_pProgDlg == nullptr)
        return 1;
    long count = 0;
    SetAndClearProgressInfo(m_pProgDlg, false);
    m_pProgDlg->SetTitle(IDS_PROGRESSWAIT);
    m_pProgDlg->ShowModeless(this);
    for (INT_PTR i = 0; (i < m_arSelectedFileList.GetCount()) && (!m_pProgDlg->HasUserCancelled()); ++i)
    {
        CFileDiffDlg::FileDiff fd   = m_arSelectedFileList[i];
        CTSVNPath              url1 = CTSVNPath(m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
        CTSVNPath              url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
        if ((fd.node == svn_node_dir) && (fd.kind != svn_client_diff_summarize_kind_added))
        {
            // just create the directory
            CreateDirectoryEx(nullptr, m_strExportDir + L"\\" + CPathUtils::PathUnescape(fd.path.GetWinPathString()), nullptr);
            continue;
        }

        m_pProgDlg->FormatPathLine(1, IDS_PROGRESSGETFILE, static_cast<LPCWSTR>(url1.GetSVNPathString()));

        CTSVNPath savepath = CTSVNPath(m_strExportDir);
        savepath.AppendPathString(L"\\" + CPathUtils::PathUnescape(fd.path.GetWinPathString()));
        CPathUtils::MakeSureDirectoryPathExists(fd.node == svn_node_file ? savepath.GetContainingDirectory().GetWinPath() : savepath.GetDirectory().GetWinPath());
        if (fd.node == svn_node_dir)
        {
            // exporting a folder requires calling SVN::Export() so we also export all
            // children of that added folder.
            if ((fd.kind == svn_client_diff_summarize_kind_added) && (!Export(url2, savepath, m_bDoPegDiff ? m_peg : m_rev2, m_rev2, true, true)))
            {
                if ((!m_bDoPegDiff) || (!Export(url2, savepath, m_rev2, m_rev2, true, true)))
                {
                    SetAndClearProgressInfo(nullptr, false);
                    delete m_pProgDlg;
                    m_pProgDlg = nullptr;
                    ShowErrorDialog(m_hWnd);
                    InterlockedExchange(&m_bThreadRunning, FALSE);
                    RefreshCursor();
                    return 1;
                }
            }
        }
        else
        {
            if ((fd.kind != svn_client_diff_summarize_kind_deleted) && (!Export(url2, savepath, m_bDoPegDiff ? m_peg : m_rev2, m_rev2)))
            {
                if ((!m_bDoPegDiff) || (!Export(url2, savepath, m_rev2, m_rev2)))
                {
                    SetAndClearProgressInfo(nullptr, false);
                    delete m_pProgDlg;
                    m_pProgDlg = nullptr;
                    ShowErrorDialog(m_hWnd);
                    InterlockedExchange(&m_bThreadRunning, FALSE);
                    RefreshCursor();
                    return 1;
                }
            }
        }
        count++;
        m_pProgDlg->SetProgress(count, static_cast<DWORD>(m_arSelectedFileList.GetCount()));
    }
    m_pProgDlg->Stop();
    SetAndClearProgressInfo(nullptr, false);
    delete m_pProgDlg;
    m_pProgDlg = nullptr;
    InterlockedExchange(&m_bThreadRunning, FALSE);
    RefreshCursor();
    return 0;
}

BOOL CFileDiffDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (pWnd != &m_cFileList && this->IsWindowEnabled())
        return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
    if ((m_bThreadRunning == 0 && netScheduler.GetRunningThreadCount() == 0) || (IsCursorOverWindowBorder()))
    {
        HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
        SetCursor(hCur);
        return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
    }
    HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
    SetCursor(hCur);
    return TRUE;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CFileDiffDlg::OnEnSetfocusFirsturl()
{
    GetDlgItem(IDC_FIRSTURL)->HideCaret();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CFileDiffDlg::OnEnSetfocusSecondurl()
{
    GetDlgItem(IDC_SECONDURL)->HideCaret();
}

void CFileDiffDlg::OnBnClickedSwitchleftright()
{
    if (m_bThreadRunning)
        return;
    CString sFilterString;
    m_cFilter.GetWindowText(sFilterString);

    m_cFileList.SetRedraw(false);
    m_cFileList.DeleteAllItems();
    for (int i = 0; i < static_cast<int>(m_arFileList.size()); ++i)
    {
        FileDiff fd = m_arFileList[i];
        if (fd.kind == svn_client_diff_summarize_kind_added)
            fd.kind = svn_client_diff_summarize_kind_deleted;
        else if (fd.kind == svn_client_diff_summarize_kind_deleted)
            fd.kind = svn_client_diff_summarize_kind_added;
        m_arFileList[i] = fd;
    }
    Filter(sFilterString);

    m_cFileList.SetRedraw(true);
    CTSVNPath path = m_path1;
    m_path1        = m_path2;
    m_path2        = path;
    SVNRev rev     = m_rev1;
    m_rev1         = m_rev2;
    m_rev2         = rev;
    SetURLLabels();
}

void CFileDiffDlg::SetURLLabels()
{
    m_cRev1Btn.SetWindowText(m_rev1.ToString());
    m_cRev2Btn.SetWindowText(m_rev2.ToString());

    SetDlgItemText(IDC_FIRSTURL, m_path1.GetSVNPathString());
    SetDlgItemText(IDC_SECONDURL, m_bDoPegDiff ? m_path1.GetSVNPathString() : m_path2.GetSVNPathString());
    m_tooltips.AddTool(IDC_FIRSTURL, m_path1.GetSVNPathString());
    m_tooltips.AddTool(IDC_SECONDURL, m_bDoPegDiff ? m_path1.GetSVNPathString() : m_path2.GetSVNPathString());
}

BOOL CFileDiffDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case 'A':
            {
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                {
                    // select all entries
                    for (int i = 0; i < m_cFileList.GetItemCount(); ++i)
                    {
                        m_cFileList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
                    }
                    return TRUE;
                }
            }
            break;
            case 'C':
            case VK_INSERT:
            {
                if (GetFocus() == &m_cFileList)
                {
                    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    {
                        CopySelectionToClipboard();
                        return TRUE;
                    }
                }
            }
            break;
            case VK_RETURN:
            {
                if (GetFocus() == GetDlgItem(IDC_FILELIST))
                {
                    // Return pressed in file list. Show diff, as for double click
                    int selIndex = m_cFileList.GetSelectionMark();
                    if ((selIndex >= 0) && (selIndex < static_cast<int>(m_arFileList.size())))
                        DoDiff(selIndex, true, false, m_bBlame, true);
                    return TRUE;
                }
            }
            break;
            case VK_F5:
            {
                if (!InterlockedExchange(&m_bThreadRunning, TRUE))
                {
                    if (AfxBeginThread(DiffThreadEntry, this) == nullptr)
                    {
                        InterlockedExchange(&m_bThreadRunning, FALSE);
                        OnCantStartThread();
                    }
                }
            }
            break;
        }
    }
    return __super::PreTranslateMessage(pMsg);
}

void CFileDiffDlg::OnCancel()
{
    if (m_bThreadRunning)
    {
        m_bCancelled = true;
        return;
    }

    netScheduler.WaitForEmptyQueueOrTimeout(5000);

    __super::OnCancel();
}

void CFileDiffDlg::OnHdnItemclickFilelist(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
    if (m_bThreadRunning)
        return;

    if (m_nSortedColumn == phdr->iItem)
        m_bAscending = !m_bAscending;
    else
        m_bAscending = TRUE;
    m_nSortedColumn = phdr->iItem;
    m_arSelectedFileList.RemoveAll();
    Sort();

    CString temp;
    m_cFileList.SetRedraw(FALSE);
    m_cFileList.DeleteAllItems();
    m_cFilter.GetWindowText(temp);
    Filter(temp);

    CHeaderCtrl* pHeader    = m_cFileList.GetHeaderCtrl();
    HDITEM       headerItem = {0};
    headerItem.mask         = HDI_FORMAT;
    for (int i = 0; i < pHeader->GetItemCount(); ++i)
    {
        pHeader->GetItem(i, &headerItem);
        headerItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &headerItem);
    }
    pHeader->GetItem(m_nSortedColumn, &headerItem);
    headerItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
    pHeader->SetItem(m_nSortedColumn, &headerItem);

    m_cFileList.SetRedraw(TRUE);

    *pResult = 0;
}

void CFileDiffDlg::Sort()
{
    if (m_arFileList.size() < 2)
    {
        return;
    }

    std::sort(m_arFileList.begin(), m_arFileList.end(), &CFileDiffDlg::SortCompare);
}

bool CFileDiffDlg::SortCompare(const FileDiff& data1, const FileDiff& data2)
{
    int result = 0;
    switch (m_nSortedColumn)
    {
        case 0: //path column
            if (s_bSortLogical)
                result = StrCmpLogicalW(data1.path.GetWinPathString(), data2.path.GetWinPathString());
            else
                result = StrCmpI(data1.path.GetWinPathString(), data2.path.GetWinPathString());
            break;
        case 1: //action column
            result = data1.kind - data2.kind;
            break;
        default:
            break;
    }

    if (!m_bAscending)
        result = -result;
    return result < 0;
}

void CFileDiffDlg::OnBnClickedRev1btn()
{
    if (m_bThreadRunning)
        return; // do nothing as long as the thread is still running

    // show a dialog where the user can enter a revision
    CRevisionDlg dlg(this);
    dlg.AllowWCRevs(false);
    dlg.SetLogPath(m_path1, m_rev1);
    *static_cast<SVNRev*>(&dlg) = m_rev1;

    if (dlg.DoModal() == IDOK)
    {
        m_rev1 = static_cast<SVNRev>(dlg);
        m_cRev1Btn.SetWindowText(m_rev1.ToString());
        m_cFileList.DeleteAllItems();
        // start a new thread to re-fetch the diff
        if (!InterlockedExchange(&m_bThreadRunning, TRUE))
        {
            if (!AfxBeginThread(DiffThreadEntry, this))
            {
                InterlockedExchange(&m_bThreadRunning, FALSE);
                OnCantStartThread();
            }
        }
    }
}

void CFileDiffDlg::OnBnClickedRev2btn()
{
    if (m_bThreadRunning)
        return; // do nothing as long as the thread is still running

    // show a dialog where the user can enter a revision
    CRevisionDlg dlg(this);
    dlg.AllowWCRevs(false);
    dlg.SetLogPath(m_path2, m_rev2);
    *static_cast<SVNRev*>(&dlg) = m_rev2;

    if (dlg.DoModal() == IDOK)
    {
        m_rev2 = static_cast<SVNRev>(dlg);
        m_cRev2Btn.SetWindowText(m_rev2.ToString());
        m_cFileList.DeleteAllItems();
        // start a new thread to re-fetch the diff
        if (!InterlockedExchange(&m_bThreadRunning, TRUE))
        {
            if (!AfxBeginThread(DiffThreadEntry, this))
            {
                InterlockedExchange(&m_bThreadRunning, FALSE);
                OnCantStartThread();
            }
        }
    }
}

LRESULT CFileDiffDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_bThreadRunning)
    {
        SetTimer(IDT_FILTER, 1000, nullptr);
        return 0L;
    }

    KillTimer(IDT_FILTER);

    m_cFileList.SetRedraw(FALSE);
    m_arFilteredList.clear();
    m_cFileList.DeleteAllItems();

    Filter(L"");

    m_cFileList.SetRedraw(TRUE);
    return 0L;
}

void CFileDiffDlg::OnEnChangeFilter()
{
    SetTimer(IDT_FILTER, 1000, nullptr);
}

void CFileDiffDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (m_bThreadRunning)
        return;

    CString sFilterText;
    KillTimer(IDT_FILTER);
    m_cFilter.GetWindowText(sFilterText);

    m_cFileList.SetRedraw(FALSE);
    m_cFileList.DeleteAllItems();

    Filter(sFilterText);

    m_cFileList.SetRedraw(TRUE);

    __super::OnTimer(nIDEvent);
}

void CFileDiffDlg::Filter(CString sFilterText)
{
    sFilterText.MakeLower();

    std::vector<CString> positives;
    std::vector<CString> negatives;

    int     pos = 0;
    CString temp;
    for (;;)
    {
        temp = sFilterText.Tokenize(L" ", pos);
        if (temp.IsEmpty())
        {
            break;
        }
        if (temp[0] == '-')
            negatives.push_back(temp.Mid(1));
        else
            positives.push_back(temp);
    }

    m_arFilteredList.clear();
    for (std::vector<FileDiff>::const_iterator it = m_arFileList.begin(); it != m_arFileList.end(); ++it)
    {
        CString sPath = it->path.GetSVNPathString();
        sPath.MakeLower();

        bool bUse = true;
        for (const auto& s : negatives)
        {
            if (sPath.Find(s) >= 0)
            {
                bUse = false;
                break;
            }
        }
        if (bUse)
        {
            if (positives.empty())
                m_arFilteredList.push_back(*it);
            else
            {
                for (const auto& s : positives)
                {
                    if (sPath.Find(s) >= 0)
                    {
                        m_arFilteredList.push_back(*it);
                        break;
                    }
                }
            }
        }
    }
    m_cFileList.SetItemCount(static_cast<int>(m_arFilteredList.size()));
}

void CFileDiffDlg::CopySelectionToClipboard() const
{
    // copy all selected paths to the clipboard
    POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
    int      index;
    CString  sTextForClipboard;
    while ((index = m_cFileList.GetNextSelectedItem(pos)) >= 0)
    {
        sTextForClipboard += m_cFileList.GetItemText(index, 0);
        sTextForClipboard += L"\t";
        sTextForClipboard += m_cFileList.GetItemText(index, 1);
        sTextForClipboard += L"\r\n";
    }
    CStringUtils::WriteAsciiStringToClipboard(sTextForClipboard);
}

void CFileDiffDlg::GetSelectedPaths(CTSVNPathList& urls1, CTSVNPathList& urls2)
{
    POSITION pos = m_cFileList.GetFirstSelectedItemPosition();
    while (pos)
    {
        int                    index = m_cFileList.GetNextSelectedItem(pos);
        CFileDiffDlg::FileDiff fd    = m_arFilteredList[index];
        CTSVNPath              url1  = CTSVNPath(m_path1.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
        urls1.AddPath(url1);
        CTSVNPath url2 = m_bDoPegDiff ? url1 : CTSVNPath(m_path2.GetSVNPathString() + L"/" + fd.path.GetSVNPathString());
        urls2.AddPath(url2);
    }
}

void CFileDiffDlg::OnSetFocus(CWnd* pOldWnd)
{
    __super::OnSetFocus(pOldWnd);
    // set focus bak to the file list
    GetDlgItem(IDC_FILELIST)->SetFocus();
}
