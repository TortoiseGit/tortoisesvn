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
#include "Utils.h"
#include ".\resmodule.h"

#define MYERROR	{CUtils::Error(); return FALSE;}

CResModule::CResModule(void)
{
	m_hResDll = NULL;
	m_bQuiet = FALSE;
	m_wTargetLang = 0;
}

CResModule::~CResModule(void)
{
}

BOOL CResModule::ExtractResources(std::vector<std::wstring> filelist, LPCTSTR lpszPOFilePath, BOOL bNoUpdate)
{
	BOOL bRet = TRUE;
	for (std::vector<std::wstring>::iterator I = filelist.begin(); I != filelist.end(); ++I)
	{
		m_hResDll = LoadLibrary(I->c_str());
		if (m_hResDll == NULL)
			MYERROR;

		size_t nEntries = m_StringEntries.size();
		// fill in the std::map with all translatable entries

		if (!m_bQuiet)
			_tcprintf(_T("Extracting StringTable..."));
		EnumResourceNames(m_hResDll, RT_STRING,  EnumResNameCallback, (long)this);
		if (!m_bQuiet)
			_tcprintf(_T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_tcprintf(_T("Extracting Dialogs......."));
		EnumResourceNames(m_hResDll, RT_DIALOG,  EnumResNameCallback, (long)this);
		if (!m_bQuiet)
			_tcprintf(_T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_tcprintf(_T("Extracting Menus........."));
		EnumResourceNames(m_hResDll, RT_MENU,    EnumResNameCallback, (long)this);
		if (!m_bQuiet)
			_tcprintf(_T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		// parse a probably existing file and update the translations which are
		// already done
		m_StringEntries.ParseFile(lpszPOFilePath, !bNoUpdate);
		
		FreeLibrary(m_hResDll);
		continue;
	}
	
	// at last, save the new file
	if (bRet)
		return m_StringEntries.SaveFile(lpszPOFilePath);
	return FALSE;
}

BOOL CResModule::ExtractResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszPoFilePath, BOOL bNoUpdate)
{
	m_hResDll = LoadLibrary(lpszSrcLangDllPath);
	if (m_hResDll == NULL)
		MYERROR;
	
	size_t nEntries = 0;
	// fill in the std::map with all translatable entries

	if (!m_bQuiet)
		_tcprintf(_T("Extracting StringTable..."));
	EnumResourceNames(m_hResDll, RT_STRING,  EnumResNameCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d Strings\n"), m_StringEntries.size());
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_tcprintf(_T("Extracting Dialogs......."));
	EnumResourceNames(m_hResDll, RT_DIALOG,  EnumResNameCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d Strings\n"), m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_tcprintf(_T("Extracting Menus........."));
	EnumResourceNames(m_hResDll, RT_MENU,    EnumResNameCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d Strings\n"), m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	// parse a probably existing file and update the translations which are
	// already done
	m_StringEntries.ParseFile(lpszPoFilePath, !bNoUpdate);

	// at last, save the new file
	if (!m_StringEntries.SaveFile(lpszPoFilePath))
		goto DONE_ERROR;

	FreeLibrary(m_hResDll);
	return TRUE;

DONE_ERROR:
	if (m_hResDll)
		FreeLibrary(m_hResDll);
	return FALSE;
}

BOOL CResModule::CreateTranslatedResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszDestLangDllPath, LPCTSTR lpszPOFilePath)
{
	if (!CopyFile(lpszSrcLangDllPath, lpszDestLangDllPath, FALSE))
		MYERROR;

	m_hResDll = LoadLibraryEx (lpszSrcLangDllPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

	sDestFile = std::wstring(lpszDestLangDllPath);

	// get all translated strings
	if (!m_StringEntries.ParseFile(lpszPOFilePath, FALSE))
		goto DONE_ERROR;
	m_bTranslatedStrings = 0;
	m_bDefaultStrings = 0;
	m_bTranslatedDialogStrings = 0;
	m_bDefaultDialogStrings = 0;
	m_bTranslatedMenuStrings = 0;
	m_bDefaultMenuStrings = 0;

	if (!m_bQuiet)
		_tcprintf(_T("Translating StringTable..."));
	EnumResourceNames(m_hResDll, RT_STRING, EnumResNameWriteCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d translated, %4d not translated\n"), m_bTranslatedStrings, m_bDefaultStrings);

	if (!m_bQuiet)
		_tcprintf(_T("Translating Dialogs......."));
	EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameWriteCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d translated, %4d not translated\n"), m_bTranslatedDialogStrings, m_bDefaultDialogStrings);

	if (!m_bQuiet)
		_tcprintf(_T("Translating Menus........."));
	EnumResourceNames(m_hResDll, RT_MENU, EnumResNameWriteCallback, (long)this);
	if (!m_bQuiet)
		_tcprintf(_T("%4d translated, %4d not translated\n"), m_bTranslatedMenuStrings, m_bDefaultMenuStrings);

	FreeLibrary(m_hResDll);
	return TRUE;
DONE_ERROR:
	if (m_hResDll)
		FreeLibrary(m_hResDll);
	return FALSE;
}

BOOL CResModule::ExtractString(UINT nID)
{
	HRSRC		hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_STRING);
	HGLOBAL		hglStringTable;
	LPWSTR		p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = (LPWSTR)LockResource(hglStringTable);

	if (p == NULL)
		goto DONE_ERROR;
	/*	[Block of 16 strings.  The strings are Pascal style with a WORD 
	length preceding the string.  16 strings are always written, even 
	if not all slots are full.  Any slots in the block with no string 
	have a zero WORD for the length.] 
	*/

	//first check how much memory we need
	LPWSTR pp = p;
	for (int i=0; i<16; ++i)
	{
		int len = GET_WORD(pp);
		pp++;
		std::wstring msgid = std::wstring(pp, len);
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		ZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);

		if (wcslen(pBuf))
		{
			std::wstring str = std::wstring(pBuf);
			TCHAR szTempBuf[1024];
			_stprintf(szTempBuf, _T("#: ID:%d,"), nID);
			RESOURCEENTRY entry = m_StringEntries[str];
			entry.reference = entry.reference.append(szTempBuf);
			if (wcschr(str.c_str(), '%'))
				entry.flag = _T("#, c-format");
			m_StringEntries[str] = entry;
		}
		delete [] pBuf;
		pp += len;
	} // for (int i=0; i<16; ++i)
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	return TRUE;
DONE_ERROR:
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	MYERROR;
}

BOOL CResModule::ReplaceString(UINT nID, WORD wLanguage)
{
	HRSRC		hrsrc = FindResourceEx(m_hResDll, RT_STRING, MAKEINTRESOURCE(nID), wLanguage);
	HGLOBAL		hglStringTable;
	LPWSTR		p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = (LPWSTR)LockResource(hglStringTable);

	if (p == NULL)
		goto DONE_ERROR;
/*	[Block of 16 strings.  The strings are Pascal style with a WORD 
	length preceding the string.  16 strings are always written, even 
	if not all slots are full.  Any slots in the block with no string 
	have a zero WORD for the length.] 
*/
	
	//first check how much memory we need
	size_t nMem = 0;
	LPWSTR pp = p;
	for (int i=0; i<16; ++i)
	{
		nMem++;
		size_t len = GET_WORD(pp);
		pp++;
		std::wstring msgid = std::wstring(pp, len);
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		ZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);
		msgid = std::wstring(pBuf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy(pBuf, resEntry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		size_t newlen = wcslen(pBuf);
		if (newlen)
			nMem += newlen;
		else
			nMem += len;
		pp += len;
		delete [] pBuf;
	} // for (int i=0; i<16; ++i)

	WORD * newTable = new WORD[nMem + (nMem % 2)];
	ZeroMemory(newTable, (nMem + (nMem % 2))*2);

	size_t index = 0;
	for (int i=0; i<16; ++i)
	{
		int len = GET_WORD(p);
		p++;
		std::wstring msgid = std::wstring(p, len);
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		ZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);
		msgid = std::wstring(pBuf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy(pBuf, resEntry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		size_t newlen = wcslen(pBuf);
		if (newlen)
		{
			newTable[index++] = (WORD)newlen;
			wcsncpy((wchar_t *)&newTable[index], pBuf, newlen);
			index += newlen;
			m_bTranslatedStrings++;
		} // if (newlen)
		else
		{
			newTable[index++] = (WORD)len;
			if (len)
				wcsncpy((wchar_t *)&newTable[index], p, len);
			index += len;
			if (len)
				m_bDefaultStrings++;
		}
		p += len;
		delete [] pBuf;
	}

	if (!UpdateResource(m_hUpdateRes, RT_STRING, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newTable, (DWORD)(nMem + (nMem % 2))*2))
	{
		delete [] newTable;
		goto DONE_ERROR;
	}
	
	if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_STRING, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
	{
		delete [] newTable;
		goto DONE_ERROR;
	}
	delete [] newTable;
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	return TRUE;
DONE_ERROR:
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	MYERROR;
}

BOOL CResModule::ExtractMenu(UINT nID)
{
	HRSRC		hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_MENU);
	HGLOBAL		hglMenuTemplate;
	WORD		version, offset;
	const WORD*	p;

	if (!hrsrc)
		MYERROR;

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = (const WORD*)LockResource(hglMenuTemplate);

	if (p == NULL)
		MYERROR;

	//struct MenuHeader { 
	//	WORD   wVersion;           // Currently zero 
	//	WORD   cbHeaderSize;       // Also zero 
	//}; 

	version = GET_WORD(p);

	p++;

	switch (version)
	{
	case 0:
		{
			offset = GET_WORD(p);
			p += offset;
			p++;
			if (!ParseMenuResource(p))
				goto DONE_ERROR;
		}
		break;
	default:
		goto DONE_ERROR;
	}

	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	MYERROR;
}

BOOL CResModule::ReplaceMenu(UINT nID, WORD wLanguage)
{
	HRSRC		hrsrc = FindResourceEx(m_hResDll, RT_MENU, MAKEINTRESOURCE(nID), wLanguage);
	HGLOBAL		hglMenuTemplate;
	WORD		version, offset;
	LPWSTR		p;

	if (!hrsrc)
		MYERROR;	//just the language wasn't found

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = (LPWSTR)LockResource(hglMenuTemplate);

	if (p == NULL)
		MYERROR;

	//struct MenuHeader { 
	//	WORD   wVersion;           // Currently zero 
	//	WORD   cbHeaderSize;       // Also zero 
	//}; 
	version = GET_WORD(p);

	p++;

	switch (version)
	{
	case 0:
		{
			offset = GET_WORD(p);
			p += offset;
			p++;
			size_t nMem = 0;
			if (!CountMemReplaceMenuResource((WORD *)p, &nMem, NULL))
				goto DONE_ERROR;
			WORD * newMenu = new WORD[nMem + (nMem % 2)+2];
			ZeroMemory(newMenu, (nMem + (nMem % 2)+2)*2);
			size_t index = 2;		// MenuHeader has 2 WORDs zero
			if (!CountMemReplaceMenuResource((WORD *)p, &index, newMenu))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			} 

			if (!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu, (DWORD)(nMem + (nMem % 2)+2)*2))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			} 
			delete [] newMenu;
		}
		break;
	default:
		goto DONE_ERROR;
	}

	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	MYERROR;
}

const WORD* CResModule::ParseMenuResource(const WORD * res)
{
	WORD		flags;
	WORD		id = 0;
	LPCWSTR		str;

	//struct PopupMenuItem { 
	//	WORD   fItemFlags; 
	//	WCHAR  szItemText[]; 
	//}; 
	//struct NormalMenuItem { 
	//	WORD   fItemFlags; 
	//	WORD   wMenuID; 
	//	WCHAR  szItemText[]; 
	//}; 

	do
	{
		flags = GET_WORD(res);
		res++;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res);	//normal menu item
			res++;
		}
		else
			id = (WORD)-1;			//popup menu item

		str = (LPCWSTR)res;
		size_t l = wcslen(str)+1;
		res += l;

		if (flags & MF_POPUP)
		{
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			ZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.reference += _T("#: MenuEntry");

			m_StringEntries[wstr] = entry;
			delete [] pBuf;

			if ((res = ParseMenuResource(res))==0)
				return NULL;
		}
		else if (id != 0)
		{
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			ZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.reference = _T("#: MenuEntry");

			m_StringEntries[wstr] = entry;
			delete [] pBuf;
		}
	} while (!(flags & MF_END));
	return res;
}

