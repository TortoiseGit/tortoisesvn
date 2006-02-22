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
#include "TortoiseProc.h"
#include "registry.h"
#include "DirFileEnum.h"
#include "SetSavedDataPage.h"


// CSetSavedDataPage dialog

IMPLEMENT_DYNAMIC(CSetSavedDataPage, CPropertyPage)

CSetSavedDataPage::CSetSavedDataPage()
	: CPropertyPage(CSetSavedDataPage::IDD)
{

}

CSetSavedDataPage::~CSetSavedDataPage()
{
}

void CSetSavedDataPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLHISTCLEAR, m_btnUrlHistClear);
	DDX_Control(pDX, IDC_LOGHISTCLEAR, m_btnLogHistClear);
	DDX_Control(pDX, IDC_RESIZABLEHISTCLEAR, m_btnResizableHistClear);
	DDX_Control(pDX, IDC_AUTHHISTCLEAR, m_btnAuthHistClear);
}

BOOL CSetSavedDataPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// find out how many log messages and URLs we've stored
	int nLogHistWC = 0;
	int nLogHistMsg = 0;
	int nUrlHistWC = 0;
	int nUrlHistItems = 0;
	CRegistryKey regloghist(_T("Software\\TortoiseSVN\\History"));
	CStringList loghistlist;
	regloghist.getSubKeys(loghistlist);
	for (POSITION pos = loghistlist.GetHeadPosition(); pos != NULL; )
	{
		CString sHistName = loghistlist.GetNext(pos);
		if (sHistName.Left(6).CompareNoCase(_T("commit"))==0)
		{
			nLogHistWC++;
			CRegistryKey regloghistwc(_T("Software\\TortoiseSVN\\History\\")+sHistName);
			CStringList loghistlistwc;
			regloghistwc.getValues(loghistlistwc);
			nLogHistMsg += loghistlistwc.GetCount();
		}
		else
		{
			// repoURLs
			CStringList urlhistlistmain;
			CStringList urlhistlistmainvalues;
			CRegistryKey regurlhistlist(_T("Software\\TortoiseSVN\\History\\repoURLS"));
			regurlhistlist.getSubKeys(urlhistlistmain);
			regurlhistlist.getValues(urlhistlistmainvalues);
			nUrlHistItems += urlhistlistmainvalues.GetCount();
			for (POSITION urlpos = urlhistlistmain.GetHeadPosition(); urlpos != NULL; )
			{
				CString sWCUID = urlhistlistmain.GetNext(urlpos);
				nUrlHistWC++;
				CStringList urlhistlistwc;
				CRegistryKey regurlhistlistwc(_T("Software\\TortoiseSVN\\History\\repoURLS\\")+sWCUID);
				regurlhistlistwc.getValues(urlhistlistwc);
				nUrlHistItems += urlhistlistwc.GetCount();
			}
		}
	}
	
	// find out how many dialog sizes / positions we've stored
	int nResizableDialogs = 0;
	CRegistryKey regResizable(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState"));
	CStringList resizablelist;
	regResizable.getValues(resizablelist);
	nResizableDialogs += resizablelist.GetCount();

	// find out how many auth data we've stored
	int nSimple = 0;
	int nSSL = 0;
	int nUsername = 0;

	TCHAR pathbuf[MAX_PATH] = {0};
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathbuf)==S_OK)
	{
		_tcscat_s(pathbuf, MAX_PATH, _T("\\Subversion\\auth\\"));
		CString sSimple = CString(pathbuf) + _T("svn.simple");
		CString sSSL = CString(pathbuf) + _T("svn.ssl.server");
		CString sUsername = CString(pathbuf) + _T("svn.username");
		CString sFile;
		bool bIsDir = false;
		CDirFileEnum simpleenum(sSimple);
		while (simpleenum.NextFile(sFile, &bIsDir))
			nSimple++;
		CDirFileEnum sslenum(sSSL);
		while (sslenum.NextFile(sFile, &bIsDir))
			nSSL++;
		CDirFileEnum userenum(sUsername);
		while (userenum.NextFile(sFile, &bIsDir))
			nUsername++;
	}

	m_btnLogHistClear.EnableWindow(nLogHistMsg || nLogHistWC);
	m_btnUrlHistClear.EnableWindow(nUrlHistItems || nUrlHistWC);
	m_btnResizableHistClear.EnableWindow(nResizableDialogs);
	m_btnAuthHistClear.EnableWindow(nSimple || nSSL || nUsername);

	EnableToolTips();

	m_tooltips.Create(this);
	CString sTT;
	sTT.Format(IDS_SETTINGS_SAVEDDATA_LOGHIST_TT, nLogHistMsg, nLogHistWC);
	m_tooltips.AddTool(IDC_LOGHISTORY, sTT);
	m_tooltips.AddTool(IDC_LOGHISTCLEAR, sTT);
	sTT.Format(IDS_SETTINGS_SAVEDDATA_URLHIST_TT, nUrlHistItems, nUrlHistWC);
	m_tooltips.AddTool(IDC_URLHISTORY, sTT);
	m_tooltips.AddTool(IDC_URLHISTCLEAR, sTT);
	sTT.Format(IDS_SETTINGS_SAVEDDATA_RESIZABLE_TT, nResizableDialogs);
	m_tooltips.AddTool(IDC_RESIZABLEHISTORY, sTT);
	m_tooltips.AddTool(IDC_RESIZABLEHISTCLEAR, sTT);
	sTT.Format(IDS_SETTINGS_SAVEDDATA_AUTH_TT, nSimple, nSSL, nUsername);
	m_tooltips.AddTool(IDC_AUTHHISTORY, sTT);
	m_tooltips.AddTool(IDC_AUTHHISTCLEAR, sTT);

	return TRUE;
}

