// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Stefan Kueng

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
#include "TortoiseProc.h"
#include "messagebox.h"
#include "cursor.h"
#include "UnicodeUtils.h"
#include "MergeDlg.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include ".\logdlg.h"

// CLogDlg dialog

#define SHORTLOGMESSAGEWIDTH 500

IMPLEMENT_DYNAMIC(CLogDlg, CResizableDialog)
CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CLogDlg::IDD, pParent)
{
	m_pFindDialog = NULL;
	m_bCancelled = FALSE;
	m_bShowedAll = FALSE;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pNotifyWindow = NULL;
	m_bThreadRunning = FALSE;
	m_bGotAllPressed = FALSE;
}

CLogDlg::~CLogDlg()
{
	for (int i=0; i<m_templist.GetCount(); i++)
	{
		DeleteFile(m_templist.GetAt(i));
	}
	m_templist.RemoveAll();
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGLIST, m_LogList);
	DDX_Control(pDX, IDC_LOGMSG, m_LogMsgCtrl);
	DDX_Control(pDX, IDC_PROGRESS, m_LogProgress);
}

const UINT CLogDlg::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CLogDlg, CResizableDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_LOGLIST, OnNMClickLoglist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LOGLIST, OnLvnKeydownLoglist)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage) 
	ON_BN_CLICKED(IDC_GETALL, OnBnClickedGetall)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGMSG, OnNMDblclkLogmsg)
	ON_NOTIFY(NM_DBLCLK, IDC_LOGLIST, OnNMDblclkLoglist)
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOGLIST, OnLvnItemchangedLoglist)
	ON_NOTIFY(EN_LINK, IDC_MSGVIEW, OnEnLinkMsgview)
END_MESSAGE_MAP()



void CLogDlg::SetParams(CString path, long startrev /* = 0 */, long endrev /* = -1 */, BOOL bStrict /* = FALSE */)
{
	m_path = path;
	m_startrev = startrev;
	m_endrev = endrev;
	m_hasWC = !PathIsURL(path);
	m_bStrict = bStrict;
}

void CLogDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CResizableDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLogDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CLogDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	LOGFONT LogFont;
	LogFont.lfHeight         = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LogFontSize"), 8), GetDeviceCaps(this->GetDC()->m_hDC, LOGPIXELSY), 72);
	LogFont.lfWidth          = 0;
	LogFont.lfEscapement     = 0;
	LogFont.lfOrientation    = 0;
	LogFont.lfWeight         = 400;
	LogFont.lfItalic         = 0;
	LogFont.lfUnderline      = 0;
	LogFont.lfStrikeOut      = 0;
	LogFont.lfCharSet        = DEFAULT_CHARSET;
	LogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	LogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality        = DRAFT_QUALITY;
	LogFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	_tcscpy(LogFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseSVN\\LogFontName"), _T("Courier New")));
	m_logFont.CreateFontIndirect(&LogFont);
	GetDlgItem(IDC_MSGVIEW)->SetFont(&m_logFont);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_AUTOURLDETECT, TRUE, NULL);
	GetDlgItem(IDC_MSGVIEW)->SendMessage(EM_SETEVENTMASK, NULL, ENM_LINK);
	m_LogList.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

	m_LogList.DeleteAllItems();
	int c = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		m_LogList.DeleteColumn(c--);
	CString temp;
	temp.LoadString(IDS_LOG_REVISION);
	m_LogList.InsertColumn(0, temp);
	temp.LoadString(IDS_LOG_AUTHOR);
	m_LogList.InsertColumn(1, temp);
	temp.LoadString(IDS_LOG_DATE);
	m_LogList.InsertColumn(2, temp);
	temp.LoadString(IDS_LOG_MESSAGE);
	m_LogList.InsertColumn(3, temp);
	m_arLogMessages.SetSize(0,100);
	m_arRevs.SetSize(0,100);

	m_LogList.SetRedraw(false);
	m_LogMsgCtrl.InsertColumn(0, _T("Message"));
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}

	if (m_hasWC)
	{
		m_BugtraqInfo.ReadProps(m_path);
	}
	//first start a thread to obtain the log messages without
	//blocking the dialog
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &LogThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);

	m_logcounter = 0;
	m_sMessageBuf.Preallocate(100000);

	CString sTitle;
	GetWindowText(sTitle);
	SetWindowText(sTitle + _T(" - ") + m_path.Mid(m_path.ReverseFind('\\')+1));

	AddAnchor(IDC_LOGLIST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_MSGVIEW, TOP_LEFT, MIDDLE_RIGHT);
	AddAnchor(IDC_LOGMSG, MIDDLE_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_GETALL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);
	this->hWnd = this->m_hWnd;
	EnableSaveRestore(_T("LogDlg"));
	CenterWindow(CWnd::FromHandle(hWndExplorer));
	GetDlgItem(IDC_LOGLIST)->SetFocus();
	return FALSE;
}

