// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2014 - TortoiseSVN

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
#include "Blame.h"
#include "ProgressDlg.h"
#include "TSVNPath.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "TempFile.h"

CBlame::CBlame()
    : SVN()
    , m_bCancelled(FALSE)
    , m_lowestrev(-1)
    , m_highestrev(-1)
    , m_nCounter(0)
    , m_nHeadRev(-1)
    , m_bNoLineNo(false)
    , extBlame(FALSE)
    , m_nFormatLine(2)
    , m_bSetProgress(true)
    , m_bHasMerges(false)
    , m_bIncludeMerge(false)
{
}
CBlame::~CBlame()
{
    m_progressDlg.Stop();
}

BOOL CBlame::BlameCallback(LONG linenumber, bool /*localchange*/, svn_revnum_t revision, const CString& author, const CString& date,
                           svn_revnum_t merged_revision, const CString& merged_author, const CString& merged_date, const CString& merged_path,
                           const CStringA& line, const CStringA& log_msg, const CStringA& merged_log_msg)
{
    if (((m_lowestrev < 0)||(m_lowestrev > revision))&&(revision >= 0))
        m_lowestrev = revision;
    if (m_highestrev < revision)
        m_highestrev = revision;
    if ((merged_revision > 0)&&(merged_revision < revision))
        m_bHasMerges = true;

    if (extBlame)
    {
        CStringA infolineA;
        CStringA fulllineA;
        svn_revnum_t origrev = revision;

        CStringA dateA(date);
        CStringA authorA(author);
        CStringA pathA(merged_path);
        char c = ' ';
        if ((merged_revision > 0)&&(merged_revision < revision))
        {
            dateA = CStringA(merged_date);
            authorA = CStringA(merged_author);
            revision = merged_revision;
            c = 'G';
        }

        if (authorA.GetLength() > 30 )
            authorA = authorA.Left(30);
        if (m_bNoLineNo)
        {
            if (m_bIncludeMerge)
            {
                if (pathA.IsEmpty())
                    infolineA.Format("%c %8ld %8ld %-30s %-30s ", c, revision, origrev, (LPCSTR)dateA, (LPCSTR)authorA);
                else
                    infolineA.Format("%c %8ld %8ld %-30s %-60s %-30s ", c, revision, origrev, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
            }
            else
            {
                if (pathA.IsEmpty())
                    infolineA.Format("%8ld %-30s %-30s ", revision, (LPCSTR)dateA, (LPCSTR)authorA);
                else
                    infolineA.Format("%8ld %-30s %-60s %-30s ", revision, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
            }
        }
        else
        {
            if (m_bIncludeMerge)
            {
                if (pathA.IsEmpty())
                    infolineA.Format("%c %6ld %8ld %8ld %-30s %-30s ", c, linenumber, revision, origrev, (LPCSTR)dateA, (LPCSTR)authorA);
                else
                    infolineA.Format("%c %6ld %8ld %8ld %-30s %-60s %-30s ", c, linenumber, revision, origrev, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
            }
            else
            {
                if (pathA.IsEmpty())
                    infolineA.Format("%6ld %8ld %-30s %-30s ", linenumber, revision, (LPCSTR)dateA, (LPCSTR)authorA);
                else
                    infolineA.Format("%6ld %8ld %-30s %-60s %-30s ", linenumber, revision, (LPCSTR)dateA, (LPCSTR)pathA, (LPCSTR)authorA);
            }
        }
        fulllineA = line;
        fulllineA.TrimRight("\r\n");
        fulllineA += "\n";
        if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
        {
            m_saveFile.WriteString(infolineA);
            m_saveFile.WriteString(fulllineA);
            return TRUE;
        }
        else
            return FALSE;
    }
    else
    {
        if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
        {
            m_saveFile.Write(&linenumber, sizeof(LONG));
            m_saveFile.Write(&revision, sizeof(svn_revnum_t));

            CStringA tempA = CUnicodeUtils::GetUTF8(author);
            int length = tempA.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)tempA, length);

            tempA = CUnicodeUtils::GetUTF8(date);
            length = tempA.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)tempA, length);

            m_saveFile.Write(&merged_revision, sizeof(svn_revnum_t));

            tempA = CUnicodeUtils::GetUTF8(merged_author);
            length = tempA.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)tempA, length);

            tempA = CUnicodeUtils::GetUTF8(merged_date);
            length = tempA.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)tempA, length);

            tempA = CUnicodeUtils::GetUTF8(merged_path);
            length = tempA.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)tempA, length);

            length = line.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)line, length);

            length = log_msg.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)log_msg, length);

            length = merged_log_msg.GetLength();
            m_saveFile.Write(&length, sizeof(int));
            m_saveFile.Write((LPCSTR)merged_log_msg, length);

            return TRUE;
        }
        else
            return FALSE;
    }
}

BOOL CBlame::Cancel()
{
    if (m_progressDlg.HasUserCancelled())
        m_bCancelled = TRUE;
    return m_bCancelled;
}