BOOL CSetSavedDataPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CSetSavedDataPage, CPropertyPage)
	ON_BN_CLICKED(IDC_URLHISTCLEAR, &CSetSavedDataPage::OnBnClickedUrlhistclear)
	ON_BN_CLICKED(IDC_LOGHISTCLEAR, &CSetSavedDataPage::OnBnClickedLoghistclear)
	ON_BN_CLICKED(IDC_RESIZABLEHISTCLEAR, &CSetSavedDataPage::OnBnClickedResizablehistclear)
	ON_BN_CLICKED(IDC_AUTHHISTCLEAR, &CSetSavedDataPage::OnBnClickedAuthhistclear)
END_MESSAGE_MAP()


// CSetSavedDataPage message handlers

void CSetSavedDataPage::OnBnClickedUrlhistclear()
{
	CRegistryKey reg(_T("Software\\TortoiseSVN\\History\\repoURLS"));
	reg.removeKey();
	m_btnUrlHistClear.EnableWindow(FALSE);
	m_tooltips.RemoveTool(GetDlgItem(IDC_URLHISTCLEAR));
	m_tooltips.RemoveTool(GetDlgItem(IDC_URLHISTORY));
}

void CSetSavedDataPage::OnBnClickedLoghistclear()
{
	CRegistryKey reg(_T("Software\\TortoiseSVN\\History"));
	CStringList histlist;
	reg.getSubKeys(histlist);
	for (POSITION pos = histlist.GetHeadPosition(); pos != NULL; )
	{
		CString sHist = histlist.GetNext(pos);
		if (sHist.Left(6).CompareNoCase(_T("commit"))==0)
		{
			CRegistryKey regkey(_T("Software\\TortoiseSVN\\History\\")+sHist);
			regkey.removeKey();
		}
	}

	m_btnLogHistClear.EnableWindow(FALSE);
	m_tooltips.RemoveTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
	m_tooltips.RemoveTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedResizablehistclear()
{
	CRegistryKey reg(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState"));
	reg.removeKey();
	m_btnResizableHistClear.EnableWindow(FALSE);
	m_tooltips.RemoveTool(GetDlgItem(IDC_RESIZABLEHISTCLEAR));
	m_tooltips.RemoveTool(GetDlgItem(IDC_RESIZABLEHISTORY));
}

void CSetSavedDataPage::OnBnClickedAuthhistclear()
{
	CRegStdString auth = CRegStdString(_T("Software\\TortoiseSVN\\Auth\\"));
	auth.removeKey();
	TCHAR pathbuf[MAX_PATH] = {0};
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, pathbuf)==S_OK)
	{
		_tcscat_s(pathbuf, MAX_PATH, _T("\\Subversion\\auth"));
		pathbuf[_tcslen(pathbuf)+1] = 0;
		SHFILEOPSTRUCT fileop;
		fileop.hwnd = this->m_hWnd;
		fileop.wFunc = FO_DELETE;
		fileop.pFrom = pathbuf;
		fileop.pTo = NULL;
		fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | FOF_NOCONFIRMATION;// | FOF_NOERRORUI | FOF_SILENT;
		fileop.lpszProgressTitle = _T("deleting file");
		SHFileOperation(&fileop);
	}
	m_btnAuthHistClear.EnableWindow(FALSE);
	m_tooltips.RemoveTool(GetDlgItem(IDC_AUTHHISTCLEAR));
	m_tooltips.RemoveTool(GetDlgItem(IDC_AUTHHISTORY));
}