void CLogDlg::FillLogMessageCtrl(const CString& msg, const CString& paths)
{
	CWnd * pMsgView = GetDlgItem(IDC_MSGVIEW);
	pMsgView->SetWindowText(msg);
	m_BugtraqInfo.FindBugID(msg, pMsgView);

	m_LogMsgCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );
	m_LogMsgCtrl.DeleteAllItems();
	m_LogMsgCtrl.SetRedraw(FALSE);
	int line = 0;
	CString temp = _T("");
	int curpos = 0;
	int curposold = 0;
	CString linestr;
	while (paths.Find('\r', curpos)>=0)
	{
		curposold = curpos;
		curpos = paths.Find('\r', curposold);
		linestr = paths.Mid(curposold, curpos-curposold);
		linestr.Trim(_T("\n\r"));
		m_LogMsgCtrl.InsertItem(line++, linestr);
		curpos++;
	} // while (msg.Find('\n', curpos)>=0) 
	linestr = paths.Mid(curpos);
	linestr.Trim(_T("\n\r"));
	m_LogMsgCtrl.InsertItem(line++, linestr);

	m_LogMsgCtrl.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_LogMsgCtrl.SetRedraw();
}

void CLogDlg::OnBnClickedGetall()
{
	if ((m_bStrict)&&(m_bGotAllPressed == FALSE))
	{
		UINT ret = 0;
		ret = CMessageBox::Show(this->m_hWnd, IDS_LOG_STRICTQUESTION, IDS_APPNAME, MB_YESNOCANCEL | MB_ICONQUESTION);
		if (ret == IDCANCEL)
			return;
		if (ret == IDNO)
		{
			m_bStrict = FALSE;
			m_bShowedAll = TRUE;
			GetDlgItem(IDC_GETALL)->ShowWindow(SW_HIDE);
		}
	}
	else
	{
		m_bStrict = FALSE;
		m_bShowedAll = TRUE;
		GetDlgItem(IDC_GETALL)->ShowWindow(SW_HIDE);
	}
	m_LogList.DeleteAllItems();
	m_arLogMessages.RemoveAll();
	m_arLogPaths.RemoveAll();
	m_arRevs.RemoveAll();
	m_logcounter = 0;
	m_endrev = 1;
	m_startrev = -1;
	m_bCancelled = FALSE;
	m_bGotAllPressed = TRUE;
	DWORD dwThreadId;
	if ((m_hThread = CreateThread(NULL, 0, &LogThread, this, 0, &dwThreadId))==0)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	GetDlgItem(IDC_LOGLIST)->UpdateData(FALSE);
}

BOOL CLogDlg::Cancel()
{
	return m_bCancelled;
}

void CLogDlg::OnCancel()
{
	CString temp, temp2;
	GetDlgItem(IDOK)->GetWindowText(temp);
	temp2.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(temp2)==0)
	{
		m_bCancelled = TRUE;
		return;
	}
	__super::OnCancel();
}

BOOL CLogDlg::Log(LONG rev, const CString& author, const CString& date, const CString& message, const CString& cpaths)
{
	int line = 0;
	CString temp;
	m_LogList.SetRedraw(FALSE);
	m_logcounter += 1;
	if (m_startrev == -1)
		m_startrev = rev;
	m_LogProgress.SetPos(m_startrev-rev+m_endrev);
	int count = m_LogList.GetItemCount();
	temp.Format(_T("%d"),rev);
	m_LogList.InsertItem(count, temp);
	m_LogList.SetItemText(count, 1, author);
	m_LogList.SetItemText(count, 2, date);

	// Add as many characters from the log message to the list control
	// so it has a fixed width. If the log message is longer than
	// this predefined fixed with, add "..." as an indication.
	CString sShortMessage = message;
	// Remove newlines 'cause those are not shown nicely in the listcontrol
	sShortMessage.Replace('\n', ' ');
	sShortMessage.Replace(_T("\r"), _T(""));
	CString sShorterMessage = message;
	sShorterMessage.Replace(_T("\r"), _T(""));
	if (sShorterMessage.Find(_T("\n\n"))>=0)
		sShortMessage = sShortMessage.Left(sShorterMessage.Find(_T("\n\n")));
	if (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH)
	{
		// Make an initial guess on how many chars fit into the fixed width
		int nPix = m_LogList.GetStringWidth(sShortMessage);
		int nAvgCharWidth = nPix / sShortMessage.GetLength();
		sShortMessage = sShortMessage.Left(SHORTLOGMESSAGEWIDTH / nAvgCharWidth + 5);
		sShortMessage += _T("...");
	} // if (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH) 
	while (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH)
	{
		sShortMessage = sShortMessage.Left(sShortMessage.GetLength()-4);
		sShortMessage += _T("...");
	} // while (m_LogList.GetStringWidth(sShortMessage) > SHORTLOGMESSAGEWIDTH) 
	m_LogList.SetItemText(count, 3, sShortMessage);

	//split multiline logentries and concatenate them
	//again but this time with \r\n as line separators
	//so that the edit control recognizes them
	try
	{
		temp = "";
		if (message.GetLength()>0)
		{
			m_sMessageBuf = message;
			m_sMessageBuf.Replace(_T("\n\r"), _T("\n"));
			m_sMessageBuf.Replace(_T("\r\n"), _T("\n"));
			int pos = 0;
			if (m_sMessageBuf.Right(1).Compare(_T("\n"))==0)
				m_sMessageBuf = m_sMessageBuf.Left(m_sMessageBuf.GetLength()-1);
		} // if (message.GetLength()>0)
		m_arLogMessages.Add(m_sMessageBuf);
		m_arLogPaths.Add(cpaths);
		m_arRevs.Add(rev);
	}
	catch (CException * e)
	{
		::MessageBox(NULL, _T("not enough memory!"), _T("TortoiseSVN"), MB_ICONERROR);
		e->Delete();
		m_bCancelled = TRUE;
	}
	m_LogList.SetRedraw();
	m_bGotRevisions = TRUE;
	return TRUE;
}

