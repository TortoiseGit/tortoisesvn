// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2013 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
//#include "vld.h"
#include "TortoiseProc.h"
#include "SysImageList.h"
#include "CrashReport.h"
#include "CmdLineParser.h"
#include "Hooks.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"
#include "libintl.h"
#include "DirFileEnum.h"
#include "SoundUtils.h"
#include "SVN.h"
#include "SVNAdminDir.h"
#include "SVNGlobal.h"
#include "svn_types.h"
#include "svn_dso.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "Commands/Command.h"
#include "../version.h"
#include "JumpListHelpers.h"
#include "CmdUrlParser.h"
#include "Libraries.h"
#include "TempFile.h"
#include "SmartHandle.h"
#include "TaskbarUUID.h"
#include "CreateProcessHelper.h"
#include "SVNConfig.h"

#define STRUCT_IOVEC_DEFINED
#include "sasl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


BEGIN_MESSAGE_MAP(CTortoiseProcApp, CWinAppEx)
    ON_COMMAND(ID_HELP, CWinAppEx::OnHelp)
END_MESSAGE_MAP()



//////////////////////////////////////////////////////////////////////////
CCrashReportTSVN crasher(L"TortoiseSVN " _T(APP_X64_STRING));

CTortoiseProcApp::CTortoiseProcApp() : hWndExplorer(NULL)
{
    SetDllDirectory(L"");
    EnableHtmlHelp();
    int argc = 0;
    const char* const * argv = NULL;
    apr_app_initialize(&argc, &argv, NULL);
    svn_dso_initialize2();
    SYS_IMAGE_LIST();
    CHooks::Create();
    g_SVNAdminDir.Init();
    m_bLoadUserToolbars = FALSE;
    m_bSaveState = FALSE;
    retSuccess = false;
}

CTortoiseProcApp::~CTortoiseProcApp()
{
    // global application exit cleanup (after all SSL activity is shutdown)
    // we have to clean up SSL ourselves, since serf doesn't do that (can't do it)
    // because those cleanup functions work globally per process.
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();

    // since it is undefined *when* the global object SVNAdminDir is
    // destroyed, we tell it to destroy the memory pools and terminate apr
    // *now* instead of later when the object itself is destroyed.
    g_SVNAdminDir.Close();
    CHooks::Destroy();
    SYS_IMAGE_LIST().Cleanup();
    apr_terminate();
    sasl_done();
}

// The one and only CTortoiseProcApp object
CTortoiseProcApp theApp;
CString sOrigCWD;
CString g_sGroupingUUID;
HWND GetExplorerHWND()
{
    return theApp.GetExplorerHWND();
}

HWND FindParentWindow(HWND hWnd)
{
    if (hWnd)
        return hWnd;
    if (theApp.m_pMainWnd->GetSafeHwnd())
        return theApp.m_pMainWnd->GetSafeHwnd();

    return GetExplorerHWND();
}