const WORD* CResModule::CountMemReplaceMenuResource(const WORD * res, size_t * wordcount, WORD * newMenu)
{
	WORD		flags;
	WORD		id = 0;

	//struct PopupMenuItem { 
	//	WORD   fItemFlags; 
	//	WCHAR  szItemText[]; 
	//}; 
	//struct NormalMenuItem { 
	//	WORD   fItemFlags; 
	//	WORD   wMenuID; 
	//	WCHAR  szItemText[]; 
	//}; 

	do
	{
		flags = GET_WORD(res);
		res++;
		if (newMenu == NULL)
			(*wordcount)++;
		else
			newMenu[(*wordcount)++] = flags;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res);	//normal menu item
			res++;
			if (newMenu == NULL)
				(*wordcount)++;
			else
				newMenu[(*wordcount)++] = id;
		}
		else
			id = (WORD)-1;			//popup menu item

		if (flags & MF_POPUP)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;

			if ((res = CountMemReplaceMenuResource(res, wordcount, newMenu))==0)
				return NULL;
		}
		else if (id != 0)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;
		}
		else
		{
			if (newMenu)
				wcscpy((wchar_t *)&newMenu[(*wordcount)], (LPCWSTR)res);
			(*wordcount) += wcslen((LPCWSTR)res) + 1;
			res += wcslen((LPCWSTR)res) + 1;
		}
	} while (!(flags & MF_END));
	return res;
}

