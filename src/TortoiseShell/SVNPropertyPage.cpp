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
#include "stdafx.h"
#include "svnpropertypage.h"
#include "SVNStatus.h"
#include "SVNProperties.h"
#include "resource.h"
#include <time.h>
#include <string>
#include <Shlwapi.h>
#include <commctrl.h>
#include "ProgressDlg.h"

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
		if (svn.GetStatus(I->c_str()) == (-2))
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



STDMETHODIMP CShellExt::ReplacePage (UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
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
        SetWindowLong (hwnd, GWL_USERDATA, (LONG) sheetpage);
        sheetpage->SetHwnd(hwnd);
    } // if (uMessage == WM_INITDIALOG) 
    else
    {
        sheetpage = (CSVNPropertyPage*) GetWindowLong (hwnd, GWL_USERDATA);
    }

    if (sheetpage != 0L)
        return sheetpage->PageProc(hwnd, uMessage, wParam, lParam);
    else
        return FALSE;
}

UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
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
			COMBOBOXINFO cbInfo;
			cbInfo.cbSize = sizeof(COMBOBOXINFO);
			if (hwndCombo)
			{
				// Initialize the combobox with the default svn: properties
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:externals"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:executable"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:mime-type"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:ignore"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:keywords"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("svn:eol-style"));

				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:label"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:message"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:number"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("bugtraq:url"));

				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logtemplate"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logwidthmarker"));
				SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)_T("tsvn:logminsize"));
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

			// Send ADDTOOL messages to the tooltip control window
			ti.uId = (UINT)GetDlgItem(hwnd, IDC_EDITVALUE);
			SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
			ti.uId = (UINT)cbInfo.hwndItem;
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
				if ((code == NM_CLICK)||(code == LVN_ITEMACTIVATE))
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
				HWND hwndCombo = GetDlgItem(m_hwnd, IDC_EDITNAME);
				TCHAR * name = NULL;
				TCHAR buf[MAX_PROP_STRING_LENGTH];
				GetDlgItemTextEx(m_hwnd, IDC_EDITNAME, name);
				lpnmtdi->hinst = NULL;
				lpnmtdi->szText[0] = 0;
				lpnmtdi->lpszText = NULL;
				lpnmtdi->uFlags = NULL;
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

				SendMessage(lpnmtdi->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, nWindowWidth);
				delete [] name;
			} // if (code == TTN_GETDISPINFO) 
			SetWindowLong(m_hwnd, DWL_MSGRESULT, FALSE);
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
						ULONG all = filenames.size();
						ULONG count = 0;
						CProgressDlg dlg;
						TCHAR s[MAX_PATH];
						LoadString(g_hResInst, IDS_SETPROPTITLE, s, MAX_PATH);
						dlg.SetTitle(s);
						LoadString(g_hResInst, IDS_PROPWAITCANCEL, s, MAX_PATH);
						dlg.SetCancelMsg(s);
						dlg.SetTime(TRUE);
						dlg.SetShowProgressBar(TRUE);
						dlg.ShowModal(m_hwnd);
						for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
						{
							dlg.SetLine(1, I->c_str(), TRUE);
							SVNProperties props = SVNProperties(I->c_str());
							props.Remove(buf, checked);
							count++;
							dlg.SetProgress(count, all);
							if (dlg.HasUserCancelled())
								break;
						} // for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I) 
						dlg.Stop();
						delete [] buf;
						InitWorkfileView();
						return TRUE;
					}
					if (LOWORD(wParam) == IDC_ADDBUTTON)
					{
						TCHAR * name = NULL;
						TCHAR * value = NULL;
						GetDlgItemTextEx(m_hwnd, IDC_EDITNAME, name);
						GetDlgItemTextEx(m_hwnd, IDC_EDITVALUE, value);
#ifdef UNICODE
						std::string t = WideToMultibyte(value);
#else
						std::string t = std::string(value);
#endif
						HWND hCheck = GetDlgItem(m_hwnd, IDC_RECURSIVE);
						BOOL checked = (SendMessage(hCheck,(UINT) BM_GETCHECK, 0, 0) == BST_CHECKED);
						ULONG all = filenames.size();
						ULONG count = 0;
						CProgressDlg dlg;
						TCHAR s[MAX_PATH];
						LoadString(g_hResInst, IDS_SETPROPTITLE, s, MAX_PATH);
						dlg.SetTitle(s);
						LoadString(g_hResInst, IDS_PROPWAITCANCEL, s, MAX_PATH);
						dlg.SetCancelMsg(s);
						dlg.SetTime(TRUE);
						dlg.SetShowProgressBar(TRUE);
						dlg.ShowModal(m_hwnd);
						for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
						{
							dlg.SetLine(1, I->c_str(), TRUE);
							SVNProperties props = SVNProperties(I->c_str());
							if (!props.Add(name, t.c_str(), checked))
							{
								::MessageBox(m_hwnd, props.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
							}
							count++;
							dlg.SetProgress(count, all);
							if (dlg.HasUserCancelled())
								break;
							SVNStatus stat = SVNStatus();
							if (stat.GetStatus(I->c_str())==(-2))
							{
								::MessageBox(m_hwnd, stat.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
								props.Remove(name);
							}
						} // for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I) 
						dlg.Stop();
						InitWorkfileView();
						delete name;
						delete value;
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
						CreateProcess(tortoiseProcPath, const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process);
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
			} // switch (HIWORD(wParam)) 
	} // switch (uMessage) 
	return FALSE;
}
void CSVNPropertyPage::Time64ToTimeString(__time64_t time, TCHAR * buf)
{
	struct tm * newtime;
	SYSTEMTIME systime;
	TCHAR timebuf[MAX_PROP_STRING_LENGTH];
	TCHAR datebuf[MAX_PROP_STRING_LENGTH];

	LCID locale = (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
	locale = MAKELCID(locale, SORT_DEFAULT);

	newtime = _localtime64(&time);

	systime.wDay = newtime->tm_mday;
	systime.wDayOfWeek = newtime->tm_wday;
	systime.wHour = newtime->tm_hour;
	systime.wMilliseconds = 0;
	systime.wMinute = newtime->tm_min;
	systime.wMonth = newtime->tm_mon+1;
	systime.wSecond = newtime->tm_sec;
	systime.wYear = newtime->tm_year+1900;
	GetDateFormat(locale, DATE_LONGDATE, &systime, NULL, datebuf, MAX_PROP_STRING_LENGTH);
	GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PROP_STRING_LENGTH);
	*buf = '\0';
	_tcsncat(buf, timebuf, MAX_PROP_STRING_LENGTH-1);
	_tcsncat(buf, _T(", "), MAX_PROP_STRING_LENGTH-1);
	_tcsncat(buf, datebuf, MAX_PROP_STRING_LENGTH-1);
}

