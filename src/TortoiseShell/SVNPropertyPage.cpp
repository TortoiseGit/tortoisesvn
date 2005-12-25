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
//
#include "stdafx.h"

#include "ShellExt.h"
#include "svnpropertypage.h"
#include "SVNProperties.h"
#include "ProgressDlg.h"
#include "UnicodeStrings.h"
#include "UnicodeUtils.h"
#include "SVNStatus.h"

#define MAX_PROP_STRING_LENGTH		4096			//should be big enough

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
    psp.pszIcon = MAKEINTRESOURCE(IDI_MENU);
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

BOOL CSVNPropertyPage::PageProc (HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if (hwnd == hwndTT)
	{
		ATLTRACE2(_T("hwndTT message\n"));
	}
	switch (uMessage)
	{
	case WM_INITDIALOG:
		{
			InitWorkfileView();
			HWND hwndCombo = GetDlgItem(hwnd, IDC_EDITNAME);
			COMBOBOXINFO cbInfo = {0};
			cbInfo.cbSize = sizeof(COMBOBOXINFO);
			if (hwndCombo)
			{
#				define FOLDERPROPS 1
#				define FILEPROPS   2
				int iShowProps = 0;
				if (filenames.size() == 1)
				{
					if (PathIsDirectory(filenames.front().c_str()))
						iShowProps = FOLDERPROPS | FILEPROPS;		// show fileprops on folders too (for recursive prop setting)
					else
						iShowProps = FILEPROPS;
				}
				else
				{
					// when multiple items are selected, show all available properties
					iShowProps = FILEPROPS | FOLDERPROPS;
				}
				// Initialize the combobox with the default svn: properties
				if (iShowProps & FILEPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:eol-style"));
				if (iShowProps & FILEPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:executable"));
				if (iShowProps & FOLDERPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:externals"));
				if (iShowProps & FOLDERPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:ignore"));
				if (iShowProps & FILEPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:keywords"));
				if (iShowProps & FILEPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:needs-lock"));
				if (iShowProps & FILEPROPS)
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:mime-type"));

				if (iShowProps & FOLDERPROPS)
				{
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:url"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:logregex"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:label"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:message"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:number"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:warnifnoissue"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:append"));

					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logtemplate"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logwidthmarker"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logminsize"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logfilelistenglish"));
					SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:projectlanguage"));
				}
				GetComboBoxInfo(hwndCombo, &cbInfo);
			}
			// Create a tooltip window
			HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
										TOOLTIPS_CLASS,
										NULL,
										WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										CW_USEDEFAULT,
										hwnd,
										NULL,
										g_hResInst,
										NULL
										);

			SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
			ti.hwnd = hwnd;
			ti.hinst = NULL;
			ti.lpszText = LPSTR_TEXTCALLBACK;
			ti.lParam = 0;

			// Send ADDTOOL messages to the tooltip control window
			ti.uId = (UINT)GetDlgItem(hwnd, IDC_EDITVALUE);
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
			ti.uId = (UINT)cbInfo.hwndItem;
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
			ti.uId = (UINT)GetDlgItem(hwnd, IDC_RECURSIVE);
			ti.lParam = IDC_RECURSIVE;
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
			
			return TRUE;
		}
	case WM_NOTIFY:
		{
			LPNMHDR point = (LPNMHDR)lParam;
			int code = point->code;
			//
			// Respond to notifications.
			//    
			if (wParam == IDC_PROPLIST)
			{
				if ((code == NM_CLICK)||(code == LVN_ITEMACTIVATE)||(code == LVN_ITEMCHANGED))
				{
					//enable the remove button if a row in the listcontrol is selected
					//if no row is selected then disable the remove button
					HWND lvh = GetDlgItem(m_hwnd, IDC_PROPLIST);
					HWND button = GetDlgItem(m_hwnd, IDC_REMOVEBUTTON);
					UINT count = ListView_GetSelectedCount(lvh);
					EnableWindow(button, (count > 0));
					//now fill in the edit boxes so it will be easier to edit existing properties
					if (count > 0)
					{
						TCHAR * buf = NULL;
						int sel = ListView_GetSelectionMark(lvh);
						ListView_GetItemTextEx(lvh, sel, 0, buf);
						SetDlgItemText(m_hwnd, IDC_EDITNAME, buf);
						if (propmap.find(stdstring(buf)) != propmap.end())
							SetDlgItemText(m_hwnd, IDC_EDITVALUE, propmap.find(stdstring(buf))->second.c_str());
						delete [] buf;
					} // if (count > 0) 
					else
					{
						SetDlgItemText(m_hwnd, IDC_EDITNAME, _T(""));
						SetDlgItemText(m_hwnd, IDC_EDITVALUE, _T(""));
					} 
				} // if ((code == LVN_ITEMCHANGED)||(code == LVN_ITEMACTIVATE)) 
			} // if (wParam == IDC_PROPLIST)
			if (code == TTN_GETDISPINFO)
			{
				int nWindowWidth = 400;
				LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO) lParam;
				TCHAR * name = NULL;
				TCHAR buf[MAX_PROP_STRING_LENGTH];
				GetDlgItemTextEx(m_hwnd, IDC_EDITNAME, name);
				lpnmtdi->hinst = NULL;
				lpnmtdi->szText[0] = 0;
				lpnmtdi->lpszText = NULL;
				lpnmtdi->uFlags = NULL;
				if (lpnmtdi->lParam == IDC_RECURSIVE)
				{
					LoadString(g_hResInst, IDS_TT_RECURSIVE, buf, MAX_PROP_STRING_LENGTH);
					lpnmtdi->lpszText = buf;
				}
				else
				{
					if (_tcscmp(name, _T("svn:externals"))==0)
					{
						LoadString(g_hResInst, IDS_TT_EXTERNALS, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("svn:executable"))==0)
					{
						LoadString(g_hResInst, IDS_TT_EXECUTABLE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("svn:needs-lock"))==0)
					{
						LoadString(g_hResInst, IDS_TT_NEEDSLOCK, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("svn:mime-type"))==0)
					{
						LoadString(g_hResInst, IDS_TT_MIMETYPE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("svn:ignore"))==0)
					{
						LoadString(g_hResInst, IDS_TT_IGNORE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("svn:keywords"))==0)
					{
						LoadString(g_hResInst, IDS_TT_KEYWORDS, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
						nWindowWidth = 800;
					}
					if (_tcscmp(name, _T("svn:eol-style"))==0)
					{
						LoadString(g_hResInst, IDS_TT_EOLSTYLE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}

					if (_tcscmp(name, _T("bugtraq:label"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQLABEL, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:message"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQMESSAGE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:number"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQNUMBER, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:url"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQURL, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:warnifnoissue"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQWARNNOISSUE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:append"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQAPPEND, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("bugtraq:logregex"))==0)
					{
						LoadString(g_hResInst, IDS_TT_BQLOGREGEX, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("tsvn:logtemplate"))==0)
					{
						LoadString(g_hResInst, IDS_TT_TSVNLOGTEMPLATE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("tsvn:logwidthmarker"))==0)
					{
						LoadString(g_hResInst, IDS_TT_TSVNLOGWIDTHMARKER, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("tsvn:logminsize"))==0)
					{
						LoadString(g_hResInst, IDS_TT_TSVNLOGMINSIZE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("tsvn:logfilelistenglish"))==0)
					{
						LoadString(g_hResInst, IDS_TT_TSVNLOGFILELISTENGLISH, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
					if (_tcscmp(name, _T("tsvn:projectlanguage"))==0)
					{
						LoadString(g_hResInst, IDS_TT_TSVNPROJECTLANGUAGE, buf, MAX_PROP_STRING_LENGTH);
						lpnmtdi->lpszText = buf;
					}
				}

				SendMessage(lpnmtdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, nWindowWidth);
				delete [] name;
			} // if (code == TTN_GETDISPINFO) 
			if (code == PSN_APPLY)
			{
				if (IsWindowEnabled(GetDlgItem(m_hwnd, IDC_ADDBUTTON)))
				{
					TCHAR s[MAX_PROP_STRING_LENGTH];
					LoadString(g_hResInst, IDS_PROPSNOTSAVED, s, MAX_PROP_STRING_LENGTH);
					if (::MessageBox(m_hwnd, s, _T("Subversion"), MB_YESNO)==IDYES)
					{
						SaveProperties();
					}
				}
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
					if (LOWORD(wParam) == IDC_REMOVEBUTTON)
					{
						HWND lvh = GetDlgItem(m_hwnd, IDC_PROPLIST);
						int sel = ListView_GetSelectionMark(lvh);
						if (sel < 0)
							return TRUE;			//nothing selected to delete
						TCHAR * buf = NULL;
						ListView_GetItemTextEx(lvh, sel, 0, buf);
						HWND hCheck = GetDlgItem(m_hwnd, IDC_RECURSIVE);
						BOOL checked = (SendMessage(hCheck,(UINT) BM_GETCHECK, 0, 0) == BST_CHECKED);
						ULONGLONG all = filenames.size();
						ULONGLONG count = 0;
						// show a progress dialog while removing the properties
						CProgressDlg dlg;
						TCHAR s[MAX_PROP_STRING_LENGTH];
						LoadString(g_hResInst, IDS_SETPROPTITLE, s, MAX_PROP_STRING_LENGTH);
						dlg.SetTitle(s);
						LoadString(g_hResInst, IDS_PROPWAITCANCEL, s, MAX_PROP_STRING_LENGTH);
						dlg.SetCancelMsg(s);
						dlg.SetTime(TRUE);
						dlg.SetShowProgressBar(TRUE);
						dlg.ShowModal(m_hwnd);
						for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
						{
							dlg.SetLine(1, I->c_str(), TRUE);
							SVNProperties props = SVNProperties(CTSVNPath(I->c_str()));
							//CShellUpdater::Instance().AddPathForUpdate(CTSVNPath(I->c_str()));
							props.Remove(buf, checked);
							count++;
							dlg.SetProgress64(count, all);
							if (dlg.HasUserCancelled())
								break;
						}
						dlg.Stop();
						delete [] buf;
						// clear the edit box and reset the combobox
						ListView_SetSelectionMark(lvh, -1);
						HWND hwndCombo = GetDlgItem(hwnd, IDC_EDITNAME);
						SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM)-1, 0);
						HWND hwndEdit = GetDlgItem(hwnd, IDC_EDITVALUE);
						SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)_T(""));
						InitWorkfileView();
						return TRUE;
					}
					if (LOWORD(wParam) == IDC_ADDBUTTON)
					{
						SaveProperties();
						return TRUE;
					}
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
				case EN_CHANGE:
					if ((LOWORD(wParam) == IDC_EDITNAME)||(LOWORD(wParam) == IDC_EDITVALUE))
					{
						//determine depending on the text in the edit control if the add button should be enabled
						TCHAR * buf = new TCHAR[MAX_PROP_STRING_LENGTH];
						if ((GetDlgItemText(m_hwnd, IDC_EDITNAME, buf, MAX_PROP_STRING_LENGTH) > 0)&&(GetDlgItemText(m_hwnd, IDC_EDITVALUE, buf, MAX_PROP_STRING_LENGTH) > 0))
						{
							EnableWindow(GetDlgItem(m_hwnd, IDC_ADDBUTTON), true);
						}
						else
						{
							EnableWindow(GetDlgItem(m_hwnd, IDC_ADDBUTTON), false);
						}
						delete buf;
						return TRUE;
					} // if ((LOWORD(wParam) == IDC_EDITNAME)||(LOWORD(wParam) == IDC_EDITVALUE)) 
					break;
				case CBN_SELCHANGE:
					if (LOWORD(wParam) == IDC_EDITNAME)
					{
						SetWindowText(GetDlgItem(m_hwnd, IDC_EDITVALUE), _T(""));
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
	TCHAR timebuf[MAX_PROP_STRING_LENGTH];
	TCHAR datebuf[MAX_PROP_STRING_LENGTH];

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
			GetDateFormat(locale, DATE_SHORTDATE, &systime, NULL, datebuf, MAX_PROP_STRING_LENGTH);
		else
			GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_PROP_STRING_LENGTH);
		GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PROP_STRING_LENGTH);
		*buf = '\0';
		_tcsncat_s(buf, buflen, timebuf, MAX_PROP_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, _T(", "), MAX_PROP_STRING_LENGTH-1);
		_tcsncat_s(buf, buflen, datebuf, MAX_PROP_STRING_LENGTH-1);
	}
}

void CSVNPropertyPage::InitWorkfileView()
{
	SVNStatus svn = SVNStatus();
	TCHAR tbuf[MAX_PROP_STRING_LENGTH];
	if (filenames.size() == 1)
	{
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			TCHAR buf[MAX_PROP_STRING_LENGTH];
			__time64_t	time;
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->entry->kind == svn_node_file)
				{
					//disable the 'recursive' checkbox for files
					HWND recursivewnd = GetDlgItem(m_hwnd, IDC_RECURSIVE);
					::EnableWindow(recursivewnd, FALSE);					
				}
				if (svn.status->text_status == svn_wc_status_added)
				{
					// disable the "show log" button for added files
					HWND showloghwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
					::EnableWindow(showloghwnd, FALSE);
				}
				else
				{
					_stprintf_s(buf, MAX_PROP_STRING_LENGTH, _T("%d"), svn.status->entry->revision);
					SetDlgItemText(m_hwnd, IDC_REVISION, buf);
				}
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
					_tcsncpy_s(tbuf, MAX_PROP_STRING_LENGTH, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				}
				if (svn.status->text_status != svn_wc_status_added)
				{
					_stprintf_s(buf, MAX_PROP_STRING_LENGTH, _T("%d"), svn.status->entry->cmt_rev);
					SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
					time = (__time64_t)svn.status->entry->cmt_date/1000000L;
					Time64ToTimeString(time, buf, MAX_PROP_STRING_LENGTH);
					SetDlgItemText(m_hwnd, IDC_CDATE, buf);
				}
				if (svn.status->entry->cmt_author)
					SetDlgItemText(m_hwnd, IDC_AUTHOR, UTF8ToWide(svn.status->entry->cmt_author).c_str());
				SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
				SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, sizeof(buf)/sizeof(TCHAR), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
				time = (__time64_t)svn.status->entry->text_time/1000000L;
				Time64ToTimeString(time, buf, MAX_PROP_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);
				time = (__time64_t)svn.status->entry->prop_time/1000000L;
				Time64ToTimeString(time, buf, MAX_PROP_STRING_LENGTH);
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
				SVNProperties props = SVNProperties(CTSVNPath(filenames.front().c_str()));
				//get the handle of the listview
				HWND lvh = GetDlgItem(m_hwnd, IDC_PROPLIST);
				ListView_SetExtendedListViewStyle (lvh, LVS_EX_FULLROWSELECT);
				ListView_DeleteAllItems(lvh);
				HWND header = ListView_GetHeader(lvh);
				if (Header_GetItemCount(header)<=0)
				{
					LVCOLUMN lcol1 = {0};
					LVCOLUMN lcol2 = {0};
					lcol1.mask = LVCF_TEXT | LVCF_WIDTH;
					MAKESTRING(IDS_PROPPROPERTY);
					lcol1.pszText = stringtablebuffer;
					lcol1.cx = 30;
					ListView_InsertColumn(lvh, 0, &lcol1);
					lcol2.mask = LVCF_TEXT;
					MAKESTRING(IDS_PROPVALUE);
					lcol2.pszText = stringtablebuffer;
					ListView_InsertColumn(lvh, 1, &lcol2);
				} // if (Header_GetItemCount(header)<=0)
				stdstring stemp;
				for (int i=0; i<props.GetCount(); i++)
				{
					stdstring temp;
					LVITEM lvitem = {0};
					lvitem.mask = LVIF_TEXT;
					temp = props.GetItemName(i);
					lvitem.pszText = (LPTSTR)temp.c_str();
					if (lvitem.pszText)
					{
						lvitem.iItem = i;
						lvitem.iSubItem = 0;
						lvitem.state = 0;
						lvitem.stateMask = 0;
						lvitem.cchTextMax = (int)min(_tcslen(lvitem.pszText)+1, (size_t)INT_MAX);
						ListView_InsertItem(lvh, &lvitem);
						temp = props.GetItemValue(i);
						//treat values as normal text even if they're not
						stemp = MultibyteToWide((char *)temp.c_str());
						propmap[props.GetItemName(i)] = stemp;
						for (int ii=0; ii<(int)stemp.length(); ++ii)
						{
							if (stemp[ii] == '\n')
								stemp[ii] = ' ';
							if (stemp[ii] == '\r')
								stemp[ii] = ' ';
						}
						ListView_SetItemText(lvh, i, 1, (LPTSTR)(stemp.c_str()));
					} // if (lvitem.pszText) 
				} // for (int i=0; i<props.GetCount(); i++) 
				//now adjust the column widths
				ListView_SetColumnWidth(lvh, 0, LVSCW_AUTOSIZE_USEHEADER);
				ListView_SetColumnWidth(lvh, 1, LVSCW_AUTOSIZE_USEHEADER);
				if (svn.status->entry->lock_owner)
					SetDlgItemText(m_hwnd, IDC_LOCKOWNER, UTF8ToWide(svn.status->entry->lock_owner).c_str());
				time = (__time64_t)svn.status->entry->lock_creation_date/1000000L;
				Time64ToTimeString(time, buf, MAX_PROP_STRING_LENGTH);
				SetDlgItemText(m_hwnd, IDC_LOCKDATE, buf);
			} // if (svn.status->entry != NULL)
		} // if (svn.GetStatus(filename.c_str())>(-2)) 
	} // if (filenames.size() == 1) 
	else if (filenames.size() != 0)
	{
		//deactivate the show log button
		HWND logwnd = GetDlgItem(m_hwnd, IDC_SHOWLOG);
		::EnableWindow(logwnd, FALSE);
		//get the handle of the listview
		HWND lvh = GetDlgItem(m_hwnd, IDC_PROPLIST);
		ListView_SetExtendedListViewStyle (lvh, LVS_EX_FULLROWSELECT);
		ListView_DeleteAllItems(lvh);
		HWND header = ListView_GetHeader(lvh);
		if (Header_GetItemCount(header)<=0)
		{
			LVCOLUMN lcol1 = {0};
			LVCOLUMN lcol2 = {0};
			lcol1.mask = LVCF_TEXT | LVCF_WIDTH;
			MAKESTRING(IDS_PROPPROPERTY);
			lcol1.pszText = stringtablebuffer;
			lcol1.cx = 30;
			ListView_InsertColumn(lvh, 0, &lcol1);
			lcol2.mask = LVCF_TEXT;
			MAKESTRING(IDS_PROPVALUE);
			lcol2.pszText = stringtablebuffer;
			ListView_InsertColumn(lvh, 1, &lcol2);
		}
		if (svn.GetStatus(CTSVNPath(filenames.front().c_str()))>(-2))
		{
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
					_tcsncpy_s(tbuf, MAX_PROP_STRING_LENGTH, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
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

		//read all properties of all selected files
		//compare the properties and show _only_ those
		//which are identical for all files!
		std::vector<listproperty> proplist;
		for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
		{
			SVNProperties props = SVNProperties(CTSVNPath(I->c_str()));
			for (int i=0; i<props.GetCount(); i++)
			{
				listproperty prop;
				prop.name = props.GetItemName(i);
				prop.value = props.GetItemValue(i);
				stdstring stemp;
				stemp = MultibyteToWide((char *)prop.value.c_str());
				prop.value = stemp;
				prop.count = 1;
				BOOL found = FALSE;
				for (std::vector<listproperty>::iterator I = proplist.begin(); I != proplist.end(); ++I)
				{
					if ((I->name.compare(prop.name)==0)&&(I->value.compare(prop.value)==0))
					{
						I->count++;
						found = TRUE;
					}
				} // for (std::vector<listproperty>::iterator I = proplist.begin(); I != proplist.end(); ++I)
				if (found == FALSE)
				{
					proplist.push_back(prop);
				}
			} // for (int i=0; i<props.GetCount(); i++) 
		} // for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
		//now go through the list of properties and add all those 
		//which are identical on all files/folders
		int i = 0;
		propmap.clear();
		for (std::vector<listproperty>::iterator I = proplist.begin(); I != proplist.end(); ++I)
		{
			if (I->count == (ULONGLONG)filenames.size())
			{
				stdstring stemp;
				LVITEM lvitem = {0};
				lvitem.mask = LVIF_TEXT;
				lvitem.pszText = (LPTSTR)I->name.c_str();
				if (lvitem.pszText)
				{
					lvitem.iItem = i;
					lvitem.iSubItem = 0;
					lvitem.state = 0;
					lvitem.stateMask = 0;
					lvitem.cchTextMax = (int)min(_tcslen(lvitem.pszText)+1, (size_t)INT_MAX);
					ListView_InsertItem(lvh, &lvitem);
					propmap[I->name] = I->value;
					for (int ii=0; ii<(int)(I->value.length()); ++ii)
					{
						if (I->value[ii] == '\n')
							I->value[ii] = ' ';
						if (I->value[ii] == '\r')
							I->value[ii] = ' ';
					}
					ListView_SetItemText(lvh, i, 1, (LPTSTR)(I->value.c_str()));
					i++;
				} // if (lvitem.pszText)
			} // if (I->count == filenames.size()) 
		} // for (std::vector<listproperty>::iterator I = proplist.begin(); I != proplist.end(); ++I) 

		//now adjust the column widths
		ListView_SetColumnWidth(lvh, 0, LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(lvh, 1, LVSCW_AUTOSIZE_USEHEADER);
	} 
}

void CSVNPropertyPage::Unescape(char * psz)
{
	char * pszSource = psz;
	char * pszDest = psz;

	// under VS.NET2k5 strchr() wants this to be a non-const array :/

	static char szHex[] = "0123456789ABCDEF";

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				 *(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			char nValue = '?';
			char * pszLow = NULL;
			char * pszHigh = NULL;
			pszSource++;

			*pszSource = (char) toupper(*pszSource);
			pszHigh = strchr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (char) toupper(*pszSource);
				pszLow = strchr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (char) (((pszHigh - szHex) << 4) +
									(pszLow - szHex));
				}
			} // if (pszHigh != NULL) 
			*pszDest++ = nValue;
		} 
		else
			*pszDest++ = *pszSource;
			
		pszSource++;
	}

	*pszDest = '\0';
}

bool CSVNPropertyPage::SaveProperties()
{
	TCHAR * name = NULL;
	TCHAR * value = NULL;
	GetDlgItemTextEx(m_hwnd, IDC_EDITNAME, name);
	GetDlgItemTextEx(m_hwnd, IDC_EDITVALUE, value);
	std::string t = WideToMultibyte(value);
	HWND hCheck = GetDlgItem(m_hwnd, IDC_RECURSIVE);
	BOOL checked = (SendMessage(hCheck,(UINT) BM_GETCHECK, 0, 0) == BST_CHECKED);
	ULONGLONG all = filenames.size();
	ULONGLONG count = 0;
	CProgressDlg dlg;
	TCHAR s[MAX_PROP_STRING_LENGTH];
	LoadString(g_hResInst, IDS_SETPROPTITLE, s, MAX_PROP_STRING_LENGTH);
	dlg.SetTitle(s);
	LoadString(g_hResInst, IDS_PROPWAITCANCEL, s, MAX_PROP_STRING_LENGTH);
	dlg.SetCancelMsg(s);
	dlg.SetTime(TRUE);
	dlg.SetShowProgressBar(TRUE);
	dlg.ShowModal(m_hwnd);
	for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
	{
		dlg.SetLine(1, I->c_str(), TRUE);
		SVNProperties props = SVNProperties(CTSVNPath(I->c_str()));
		if ((!checked)&&(all == 1)&&(PathIsDirectory(I->c_str())))
		{
			if ((_tcscmp(name, _T("svn:eol-style"))==0)||
				(_tcscmp(name, _T("svn:executable"))==0)||
				(_tcscmp(name, _T("svn:keywords"))==0)||
				(_tcscmp(name, _T("svn:needs-lock"))==0)||
				(_tcscmp(name, _T("svn:mime-type"))==0))
			{
				TCHAR msg[2048];
				LoadString(g_hResInst, IDS_FILEPROPONFOLDER, msg, 2048);
				::MessageBox(m_hwnd, msg, _T("TortoiseSVN"), MB_ICONERROR);
				break;
			}
		}
		if (!props.Add(name, t.c_str(), checked))
		{
			::MessageBox(m_hwnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		count++;
		dlg.SetProgress64(count, all);
		if (dlg.HasUserCancelled())
			break;
		SVNStatus stat = SVNStatus();
		if (stat.GetStatus(CTSVNPath(I->c_str()))==(-2))
		{
			::MessageBox(m_hwnd, stat.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
			props.Remove(name);
		}
	} // for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I) 
	dlg.Stop();
	InitWorkfileView();
	EnableWindow(GetDlgItem(m_hwnd, IDC_ADDBUTTON), false);
	delete name;
	delete value;
	return true;
}
