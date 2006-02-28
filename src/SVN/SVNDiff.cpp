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

#include "StdAfx.h"
#include "resource.h"
#include "..\TortoiseShell\resource.h"
#include "utils.h"
#include "TempFile.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "SVNProperties.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "MessageBox.h"
#include "FileDiffDlg.h"
#include "ProgressDlg.h"
#include ".\svndiff.h"

SVNDiff::SVNDiff(SVN * pSVN /* = NULL */, HWND hWnd /* = NULL */, bool bRemoveTempFiles /* = false */) :
	m_bDeleteSVN(false),
	m_pSVN(NULL),
	m_hWnd(NULL),
	m_bRemoveTempFiles(false),
	m_headPeg(SVNRev::REV_HEAD)
{
	if (pSVN)
		m_pSVN = pSVN;
	else
	{
		m_pSVN = new SVN;
		m_bDeleteSVN = true;
	}
	m_hWnd = hWnd;
	m_bRemoveTempFiles = bRemoveTempFiles;
}

SVNDiff::~SVNDiff(void)
{
	if (m_bDeleteSVN)
		delete m_pSVN;
}

bool SVNDiff::DiffWCFile(const CTSVNPath& filePath, 
						svn_wc_status_kind text_status /* = svn_wc_status_none */, 
						svn_wc_status_kind prop_status /* = svn_wc_status_none */, 
						svn_wc_status_kind remotetext_status /* = svn_wc_status_none */, 
						svn_wc_status_kind remoteprop_status /* = svn_wc_status_none */)
{
	CTSVNPath basePath;
	CTSVNPath remotePath;
	
	// first diff the remote properties against the wc props
	// TODO: should we attempt to do a three way diff with the properties too
	// if they're modified locally and remotely?
	if (remoteprop_status > svn_wc_status_normal)
	{
		DiffProps(filePath, SVNRev::REV_HEAD, SVNRev::REV_WC);
	}

	if (text_status > svn_wc_status_normal)
		basePath = SVN::GetPristinePath(filePath);

	if (remotetext_status > svn_wc_status_normal)
	{
		remotePath = CTempFiles::Instance().GetTempFilePath(true, filePath, SVNRev::REV_HEAD);

		CProgressDlg progDlg;
		progDlg.SetTitle(IDS_APPNAME);
		progDlg.SetTime(false);
		m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
		progDlg.ShowModeless(m_hWnd);
		progDlg.FormatPathLine(1, IDS_PROGRESSGETFILE, (LPCTSTR)filePath.GetUIPathString());
		if (!m_pSVN->Cat(filePath, SVNRev(SVNRev::REV_HEAD), SVNRev::REV_HEAD, remotePath))
		{
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
		progDlg.Stop();
		m_pSVN->SetAndClearProgressInfo((HWND)NULL);
		SetFileAttributes(remotePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
	}

	CString name = filePath.GetFileOrDirectoryName();
	CString n1, n2, n3;
	n1.Format(IDS_DIFF_WCNAME, name);
	n2.Format(IDS_DIFF_BASENAME, name);
	n3.Format(IDS_DIFF_REMOTENAME, name);

	if ((text_status <= svn_wc_status_normal)&&(prop_status <= svn_wc_status_normal))
	{
		// Hasn't changed locally - diff remote against WC
		return !!CUtils::StartExtDiff(filePath, remotePath, n1, n3);
	}
	else if (remotePath.IsEmpty())
	{
		return DiffFileAgainstBase(filePath, text_status, prop_status);
	}
	else
	{
		// Three-way diff
		return !!CUtils::StartExtMerge(basePath, remotePath, filePath, CTSVNPath(), n2, n3, n1, CString(), true);
	}
}

bool SVNDiff::StartConflictEditor(const CTSVNPath& conflictedFilePath)
{
	CTSVNPath merge = conflictedFilePath;
	CTSVNPath directory = merge.GetDirectory();
	CTSVNPath theirs(directory);
	CTSVNPath mine(directory);
	CTSVNPath base(directory);
	bool bConflictData = false;

	//we have the conflicted file (%merged)
	//now look for the other required files
	SVNStatus stat;
	stat.GetStatus(merge);
	if ((stat.status == NULL)||(stat.status->entry == NULL))
		return false;

	if (stat.status->entry->conflict_new)
	{
		theirs.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_new));
		bConflictData = true;
	}
	if (stat.status->entry->conflict_old)
	{
		base.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_old));
		bConflictData = true;
	}
	if (stat.status->entry->conflict_wrk)
	{
		mine.AppendPathString(CUnicodeUtils::GetUnicode(stat.status->entry->conflict_wrk));
		bConflictData = true;
	}
	else
	{
		mine = merge;
	}
	if (bConflictData)
		return !!CUtils::StartExtMerge(base,theirs,mine,merge);
	return false;
}

