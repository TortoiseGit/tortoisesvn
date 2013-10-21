// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseSVN

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
#include "SVNHelpers.h"
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
    , conflict_reason(svn_wc_conflict_reason_edited)
    , conflict_action(svn_wc_conflict_action_edit)
    , src_right_version_kind(svn_node_unknown)
    , src_left_version_kind(svn_node_unknown)
    , m_theirsChoice(svn_wc_conflict_choose_merged)
    , m_mineChoice(svn_wc_conflict_choose_mine_full)
    , m_choice(svn_wc_conflict_choose_postpone)
    , m_bInteractive(false)
    , m_bCancelled(false)
{

}

CTreeConflictEditorDlg::~CTreeConflictEditorDlg()
{
}

void CTreeConflictEditorDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SOURCELEFTURL, src_leftedit);
    DDX_Control(pDX, IDC_SOURCERIGHTURL, src_rightedit);
}


BEGIN_MESSAGE_MAP(CTreeConflictEditorDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_RESOLVEUSINGTHEIRS, &CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs)
    ON_BN_CLICKED(IDC_RESOLVEUSINGMINE, &CTreeConflictEditorDlg::OnBnClickedResolveusingmine)
    ON_BN_CLICKED(IDC_LOG, &CTreeConflictEditorDlg::OnBnClickedShowlog)
    ON_BN_CLICKED(IDHELP, &CTreeConflictEditorDlg::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_BRANCHLOG, &CTreeConflictEditorDlg::OnBnClickedBranchlog)
    ON_BN_CLICKED(IDC_POSTPONE_ALL, &CTreeConflictEditorDlg::OnBnClickedPostponeAll)
    ON_BN_CLICKED(IDC_ABORT, &CTreeConflictEditorDlg::OnBnClickedAbort)
END_MESSAGE_MAP()


// CTreeConflictEditorDlg message handlers