// CTortoiseProcApp initialization
BOOL CTortoiseProcApp::InitInstance()
{
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
    svn_error_set_malfunction_handler(svn_error_handle_malfunction);
    CheckUpgrade();
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
    CMFCButton::EnableWindowsTheming();
    //set the resource dll for the required language
    CRegDWORD loc = CRegDWORD(_T("Software\\TortoiseSVN\\LanguageID"), 1033);
    long langId = loc;
    CString langDll;
    CStringA langpath = CStringA(CPathUtils::GetAppParentDirectory());
    langpath += "Languages";
    bindtextdomain("subversion", (LPCSTR)langpath);
    bind_textdomain_codeset("subversion", "UTF-8");
    HINSTANCE hInst = NULL;
    do
    {
        langDll.Format(_T("%sLanguages\\TortoiseProc%ld.dll"), (LPCTSTR)CPathUtils::GetAppParentDirectory(), langId);

        hInst = LoadLibrary(langDll);

        CString sVer = _T(STRPRODUCTVER);
        CString sFileVer = CPathUtils::GetVersionFromFile(langDll);
        if (sFileVer.Compare(sVer)!=0)
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
    // MFC uses a help file with the same name as the application by default,
    // which means we have to change that default to our language specific help files
    sHelppath.Replace(_T("tortoiseproc.chm"), _T("TortoiseSVN_en.chm"));
    free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath=_tcsdup(sHelppath);
    sHelppath = CPathUtils::GetAppParentDirectory() + _T("Languages\\TortoiseSVN_en.chm");
    do
    {
        CString sLang = _T("_");
        if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf)))
        {
            sLang += buf;
            sHelppath.Replace(_T("_en"), sLang);
            if (PathFileExists(sHelppath))
            {
                free((void*)m_pszHelpFilePath);
                m_pszHelpFilePath=_tcsdup(sHelppath);
                break;
            }
        }
        sHelppath.Replace(sLang, _T("_en"));
        if (GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf)))
        {
            sLang += _T("_");
            sLang += buf;
            sHelppath.Replace(_T("_en"), sLang);
            if (PathFileExists(sHelppath))
            {
                free((void*)m_pszHelpFilePath);
                m_pszHelpFilePath=_tcsdup(sHelppath);
                break;
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
    CWinAppEx::InitInstance();
    SetRegistryKey(_T("TortoiseSVN"));

    CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);

    // if HKCU\Software\TortoiseSVN\Debug is not 0, show our command line
    // in a message box
    if (CRegDWORD(_T("Software\\TortoiseSVN\\Debug"), FALSE)==TRUE)
        AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

    hWndExplorer = NULL;
    CString sVal = parser.GetVal(_T("hwnd"));
    if (!sVal.IsEmpty())
    {
        hWndExplorer = (HWND)_wcstoui64(sVal, NULL, 16);
    }

    while (GetParent(hWndExplorer)!=NULL)
        hWndExplorer = GetParent(hWndExplorer);
    if (!IsWindow(hWndExplorer))
    {
        hWndExplorer = NULL;
    }

    if ( parser.HasVal(_T("urlcmd")) )
    {
        CmdUrlParser p(parser.GetVal(_T("urlcmd")));
        CString newCmdLine = p.GetCommandLine();
        if (newCmdLine.IsEmpty())
        {
            TSVNMessageBox(GetExplorerHWND(), IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
            return FALSE;
        }
        CCmdLineParser p2(newCmdLine);
        parser = p2;
    }
    if ( parser.HasKey(_T("path")) && parser.HasKey(_T("pathfile")))
    {
        TSVNMessageBox(GetExplorerHWND(), IDS_ERR_INVALIDPATH, IDS_APPNAME, MB_ICONERROR);
        return FALSE;
    }

    if (parser.HasVal(_T("configdir")))
    {
        // the user can override the location of the Subversion config directory here
        CString sConfigDir = parser.GetVal(_T("configdir"));
        g_SVNGlobal.SetConfigDir(sConfigDir);
    }
    // load the configuration now
    SVNConfig::Instance();
    {
        if (SVNConfig::Instance().GetError())
        {
            CString msg;
            CString temp;
            svn_error_t * ErrPtr = SVNConfig::Instance().GetError();
            msg = CUnicodeUtils::GetUnicode(ErrPtr->message);
            while (ErrPtr->child)
            {
                ErrPtr = ErrPtr->child;
                msg += _T("\n");
                msg += CUnicodeUtils::GetUnicode(ErrPtr->message);
            }
            if (!temp.IsEmpty())
            {
                msg += _T("\n") + temp;
            }

            ::MessageBox(hWndExplorer, msg, _T("TortoiseSVN"), MB_ICONERROR);
            // Normally, we give-up and exit at this point, but there is a trap here
            // in that the user might need to use the settings dialog to edit the config file.
            if (CString(parser.GetVal(_T("command"))).Compare(_T("settings"))==0)
            {
                // just open the config file
                TCHAR buf2[MAX_PATH];
                SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf2);
                CString path = buf2;
                path += _T("\\Subversion\\config");
                CAppUtils::StartTextViewer(path);
                return FALSE;
            }
        }
    }

    CTSVNPath cmdLinePath;
    CTSVNPathList pathList;
    if (g_sGroupingUUID.IsEmpty())
        g_sGroupingUUID = parser.GetVal(L"groupuuid");
    if ( parser.HasKey(_T("pathfile")) )
    {
        CString sPathfileArgument = CPathUtils::GetLongPathname(parser.GetVal(_T("pathfile")));
        if (sPathfileArgument.IsEmpty())
        {
            TSVNMessageBox(GetExplorerHWND(), IDS_ERR_NOPATH, IDS_APPNAME, MB_ICONERROR);
            return FALSE;
        }
        cmdLinePath.SetFromUnknown(sPathfileArgument);
        if (pathList.LoadFromFile(cmdLinePath)==false)
            return FALSE;       // no path specified!
        if ( parser.HasKey(_T("deletepathfile")) )
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
        CString sPathArgument = CPathUtils::GetLongPathname(parser.GetVal(_T("path")));
        if (parser.HasKey(_T("expaths")))
        {
            // an /expaths param means we're started via the buttons in our Win7 library
            // and that means the value of /expaths is the current directory, and
            // the selected paths are then added as additional parameters but without a key, only a value

            // because of the "strange treatment of quotation marks and backslashes by CommandLineToArgvW"
            // we have to escape the backslashes first. Since we're only dealing with paths here, that's
            // a save bet.
            // Without this, a command line like:
            // /command:commit /expaths:"D:\" "D:\Utils"
            // would fail because the "D:\" is treated as the backslash being the escape char for the quotation
            // mark and we'd end up with:
            // argv[1] = /command:commit
            // argv[2] = /expaths:D:" D:\Utils
            // See here for more details: http://blogs.msdn.com/b/oldnewthing/archive/2010/09/17/10063629.aspx
            CString cmdLine = GetCommandLineW();
            cmdLine.Replace(L"\\", L"\\\\");
            int nArgs = 0;
            LPWSTR *szArglist = CommandLineToArgvW(cmdLine, &nArgs);
            if (szArglist)
            {
                // argument 0 is the process path, so start with 1
                for (int i=1; i<nArgs; i++)
                {
                    if (szArglist[i][0] != '/')
                    {
                        if (!sPathArgument.IsEmpty())
                            sPathArgument += '*';
                        sPathArgument += szArglist[i];
                    }
                }
                sPathArgument.Replace(L"\\\\", L"\\");
            }
            LocalFree(szArglist);
        }
        if (sPathArgument.IsEmpty() && parser.HasKey(L"path"))
        {
            TSVNMessageBox(GetExplorerHWND(), IDS_ERR_NOPATH, IDS_APPNAME, MB_ICONERROR);
            return FALSE;
        }
        int asterisk = sPathArgument.Find('*');
        cmdLinePath.SetFromUnknown(asterisk >= 0 ? sPathArgument.Left(asterisk) : sPathArgument);
        pathList.LoadFromAsteriskSeparatedString(sPathArgument);
        if (g_sGroupingUUID.IsEmpty() && !cmdLinePath.IsEmpty())
        {
            // when started from the win7 library buttons, we don't get the /groupuuid:xxx parameter
            // passed to us. In that case we have to fetch the uuid (or try to) here,
            // otherwise the grouping wouldn't work.
            CRegStdDWORD groupSetting = CRegStdDWORD(_T("Software\\TortoiseSVN\\GroupTaskbarIconsPerRepo"), 3);
            switch (DWORD(groupSetting))
            {
            case 1:
            case 2:
                {
                    SVN svn;
                    g_sGroupingUUID = svn.GetUUIDFromPath(cmdLinePath);
                }
                break;
            case 3:
            case 4:
                {
                    SVN svn;
                    CString wcroot = svn.GetWCRootFromPath(cmdLinePath).GetWinPathString();
                    g_sGroupingUUID = svn.GetChecksumString(svn_checksum_md5, wcroot);
                }
            }
        }
    }

    CString sAppID = GetTaskIDPerUUID(g_sGroupingUUID).c_str();
    InitializeJumpList(sAppID);
    EnsureSVNLibrary(false);

    // Subversion sometimes writes temp files to the current directory!
    // Since TSVN doesn't need a specific CWD anyway, we just set it
    // to the users temp folder: that way, Subversion is guaranteed to
    // have write access to the CWD
    {
        DWORD len = GetCurrentDirectory(0, NULL);
        if (len)
        {
            std::unique_ptr<TCHAR[]> originalCurrentDirectory(new TCHAR[len]);
            if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
            {
                sOrigCWD = originalCurrentDirectory.get();
                sOrigCWD = CPathUtils::GetLongPathname(sOrigCWD);
            }
        }
        TCHAR pathbuf[MAX_PATH];
        GetTempPath(_countof(pathbuf), pathbuf);
        SetCurrentDirectory(pathbuf);
    }

    CheckForNewerVersion();

    // to avoid that SASL will look for and load its plugin dlls all around the
    // system, we set the path here.
    // Note that SASL doesn't have to be initialized yet for this to work
    sasl_set_path(SASL_PATH_TYPE_PLUGIN, (LPSTR)(LPCSTR)CUnicodeUtils::GetUTF8(CPathUtils::GetAppDirectory().TrimRight('\\')));

    CAutoGeneralHandle TSVNMutex = ::CreateMutex(NULL, FALSE, _T("TortoiseProc.exe"));

    // execute the requested command
    CommandServer server;
    Command * cmd = server.GetCommand(parser.GetVal(_T("command")));
    if (cmd)
    {
        cmd->SetExplorerHwnd(hWndExplorer);
        cmd->SetParser(parser);
        cmd->SetPaths(pathList, cmdLinePath);
        if (cmd->CheckPaths())
            retSuccess = cmd->Execute();
        delete cmd;
    }


    // Look for temporary files left around by TortoiseSVN and
    // remove them. But only delete 'old' files because some
    // apps might still be needing the recent ones.
    CTempFiles::DeleteOldTempFiles(_T("*svn*.*"));

    // Since the dialog has been closed, return FALSE so that we exit the
    // application, rather than start the application's message pump.
    return FALSE;
}