//this is the thread function which calls the subversion function
DWORD WINAPI LogThread(LPVOID pVoid)
{
	CLogDlg*	pDlg;
	pDlg = (CLogDlg*)pVoid;
	pDlg->m_bThreadRunning = TRUE;

	// to make gettext happy
	SetThreadLocale(CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033));

	CString temp;
	temp.LoadString(IDS_MSGBOX_CANCEL);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
	long r = pDlg->GetHEADRevision(pDlg->m_path);
	if (pDlg->m_startrev == -1)
		pDlg->m_startrev = r;
	if (pDlg->m_endrev < (-5) || pDlg->m_bStrict)
	{
		if ((r != (-2))&&(pDlg->m_endrev < (-5)))
		{
			pDlg->m_endrev = pDlg->m_startrev + pDlg->m_endrev;
		} // if (r != (-2))
		if (pDlg->m_endrev <= 0)
		{
			pDlg->m_endrev = 1;
			if (!pDlg->m_bStrict)
				pDlg->m_bShowedAll = TRUE;
		}
	} // if (pDlg->m_endrev < (-5))
	else
	{
		pDlg->m_bShowedAll = TRUE;
	}
	//disable the "Get All" button while we're receiving
	//log messages.
	pDlg->GetDlgItem(IDC_GETALL)->EnableWindow(FALSE);
	pDlg->m_LogProgress.SetRange32(pDlg->m_endrev, pDlg->m_startrev);
	pDlg->m_LogProgress.SetPos(0);
	pDlg->GetDlgItem(IDC_PROGRESS)->ShowWindow(TRUE);
	pDlg->m_bGotRevisions = FALSE;
	while ((pDlg->m_bCancelled == FALSE)&&(pDlg->m_bGotRevisions == FALSE))
	{
		if (!pDlg->ReceiveLog(pDlg->m_path, pDlg->m_startrev, pDlg->m_endrev, true, pDlg->m_bStrict))
		{
			CMessageBox::Show(pDlg->m_hWnd, pDlg->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			break;
		}
		if (pDlg->m_endrev <= 1)
			break;
		pDlg->m_endrev -= (LONG)(DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
		if (pDlg->m_endrev <= 0)
		{
			pDlg->m_endrev = 1;
		}
	}

	pDlg->m_LogList.SetRedraw(false);
	int mincol = 0;
	int maxcol = ((CHeaderCtrl*)(pDlg->m_LogList.GetDlgItem(0)))->GetItemCount()-1;
	int col;
	for (col = mincol; col <= maxcol; col++)
	{
		pDlg->m_LogList.SetColumnWidth(col,LVSCW_AUTOSIZE_USEHEADER);
	}
	pDlg->m_LogList.SetRedraw(true);

	temp.LoadString(IDS_MSGBOX_OK);
	pDlg->GetDlgItem(IDOK)->SetWindowText(temp);
	if (!pDlg->m_bShowedAll)
		pDlg->GetDlgItem(IDC_GETALL)->EnableWindow(TRUE);
	pDlg->GetDlgItem(IDC_PROGRESS)->ShowWindow(FALSE);
	pDlg->m_bCancelled = TRUE;
	pDlg->m_bThreadRunning = FALSE;
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return 0;
}

void CLogDlg::OnNMClickLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
		FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex), m_arLogPaths.GetAt(selIndex));
		this->m_nSearchIndex = selIndex;
		UpdateData(FALSE);
	}
	else
	{
		//m_sLogMsgCtrl = _T("");
		FillLogMessageCtrl(_T(""), _T(""));
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CLogDlg::OnLvnKeydownLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex >= 0)
	{
		//check which key was pressed
		//this is necessary because we get here BEFORE
		//the selection actually changes, so we have to
		//adjust the selected index
		if ((pLVKeyDow->wVKey == VK_UP)||(pLVKeyDow->wVKey == VK_DOWN))
		{
			if (pLVKeyDow->wVKey == VK_UP)
			{
				if (selIndex > 0)
					selIndex--;
			}
			if (pLVKeyDow->wVKey == VK_DOWN)
			{
				selIndex++;		
				if (selIndex >= m_LogList.GetItemCount())
					selIndex = m_LogList.GetItemCount()-1;
			}
			//m_sLogMsgCtrl = m_arLogMessages.GetAt(selIndex);
			this->m_nSearchIndex = selIndex;
			FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex), m_arLogPaths.GetAt(selIndex));
			UpdateData(FALSE);
		}
		if (pLVKeyDow->wVKey == 'C')
		{
			if (GetKeyState(VK_CONTROL)&0x8000)
			{
				//Ctrl-C -> copy to clipboard
				CStringA sClipdata;
				POSITION pos = m_LogList.GetFirstSelectedItemPosition();
				if (pos != NULL)
				{
					CString sRev;
					sRev.LoadString(IDS_LOG_REVISION);
					CString sAuthor;
					sAuthor.LoadString(IDS_LOG_AUTHOR);
					CString sDate;
					sDate.LoadString(IDS_LOG_DATE);
					CString sMessage;
					sMessage.LoadString(IDS_LOG_MESSAGE);
					while (pos)
					{
						int nItem = m_LogList.GetNextSelectedItem(pos);
						CString sLogCopyText;
						sLogCopyText.Format(_T("%s: %d\n%s: %s\n%s: %s\n%s:\n%s\n----\n%s\n\n"),
							sRev, m_arRevs.GetAt(nItem),
							sAuthor, m_LogList.GetItemText(nItem, 1),
							sDate, m_LogList.GetItemText(nItem, 2),
							sMessage, m_arLogMessages.GetAt(nItem),
							m_arLogPaths.GetAt(nItem));
						sClipdata +=  CStringA(sLogCopyText);
					}
					if (OpenClipboard())
					{
						EmptyClipboard();
						HGLOBAL hClipboardData;
						hClipboardData = GlobalAlloc(GMEM_DDESHARE, sClipdata.GetLength()+1);
						char * pchData;
						pchData = (char*)GlobalLock(hClipboardData);
						strcpy(pchData, (LPCSTR)sClipdata);
						GlobalUnlock(hClipboardData);
						SetClipboardData(CF_TEXT,hClipboardData);
						CloseClipboard();
					} // if (OpenClipboard()) 
				}
			}
		}
	}
	else
	{
		if (m_arLogMessages.GetCount()>0)
			FillLogMessageCtrl(m_arLogMessages.GetAt(0), m_arLogPaths.GetAt(0));
		UpdateData(FALSE);
	}
	*pResult = 0;
}

void CLogDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
//#region m_LogList
	if (pWnd == &m_LogList)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			m_LogList.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_LogList.ClientToScreen(&rect);
			point.x = rect.left + rect.Width()/2;
			point.y = rect.top + rect.Height()/2;
		}
		m_nSearchIndex = selIndex;

		if (selIndex >= 0)
		{
			//entry is selected, now show the popup menu
			CMenu popup;
			if (popup.CreatePopupMenu())
			{
				CString temp;
				if (m_LogList.GetSelectedCount() == 1)
				{
					if (!PathIsDirectory(m_path))
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						if (m_hasWC)
						{
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
							popup.SetDefaultItem(ID_COMPARE, FALSE);
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
						popup.AppendMenu(MF_SEPARATOR, NULL);
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_SAVEAS, temp);
					}
					else
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARE);
						if (m_hasWC)
						{
							popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARE, temp);
							popup.SetDefaultItem(ID_COMPARE, FALSE);
						}
						temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF1, temp);
						popup.AppendMenu(MF_SEPARATOR, NULL);
						temp.LoadString(IDS_LOG_BROWSEREPO);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REPOBROWSE, temp);
					}
					temp.LoadString(IDS_LOG_POPUP_COPY);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COPY, temp);
					temp.LoadString(IDS_LOG_POPUP_UPDATE);
					if (m_hasWC)
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_UPDATE, temp);
					temp.LoadString(IDS_LOG_POPUP_REVERTREV);
					if (m_hasWC)
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_REVERTREV, temp);
					temp.LoadString(IDS_REPOBROWSE_SHOWPROP);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_POPPROPS, temp);			// "Show Properties"
				}
				else if (m_LogList.GetSelectedCount() == 2)
				{
					if (!PathIsDirectory(m_path))
					{
						temp.LoadString(IDS_LOG_POPUP_COMPARETWO);
						popup.AppendMenu(MF_STRING | MF_ENABLED, ID_COMPARETWO, temp);
					}
					temp.LoadString(IDS_LOG_POPUP_GNUDIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_GNUDIFF2, temp);
				}
				popup.AppendMenu(MF_SEPARATOR, NULL);
				temp.LoadString(IDS_LOG_POPUP_FIND);
				popup.AppendMenu(MF_STRING | MF_ENABLED, ID_FINDENTRY, temp);

				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				GetDlgItem(IDOK)->EnableWindow(FALSE);
				this->m_app = &theApp;
				theApp.DoWaitCursor(1);
				switch (cmd)
				{
				case ID_GNUDIFF1:
					{
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);
						this->m_bCancelled = FALSE;
						CString tempfile = CUtils::GetTempFile();
						tempfile += _T(".diff");
						m_templist.Add(tempfile);
						if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), rev-1, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;		//exit
						}
						else
						{
							if (CUtils::CheckForEmptyDiff(tempfile))
							{
								CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
								break;
							}
							CUtils::StartDiffViewer(tempfile);
						}
					}
					break;
				case ID_GNUDIFF2:
					{
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
						long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
						this->m_bCancelled = FALSE;
						CString tempfile = CUtils::GetTempFile();
						tempfile += _T(".diff");
						m_templist.Add(tempfile);
						if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), rev2, rev1, TRUE, FALSE, TRUE, _T(""), tempfile))
						{
							CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							break;		//exit
						} // if (!Diff(m_path, rev2, m_path, rev1, TRUE, FALSE, TRUE, _T(""), tempfile))
						else
						{
							if (CUtils::CheckForEmptyDiff(tempfile))
							{
								CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
								break;
							}
							CUtils::StartDiffViewer(tempfile);
						}
					}
					break;
				case ID_REVERTREV:
					{
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);
						if (CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERT_CONFIRM, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES)
						{
							CString url = this->GetURLFromPath(m_path);
							if (url.IsEmpty())
							{
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, m_path);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								TRACE(_T("could not retrieve the URL of the folder!\n"));
								break;		//exit
							} // if ((rev == (-2))||(status.status->entry == NULL))
							else
							{
								CSVNProgressDlg dlg;
								dlg.SetParams(Enum_Merge, false, m_path, url, url, rev);		//use the message as the second url
								dlg.m_RevisionEnd = rev-1;
								dlg.DoModal();
							}
						} // if (CMessageBox::Show(this->m_hWnd, IDS_LOG_REVERT_CONFIRM, IDS_APPNAME, MB_ICONQUESTION) == IDOK) 
					}
					break;
				case ID_COPY:
					{
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);
						CCopyDlg dlg;
						CString url = GetURLFromPath(m_path);
						if (url.IsEmpty())
						{
							CString temp;
							temp.Format(IDS_ERR_NOURLOFFILE, m_path);
							CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
							TRACE(_T("could not retrieve the URL of the folder!\n"));
							break;		//exit
						} // if (status.status->entry == NULL) 
						else
						{
							dlg.m_URL = url;
							dlg.m_path = m_path;
							if (dlg.DoModal() == IDOK)
							{
								SVN svn;
								if (!svn.Copy(url, dlg.m_URL, rev, dlg.m_sLogMessage))
								{
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
								else
								{
									CMessageBox::Show(this->m_hWnd, IDS_LOG_COPY_SUCCESS, IDS_APPNAME, MB_ICONINFORMATION);
								}
							} // if (dlg.DoModal() == IDOK) 
						}
					} 
					break;
				case ID_COMPARE:
					{
						//user clicked on the menu item "compare with working copy"
						//now first get the revision which is selected
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);
						if ((PathIsDirectory(m_path))||(!m_hasWC))
						{
							this->m_bCancelled = FALSE;
							CString tempfile = CUtils::GetTempFile();
							tempfile += _T(".diff");
							m_templist.Add(tempfile);
							if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), SVNRev::REV_WC, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
							{
								CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								break;		//exit
							} 
							else
							{
								if (CUtils::CheckForEmptyDiff(tempfile))
								{
									CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
									break;
								}
								CString sWC, sRev;
								sWC.LoadString(IDS_DIFF_WORKINGCOPY);
								sRev.Format(IDS_DIFF_REVISIONPATCHED, rev);
								CUtils::StartDiffViewer(tempfile, m_path.Left(m_path.Find('\\')), FALSE, _T(""), _T(""), _T(""), TRUE, sWC, sRev);
							}
						}
						else
						{
							CString tempfile = CUtils::GetTempFile();
							m_templist.Add(tempfile);
							SVN svn;
							if (!svn.Cat(m_path, rev, tempfile))
							{
								CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							} // if (!svn.Cat(m_path, rev, tempfile))
							else
							{
								CString revname, wcname;
								CString ext = CUtils::GetFileExtFromPath(m_path);
								revname.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(m_path), rev);
								wcname.Format(IDS_DIFF_WCNAME, CUtils::GetFileNameFromPath(m_path));
								CUtils::StartDiffViewer(tempfile, m_path, FALSE, revname, wcname, ext);
							}
						}
					}
					break;
				case ID_COMPARETWO:
					{
						//user clicked on the menu item "compare revisions"
						//now first get the revisions which are selected
						POSITION pos = m_LogList.GetFirstSelectedItemPosition();
						long rev1 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));
						long rev2 = m_arRevs.GetAt(m_LogList.GetNextSelectedItem(pos));

						StartDiff(m_path, rev1, m_path, rev2);
					}
					break;
				case ID_SAVEAS:
					{
						//now first get the revision which is selected
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);

						OPENFILENAME ofn;		// common dialog box structure
						TCHAR szFile[MAX_PATH];  // buffer for file name
						ZeroMemory(szFile, sizeof(szFile));
						if (m_hasWC)
						{
							CString revFilename;
							int rfind = m_path.ReverseFind('.');
							if (rfind > 0)
								revFilename.Format(_T("%s-%ld%s"), m_path.Left(rfind), rev, m_path.Mid(rfind));
							else
								revFilename.Format(_T("%s-%ld"), m_path, rev);
							_tcscpy(szFile, revFilename);
						}
						// Initialize OPENFILENAME
						ZeroMemory(&ofn, sizeof(OPENFILENAME));
						//ofn.lStructSize = sizeof(OPENFILENAME);
						ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
						ofn.hwndOwner = this->m_hWnd;
						ofn.lpstrFile = szFile;
						ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
						CString temp;
						temp.LoadString(IDS_LOG_POPUP_SAVE);
						//ofn.lpstrTitle = "Save revision to...\0";
						if (temp.IsEmpty())
							ofn.lpstrTitle = NULL;
						else
							ofn.lpstrTitle = temp;
						ofn.Flags = OFN_OVERWRITEPROMPT;

						CString sFilter;
						sFilter.LoadString(IDS_COMMONFILEFILTER);
						TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
						_tcscpy (pszFilters, sFilter);
						// Replace '|' delimeters with '\0's
						TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
						while (ptr != pszFilters)
						{
							if (*ptr == '|')
								*ptr = '\0';
							ptr--;
						} // while (ptr != pszFilters) 
						ofn.lpstrFilter = pszFilters;
						ofn.nFilterIndex = 1;
						// Display the Open dialog box. 
						CString tempfile;
						if (GetSaveFileName(&ofn)==TRUE)
						{
							tempfile = CString(ofn.lpstrFile);
							SVN svn;
							if (!svn.Cat(m_path, rev, tempfile))
							{
								delete [] pszFilters;
								CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							} // if (!svn.Cat(m_path, rev, tempfile)) 
						} // if (GetSaveFileName(&ofn)==TRUE)
						delete [] pszFilters;
					}
					break;
				case ID_UPDATE:
					{
						//now first get the revision which is selected
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);

						SVN svn;
						if (!svn.Update(m_path, rev, TRUE))
						{
							CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							GetDlgItem(IDOK)->EnableWindow(TRUE);
							break;
						}
					}
					break;
				case ID_FINDENTRY:
					{
						m_nSearchIndex = m_LogList.GetSelectionMark();
						if (m_pFindDialog)
						{
							break;
						}
						else
						{
							m_pFindDialog = new CFindReplaceDialog();
							m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
						}
					}
					break;
				case ID_REPOBROWSE:
					{
						int selIndex = m_LogList.GetSelectionMark();
						long rev = m_arRevs.GetAt(selIndex);
						CString url = m_path;
						if (m_hasWC)
						{
							url = GetURLFromPath(m_path);
							if (url.IsEmpty())
							{
								CString temp;
								temp.Format(IDS_ERR_NOURLOFFILE, m_path);
								CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
								GetDlgItem(IDOK)->EnableWindow(TRUE);
								break;
							}
						} // if (m_hasWC)

						CRepositoryBrowser dlg(SVNUrl(url, SVNRev(rev)));
						dlg.DoModal();
					}
					break;
				case ID_POPPROPS:
					{
						int selIndex = m_LogList.GetSelectionMark();
						CPropDlg dlg;
						dlg.m_rev = m_arRevs.GetAt(selIndex);
						dlg.m_sPath = GetURLFromPath(m_path);
						dlg.DoModal();
					}
					break;
				default:
					break;
				} // switch (cmd)
				theApp.DoWaitCursor(-1);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			} // if (popup.CreatePopupMenu())
		} // if (selIndex >= 0)
	} // if (pWnd == &m_LogList)
