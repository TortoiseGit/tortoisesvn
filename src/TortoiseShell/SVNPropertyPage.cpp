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

#include "ShellExt.h"
#include "svnpropertypage.h"
#include "ProgressDlg.h"
#include "UnicodeStrings.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"

#define MAX_STRING_LENGTH		4096			//should be big enough

extern void Unescape(char * psz);

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,
                                  LPARAM lParam)
{
	for (std::vector<stdstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		SVNStatus svn = SVNStatus();
		if (svn.GetStatus(CTSVNPath(I->c_str())) == (-2))
			return NOERROR;			// file/directory not under version control

		if (svn.status->entry == NULL)
			return NOERROR;
	} // for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)

	if (files_.size() == 0)
		return NOERROR;

	LoadLangDll();
    PROPSHEETPAGE psp;
	ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	HPROPSHEETPAGE hPage;
	CSVNPropertyPage *sheetpage = new CSVNPropertyPage(files_);

    psp.dwSize = sizeof (psp);
    psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK;	
	psp.hInstance = g_hResInst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE);
    psp.pszIcon = MAKEINTRESOURCE(IDI_APP);
    psp.pszTitle = _T("Subversion");
    psp.pfnDlgProc = (DLGPROC) PageProc;
    psp.lParam = (LPARAM) sheetpage;
    psp.pfnCallback = PropPageCallbackProc;
    psp.pcRefParent = &g_cRefThisDll;

    hPage = CreatePropertySheetPage (&psp);

	if (hPage != NULL)
	{
        if (!lpfnAddPage (hPage, lParam))
        {
            delete sheetpage;
            DestroyPropertySheetPage (hPage);
        }
	} // if (hPage != NULL) 

    return NOERROR;
}



STDMETHODIMP CShellExt::ReplacePage (UINT /*uPageID*/, LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, LPARAM /*lParam*/)
{
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog procedures and other callback functions

BOOL CALLBACK PageProc (HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CSVNPropertyPage * sheetpage;

    if (uMessage == WM_INITDIALOG)
    {
        sheetpage = (CSVNPropertyPage*) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) sheetpage);
        sheetpage->SetHwnd(hwnd);
    } // if (uMessage == WM_INITDIALOG) 
    else
    {
        sheetpage = (CSVNPropertyPage*) GetWindowLongPtr (hwnd, GWLP_USERDATA);
    }

    if (sheetpage != 0L)
        return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
    else
        return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND /*hwnd*/, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    // Delete the page before closing.
    if ( PSPCB_RELEASE == uMsg )
    {
        CSVNPropertyPage* sheetpage = (CSVNPropertyPage*) ppsp->lParam;
        if (sheetpage != NULL)
            delete sheetpage;
    } // if ( PSPCB_RELEASE == uMsg ) 
    return 1;
}

// *********************** CSVNPropertyPage *************************

CSVNPropertyPage::CSVNPropertyPage(const std::vector<stdstring> &newFilenames)
	:filenames(newFilenames)
{
}

CSVNPropertyPage::~CSVNPropertyPage(void)
{
}

void CSVNPropertyPage::SetHwnd(HWND newHwnd)
{
    m_hwnd = newHwnd;
}

BOOL CSVNPropertyPage::PageProc (HWND /*hwnd*/, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_INITDIALOG:
		{
			InitWorkfileView();
			return TRUE;
		}
	case WM_NOTIFY:
		{
			LPNMHDR point = (LPNMHDR)lParam;
			int code = point->code;
			//
			// Respond to notifications.
			//    
			if (code == PSN_APPLY)
			{
			}
			SetWindowLongPtr (m_hwnd, DWLP_MSGRESULT, FALSE);
			return TRUE;        

			}
		case WM_DESTROY:
			return TRUE;

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
					if (LOWORD(wParam) == IDC_SHOWLOG)
					{
						STARTUPINFO startup;
						PROCESS_INFORMATION process;
						memset(&startup, 0, sizeof(startup));
						startup.cb = sizeof(startup);
						memset(&process, 0, sizeof(process));
						CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
						stdstring svnCmd = _T(" /command:");
						svnCmd += _T("log /path:\"");
						svnCmd += filenames.front().c_str();
						svnCmd += _T("\"");
						if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
						{
							CloseHandle(process.hThread);
							CloseHandle(process.hProcess);
						}
					}
					if (LOWORD(wParam) == IDC_EDITPROPERTIES)
					{
						DWORD pathlength = GetTempPath(0, NULL);
						TCHAR * path = new TCHAR[pathlength+1];
						TCHAR * tempFile = new TCHAR[pathlength + 100];
						GetTempPath (pathlength+1, path);
						GetTempFileName (path, _T("svn"), 0, tempFile);
						stdstring retFilePath = stdstring(tempFile);

						HANDLE file = ::CreateFile (tempFile,
							GENERIC_WRITE, 
							FILE_SHARE_READ, 
							0, 
							CREATE_ALWAYS, 
							FILE_ATTRIBUTE_TEMPORARY,
							0);

						delete path;
						delete tempFile;
						if (file != INVALID_HANDLE_VALUE)
						{
							DWORD written = 0;
							for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
							{
								::WriteFile (file, I->c_str(), I->size()*sizeof(TCHAR), &written, 0);
								::WriteFile (file, _T("\n"), 2, &written, 0);
							}
							::CloseHandle(file);

							STARTUPINFO startup;
							PROCESS_INFORMATION process;
							memset(&startup, 0, sizeof(startup));
							startup.cb = sizeof(startup);
							memset(&process, 0, sizeof(process));
							CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
							stdstring svnCmd = _T(" /command:");
							svnCmd += _T("properties /path:\"");
							svnCmd += retFilePath.c_str();
							if (CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
							{
								CloseHandle(process.hThread);
								CloseHandle(process.hProcess);
							}
						}
					}
					break;
			} // switch (HIWORD(wParam)) 
	} // switch (uMessage) 
	//CShellUpdater::Instance().Flush();
	return FALSE;
}
void CSVNPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf, size_t buflen)
{
	struct tm newtime;
	SYSTEMTIME systime;
	TCHAR timebuf[MAX_STRING_LENGTH];
	TCHAR datebuf[MAX_STRING_LENGTH];

	LCID locale = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	*buf = '\0';
	if (time)
	{
		_localtime64_s(&newtime, &time);

		systime.wDay = (WORD)newtime.tm_mday;
		systime.wDayOfWeek = (WORD)newtime.tm_wday;
		systime.wHour = (WORD)newtime.tm_hour;
		systime.wMilliseconds = 0;
		systime.wMinute = (WORD)newtime.tm_min;
		systime.wMonth = (WORD)newtime.tm_mon+1;
		systime.wSecond = (WORD)newtime.tm_sec;
		systime.wYear = (WORD)newtime.tm_year+1900;
		if (CRegStdWORD(_T("Software\\TortoiseSVN\\LogDateFormat")) == 1)
			GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		else
			GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_STRING_LENGTH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_STRING_LENGTH);
		*buf = '\0';
		_tcsncat_s(buf, buflen, timebuf, MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, _T(", "), MAX_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, datebuf, MAX_STRING_LENGTH-1);
	}
}

