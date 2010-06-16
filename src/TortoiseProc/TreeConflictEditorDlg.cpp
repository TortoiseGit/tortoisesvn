// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2010 - TortoiseSVN

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
#include "TreeConflictEditorDlg.h"

#include "SVN.h"
#include "SVNInfo.h"
#include "SVNStatus.h"
#include "SVNProgressDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"

// CTreeConflictEditorDlg dialog

IMPLEMENT_DYNAMIC(CTreeConflictEditorDlg, CResizableStandAloneDialog)

CTreeConflictEditorDlg::CTreeConflictEditorDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CTreeConflictEditorDlg::IDD, pParent)
    , m_bThreadRunning(false)
{

}

CTreeConflictEditorDlg::~CTreeConflictEditorDlg()
{
}

void CTreeConflictEditorDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTreeConflictEditorDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_RESOLVEUSINGTHEIRS, &CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs)
    ON_BN_CLICKED(IDC_RESOLVEUSINGMINE, &CTreeConflictEditorDlg::OnBnClickedResolveusingmine)
    ON_REGISTERED_MESSAGE(WM_AFTERTHREAD, OnAfterThread)
    ON_BN_CLICKED(IDC_LOG, &CTreeConflictEditorDlg::OnBnClickedShowlog)
    ON_BN_CLICKED(IDHELP, &CTreeConflictEditorDlg::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_BRANCHLOG, &CTreeConflictEditorDlg::OnBnClickedBranchlog)
END_MESSAGE_MAP()


// CTreeConflictEditorDlg message handlers

