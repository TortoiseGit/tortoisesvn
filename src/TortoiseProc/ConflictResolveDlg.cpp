// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2015 - TortoiseSVN

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
#include "ConflictResolveDlg.h"
#include "TSVNPath.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "TempFile.h"
#include "SVNProperties.h"
#include "SVN.h"

IMPLEMENT_DYNAMIC(CConflictResolveDlg, CResizableStandAloneDialog)

CConflictResolveDlg::CConflictResolveDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CConflictResolveDlg::IDD, pParent)
    , m_pConflict(NULL)
    , m_choice(svn_wc_conflict_choose_postpone)
    , m_bCancelled(false)
    , m_bIsImage(false)
    , m_bPropertyConflict(false)
    , m_svn(NULL)
{

}

CConflictResolveDlg::~CConflictResolveDlg()
{
}

void CConflictResolveDlg::SetTextConflict(SVNConflictInfo * conflict)
{
    m_bPropertyConflict = false;
    m_pConflict = conflict;
}

void CConflictResolveDlg::SetPropertyConflict(SVNConflictInfo * conflict, const CString & propertyName)
{
    m_bPropertyConflict = true;
    m_pConflict = conflict;
    m_propertyName = propertyName;
}

void CConflictResolveDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConflictResolveDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_USELOCAL, &CConflictResolveDlg::OnBnClickedUselocal)
    ON_BN_CLICKED(IDC_USEREPO, &CConflictResolveDlg::OnBnClickedUserepo)
    ON_BN_CLICKED(IDC_EDITCONFLICT, &CConflictResolveDlg::OnBnClickedEditconflict)
    ON_BN_CLICKED(IDC_RESOLVED, &CConflictResolveDlg::OnBnClickedResolved)
    ON_BN_CLICKED(IDC_RESOLVEALLLATER, &CConflictResolveDlg::OnBnClickedResolvealllater)
    ON_BN_CLICKED(IDHELP, &CConflictResolveDlg::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_ABORT, &CConflictResolveDlg::OnBnClickedAbort)
END_MESSAGE_MAP()

// CConflictResolveDlg message handlers