bool SVNDiff::DiffFileAgainstBase(const CTSVNPath& filePath, svn_wc_status_kind text_status /* = svn_wc_status_none */, svn_wc_status_kind prop_status /* = svn_wc_status_none */)
{
	bool retvalue = false;
	if ((text_status == svn_wc_status_none)||(prop_status == svn_wc_status_none))
	{
		SVNStatus stat;
		stat.GetStatus(filePath);
		if (stat.status == NULL)
			return false;
		text_status = stat.status->text_status;
		prop_status = stat.status->prop_status;
	}
	if (prop_status > svn_wc_status_normal)
	{
		DiffProps(filePath, SVNRev::REV_WC, SVNRev::REV_BASE);
	}

	if (text_status >= svn_wc_status_normal)
	{
		CTSVNPath basePath(SVN::GetPristinePath(filePath));
		// If necessary, convert the line-endings on the file before diffing
		if ((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"), TRUE))
		{
			CTSVNPath temporaryFile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, filePath, SVNRev::REV_BASE);
			if (!m_pSVN->Cat(filePath, SVNRev(SVNRev::REV_BASE), SVNRev(SVNRev::REV_BASE), temporaryFile))
			{
				temporaryFile.Reset();
				return FALSE;
			}
			else
			{
				basePath = temporaryFile;
				SetFileAttributes(basePath.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			}
		}
		CString name = filePath.GetFilename();
		CString n1, n2;
		n1.Format(IDS_DIFF_WCNAME, (LPCTSTR)name);
		n2.Format(IDS_DIFF_BASENAME, (LPCTSTR)name);
		retvalue = !!CUtils::StartExtDiff(basePath, filePath, n2, n1, TRUE);
	}
	return retvalue;
}

bool SVNDiff::UnifiedDiff(CTSVNPath& tempfile, const CTSVNPath& url1, const SVNRev& rev1, const CTSVNPath& url2, const SVNRev& rev2, const SVNRev& peg /* = SVNRev() */)
{
	tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, CTSVNPath(_T("Test.diff")));
	bool bIsUrl = !!SVN::PathIsURL(url1.GetSVNPathString());
	
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
	progDlg.SetTime(false);
	m_pSVN->SetAndClearProgressInfo(&progDlg);
	progDlg.ShowModeless(m_hWnd);
	if ((!url1.IsEquivalentTo(url2))||((rev1.IsWorking() || rev1.IsBase())&&(rev2.IsWorking() || rev2.IsBase())))
	{
		if (!m_pSVN->Diff(url1, rev1, url2, rev2, TRUE, FALSE, FALSE, FALSE, _T(""), false, tempfile))
		{
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	else
	{
		if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : (bIsUrl ? m_headPeg : SVNRev::REV_WC)), rev1, rev2, TRUE, FALSE, FALSE, FALSE, _T(""), tempfile))
		{
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
			CMessageBox::Show(this->m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return false;
		}
	}
	if (CUtils::CheckForEmptyDiff(tempfile))
	{
		progDlg.Stop();
		m_pSVN->SetAndClearProgressInfo((HWND)NULL);
		CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
		return false;
	}
	progDlg.Stop();
	m_pSVN->SetAndClearProgressInfo((HWND)NULL);
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
						  SVNRev peg /* = SVNRev( */,
						  bool ignoreancestry /* = false */, bool nodiffdeleted /* = false */)
{
	CTSVNPath tempfile;
	
	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_APPNAME);
	progDlg.SetTime(false);
	m_pSVN->SetAndClearProgressInfo(&progDlg);

	if ((m_pSVN->PathIsURL(url1.GetSVNPathString()))||(!rev1.IsWorking())||(!url1.IsEquivalentTo(url2)))
	{
		// no working copy path!
		progDlg.ShowModeless(m_hWnd);

		tempfile = CTempFiles::Instance().GetTempFilePath(true, url1);		
		// first find out if the url points to a file or dir
		progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_INFO)));
		SVNInfo info;
		const SVNInfoData * data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : m_headPeg), rev1, false);
		if (data == NULL)
		{
			data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev1), rev1, false);
			if (data == NULL)
			{
				data = info.GetFirstFileInfo(url1, (peg.IsValid() ? peg : rev2), rev1, false);
				if (data == NULL)
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, info.GetLastErrorMsg(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
				else
					peg = peg.IsValid() ? peg : rev2;
			}
			else
				peg = peg.IsValid() ? peg : rev1;
		}
		else
			peg = peg.IsValid() ? peg : m_headPeg;
		
		if (data->kind == svn_node_dir)
		{
			progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PROGRESS_UNIFIEDDIFF)));
			if (url1.IsEquivalentTo(url2))
			{
				if (!m_pSVN->PegDiff(url1, (peg.IsValid() ? peg : m_headPeg), rev1, rev2, TRUE, ignoreancestry, nodiffdeleted, FALSE, _T(""), tempfile))
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
			}
			else
			{
				if (!m_pSVN->Diff(url1, rev1, url2, rev2, TRUE, ignoreancestry, nodiffdeleted, FALSE, _T(""), false, tempfile))
				{
					progDlg.Stop();
					m_pSVN->SetAndClearProgressInfo((HWND)NULL);
					CMessageBox::Show(m_hWnd, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return false;
				}
			}
			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);
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
			CTSVNPath tempfile1 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev1);
			CTSVNPath tempfile2 = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url2, rev2);

			m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIPathString(), (LONG)rev1);
			if (!m_pSVN->Cat(url1, peg.IsValid() ? peg : rev1, rev1, tempfile1))
			{
				progDlg.Stop();
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
			SetFileAttributes(tempfile1.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url2.GetUIPathString(), (LONG)rev2);
			if (!m_pSVN->Cat(url2, peg.IsValid() ? peg : rev2, rev2, tempfile2))
			{
				progDlg.Stop();
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			}
			SetFileAttributes(tempfile2.GetWinPath(), FILE_ATTRIBUTE_READONLY);

			progDlg.Stop();
			m_pSVN->SetAndClearProgressInfo((HWND)NULL);

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

			m_pSVN->SetAndClearProgressInfo(&progDlg, true);	// activate progress bar
			progDlg.ShowModeless(m_hWnd);
			progDlg.FormatPathLine(1, IDS_PROGRESSGETFILEREVISION, (LPCTSTR)url1.GetUIPathString(), (LONG)rev2);

			tempfile = CTempFiles::Instance().GetTempFilePath(m_bRemoveTempFiles, url1, rev2);
			if (!m_pSVN->Cat(url1, (peg.IsValid() ? peg : SVNRev::REV_WC), rev2, tempfile))
			{
				progDlg.Stop();
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				CMessageBox::Show(NULL, m_pSVN->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				return false;
			} 
			else
			{
				progDlg.Stop();
				m_pSVN->SetAndClearProgressInfo((HWND)NULL);
				SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
				CString revname, wcname;
				revname.Format(_T("%s Revision %ld"), (LPCTSTR)url1.GetFilename(), (LONG)rev2);
				wcname.Format(IDS_DIFF_WCNAME, (LPCTSTR)url1.GetFilename());
				return !!CUtils::StartExtDiff(tempfile, url1, revname, wcname);
			}
		}
	}
	return false;
}