CString CBlame::BlameToTempFile(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, SVNRev pegrev,
                                const CString& options, BOOL includemerge,
                                BOOL showprogress, BOOL ignoremimetype)
{
    // if the user specified to use another tool to show the blames, there's no
    // need to fetch the log later: only TortoiseBlame uses those logs to give
    // the user additional information for the blame.
    extBlame = CRegDWORD(L"Software\\TortoiseSVN\\TextBlame", FALSE);
    m_bIncludeMerge = !!includemerge;
    CString temp;
    m_sSavePath = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
    if (m_sSavePath.IsEmpty())
        return L"";
    temp = path.GetFileExtension();
    if (!temp.IsEmpty() && !extBlame)
        m_sSavePath += temp;
    if (!m_saveFile.Open(m_sSavePath, (extBlame ? CFile::typeText : CFile::typeBinary) | CFile::modeReadWrite | CFile::modeCreate))
        return L"";
    CString headline;
    m_bNoLineNo = false;
    if (extBlame)
    {
        if(includemerge)
            headline.Format(L"%c %-6s %-8s %-8s %-30s %-60s %-30s %-s \n", ' ', L"line", L"rev", L"merged", L"date", L"path", L"author", L"content");
        else
            headline.Format(L"%c %-6s %-8s %-8s %-30s %-30s %-s \n", ' ', L"line", L"rev", L"merged", L"date",L"author", L"content");
        m_saveFile.WriteString(headline);
        m_saveFile.WriteString(L"\n");
    }

    m_progressDlg.SetTitle(IDS_BLAME_PROGRESSTITLE);
    m_progressDlg.SetAnimation(IDR_DOWNLOAD);
    m_progressDlg.SetShowProgressBar(TRUE);
    if (showprogress)
    {
        m_progressDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    }
    m_progressDlg.FormatNonPathLine(1, IDS_BLAME_PROGRESSINFO);
    m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFOSTART);
    m_progressDlg.SetCancelMsg(IDS_BLAME_PROGRESSCANCEL);
    m_progressDlg.SetTime(FALSE);
    m_nHeadRev = endrev;
    if (m_nHeadRev < 0)
        m_nHeadRev = GetHEADRevision(path);
    m_progressDlg.SetProgress(0, m_nHeadRev);

    m_bHasMerges = false;
    BOOL bBlameSuccesful = this->Blame(path, startrev, endrev, pegrev, options, !!ignoremimetype, !!includemerge);
    if (!bBlameSuccesful && !pegrev.IsValid())
    {
        // retry with the end rev as peg rev
        if (this->Blame(path, startrev, endrev, endrev, options, !!ignoremimetype, !!includemerge))
        {
            bBlameSuccesful = TRUE;
            pegrev = endrev;
        }
    }
    m_saveFile.Close();
    if (!bBlameSuccesful)
    {
        DeleteFile(m_sSavePath);
        m_sSavePath.Empty();
    }
    m_progressDlg.SetLine(2, L"");
    if (m_pProgressDlg)
        m_pProgressDlg->SetLine(m_nFormatLine, L"");
    m_progressDlg.Stop();

    return m_sSavePath;
}
BOOL CBlame::Notify(const CTSVNPath& /*path*/, const CTSVNPath& /*url*/, svn_wc_notify_action_t /*action*/,
                    svn_node_kind_t /*kind*/, const CString& /*mime_type*/,
                    svn_wc_notify_state_t /*content_state*/,
                    svn_wc_notify_state_t /*prop_state*/, svn_revnum_t rev,
                    const svn_lock_t * /*lock*/, svn_wc_notify_lock_state_t /*lock_state*/,
                    const CString& /*changelistname*/,
                    const CString& /*propertyName*/,
                    svn_merge_range_t * /*range*/,
                    svn_error_t * /*err*/, apr_pool_t * /*pool*/)
{
    m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFO2, rev, m_nHeadRev);
    m_progressDlg.SetProgress(rev, m_nHeadRev);
    if (m_pProgressDlg)
    {
        m_pProgressDlg->FormatNonPathLine(m_nFormatLine, IDS_BLAME_PROGRESSINFO2, rev, m_nHeadRev);
        if (m_bSetProgress)
            m_pProgressDlg->SetProgress(rev, m_nHeadRev);
    }
    return TRUE;
}

bool CBlame::BlameToFile(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, SVNRev peg,
                         const CTSVNPath& tofile, const CString& options, BOOL ignoremimetype, BOOL includemerge)
{
    extBlame = TRUE;
    CString temp;
    if (!m_saveFile.Open(tofile.GetWinPathString(), CFile::typeText | CFile::modeReadWrite | CFile::modeCreate))
        return false;
    m_bNoLineNo = true;
    m_nHeadRev = endrev;
    if (m_nHeadRev < 0)
        m_nHeadRev = GetHEADRevision(path);
    m_bIncludeMerge = !!includemerge;

    BOOL bBlameSuccesful = this->Blame(path, startrev, endrev, peg, options, !!ignoremimetype, !!includemerge);
    if ( !bBlameSuccesful && !peg.IsValid() )
    {
        // retry with the end rev as peg rev
        if (this->Blame(path, startrev, endrev, endrev, options, !!ignoremimetype, !!includemerge))
        {
            bBlameSuccesful = TRUE;
            peg = endrev;
        }
    }

    m_progressDlg.SetLine(2, L"");
    if (m_pProgressDlg)
        m_pProgressDlg->SetLine(m_nFormatLine, L"");

    if (!bBlameSuccesful)
    {
        m_saveFile.Close();
        return false;
    }

    if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
        m_saveFile.Close();

    return true;
}