void CSVNPropertyPage::InitWorkfileView()
{
	SVNStatus svn = SVNStatus();
	TCHAR tbuf[MAX_STRING_LENGTH];
	if (filenames.size() == 1)
	{
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			TCHAR buf[MAX_STRING_LENGTH];
			__time64_t	time;
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->entry->kind == svn_node_file)
				{
					//disable the 'recursive' checkbox for files
					HWND recursivewnd = GetDlgItem(m_hwnd, IDC_RECURSIVE);
					if (GetFocus() == recursivewnd)
						::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
					::EnableWindow(recursivewnd, FALSE);					
				}
				if (svn.status->text_status == svn_wc_status_added)
				{
					// disable the "show log" button for added files
					HWND showloghwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
					if (GetFocus() == showloghwnd)
						::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
					::EnableWindow(showloghwnd, FALSE);
				}
				else
				{
					_stprintf_s(buf, MAX_STRING_LENGTH, _T("%d"), svn.status->entry->revision);
					SetDlgItemText(m_hwnd, IDC_REVISION, buf);
				}
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
					_tcsncpy_s(tbuf, MAX_STRING_LENGTH, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				}
				if (svn.status->text_status != svn_wc_status_added)
				{
					_stprintf_s(buf, MAX_STRING_LENGTH, _T("%d"), svn.status->entry->cmt_rev);
					SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
					time = (__time64_t)svn.status->entry->cmt_date/1000000L;
					Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
					SetDlgItemText(m_hwnd, IDC_CDATE, buf);
				}
				if (svn.status->entry->cmt_author)
					SetDlgItemText(m_hwnd, IDC_AUTHOR, UTF8ToWide(svn.status->entry->cmt_author).c_str());
				SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
				SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
				time = (__time64_t)svn.status->entry->text_time/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);
				time = (__time64_t)svn.status->entry->prop_time/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_PROPDATE, buf);
				if (svn.status->locked)
				{
					MAKESTRING(IDS_PROPLOCKED);
					SetDlgItemText(m_hwnd, IDC_LOCKED, stringtablebuffer);
				}
				else
				{
					SetDlgItemText(m_hwnd, IDC_LOCKED, _T(""));
				}
				if (svn.status->entry->lock_owner)
					SetDlgItemText(m_hwnd, IDC_LOCKOWNER, UTF8ToWide(svn.status->entry->lock_owner).c_str());
				time = (__time64_t)svn.status->entry->lock_creation_date/1000000L;
				Time64ToTimeString(time, buf, MAX_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
			} // if (svn.status->entry != NULL)
		} // if (svn.GetStatus(filename.c_str())>(-2)) 
	} // if (filenames.size() == 1) 
	else if (filenames.size() != 0)
	{
		//deactivate the show log button
		HWND logwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
		if (GetFocus() == logwnd)
			::SendMessage(m_hwnd, WM_NEXTDLGCTL, 0, FALSE);
		::EnableWindow(logwnd, FALSE);
		//get the handle of the listview
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
					_tcsncpy_s(tbuf, MAX_STRING_LENGTH, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
					TCHAR * ptr = _tcsrchr(tbuf, '/');
					if (ptr != 0)
					{
						*ptr = 0;
					}
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				}
				SetDlgItemText(m_hwnd, IDC_LOCKED, _T(""));
			}
		}
	} 
}


