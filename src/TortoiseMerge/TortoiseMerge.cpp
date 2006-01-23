// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "CmdLineParser.h"
#include "version.h"
#include "Utils.h"
#include "BrowseFolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _WIN32
#	pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#	ifdef _DEBUG
#		pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#	endif
#endif

BEGIN_MESSAGE_MAP(CTortoiseMergeApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


CTortoiseMergeApp::CTortoiseMergeApp()
{
	EnableHtmlHelp();
}

// The one and only CTortoiseMergeApp object
CTortoiseMergeApp theApp;
CCrashReport g_crasher("crashreports@tortoisesvn.tigris.org", "Crashreport for TortoiseMerge : " STRPRODUCTVER, TRUE);

// CTortoiseMergeApp initialization
BOOL CTortoiseMergeApp::InitInstance()
{
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseMerge\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("..\\Languages\\TortoiseMerge%d.dll"), langId);
		
		hInst = LoadLibrary(langDll);
		CString sVer = _T(STRPRODUCTVER);
		sVer = sVer.Left(sVer.ReverseFind(','));
		CString sFileVer = CUtils::GetVersionFromFile(langDll);
		sFileVer = sFileVer.Left(sFileVer.ReverseFind(','));
		if (sFileVer.Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = NULL;
		}
		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
		else
		{
			DWORD lid = SUBLANGID(langId);
			lid--;
			if (lid > 0)
			{
				langId = MAKELANGID(PRIMARYLANGID(langId), lid);
			}
			else
				langId = 0;
		}
	} while ((hInst == NULL) && (langId != 0));
	TCHAR buf[6];
	_tcscpy_s(buf, _T("en"));
	langId = loc;
	CString sHelppath;
	sHelppath = this->m_pszHelpFilePath;
	sHelppath = sHelppath.MakeLower();
	sHelppath.Replace(_T(".chm"), _T("_en.chm"));
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	do
	{
		GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf));
		CString sLang = _T("_");
		sLang += buf;
		sHelppath.Replace(_T("_en"), sLang);
		if (PathFileExists(sHelppath))
		{
			free((void*)m_pszHelpFilePath);
			m_pszHelpFilePath=_tcsdup(sHelppath);
		}

		DWORD lid = SUBLANGID(langId);
		lid--;
		if (lid > 0)
		{
			langId = MAKELANGID(PRIMARYLANGID(langId), lid);
		}
		else
			langId = 0;
	} while (langId);
	setlocale(LC_ALL, ""); 

	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	CCmdLineParser parser = CCmdLineParser(this->m_lpCmdLine);

	if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		CString sHelpText;
		sHelpText.LoadString(IDS_COMMANDLINEHELP);
		MessageBox(NULL, sHelpText, _T("TortoiseMerge"), MB_ICONINFORMATION);
		return FALSE;
	} // if (parser.HasKey(_T("?")) || parser.HasKey(_T("help"))) 

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey(_T("TortoiseMerge"));

	if (CRegDWORD(_T("Software\\TortoiseMerge\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (pFrame == NULL)
		return FALSE;
	m_pMainWnd = pFrame;
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// Fill in the command line options
	pFrame->m_Data.m_baseFile.SetFileName(parser.GetVal(_T("base")));
	pFrame->m_Data.m_baseFile.SetDescriptiveName(parser.GetVal(_T("basename")));
	pFrame->m_Data.m_theirFile.SetFileName(parser.GetVal(_T("theirs")));
	pFrame->m_Data.m_theirFile.SetDescriptiveName(parser.GetVal(_T("theirsname")));
	pFrame->m_Data.m_yourFile.SetFileName(parser.GetVal(_T("mine")));
	pFrame->m_Data.m_yourFile.SetDescriptiveName(parser.GetVal(_T("minename")));
	pFrame->m_Data.m_mergedFile.SetFileName(parser.GetVal(_T("merged")));
	pFrame->m_Data.m_mergedFile.SetDescriptiveName(parser.GetVal(_T("mergedname")));
	pFrame->m_Data.m_sPatchPath = parser.GetVal(_T("patchpath"));
	pFrame->m_Data.m_sPatchPath.Replace('/', '\\');
	if (parser.HasKey(_T("patchoriginal")))
		pFrame->m_Data.m_sPatchOriginal = parser.GetVal(_T("patchoriginal"));
	if (parser.HasKey(_T("patchpatched")))
		pFrame->m_Data.m_sPatchPatched = parser.GetVal(_T("patchpatched"));
	pFrame->m_Data.m_sDiffFile = parser.GetVal(_T("diff"));
	pFrame->m_Data.m_sDiffFile.Replace('/', '\\');
	if (parser.HasKey(_T("oneway")))
        pFrame->m_bOneWay = TRUE;
	if (parser.HasKey(_T("reversedpatch")))
		pFrame->m_bReversedPatch = TRUE;
	if (pFrame->m_Data.IsBaseFileInUse() && !pFrame->m_Data.IsYourFileInUse() && pFrame->m_Data.IsTheirFileInUse())
	{
		pFrame->m_Data.m_yourFile.TransferDetailsFrom(pFrame->m_Data.m_theirFile);
	}

	if ((!parser.HasKey(_T("patchpath")))&&(parser.HasVal(_T("diff"))))
	{
		// a patchfile was given, but not folder path to apply the patch to
		// If the patchfile is located inside a working copy, then use the parent directory
		// of the patchfile as the target directory, otherwise ask the user for a path.
		if (parser.HasKey(_T("wc")))
			pFrame->m_Data.m_sPatchPath = pFrame->m_Data.m_sDiffFile.Left(pFrame->m_Data.m_sDiffFile.ReverseFind('\\'));
		else
		{
			CBrowseFolder fbrowser;
			fbrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (fbrowser.Show(NULL, pFrame->m_Data.m_sPatchPath)==CBrowseFolder::CANCEL)
				return FALSE;
		}
	}

	if ((parser.HasKey(_T("patchpath")))&&(!parser.HasVal(_T("diff"))))
	{
		// A path was given for applying a patchfile, but
		// the patchfile itself was not.
		// So ask the user for that patchfile

		OPENFILENAME ofn;		// common dialog box structure
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
		ofn.hwndOwner = pFrame->m_hWnd;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		CString temp;
		temp.LoadString(IDS_OPENDIFFFILETITLE);
		if (temp.IsEmpty())
			ofn.lpstrTitle = NULL;
		else
			ofn.lpstrTitle = temp;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
		CString sFilter;
		sFilter.LoadString(IDS_PATCHFILEFILTER);
		TCHAR * pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
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
		if (GetOpenFileName(&ofn)==FALSE)
		{
			delete [] pszFilters;
			return FALSE;
		} // if (GetOpenFileName(&ofn)==FALSE)
		delete [] pszFilters;
		pFrame->m_Data.m_sDiffFile = ofn.lpstrFile;
	} // if ((parser.HasKey(_T("patchpath")))&&(!parser.HasVal(_T("diff")))) 

	pFrame->m_bReadOnly = !!parser.HasKey(_T("readonly"));
	// The one and only window has been initialized, so show and update it
	pFrame->ActivateFrame();
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	if (!pFrame->m_Data.IsBaseFileInUse() && pFrame->m_Data.m_sPatchPath.IsEmpty() && pFrame->m_Data.m_sDiffFile.IsEmpty())
	{
		pFrame->OnFileOpen();
		return TRUE;
	}
	return pFrame->LoadViews();
}

// CTortoiseMergeApp message handlers

void CTortoiseMergeApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