void CSVNPropertyPage::InitWorkfileView()
{
	SVNStatus svn = SVNStatus();
	TCHAR tbuf[MAX_PROP_STRING_LENGTH];
	if (filenames.size() == 1)
	{
		if (svn.GetStatus(filenames.front().c_str())>(-2))
		{
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				TCHAR buf[MAX_PROP_STRING_LENGTH];
				__time64_t	time;
				int datelen = 0;
				_stprintf(buf, _T("%d"), svn.status->entry->revision);
				SetDlgItemText(m_hwnd, IDC_REVISION, buf);
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
#ifdef UNICODE
					_tcsncpy(tbuf, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
#else
					_tcsncpy(tbuf, svn.status->entry->url, 4095);
#endif
					//Unescape(tbuf);
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				} // if (svn.status->entry->url) 
				_stprintf(buf, _T("%d"), svn.status->entry->cmt_rev);
				SetDlgItemText(m_hwnd, IDC_CREVISION, buf);
				time = (__time64_t)svn.status->entry->cmt_date/1000000L;
				Time64ToTimeString(time, buf);
				SetDlgItemText(m_hwnd, IDC_CDATE, buf);
				if (svn.status->entry->cmt_author)
#ifdef UNICODE
					SetDlgItemText(m_hwnd, IDC_AUTHOR, UTF8ToWide(svn.status->entry->cmt_author).c_str());
#else
					SetDlgItemText(m_hwnd, IDC_AUTHOR, svn.status->entry->cmt_author);
#endif
				SVNStatus::GetStatusString(g_hResInst, svn.status->text_status, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
				SVNStatus::GetStatusString(g_hResInst, svn.status->prop_status, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
				SetDlgItemText(m_hwnd, IDC_PROPSTATUS, buf);
				time = (__time64_t)svn.status->entry->text_time/1000000L;
				Time64ToTimeString(time, buf);
				SetDlgItemText(m_hwnd, IDC_TEXTDATE, buf);
				time = (__time64_t)svn.status->entry->prop_time/1000000L;
				Time64ToTimeString(time, buf);
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
				SVNProperties props = SVNProperties(filenames.front().c_str());
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
						lvitem.cchTextMax = _tcslen(lvitem.pszText)+1;
						ListView_InsertItem(lvh, &lvitem);
						temp = props.GetItemValue(i);
						//treat values as normal text even if they're not
#ifdef UNICODE
						stemp = MultibyteToWide((char *)temp.c_str());
#else
						stemp = temp;
#endif
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
		} // if (Header_GetItemCount(header)<=0)
		if (svn.GetStatus(filenames.front().c_str())>(-2))
		{
			if (svn.status->entry != NULL)
			{
				LoadLangDll();
				int datelen = 0;
				if (svn.status->entry->url)
				{
					Unescape((char*)svn.status->entry->url);
#ifdef UNICODE
					_tcsncpy(tbuf, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
#else
					_tcsncpy(tbuf, svn.status->entry->url, 4095);
#endif
					//Unescape(tbuf);
					TCHAR * ptr = _tcsrchr(tbuf, '/');
					if (ptr != 0)
					{
						*ptr = 0;
					}
					SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
				} // if (svn.status->entry->url) 
				SetDlgItemText(m_hwnd, IDC_LOCKED, _T(""));
			} // if (svn.status->entry != NULL)
		} // if (svn.GetStatus(filenames.front().c_str())>(-2))

		//read all properties of all selected files
		//compare the properties and show _only_ those
		//which are identical for all files!
		std::vector<listproperty> proplist;
		for (std::vector<stdstring>::iterator I = filenames.begin(); I != filenames.end(); ++I)
		{
			SVNProperties props = SVNProperties(I->c_str());
			for (int i=0; i<props.GetCount(); i++)
			{
				listproperty prop;
				prop.name = props.GetItemName(i);
				prop.value = props.GetItemValue(i);
				stdstring stemp;
#ifdef UNICODE
				stemp = MultibyteToWide((char *)prop.value.c_str());
#else
				stemp = prop.value;
#endif
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
			if (I->count == filenames.size())
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
					lvitem.cchTextMax = _tcslen(lvitem.pszText)+1;
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

	static const char szHex[] = "0123456789ABCDEF";

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