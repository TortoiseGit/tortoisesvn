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
//#include "vld.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "SysImageList.h"
#include "CrashReport.h"
#include "SVNProperties.h"
#include "Blame.h"
#include "DirFileEnum.h"
#include "CmdLineParser.h"
#include "AboutDlg.h"
#include "LogDlg.h"
#include "SVNProgressDlg.h"
#include "ImportDlg.h"
#include "CheckoutDlg.h"
#include "UpdateDlg.h"
#include "LogPromptDlg.h"
#include "AddDlg.h"
#include "ResolveDlg.h"
#include "RevertDlg.h"
#include "RepoCreateDlg.h"
#include "RenameDlg.h"
#include "SwitchDlg.h"
#include "MergeDlg.h"
#include "CopyDlg.h"
#include "Settings.h"
#include "RelocateDlg.h"
#include "URLDlg.h"
#include "ChangedDlg.h"
#include "RepositoryBrowser.h"
#include "BlameDlg.h"
#include "LockDlg.h"
#include "CheckForUpdatesDlg.h"
#include "RevisionGraphDlg.h"
#include "FileDiffDlg.h"
#include "InputDlg.h"
#include "EditPropertiesDlg.h"
#include "EditPropertyValueDlg.h"
#include "BrowseFolder.h"
#include "SVNStatus.h"
#include "SVNInfo.h"
#include "Utils.h"
#include "TempFile.h"
#include "SoundUtils.h"
#include "libintl.h"
#include "ShellUpdater.h"
#include "SVNDiff.h"
#include "CreatePatch.h"
#include "SVNAdminDir.h"

#include "..\version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef WIN64
#	pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#	ifdef _DEBUG
#		pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.VC80.CRT' version='8.0.50608.0' processorArchitecture='X86' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#	endif
#endif

#define PWND (hWndExplorer ? CWnd::FromHandle(hWndExplorer) : NULL)
#define EXPLORERHWND (IsWindow(hWndExplorer) ? hWndExplorer : NULL)

// This is a fake filename which we use to fill-in the create-patch file-open dialog
#define PATCH_TO_CLIPBOARD_PSEUDO_FILENAME		_T(".TSVNPatchToClipboard")

BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

typedef enum
{
	cmdTest,
	cmdCrash,
	cmdAbout,
	cmdRTFM,
	cmdLog,
	cmdCheckout,
	cmdImport,
	cmdUpdate,
	cmdCommit,
	cmdAdd,
	cmdRevert,
	cmdCleanup,
	cmdResolve,
	cmdRepoCreate,
	cmdSwitch,
	cmdExport,
	cmdMerge,
	cmdCopy,
	cmdSettings,
	cmdRemove,
	cmdRename,
	cmdDiff,
	cmdDropCopyAdd,
	cmdDropMove,
	cmdDropExport,
	cmdDropCopy,
	cmdConflictEditor,
	cmdRelocate,
	cmdHelp,
	cmdRepoStatus,
	cmdRepoBrowser,
	cmdIgnore,
	cmdUnIgnore,
	cmdBlame,
	cmdCat,
	cmdCreatePatch,
	cmdUpdateCheck,
	cmdRevisionGraph,
	cmdLock,
	cmdUnlock,
	cmdRebuildIconCache,
	cmdProperties
} TSVNCommand;

static const struct CommandInfo
{
	TSVNCommand command;
	LPCTSTR pCommandName;
	bool bPathIsTempFileByDefault;
} commandInfo[] = 
{
	//                                          PathIsTempFile?
	{	cmdTest,			_T("test"),				false	},
	{	cmdCrash,			_T("crash"),			false	},
	{	cmdAbout,			_T("about"),			false	},
	{	cmdRTFM,			_T("rtfm"),				false	},
	{	cmdLog,				_T("log"),				false	},
	{	cmdCheckout,		_T("checkout"),			false	},
	{	cmdImport,			_T("import"),			false	},
	{	cmdUpdate,			_T("update"),			true	},
	{	cmdCommit,			_T("commit"),			true	},
	{	cmdAdd,				_T("add"),				true	},
	{	cmdRevert,			_T("revert"),			true	},
	{	cmdCleanup,			_T("cleanup"),			false	},
	{	cmdResolve,			_T("resolve"),			true	},
	{	cmdRepoCreate,		_T("repocreate"),		false	},
	{	cmdSwitch,			_T("switch"),			false	},
	{	cmdExport,			_T("export"),			false	},
	{	cmdMerge,			_T("merge"),			false	},
	{	cmdCopy,			_T("copy"),				false	},
	{	cmdSettings,		_T("settings"),			false	},
	{	cmdRemove,			_T("remove"),			true	},
	{	cmdRename,			_T("rename"),			false	},
	{	cmdDiff,			_T("diff"),				false	},
	{	cmdDropCopyAdd,		_T("dropcopyadd"),		true	},
	{	cmdDropMove,		_T("dropmove"),			true	},
	{	cmdDropExport,		_T("dropexport"),		true	},
	{	cmdDropCopy,		_T("dropcopy"),			true	},
	{	cmdConflictEditor,	_T("conflicteditor"),	false	},
	{	cmdRelocate,		_T("relocate"),			false	},
	{	cmdHelp,			_T("help"),				false	},
	{	cmdRepoStatus,		_T("repostatus"),		false	},
	{	cmdRepoBrowser,		_T("repobrowser"),		false	},
	{	cmdIgnore,			_T("ignore"),			true	},
	{	cmdUnIgnore,		_T("unignore"),			true	},
	{	cmdBlame,			_T("blame"),			false	},
	{	cmdCat,				_T("cat"),				false	},
	{	cmdCreatePatch,		_T("createpatch"),		true	},
	{	cmdUpdateCheck,		_T("updatecheck"),		false	},
	{	cmdRevisionGraph,	_T("revisiongraph"),	false	},
	{	cmdLock,			_T("lock"),				true	},
	{	cmdUnlock,			_T("unlock"),			true	},
	{	cmdRebuildIconCache,_T("rebuildiconcache"),	false	},
	{	cmdProperties,		_T("properties"),		true	},
};

//////////////////////////////////////////////////////////////////////////

CTortoiseProcApp::CTortoiseProcApp()
{
	EnableHtmlHelp();
	int argc = 0;
	const char* const * argv = NULL;
	apr_app_initialize(&argc, &argv, NULL);
	SYS_IMAGE_LIST();
	g_SVNAdminDir.Init();
}

