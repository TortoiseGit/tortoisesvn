// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

#include "StdAfx.h"
#include "resource.h"
#include "..\TortoiseShell\resource.h"
#include "utils.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "MessageBox.h"
#include "FileDiffDlg.h"
#include "ProgressDlg.h"
#include ".\svndiff.h"

SVNDiff::SVNDiff(SVN * pSVN /* = NULL */, HWND hWnd /* = NULL */, CTSVNPathList * pTempFileList /* = NULL */) :
	m_bDeleteSVN(false),
	m_pSVN(NULL),
	m_hWnd(NULL),
	m_pTempFileList(NULL)
{
	if (pSVN)
		m_pSVN = pSVN;
	else
	{
		m_pSVN = new SVN;
		m_bDeleteSVN = true;
	}
	m_hWnd = hWnd;
	m_pTempFileList = pTempFileList;
}

SVNDiff::~SVNDiff(void)
{
	if (m_bDeleteSVN)
		delete m_pSVN;
}


bool SVNDiff::StartConflictEditor(const CTSVNPath& conflictedFilePath)
{
	CTSVNPath merge = conflictedFilePath;
	CTSVNPath directory = merge.GetDirectory();
	CTSVNPath theirs(directory);
	CTSVNPath mine(directory);
	CTSVNPath base(directory);

	//we have the conflicted file (%merged)
	//now look for the other required files
	SVNStatus stat;
	stat.GetStatus(merge);
	if (stat.status && stat.status->entry)
	{
		if (stat.status->entry->conflict_new)
		{
			theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
		}
		if (stat.status->entry->conflict_old)
		{
			base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
		}
		if (stat.status->entry->conflict_wrk)
		{
			mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
		}
		else
		{
			mine = merge;
		}
	}
	return !!CUtils::StartExtMerge(base,theirs,mine,merge);
}

