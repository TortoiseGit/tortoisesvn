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
#include "StdAfx.h"
#include "resource.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "SVNProperties.h"
#include "StringUtils.h"
#include "MessageBox.h"
#include "Registry.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"


CAppUtils::CAppUtils(void)
{
}

CAppUtils::~CAppUtils(void)
{
}

BOOL CAppUtils::StartExtMerge(const CTSVNPath& basefile, const CTSVNPath& theirfile, const CTSVNPath& yourfile, const CTSVNPath& mergedfile,
						   		const CString& basename, const CString& theirname, const CString& yourname, const CString& mergedname, bool bReadOnly)
{

	CRegString regCom = CRegString(_T("Software\\TortoiseSVN\\Merge"));
	CString ext = mergedfile.GetFileExtension();
	CString mimetype;
	CString com = regCom;
	bool bInternal = false;
	SVNProperties props(yourfile);
	for (int i=0; i<props.GetCount(); ++i)
	{
		if (props.GetItemName(i).compare(_T("svn:mime-type"))==0)
		{
			mimetype = props.GetItemValue(i).c_str();
		}
	}
	if (mimetype.IsEmpty())
	{
		SVNProperties props2(theirfile);
		for (int i=0; i<props2.GetCount(); ++i)
		{
			if (props2.GetItemName(i).compare(_T("svn:mime-type"))==0)
			{
				mimetype = props2.GetItemValue(i).c_str();
			}
		}
	}
	if (mimetype.IsEmpty())
	{
		SVNProperties props3(basefile);
		for (int i=0; i<props.GetCount(); ++i)
		{
			if (props3.GetItemName(i).compare(_T("svn:mime-type"))==0)
			{
				mimetype = props3.GetItemValue(i).c_str();
			}
		}
	}
	if (mimetype != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + mimetype);
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}
	else if (ext != "")
	{
		// is there an extension specific merge tool?
		CRegString mergetool(_T("Software\\TortoiseSVN\\MergeTools\\") + ext.MakeLower());
		if (CString(mergetool) != "")
		{
			com = mergetool;
		}
	}
	
	if (com.IsEmpty()||(com.Left(1).Compare(_T("#"))==0))
	{
		// use TortoiseMerge
		bInternal = true;
		CRegString tortoiseMergePath(_T("Software\\TortoiseSVN\\TMergePath"), _T(""), false, HKEY_LOCAL_MACHINE);
		com = tortoiseMergePath;
		if (com.IsEmpty())
		{
			com = CPathUtils::GetAppDirectory();
			com += _T("TortoiseMerge.exe");
		}
		com = _T("\"") + com + _T("\"");
		com = com + _T(" /base:%base /theirs:%theirs /mine:%mine /merged:%merged");
		com = com + _T(" /basename:%bname /theirsname:%tname /minename:%yname /mergedname:%mname");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((com.Find(_T("%base"))<0)&&(com.Find(_T("%theirs"))<0)&&(com.Find(_T("%mine"))<0))
	{
		com += _T(" \"")+basefile.GetWinPathString()+_T("\"");
		com += _T(" \"")+theirfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+yourfile.GetWinPathString()+_T("\"");
		com += _T(" \"")+mergedfile.GetWinPathString()+_T("\"");
	}
	if (basefile.IsEmpty())
	{
		com.Replace(_T("/base:%base"), _T(""));
		com.Replace(_T("%base"), _T(""));		
	}
	else
		com.Replace(_T("%base"), _T("\"") + basefile.GetWinPathString() + _T("\""));
	if (theirfile.IsEmpty())
	{
		com.Replace(_T("/theirs:%theirs"), _T(""));
		com.Replace(_T("%theirs"), _T(""));
	}
	else
		com.Replace(_T("%theirs"), _T("\"") + theirfile.GetWinPathString() + _T("\""));
	if (yourfile.IsEmpty())
	{
		com.Replace(_T("/mine:%mine"), _T(""));
		com.Replace(_T("%mine"), _T(""));
	}
	else
		com.Replace(_T("%mine"), _T("\"") + yourfile.GetWinPathString() + _T("\""));
	if (mergedfile.IsEmpty())
	{
		com.Replace(_T("/merged:%merged"), _T(""));
		com.Replace(_T("%merged"), _T(""));
	}
	else
		com.Replace(_T("%merged"), _T("\"") + mergedfile.GetWinPathString() + _T("\""));
	if (basename.IsEmpty())
	{
		if (basefile.IsEmpty())
		{
			com.Replace(_T("/basename:%bname"), _T(""));
			com.Replace(_T("%bname"), _T(""));
		}
		else
		{
			com.Replace(_T("%bname"), _T("\"") + basefile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%bname"), _T("\"") + basename + _T("\""));
	if (theirname.IsEmpty())
	{
		if (theirfile.IsEmpty())
		{
			com.Replace(_T("/theirsname:%tname"), _T(""));
			com.Replace(_T("%tname"), _T(""));
		}
		else
		{
			com.Replace(_T("%tname"), _T("\"") + theirfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%tname"), _T("\"") + theirname + _T("\""));
	if (yourname.IsEmpty())
	{
		if (yourfile.IsEmpty())
		{
			com.Replace(_T("/minename:%yname"), _T(""));
			com.Replace(_T("%yname"), _T(""));
		}
		else
		{
			com.Replace(_T("%yname"), _T("\"") + yourfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%yname"), _T("\"") + yourname + _T("\""));
	if (mergedname.IsEmpty())
	{
		if (mergedfile.IsEmpty())
		{
			com.Replace(_T("/mergedname:%mname"), _T(""));
			com.Replace(_T("%mname"), _T(""));
		}
		else
		{
			com.Replace(_T("%mname"), _T("\"") + mergedfile.GetUIFileOrDirectoryName() + _T("\""));
		}
	}
	else
		com.Replace(_T("%mname"), _T("\"") + mergedname + _T("\""));

	if ((bReadOnly)&&(bInternal))
		com += _T(" /readonly");

	if(!LaunchApplication(com, IDS_ERR_EXTMERGESTART, false))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CAppUtils::StartExtPatch(const CTSVNPath& patchfile, const CTSVNPath& dir, const CString& sOriginalDescription, const CString& sPatchedDescription, BOOL bReversed, BOOL bWait)
{
	CString viewer;
	// use TortoiseMerge
	viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseMerge.exe");

	viewer = _T("\"") + viewer + _T("\"");
	viewer = viewer + _T(" /diff:\"") + patchfile.GetWinPathString() + _T("\"");
	viewer = viewer + _T(" /patchpath:\"") + dir.GetWinPathString() + _T("\"");
	if (bReversed)
		viewer += _T(" /reversedpatch");
	if (!sOriginalDescription.IsEmpty())
		viewer = viewer + _T(" /patchoriginal:\"") + sOriginalDescription + _T("\"");
	if (!sPatchedDescription.IsEmpty())
		viewer = viewer + _T(" /patchpatched:\"") + sPatchedDescription + _T("\"");
	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartExtDiff(const CTSVNPath& file1, const CTSVNPath& file2, const CString& sName1, const CString& sName2, BOOL bWait, BOOL bBlame)
{
	CString viewer;
	CString mimetype;
	CRegString diffexe(_T("Software\\TortoiseSVN\\Diff"));
	CRegDWORD blamediff(_T("Software\\TortoiseSVN\\DiffBlamesWithTortoiseMerge"), FALSE);
	bool bUseTMerge = !!(DWORD)blamediff;
	viewer = diffexe;

	SVNProperties props(file1);
	for (int i=0; i<props.GetCount(); ++i)
	{
		if (props.GetItemName(i).compare(_T("svn:mime-type"))==0)
		{
			mimetype = props.GetItemValue(i).c_str();
		}
	}
	if (mimetype.IsEmpty())
	{
		SVNProperties props2(file2);
		for (int i=0; i<props2.GetCount(); ++i)
		{
			if (props2.GetItemName(i).compare(_T("svn:mime-type"))==0)
			{
				mimetype = props2.GetItemValue(i).c_str();
			}
		}
	}
	if (mimetype != "")
	{
		// is there an extension specific merge tool?
		CRegString difftool(_T("Software\\TortoiseSVN\\DiffTools\\") + mimetype);
		if (CString(difftool) != "")
		{
			viewer = difftool;
		}
	}
	if (!file2.GetFileExtension().IsEmpty())
	{
		// is there an extension specific diff tool?
		CRegString difftool(_T("Software\\TortoiseSVN\\DiffTools\\") + file2.GetFileExtension().MakeLower());
		if (CString(difftool) != "")
		{
			viewer = difftool;
		}
		else
		{
			// check if we maybe should use TortoiseIDiff
			CString sExtension = file2.GetFileExtension();
			if ((sExtension.CompareNoCase(_T(".jpg"))==0)||
				(sExtension.CompareNoCase(_T(".jpeg"))==0)||
				(sExtension.CompareNoCase(_T(".bmp"))==0)||
				(sExtension.CompareNoCase(_T(".gif"))==0)||
				(sExtension.CompareNoCase(_T(".png"))==0)||
				(sExtension.CompareNoCase(_T(".ico"))==0)||
				(sExtension.CompareNoCase(_T(".dib"))==0)||
				(sExtension.CompareNoCase(_T(".emf"))==0))
			{
				viewer = CPathUtils::GetAppDirectory();
				viewer += _T("TortoiseIDiff.exe");
				viewer = _T("\"") + viewer + _T("\"");
				viewer = viewer + _T(" /left:%base /right:%mine /lefttitle:%bname /righttitle:%yname");
			}
		}
	}
	if (bUseTMerge||viewer.IsEmpty()||(viewer.Left(1).Compare(_T("#"))==0))
	{
		//no registry entry (or commented out) for a diff program
		//use TortoiseMerge
		viewer = CPathUtils::GetAppDirectory();
		viewer += _T("TortoiseMerge.exe");
		viewer = _T("\"") + viewer + _T("\"");
		viewer = viewer + _T(" /base:%base /mine:%mine /basename:%bname /minename:%yname");
		if (bBlame)
			viewer += _T(" /blame");
	}
	// check if the params are set. If not, just add the files to the command line
	if ((viewer.Find(_T("%base"))<0)&&(viewer.Find(_T("%mine"))<0))
	{
		viewer += _T(" \"")+file1.GetWinPathString()+_T("\"");
		viewer += _T(" \"")+file2.GetWinPathString()+_T("\"");
	}
	if (viewer.Find(_T("%base")) >= 0)
	{
		viewer.Replace(_T("%base"),  _T("\"")+file1.GetWinPathString()+_T("\""));
	}
	if (viewer.Find(_T("%mine")) >= 0)
	{
		viewer.Replace(_T("%mine"),  _T("\"")+file2.GetWinPathString()+_T("\""));
	}

	if (sName1.IsEmpty())
		viewer.Replace(_T("%bname"), _T("\"") + file1.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%bname"), _T("\"") + sName1 + _T("\""));

	if (sName2.IsEmpty())
		viewer.Replace(_T("%yname"), _T("\"") + file2.GetUIFileOrDirectoryName() + _T("\""));
	else
		viewer.Replace(_T("%yname"), _T("\"") + sName2 + _T("\""));

	if(!LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartUnifiedDiffViewer(const CTSVNPath& patchfile, BOOL bWait)
{
	CString viewer;
	CRegString v = CRegString(_T("Software\\TortoiseSVN\\DiffViewer"));
	viewer = v;
	if (viewer.IsEmpty() || (viewer.Left(1).Compare(_T("#"))==0))
	{
		//first try the default app which is associated with diff files
		CRegString diff = CRegString(_T(".diff\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
		viewer = diff;
		viewer = viewer + _T("\\Shell\\Open\\Command\\");
		CRegString diffexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
		viewer = diffexe;
		if (viewer.IsEmpty())
		{
			CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = txt;
			viewer = viewer + _T("\\Shell\\Open\\Command\\");
			CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
			viewer = txtexe;
		}
		DWORD len = ExpandEnvironmentStrings(viewer, NULL, 0);
		TCHAR * buf = new TCHAR[len+1];
		ExpandEnvironmentStrings(viewer, buf, len);
		viewer = buf;
		delete buf;
	}
	if (viewer.Find(_T("%1"))>=0)
	{
		if (viewer.Find(_T("\"%1\"")) >= 0)
			viewer.Replace(_T("%1"), patchfile.GetWinPathString());
		else
			viewer.Replace(_T("%1"), _T("\"") + patchfile.GetWinPathString() + _T("\""));
	}
	else
		viewer += _T(" \"") + patchfile.GetWinPathString() + _T("\"");

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, !!bWait))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::StartTextViewer(CString file)
{
	CString viewer;
	CRegString txt = CRegString(_T(".txt\\"), _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txt;
	viewer = viewer + _T("\\Shell\\Open\\Command\\");
	CRegString txtexe = CRegString(viewer, _T(""), FALSE, HKEY_CLASSES_ROOT);
	viewer = txtexe;

	DWORD len = ExpandEnvironmentStrings(viewer, NULL, 0);
	TCHAR * buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(viewer, buf, len);
	viewer = buf;
	delete buf;
	len = ExpandEnvironmentStrings(file, NULL, 0);
	buf = new TCHAR[len+1];
	ExpandEnvironmentStrings(file, buf, len);
	file = buf;
	delete buf;
	file = _T("\"")+file+_T("\"");
	if (viewer.IsEmpty())
	{
		OPENFILENAME ofn;		// common dialog box structure
		TCHAR szFile[MAX_PATH];  // buffer for file name. Explorer can't handle paths longer than MAX_PATH.
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		CString sFilter;
		sFilter.LoadString(IDS_PROGRAMSFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		CString temp;
		temp.LoadString(IDS_UTILS_SELECTTEXTVIEWER);
		CStringUtils::RemoveAccelerators(temp);
		ofn.lpstrTitle = temp;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		// Display the Open dialog box. 

		if (GetOpenFileName(&ofn)==TRUE)
		{
			delete [] pszFilters;
			viewer = CString(ofn.lpstrFile);
		} // if (GetOpenFileName(&ofn)==TRUE)
		else
		{
			delete [] pszFilters;
			return FALSE;
		}
	}
	if (viewer.Find(_T("\"%1\"")) >= 0)
	{
		viewer.Replace(_T("\"%1\""), file);
	}
	else if (viewer.Find(_T("%1")) >= 0)
	{
		viewer.Replace(_T("%1"),  file);
	}
	else
	{
		viewer += _T(" ");
		viewer += file;
	}

	if(!LaunchApplication(viewer, IDS_ERR_DIFFVIEWSTART, false))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CAppUtils::CheckForEmptyDiff(const CTSVNPath& sDiffPath)
{
	DWORD length = 0;
	HANDLE hFile = ::CreateFile(sDiffPath.GetWinPath(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return TRUE;
	length = ::GetFileSize(hFile, NULL);
	::CloseHandle(hFile);
	if (length < 4)
		return TRUE;
	return FALSE;

}

void CAppUtils::CreateFontForLogs(CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(NULL);
	logFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(hScreenDC, LOGPIXELSY), 72);
	::ReleaseDC(NULL, hScreenDC);
	logFont.lfWidth          = 0;
	logFont.lfEscapement     = 0;
	logFont.lfOrientation    = 0;
	logFont.lfWeight         = FW_NORMAL;
	logFont.lfItalic         = 0;
	logFont.lfUnderline      = 0;
	logFont.lfStrikeOut      = 0;
	logFont.lfCharSet        = DEFAULT_CHARSET;
	logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality        = DRAFT_QUALITY;
	logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy_s(logFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

bool CAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	CString cleanCommandLine(sCommandLine);

	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
	{
		if(idErrMessageFormat != 0)
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			CString temp;
			temp.Format(idErrMessageFormat, lpMsgBuf);
			CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
			LocalFree( lpMsgBuf );
		}
		return false;
	}

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

/**
* Launch the external blame viewer
*/
bool CAppUtils::LaunchTortoiseBlame(const CString& sBlameFile, const CString& sLogFile, const CString& sOriginalFile, const CString& sParams)
{
	CString viewer = CPathUtils::GetAppDirectory();
	viewer += _T("TortoiseBlame.exe");
	viewer += _T(" \"") + sBlameFile + _T("\"");
	viewer += _T(" \"") + sLogFile + _T("\"");
	viewer += _T(" \"") + sOriginalFile + _T("\"");
	viewer += _T(" ")+sParams;
	
	return LaunchApplication(viewer, IDS_ERR_EXTDIFFSTART, false);
}

void CAppUtils::ResizeAllListCtrlCols(CListCtrl * pListCtrl)
{
	int maxcol = ((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = pListCtrl->GetItemCount();
	TCHAR textbuf[MAX_PATH];
	for (int col = 0; col <= maxcol; col++)
	{
		HDITEM hdi = {0};
		hdi.mask = HDI_TEXT;
		hdi.pszText = textbuf;
		hdi.cchTextMax = sizeof(textbuf);
		((CHeaderCtrl*)(pListCtrl->GetDlgItem(0)))->GetItem(col, &hdi);
		int cx = pListCtrl->GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
		for (int index = 0; index<nItemCount; ++index)
		{
			// get the width of the string and add 12 pixels for the column separator and margins
			int linewidth = pListCtrl->GetStringWidth(pListCtrl->GetItemText(index, col)) + 12;
			if (index == 0)
			{
				// add the image size
				CImageList * pImgList = pListCtrl->GetImageList(LVSIL_SMALL);
				if ((pImgList)&&(pImgList->GetImageCount()))
				{
					IMAGEINFO imginfo;
					pImgList->GetImageInfo(0, &imginfo);
					linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
					linewidth += 3;	// 3 pixels between icon and text
				}
			}
			if (cx < linewidth)
				cx = linewidth;
		}
		pListCtrl->SetColumnWidth(col, cx);
	}
}

bool CAppUtils::FormatTextInRichEditControl(CWnd * pWnd)
{
	CString sText;
	if (pWnd == NULL)
		return false;
	bool bStyled = false;
	pWnd->GetWindowText(sText);
	// the rich edit control doesn't count the CR char!
	// to be exact: CRLF is treated as one char.
	sText.Replace(_T("\r"), _T(""));

	// style each line separately
	int offset = 0;
	int nNewlinePos;
	do 
	{
		nNewlinePos = sText.Find('\n', offset);
		CString sLine = sText.Mid(offset);
		if (nNewlinePos>=0)
			sLine = sLine.Left(nNewlinePos-offset);
		int start = 0;
		int end = 0;
		while (FindStyleChars(sLine, '*', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			ZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_BOLD;
			format.dwEffects = CFE_BOLD;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '^', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			ZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_ITALIC;
			format.dwEffects = CFE_ITALIC;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(sLine, '_', start, end))
		{
			CHARRANGE range = {(LONG)start+offset, (LONG)end+offset};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			CHARFORMAT2 format;
			ZeroMemory(&format, sizeof(CHARFORMAT2));
			format.cbSize = sizeof(CHARFORMAT2);
			format.dwMask = CFM_UNDERLINE;
			format.dwEffects = CFE_UNDERLINE;
			pWnd->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
			bStyled = true;
			start = end;
		}
		offset = nNewlinePos+1;
	} while(nNewlinePos>=0);
	return bStyled;	
}

bool CAppUtils::FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end)
{
	int i=start;
	bool bFoundMarker = false;
	// find a starting marker
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if (((i+1)<sText.GetLength())&&(IsCharAlphaNumeric(sText[i+1])) &&
				(((i>0)&&(!IsCharAlphaNumeric(sText[i-1])))||(i==0)))
			{
				start = i+1;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	if (!bFoundMarker)
		return false;
	// find ending marker
	bFoundMarker = false;
	while (sText[i] != 0)
	{
		if (sText[i] == stylechar)
		{
			if ((IsCharAlphaNumeric(sText[i-1])) &&
				((((i+1)<sText.GetLength())&&(!IsCharAlphaNumeric(sText[i+1])))||(i+1)==sText.GetLength()))
			{
				end = i;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		i++;
	}
	return bFoundMarker;
}

bool CAppUtils::BrowseRepository(CHistoryCombo& combo, CWnd * pParent, bool bFile /* = true */ )
{
	CString strUrl;
	combo.GetWindowText(strUrl);
	if (strUrl.Left(7) == _T("file://"))
	{
		CString strFile(strUrl);
		SVN::UrlToPath(strFile);

		SVN svn;
		if (svn.IsRepository(strFile))
		{
			// browse repository - show repository browser
			CRepositoryBrowser browser(strUrl, pParent, bFile);
			if (browser.DoModal() == IDOK)
			{
				combo.SetCurSel(-1);
				combo.SetWindowText(browser.GetPath());
				return true;
			}
		}
		else
		{
			// browse local directories
			CBrowseFolder folderBrowser;
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
			{
				SVN::PathToUrl(strUrl);

				combo.SetCurSel(-1);
				combo.SetWindowText(strUrl);
				return true;
			}
		}
	} // if (strUrl.Left(7) == _T("file://")) 
	else if ((strUrl.Left(7) == _T("http://")
		||(strUrl.Left(8) == _T("https://"))
		||(strUrl.Left(6) == _T("svn://"))
		||(strUrl.Left(4) == _T("svn+"))) && strUrl.GetLength() > 6)
	{
		// browse repository - show repository browser
		CRepositoryBrowser browser(strUrl, pParent, bFile);
		if (browser.DoModal() == IDOK)
		{
			combo.SetCurSel(-1);
			combo.SetWindowText(browser.GetPath());
			return true;
		}
	}
	else
	{
		// browse local directories
		CBrowseFolder folderBrowser;
		folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
		if (folderBrowser.Show(pParent->GetSafeHwnd(), strUrl) == CBrowseFolder::OK)
		{
			SVN::PathToUrl(strUrl);

			combo.SetCurSel(-1);
			combo.SetWindowText(strUrl);
			return true;
		}
	}
	return false;
}