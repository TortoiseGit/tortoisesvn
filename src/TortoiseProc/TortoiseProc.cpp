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
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CheckTempFiles.h"
#include "messagebox.h"
#include "UnicodeUtils.h"
#include "CrashReport.h"
#include "DirFileList.h"
#include "SVNProperties.h"
#include "Blame.h"
#include "..\version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PWND (hWndExplorer ? CWnd::FromHandle(hWndExplorer) : NULL)
#define EXPLORERHWND (IsWindow(hWndExplorer) ? hWndExplorer : NULL)

BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


CTortoiseProcApp::CTortoiseProcApp()
{
	EnableHtmlHelp();
}


// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
HWND hWndExplorer;

CCrashReport crasher("steveking@gmx.ch", "Crash Report for TortoiseSVN");// crash

// CTortoiseProcApp initialization

void CTortoiseProcApp::CrashProgram()
{
	int * a;
	a = NULL;
	*a = 7;
}
BOOL CTortoiseProcApp::InitInstance()
{
	//set the resource dll for the required language
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
	long langId = loc;
	CString langDll;
	HINSTANCE hInst = NULL;
	do
	{
		langDll.Format(_T("Languages\\TortoiseProc%d.dll"), langId);
		
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
	langId = loc;
	CString sHelppath;
	sHelppath = this->m_pszHelpFilePath;
	sHelppath = sHelppath.MakeLower();
	sHelppath.Replace(_T("tortoiseproc.chm"), _T("TortoiseSVN_en.chm"));
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
		} // if (PathFileExists(sHelppath))

		DWORD lid = SUBLANGID(langId);
		lid--;
		if (lid > 0)
		{
			langId = MAKELANGID(PRIMARYLANGID(langId), lid);
		}
		else
			langId = 0;
	} while (langId);
	
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();
	AfxOleInit();
	CWinApp::InitInstance();
	SetRegistryKey(_T("TortoiseSVN"));

	CCmdLineParser parser = CCmdLineParser(AfxGetApp()->m_lpCmdLine);
	if (&parser == NULL)
		return FALSE;

	if (CRegDWORD(_T("Software\\TortoiseSVN\\Debug"), FALSE)==TRUE)
		AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

	if (!parser.HasKey(_T("command")))
	{
		CMessageBox::Show(NULL, _T("TortoiseSVN should run automatically when Windows starts.\nYou do NOT run it directly.\nIt is designed to crash if you run it directly for the purpose\nof testing the crash handler.\n<ct=0x0000FF>Do NOT send the crashreport!!!!</ct>"), _T("TortoiseSVN"), MB_ICONINFORMATION);
		CrashProgram();
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
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
		//set the APR_ICONV_PATH environment variable for UTF8-conversions
		//we don't set the environment variable in the registry or otherwise globally
		//since it would break other subversion clients. 
		CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\Directory"), _T(""), false, HKEY_LOCAL_MACHINE);
		CString temp = tortoiseProcPath;
		if (!temp.IsEmpty())
		{
			temp += _T("iconv");
			SetEnvironmentVariable(_T("APR_ICONV_PATH"), temp);
		}
		if (comVal.Compare(_T("test"))==0)
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

		HANDLE TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));	

		//#region about
		if (comVal.Compare(_T("about"))==0)
		{
			CAboutDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
		//#endregion
		//#region log
		if (comVal.Compare(_T("log"))==0)
		{
			//the log command line looks like this:
			//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [revstart:<startrevision>] [revend:<endrevision>]
			CString path = parser.GetVal(_T("path"));
			CString val = parser.GetVal(_T("revstart"));
			long revstart = _tstol(val);
			val = parser.GetVal(_T("revend"));
			long revend = _tstol(val);
			if (revstart == 0)
			{
				revstart = SVN::REV_HEAD;
			}
			if (revend == 0)
			{
				CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\NumberOfLogs"), 100);
				revend = reg;
				revend = -revend;
			}
			CLogDlg dlg;
			m_pMainWnd = &dlg;
			dlg.SetParams(path, revstart, revend);
			dlg.DoModal();			
		}
		//#endregion
		//#region checkout
		if (comVal.Compare(_T("checkout"))==0)
		{
			//
			// Get the directory supplied in the command line. If there isn't
			// one then we should use the current working directory instead.
			//
			CString strPath = parser.GetVal(_T("path"));
			if (strPath.IsEmpty())
			{
				DWORD dwBufSize = ::GetCurrentDirectory(0, NULL);
				LPTSTR tszPath = strPath.GetBufferSetLength(dwBufSize);
				::GetCurrentDirectory(dwBufSize, tszPath);
				strPath.ReleaseBuffer();
			}

			//
			// Create a checkout dialog and display it. If the user clicks OK,
			// we should create an SVN progress dialog, set it as the main
			// window and then display it.
			//
			CCheckoutDlg dlg;
			dlg.m_strCheckoutDirectory = strPath;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), dlg.m_URL);
				TRACE(_T("checkout dir = %s \n"), dlg.m_strCheckoutDirectory);

				strPath = dlg.m_strCheckoutDirectory;

				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Checkout, false, strPath, dlg.m_URL, _T(""), dlg.m_lRevision);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region import
		if (comVal.Compare(_T("import"))==0)
		{
			CImportDlg dlg;
			CRegString logmessage = CRegString(_T("\\Software\\TortoiseSVN\\lastlogmessage"));
			CString logmsg = logmessage;
			if (!logmsg.IsEmpty())
				dlg.m_message = logmsg;
			CString path = parser.GetVal(_T("path"));
			dlg.m_path = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), dlg.m_url);
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				//construct the module name out of the path
				CString modname;
				progDlg.SetParams(Import, false, path, dlg.m_url, dlg.m_message);
				logmessage = dlg.m_message;
				progDlg.DoModal();
			}
			else
				logmessage.removeValue();
		}
		//#endregion
		//#region update
		if (comVal.Compare(_T("update"))==0)
		{
			LONG rev = -1;
			CString path = parser.GetVal(_T("path"));
			TRACE(_T("tempfile = %s\n"), path);
			if (parser.HasKey(_T("rev")))
			{
				CUpdateDlg dlg;
				if (dlg.DoModal() == IDOK)
				{
					rev = dlg.m_revnum;
				}
				else 
					return FALSE;
			}
			CSVNProgressDlg progDlg;
			m_pMainWnd = &progDlg;
			progDlg.SetParams(Update, true, path, _T(""), _T(""), rev);
			progDlg.DoModal();
		}
		//#endregion
		//#region commit
		if (comVal.Compare(_T("commit"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CLogPromptDlg dlg;
			CRegString logmessage = CRegString(_T("\\Software\\TortoiseSVN\\lastlogmessage"));
			CString logmsg = logmessage;
			if (!logmsg.IsEmpty())
				dlg.m_sLogMessage = logmsg;
			dlg.m_sPath = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("tempfile = %s\n"), path);
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Commit, true, path, _T(""), dlg.m_sLogMessage);
				logmessage = dlg.m_sLogMessage;
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK)
			else
			{
				CRegStdWORD nodelete = CRegStdWORD(_T("Software\\TortoiseSVN\\NoDeleteLogMsg"));
				if (!nodelete)
					logmessage.removeValue();
				else
					logmessage = dlg.m_sLogMessage;
			}
		}
		//#endregion
		//#region add
		if (comVal.Compare(_T("add"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			//if the user selected a folder
			//then we scan recursively for unversioned
			//files to show to the user
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			file.ReadString(strLine);
			file.Close();
			CAddDlg dlg;
			dlg.m_sPath = path;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("tempfile = %s\n"), path);
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Add, true, path);
				progDlg.DoModal();
			} // if (dlg.DoModal() == IDOK) // if (dlg.DoModal() == IDOK) 
		}
		//#endregion
		//#region revert
		if (comVal.Compare(_T("revert"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			TRACE(_T("tempfile = %s\n"), path);
			if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO | MB_ICONEXCLAMATION)==IDYES)
			{
				CSVNProgressDlg progDlg(PWND);
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Revert, true, path);
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region cleanup
		if (comVal.Compare(_T("cleanup"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			SVN svn;
			if (!svn.CleanUp(path))
			{
				CString temp;
				temp.Format(IDS_ERR_CLEANUP, svn.GetLastErrorMessage());
				CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				CMessageBox::Show(EXPLORERHWND, IDS_PROC_CLEANUPFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
			}
		}
		//#endregion
		//#region resolve
		if (comVal.Compare(_T("resolve"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CSVNProgressDlg progDlg;
			m_pMainWnd = &progDlg;
			progDlg.SetParams(Resolve, false, path);
			progDlg.DoModal();
		}
		//#endregion
		//#region repocreate
		if (comVal.Compare(_T("repocreate"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			path.Replace('/', '\\');
			if (GetDriveType(path.Left(path.Find('\\')+1))==DRIVE_REMOTE)
			{
				if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATESHAREWARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES)
				{
					if (!SVN::CreateRepository(path))
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
					}
					else
					{
						CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
					}
				} // if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATESHAREWARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO)==IDYES) 
			} // if (GetDriveType(path.Left(path.Find('\\')+1))==DRIVE_REMOTE) 
			else
			{
				if (!SVN::CreateRepository(path))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
				}
				else
				{
					CMessageBox::Show(EXPLORERHWND, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK | MB_ICONINFORMATION);
				}
			}
		} // if (comVal.Compare(_T("repocreate"))==0) 
		//#endregion
		//#region switch
		if (comVal.Compare(_T("switch"))==0)
		{
			CSwitchDlg dlg;
			CString path = parser.GetVal(_T("path"));
			dlg.m_path = path;

			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s, revision = %s\n"), dlg.m_URL, dlg.m_rev);
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Switch, false, path, dlg.m_URL, _T(""), _tstol((LPCTSTR)dlg.m_rev));
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region export
		if (comVal.Compare(_T("export"))==0)
		{
			TCHAR saveto[MAX_PATH];
			CString path = parser.GetVal(_T("path"));
			if (SVNStatus::GetAllStatus(path) == svn_wc_status_unversioned)
			{
				CCheckoutDlg dlg;
				dlg.m_strCheckoutDirectory = path;
				dlg.IsExport = TRUE;
				if (dlg.DoModal() == IDOK)
				{
					path = dlg.m_strCheckoutDirectory;

					CSVNProgressDlg progDlg;
					m_pMainWnd = &progDlg;
					progDlg.SetParams(Export, false, path, dlg.m_URL, _T(""), dlg.m_lRevision);
					progDlg.DoModal();
				}
			}
			else
			{
				CBrowseFolder folderBrowser;
				temp.LoadString(IDS_PROC_EXPORT_1);
				folderBrowser.SetInfo(temp);
				folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
				if (folderBrowser.Show(EXPLORERHWND, saveto)==CBrowseFolder::OK)
				{
					CString saveplace = CString(saveto);
					saveplace += path.Right(path.GetLength() - path.ReverseFind('\\'));
					TRACE(_T("export %s to %s\n"), path, saveto);
					CProgressDlg progDlg;
					if (progDlg.IsValid())
					{
						CString temp;
						temp.Format(IDS_PROC_EXPORT_2, path);
						progDlg.SetLine(1, temp, true);
						progDlg.SetLine(2, saveto, true);
						temp.LoadString(IDS_PROC_EXPORT_3);
						progDlg.SetTitle(temp);
						progDlg.SetShowProgressBar(false);
						progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
						progDlg.SetAnimation(IDR_ANIMATION);
					} // if (progDlg.IsValid()){
					SVN svn;
					if (!svn.Export(path, saveplace, -1))
					{
						if (progDlg.IsValid())
						{
							progDlg.Stop();
						}
						CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
					}
					else
					{
						if (progDlg.IsValid())
						{
							progDlg.Stop();
						}
						CString temp;
						temp.Format(IDS_PROC_EXPORT_4, path, saveplace);
						CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
					}
				}
			}
		}
		//#endregion
		//#region merge
		if (comVal.Compare(_T("merge"))==0)
		{
			CMergeDlg dlg;
			CString path = parser.GetVal(_T("path"));
			dlg.m_URL = path;
			if (dlg.DoModal() == IDOK)
			{
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				progDlg.SetParams(Merge, false, path, dlg.m_URL, dlg.m_URL, dlg.m_lStartRev);		//use the message as the second url
				progDlg.m_nRevisionEnd = dlg.m_lEndRev;
				progDlg.DoModal();
			}
		}
		//#endregion
		//#region copy
		if (comVal.Compare(_T("copy"))==0)
		{
			CCopyDlg dlg;
			CString path = parser.GetVal(_T("path"));
			dlg.m_path = path;
			if (dlg.DoModal() == IDOK)
			{
				m_pMainWnd = NULL;
				TRACE(_T("copy %s to %s\n"), path, dlg.m_URL);
				if (SVNStatus::GetAllStatusRecursive(path)==svn_wc_status_normal)
				{
					//no changes in the working copy, so just do a repo->repo copy
					CSVNProgressDlg progDlg;
					progDlg.SetParams(Copy, FALSE, dlg.m_wcURL, dlg.m_URL, dlg.m_sLogMessage);
					progDlg.DoModal();
				}
				else
				{
					CSVNProgressDlg progDlg;
					progDlg.SetParams(Copy, FALSE, path, dlg.m_URL, dlg.m_sLogMessage, SVN::REV_WC);
					progDlg.DoModal();
				}
			}
		}
		//#endregion
		//#region settings
		if (comVal.Compare(_T("settings"))==0)
		{
			temp.LoadString(IDS_PROC_SETTINGS_TITLE);
			CSettings dlg(temp);
			if (dlg.DoModal()==IDOK)
			{
				dlg.SaveData();
			}
		}
		//#endregion
		//#region remove
		if (comVal.Compare(_T("remove"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			TRACE(_T("tempfile = %s\n"), path);
			if (CMessageBox::Show(EXPLORERHWND, IDS_PROC_DELETE_CONFIRM, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)==IDYES)
			{
				try
				{
					// open the temp file
					CStdioFile file(path, CFile::typeBinary | CFile::modeRead); 
					CString strLine = _T(""); // initialise the variable which holds each line's contents
					while (file.ReadString(strLine))
					{
						TRACE(_T("update file %s\n"), strLine);
						SVN svn;
						if (!svn.Remove(strLine,TRUE))
						{
							TRACE("%s\n", svn.GetLastErrorMessage());
							CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
						}
					}
					file.Close();
					CFile::Remove(path);
				}
				catch (CFileException* )
				{
					TRACE(_T("CFileException in Update!\n"));
				}
			}
		}
		//#endregion
		//#region rename
		if (comVal.Compare(_T("rename"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CRenameDlg dlg;
			CString filename = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
			CString filepath = path.Left(path.ReverseFind('\\') + 1);
			dlg.m_name = filename;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("rename file %s to %s\n"), path, dlg.m_name);
				filepath =  filepath + dlg.m_name;
				SVN svn;
				if (!svn.Move(path, filepath, TRUE))
				{
					TRACE(_T("%s\n"), svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				} // if (!svn.Move(path, dlg.m_name, FALSE))
			}
		}
		//#endregion
		//#region diff
		if (comVal.Compare(_T("diff"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString path2 = parser.GetVal(_T("path2"));
			BOOL bDelete = FALSE;
			if (path2.IsEmpty())
			{
				CString name = CUtils::GetFileNameFromPath(path);
				CString ext = CUtils::GetFileExtFromPath(path);
				path2 = SVN::GetPristinePath(path);
				if ((!CRegDWORD(_T("Software\\TortoiseSVN\\DontConvertBase")))&&(SVN::GetTranslatedFile(path, path)))
				{
					bDelete = TRUE;
				}
				CString n1, n2;
				n1.Format(IDS_DIFF_WCNAME, name);
				n2.Format(IDS_DIFF_BASENAME, name);
				CUtils::StartDiffViewer(path2, path, TRUE, n2, n1, ext);
			} // if (path2.IsEmpty())
			else
				CUtils::StartDiffViewer(path2, path, TRUE);
			if (bDelete)
				DeleteFile(path);
		}
		//#endregion
		//#region dropcopyadd
		if (comVal.Compare(_T("dropcopyadd"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString droppath = parser.GetVal(_T("droptarget"));

			CStringArray sarray;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
			{
				if (strLine.CompareNoCase(droppath) != 0)
				{
					//copy the file to the new location
					CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
					if (PathFileExists(droppath+_T("\\")+name))
					{
						CString temp;
						temp.Format(IDS_PROC_OVERWRITE_CONFIRM, droppath+_T("\\")+name);
						int ret = CMessageBox::Show(EXPLORERHWND, temp, _T("TortoiseSVN"), MB_YESNOCANCEL | MB_ICONQUESTION);
						if (ret == IDYES)
						{
							if (!CopyFile(strLine, droppath+_T("\\")+name, TRUE))
							{
								//the copy operation failed! Get out of here!
								file.Close();
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
								CString error;
								temp.Format(IDS_ERR_COPYFILES, lpMsgBuf);
								CMessageBox::Show(EXPLORERHWND, error, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
								LocalFree( lpMsgBuf );
								return FALSE;
							}
						}
						if (ret == IDCANCEL)
						{
							file.Close();
							return FALSE;		//cancel the whole operation
						}
					}
					else if (!CopyFile(strLine, droppath+_T("\\")+name, FALSE))
					{
						//the copy operation failed! Get out of here!
						file.Close();
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
						CString error;
						temp.Format(IDS_ERR_COPYFILES, lpMsgBuf);
						CMessageBox::Show(EXPLORERHWND, error, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
						LocalFree( lpMsgBuf );
						return FALSE;
					}
					sarray.Add(droppath+_T("\\")+name);		//add the new filepath
				}
			}
			file.Close();
			CStdioFile newfile(path, CFile::typeBinary | CFile::modeCreate | CFile::modeWrite);
			for (int i=0; i<sarray.GetSize(); i++)
			{
				newfile.WriteString(sarray.GetAt(i)+_T("\n"));
			}
			newfile.Close();
			//now add all the newly copied files to the working copy
			TRACE(_T("tempfile = %s\n"), path);
			CSVNProgressDlg progDlg;
			m_pMainWnd = &progDlg;
			progDlg.SetParams(Add, true, path);
			progDlg.DoModal();
		}
		//#endregion
		//#region dropmove
		if (comVal.Compare(_T("dropmove"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long totalcount = 0;
			unsigned long count = 0;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
				totalcount++;
			file.SeekToBegin();
			CProgressDlg progress;
			if (progress.IsValid())
			{
				CString temp;
				temp.LoadString(IDS_PROC_MOVING);
				progress.SetTitle(temp);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			while (file.ReadString(strLine))
			{
				CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
				CString temp;
				temp.Format(IDS_PROC_MOVINGPROG, strLine);
				CString temp2;
				temp2.Format(IDS_PROC_CPYMVPROG2, droppath+_T("\\")+name);
				if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE))
				{
					TRACE(_T("%s\n"), svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.SetLine(1, temp, true);
					progress.SetLine(2, temp2, true);
					progress.SetProgress(count, totalcount);
				} // if (progress.IsValid())
				if ((progress.IsValid())&&(progress.HasUserCancelled()))
				{
					CMessageBox::Show(EXPLORERHWND, IDS_SVN_USERCANCELLED, IDS_APPNAME, MB_ICONINFORMATION);
					return FALSE;
				}
			}
		}
		//#endregion
		//#region dropcopy
		if (comVal.Compare(_T("dropcopy"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString droppath = parser.GetVal(_T("droptarget"));
			SVN svn;
			unsigned long totalcount = 0;
			unsigned long count = 0;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
				totalcount++;
			file.SeekToBegin();
			CProgressDlg progress;
			if (progress.IsValid())
			{
				CString temp;
				temp.LoadString(IDS_PROC_COPYING);
				progress.SetTitle(temp);
				progress.SetAnimation(IDR_MOVEANI);
				progress.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			}
			while (file.ReadString(strLine))
			{
				CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
				CString temp;
				temp.Format(IDS_PROC_COPYINGPROG, strLine);
				CString temp2;
				temp2.Format(IDS_PROC_CPYMVPROG2, droppath+_T("\\")+name);
				if (strLine.CompareNoCase(droppath+_T("\\")+name)==0)
				{
					progress.Stop();
					CRenameDlg dlg;
					dlg.m_windowtitle.Format(IDS_PROC_RENAME, name);
					if (dlg.DoModal() != IDOK)
					{
						return FALSE;
					}
					name = dlg.m_name;
				} // if (strLine.CompareNoCase(droppath+_T("\\")+name)==0) 
				if (!svn.Copy(strLine, droppath+_T("\\")+name, SVN::REV_WC, _T("")))
				{
					TRACE(_T("%s\n"), svn.GetLastErrorMessage());
					CMessageBox::Show(EXPLORERHWND, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) 
				count++;
				if (progress.IsValid())
				{
					progress.SetLine(1, temp, true);
					progress.SetLine(2, temp2, true);
					progress.SetProgress(count, totalcount);
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
		if (comVal.Compare(_T("conflicteditor"))==0)
		{
			long rev = -1;
			CString theirs;
			CString mine;
			CString base;
			CString merge = parser.GetVal(_T("path"));
			CString path = merge.Left(merge.ReverseFind('\\'));
			path = path + _T("\\");

			//we have the conflicted file (%merged)
			//now look for the other required files
			SVNStatus stat;
			stat.GetStatus(merge);
			if (stat.status->entry)
			{
				if (stat.status->entry->conflict_new)
				{
					theirs = stat.status->entry->conflict_new;
					theirs = path + theirs;
				}
				if (stat.status->entry->conflict_old)
				{
					base = stat.status->entry->conflict_old;
					base = path + base;
				}
				if (stat.status->entry->conflict_wrk)
				{
					mine = stat.status->entry->conflict_wrk;
					mine = path + mine;
				}
			}
			CUtils::StartExtMerge(base, theirs, mine, merge);
		} 
		//#endregion
		//#region relocate
		if (comVal.Compare(_T("relocate"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			SVNStatus svn;
			svn.GetStatus(path);
			CRelocateDlg dlg;
			if ((svn.status)&&(svn.status->entry)&&(svn.status->entry->url))
			{
				dlg.m_sFromUrl = svn.status->entry->url;
				dlg.m_sToUrl = svn.status->entry->url;
			}
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("relocate from %s to %s\n"), dlg.m_sFromUrl, dlg.m_sToUrl);
				SVN s;

				CProgressDlg progress;
				if (progress.IsValid())
				{
					CString temp;
					temp.LoadString(IDS_PROC_RELOCATING);
					progress.SetTitle(temp);
					progress.ShowModeless(PWND);
				}
				if (!s.Relocate(path, dlg.m_sFromUrl, dlg.m_sToUrl, TRUE))
				{
					if (progress.IsValid())
						progress.Stop();
					TRACE(_T("%s\n"), s.GetLastErrorMessage());
					CMessageBox::Show((EXPLORERHWND), s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
				else
				{
					if (progress.IsValid())
						progress.Stop();
					CString temp;
					temp.Format(IDS_PROC_RELOCATEFINISHED, dlg.m_sToUrl);
					CMessageBox::Show((EXPLORERHWND), temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
				}
			}
		} // if (comVal.Compare(_T("relocate"))==0)
		//#endregion
		//#region help
		if (comVal.Compare(_T("help"))==0)
		{
			ShellExecute(EXPLORERHWND, _T("open"), m_pszHelpFilePath, NULL, NULL, SW_SHOWNORMAL);
		}
		//#endregion
		//#region repostatus
		if (comVal.Compare(_T("repostatus"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CChangedDlg dlg;
			dlg.m_path = path;
			dlg.DoModal();
		} // if (comVal.Compare(_T("repostatus"))==0)
		//#endregion 
		//#region repobrowser
		if (comVal.Compare(_T("repobrowser"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString url;
			SVNStatus status;
			long reporev;
			if ((reporev = status.GetStatus(path)) != -2)
			{
				if (status.status->entry)
					url = status.status->entry->url;
			} // if (status.GetStatus(path)!=-2)
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
			} // if (dlg.m_strUrl.IsEmpty())
			CRepositoryBrowser dlg(url);
			dlg.m_bStandAlone = TRUE;
			CString val = parser.GetVal(_T("rev"));
			long rev = _tstol(val);
			if (rev != 0)
				dlg.m_nRevision = rev;
			else if (reporev > 0)
				dlg.m_nRevision = reporev;
			dlg.DoModal();
		}
		//#endregion 
		//#region ignore
		if (comVal.Compare(_T("ignore"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			SVN svn;
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
			{
				//strLine = _T("F:\\Development\\DirSync\\DirSync.cpp");
				CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
				name = name.Trim(_T("\n\r"));
				CString parentfolder = strLine.Left(strLine.ReverseFind('\\'));
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
				}
			}
		}
		//#endregion
		//#region blame
		if (comVal.Compare(_T("blame"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CBlameDlg dlg;
			if (dlg.DoModal() == IDOK)
			{
				CBlame blame;
				CString tempfile;
				tempfile = blame.BlameToTempFile(path, dlg.m_lStartRev, dlg.m_lEndRev, TRUE);
				if (!tempfile.IsEmpty())
				{
					//open the default text editor for the result file
					CUtils::StartTextViewer(tempfile);
				} // if (blame.BlameToTempFile(path, dlg.m_lStartRev, dlg.m_lEndRev, FALSE, TRUE) 
				else
				{
					CMessageBox::Show(EXPLORERHWND, blame.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
			} // if (dlg.DoModal() == IDOK) 
		} // if (comVal.Compare(_T("blame"))==0) 
		//#endregion 
		//#region cat
		if (comVal.Compare(_T("cat"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString savepath = parser.GetVal(_T("savepath"));
			CString revision = parser.GetVal(_T("revision"));
			LONG rev = _ttol(revision);
			if (rev==0)
				rev = SVN::REV_HEAD;
			SVN svn;
			if (!svn.Cat(path, rev, savepath))
			{
				::MessageBox(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				DeleteFile(savepath);
			} // if (!svn.Cat(path, rev, savepath)) 
		} // if (comVal.Compare(_T("cat"))==0) 
		//#endregion
		//#region createpatch
		if (comVal.Compare(_T("createpatch"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			OPENFILENAME ofn;		// common dialog box structure
			TCHAR szFile[MAX_PATH];  // buffer for file name
			ZeroMemory(szFile, sizeof(szFile));
			// Initialize OPENFILENAME
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			//ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;		//to stay compatible with NT4
			ofn.hwndOwner = (EXPLORERHWND);
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
			CString temp;
			temp.LoadString(IDS_REPOBROWSE_SAVEAS);
			if (temp.IsEmpty())
				ofn.lpstrTitle = NULL;
			else
				ofn.lpstrTitle = temp;
			ofn.Flags = OFN_OVERWRITEPROMPT;

			CString sFilter;
			sFilter.LoadString(IDS_PATCHFILEFILTER);
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
			if (GetSaveFileName(&ofn)==FALSE)
			{
				delete [] pszFilters;
				return FALSE;
			} // if (GetSaveFileName(&ofn)==FALSE)
			delete [] pszFilters;

			CProgressDlg progDlg;
			temp.LoadString(IDS_PROC_SAVEPATCHTO);
			progDlg.SetLine(1, temp, true);
			progDlg.SetLine(2, ofn.lpstrFile, true);
			temp.LoadString(IDS_PROC_PATCHTITLE);
			progDlg.SetTitle(temp);
			progDlg.SetShowProgressBar(false);
			progDlg.ShowModeless(CWnd::FromHandle(EXPLORERHWND));
			//progDlg.SetAnimation(IDR_ANIMATION);

			path = path.TrimRight('\\');
			SetCurrentDirectory(path.Left(path.ReverseFind('\\')));
			CString sDir = path.Mid(path.ReverseFind('\\')+1);
			SVN svn;
			if (!svn.Diff(sDir, SVN::REV_BASE, sDir, SVN::REV_WC, TRUE, FALSE, FALSE, _T(""), ofn.lpstrFile))
			{
				progDlg.Stop();
				::MessageBox((EXPLORERHWND), svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				DeleteFile(ofn.lpstrFile);
			} // if (!svn.Diff(path, SVN::REV_WC, path, SVN::REV_BASE, TRUE, FALSE, FALSE, _T(""), sSavePath))
			progDlg.Stop();
		} // if (comVal.Compare(_T("cat"))==0) 
		//#endregion

		if (TSVNMutex)
			::CloseHandle(TSVNMutex);
	} 

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}