BOOL CResModule::ExtractDialog(UINT nID)
{
	const WORD*	lpDlg;
	const WORD*	lpDlgItem;
	DIALOGINFO	dlg;
	DLGITEMINFO	dlgItem;
	WORD		bNumControls;
	HRSRC		hrsrc;
	HGLOBAL		hGlblDlgTemplate;

	hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_DIALOG);

	if (hrsrc == NULL)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);
	if (hGlblDlgTemplate == NULL)
		MYERROR;

	lpDlg = (const WORD*) LockResource(hGlblDlgTemplate);

	if (lpDlg == NULL)
		MYERROR;

	lpDlgItem = (const WORD*) GetDialogInfo(lpDlg, &dlg);
	bNumControls = dlg.nbItems;

	if (dlg.caption)
	{
		TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
		ZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
		_tcscpy(pBuf, dlg.caption);
		CUtils::StringExtend(pBuf);

		TCHAR szTempBuf[1024];
		_stprintf(szTempBuf, _T("#: Dlg-ID:%d,"), nID);
		std::wstring wstr = std::wstring(pBuf);
		RESOURCEENTRY entry = m_StringEntries[wstr];
		entry.reference = entry.reference.append(szTempBuf);

		m_StringEntries[wstr] = entry;
		delete [] pBuf;
	}

	while (bNumControls-- != 0)
	{
		TCHAR szTitle[500];
		ZeroMemory(szTitle, sizeof(szTitle));
		BOOL  bCode;

		lpDlgItem = GetControlInfo((WORD *) lpDlgItem, &dlgItem, dlg.dialogEx, &bCode);

		if (bCode == FALSE)
			_tcscpy(szTitle, dlgItem.windowName);

		if (_tcslen(szTitle) > 0)
		{
			CUtils::StringExtend(szTitle);

			TCHAR szTempBuf[1024];
			_stprintf(szTempBuf, _T("#: Control-ID:%d,"), dlgItem.id);
			std::wstring wstr = std::wstring(szTitle);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.reference = entry.reference.append(szTempBuf);

			m_StringEntries[wstr] = entry;
		}
	}

	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	return (TRUE);
}