bool SVNDiff::DiffProps(const CTSVNPath& filePath, SVNRev rev1, SVNRev rev2)
{
	bool retvalue = false;
	// diff the properties
	SVNProperties propswc(filePath, rev1);
	SVNProperties propsbase(filePath, rev2);

	for (int wcindex = 0; wcindex < propswc.GetCount(); ++wcindex)
	{
		stdstring wcname = propswc.GetItemName(wcindex);
		stdstring wcvalue = CUnicodeUtils::StdGetUnicode((char *)propswc.GetItemValue(wcindex).c_str());
		stdstring basevalue;
		bool bDiffRequired = true;
		for (int baseindex = 0; baseindex < propsbase.GetCount(); ++baseindex)
		{
			if (propsbase.GetItemName(baseindex).compare(wcname)==0)
			{
				basevalue = CUnicodeUtils::StdGetUnicode((char *)propsbase.GetItemValue(baseindex).c_str());
				if (basevalue.compare(wcvalue)==0)
				{
					// name and value are identical
					bDiffRequired = false;
					break;
				}
			}
		}
		if (bDiffRequired)
		{
			// write both property values to temporary files
			CTSVNPath wcpropfile = CTempFiles::Instance().GetTempFilePath(true);
			CTSVNPath basepropfile = CTempFiles::Instance().GetTempFilePath(true);
			FILE * pFile;
			_tfopen_s(&pFile, wcpropfile.GetWinPath(), _T("wb"));
			if (pFile)
			{
				fputs(CUnicodeUtils::StdGetUTF8(wcvalue).c_str(), pFile);
				fclose(pFile);
				FILE * pFile;
				_tfopen_s(&pFile, basepropfile.GetWinPath(), _T("wb"));
				if (pFile)
				{
					fputs(CUnicodeUtils::StdGetUTF8(basevalue).c_str(), pFile);
					fclose(pFile);
				}
				else
					return false;
			}
			else
				return false;
			SetFileAttributes(wcpropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			SetFileAttributes(basepropfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
			CString n1, n2;
			if (rev1.IsWorking())
				n1.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (rev1.IsBase())
				n1.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (rev1.IsHead())
				n1.Format(IDS_DIFF_REMOTENAME, wcname.c_str());
			if (rev2.IsWorking())
				n2.Format(IDS_DIFF_WCNAME, wcname.c_str());
			if (rev2.IsBase())
				n2.Format(IDS_DIFF_BASENAME, wcname.c_str());
			if (rev2.IsHead())
				n2.Format(IDS_DIFF_REMOTENAME, wcname.c_str());
			retvalue = !!CUtils::StartExtDiff(basepropfile, wcpropfile, n2, n1, TRUE);
		}
	}
	return retvalue;
}