BOOL CConflictResolveDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(0, 0, 0, IDC_GROUP);
    m_aeroControls.SubclassControl(this, IDC_ABORT);
    m_aeroControls.SubclassControl(this, IDHELP);

    // without a conflict description, this dialog is useless.
    ASSERT(m_pConflict);
    if (m_bPropertyConflict)
    {
        if (!m_pConflict->GetPropResolutionOptions(m_conflictOptions))
        {
            m_pConflict->ShowErrorDialog(m_hWnd);
            EndDialog(IDC_ABORT);
            return FALSE;
        }
    }
    else
    {
        if (!m_pConflict->GetTextResolutionOptions(m_conflictOptions))
        {
            m_pConflict->ShowErrorDialog(m_hWnd);
            EndDialog(IDC_ABORT);
            return FALSE;
        }
    }

    CString filepath = m_pConflict->GetPath().GetWinPathString();
    CString filename = CPathUtils::GetFileNameFromPath(filepath);

    CString sInfoText;
    CString sActionText;
    CString sReasonText;
    switch (m_pConflict->GetIncomingChange())
    {
    case svn_wc_conflict_action_edit:
        if (m_bPropertyConflict)
            sActionText.FormatMessage(IDS_EDITCONFLICT_PROP_ACTIONINFO_MODIFY,
                (LPCTSTR)m_propertyName,
                (LPCTSTR)filename);
        else
            sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_MODIFY, (LPCTSTR)filename);
        break;
    case svn_wc_conflict_action_add:
        if (m_bPropertyConflict)
            sActionText.FormatMessage(IDS_EDITCONFLICT_PROP_ACTIONINFO_ADD,
                (LPCTSTR)m_propertyName,
                (LPCTSTR)filename);
        else
            sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_ADD, (LPCTSTR)filename);
        break;
    case svn_wc_conflict_action_delete:
        if (m_bPropertyConflict)
            sActionText.FormatMessage(IDS_EDITCONFLICT_PROP_ACTIONINFO_DELETE,
            (LPCTSTR)m_propertyName,
            (LPCTSTR)filename);
        else
            sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_DELETE, (LPCTSTR)filename);
        break;
    case svn_wc_conflict_action_replace:
        if (m_bPropertyConflict)
            sActionText.FormatMessage(IDS_EDITCONFLICT_PROP_ACTIONINFO_REPLACE,
            (LPCTSTR)m_propertyName,
            (LPCTSTR)filename);
        else
            sActionText.Format(IDS_EDITCONFLICT_ACTIONINFO_REPLACE, (LPCTSTR)filename);
        break;
    default:
        break;
    }

    switch (m_pConflict->GetLocalChange())
    {
    case svn_wc_conflict_reason_added:  // properties are always added
    case svn_wc_conflict_reason_edited:
        sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_EDITED);
        break;
    case svn_wc_conflict_reason_obstructed:
        sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_OBSTRUCTED);
        break;
    case svn_wc_conflict_reason_deleted:
        sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_DELETED);
        break;
    case svn_wc_conflict_reason_missing:
        sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_MISSING);
        break;
    case svn_wc_conflict_reason_unversioned:
        sReasonText.LoadString(IDS_EDITCONFLICT_REASONINFO_UNVERSIONED);
        break;
    default:
        break;
    }

    sInfoText = filepath + L"\r\n" + sActionText + L" " + sReasonText;
    SetDlgItemText(IDC_INFOLABEL, sInfoText);

    // if we deal with a binary file, editing the conflict isn't possible
    // because the 'merged_file' is not used and Subversion therefore can't
    // use it as the result of the edit.
    if (!m_bPropertyConflict && m_pConflict->IsBinary())
    {
        // in case the binary file is an image, we can use TortoiseIDiff
        m_bIsImage = IsImage(m_pConflict->GetPath());

        if (!m_bIsImage)
        {
            GetDlgItem(IDC_RESOLVELABEL)->EnableWindow(FALSE);
            GetDlgItem(IDC_EDITCONFLICT)->EnableWindow(FALSE);
            GetDlgItem(IDC_RESOLVED)->EnableWindow(FALSE);
        }
    }

    // the "resolved" button must not be enabled as long as the user hasn't used
    // the "edit" button.
    GetDlgItem(IDC_RESOLVED)->EnableWindow(FALSE);

    m_bCancelled = false;

    AddAnchor(IDC_INFOLABEL, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_GROUP, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_CHOOSELABEL, BOTTOM_LEFT);
    AddAnchor(IDC_USELOCAL, BOTTOM_LEFT);
    AddAnchor(IDC_USEREPO, BOTTOM_RIGHT);
    AddAnchor(IDC_ORLABEL, BOTTOM_LEFT);
    AddAnchor(IDC_RESOLVELABEL, BOTTOM_LEFT);
    AddAnchor(IDC_EDITCONFLICT, BOTTOM_LEFT);
    AddAnchor(IDC_RESOLVED, BOTTOM_RIGHT);
    AddAnchor(IDC_ORLABEL2, BOTTOM_LEFT);
    AddAnchor(IDC_LEAVELABEL, BOTTOM_LEFT);
    AddAnchor(IDCANCEL, BOTTOM_LEFT);
    AddAnchor(IDC_RESOLVEALLLATER, BOTTOM_RIGHT);
    AddAnchor(IDC_ABORT, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);


    return TRUE;
}

void CConflictResolveDlg::OnBnClickedUselocal()
{
    if (m_bPropertyConflict)
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_working_text_where_conflicted);
        if (!option)
            option = m_conflictOptions.FindOptionById(svn_client_conflict_option_working_text);

        if (option)
        {
            if (m_svn->ResolvePropConflict(*m_pConflict, m_propertyName, *option))
            {
                m_choice = svn_wc_conflict_choose_mine_full;
                EndDialog(IDOK);
            }
            else
            {
                m_svn->ShowErrorDialog(m_hWnd);
            }
        }
    }
    else
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_working_text_where_conflicted);
        if (!option)
            option = m_conflictOptions.FindOptionById(svn_client_conflict_option_working_text);

        if (option)
        {
            if (m_svn->ResolveTextConflict(*m_pConflict, *option))
            {
                m_choice = svn_wc_conflict_choose_mine_full;
                EndDialog(IDOK);
            }
            else
            {
                m_svn->ShowErrorDialog(m_hWnd);
            }
        }
    }
}

void CConflictResolveDlg::OnBnClickedUserepo()
{
    if (m_bPropertyConflict)
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_incoming_text_where_conflicted);
        if (!option)
            option = m_conflictOptions.FindOptionById(svn_client_conflict_option_incoming_text);

        if (option)
        {
            m_svn->ResolvePropConflict(*m_pConflict, m_propertyName, *option);
            m_choice = svn_wc_conflict_choose_theirs_full;
            EndDialog(IDOK);
        }
    }
    else
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_incoming_text_where_conflicted);
        if (!option)
            option = m_conflictOptions.FindOptionById(svn_client_conflict_option_incoming_text);

        if (option)
        {
            m_svn->ResolveTextConflict(*m_pConflict, *option);
            m_choice = svn_wc_conflict_choose_theirs_full;
            EndDialog(IDOK);
        }
    }
}