BOOL CResModule::ReplaceDialog(UINT nID, WORD wLanguage)
{
	const WORD*	lpDlg;
	HRSRC		hrsrc;
	HGLOBAL		hGlblDlgTemplate;

	hrsrc = FindResourceEx(m_hResDll, RT_DIALOG, MAKEINTRESOURCE(nID), wLanguage);

	if (hrsrc == NULL)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);

	if (hGlblDlgTemplate == NULL)
		MYERROR;

	lpDlg = (WORD *) LockResource(hGlblDlgTemplate);

	if (lpDlg == NULL)
		MYERROR;

	size_t nMem = 0;
	const WORD * p = lpDlg;
	if (!CountMemReplaceDialogResource(p, &nMem, NULL))
		goto DONE_ERROR;
	WORD * newDialog = new WORD[nMem + (nMem % 2)];
	ZeroMemory(newDialog, (nMem + (nMem % 2))*2);

	size_t index = 0;
	if (!CountMemReplaceDialogResource(lpDlg, &index, newDialog))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}
	
	if (!UpdateResource(m_hUpdateRes, RT_DIALOG, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newDialog, (DWORD)(nMem + (nMem % 2))*2))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}
	
	if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_DIALOG, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	delete [] newDialog;
	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	MYERROR;
}

const WORD* CResModule::GetDialogInfo(const WORD * pTemplate, LPDIALOGINFO lpDlgInfo)
{
	const WORD* p = (const WORD *)pTemplate;

	lpDlgInfo->style = GET_DWORD(p);
	p += 2;

	if (lpDlgInfo->style == 0xffff0001)	// DIALOGEX resource
	{
		lpDlgInfo->dialogEx = TRUE;
		lpDlgInfo->helpId   = GET_DWORD(p);
		p += 2;
		lpDlgInfo->exStyle  = GET_DWORD(p);
		p += 2;
		lpDlgInfo->style    = GET_DWORD(p);
		p += 2;
	}
	else
	{
		lpDlgInfo->dialogEx = FALSE;
		lpDlgInfo->helpId   = 0;
		lpDlgInfo->exStyle  = GET_DWORD(p);
		p += 2;
	}

	lpDlgInfo->nbItems = GET_WORD(p);
	p++;

	lpDlgInfo->x = GET_WORD(p);
	p++;

	lpDlgInfo->y = GET_WORD(p);
	p++;

	lpDlgInfo->cx = GET_WORD(p);
	p++;

	lpDlgInfo->cy = GET_WORD(p);
	p++;

	// Get the menu name

	switch (GET_WORD(p))
	{
	case 0x0000:
		lpDlgInfo->menuName = NULL;
		p++;
		break;
	case 0xffff:
		lpDlgInfo->menuName = (LPCTSTR) (WORD) GET_WORD(p + 1);
		p += 2;
		break;
	default:
		lpDlgInfo->menuName = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
		break;
	}

	// Get the class name

	switch (GET_WORD(p))
	{
	case 0x0000:
		lpDlgInfo->className = (LPCTSTR)MAKEINTATOM(32770);
		p++;
		break;
	case 0xffff:
		lpDlgInfo->className = (LPCTSTR) (WORD) GET_WORD(p + 1);
		p += 2;
		break;
	default:
		lpDlgInfo->className = (LPCTSTR) p;
		p += wcslen((LPCTSTR)p) + 1;
		break;
	}

	// Get the window caption

	lpDlgInfo->caption = (LPCTSTR)p;
	p += wcslen((LPCWSTR) p) + 1;

	// Get the font name

	if (lpDlgInfo->style & DS_SETFONT)
	{
		lpDlgInfo->pointSize = GET_WORD(p);
		p++;

		if (lpDlgInfo->dialogEx)
		{
			lpDlgInfo->weight = GET_WORD(p);
			p++;
			lpDlgInfo->italic = LOBYTE(GET_WORD(p));
			p++;
		}
		else
		{
			lpDlgInfo->weight = FW_DONTCARE;
			lpDlgInfo->italic = FALSE;
		}

		lpDlgInfo->faceName = (LPCTSTR)p;
		p += wcslen((LPCWSTR) p) + 1;
	}
	// First control is on DWORD boundary
	return (const WORD *) ((((long)p) + 3) & ~3);
}