bool SVNDiff::DiffFileAgainstBase(const CTSVNPath& filePath, CTSVNPath& temporaryFile)
{
	CTSVNPath basePath(SVN::GetPristinePath(filePath));
	// If necessary, convert the line-endings on the file before diffing
	if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
	{
		temporaryFile = CUtils::GetTempFilePath(filePath);
		if (m_pTempFileList)
			m_pTempFileList->AddPath(temporaryFile);
		if (!m_pSVN->Cat(filePath, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
		{
			temporaryFile.Reset();
			return FALSE;
		}
		else
		{
			basePath = temporaryFile;
		}
	}
	CString name = filePath.GetFilename();
	CString n1, n2;
	n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
	n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
	return !!CUtils::StartExtDiff(basePath, filePath, n2, n1, TRUE);
}

bool SVNDiff::UnifiedDiff(CTSVNPath& tempfile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg /* = SVNRev() */)
{
	tempfile = CUtils::GetTempFilePath(CTSVNPath(_T("Test.diff")));
	if (m_pTempFileList)
		m_pTempFileList->AddPath(tempfile);
	bool bIsUrl = !!SVN::PathIsURL(url1.GetSVNPathString());
	
	if ((!url1.IsEquivalentTo(url2))||((rev1.IsWorking() || rev1.IsBase())&&(rev2.IsWorking() || rev2.IsBase())))
	{
		if (!m_pSVN->Diff(url1, rev1, url2, rev2, TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : (bIsUrl ? SVNRev::REV_HEAD : SVNRev::REV_WC)), rev1, rev2, TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
		{
			CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	if (CUtils::CheckForEmptyDiff(tempfile))
	{
		CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	return true;
}

bool SVNDiff::ShowUnifiedDiff(const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg /* = SVNRev() */)
{
	CTSVNPath tempfile;
	if (UnifiedDiff(tempfile, url1, rev1, url2, rev2, peg))
		return !!CUtils::StartUnifiedDiffViewer(tempfile);

	return false;
}

bool SVNDiff::ShowCompare(const CTSVNPath& url1, const SVNRev& rev1, 
						  const CTSVNPath& url2, const SVNRev& rev2, 
						  const SVNRev& peg /* = SVNRev( */,
						  bool ignoreancestry /* = false */, bool nodiffdeleted /* = false */)
{
	CTSVNPath tempfile;
	
	if ((m_pSVN->PathIsURL(url1.GetSVNPathString()))||(!rev1.IsWorking())||(!url1.IsEquivalentTo(url2)))
	{
		// no working copy path!
		
		tempfile = CUtils::GetTempFilePath(url1);		
		// first find out if the url points to a file or dir
		SVNInfo info;
		const SVNInfoData * data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : SVNRev::REV_HEAD), rev1, false);
		if (data == NULL)
		{
			CMessageBox::Show(m_hWnd, info.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
		
		if (data->kind == svn_node_dir)
		{
			if (url1.IsEquivalentTo(url2))
			{
				if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : SVNRev::REV_HEAD), rev1, rev2, TRUE, ignoreancestry, nodiffdeleted, FALSE, _T(""), tempfile))
				{
					CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
			}
			else
			{
				if (!m_pSVN->Diff(url1, rev1, url2, rev2, TRUE, ignoreancestry, nodiffdeleted, FALSE, _T(""), tempfile))
				{
					CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
			}
			if (CUtils::CheckForEmptyDiff(tempfile))
			{
				CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
				return false;
			}
			CFileDiffDlg fdlg;
			if ((!url1.IsEquivalentTo(url2))||(m_pSVN->PathIsURL(url1.GetSVNPathString())))
			{
				// we have to find the common root of both urls, because
				// the unified diff has paths relative to that in it.
				// now find the common root of both urls
				CTSVNPath baseDirectory1 = url1.GetDirectory();
				CTSVNPath baseDirectory2 = url2.GetDirectory();
				while (!baseDirectory1.IsEquivalentTo(baseDirectory2))
				{
					if (baseDirectory1.GetSVNPathString().GetLength() > baseDirectory2.GetSVNPathString().GetLength())
						baseDirectory1 = baseDirectory1.GetDirectory();
					else
						baseDirectory2 = baseDirectory2.GetDirectory();
				};
				// if url1 and url2 are equivalent and point to a dir,
				// then the they already *are* the common root.
				if ((url1.IsEquivalentTo(url2))&&(data->kind == svn_node_dir))
					baseDirectory2 = url1;
				fdlg.SetUnifiedDiff(CTSVNPath(tempfile), baseDirectory2.GetSVNPathString());
			}
			else
				fdlg.SetUnifiedDiff(CTSVNPath(tempfile), CString());

			fdlg.DoModal();
		}
		else
		{
			// diffing two revs of a file, so cat two files
			CTSVNPath tempfile1 = CUtils::GetTempFilePath(url1);
			CTSVNPath tempfile2 = CUtils::GetTempFilePath(url2);

			if (m_pTempFileList)
			{
				m_pTempFileList->AddPath(tempfile1);
				m_pTempFileList->AddPath(tempfile2);
			}

			CProgressDlg progDlg;
			progDlg.SetTitle(IDS_PROGRESSWAIT);
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url1.GetUIPathString());
			progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev1);
			if (!m_pSVN->Cat(url1, peg.IsValid() ? peg : rev1, rev1, tempfile1))
			{
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
			progDlg.SetProgress(1, 2);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)url2.GetUIPathString());
			progDlg.FormatNonPathLine(2, IDS_PROGRESSREVISION, rev2);

			if (!m_pSVN->Cat(url2, peg.IsValid() ? peg : rev2, rev2, tempfile2))
			{
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
			progDlg.SetProgress(2,2);
			progDlg.Stop();

			CString revname1, revname2;
			revname1.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFileOrDirectoryName(), (LONG)rev1);
			revname2.Format(_T("%s Revision %ld"), (LPCTSTR)url2.GetFileOrDirectoryName(), (LONG)rev2);
			return !!CUtils::StartExtDiff(tempfile1, tempfile2, revname1, revname2);
		}
	}
	else
	{
		// compare with working copy
		if (PathIsDirectory(url1.GetWinPath()))
		{
			if (UnifiedDiff(tempfile, url1, rev1, url1, rev2, (peg.IsValid() ? peg : SVNRev::REV_WC)))
			{
				CString sWC, sRev;
				sWC.LoadString(IDS_DIFF_WORKINGCOPY);
				sRev.Format(IDS_DIFF_REVISIONPATCHED, (LONG)rev2);
				return !!CUtils::StartExtPatch(tempfile, url1.GetDirectory(), sWC, sRev, TRUE);
			}
		}
		else
		{
			ASSERT(rev1.IsWorking());
			tempfile = CUtils::GetTempFilePath(url1);
			if (m_pTempFileList)
				m_pTempFileList->AddPath(tempfile);
			if (!m_pSVN->Cat(url1, (peg.IsValid() ? peg : SVNRev::REV_WC), rev2, tempfile))
			{
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			} 
			else
			{
				CString revname, wcname;
				revname.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), (LONG)rev2);
				wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
				return !!CUtils::StartExtDiff(tempfile, url1, revname, wcname);
			}
		}
	}
	return false;
}