CTortoiseProcApp::~CTortoiseProcApp()
{
	// since it is undefined *when* the global object SVNAdminDir is
	// destroyed, we tell it to destroy the memory pools and terminate apr
	// *now* instead of later when the object itself is destroyed.
	g_SVNAdminDir.Close();
	SYS_IMAGE_LIST().Cleanup();
	apr_terminate();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
HWND hWndExplorer;

CCrashReport crasher("crashreports@tortoisesvn.tigris.org", "Crash Report for TortoiseSVN : " STRPRODUCTVER, TRUE);// crash

// CTortoiseProcApp initialization

void CTortoiseProcApp::CrashProgram()
{
	int * a;
	a = NULL;
	*a = 7;
}
BOOL CTortoiseProcApp::InitInstance()
{
	CheckUpgrade();
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	CStringA langpath = CStringA(CUtils::GetAppParentDirectory());
	langpath += "Languages";
	bindtextdomain("subversion", (LPCSTR)langpath);
	bind_textdomain_codeset("subversion", "UTF-8");
	HINSTANCE hInst = NULL;
	
	do
	{
		langDll.Format(_T("..\\Languages\\TortoiseProc%d.dll"), langId);
		
		hInst = LoadLibrary(langDll);

		CString sVer = _T(STRPRODUCTVER);
		sVer = sVer.Left(sVer.ReverseFind(','));
		CString sFileVer = CUtils::GetVersionFromFile(langDll);
		int commaIndex = sFileVer.ReverseFind(',');
		if (commaIndex==-1 || sFileVer.Left(commaIndex).Compare(sVer)!=0)
		{
			FreeLibrary(hInst);
			hInst = NULL;
		}
		if (hInst != NULL)
		{
			AfxSetResourceHandle(hInst);
		}
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
	sHelppath.Replace(_T("tortoiseproc.chm"), _T("TortoiseSVN_en.chm"));
	free((void*)m_pszHelpFilePath);
	m_pszHelpFilePath=_tcsdup(sHelppath);
	do
	{
		CString sLang = _T("_");
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, sizeof(buf)))
		{
			sLang += buf;
			sHelppath.Replace(_T("_en"), sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_tcsdup(sHelppath);
				langId = 0;
			}
		}
		sHelppath.Replace(sLang, _T("_en"));
		if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, sizeof(buf)))
		{
			sLang += _T("_");
			sLang += buf;
			sHelppath.Replace(_T("_en"), sLang);
			if (PathFileExists(sHelppath))
			{
				free((void*)m_pszHelpFilePath);
				m_pszHelpFilePath=_tcsdup(sHelppath);
				langId = 0;
			}
		}
		sHelppath.Replace(sLang, _T("_en"));

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
	
    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
			ICC_ANIMATE_CLASS | ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES |
			ICC_HOTKEY_CLASS | ICC_INTERNET_CLASSES | ICC_LISTVIEW_CLASSES |
			ICC_NATIVEFNTCTL_CLASS | ICC_PAGESCROLLER_CLASS | ICC_PROGRESS_CLASS |
			ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS |
			ICC_USEREX_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);
	AfxOleInit();
	AfxEnableControlContainer();
	AfxInitRichEdit2();
	CWinApp::InitInstance();
	SetRegistryKey(_T("TortoiseSVN"));

	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

	if (CRegDWORD(_T("Software\\TortoiseSVN\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	if (!parser.HasKey(_T("command")))
	{
		CAboutDlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();
		return FALSE;
	}

	CString comVal = parser.GetVal(_T("command"));
	if (comVal.IsEmpty())
	{
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMANDVALUE, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}
	else
	{

		// Look up the command
		TSVNCommand command = cmdAbout;		// Something harmless as a default
		for(int nCommand = 0; nCommand < (sizeof(commandInfo)/sizeof(commandInfo[0])); nCommand++)
		{
			if(comVal.Compare(commandInfo[nCommand].pCommandName) == 0)
			{
				// We've found the command
				command = commandInfo[nCommand].command;
				// If this fires, you've let the enum get out of sync with the commandInfo array
				ASSERT((int)command == nCommand);
				break;
			}
		}

		bool bPathIsTempfile = false;
		if(commandInfo[command].bPathIsTempFileByDefault)
		{
			// The path argument is probably a temporary file containing path names, unless
			// the notempfile argument is present
			bPathIsTempfile = !parser.HasKey(_T("notempfile"));
		}

		CString sPathArgument = CUtils::GetLongPathname(parser.GetVal(_T("path")));
		CTSVNPath cmdLinePath(sPathArgument);

		CTSVNPathList pathList;
		if(bPathIsTempfile)
		{
			if (pathList.LoadFromTemporaryFile(cmdLinePath)==false)
				return FALSE;		// no path specified!
			if(cmdLinePath.GetFileExtension().CompareNoCase(_T(".tmp")) == 0)
			{
				// We can delete the temporary path file, now that we've loaded it
				::DeleteFile(cmdLinePath.GetWinPath());
			}
			// This was a path to a temporary file - it's got no meaning now, and
			// anybody who uses it again is in for a problem...
			cmdLinePath.Reset();
		}
		else
		{
			pathList.LoadFromAsteriskSeparatedString(sPathArgument);
		}
		
		if (command == cmdTest)
		{
			CMessageBox::Show(NULL, _T("Test command successfully executed"), _T("Info"), MB_OK);
			return FALSE;
		} // if (comVal.Compare(_T("test"))==0)

		CString sVal = parser.GetVal(_T("hwnd"));
		hWndExplorer = (HWND)_ttoi64(sVal);

		while (GetParent(hWndExplorer)!=NULL)
			hWndExplorer = GetParent(hWndExplorer);
		if (!IsWindow(hWndExplorer))
		{
			hWndExplorer = NULL;
		}
		
		// Subversion sometimes writes temp files to the current directory!
		// so here, we check if we can write to the cwd, and if not, set the cwd
		// to the temp directory
		// We use a GUID here for the filename - this should be safe enough to do.
		HANDLE hFile = ::CreateFile(_T("{E80E91D7-D534-47a6-AEAC-239F5553E173}"), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			if (cmdLinePath.IsEmpty())
			{
				TCHAR pathbuf[MAX_PATH];
				GetTempPath(MAX_PATH, pathbuf);
				SetCurrentDirectory(pathbuf);		
			}
			else
			{
				SetCurrentDirectory(cmdLinePath.GetDirectory().GetWinPath());
			}
		}
		else
		{
			CloseHandle(hFile);
			DeleteFile(_T("{E80E91D7-D534-47a6-AEAC-239F5553E173}"));
		}

// check for newer versions
		if (CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewer"), TRUE) != FALSE)
		{
			time_t now;
			struct tm ptm;

			time(&now);
			if (localtime_s(&ptm, &now)==0)
			{
				int week = 0;
				week = ptm.tm_yday / 7;

				CRegDWORD oldweek = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewerWeek"), (DWORD)-1);
				if (((DWORD)oldweek) == -1)
					oldweek = week;
				else
				{
					if ((DWORD)week != oldweek)
					{
						oldweek = week;

						TCHAR com[MAX_PATH+100];
						GetModuleFileName(NULL, com, MAX_PATH);
						_tcscat_s(com, MAX_PATH+100, _T(" /command:updatecheck"));

//BUGBUG - Should this really have an error message string resource ID?
						CUtils::LaunchApplication(com, 0, false);
					}
				}
			}
		}

		if (parser.HasVal(_T("configdir")))
		{
			CString sConfigDir = parser.GetVal(_T("configdir"));
			g_SVNGlobal.SetConfigDir(sConfigDir);
		}

		//#region crash
		if (command == cmdCrash)
		{
			CMessageBox::Show(NULL, _T("You are testing the crashhandler.\n<ct=0x0000FF>Do NOT send the crashreport!!!!</ct>"), _T("TortoiseSVN"), MB_ICONINFORMATION);
			CrashProgram();
			CMessageBox::Show(NULL, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
			return FALSE;
		}
		//#endregion
		HANDLE TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));	
		{
			CString err = SVN::CheckConfigFile();
			if (!err.IsEmpty())
			{
				CMessageBox::Show(EXPLORERHWND, err, _T("TortoiseSVN"), MB_ICONERROR);
				// Normally, we give-up and exit at this point, but there is a trap here
				// in that the user might need to use the settings dialog to edit the config file.
				if(command != cmdSettings)
				{
					return FALSE;
				}
			}
		}

		//#region about
		if (command == cmdAbout)
		{
			CAboutDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
		//#endregion
		//#region rtfm
		if (command == cmdRTFM)
		{
			CMessageBox::Show(EXPLORERHWND, IDS_PROC_RTFM, IDS_APPNAME, MB_ICONINFORMATION);
			TCHAR path[MAX_PATH];
			SHGetFolderPath(EXPLORERHWND, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path);
			ShellExecute(0,_T("explore"),path,NULL,NULL,SW_SHOWNORMAL);
		}
		//#endregion
		//#region log
		if (command == cmdLog)
		{
			//the log command line looks like this:
			//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [revstart:<startrevision>] [revend:<endrevision>]
			CString val = parser.GetVal(_T("revstart"));
			long revstart = _tstol(val);
			val = parser.GetVal(_T("revend"));
			long revend = _tstol(val);
			val = parser.GetVal(_T("limit"));
			int limit = _tstoi(val);
			if (revstart == 0)
			{
				revstart = SVNRev::REV_HEAD;
			}
			if ((limit == 0)&&(revend == 0))
			{
				CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
				limit = (int)(LONG)reg;
			}
			if (revend == 0)
			{
				revend = 1;
			}
			BOOL bStrict = (DWORD)CRegDWORD(_T("Software\\TortoiseSVN\\LastLogStrict"), FALSE);
			if (parser.HasKey(_T("strict")))
			{
				bStrict = TRUE;
			}
			CLogDlg dlg;
			m_pMainWnd = &dlg;
			dlg.SetParams(cmdLinePath, revstart, revend, limit, bStrict);
			val = parser.GetVal(_T("propspath"));
			if (!val.IsEmpty())
				dlg.SetProjectPropertiesPath(CTSVNPath(val));
			dlg.DoModal();			
		}
		//#endregion
		//#region checkout
		if (command == cmdCheckout)
		{
			//
			// Get the directory supplied in the command line. If there isn't
			// one then we should use the current working directory instead.
			CTSVNPath checkoutDirectory;
			if (cmdLinePath.IsEmpty())
			{
				DWORD len = ::GetCurrentDirectory(0, NULL);
				TCHAR * tszPath = new TCHAR[len];
				::GetCurrentDirectory(len, tszPath);
				checkoutDirectory.SetFromWin(tszPath, true);
				delete tszPath;
			}
			else
			{
				checkoutDirectory = cmdLinePath;
			}

			//
			// Create a checkout dialog and display it. If the user clicks OK,
			// we should create an SVN progress dialog, set it as the main
			// window and then display it.
			//
			CCheckoutDlg dlg;
			dlg.m_strCheckoutDirectory = cmdLinePath.GetDirectory().GetWinPathString();
			dlg.m_URL = parser.GetVal(_T("url"));
			if (dlg.m_URL.Left(5).Compare(_T("tsvn:"))==0)
			{
				dlg.m_URL = dlg.m_URL.Mid(5);
			}
			if (parser.HasKey(_T("revision")))
			{
				LONG lRev = parser.GetLongVal(_T("revision"));
				dlg.Revision = lRev;
			}
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_URL);
				TRACE(_T("checkout dir = %s \n"), (LPCTSTR)dlg.m_strCheckoutDirectory);

				checkoutDirectory.SetFromWin(dlg.m_strCheckoutDirectory, true);

				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				int opts = dlg.m_bNonRecursive ? ProgOptNonRecursive : ProgOptRecursive;
				if (dlg.m_bNoExternals)
					opts |= ProgOptIgnoreExternals;
				progDlg.SetParams(CSVNProgressDlg::Checkout, opts, CTSVNPathList(checkoutDirectory), dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region import
		if (command == cmdImport)
		{
			CImportDlg dlg;
			dlg.m_path = cmdLinePath;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), (LPCTSTR)dlg.m_url);
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Import, dlg.m_bIncludeIgnored ? ProgOptIncludeIgnored : 0, pathList, dlg.m_url, dlg.m_sMessage);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region update
		if (command == cmdUpdate)
		{
			SVNRev rev = SVNRev(_T("HEAD"));
			int options = ProgOptRecursive;
			if (parser.HasKey(_T("rev")))
			{
				CUpdateDlg dlg;
				if (pathList.GetCount()>0)
					dlg.m_wcPath = pathList[0];
				if (dlg.DoModal() == IDOK)
				{
					rev = dlg.Revision;
					if (dlg.m_bNonRecursive)
					{
						options = ProgOptNonRecursive;
					}
					if (dlg.m_bNoExternals)
					{
						options |= ProgOptIgnoreExternals;
					}
					if (CRegDWORD(_T("Software\\TortoiseSVN\\updatetorev"), (DWORD)-1)==(DWORD)-1)
						if (SVNStatus::GetAllStatusRecursive(cmdLinePath)>svn_wc_status_normal)
							if (CMessageBox::ShowCheck(EXPLORERHWND, IDS_WARN_UPDATETOREV_WITHMODS, IDS_APPNAME, MB_OKCANCEL | MB_ICONWARNING, _T("updatetorev"), IDS_MSGBOX_DONOTSHOWAGAIN)!=IDOK)
								return FALSE;
				}
				else 
					return FALSE;
			}
			CSVNProgressDlg progDlg;
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Update, options, pathList, _T(""), _T(""), rev);
			progDlg.DoModal();
		}
		//#endregion
		//#region commit
		if (command == cmdCommit)
		{
			CLogPromptDlg dlg;
			if (parser.HasKey(_T("logmsg")))
			{
				dlg.m_sLogMessage = parser.GetVal(_T("logmsg"));
			}
			if (parser.HasKey(_T("logmsgfile")))
			{
				CString logmsgfile = parser.GetVal(_T("logmsgfile"));
				if (PathFileExists(logmsgfile))
				{
					try
					{
						CStdioFile msgfile;
						if (msgfile.Open(logmsgfile, CFile::modeRead))
						{
							CStringA filecontent;
							int filelength = (int)msgfile.GetLength();
							int bytesread = (int)msgfile.Read(filecontent.GetBuffer(filelength), filelength);
							filecontent.ReleaseBuffer(bytesread);
							dlg.m_sLogMessage = CUnicodeUtils::GetUnicode(filecontent);
							msgfile.Close();
						}
					} 
					catch (CFileException* /*pE*/)
					{
						dlg.m_sLogMessage.Empty();
					}
				}
			}
			if (parser.HasKey(_T("bugid")))
			{
				dlg.m_sBugID = parser.GetVal(_T("bugid"));
			}
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				if (dlg.m_pathList.GetCount()==0)
					return FALSE;
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Commit, dlg.m_bKeepLocks ? ProgOptKeeplocks : 0, dlg.m_pathList, _T(""), dlg.m_sLogMessage, !dlg.m_bRecursive);
				progDlg.DoModal();
				CRegDWORD err = CRegDWORD(_T("Software\\TortoiseSVN\\ErrorOccurred"), FALSE);
				err = (DWORD)progDlg.DidErrorsOccur();
			}
		}
		//#endregion
		//#region add
		if (command == cmdAdd)
		{
			CAddDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				if (dlg.m_pathList.GetCount() == 0)
					return FALSE;
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Add, 0, dlg.m_pathList);
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK) // if (dlg.DoModal() == IDOK) 
		}
		//#endregion
		//#region revert
		if (command == cmdRevert)
		{
			CRevertDlg dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal() == IDOK)
			{
				if (dlg.m_pathList.GetCount() == 0)
					return FALSE;
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				int options = (dlg.m_bRecursive ? ProgOptRecursive : ProgOptNonRecursive);
				progDlg.SetParams(CSVNProgressDlg::Revert, options, dlg.m_pathList);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region cleanup
		if (command == cmdCleanup)
		{
			CProgressDlg progress;
			progress.SetTitle(IDS_PROC_CLEANUP);
			progress.ShowModeless(PWND);
			progress.SetAnimation(IDR_CLEANUPANI);
			SVN svn;
			if (!svn.CleanUp(cmdLinePath))
			{
				progress.Stop();
				CString errorMessage;
				errorMessage.Format(IDS_ERR_CLEANUP, (LPCTSTR)svn.GetLastErrorMessage());
				CMessageBox::Show(EXPLORERHWND, errorMessage, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				// after the cleanup has finished, crawl the path downwards and send a change
				// notification for every directory to the shell. This will update the
				// overlays in the left treeview of the explorer.
				CDirFileEnum crawler(cmdLinePath.GetWinPathString());
				CString sPath;
				bool bDir = false;
				CTSVNPathList updateList;
				while (crawler.NextFile(sPath, &bDir))
				{
					if ((bDir) && (!g_SVNAdminDir.IsAdminDirPath(sPath)))
					{
						updateList.AddPath(CTSVNPath(sPath));
					}
				}
				updateList.AddPath(cmdLinePath);
				CShellUpdater::Instance().AddPathsForUpdate(updateList);
				CShellUpdater::Instance().Flush();
				updateList.SortByPathname(true);
				for (INT_PTR i=0; i<updateList.GetCount(); ++i)
				{
					SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, updateList[i].GetWinPath(), NULL);
					ATLTRACE("notify change for path %ws\n", updateList[i].GetWinPath());
				}
				
				progress.Stop();
				CMessageBox::Show(EXPLORERHWND, IDS_PROC_CLEANUPFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region resolve
		if (command == cmdResolve)
		{
			CResolveDlg dlg;
			dlg.m_pathList = pathList;
			INT_PTR ret = IDOK;
			if (!parser.HasKey(_T("noquestion")))
				ret = dlg.DoModal();
			if (ret == IDOK)
			{
				if (dlg.m_pathList.GetCount())
				{
					CSVNProgressDlg progDlg(PWND);
					progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
					m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Resolve, 0, dlg.m_pathList);
					progDlg.DoModal();
				}
			}
		}
		//#endregion
		//#region repocreate
		if (command == cmdRepoCreate)
		{
			CRepoCreateDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				if ((dlg.RepoType.Compare(_T("bdb"))==0)&&(GetDriveType(cmdLinePath.GetRootPathString())==DRIVE_REMOTE))
				{
					if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATESHAREWARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
					{
						if (!SVN::CreateRepository(cmdLinePath.GetWinPathString(), dlg.RepoType))
						{
							CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
						}
						else
						{
							CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
						}
					} 
				}
				else
				{
					if (!SVN::CreateRepository(cmdLinePath.GetWinPathString(), dlg.RepoType))
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
					}
					else
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
					}
				}
			}
		} // if (comVal.Compare(_T("repocreate"))==0) 
		//#endregion
		//#region switch
		if (command == cmdSwitch)
		{
			CSwitchDlg dlg;
			dlg.m_path = cmdLinePath.GetWinPathString();

			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				m_pMainWnd = &progDlg;
				progDlg.SetParams(CSVNProgressDlg::Switch, 0, CTSVNPathList(cmdLinePath), dlg.m_URL, _T(""), dlg.Revision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region export
		if (command == cmdExport)
		{
			TCHAR saveto[MAX_PATH];
			bool bURL = !!SVN::PathIsURL(cmdLinePath.GetSVNPathString());
			if ((bURL)||(SVNStatus::GetAllStatus(cmdLinePath) == svn_wc_status_unversioned))
			{
				CCheckoutDlg dlg;
				if (bURL)
					dlg.m_URL = cmdLinePath.GetSVNPathString();
				else
					dlg.m_strCheckoutDirectory = cmdLinePath.GetWinPathString();
				dlg.IsExport = TRUE;
				if (dlg.DoModal() == IDOK)
				{
					CTSVNPath checkoutPath(dlg.m_strCheckoutDirectory);

					CSVNProgressDlg progDlg;
					progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
					m_pMainWnd = &progDlg;
					progDlg.SetParams(CSVNProgressDlg::Export, dlg.m_bNoExternals ? ProgOptIgnoreExternals : 0, CTSVNPathList(checkoutPath), dlg.m_URL, _T(""), dlg.Revision);
					progDlg.DoModal();
				}
			}
			else
			{
				CBrowseFolder folderBrowser;
				CString strTemp;
				strTemp.LoadString(IDS_PROC_EXPORT_1);
				folderBrowser.SetInfo(strTemp);
				folderBrowser.m_style = BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
				strTemp.LoadString(IDS_PROC_EXPORT_2);
				folderBrowser.SetCheckBoxText(strTemp);
				strTemp.LoadString(IDS_PROC_OMMITEXTERNALS);
				folderBrowser.SetCheckBoxText2(strTemp);
				CRegDWORD regExtended = CRegDWORD(_T("Software\\TortoiseSVN\\ExportExtended"), FALSE);
				CBrowseFolder::m_bCheck = regExtended;
				if (folderBrowser.Show(EXPLORERHWND, saveto, MAX_PATH)==CBrowseFolder::OK)
				{
					CString saveplace = CString(saveto);
					saveplace += _T("\\") + cmdLinePath.GetFileOrDirectoryName();
					TRACE(_T("export %s to %s\n"), (LPCTSTR)cmdLinePath.GetUIPathString(), (LPCTSTR)saveto);
					SVN svn;
					if (!svn.Export(cmdLinePath, CTSVNPath(saveplace), bURL ? SVNRev::REV_HEAD : SVNRev::REV_WC, bURL ? SVNRev::REV_HEAD : SVNRev::REV_WC, TRUE, folderBrowser.m_bCheck2, EXPLORERHWND, folderBrowser.m_bCheck))
					{
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
					}
					else
					{
						CString strMessage;
						strMessage.Format(IDS_PROC_EXPORT_4, (LPCTSTR)cmdLinePath.GetUIPathString(), (LPCTSTR)saveplace);
						CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
					}
					regExtended = CBrowseFolder::m_bCheck;
				}
			}
		}
		//#endregion
		//#region merge
		if (command == cmdMerge)
		{
			BOOL repeat = FALSE;
			CMergeDlg dlg;
			dlg.m_wcPath = cmdLinePath;
			do 
			{	
				if (dlg.DoModal() == IDOK)
				{
					CSVNProgressDlg progDlg;
					progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
					//m_pMainWnd = &progDlg;
					int options = dlg.m_bDryRun ? ProgOptDryRun : 0;
					options |= dlg.m_bIgnoreAncestry ? ProgOptIgnoreAncestry : 0;
					progDlg.SetParams(CSVNProgressDlg::Merge, options, pathList, dlg.m_URLFrom, dlg.m_URLTo, dlg.StartRev);		//use the message as the second url
					progDlg.m_RevisionEnd = dlg.EndRev;
					progDlg.DoModal();
					repeat = dlg.m_bDryRun;
					dlg.bRepeating = TRUE;
				}
				else
					repeat = FALSE;
			} while(repeat);
		}
		//#endregion
		//#region copy
		if (command == cmdCopy)
		{
			CCopyDlg dlg;
			dlg.m_path = cmdLinePath;
			if (dlg.DoModal() == IDOK)
			{
				m_pMainWnd = NULL;
				TRACE(_T("copy %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)dlg.m_URL);
				CSVNProgressDlg progDlg;
				progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
				progDlg.SetParams(CSVNProgressDlg::Copy, dlg.m_bDoSwitch ? ProgOptSwitchAfterCopy : 0, pathList, dlg.m_URL, dlg.m_sLogMessage, dlg.m_CopyRev);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region settings
		if (command == cmdSettings)
		{
			CSettings dlg(IDS_PROC_SETTINGS_TITLE);
			dlg.SetTreeViewMode(TRUE, TRUE, TRUE);
			dlg.SetTreeWidth(180);

			if (dlg.DoModal()==IDOK)
			{
				dlg.SaveData();
			}
		}
		//#endregion
		//#region remove
		if (command == cmdRemove)
		{
			BOOL bForce = FALSE;
			SVN svn;
			if ((pathList.GetCount())&&(SVN::PathIsURL(pathList[0].GetSVNPathString())))
			{
				// Delete using URL's, not wc paths
				svn.SetPromptApp(&theApp);
				CInputDlg dlg;
				dlg.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
				CUtils::RemoveAccelerators(dlg.m_sHintText);
				dlg.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
				CUtils::RemoveAccelerators(dlg.m_sTitle);
				dlg.m_sInputText.LoadString(IDS_INPUT_REMOVELOGMSG);
				CUtils::RemoveAccelerators(dlg.m_sInputText);
				if (dlg.DoModal()==IDOK)
				{
					if (!svn.Remove(pathList, TRUE, dlg.m_sInputText))
					{
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return FALSE;
					}
				}
				return FALSE;
			}
			else
			{
				for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
				{
					TRACE(_T("remove file %s\n"), (LPCTSTR)pathList[nPath].GetUIPathString());
					// even though SVN::Remove takes a list of paths to delete at once
					// we delete each item individually so we can prompt the user
					// if something goes wrong or unversioned/modified items are
					// to be deleted
					CTSVNPathList removePathList(pathList[nPath]);
					if (!svn.Remove(removePathList, bForce))
					{
						if ((svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE) ||
							(svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
						{
							CString msg, yes, no, yestoall;
							msg.Format(IDS_PROC_REMOVEFORCE, svn.GetLastErrorMessage());
							yes.LoadString(IDS_MSGBOX_YES);
							no.LoadString(IDS_MSGBOX_NO);
							yestoall.LoadString(IDS_PROC_YESTOALL);
							UINT ret = CMessageBox::Show(EXPLORERHWND, msg, _T("TortoiseSVN"), 2, IDI_ERROR, yes, no, yestoall);
							if (ret == 3)
								bForce = TRUE;
							if ((ret == 1)||(ret==3))
								if (!svn.Remove(removePathList, TRUE))
								{
									CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
						}
						else
							CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
				}
			}
		}
		//#endregion
		//#region rename
		if (command == cmdRename)
		{
			CRenameDlg dlg;
			CString filename = cmdLinePath.GetFileOrDirectoryName();
			CString basePath = cmdLinePath.GetContainingDirectory().GetWinPathString();
			::SetCurrentDirectory(basePath);
			dlg.m_name = filename;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("rename file %s to %s\n"), (LPCTSTR)cmdLinePath.GetWinPathString(), (LPCTSTR)dlg.m_name);
				CTSVNPath destinationPath(basePath);
				if (PathIsRelative(dlg.m_name) && !PathIsURL(dlg.m_name))
					destinationPath.AppendPathString(dlg.m_name);
				else
					destinationPath.SetFromWin(dlg.m_name);
				if (cmdLinePath.IsEquivalentTo(destinationPath))
				{
					//rename to the same file!
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_CASERENAME, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					CString sMsg;
					if (SVN::PathIsURL(cmdLinePath.GetSVNPathString()))
					{
						// rename an URL.
						// Ask for a commit message, then rename directly in
						// the repository
						CInputDlg input;
						input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
						CUtils::RemoveAccelerators(input.m_sHintText);
						input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
						CUtils::RemoveAccelerators(input.m_sTitle);
						input.m_sInputText.LoadString(IDS_INPUT_MOVELOGMSG);
						if (input.DoModal() == IDOK)
						{
							sMsg = input.m_sInputText;
						}
						else
						{
							return FALSE;
						}
					}
					if ((cmdLinePath.IsDirectory())||(pathList.GetCount() > 1))
					{
						// renaming a directory can take a while: use the
						// progress dialog to show the progress of the renaming
						// operation.
						CSVNProgressDlg progDlg;
						progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
						progDlg.SetParams(CSVNProgressDlg::Rename, 0, pathList, destinationPath.GetWinPathString(), sMsg, SVNRev::REV_WC);
						progDlg.DoModal();
					}
					else
					{
						SVN svn;
						CString sFilemask = cmdLinePath.GetFilename();
						if (sFilemask.Find('.')>=0)
						{
							sFilemask = sFilemask.Left(sFilemask.Find('.'));
						}
						else
							sFilemask.Empty();
						CString sNewMask = destinationPath.GetFilename();
						if (sNewMask.Find('.'>=0))
						{
							sNewMask = sNewMask.Left(sNewMask.Find('.'));
						}
						else
							sNewMask.Empty();

						if (((!sFilemask.IsEmpty()) && (parser.HasKey(_T("noquestion")))) ||
							(cmdLinePath.GetFileExtension().Compare(destinationPath.GetFileExtension())!=0))
						{
							if (!svn.Move(cmdLinePath, destinationPath, TRUE, sMsg))
							{
								TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
								CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
							}
						}
						else
						{
							// when refactoring, multiple files have to be renamed
							// at once because those files belong together.
							// e.g. file.aspx, file.aspx.cs, file.aspx.resx
							CTSVNPathList renlist;
							CSimpleFileFind filefind(cmdLinePath.GetDirectory().GetWinPathString(), sFilemask+_T(".*"));
							while (filefind.FindNextFileNoDots())
							{
								if (!filefind.IsDirectory())
									renlist.AddPath(CTSVNPath(filefind.GetFilePath()));
							}
							if (renlist.GetCount()<=1)
							{
								// we couldn't find any other matching files
								// just do the default...
								if (!svn.Move(cmdLinePath, destinationPath, TRUE, sMsg))
								{
									TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
									CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								}
							}
							else
							{
								std::map<CString, CString> renmap;
								CString sTemp;
								CString sRenList;
								for (int i=0; i<renlist.GetCount(); ++i)
								{
									CString sFilename = renlist[i].GetFilename();
									CString sNewFilename = sNewMask + sFilename.Mid(sFilemask.GetLength());
									sTemp.Format(_T("\n%s -> %s"), sFilename, sNewFilename);
									sRenList += sTemp;
									renmap[renlist[i].GetWinPathString()] = renlist[i].GetContainingDirectory().GetWinPathString()+_T("\\")+sNewFilename;
								}
								CString sRenameMultipleQuestion;
								sRenameMultipleQuestion.Format(IDS_PROC_MULTIRENAME, sRenList);
								UINT idret = CMessageBox::Show(EXPLORERHWND, sRenameMultipleQuestion, _T("TortoiseSVN"), MB_ICONQUESTION|MB_YESNO);
								if (idret == IDYES)
								{
									CProgressDlg progress;
									progress.SetTitle(IDS_PROC_MOVING);
									progress.SetAnimation(IDR_MOVEANI);
									progress.SetTime(true);
									progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
									DWORD count = 1;
									for (std::map<CString, CString>::iterator it=renmap.begin(); it != renmap.end(); ++it)
									{
										progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, it->first);
										progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, it->second);
										progress.SetProgress(count, renmap.size());
										if (!svn.Move(CTSVNPath(it->first), CTSVNPath(it->second), TRUE, sMsg))
										{
											TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
											CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
										}
									}
									progress.Stop();
								}
								else if (idret == IDNO)
								{
									// no, user wants to just rename the file he selected
									if (!svn.Move(cmdLinePath, destinationPath, TRUE, sMsg))
									{
										TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
										CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
									}
								}
							}
						}
					} // else from if ((cmdLinePath.IsDirectory())||(pathList.GetCount() > 1))
				} // if (cmdLinePath.IsEquivalentTo(destinationPath))
			} // if (dlg.DoModal() == IDOK)
		}
		//#endregion
		//#region diff
		if (command == cmdDiff)
		{
			CString path2 = CUtils::GetLongPathname(parser.GetVal(_T("path2")));
			if (path2.IsEmpty())
			{
				if (cmdLinePath.IsDirectory())
				{
					CChangedDlg dlg;
					dlg.m_pathList = CTSVNPathList(cmdLinePath);
					dlg.DoModal();
				}
				else
				{
					CTSVNPath temporaryFile;
					SVNDiff diff;
					diff.DiffFileAgainstBase(cmdLinePath);
				}
			} 
			else
				CUtils::StartExtDiff(CTSVNPath(path2), cmdLinePath);
		}
		//#endregion
		//#region dropcopyadd
		if (command == cmdDropCopyAdd)
		{
			CString droppath = parser.GetVal(_T("droptarget"));
			if (CTSVNPath(droppath).IsAdminDir())
				return FALSE;

			pathList.RemoveAdminPaths();
			CTSVNPathList copiedFiles;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				if (!pathList[nPath].IsEquivalentTo(CTSVNPath(droppath)))
				{
					//copy the file to the new location
					CString name = pathList[nPath].GetFileOrDirectoryName();
					if (::PathFileExists(droppath+_T("\\")+name))
					{
						CString strMessage;
						strMessage.Format(IDS_PROC_OVERWRITE_CONFIRM, (LPCTSTR)(droppath+_T("\\")+name));
						int ret = CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_YESNOCANCEL | MB_ICONQUESTION);
						if (ret == IDYES)
						{
							if (!::CopyFile(pathList[nPath].GetWinPath(), droppath+_T("\\")+name, FALSE))
							{
								//the copy operation failed! Get out of here!
								LPVOID lpMsgBuf;
								FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
									FORMAT_MESSAGE_FROM_SYSTEM | 
									FORMAT_MESSAGE_IGNORE_INSERTS,
									NULL,
									GetLastError(),
									MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
									(LPTSTR) &lpMsgBuf,
									0,
									NULL 
									);
								CString strMessage;
								strMessage.Format(IDS_ERR_COPYFILES, (LPTSTR)lpMsgBuf);
								CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								LocalFree( lpMsgBuf );
								return FALSE;
							}
						}
						if (ret == IDCANCEL)
						{
							return FALSE;		//cancel the whole operation
						}
					}
					else if (!CopyFile(pathList[nPath].GetWinPath(), droppath+_T("\\")+name, FALSE))
					{
						//the copy operation failed! Get out of here!
						LPVOID lpMsgBuf;
						FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM | 
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							GetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPTSTR) &lpMsgBuf,
							0,
							NULL 
							);
						CString strMessage;
						strMessage.Format(IDS_ERR_COPYFILES, lpMsgBuf);
						CMessageBox::Show(EXPLORERHWND, strMessage, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
						LocalFree( lpMsgBuf );
						return FALSE;
					}
					copiedFiles.AddPath(CTSVNPath(droppath+_T("\\")+name));		//add the new filepath
				}
			}
			//now add all the newly copied files to the working copy
			CSVNProgressDlg progDlg;
			progDlg.m_dwCloseOnEnd = parser.GetLongVal(_T("closeonend"));
			m_pMainWnd = &progDlg;
			progDlg.SetParams(CSVNProgressDlg::Add, 0, copiedFiles);
			progDlg.DoModal();
		}
		//#endregion
		//#region dropmove
		if (command == cmdDropMove)
		{
			CString droppath = parser.GetVal(_T("droptarget"));
			if (CTSVNPath(droppath).IsAdminDir())
				return FALSE;
			SVN svn;
			unsigned long count = 0;
			pathList.RemoveAdminPaths();
			CString sNewName;
			if ((parser.HasKey(_T("rename")))&&(pathList.GetCount()==1))
			{
				// ask for a new name of the source item
				CRenameDlg renDlg;
				renDlg.m_name = pathList[0].GetFileOrDirectoryName();
				if (renDlg.DoModal() != IDOK)
				{
					return FALSE;
				}
				sNewName = renDlg.m_name;
			}
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_MOVING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.SetTime(true);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				CTSVNPath destPath;
				if (sNewName.IsEmpty())
					destPath = CTSVNPath(droppath+_T("\\")+pathList[nPath].GetFileOrDirectoryName());
				else
					destPath = CTSVNPath(droppath+_T("\\")+sNewName);
				if (destPath.Exists())
				{
					CString name = pathList[nPath].GetFileOrDirectoryName();
					if (!sNewName.IsEmpty())
						name = sNewName;
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_NEWNAMEMOVE, (LPCTSTR)name);
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					destPath.SetFromWin(droppath+_T("\\")+dlg.m_name);
				} 
				if (!svn.Move(pathList[nPath], destPath, FALSE))
				{
					if (svn.Err && (svn.Err->apr_err == SVN_ERR_UNVERSIONED_RESOURCE ||
						svn.Err->apr_err == SVN_ERR_CLIENT_MODIFIED))
					{
						// file/folder seems to have local modifications. Ask the user if
						// a force is requested.
						CString temp = svn.GetLastErrorMessage();
						CString sQuestion(MAKEINTRESOURCE(IDS_PROC_FORCEMOVE));
						temp += _T("\n") + sQuestion;
						if (CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_YESNO)==IDYES)
						{
							if (!svn.Move(pathList[nPath], destPath, TRUE))
							{
								CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
								return FALSE;		//get out of here
							}
						}
					}
					else
					{
						TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						return FALSE;		//get out of here
					}
				} 
				count++;
				if (progress.IsValid())
				{
					progress.FormatPathLine(1, IDS_PROC_MOVINGPROG, pathList[nPath].GetWinPath());
					progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, destPath.GetWinPath());
					progress.SetProgress(count, pathList.GetCount());
				}
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region dropexport
		if (command == cmdDropExport)
		{
			CString droppath = parser.GetVal(_T("droptarget"));
			if (CTSVNPath(droppath).IsAdminDir())
				return FALSE;
			SVN svn;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				CString dropper = droppath + _T("\\") + pathList[nPath].GetFileOrDirectoryName();
				if (PathFileExists(dropper))
					dropper.Format(IDS_PROC_EXPORTFOLDERNAME, droppath, pathList[nPath].GetFileOrDirectoryName());
				int exportcount = 1;
				while (PathFileExists(dropper))
				{
					dropper.Format(IDS_PROC_EXPORTFOLDERNAME2, droppath, exportcount++, pathList[nPath].GetFileOrDirectoryName());
				}
				if (!svn.Export(pathList[nPath], CTSVNPath(dropper), SVNRev::REV_WC ,SVNRev::REV_WC, TRUE, FALSE, EXPLORERHWND, parser.HasKey(_T("extended"))))
				{
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
				}
			}
		}
		//#endregion
		//#region dropcopy
		if (command == cmdDropCopy)
		{
			CString sDroppath = parser.GetVal(_T("droptarget"));
			if (CTSVNPath(sDroppath).IsAdminDir())
				return FALSE;
			SVN svn;
			unsigned long count = 0;
			CString sNewName;
			pathList.RemoveAdminPaths();
			if ((parser.HasKey(_T("rename")))&&(pathList.GetCount()==1))
			{
				// ask for a new name of the source item
				CRenameDlg renDlg;
				renDlg.m_name = pathList[0].GetFileOrDirectoryName();
				if (renDlg.DoModal() != IDOK)
				{
					return FALSE;
				}
				sNewName = renDlg.m_name;
			}
			CProgressDlg progress;
			if (progress.IsValid())
			{
				progress.SetTitle(IDS_PROC_COPYING);
				progress.SetAnimation(IDR_MOVEANI);
				progress.SetTime(true);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				const CTSVNPath& sourcePath = pathList[nPath];

				CTSVNPath fullDropPath(sDroppath);
				if (sNewName.IsEmpty())
					fullDropPath.AppendPathString(sourcePath.GetFileOrDirectoryName());
				else
					fullDropPath.AppendPathString(sNewName);
				
				// Check for a drop-on-to-ourselves
				if (sourcePath.IsEquivalentTo(fullDropPath))
				{
					// Offer a rename
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_NEWNAMECOPY, (LPCTSTR)sourcePath.GetFileOrDirectoryName());
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					// Rebuild the destination path, with the new name
					fullDropPath.SetFromUnknown(sDroppath);
					fullDropPath.AppendPathString(dlg.m_name);
				} 
				if (!svn.Copy(sourcePath, fullDropPath, SVNRev::REV_WC, _T("")))
				{
					TRACE(_T("%s\n"), (LPCTSTR)svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.FormatPathLine(1, IDS_PROC_COPYINGPROG, sourcePath.GetWinPath());
					progress.FormatPathLine(2, IDS_PROC_CPYMVPROG2, fullDropPath.GetWinPath());
					progress.SetProgress(count, pathList.GetCount());
				} // if (progress.IsValid())
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region conflicteditor
		if (command == cmdConflictEditor)
		{
			SVNDiff::StartConflictEditor(cmdLinePath);
		} 
		//#endregion
		//#region relocate
		if (command == cmdRelocate)
		{
			SVN svn;
			CRelocateDlg dlg;
			dlg.m_sFromUrl = svn.GetUIURLFromPath(cmdLinePath);
			dlg.m_sToUrl = dlg.m_sFromUrl;

			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("relocate from %s to %s\n"), (LPCTSTR)dlg.m_sFromUrl, (LPCTSTR)dlg.m_sToUrl);
				if (CMessageBox::Show((EXPLORERHWND), IDS_WARN_RELOCATEREALLY, IDS_APPNAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2)==IDYES)
				{
					SVN s;

					CProgressDlg progress;
					if (progress.IsValid())
					{
						progress.SetTitle(IDS_PROC_RELOCATING);
						progress.ShowModeless(PWND);
					}
					if (!s.Relocate(cmdLinePath, CTSVNPath(dlg.m_sFromUrl), CTSVNPath(dlg.m_sToUrl), TRUE))
					{
						progress.Stop();
						TRACE(_T("%s\n"), (LPCTSTR)s.GetLastErrorMessage());
						CMessageBox::Show((EXPLORERHWND), s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					}
					else
					{
						progress.Stop();
						CString strMessage;
						strMessage.Format(IDS_PROC_RELOCATEFINISHED, (LPCTSTR)dlg.m_sToUrl);
						CMessageBox::Show((EXPLORERHWND), strMessage, _T("TortoiseSVN"), MB_ICONINFORMATION);
					}
				}
			}
		} // if (comVal.Compare(_T("relocate"))==0)
		//#endregion
		//#region help
		if (command == cmdHelp)
		{
			ShellExecute(EXPLORERHWND, _T("open"), m_pszHelpFilePath, NULL, NULL, SW_SHOWNORMAL);
		}
		//#endregion
		//#region repostatus
		if (command == cmdRepoStatus)
		{
			CChangedDlg dlg;
			dlg.m_pathList = pathList;
			dlg.DoModal();
		} // if (comVal.Compare(_T("repostatus"))==0)
		//#endregion 
		//#region repobrowser
		if (command == cmdRepoBrowser)
		{
			CString url;
			BOOL bFile = FALSE;
			SVN svn;
			if (!cmdLinePath.IsEmpty())
			{
				if (cmdLinePath.GetSVNPathString().Left(4).CompareNoCase(_T("svn:"))==0)
				{
					// If the path starts with "svn:" and there is another protocol
					// found in the path (a "://" found after the "svn:") then
					// remove "svn:" from the beginning of the path.
					if (cmdLinePath.GetSVNPathString().Find(_T("://"), 4)>=0)
						cmdLinePath.SetFromSVN(cmdLinePath.GetSVNPathString().Mid(4));
				}

				url = svn.GetURLFromPath(cmdLinePath);

				if (url.IsEmpty())
				{
					if (SVN::PathIsURL(cmdLinePath.GetSVNPathString()))
						url = cmdLinePath.GetSVNPathString();
					else if (svn.IsRepository(cmdLinePath.GetWinPathString()))
					{
						// The path points to a local repository.
						// Add 'file:///' so the repository browser recognizes
						// it as an URL to the local repository.
						url = _T("file:///")+cmdLinePath.GetWinPathString();
					}
				}
			}
			if (cmdLinePath.GetUIPathString().Left(8).CompareNoCase(_T("file:///"))==0)
			{
				cmdLinePath.SetFromUnknown(cmdLinePath.GetUIPathString().Mid(8));
			}
			bFile = PathFileExists(cmdLinePath.GetWinPath()) ? !cmdLinePath.IsDirectory() : FALSE;

			if (url.IsEmpty())
			{
				CURLDlg urldlg;
				if (urldlg.DoModal() != IDOK)
				{
					if (TSVNMutex)
						::CloseHandle(TSVNMutex);
					return FALSE;
				}
				url = urldlg.m_url;
			}

			CString val = parser.GetVal(_T("rev"));
			SVNRev rev(val);
			CRepositoryBrowser dlg(SVNUrl(url, rev), bFile);
			dlg.m_ProjectProperties.ReadProps(cmdLinePath);
			dlg.m_path = cmdLinePath;
			dlg.DoModal();
		}
		//#endregion 
		//#region ignore
		if (command == cmdIgnore)
		{

			CString filelist;
			BOOL err = FALSE;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				//strLine = _T("F:\\Development\\DirSync\\DirSync.cpp");
				CString name = pathList[nPath].GetFileOrDirectoryName();
				if (parser.HasKey(_T("onlymask")))
				{
					name = _T("*")+pathList[nPath].GetFileExtension();
				}
				filelist += name + _T("\n");
				CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
				SVNProperties props(parentfolder);
				CStringA value;
				for (int i=0; i<props.GetCount(); i++)
				{
					CString propname(props.GetItemName(i).c_str());
					if (propname.CompareNoCase(_T("svn:ignore"))==0)
					{
						stdstring stemp;
						stdstring tmp = props.GetItemValue(i);
						//treat values as normal text even if they're not
						value = (char *)tmp.c_str();
					}
				}
				if (value.IsEmpty())
					value = name;
				else
				{
					value = value.Trim("\n\r");
					value += "\n";
					value += name;
					value.Remove('\r');
				}
				if (!props.Add(_T("svn:ignore"), value))
				{
					CString temp;
					temp.Format(IDS_ERR_FAILEDIGNOREPROPERTY, name);
					CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
					err = TRUE;
					break;
				}
			}
			if (err == FALSE)
			{
				CString temp;
				temp.Format(IDS_PROC_IGNORESUCCESS, filelist);
				CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region unignore
		if (command == cmdUnIgnore)
		{
			CString filelist;
			BOOL err = FALSE;
			for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
			{
				CString name = pathList[nPath].GetFileOrDirectoryName();
				if (parser.HasKey(_T("onlymask")))
				{
					name = _T("*")+pathList[nPath].GetFileExtension();
				}
				filelist += name + _T("\n");
				CTSVNPath parentfolder = pathList[nPath].GetContainingDirectory();
				SVNProperties props(parentfolder);
				CStringA value;
				for (int i=0; i<props.GetCount(); i++)
				{
					CString propname(props.GetItemName(i).c_str());
					if (propname.CompareNoCase(_T("svn:ignore"))==0)
					{
						stdstring tmp = props.GetItemValue(i);
						//treat values as normal text even if they're not
						value = (char *)tmp.c_str();
						break;
					}
				}
				value = value.Trim("\n\r");
				value += "\n";
				value.Remove('\r');
				value.Replace("\n\n", "\n");
				
				// Delete all occurences of 'name'
				// "\n" is temporarily prepended to make the algorithm easier
				value = "\n" + value;
				value.Replace("\n" + CUnicodeUtils::GetUTF8(name) + "\n", "\n");
				value = value.Mid(1);
				
				CStringA sTrimmedvalue = value;
				sTrimmedvalue.Trim();
				if (sTrimmedvalue.IsEmpty())
				{
					if (!props.Remove(_T("svn:ignore")))
					{
						CString temp;
						temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, name);
						CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
						err = TRUE;
						break;
					}
				}
				else
				{
					if (!props.Add(_T("svn:ignore"), value))
					{
						CString temp;
						temp.Format(IDS_ERR_FAILEDUNIGNOREPROPERTY, name);
						CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
						err = TRUE;
						break;
					}
				}
			}
			if (err == FALSE)
			{
				CString temp;
				temp.Format(IDS_PROC_UNIGNORESUCCESS, filelist);
				CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region blame
		if (command == cmdBlame)
		{
			bool bShowDialog = true;
			CBlameDlg dlg;
			dlg.EndRev = SVNRev::REV_HEAD;
			if (parser.HasKey(_T("startrev")) && parser.HasKey(_T("endrev")))
			{
				bShowDialog = false;
				dlg.StartRev = parser.GetLongVal(_T("startrev"));
				dlg.EndRev = parser.GetLongVal(_T("endrev"));
			}
			if ((!bShowDialog)||(dlg.DoModal() == IDOK))
			{
				CBlame blame;
				CString tempfile;
				CString logfile;
				tempfile = blame.BlameToTempFile(cmdLinePath, dlg.StartRev, dlg.EndRev, logfile, TRUE);
				if (!tempfile.IsEmpty())
				{
					if (logfile.IsEmpty())
					{
						//open the default text editor for the result file
						CUtils::StartTextViewer(tempfile);
					}
					else
					{
						CUtils::LaunchTortoiseBlame(tempfile, logfile, cmdLinePath.GetFileOrDirectoryName());
					}
				} // if (!tempfile.IsEmpty()) 
				else
				{
					CMessageBox::Show(EXPLORERHWND, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
			} // if (dlg.DoModal() == IDOK) 
		} // if (comVal.Compare(_T("blame"))==0) 
		//#endregion 
		//#region cat
		if (command == cmdCat)
		{
			CString savepath = CUtils::GetLongPathname(parser.GetVal(_T("savepath")));
			CString revision = parser.GetVal(_T("revision"));
			CString pegrevision = parser.GetVal(_T("pegrevision"));
			LONG rev = _ttol(revision);
			if (rev==0)
				rev = SVNRev::REV_HEAD;
			LONG pegrev = _ttol(pegrevision);
			if (pegrev == 0)
				pegrev = SVNRev::REV_HEAD;
			SVN svn;
			if (!svn.Cat(cmdLinePath, pegrev, rev, CTSVNPath(savepath)))
			{
				::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				::DeleteFile(savepath);
			} 
		}
		//#endregion
		//#region createpatch
		if (command == cmdCreatePatch)
		{
			CString savepath = CUtils::GetLongPathname(parser.GetVal(_T("savepath")));
			CCreatePatch dlg;
			dlg.m_pathList = pathList;
			if (dlg.DoModal()==IDOK)
			{
				if (cmdLinePath.IsEmpty())
				{
					if (pathList.GetCount() == 1)
						cmdLinePath = pathList[0];
					else
					{
						cmdLinePath = pathList.GetCommonRoot();
					}
				}
				CreatePatch(cmdLinePath, dlg.m_pathList, CTSVNPath(savepath));
				SVN svn;
				svn.Revert(dlg.m_filesToRevert, false);
			}
		}
		//#endregion
		//#region updatecheck
		if (command == cmdUpdateCheck)
		{
			CCheckForUpdatesDlg dlg;
			if (parser.HasKey(_T("visible")))
				dlg.m_bShowInfo = TRUE;
			dlg.DoModal();
		}
		//#endregion
		//#region revisiongraph
		if (command == cmdRevisionGraph)
		{
			CRevisionGraphDlg dlg;
			dlg.SetPath(cmdLinePath.GetWinPathString());
			dlg.DoModal();
		} 
		//#endregion
		//#region lock
		if (command == cmdLock)
		{
			CLockDlg lockDlg;
			lockDlg.m_pathList = pathList;
			if (lockDlg.DoModal()==IDOK)
			{
				if (lockDlg.m_pathList.GetCount() != 0)
				{
					CSVNProgressDlg progDlg;
					progDlg.SetParams(CSVNProgressDlg::Lock, lockDlg.m_bStealLocks ? ProgOptLockForce : 0, lockDlg.m_pathList, CString(), lockDlg.m_sLockMessage);
					progDlg.DoModal();
				}
			}
		}
		//#endregion
		//#region unlock
		if (command == cmdUnlock)
		{
			// create a pathlist with all locked files in it
			CTSVNPathList lockedList;
			for (int i=0; i<pathList.GetCount(); ++i)
			{
				if (pathList[i].IsDirectory())
				{
					// a folder was selected. Get all files in it and add the ones
					// with a lock to the locked list.
					SVNStatus stat;
					CTSVNPath retPath;
					svn_wc_status2_t * status;
					status = stat.GetFirstFileStatus(pathList[i], retPath);
					do
					{
						if (status && status->entry && status->entry->lock_token)
						{
							if (status->entry->lock_token[0] != 0)
								lockedList.AddPath(retPath);
						}
						if (status)
							status = stat.GetNextFileStatus(retPath);
					} while(status && !retPath.IsEmpty());
				}
				else
					lockedList.AddPath(pathList[i]);
			}
			if (lockedList.GetCount())
			{
				CSVNProgressDlg progDlg;
				progDlg.SetParams(CSVNProgressDlg::Unlock, parser.HasKey(_T("force")) ? ProgOptLockForce : 0, lockedList);
				progDlg.DoModal();
			}
		} 
		//#endregion
		//#region rebuildiconcache
		if (command == cmdRebuildIconCache)
		{
			bool bQuiet = !!parser.HasKey(_T("noquestion"));
			if (CShellUpdater::RebuildIcons())
			{
				if (!bQuiet)
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_ICONCACHEREBUILT, IDS_APPNAME, MB_ICONINFORMATION);
			}
			else
			{
				if (!bQuiet)
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_ICONCACHENOTREBUILT, IDS_APPNAME, MB_ICONINFORMATION);
			}
		} 
		//#endregion
		//#region properties
		if (command == cmdProperties)
		{
			CEditPropertiesDlg dlg;
			dlg.SetPathList(pathList);
			dlg.DoModal();
		}
		//#endregion


		if (TSVNMutex)
			::CloseHandle(TSVNMutex);
	} 

	// Look for temporary files left around by TortoiseSVN and
	// remove them. But only delete 'old' files because some
	// apps might still be needing the recent ones.
	{
		DWORD len = ::GetTempPath(0, NULL);
		TCHAR * path = new TCHAR[len + 100];
		len = ::GetTempPath (len+100, path);
		if (len != 0)
		{
			CSimpleFileFind finder = CSimpleFileFind(path, _T("*svn*.*"));
			FILETIME systime_;
			::GetSystemTimeAsFileTime(&systime_);
			__int64 systime = (((_int64)systime_.dwHighDateTime)<<32) | ((__int64)systime_.dwLowDateTime);
			while (finder.FindNextFileNoDirectories())
			{
				CString filepath = finder.GetFilePath();
				HANDLE hFile = ::CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					FILETIME createtime_;
					if (::GetFileTime(hFile, &createtime_, NULL, NULL))
					{
						::CloseHandle(hFile);
						__int64 createtime = (((_int64)createtime_.dwHighDateTime)<<32) | ((__int64)createtime_.dwLowDateTime);
						if ((createtime + 864000000000) < systime)		//only delete files older than a day
						{
							::SetFileAttributes(filepath, FILE_ATTRIBUTE_NORMAL);
							::DeleteFile(filepath);
						}
					}
					else
						::CloseHandle(hFile);
				}
			}
		}	
		delete path;		
	}


	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}