const WORD* CResModule::GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID)
{
	if (dialogEx)
	{
		lpDlgItemInfo->helpId = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->exStyle = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->style = GET_DWORD(p);
		p += 2;
	}
	else
	{
		lpDlgItemInfo->helpId = 0;
		lpDlgItemInfo->style = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->exStyle = GET_DWORD(p);
		p += 2;
	}

	lpDlgItemInfo->x = GET_WORD(p);
	p++;

	lpDlgItemInfo->y = GET_WORD(p);
	p++;

	lpDlgItemInfo->cx = GET_WORD(p);
	p++;

	lpDlgItemInfo->cy = GET_WORD(p);
	p++;

	if (dialogEx)
	{
		// ID is a DWORD for DIALOGEX
		lpDlgItemInfo->id = (WORD) GET_DWORD(p);
		p += 2;
	}
	else
	{
		lpDlgItemInfo->id = GET_WORD(p);
		p++;
	}

	if (GET_WORD(p) == 0xffff)
	{
		GET_WORD(p + 1);

		p += 2;
	}
	else
	{
		lpDlgItemInfo->className = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
	}

	if (GET_WORD(p) == 0xffff)	// an integer ID?
	{
		*bIsID = TRUE;
		lpDlgItemInfo->windowName = (LPCTSTR) (DWORD) GET_WORD(p + 1);
		p += 2;
	}
	else
	{
		*bIsID = FALSE;
		lpDlgItemInfo->windowName = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
	}

	if (GET_WORD(p))
	{
		lpDlgItemInfo->data = (LPVOID) (p + 1);
		p += GET_WORD(p) / sizeof(WORD);
	}
	else
		lpDlgItemInfo->data = NULL;

	p++;
	// Next control is on DWORD boundary
	return (const WORD *)((((long)p) + 3) & ~3);
}