void CTortoiseProcApp::CheckUpgrade()
{
    CRegString regVersion = CRegString(_T("Software\\TortoiseSVN\\CurrentVersion"));
    CString sVersion = regVersion;
    if (sVersion.Compare(_T(STRPRODUCTVER))==0)
        return;
    // we're starting the first time with a new version!

    LONG lVersion = 0;
    int pos = sVersion.Find('.');
    if (pos > 0)
    {
        lVersion = (_ttol(sVersion.Left(pos))<<24);
        lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
        pos = sVersion.Find('.', pos+1);
        lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
    }
    else
    {
        pos = sVersion.Find(',');
        if (pos > 0)
        {
            lVersion = (_ttol(sVersion.Left(pos))<<24);
            lVersion |= (_ttol(sVersion.Mid(pos+1))<<16);
            pos = sVersion.Find(',', pos+1);
            lVersion |= (_ttol(sVersion.Mid(pos+1))<<8);
        }
    }

    if (lVersion <= 0x01070000)
    {
        // create a default "Subversion" library with our own template which includes
        // buttons for TortoiseSVN
        CoInitialize(NULL);
        EnsureSVNLibrary();
        CoUninitialize();
    }
    if (lVersion <= 0x01080000)
    {
        // upgrade to 1.7.1: force recreation of all diff scripts.
        CAppUtils::SetupDiffScripts(true, CString());
    }
    CAppUtils::SetupDiffScripts(false, CString());

    // set the current version so we don't come here again until the next update!
    regVersion = _T(STRPRODUCTVER);
}

