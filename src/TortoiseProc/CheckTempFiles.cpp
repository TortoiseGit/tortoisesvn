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

BOOL CCheckTempFiles::IsTemp(CString filename)
{
	filename = filename.MakeLower();
	CPath filepath = filename;
	filepath.StripPath();
	//CRegString maskReg = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"), _T(""), 0, HKEY_LOCAL_MACHINE);
	CRegString maskReg = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"));
	CString mask = maskReg;

	if (mask.GetLength() == 0)
		return FALSE;

	//check for every extension
	int curPos= 0;
	CString resToken= mask.Tokenize(_T(";"),curPos);
	while (resToken != _T(""))
	{
		//if (filepath.GetExtension().CompareNoCase(resToken)==0)
		//	return TRUE;
		resToken = resToken.MakeLower();
		if (CStringUtils::WildCardMatch(resToken, filepath))
			return TRUE;
		resToken = mask.Tokenize(_T(";"),curPos);
	};
	return FALSE;
}
int CCheckTempFiles::Check(CString dirName, const BOOL recurse, const BOOL includeDirs)
{
	//CRegString maskReg = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"), _T(""), 0, HKEY_LOCAL_MACHINE);
	CRegString maskReg = CRegString(_T("Software\\TortoiseSVN\\TempFileExtensions"));
	CString mask = maskReg;

	if (mask.GetLength() == 0)
		return IDOK;

	//separate the extensions
	CStringArray maskarray;
	int curPos= 0;
	CString resToken= mask.Tokenize(_T(";"),curPos);
	while (resToken != _T(""))
	{
		resToken = resToken.MakeLower();
		maskarray.Add(resToken);
		resToken = mask.Tokenize(_T(";"),curPos);
	};

	CDirFileList filelist;
	filelist.BuildList(dirName, recurse, includeDirs);

	//now check the filelist against the extensions
	int count = 0;
	for (int i=0; i<filelist.GetCount(); i++)
	{
		CString fpath = filelist.GetAt(i);
		fpath = fpath.MakeLower();
		CPath filepath = fpath;
		filepath.StripPath();
		//check for every extension in the list
		for (int j=0; j<maskarray.GetCount(); j++)
		{
			//if (filepath.GetExtension().CompareNoCase(maskarray.GetAt(j))==0)
			if (CStringUtils::WildCardMatch(maskarray.GetAt(j), filepath))
			{
				count++;
			}
		}
	}

	if (count > 0)
	{
		//found temporary files
		CString temp;
		temp.Format(_T("there are %d files selected which appear to be\neither temporary or compiler generated.\n"),count);
		temp += _T("Do you want to delete them first?\n");
		temp += _T("<ct=0x0000ff><b>Warning!</b></ct> if you click yes then <u>all</u> files with endings\n");
		temp += maskReg;
		temp += _T("\nwill be deleted!");
		int ret;
		if ((ret = CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON3))==IDYES)
		{
			for (int i=0; i<filelist.GetCount(); i++)
			{
				CString fpath = filelist.GetAt(i);
				fpath = fpath.MakeLower();
				CPath filepath = fpath;
				filepath.StripPath();
				for (int j=0; j<maskarray.GetCount(); j++)
				{
					//if (filepath.GetExtension().CompareNoCase(maskarray.GetAt(j))==0)
					if (CStringUtils::WildCardMatch(maskarray.GetAt(j), filepath))
					{
						//try
						//{
							//TODO: replace CFile::Remove with a shell delete (trashbin)
							//CFile::Remove(filepath);
							TCHAR filename[MAX_PATH] = {0};
							_tcscpy(filename, filepath);
							SHFILEOPSTRUCT fileop;
							fileop.hwnd = NULL;
							fileop.wFunc = FO_DELETE;
							fileop.pFrom = filename;
							fileop.pTo = _T("");
							fileop.fFlags = FOF_ALLOWUNDO | FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;
							SHFileOperation(&fileop);
						//} 
						//catch (CFileException* pEx)
						//{
						//	pEx->Delete();
						//}
					}
				}
			} // for (int i=0; i<filelist.GetCount(); i++)
			return IDOK;
		} // if (MessageBox(NULL, temp, "TortoiseSVN", MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON3)==IDYES)
		return ret;
	}
	return IDOK;
}