BOOL CTreeConflictEditorDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    CString sConflictAction;
    CString sConflictReason;
    CString sResolveTheirs;
    CString sResolveMine;
    CString sItemName = m_path.GetUIFileOrDirectoryName();

    if (kind == svn_node_file)
    {
        switch (conflict_operation)
        {
        default:
        case svn_wc_operation_none:
        case svn_wc_operation_update:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_FILEUPDATEREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            }
            break;
        case svn_wc_operation_switch:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_FILESWITCHREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            }
            break;
        case svn_wc_operation_merge:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYFILE);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_FILEMERGEREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEFILE);
                break;
            }
            break;
        }
    }
    else //if (pInfoData->treeconflict_nodekind == svn_node_dir)
    {
        switch (conflict_operation)
        {
        default:
        case svn_wc_operation_none:
        case svn_wc_operation_update:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_DIRUPDATEREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            }
            break;
        case svn_wc_operation_switch:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_DIRSWITCHREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            }
            break;
        case svn_wc_operation_merge:
            switch (conflict_action)
            {
            default:
            case svn_wc_conflict_action_edit:
                sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEEDIT, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_add:
                sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEADD, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPREPOSITORYDIR);
                break;
            case svn_wc_conflict_action_delete:
                sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEDELETE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            case svn_wc_conflict_action_replace:
                sConflictAction.Format(IDS_TREECONFLICT_DIRMERGEREPLACE, (LPCTSTR)sItemName);
                sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_REMOVEDIR);
                break;
            }
            break;
        }
    }

    UINT uReasonID = 0;
    switch (conflict_reason)
    {
    case svn_wc_conflict_reason_edited:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_EDITED : IDS_TREECONFLICT_REASON_FILE_EDITED;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_obstructed:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_OBSTRUCTED : IDS_TREECONFLICT_REASON_FILE_OBSTRUCTED;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_deleted:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_DELETED : IDS_TREECONFLICT_REASON_FILE_DELETED;
        sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
        break;
    case svn_wc_conflict_reason_added:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_ADDED : IDS_TREECONFLICT_REASON_FILE_ADDED;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_missing:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MISSING : IDS_TREECONFLICT_REASON_FILE_MISSING;
        sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
        break;
    case svn_wc_conflict_reason_unversioned:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_UNVERSIONED : IDS_TREECONFLICT_REASON_FILE_UNVERSIONED;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_replaced:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_REPLACED : IDS_TREECONFLICT_REASON_FILE_REPLACED;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_moved_away:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MOVEDAWAY : IDS_TREECONFLICT_REASON_FILE_MOVEDAWAY;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    case svn_wc_conflict_reason_moved_here:
        uReasonID = kind == svn_node_dir ? IDS_TREECONFLICT_REASON_DIR_MOVEDHERE : IDS_TREECONFLICT_REASON_FILE_MOVEDHERE;
        sResolveMine.LoadString(kind == svn_node_dir ? IDS_TREECONFLICT_RESOLVE_KEEPLOCALDIR : IDS_TREECONFLICT_RESOLVE_KEEPLOCALFILE);
        break;
    }
    sConflictReason.Format(uReasonID, (LPCTSTR)sConflictAction);

    sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_MARKASRESOLVED);
    SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, sResolveTheirs);
    GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_HIDE);

    SetDlgItemText(IDC_CONFLICTINFO, sConflictReason);
    SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, sResolveTheirs);
    SetDlgItemText(IDC_RESOLVEUSINGMINE, sResolveMine);

    if (m_bInteractive)
    {
        // hide the log buttons in interactive mode
        GetDlgItem(IDC_LOG)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_BRANCHLOG)->ShowWindow(SW_HIDE);
    }
    else
    {
        GetDlgItem(IDC_POSTPONE_ALL)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_ABORT)->ShowWindow(SW_HIDE);
    }
    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sWindowTitle);

    CString sTemp;
    sTemp.Format(_T("%s/%s@%ld"), (LPCTSTR)src_left_version_url, (LPCWSTR)src_left_version_path, (svn_revnum_t)src_left_version_rev);
    SetDlgItemText(IDC_SOURCELEFTURL, sTemp);
    sTemp.Format(_T("%s/%s@%ld"), (LPCTSTR)src_right_version_url, (LPCTSTR)src_right_version_path, (svn_revnum_t)src_right_version_rev);
    SetDlgItemText(IDC_SOURCERIGHTURL, sTemp);


    if (conflict_operation == svn_wc_operation_update ||
        conflict_operation == svn_wc_operation_switch)
    {
        if (conflict_reason == svn_wc_conflict_reason_moved_away)
        {
            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_APPLYTOMOVE);
            sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MOVETOCOPY);
            SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, sResolveTheirs);
            SetDlgItemText(IDC_RESOLVEUSINGMINE, sResolveMine);
            GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
        }
        else if (conflict_reason == svn_wc_conflict_reason_deleted)
        {
            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPMOVES);
            sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MOVETOCOPY);
            SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, sResolveTheirs);
            SetDlgItemText(IDC_RESOLVEUSINGMINE, sResolveMine);
            GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
        }
        else if (conflict_reason == svn_wc_conflict_reason_replaced)
        {
            sResolveTheirs.LoadString(IDS_TREECONFLICT_RESOLVE_KEEPMOVES);
            sResolveMine.LoadString(IDS_TREECONFLICT_RESOLVE_MOVETOCOPY);
            SetDlgItemText(IDC_RESOLVEUSINGTHEIRS, sResolveTheirs);
            SetDlgItemText(IDC_RESOLVEUSINGMINE, sResolveMine);
            GetDlgItem(IDC_RESOLVEUSINGMINE)->ShowWindow(SW_SHOW);
        }
        else
        {
            // use default texts and resolve answers
        }
    }


    AddAnchor(IDC_CONFLICTINFO, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCEGROUP, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCELEFT, TOP_LEFT);
    AddAnchor(IDC_SOURCERIGHT, TOP_LEFT);
    AddAnchor(IDC_SOURCELEFTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_SOURCERIGHTURL, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_RESOLVEUSINGTHEIRS, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_RESOLVEUSINGMINE, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDC_POSTPONE_ALL, BOTTOM_LEFT);
    AddAnchor(IDC_LOG, BOTTOM_LEFT);
    AddAnchor(IDC_BRANCHLOG, BOTTOM_LEFT);
    AddAnchor(IDC_ABORT, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

CString CTreeConflictEditorDlg::GetShowLogCmd(const CTSVNPath &logPath, const CString &sFile)
{
    CString sFilter;
    if (!sFile.IsEmpty())
        sFilter.Format(_T("/findstring:\"%s\" /findtype:2 /findtext"), (LPCTSTR)sFile);

    CString sCmd;
    sCmd.Format(_T("/command:log /path:\"%s\" /pegrev:%ld %s"),
        (LPCTSTR)logPath.GetSVNPathString(),
        (svn_revnum_t)src_left_version_rev,
        (LPCTSTR)sFilter);

    return sCmd;
}

void CTreeConflictEditorDlg::OnBnClickedShowlog()
{
    CString sFile;
    CTSVNPath logPath = m_path;
    if (SVNHelper::IsVersioned(logPath.GetContainingDirectory(), true))
    {
        sFile = logPath.GetFilename();
        logPath = logPath.GetContainingDirectory();
    }
    CAppUtils::RunTortoiseProc(GetShowLogCmd(logPath, sFile));
}

void CTreeConflictEditorDlg::OnBnClickedBranchlog()
{
    CString sTemp;
    sTemp.Format(_T("%s/%s"), (LPCTSTR)src_left_version_url, (LPCTSTR)src_left_version_path);

    CString sFile;
    CTSVNPath logPath = CTSVNPath(sTemp);
    if (src_left_version_kind != svn_node_dir)
    {
        sFile = logPath.GetFilename();
        logPath = logPath.GetContainingDirectory();
    }

    CAppUtils::RunTortoiseProc(GetShowLogCmd(logPath, sFile));
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingtheirs()
{
    int retVal = IDOK;
    if (m_bInteractive)
    {
        m_choice = svn_wc_conflict_choose_merged;
        EndDialog(retVal);
    }
    else
    {
        SVN svn;
        if (!svn.Resolve(m_path, svn_wc_conflict_choose_merged, false, true, svn_wc_conflict_kind_tree))
        {
            svn.ShowErrorDialog(m_hWnd, m_path);
            retVal = IDCANCEL;
        }
        else
            m_choice = svn_wc_conflict_choose_merged;
        EndDialog(retVal);
    }
}

void CTreeConflictEditorDlg::OnBnClickedResolveusingmine()
{
    int retVal = IDOK;
    if (m_bInteractive)
    {
        m_choice = svn_wc_conflict_choose_mine_conflict;
        EndDialog(retVal);
    }
    else
    {
        SVN svn;
        if (!svn.Resolve(m_path, svn_wc_conflict_choose_mine_conflict, false, true, svn_wc_conflict_kind_tree))
        {
            svn.ShowErrorDialog(m_hWnd, m_path);
            retVal = IDCANCEL;
        }
        else
            m_choice = svn_wc_conflict_choose_mine_conflict;

        EndDialog(retVal);
    }
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

void CTreeConflictEditorDlg::SetConflictSources( const svn_wc_conflict_version_t * left, const svn_wc_conflict_version_t * right )
{
    src_left_version_url    = CUnicodeUtils::GetUnicode(left->repos_url);
    src_left_version_path   = CUnicodeUtils::GetUnicode(left->path_in_repos);
    src_left_version_rev    = left->peg_rev;
    src_left_version_kind   = left->node_kind;

    src_right_version_url   = CUnicodeUtils::GetUnicode(right->repos_url);
    src_right_version_path  = CUnicodeUtils::GetUnicode(right->path_in_repos);
    src_right_version_rev   = right->peg_rev;
    src_right_version_kind  = right->node_kind;
}



void CTreeConflictEditorDlg::OnBnClickedPostponeAll()
{
    EndDialog(IDOK);
}


void CTreeConflictEditorDlg::OnBnClickedAbort()
{
    m_bCancelled = true;
    EndDialog(IDC_POSTPONE_ALL);
}