const WORD * CResModule::CountMemReplaceDialogResource(const WORD * res, size_t * wordcount, WORD * newDialog)
{
	BOOL bEx = FALSE;
	DWORD style = GET_DWORD(res);
	if (newDialog)
	{
		newDialog[(*wordcount)++] = GET_WORD(res++);
		newDialog[(*wordcount)++] = GET_WORD(res++);
	} // if (newDialog)
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (style == 0xffff0001)	// DIALOGEX resource
	{
		bEx = TRUE;
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);	//help id
			newDialog[(*wordcount)++] = GET_WORD(res++);	//help id
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			style = GET_DWORD(res);
			newDialog[(*wordcount)++] = GET_WORD(res++);	//style
			newDialog[(*wordcount)++] = GET_WORD(res++);	//style
		} // if (newDialog
		else
		{
			res += 4;
			style = GET_DWORD(res);
			res += 2;
			(*wordcount) += 6;
		}
	}
	else
	{
		bEx = FALSE;
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			//style = GET_DWORD(res);
			//newDialog[(*wordcount)++] = GET_WORD(res++);	//style
			//newDialog[(*wordcount)++] = GET_WORD(res++);	//style
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);
	WORD nbItems = GET_WORD(res);
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //x
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //y
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //cx
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //cy
	(*wordcount)++;
	res++;

	// Get the menu name

	switch (GET_WORD(res))
	{
	case 0x0000:
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		(*wordcount)++;
		res++;
		break;
	case 0xffff:
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		} // if (newDialog)
		else
		{
			(*wordcount) += 2;
			res += 2;
		}
		break;
	default:
		if (newDialog)
		{
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		}
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
		break;
	}

	// Get the class name

	switch (GET_WORD(res))
	{
	case 0x0000:
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		(*wordcount)++;
		res++;
		break;
	case 0xffff:
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		} // if (newDialog)
		else
		{
			(*wordcount) += 2;
			res += 2;
		}
		break;
	default:
		if (newDialog)
		{
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		}
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
		break;
	}

	// Get the window caption

	ReplaceStr((LPCWSTR)res, newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
	res += wcslen((LPCWSTR)res) + 1;

	// Get the font name

	if (style & DS_SETFONT)
	{
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		res++;
		(*wordcount)++;

		if (bEx)
		{
			if (newDialog)
			{
				newDialog[(*wordcount)++] = GET_WORD(res++);
				newDialog[(*wordcount)++] = GET_WORD(res++);
			} // if (newDialog)
			else
			{
				res += 2;
				(*wordcount) += 2;
			}
		} // if (bEx)

		if (newDialog)
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		(*wordcount) += wcslen((LPCWSTR)res) + 1;
		res += wcslen((LPCWSTR)res) + 1;
	}
	// First control is on DWORD boundary
	while ((*wordcount)%2)
		(*wordcount)++;
	while ((ULONG)res % 4)
		res++;

	while (nbItems--)
	{
		res = ReplaceControlInfo(res, wordcount, newDialog, bEx);
	}
	return res;
}

