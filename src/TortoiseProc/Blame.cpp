// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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
#include "Blame.h"
#include "Utils.h"
#include "ProgressDlg.h"
#include "TortoiseProc.h"

CBlame::CBlame()
{
	m_bCancelled = FALSE;
}
CBlame::~CBlame()
{
	m_progressDlg.Stop();
}

BOOL CBlame::BlameCallback(LONG linenumber, LONG revision, CString author, CString date, CString line)
{
	line = line.TrimRight(_T("\n\r"));
	CString fullline;
	fullline.Format(_T("%6ld %6ld %20s %-30s %s \n"), linenumber, revision, date, author, line);
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
		m_saveFile.WriteString(fullline);
	else
		return FALSE;
	return TRUE;
}

BOOL CBlame::Cancel()
{
	if (m_progressDlg.HasUserCancelled())
		m_bCancelled = TRUE;
	return m_bCancelled;
}

CString CBlame::BlameToTempFile(CString path, LONG startrev, LONG endrev, BOOL strict, BOOL showprogress /* = TRUE */)
{
	CString temp;
	m_sSavePath = CUtils::GetTempFile();
	if (m_sSavePath.IsEmpty())
		return _T("");
	if (!m_saveFile.Open(m_sSavePath, CFile::typeText | CFile::modeReadWrite))
		return _T("");
	CString headline;
	headline.Format(_T("%-6s %-6s %-20s %-30s %-s \n"), _T("line"), _T("rev"), _T("date"), _T("author"), _T("content"));
	m_saveFile.WriteString(headline);
	m_saveFile.WriteString(_T("\n"));
	temp.LoadString(IDS_BLAME_PROGRESSTITLE);
	m_progressDlg.SetTitle(temp);
	m_progressDlg.SetAnimation(IDR_SEARCH);
	temp.LoadString(IDS_BLAME_PROGRESSINFO);
	m_progressDlg.SetLine(1, temp, false);
	temp.LoadString(IDS_BLAME_PROGRESSCANCEL);
	m_progressDlg.SetCancelMsg(temp);
	m_progressDlg.SetShowProgressBar(FALSE);
	m_progressDlg.SetTime(FALSE);
	if (showprogress)
	{
		m_progressDlg.ShowModeless(CWnd::FromHandle(hWndExplorer));
	}
	SVNStatus s;
	m_nHeadRev = s.GetStatus(path, TRUE);
	m_progressDlg.SetProgress((DWORD)0, (DWORD)m_nHeadRev);
	if (!this->Blame(path, startrev, endrev, strict))
	{
		m_saveFile.Close();
		DeleteFile(m_sSavePath);
		m_sSavePath = _T("");
	} // if (!this->Blame(path, startrev, endrev, strict))
	m_progressDlg.Stop();
	if (m_saveFile.m_hFile != INVALID_HANDLE_VALUE)
		m_saveFile.Close();

	return m_sSavePath;
}
