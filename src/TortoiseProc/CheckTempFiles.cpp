// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

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
#include "checktempfiles.h"
#include "registry.h"
#include "DirFileList.h"
#include "messagebox.h"
#include <atlpath.h>
#include "StringUtils.h"

CCheckTempFiles::CCheckTempFiles(void)
{
}

CCheckTempFiles::~CCheckTempFiles(void)
{
}

BOOL CCheckTempFiles::FolderMatch(CString folder, CString path)
{
	path.Replace('/', '\\');
	if (!PathIsDirectory(path))
	{
		if (path.ReverseFind('\\')<0)
			path.Empty();
		else
			path = path.Left(path.ReverseFind('\\'));
	}
	folder = folder.MakeLower();
	int curPos = 0;
	CString resToken = path.Tokenize(_T("\\"), curPos);
	while (resToken != _T(""))
	{
		resToken = resToken.MakeLower();
		if (resToken.CompareNoCase(folder)==0)
			return TRUE;
		resToken = path.Tokenize(_T("\\"), curPos);
	} // while (resToken != _T(""))
	return FALSE;
}

BOOL CCheckTempFiles::IsTemp(CString filename)
{
	filename = filename.MakeLower();
	filename.Replace('/', '\\');
	CPath filepath = filename;
	filepath.StripPath();
	CRegString maskReg = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"));
	CString mask = maskReg;

	if (mask.GetLength() == 0)
		return FALSE;

	//check for every extension
	int curPos= 0;
	CString resToken= mask.Tokenize(_T(";"),curPos);
	while (resToken != _T(""))
	{
		resToken = resToken.MakeLower();
		BOOL plus = FALSE;
		BOOL folder = FALSE;
		if (resToken.Left(1).Compare(_T("+"))==0)
		{
			resToken = resToken.Right(resToken.GetLength() - 1);
			plus = TRUE;
		} // if (resToken.Left(1).Compare(_T("+"))==0)
		if (resToken.Right(1).Compare(_T("\\"))==0)
		{
			resToken = resToken.Left(resToken.GetLength() - 1);
			folder = TRUE;
		}
		if ((!folder)&&(CStringUtils::WildCardMatch(resToken, filepath)))
		{
			if (plus)
				return FALSE;
			else
				return TRUE;
		} // if ((!folder)&&(CStringUtils::WildCardMatch(resToken, filepath)))
		if ((folder)&&(FolderMatch(resToken, filename)))
		{
			if (plus)
				return FALSE;
			else
				return TRUE;
		}
		resToken = mask.Tokenize(_T(";"),curPos);
	};
	return FALSE;
}

int CCheckTempFiles::Check(CString dirName, const BOOL recurse, const BOOL includeDirs)
{
	CDirFileList filelist;
	filelist.BuildList(dirName, recurse, includeDirs);

	//now check the filelist against the extensions
	int count = 0;
	for (int i=0; i<filelist.GetCount(); i++)
	{
		if (IsTemp(filelist.GetAt(i)))
			count++;
	}

	if (count > 0)
	{
		//found temporary files
		CString temp;
		temp.Format(_T("there are %d files selected which appear to be\neither temporary or compiler generated.\n"),count);
		temp += _T("Do you want to delete them first?\n");
		temp += _T("<ct=0x0000ff><b>Warning!</b></ct> if you click yes then <u>all</u> files you marked as temporary\nin the settings will be deleted!");
		int ret;
		if ((ret = CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON3))==IDYES)
		{
			for (int i=0; i<filelist.GetCount(); i++)
			{
				if (IsTemp(filelist.GetAt(i)))
				{
					TCHAR filename[MAX_PATH] = {0};
					_tcscpy(filename, filelist.GetAt(i));
					SHFILEOPSTRUCT fileop;
					fileop.hwnd = NULL;
					fileop.wFunc = FO_DELETE;
					fileop.pFrom = filename;
					fileop.pTo = _T("");
					fileop.fFlags = FOF_ALLOWUNDO | FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;
					SHFileOperation(&fileop);
				}
			} // for (int i=0; i<filelist.GetCount(); i++)
			return IDOK;
		} // if (MessageBox(NULL, temp, "TortoiseSVN", MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON3)==IDYES)
		return ret;
	}
	return IDOK;
}