UINT_PTR CALLBACK 
CTortoiseProcApp::CreatePatchFileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	if(uiMsg ==	WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
	{
		HWND hFileDialog = GetParent(hDlg);
		
		CString strFilename = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString() + PATCH_TO_CLIPBOARD_PSEUDO_FILENAME;

		CommDlg_OpenSave_SetControlText(hFileDialog, edt1, (LPCTSTR)strFilename);   

		PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
	}
	return 0;
}

BOOL CTortoiseProcApp::CreatePatch(const CTSVNPath& root, const CTSVNPathList& path, const CTSVNPath& cmdLineSavePath)
{
	OPENFILENAME ofn;		// common dialog box structure
	CString temp;
	CTSVNPath savePath;

	if (cmdLineSavePath.IsEmpty())
	{
		TCHAR szFile[MAX_PATH];  // buffer for file name
		ZeroMemory(szFile, sizeof(szFile));
		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
		ofn.hwndOwner = (EXPLORERHWND);
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
		temp.LoadString(IDS_REPOBROWSE_SAVEAS);
		CUtils::RemoveAccelerators(temp);
		if (temp.IsEmpty())
			ofn.lpstrTitle = NULL;
		else
			ofn.lpstrTitle = temp;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLEHOOK;

		ofn.hInstance = AfxGetResourceHandle();
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
		ofn.lpfnHook = CreatePatchFileOpenHook;

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
		if (GetSaveFileName(&ofn)==FALSE)
		{
			delete [] pszFilters;
			return FALSE;
		} // if (GetSaveFileName(&ofn)==FALSE)
		delete [] pszFilters;
		savePath = CTSVNPath(ofn.lpstrFile);
		if (ofn.nFilterIndex == 1)
		{
			if (savePath.GetFileExtension().IsEmpty())
				savePath.AppendRawString(_T(".patch"));
		}
	}
	else
	{
		savePath = cmdLineSavePath;
	}

	// This is horrible and I should be ashamed of myself, but basically, the 
	// the file-open dialog writes ".TSVNPatchToClipboard" to its file field if the user clicks
	// the "Save To Clipboard" button.
	bool bToClipboard = _tcsstr(savePath.GetWinPath(), PATCH_TO_CLIPBOARD_PSEUDO_FILENAME) != NULL;

	CProgressDlg progDlg;
	progDlg.SetTitle(IDS_PROC_PATCHTITLE);
	progDlg.SetShowProgressBar(false);
	progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
	progDlg.FormatNonPathLine(1, IDS_PROC_SAVEPATCHTO);
	if(bToClipboard)
	{
		progDlg.FormatNonPathLine(2, IDS_CLIPBOARD_PROGRESS_DEST);
	}
	else
	{
		progDlg.SetLine(2, savePath.GetUIPathString(), true);
	}
	//progDlg.SetAnimation(IDR_ANIMATION);

	CTSVNPath tempPatchFilePath;
	if (bToClipboard)
		tempPatchFilePath = CTempFiles::Instance().GetTempFilePath(true);
	else
		tempPatchFilePath = savePath;

	::DeleteFile(tempPatchFilePath.GetWinPath());
	
	CTSVNPath sDir;
	if (root.GetWinPathString().Find('*')>=0)
		sDir = path.GetCommonRoot();
	else
		sDir = CTSVNPath(root);
	if (sDir.IsEmpty())
		sDir = root;
	if (!sDir.IsDirectory())
	{
		::SetCurrentDirectory(sDir.GetDirectory().GetWinPath());
	}
	else
	{
		::SetCurrentDirectory(sDir.GetWinPath());
	}

	SVN svn;
	for (int fileindex = 0; fileindex < path.GetCount(); ++fileindex)
	{
		// use the relative path
		CString sRelativePath = path[fileindex].GetWinPathString().Mid(sDir.GetDirectory().GetWinPathString().GetLength());
		sRelativePath.Trim(_T("/\\"));
		CTSVNPath diffpath = CTSVNPath(sRelativePath);
		if (!svn.Diff(diffpath, SVNRev::REV_BASE, diffpath, SVNRev::REV_WC, FALSE, FALSE, FALSE, FALSE, _T(""), true, tempPatchFilePath))
		{
			progDlg.Stop();
			::MessageBox((EXPLORERHWND), svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return FALSE;
		}
	}

	if(bToClipboard)
	{
		// The user actually asked for the patch to be written to the clipboard
		CStringA sClipdata;
		FILE * inFile;
		_tfopen_s(&inFile, tempPatchFilePath.GetWinPath(), _T("rb"));
		if(inFile)
		{
			char chunkBuffer[16384];
			while(!feof(inFile))
			{
				size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);
				sClipdata.Append(chunkBuffer, (int)readLength);
			}
			fclose(inFile);

			CUtils::WriteAsciiStringToClipboard(sClipdata);

		}
	}

	progDlg.Stop();
	return TRUE;
}