//#endregion
//#region m_LogMsgCtrl
	if (pWnd == &m_LogMsgCtrl)
	{
		int selIndex = m_LogMsgCtrl.GetSelectionMark();
		if ((point.x == -1) && (point.y == -1))
		{
			CRect rect;
			m_LogMsgCtrl.GetItemRect(selIndex, &rect, LVIR_LABEL);
			m_LogMsgCtrl.ClientToScreen(&rect);
			point.x = rect.left + rect.Width()/2;
			point.y = rect.top + rect.Height()/2;
		}
		if (selIndex < 0)
			return;
		int s = m_LogList.GetSelectionMark();
		if (s < 0)
			return;
		long rev = m_arRevs.GetAt(s);
		//entry is selected, now show the popup menu
		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp;
			if (m_LogMsgCtrl.GetSelectedCount() == 1)
			{
				temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
				temp = temp.Left(temp.Find(' '));
				CString t, tt;
				t.LoadString(IDS_SVNACTION_ADD);
				tt.LoadString(IDS_SVNACTION_DELETE);
				if ((rev > 1)&&(temp.Compare(t)!=0)&&(temp.Compare(tt)!=0))
				{
					temp.LoadString(IDS_LOG_POPUP_DIFF);
					popup.AppendMenu(MF_STRING | MF_ENABLED, ID_DIFF, temp);
				}
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
				switch (cmd)
				{
				case ID_DIFF:
					{
						DoDiffFromLog(selIndex, rev);
					}
					break;
				default:
					break;
				} // switch (cmd)
			} // if (m_LogMsgCtrl.GetSelectedCount() == 1)
		} // if (popup.CreatePopupMenu())
	} // if (pWnd == &m_LogMsgCtrl) 