void CTortoiseProcApp::InitializeJumpList(const CString& appid)
{
    // for Win7 : use a custom jump list
    CoInitialize(NULL);
    SetAppID(appid);
    DeleteJumpList(appid);
    DoInitializeJumpList(appid);
    CoUninitialize();
}

void CTortoiseProcApp::DoInitializeJumpList(const CString& appid)
{
    ATL::CComPtr<ICustomDestinationList> pcdl;
    HRESULT hr = pcdl.CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hr))
        return;

    hr = pcdl->SetAppID(appid);
    if (FAILED(hr))
        return;

    UINT uMaxSlots;
    ATL::CComPtr<IObjectArray> poaRemoved;
    hr = pcdl->BeginList(&uMaxSlots, IID_PPV_ARGS(&poaRemoved));
    if (FAILED(hr))
        return;

    ATL::CComPtr<IObjectCollection> poc;
    hr = poc.CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER);
    if (FAILED(hr))
        return;

    CString sTemp = CString(MAKEINTRESOURCE(IDS_MENUSETTINGS));
    CStringUtils::RemoveAccelerators(sTemp);

    ATL::CComPtr<IShellLink> psl;
    hr = CreateShellLink(_T("/command:settings"), (LPCTSTR)sTemp, 19, &psl);
    if (SUCCEEDED(hr))
    {
        poc->AddObject(psl);
    }
    sTemp = CString(MAKEINTRESOURCE(IDS_MENUHELP));
    CStringUtils::RemoveAccelerators(sTemp);
    psl.Release(); // Need to release the object before calling operator&()
    hr = CreateShellLink(_T("/command:help"), (LPCTSTR)sTemp, 18, &psl);
    if (SUCCEEDED(hr))
    {
        poc->AddObject(psl);
    }

    ATL::CComPtr<IObjectArray> poa;
    hr = poc.QueryInterface(&poa);
    if (SUCCEEDED(hr))
    {
        pcdl->AppendCategory((LPCTSTR)CString(MAKEINTRESOURCE(IDS_PROC_TASKS)), poa);
        pcdl->CommitList();
    }
}