void CConflictResolveDlg::OnBnClickedEditconflict()
{
    CAppUtils::MergeFlags flags;
    flags.bAlternativeTool = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    flags.bReadOnly = true;

    CString filename, n1, n2, n3, n4;
    CTSVNPath basefile;
    CTSVNPath theirfile;
    CTSVNPath myfile;

    if (m_bPropertyConflict)
    {
        filename = m_propertyName;
        n1.Format(IDS_DIFF_PROP_WCNAME, (LPCTSTR)filename);
        n2.Format(IDS_DIFF_PROP_BASENAME, (LPCTSTR)filename);
        n3.Format(IDS_DIFF_PROP_REMOTENAME, (LPCTSTR)filename);
        n4.Format(IDS_DIFF_PROP_MERGENAME, (LPCTSTR)filename);

        if (!m_pConflict->GetPropValFiles(m_propertyName, m_mergedfile, basefile, theirfile, myfile))
        {
            m_pConflict->ShowErrorDialog(m_hWnd);
            return;
        }
    }
    else
    {
        filename = m_pConflict->GetPath().GetWinPath();
        filename = CPathUtils::GetFileNameFromPath(filename);
        n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)filename);
        n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)filename);
        n3.Format(IDS_DIFF_REMOTENAME, (LPCTSTR)filename);
        n4.Format(IDS_DIFF_MERGEDNAME, (LPCTSTR)filename);

        m_mergedfile = m_pConflict->GetPath();
        if (!m_pConflict->GetTextContentFiles(basefile, theirfile, myfile))
        {
            m_pConflict->ShowErrorDialog(m_hWnd);
            return;
        }
    }

    CAppUtils::StartExtMerge(flags,
                             basefile,
                             theirfile,
                             myfile,
                             m_mergedfile, true,
                             n2, n3, n1, n4, filename);

    GetDlgItem(IDC_RESOLVED)->EnableWindow(TRUE);
}

void CConflictResolveDlg::OnBnClickedResolved()
{
    if (m_bPropertyConflict)
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_merged_text);

        if (option)
        {
            option->SetMergedPropValFile(m_mergedfile);
            if (m_svn->ResolvePropConflict(*m_pConflict, m_propertyName, *option))
            {
                m_choice = svn_wc_conflict_choose_merged;
                EndDialog(IDOK);
            }
            else
            {
                m_svn->ShowErrorDialog(m_hWnd);
            }
        }
    }
    else
    {
        SVNConflictOption *option;
        option = m_conflictOptions.FindOptionById(svn_client_conflict_option_merged_text);

        if (option)
        {
            m_svn->ResolveTextConflict(*m_pConflict, *option);
            EndDialog(IDOK);
            m_choice = svn_wc_conflict_choose_merged;
        }
    }
}

void CConflictResolveDlg::OnBnClickedResolvealllater()
{
    m_choice = svn_wc_conflict_choose_postpone;
    m_bCancelled = true;
    EndDialog(IDOK);
}

void CConflictResolveDlg::OnCancel()
{
    m_choice = svn_wc_conflict_choose_postpone;
    m_bCancelled = true;

    CResizableStandAloneDialog::OnCancel();
}

void CConflictResolveDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CConflictResolveDlg::OnBnClickedAbort()
{
    m_bCancelled = true;
    m_choice = svn_wc_conflict_choose_postpone;
    CResizableStandAloneDialog::OnCancel();
}

bool CConflictResolveDlg::IsImage(const CTSVNPath & path)
{
    CString sExt = path.GetFileExtension();

    if ( (sExt.CompareNoCase(L".jpg") == 0) ||
         (sExt.CompareNoCase(L".jpeg") == 0) ||
         (sExt.CompareNoCase(L".bmp") == 0) ||
         (sExt.CompareNoCase(L".gif") == 0) ||
         (sExt.CompareNoCase(L".png") == 0) ||
         (sExt.CompareNoCase(L".ico") == 0) ||
         (sExt.CompareNoCase(L".tif") == 0) ||
         (sExt.CompareNoCase(L".tiff") == 0) ||
         (sExt.CompareNoCase(L".dib") == 0) ||
         (sExt.CompareNoCase(L".emf") == 0))
        {
            return true;
        }

    return false;
}