BOOL CTreeConflictEditorDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_SOURCEGROUP, IDC_SOURCEGROUP, IDC_SOURCEGROUP, IDC_SOURCEGROUP);
    m_aeroControls.SubclassControl(this, IDC_CONFLICTINFO);
    m_aeroControls.SubclassControl(this, IDC_INFOLABEL);
    m_aeroControls.SubclassControl(this, IDC_RESOLVEUSINGTHEIRS);
    m_aeroControls.SubclassControl(this, IDC_RESOLVEUSINGMINE);
    m_aeroControls.SubclassControl(this, IDC_LOG);
    m_aeroControls.SubclassControl(this, IDC_BRANCHLOG);
    m_aeroControls.SubclassControl(this, IDCANCEL);
    m_aeroControls.SubclassControl(this, IDHELP);

    SetDlgItemText(IDC_CONFLICTINFO, m_sConflictInfo);
    SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, m_sUseTheirs);
    SetDlgItemText(IDC_RESOLVEUSINGMINE, m_sUseMine);


    CString sTemp;
    sTemp.Format(_T("%s/%s@%ld"), (LPCTSTR)src_left_version_url, src_left_version_path, src_left_version_rev);
    SetDlgItemText(IDC_SOURCELEFTURL, sTemp);
    sTemp.Format(_T("%s/%s@%ld"), (LPCTSTR)src_right_version_url, (LPCTSTR)src_right_version_path, src_right_version_rev);
    SetDlgItemText(IDC_SOURCERIGHTURL, sTemp);

    if (conflict_reason == svn_wc_conflict_reason_deleted)
    {
        // unfortunately, if the action is also 'delete', then we don't have
        // the url of the item anymore and therefore can't find the copyfrom
        // path of the locally renamed item. So starting the thread to do that
        // would be a waste of time...
        if (conflict_action != svn_wc_conflict_action_delete)
        {
            // start thread to find the possible copyfrom item
            InterlockedExchange(&m_bThreadRunning, TRUE);
            if (AfxBeginThread(StatusThreadEntry, this)==NULL)
            {
                InterlockedExchange(&m_bThreadRunning, FALSE);
                OnCantStartThread();
            }
        }
        else
        {
            // two deletes leave as the only option: mark the conflict as resolved :(
            SetDlgItemText(IDC_INFOLABEL, CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_HINT_DELETEUPONDELETE)));
            GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
        }
    }
    else if (conflict_reason == svn_wc_conflict_reason_missing)
    {
        // a missing item leaves as the only option: mark the conflict as resolved :(
        if (conflict_action == svn_wc_conflict_action_edit)
            SetDlgItemText(IDC_INFOLABEL, CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_HINT_EDITUPONDELETE)));
        else
            SetDlgItemText(IDC_INFOLABEL, CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_HINT_DELETEUPONDELETE)));
        GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
    }
    else
    {
        SetDlgItemText(IDC_INFOLABEL, CString(MAKEINTRESOURCE(IDC_TREECONFLICT_HOWTORESOLVE)));
        GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
        GetDlgItem(IDC_RESOLVEUSINGTHEIRS)->ShowWindow(SW_SHOW);
    }

    AddAnchor(IDC_CONFLICTINFO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCELEFT, TOP_LEFT);
    AddAnchor(IDC_SOURCERIGHT, TOP_LEFT);
    AddAnchor(IDC_SOURCELEFTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCERIGHTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_INFOLABEL, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_RESOLVEUSINGTHEIRS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_RESOLVEUSINGMINE, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_LOG, BOTTOM_LEFT);
    AddAnchor(IDC_BRANCHLOG, BOTTOM_LEFT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTreeConflictEditorDlg::OnBnClickedShowlog()
{
    if (m_bThreadRunning)
        return;
    CTSVNPath logPath = m_path;
    if (logPath.GetContainingDirectory().HasAdminDir())
        logPath = logPath.GetContainingDirectory();
    CString sCmd;
    sCmd.Format(_T("\"%s\" /command:log /path:\"%s\""), (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")), logPath.GetWinPath());
    CAppUtils::LaunchApplication(sCmd, NULL, false);
}

void CTreeConflictEditorDlg::OnBnClickedBranchlog()
{
    if (m_bThreadRunning)
        return;
    CString sTemp;
    sTemp.Format(_T("%s/%s"), (LPCTSTR)src_left_version_url, (LPCTSTR)src_left_version_path);

    CTSVNPath logPath = CTSVNPath(sTemp);
    if (src_left_version_kind != svn_node_dir)
        logPath = logPath.GetContainingDirectory();
    CString sCmd;
    sCmd.Format(_T("\"%s\" /command:log /path:\"%s\" /pegrev:%ld"),
        (LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
        (LPCTSTR)logPath.GetSVNPathString(),
        (svn_revnum_t)src_left_version_rev);
    CAppUtils::LaunchApplication(sCmd, NULL, false);
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs()
{
    if (m_bThreadRunning)
        return;

    int retVal = IDC_RESOLVEUSINGTHEIRS;
    SVN svn;

    if (conflict_reason == svn_wc_conflict_reason_deleted)
    {
        if ((!m_copyfromPath.IsEmpty())&&(src_left_version_rev.IsValid()))
        {
            CTSVNPath url1 = CTSVNPath(src_left_version_url);
            url1.AppendPathString(src_left_version_path);
            CTSVNPath url2 = CTSVNPath(src_right_version_url);
            url2.AppendPathString(src_right_version_path);

            CSVNProgressDlg progDlg;
            progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
            progDlg.SetPathList(CTSVNPathList(m_copyfromPath));
            progDlg.SetUrl(url1.GetSVNPathString());
            progDlg.SetSecondUrl(url2.GetSVNPathString());
            progDlg.SetRevision(src_left_version_rev);
            progDlg.SetRevisionEnd(src_right_version_rev);
            if (url1.IsEquivalentTo(url2))
            {
                SVNRevRange range;
                range.SetRange(src_left_version_rev, src_right_version_rev);
                SVNRevRangeArray array;
                array.AddRevRange(range);
                progDlg.SetRevisionRanges(array);
            }
            int options = ProgOptIgnoreAncestry;
            progDlg.SetOptions(options);
            progDlg.DoModal();
            return;
        }
        else
        {
            // get the file back from the repository:
            // revert the deletion
            svn.Revert(CTSVNPathList(m_path), CStringArray(), false);
        }
    }

    if (!svn.Resolve(m_path, svn_wc_conflict_choose_merged, false))
    {
        CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
        retVal = IDCANCEL;
    }
    else
    {
        // Subversion conflict resolving does *not* remove files/dirs automatically but
        // (currently?) simply marks the conflict as resolved.
        // We try to do the deletion here ourselves since that's what the dialog button
        // suggested
        if ((m_sUseTheirs.Compare(CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_REMOVEFILE))) == 0)||
            (m_sUseTheirs.Compare(CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_REMOVEDIR))) == 0))
        {
            if (m_path.Exists())
            {
                if (!svn.Remove(CTSVNPathList(m_path), true, false))
                {
                    CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
                    retVal = IDCANCEL;
                }
            }
        }
    }
    EndDialog(retVal);
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingmine()
{
    if (m_bThreadRunning)
        return;

    int retVal = IDC_RESOLVEUSINGMINE;
    SVN svn;
    if (!svn.Resolve(m_path, svn_wc_conflict_choose_merged, false))
    {
        CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
        retVal = IDCANCEL;
    }
    else
    {
        // Subversion conflict resolving does *not* remove files/dirs automatically but
        // (currently?) simply marks the conflict as resolved.
        // We try to do the deletion here ourselves since that's what the dialog button
        // suggested
        if ((m_sUseMine.Compare(CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_REMOVEFILE))) == 0)||
            (m_sUseMine.Compare(CString(MAKEINTRESOURCE(IDS_TREECONFLICT_RESOLVE_REMOVEDIR))) == 0))
        {
            if (m_path.Exists())
            {
                if (!svn.Remove(CTSVNPathList(m_path), true, false))
                {
                    CMessageBox::Show(m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
                    retVal = IDCANCEL;
                }
            }
        }
    }
    EndDialog(retVal);
}


//this is the thread function which calls the subversion function
UINT CTreeConflictEditorDlg::StatusThreadEntry(LPVOID pVoid)
{
    return ((CTreeConflictEditorDlg*)pVoid)->StatusThread();
}


//this is the thread function which calls the subversion function
UINT CTreeConflictEditorDlg::StatusThread()
{
    InterlockedExchange(&m_bThreadRunning, TRUE);
    m_progressDlg.SetTitle(IDS_REPOBROWSE_WAIT);
    m_progressDlg.SetShowProgressBar(false);
    m_progressDlg.ShowModeless(m_hWnd, false);
    SVNInfo info;
    const SVNInfoData * pData = info.GetFirstFileInfo(m_path, SVNRev(), SVNRev());
    CTSVNPath statPath = m_path.GetContainingDirectory();
    bool bFound = false;
    m_copyfromPath.Reset();
    if (pData)
    {
        CString url = pData->url;

        SVNStatus stat;
        SVNInfo info;
        do
        {
            const SVNInfoData * infodata = info.GetFirstFileInfo(statPath, SVNRev::REV_WC, SVNRev::REV_WC, svn_depth_infinity);
            if (infodata)
            {
                do
                {
                    if (infodata->copyfromurl)
                    {
                        if (infodata->copyfromurl.Compare(url) == 0)
                        {
                            bFound = true;
                            m_copyfromPath = infodata->path;
                            break;  // found the copyfrom path!
                        }
                    }
                } while ((infodata = info.GetNextFileInfo()) != NULL);
            }
            statPath = statPath.GetContainingDirectory();
        } while (!bFound && statPath.HasAdminDir());
    }

    PostMessage(WM_AFTERTHREAD);
    m_progressDlg.Stop();
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

LRESULT CTreeConflictEditorDlg::OnAfterThread(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if ((!m_copyfromPath.IsEmpty())&&(src_left_version_rev.IsValid()))
    {
        m_sUseTheirs.Format(IDS_TREECONFLICT_RESOLVE_MERGECHANGES, (LPCTSTR)m_copyfromPath.GetFileOrDirectoryName());
        SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, m_sUseTheirs);
    }

    GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
    GetDlgItem(IDC_RESOLVEUSINGTHEIRS)->ShowWindow(SW_SHOW);
    SetDlgItemText(IDC_INFOLABEL, CString(MAKEINTRESOURCE(IDC_TREECONFLICT_HOWTORESOLVE)));

    return 0;
}


void CTreeConflictEditorDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CTreeConflictEditorDlg::SetConflictLeftSources(const CString& url, const CString& path, const SVNRev& rev, svn_node_kind_t kind)
{
    src_left_version_url = url;
    src_left_version_path = path;
    src_left_version_rev = rev;
    src_left_version_kind = kind;
}

void CTreeConflictEditorDlg::SetConflictRightSources(const CString& url, const CString& path, const SVNRev& rev, svn_node_kind_t kind)
{
    src_right_version_url = url;
    src_right_version_path = path;
    src_right_version_rev = rev;
    src_right_version_kind = kind;
}