//#endregion

}

void CLogDlg::OnNMDblclkLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	int selIndex = pNMLV->iItem;
	int selSub = pNMLV->iSubItem;
	BOOL isSpecial = (GetKeyState(VK_SHIFT)<0)&&(selSub==1 || selSub==3);
	if ((selIndex >= 0)&&(!isSpecial))
	{
		CString temp;
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		this->m_app = &theApp;
		theApp.DoWaitCursor(1);
		if ((!PathIsDirectory(m_path))&&(m_hasWC))
		{
			long rev = m_arRevs.GetAt(selIndex);
			CString tempfile = CUtils::GetTempFile();
			m_templist.Add(tempfile);

			SVN svn;
			if (!svn.Cat(m_path, rev, tempfile))
			{
				CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
			} // if (!svn.Cat(m_path, rev, tempfile))
			else
			{
				CString revname, wcname;
				CString ext = CUtils::GetFileExtFromPath(m_path);
				revname.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(m_path), rev);
				wcname.Format(IDS_DIFF_WCNAME, CUtils::GetFileNameFromPath(m_path));
				CUtils::StartDiffViewer(tempfile, m_path, FALSE, revname, wcname, ext);
			}
		} 
		else
		{
			long rev = m_arRevs.GetAt(selIndex);
			this->m_bCancelled = FALSE;
			CString tempfile = CUtils::GetTempFile();
			tempfile += _T(".diff");
			m_templist.Add(tempfile);
			if (!PegDiff(m_path, (m_hasWC ? SVNRev::REV_WC : SVNRev::REV_HEAD), SVNRev::REV_WC, rev, TRUE, FALSE, TRUE, _T(""), tempfile))
			{
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				if (CUtils::CheckForEmptyDiff(tempfile))
				{
					CMessageBox::Show(m_hWnd, IDS_ERR_EMPTYDIFF, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					CString sWC, sRev;
					sWC.LoadString(IDS_DIFF_WORKINGCOPY);
					sRev.Format(IDS_DIFF_REVISIONPATCHED, rev);
					CUtils::StartDiffViewer(tempfile, m_path.Left(m_path.Find('\\')), FALSE, _T(""), _T(""), _T(""), TRUE, sWC, sRev); 
				}
			}
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	if (isSpecial)
	{
		CString url;
		CString name;
		CString text;
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		this->m_app = &theApp;
		theApp.DoWaitCursor(1);
		if (SVN::PathIsURL(m_path))
			url = m_path;
		else
		{
			SVN svn;
			url = svn.GetURLFromPath(m_path);
		}
		if (selSub == 1)
		{
			text.LoadString(IDS_LOG_AUTHOR);
			name = SVN_PROP_REVISION_AUTHOR;
		}
		else if (selSub == 3)
		{
			text.LoadString(IDS_LOG_MESSAGE);
			name = SVN_PROP_REVISION_LOG;
		}

		CString value = RevPropertyGet(name, url, m_arRevs.GetAt(selIndex));
		value.Replace(_T("\n"), _T("\r\n"));
		CInputDlg dlg;
		dlg.m_sHintText = text;
		dlg.m_sInputText = value;
		if (dlg.DoModal() == IDOK)
		{
			dlg.m_sInputText.Replace(_T("\r"), _T(""));
			if (!RevPropertySet(name, dlg.m_sInputText, url, m_arRevs.GetAt(selIndex)))
				CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		}
		theApp.DoWaitCursor(-1);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
}

LRESULT CLogDlg::OnFindDialogMessage(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_pFindDialog != NULL);

    if (m_pFindDialog->IsTerminating())
    {
	    // invalidate the handle identifying the dialog box.
        m_pFindDialog = NULL;
        return 0;
    }

    if(m_pFindDialog->FindNext())
    {
        //read data from dialog
        CString FindText = m_pFindDialog->GetFindString();
        bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		bool bFound = FALSE;

		for (int i=this->m_nSearchIndex; i<m_LogList.GetItemCount(); i++)
		{
			if (bMatchCase)
			{
				if (m_arLogMessages.GetAt(i).Find(FindText) >= 0)
				{
					bFound = TRUE;
					break;
				} // if (m_arLogMessages.GetAt(i).Find(FindText) >= 0) 
				if (m_arLogPaths.GetAt(i).Find(FindText) >= 0)
				{
					bFound = TRUE;
					break;
				}
			} // if (bMatchCase) 
			else
			{
				CString msg = m_arLogMessages.GetAt(i);
				msg += m_arLogPaths.GetAt(i);
				msg = msg.MakeLower();
				CString find = FindText.MakeLower();
				if (msg.Find(FindText) >= 0)
				{
					bFound = TRUE;
					break;
				} // if (msg.Find(FindText) >= 0) 
			} 
		} // for (int i=this->m_nSearchIndex; i<m_LogList.GetItemCount(); i++) 
		if (bFound)
		{
			this->m_nSearchIndex = (i+1);
			m_LogList.EnsureVisible(i, FALSE);
			m_LogList.SetItemState(m_LogList.GetSelectionMark(), 0, LVIS_SELECTED);
			m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_LogList.SetSelectionMark(i);
			FillLogMessageCtrl(m_arLogMessages.GetAt(i), m_arLogPaths.GetAt(i));
			UpdateData(FALSE);
			m_nSearchIndex++;
			if (m_nSearchIndex >= m_arLogMessages.GetCount())
				m_nSearchIndex = (int)m_arLogMessages.GetCount()-1;
		}
    } // if(m_pFindDialog->FindNext()) 

    return 0;
}

void CLogDlg::OnOK()
{
	CString temp;
	CString buttontext;
	GetDlgItem(IDOK)->GetWindowText(buttontext);
	temp.LoadString(IDS_MSGBOX_CANCEL);
	if (temp.Compare(buttontext) != 0)
		__super::OnOK();
	m_bCancelled = TRUE;
	if (m_pNotifyWindow)
	{
		int selIndex = m_LogList.GetSelectionMark();
		if (selIndex >= 0)
		{	
			m_pNotifyWindow->SendMessage(WM_REVSELECTED, m_wParam, m_arRevs.GetAt(selIndex));
		}
	}
}

void CLogDlg::OnNMDblclkLogmsg(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int selIndex = m_LogMsgCtrl.GetSelectionMark();
	if (selIndex < 0)
		return;
	int s = m_LogList.GetSelectionMark();
	if (s < 0)
		return;
	long rev = m_arRevs.GetAt(s);
	CString temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
	temp = temp.Left(temp.Find(' '));
	CString t, tt;
	t.LoadString(IDS_SVNACTION_ADD);
	tt.LoadString(IDS_SVNACTION_DELETE);
	if ((rev > 1)&&(temp.Compare(t)!=0)&&(temp.Compare(tt)!=0))
	{
		DoDiffFromLog(selIndex, rev);
	}
}

void CLogDlg::DoDiffFromLog(int selIndex, long rev)
{
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	this->m_app = &theApp;
	theApp.DoWaitCursor(1);
	//get the filename
	CString filepath;
	if (SVN::PathIsURL(m_path))
	{
		filepath = m_path;
	}
	else
	{
		SVN svn;
		filepath = svn.GetURLFromPath(m_path);
		if (filepath.IsEmpty())
		{
			theApp.DoWaitCursor(-1);
			CString temp;
			temp.Format(IDS_ERR_NOURLOFFILE, filepath);
			CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
			TRACE(_T("could not retrieve the URL of the file!\n"));
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			theApp.DoWaitCursor(-11);
			return;		//exit
		} // if ((rev == (-2))||(status.status->entry == NULL))
	}
	CString temp = m_LogMsgCtrl.GetItemText(selIndex, 0);
	m_bCancelled = FALSE;
	filepath = GetRepositoryRoot(filepath);
	temp = temp.Mid(temp.Find(' '));
	if (temp.Find('(')>=0)
	{
		temp = temp.Left(temp.Find('(')-1);
	}
	temp = temp.Trim();
	filepath += temp;
	StartDiff(filepath, rev, filepath, rev-1);
	theApp.DoWaitCursor(-1);
	GetDlgItem(IDOK)->EnableWindow(TRUE);
}

BOOL CLogDlg::StartDiff(CString path1, LONG rev1, CString path2, LONG rev2)
{
	CString tempfile1 = CUtils::GetTempFile();
	CString tempfile2 = CUtils::GetTempFile();
	SVN::preparePath(path1);
	SVN::preparePath(path2);
	if (path1.ReverseFind('.')>=path1.ReverseFind('/'))
		tempfile1 += path1.Mid(path1.ReverseFind('.'));
	if (path2.ReverseFind('.')>=path2.ReverseFind('/'))
		tempfile2 += path2.Mid(path2.ReverseFind('.'));
	m_templist.Add(tempfile1);
	m_templist.Add(tempfile2);
	CProgressDlg progDlg;
	if (progDlg.IsValid())
	{
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
		temp.LoadString(IDS_PROGRESSWAIT);
		progDlg.SetTitle(temp);
		progDlg.SetLine(3, _T(""));
		progDlg.SetCancelMsg(_T(""));
		progDlg.ShowModeless(this);
	}
	m_bCancelled = FALSE;
	this->m_app = &theApp;
	theApp.DoWaitCursor(1);
	if (!Cat(path1, rev1, tempfile1))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	} // if (!Cat(path1, rev1, tempfile1))
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)1,(DWORD)2);
		CString temp;
		temp.Format(IDS_PROGRESSGETFILE, path1);
		progDlg.SetLine(1, temp, true);
		temp.Format(IDS_PROGRESSREVISION, rev1);
		progDlg.SetLine(2, temp, false);
	}
	if (!Cat(path2, rev2, tempfile2))
	{
		theApp.DoWaitCursor(-1);
		CMessageBox::Show(NULL, GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		return FALSE;
	}
	if (progDlg.IsValid())
	{
		progDlg.SetProgress((DWORD)2,(DWORD)2);
		progDlg.Stop();
	}
	theApp.DoWaitCursor(-1);
	CString revname1, revname2;
	CString ext = CUtils::GetFileExtFromPath(path1);
	revname1.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(path1), rev1);
	revname2.Format(_T("%s Revision %ld"), CUtils::GetFileNameFromPath(path2), rev2);
	return CUtils::StartDiffViewer(tempfile2, tempfile1, FALSE, revname2, revname1, ext);
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F5:
			{
				if (!GetDlgItem(IDC_GETALL)->IsWindowEnabled())
					return __super::PreTranslateMessage(pMsg);
				OnBnClickedGetall();
			}
			break;
		case 'F':
			{
				if (GetKeyState(VK_CONTROL)&0x8000)
				{
					if (m_pFindDialog)
					{
						break;
					}
					else
					{
						m_pFindDialog = new CFindReplaceDialog();
						m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
					}
				}
			}	
			break;
		case VK_F3:
			{
				if (m_pFindDialog)
				{
					break;
				}
				else
				{
					m_pFindDialog = new CFindReplaceDialog();
					m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
				}
			}
			break;
		default:
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

