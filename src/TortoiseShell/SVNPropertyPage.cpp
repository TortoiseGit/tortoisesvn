#include "stdafx.h"
#include "svnpropertypage.h"
#include "SVNStatus.h"
#include "SVNProperties.h"
#include "resource.h"
#include <time.h>
#include <string>
#include <Shlwapi.h>
#include <commctrl.h>

#define MAX_PROP_STRING_LENGTH		4096			//should be big enough

// Nonmember function prototypes
BOOL CALLBACK PageProc (HWND, UINT, WPARAM, LPARAM);
UINT CALLBACK PropPageCallbackProc ( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );

// CShellExt member functions (needed for IShellPropSheetExt)
STDMETHODIMP CShellExt::AddPages (LPFNADDPROPSHEETPAGE lpfnAddPage,
                                  LPARAM lParam)
{
	SVNStatus svn = SVNStatus();
	if ((!isOnlyOneItemSelected)||(svn.GetStatus(files_.front().c_str()) == (-2)))
		return NOERROR;			// file/directory not under version control

	if (svn.status->entry == NULL)
		return NOERROR;

    PROPSHEETPAGE psp;
	ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
	HPROPSHEETPAGE hPage;
    CSVNPropertyPage *sheetpage = new CSVNPropertyPage(files_.front());

    psp.dwSize = sizeof (psp);
    psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE | PSP_USEICONID | PSP_USECALLBACK | PSP_DLGINDIRECT;	
	psp.hInstance = g_hmodThisDll;
	psp.pszTemplate = NULL;
	psp.pResource = (PROPSHEETPAGE_RESOURCE)LockResource(LoadResource(g_hmodThisDll, FindResourceEx(g_hmodThisDll, RT_DIALOG, MAKEINTRESOURCE(IDD_PROPPAGE), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)))));
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
	}


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
    }
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
    }
    return 1;
}

// *********************** CSVNPropertyPage *************************

CSVNPropertyPage::CSVNPropertyPage(const stdstring &newFilename)
	:filename(newFilename)
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
	switch (uMessage)
	{
		case WM_INITDIALOG:
			InitWorkfileView();
			return TRUE;    

		case WM_NOTIFY:
			//
			// Respond to notifications.
			//    
			if (wParam == IDC_PROPLIST)
			{
				LPNMHDR point = (LPNMHDR)lParam;
				int code = point->code;
				if ((code == LVN_ITEMCHANGED)||(code == LVN_ITEMACTIVATE))
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
						//ListView_GetItemText(lvh, sel, 0, buf, MAX_PROP_STRING_LENGTH);
						ListView_GetItemTextEx(lvh, sel, 0, buf);
						SetDlgItemText(m_hwnd, IDC_EDITNAME, buf);
						//ListView_GetItemText(lvh, sel, 1, buf, MAX_PROP_STRING_LENGTH);
						ListView_GetItemTextEx(lvh, sel, 1, buf);
						SetDlgItemText(m_hwnd, IDC_EDITVALUE, buf);
						delete [] buf;
					}
					else
					{
						SetDlgItemText(m_hwnd, IDC_EDITNAME, _T(""));
						SetDlgItemText(m_hwnd, IDC_EDITVALUE, _T(""));
					}
				}
			}
			SetWindowLong(m_hwnd, DWL_MSGRESULT, FALSE);
			return TRUE;        

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
						//ListView_GetItemText(lvh, sel, 0, buf, MAX_PROP_STRING_LENGTH);
						ListView_GetItemTextEx(lvh, sel, 0, buf);
						SVNProperties props = SVNProperties(filename.c_str());
						props.Remove(buf);
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
						SVNProperties props = SVNProperties(filename.c_str());
#ifdef UNICODE
						std::string t = WideToMultibyte(value);
#else
						std::string t = std::string(value);
#endif
						props.Add(name, t.c_str());
						SVNStatus stat = SVNStatus();
						if (stat.GetStatus(filename.c_str())==(-2))
						{
							::MessageBox(m_hwnd, stat.GetLastErrorMsg().c_str(), _T("TortoiseSVN"), MB_ICONERROR);
							props.Remove(name);
						}
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
						svnCmd += filename.c_str();
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
					}
			}
	}
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
	GetDateFormat(locale, 0, &systime, NULL, datebuf, MAX_PROP_STRING_LENGTH);
	GetTimeFormat(locale, 0, &systime, NULL, timebuf, MAX_PROP_STRING_LENGTH);
	*buf = '\0';
	_tcsncat(buf, timebuf, MAX_PROP_STRING_LENGTH);
	_tcsncat(buf, _T(", "), MAX_PROP_STRING_LENGTH);
	_tcsncat(buf, datebuf, MAX_PROP_STRING_LENGTH);
}

void CSVNPropertyPage::InitWorkfileView()
{
	SVNStatus svn = SVNStatus();
	TCHAR tbuf[MAX_PROP_STRING_LENGTH];
	if (svn.GetStatus(filename.c_str())>(-2))
	{
		if (svn.status->entry != NULL)
		{
			TCHAR buf[MAX_PROP_STRING_LENGTH];
			__time64_t	time;
			int datelen = 0;
			_stprintf(buf, _T("%d"), svn.status->entry->revision);
			SetDlgItemText(m_hwnd, IDC_REVISION, buf);
			if (svn.status->entry->url)
			{
#ifdef UNICODE
				_tcsncpy(tbuf, UTF8ToWide(svn.status->entry->url).c_str(), 4095);
#else
				_tcsncpy(tbuf, svn.status->entry->url, 4095);
#endif
				Unescape(tbuf);
				SetDlgItemText(m_hwnd, IDC_REPOURL, tbuf);
			}
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
			SVNStatus::GetStatusString(g_hmodThisDll, svn.status->text_status, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
			SetDlgItemText(m_hwnd, IDC_TEXTSTATUS, buf);
			SVNStatus::GetStatusString(g_hmodThisDll, svn.status->prop_status, buf, sizeof(buf), (WORD)CRegStdWORD(_T("Software\\TortoiseSVN\\LanguageID"), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)));
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
			SVNProperties props = SVNProperties(filename.c_str());
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
					ListView_SetItemText(lvh, i, 1, (LPTSTR)(stemp.c_str()));
				}
			}
			//now adjust the column widths
			ListView_SetColumnWidth(lvh, 0, LVSCW_AUTOSIZE_USEHEADER);
			ListView_SetColumnWidth(lvh, 1, LVSCW_AUTOSIZE_USEHEADER);
		}
	}
}

void CSVNPropertyPage::Unescape(LPTSTR psz)
{
	LPTSTR pszSource = psz;
	LPTSTR pszDest = psz;

	static const TCHAR szHex[] = _T("0123456789ABCDEF");

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '+')
			*pszDest++ = ' ';
		else if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				 *(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			TCHAR nValue = '?';
			LPCTSTR pszLow = NULL;
			LPCTSTR pszHigh = NULL;
			pszSource++;

			*pszSource = (TCHAR) _totupper(*pszSource);
			pszHigh = _tcschr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (TCHAR) _totupper(*pszSource);
				pszLow = _tcschr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (TCHAR) (((pszHigh - szHex) << 4) +
									(pszLow - szHex));
				}
			}
			*pszDest++ = nValue;
		}
		else
			*pszDest++ = *pszSource;
			
		pszSource++;
	}

	*pszDest = '\0';
}