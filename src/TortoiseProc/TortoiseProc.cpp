// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// CTortoiseProcApp construction

CTortoiseProcApp::CTortoiseProcApp()
{
}


// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;


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
	CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), GetUserDefaultLangID());
	long langId = loc;
	CString langDll;
	HINSTANCE hInst;
	do
	{
		langDll.Format(_T("TortoiseProc%d.dll"), langId);
		hInst = LoadLibrary(langDll);
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
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();
	AfxOleInit();
	CWinApp::InitInstance();
	SetRegistryKey(_T("TortoiseSVN"));

	CCmdLineParser parser = CCmdLineParser(AfxGetApp()->m_lpCmdLine);

	if (!parser.HasKey(_T("command")))
	{
		CrashProgram();
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMAND, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}

	CString comVal = parser.GetVal(_T("command"));
	if (comVal.IsEmpty())
	{
		//CMessageBox::Show(NULL, "no command value specified!", "Error", MB_ICONERROR);
		CMessageBox::Show(NULL, IDS_ERR_NOCOMMANDVALUE, IDS_APPNAME, MB_ICONERROR);
		return FALSE;
	}
	else
	{
		//set the APR_ICONV_PATH environment variable for UTF8-conversions
		//we don't set the environment variable in the registry or otherwise globally
		//since it would break other subversion clients. 
		CRegStdString tortoiseProcPath(_T("Software\\TortoiseSVN\\ProcPath"), _T(""), false, HKEY_LOCAL_MACHINE);
		CString temp = tortoiseProcPath;
		if (!temp.IsEmpty())
		{
			temp = temp.Left(temp.ReverseFind('\\')+1);
			temp += _T("iconv");
			SetEnvironmentVariable(_T("APR_ICONV_PATH"), temp);
		}
		if (comVal.Compare(_T("test"))==0)
		{
			CMessageBox::Show(NULL, _T("Test command successfully executed"), _T("Info"), MB_OK);
			return FALSE;
		} // if (comVal.Compare(_T("test"))==0)
		{
			SVN svn;
			if (!svn.GetLastErrorMessage().IsEmpty())
			{
				CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK | MB_ICONERROR);
				return FALSE;
			}
		}
		//#region about
		if (comVal.Compare(_T("about"))==0)
		{
			CAboutDlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
			return FALSE;
		}
		//#endregion
		//#region log
		if (comVal.Compare(_T("log"))==0)
		{
			//the log command line looks like this:
			//command:log path:<path_to_file_or_directory_to_show_the_log_messages> [revstart:<startrevision>] [revend:<endrevision>]
			CLogDlg dlg;
			m_pMainWnd = &dlg;
			CString path = parser.GetVal(_T("path"));
			SVNStatus svn;
			long rev = svn.GetStatus(path, true);
			if (rev == (-2))
			{
				rev = svn.GetStatus(path, false);
				if (rev == (-2))
				{
					CMessageBox::Show(NULL, IDS_ERR_NOSTATUS, IDS_APPNAME, MB_ICONERROR);
					return FALSE;
				} // if (rev == (-2))
				if (svn.status->entry)
					rev = svn.status->entry->revision;
				else 
					rev = -1;
			}
			CString val = parser.GetVal(_T("revstart"));
			long revstart = _tstol(val);
			val = parser.GetVal(_T("revend"));
			long revend = _tstol(val);
			if (revstart == 0)
			{
				revstart = -1;
			}
			if (revend == 0)
				revend = 1;
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
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url = %s\n"), dlg.m_url);
				CCheckTempFiles checker;
				if (checker.Check(path, true, false)!=IDCANCEL)
				{
					CSVNProgressDlg progDlg;
					m_pMainWnd = &progDlg;
					//construct the module name out of the path
					CString modname;
					if (dlg.m_bUseFolderAsModule)
						modname = path.Right(path.GetLength() - path.ReverseFind('\\') - 1);
					else
						modname = _T("");
					progDlg.SetParams(Import, false, path, dlg.m_url, dlg.m_message, -1, modname);
					logmessage = dlg.m_message;
					progDlg.DoModal();
				} // if (checker.Check(path, true, false)!=IDCANCEL)
				else
					logmessage.removeValue();
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
				logmessage.removeValue();
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
			if (CMessageBox::Show(NULL, IDS_PROC_WARNREVERT, IDS_APPNAME, MB_YESNO)==IDYES)
			{
				CSVNProgressDlg progDlg;
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
				CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_ICONERROR);
			}
			else
			{
				CMessageBox::Show(NULL, IDS_PROC_CLEANUPFINISHED, IDS_APPNAME, MB_OK);
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
			if (!SVN::CreateRepository(path))
			{
				CMessageBox::Show(NULL, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
			}
			else
			{
				CMessageBox::Show(NULL, IDS_PROC_REPOCREATEFINISHED, IDS_APPNAME, MB_OK);
			}
		}
		//#endregion
		//#region switch
		if (comVal.Compare(_T("switch"))==0)
		{
			CSwitchDlg dlg;
			CString path = parser.GetVal(_T("path"));
			SVNStatus status;
			status.GetStatus(path);
			if (status.status->entry != NULL)
			{
				dlg.m_path = status.status->entry->url;
				dlg.m_URL = dlg.m_path;
			}

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
			CBrowseFolder folderBrowser;
			temp.LoadString(IDS_PROC_EXPORT_1);
			folderBrowser.SetInfo(temp);
			folderBrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
			if (folderBrowser.Show(NULL, saveto)==CBrowseFolder::OK)
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
					progDlg.ShowModeless(NULL);
					progDlg.SetAnimation(IDR_ANIMATION);
				} // if (progDlg.IsValid()){
				SVN svn;
				if (!svn.Export(path, saveplace, -1))
				{
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_OK);
				}
				else
				{
					CString temp;
					temp.Format(IDS_PROC_EXPORT_4, path, saveplace);
					CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK);
				}
				if (progDlg.IsValid())
				{
					progDlg.Stop();
				}
			}
		}
		//#endregion
		//#region merge
		if (comVal.Compare(_T("merge"))==0)
		{
			CMergeDlg dlg;
			CString path = parser.GetVal(_T("path"));
			if (SVNStatus::GetAllStatusRecursive(path)!=svn_wc_status_normal)
			{
				CMessageBox::Show(NULL, IDS_ERR_WCCHANGED, IDS_APPNAME, MB_ICONERROR);
				return FALSE;
			}
			SVNStatus status;
			status.GetStatus(path);
			if ((status.status == NULL) || (status.status->entry == NULL))
			{
				CMessageBox::Show(NULL, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
				return FALSE;		//exit
			} // if ((status.status == NULL) || (status.status->entry == NULL)) // if ((status.status == NULL) || (status.status->entry == NULL))
			CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
			dlg.m_URL = url;
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("url1 = %s, url2 = %s, path = %s\n"), status.status->entry->url, dlg.m_URL, path);
				CSVNProgressDlg progDlg;
				m_pMainWnd = &progDlg;
				//progDlg.SetParams(Merge, false, path, dlg.m_URL, CUnicodeUtils::GetUnicode(status.status->entry->url),dlg.m_lStartRev);		//use the message as the second url
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
			SVNStatus status;
			long rev = status.GetStatus(path);
			if ((rev == (-2))||(status.status->entry == NULL))
			{
				CMessageBox::Show(NULL, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
				TRACE(_T("could not retrieve the URL of the file!\n"));
				return FALSE;		//exit
			}
			CString url = CUnicodeUtils::GetUnicode(status.status->entry->url);
			dlg.m_URL = url;
			if (dlg.DoModal() == IDOK)
			{
				m_pMainWnd = NULL;
				TRACE(_T("copy %s to %s\n"), path, dlg.m_URL);
				if (SVNStatus::GetAllStatusRecursive(path)==svn_wc_status_normal)
				{
					//no changes in the working copy, so just do a repo->repo copy
					CSVNProgressDlg progDlg;
					progDlg.SetParams(Copy, FALSE, url, dlg.m_URL);
					progDlg.DoModal();
				}
				else
				{
					CSVNProgressDlg progDlg;
					progDlg.SetParams(Copy, FALSE, path, dlg.m_URL);
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
			if (CMessageBox::Show(NULL, IDS_PROC_DELETE_CONFIRM, IDS_APPNAME, MB_YESNO)==IDYES)
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
							CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
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
				path =  filepath + dlg.m_name;
				SVN svn;
				if (!svn.Move(path, path, FALSE))
				{
					TRACE(_T("%s\n"), svn.GetLastErrorMessage());
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				} // if (!svn.Move(path, dlg.m_name, FALSE))
			}
		}
		//#endregion
		//#region diff
		if (comVal.Compare(_T("diff"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			CString diffpath = CUtils::GetDiffPath();
			if (diffpath != "")
			{
				CString path2 = parser.GetVal(_T("path2"));
				if (path2.IsEmpty())
				{
					path2 = SVN::GetPristinePath(path);
				}
				CString cmdline;
				cmdline = _T("\"") + diffpath;
				cmdline += _T("\" ");
				cmdline += _T("\"") + path2;
				cmdline += _T("\" ");
				cmdline += _T(" \"") + path;
				cmdline += _T("\"");
				STARTUPINFO startup;
				PROCESS_INFORMATION process;
				memset(&startup, 0, sizeof(startup));
				startup.cb = sizeof(startup);
				memset(&process, 0, sizeof(process));
				if (CreateProcess(NULL /*(LPCTSTR)diffpath*/, const_cast<TCHAR*>((LPCTSTR)cmdline), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
				{
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
					CString temp;
					temp.Format(IDS_ERR_EXTDIFFSTART, lpMsgBuf);
					CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
					LocalFree( lpMsgBuf );
				} // if (CreateProcess(diffpath, cmdline, NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
			}
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
						int ret = CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_YESNOCANCEL | MB_ICONQUESTION);
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
								CMessageBox::Show(NULL, error, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
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
						CMessageBox::Show(NULL, error, _T("TortoiseSVN"), MB_OK | MB_ICONINFORMATION);
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
			CStdioFile file(path, CFile::typeBinary | CFile::modeRead);
			CString strLine = _T("");
			while (file.ReadString(strLine))
			{
				CString name = strLine.Right(strLine.GetLength() - strLine.ReverseFind('\\') - 1);
				if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE))
				{
					TRACE(_T("%s\n"), svn.GetLastErrorMessage());
					CMessageBox::Show(NULL, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
					return FALSE;		//get out of here
				} // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE)) // if (!svn.Move(strLine, droppath+_T("\\")+name, FALSE))
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

			//we have the conflicted file (%merged)
			//now look for the other required files
			CDirFileList list;
			list.BuildList(merge.Left(merge.ReverseFind('\\')), FALSE, FALSE);
			for (int i=0; i<list.GetCount(); i++)
			{
				CString temp = list.GetAt(i);
				if (merge.CompareNoCase(temp.Left(merge.GetLength()))==0)
				{
					//we have one of the "conflict" files
					CString ends = temp.Right(temp.GetLength() - temp.ReverseFind('.') - 1);
					if (ends.CompareNoCase(_T("mine"))==0)
					{
						mine = temp;
					}
					if (ends.Left(1).CompareNoCase(_T("r"))==0)
					{
						long r = _tstol(ends.Right(ends.GetLength() - 1));
						if (r < rev)
						{
							base = temp;
							rev = r;
						}
						else
						{
							base = theirs;
							theirs = temp;
							rev = r;
						}
					}
				} 
			}
			CUtils::StartExtMerge(base, theirs, mine, merge);
		} 
		//#endregion
		if (comVal.Compare(_T("relocate"))==0)
		{
			CString path = parser.GetVal(_T("path"));
			SVNStatus svn;
			svn.GetStatus(path);
			CRelocateDlg dlg;
			if ((svn.status->entry)&&(svn.status->entry->url))
			{
				dlg.m_sFromUrl = svn.status->entry->url;
				dlg.m_sToUrl = svn.status->entry->url;
			}
			if (dlg.DoModal() == IDOK)
			{
				TRACE(_T("relocate from %s to %s\n"), dlg.m_sFromUrl, dlg.m_sToUrl);
				SVN s;
				if (!s.Relocate(path, dlg.m_sFromUrl, dlg.m_sToUrl, TRUE))
				{
					TRACE(_T("%s\n"), s.GetLastErrorMessage());
					CMessageBox::Show(NULL, s.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
				}
				else
				{
					CString temp;
					temp.Format(IDS_PROC_RELOCATEFINISHED, dlg.m_sToUrl);
					CMessageBox::Show(NULL, temp, _T("TortoiseSVN"), MB_ICONINFORMATION);
				}
			}
		}
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
	return FALSE;
}