BOOL CLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bThreadRunning)
	{
		// only show the wait cursor over the list control
		if ((pWnd)&&
			((pWnd == GetDlgItem(IDC_LOGLIST))||
			(pWnd == GetDlgItem(IDC_MSGVIEW))||
			(pWnd == GetDlgItem(IDC_LOGMSG))))
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT));
			SetCursor(hCur);
			return TRUE;
		}
	}
	HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	SetCursor(hCur);
	return CResizableDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CLogDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CLogDlg::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	if (pNMLV->iItem >= 0)
	{
		int selIndex = pNMLV->iItem;
		m_nSearchIndex = selIndex;
		if (pNMLV->iSubItem != 0)
			return;
		if (pNMLV->uNewState & LVIS_SELECTED)
		{
			FillLogMessageCtrl(m_arLogMessages.GetAt(selIndex), m_arLogPaths.GetAt(selIndex));
			UpdateData(FALSE);
		}
	}
}

void CLogDlg::OnEnLinkMsgview(NMHDR *pNMHDR, LRESULT *pResult)
{
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink->msg == WM_LBUTTONUP)
	{
		CString url, msg;
		GetDlgItem(IDC_MSGVIEW)->GetWindowText(msg);
		msg.Replace(_T("\r\n"), _T("\n"));
		url = msg.Mid(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax-pEnLink->chrg.cpMin);
		if (!::PathIsURL(url))
		{
			url = m_BugtraqInfo.GetBugIDUrl(url);
		}
		if (!url.IsEmpty())
			ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);
	}
	*pResult = 0;
}
















