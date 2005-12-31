// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "Blame.h"
#include "ProgressDlg.h"
#include "TSVNPath.h"
#include "Registry.h"
#include "Utils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"


void CStdioFileA::WriteString(LPCSTR lpsz)
{
	ASSERT(lpsz != NULL);
	ASSERT(m_pStream != NULL);
	
	if (lpsz == NULL)
	{
		AfxThrowInvalidArgException();
	}

	if (fputs(lpsz, m_pStream) == EOF)
		AfxThrowFileException(CFileException::diskFull, _doserrno, m_strFileName);
}


void CStdioFileA::WriteString(LPCWSTR lpsz)
{
	ASSERT(lpsz != NULL);
	ASSERT(m_pStream != NULL);
	
	if (lpsz == NULL)
	{
		AfxThrowInvalidArgException();
	}

	if (fputws(lpsz, m_pStream) == EOF)
		AfxThrowFileException(CFileException::diskFull, _doserrno, m_strFileName);
}



CBlame::CBlame()
{
	m_bCancelled = FALSE;
	m_lowestrev = -1;
	m_highestrev = -1;
	m_nCounter = 0;
	m_nHeadRev = -1;
}
CBlame::~CBlame()
{
	m_progressDlg.Stop();
}

BOOL CBlame::BlameCallback(LONG linenumber, LONG revision, const CString& author, const CString& date, const CStringA& line)
{
	CStringA infolineA;
	CStringA fulllineA;

	if ((m_lowestrev < 0)||(m_lowestrev > revision))
		m_lowestrev = revision;
	if (m_highestrev < revision)
		m_highestrev = revision;

	CStringA dateA(date);
	CStringA authorA(author);

	infolineA.Format("%6ld %6ld %30s %-30s ", linenumber, revision, dateA, authorA);
	fulllineA = line;
	fulllineA.TrimRight("\r\n");
	fulllineA += "\n";
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
	{
		m_saveFile.WriteString(infolineA);
		m_saveFile.WriteString(fulllineA);
	}
	else
		return FALSE;
	return TRUE;
}

BOOL CBlame::Log(svn_revnum_t revision, const CString& /*author*/, const CString& /*date*/, const CString& message, LogChangedPathArray * cpaths, apr_time_t /*time*/, int /*filechanges*/, BOOL /*copies*/, DWORD /*actions*/)
{
	m_progressDlg.SetProgress(m_highestrev - revision, m_highestrev);
	if (m_saveLog.m_hFile != INVALID_HANDLE_VALUE)
	{
		CStringA msgutf8 = CUnicodeUtils::GetUTF8(message);
		int length = msgutf8.GetLength();
		m_saveLog.Write(&revision, sizeof(LONG));
		m_saveLog.Write(&length, sizeof(int));
		m_saveLog.Write((LPCSTR)msgutf8, length);
	}
	for (INT_PTR i=0; i<cpaths->GetCount(); ++i)
		delete cpaths->GetAt(i);
	cpaths->RemoveAll();
	delete cpaths;
	return TRUE;
}

BOOL CBlame::Cancel()
{
	if (m_progressDlg.HasUserCancelled())
		m_bCancelled = TRUE;
	return m_bCancelled;
}

CString CBlame::BlameToTempFile(const CTSVNPath& path, SVNRev startrev, SVNRev endrev, CString& logfile, BOOL showprogress /* = TRUE */)
{
	BOOL extBlame = CRegDWORD(_T("Software\\TortoiseSVN\\TextBlame"), FALSE);
	CString temp;
	m_sSavePath = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
	if (m_sSavePath.IsEmpty())
		return _T("");
	temp = path.GetFileExtension();
	if (!temp.IsEmpty() && !extBlame)
		m_sSavePath += temp;
	if (!m_saveFile.Open(m_sSavePath, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate))
		return _T("");
	CString headline;
	headline.Format(_T("%-6s %-6s %-30s %-30s %-s \n"), _T("line"), _T("rev"), _T("date"), _T("author"), _T("content"));
	m_saveFile.WriteString(headline);
	m_saveFile.WriteString(_T("\n"));
	m_progressDlg.SetTitle(IDS_BLAME_PROGRESSTITLE);
	m_progressDlg.SetAnimation(IDR_SEARCH);
	m_progressDlg.SetShowProgressBar(TRUE);
	if (showprogress)
	{
		m_progressDlg.ShowModeless(CWnd::FromHandle(hWndExplorer));
	}
	m_progressDlg.FormatNonPathLine(1, IDS_BLAME_PROGRESSINFO);
	m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFOSTART);
	m_progressDlg.SetCancelMsg(IDS_BLAME_PROGRESSCANCEL);
	m_progressDlg.SetTime(FALSE);
	m_nHeadRev = endrev;
	if (m_nHeadRev < 0)
		m_nHeadRev = GetHEADRevision(path);
	m_progressDlg.SetProgress(0, m_nHeadRev);
	if (!this->Blame(path, startrev, endrev))
	{
		m_saveFile.Close();
		DeleteFile(m_sSavePath);
		m_sSavePath.Empty();
	}
	else if (!extBlame)
	{
		m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSLOGSTART);
		m_progressDlg.SetProgress(0, m_highestrev);
		logfile = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString();
		if (!m_saveLog.Open(logfile, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate))
		{
			logfile.Empty();
			return m_sSavePath;
		}
		if (!this->ReceiveLog(CTSVNPathList(path), SVNRev::REV_HEAD, m_lowestrev, 0, TRUE))
		{
			m_saveLog.Close();
			DeleteFile(logfile);
			logfile.Empty();
		}
	}
	m_progressDlg.Stop();
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
		m_saveFile.Close();

	return m_sSavePath;
}
BOOL CBlame::Notify(const CTSVNPath& /*path*/, svn_wc_notify_action_t /*action*/, 
					svn_node_kind_t /*kind*/, const CString& /*mime_type*/, 
					svn_wc_notify_state_t /*content_state*/, 
					svn_wc_notify_state_t /*prop_state*/, LONG rev,
					const svn_lock_t * /*lock*/, svn_wc_notify_lock_state_t /*lock_state*/,
					svn_error_t * /*err*/, apr_pool_t * /*pool*/)
{
	m_progressDlg.FormatNonPathLine(2, IDS_BLAME_PROGRESSINFO2, rev, m_nHeadRev);
	m_progressDlg.SetProgress(rev, m_nHeadRev);
	return TRUE;
}