void CTortoiseProcApp::CheckUpgrade()
{
	CRegString regVersion = CRegString(_T("Software\\TortoiseSVN\\CurrentVersion"));
	CString sVersion = regVersion;
	if (sVersion.Compare(_T(STRPRODUCTVER))==0)
		return;
	// we're starting the first time with a new version!
	
	LONG lVersion = 0;
	int pos = sVersion.Find(',');
	if (pos > 0)
	{
		lVersion = (_ttol(sVersion.Left(pos))<<24);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
		pos = sVersion.Find(',', pos+1);
		lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
	}
	
	CRegDWORD regval = CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase"), 999);
	if ((DWORD)regval != 999)
	{
		// there's a leftover registry setting we have to convert and then delete it
		CRegDWORD newregval = CRegDWORD(_T("Software\\TortoiseSVN\\ConvertBase"));
		newregval = !regval;
		regval.removeValue();
	}

	if (lVersion <= 0x01010300)
	{
		CSoundUtils::RegisterTSVNSounds();
		// remove all saved dialog positions
		CRegString(_T("Software\\TortoiseSVN\\TortoiseProc\\ResizableState\\")).removeKey();
		CRegDWORD(_T("Software\\TortoiseSVN\\RecursiveOverlay")).removeValue();
		// remove the external cache key
		CRegDWORD(_T("Software\\TortoiseSVN\\ExternalCache")).removeValue();
	}
	
	if (lVersion <= 0x01020200)
	{
		// upgrade to > 1.2.3 means the doc diff scripts changed from vbs to js
		// so remove the diff/merge scripts if they're the defaults
		CRegString diffreg = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\.doc"));
		CString sDiff = diffreg;
		CString sCL = _T("wscript.exe \"") + CUtils::GetAppParentDirectory()+_T("Diff-Scripts\\diff-doc.vbs\"");
		if (sDiff.Left(sCL.GetLength()).CompareNoCase(sCL)==0)
			diffreg = _T("");
		CRegString mergereg = CRegString(_T("Software\\TortoiseSVN\\MergeTools\\.doc"));
		sDiff = mergereg;
		sCL = _T("wscript.exe \"") + CUtils::GetAppParentDirectory()+_T("Diff-Scripts\\merge-doc.vbs\"");
		if (sDiff.Left(sCL.GetLength()).CompareNoCase(sCL)==0)
			mergereg = _T("");
	}
	
	// set the custom diff scripts for every user
	CString scriptsdir = CUtils::GetAppParentDirectory();
	scriptsdir += _T("Diff-Scripts");
	CSimpleFileFind files(scriptsdir);
	while (files.FindNextFileNoDirectories())
	{
		CString file = files.GetFilePath();
		CString filename = files.GetFileName();
		CString ext = file.Mid(file.ReverseFind('-')+1);
		ext = _T(".")+ext.Left(ext.ReverseFind('.'));
		CString kind;
		if (file.Right(3).CompareNoCase(_T("vbs"))==0)
		{
			kind = _T(" //E:vbscript");
		}
		if (file.Right(2).CompareNoCase(_T("js"))==0)
		{
			kind = _T(" //E:javascript");
		}
		
		if (filename.Left(5).CompareNoCase(_T("diff-"))==0)
		{
			CRegString diffreg = CRegString(_T("Software\\TortoiseSVN\\DiffTools\\")+ext);
			if (((CString)diffreg).IsEmpty())
				diffreg = _T("wscript.exe \"") + file + _T("\" %base %mine") + kind;
		}
		if (filename.Left(6).CompareNoCase(_T("merge-"))==0)
		{
			CRegString diffreg = CRegString(_T("Software\\TortoiseSVN\\MergeTools\\")+ext);
			if (((CString)diffreg).IsEmpty())
				diffreg = _T("wscript.exe \"") + file + _T("\" %merged %theirs %mine %base") + kind;
		}
	}

	// set the current version so we don't come here again until the next update!
	regVersion = _T(STRPRODUCTVER);	
}