const WORD* CResModule::ReplaceControlInfo(const WORD * res, size_t * wordcount, WORD * newDialog, BOOL bEx)
{
	if (bEx)
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);	//helpid
			newDialog[(*wordcount)++] = GET_WORD(res++);	//helpid
		} // if (newDialog)
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	} // if (bEx)
	if (newDialog)
	{
		newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
		newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
	} // if (newDialog)
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (newDialog)
	{
		newDialog[(*wordcount)++] = GET_WORD(res++);	//style
		newDialog[(*wordcount)++] = GET_WORD(res++);	//style
	} // if (newDialog)
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//x
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//y
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//cx
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//cy
	res++;
	(*wordcount)++;

	if (bEx)
	{
		// ID is a DWORD for DIALOGEX
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		res++;
		(*wordcount)++;
	}

	if (GET_WORD(res) == 0xffff)	//classID
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		if (newDialog)
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
	}

	if (GET_WORD(res) == 0xffff)	// an integer ID?
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		ReplaceStr((LPCWSTR)res, newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
		res += wcslen((LPCWSTR)res) + 1;
	}

	if (newDialog)
		memcpy(&newDialog[(*wordcount)], res, (GET_WORD(res)+1)*sizeof(WORD));
	(*wordcount) += (GET_WORD(res)+1);
	res += (GET_WORD(res)+1);
	// Next control is on DWORD boundary
	while ((*wordcount) % 2)
		(*wordcount)++;
	return (const WORD *)((((long)res) + 3) & ~3);
}

BOOL CALLBACK CResModule::EnumResNameCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	CResModule* lpResModule = (CResModule*)lParam;

	if (lpszType == RT_STRING)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractString(LOWORD(lpszName)))
				return FALSE;
		}
	} 
	else if (lpszType == RT_MENU)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractMenu(LOWORD(lpszName)))
				return FALSE;
		}
	}
	else if (lpszType == RT_DIALOG)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractDialog(LOWORD(lpszName)))
				return FALSE;
		}
	}
	return TRUE;
}

#pragma warning(push)
#pragma warning(disable: 4189)
BOOL CALLBACK CResModule::EnumResNameWriteCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	CResModule* lpResModule = (CResModule*)lParam;
	return EnumResourceLanguages(hModule, lpszType, lpszName, (ENUMRESLANGPROC)&lpResModule->EnumResWriteLangCallback, lParam);
}
#pragma warning(pop)

BOOL CALLBACK CResModule::EnumResWriteLangCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, WORD wLanguage, LONG_PTR lParam)
{
	BOOL bRes = FALSE;
	CResModule* lpResModule = (CResModule*)lParam;


	lpResModule->m_hUpdateRes = BeginUpdateResource(lpResModule->sDestFile.c_str(), FALSE);
	if (lpszType == RT_STRING)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceString(LOWORD(lpszName), wLanguage);
		}
	} 
	else if (lpszType == RT_MENU)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceMenu(LOWORD(lpszName), wLanguage);
		}
	}
	else if (lpszType == RT_DIALOG)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceDialog(LOWORD(lpszName), wLanguage);
		}
	}
	if (!EndUpdateResource(lpResModule->m_hUpdateRes, !bRes))
		MYERROR;
	return bRes;

}

void CResModule::ReplaceStr(LPCWSTR src, WORD * dest, size_t * count, int * translated, int * def)
{
	TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
	ZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
	wcscpy(pBuf, src);
	CUtils::StringExtend(pBuf);

	std::wstring wstr = std::wstring(pBuf);
	RESOURCEENTRY entry = m_StringEntries[wstr];
	if (entry.msgstr.size())
	{
		wcscpy(pBuf, entry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		if (dest)
			wcscpy((wchar_t *)&dest[(*count)], pBuf);
		(*count) += wcslen(pBuf)+1;
		(*translated)++;
	}
	else
	{
		if (dest)
			wcscpy((wchar_t *)&dest[(*count)], src);
		(*count) += wcslen(src) + 1;
		if (wcslen(src))
			(*def)++;
	}
	delete [] pBuf;
}