int CTortoiseProcApp::ExitInstance()
{
    CWinAppEx::ExitInstance();
    if (retSuccess)
        return 0;
    return -1;
}

void CTortoiseProcApp::CheckForNewerVersion()
{
    if (CRegDWORD(_T("Software\\TortoiseSVN\\VersionCheck"), TRUE) == FALSE)
        return;

    time_t now;
    time(&now);
    if (now == 0 )
        return;

    struct tm ptm;
    if (localtime_s(&ptm, &now)!=0)
        return;

    DWORD count = MAX_PATH;
    TCHAR username[MAX_PATH] = {0};
    GetUserName(username, &count);
    // add a user specific diff to the current day count
    // so that the update check triggers for different users
    // at different days. This way we can 'spread' the update hits
    // to our website a little bit
    ptm.tm_yday += (username[0] % 7);

    // we don't calculate the real 'week of the year' here
    // because just to decide if we should check for an update
    // that's not needed.
    int week = ptm.tm_yday / 7;

    CRegDWORD oldweek = CRegDWORD(_T("Software\\TortoiseSVN\\CheckNewerWeek"), (DWORD)-1);
    if (((DWORD)oldweek) == -1)
    {
        oldweek = week;     // first start of TortoiseProc, no update check needed
        return;
    }
    if ((DWORD)week == oldweek)
        return;

    oldweek = week;

    TCHAR com[MAX_PATH+100];
    GetModuleFileName(NULL, com, MAX_PATH);

    CCreateProcessHelper::CreateProcessDetached(com, L" /command:updatecheck");
}
