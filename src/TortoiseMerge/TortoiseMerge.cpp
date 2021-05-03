// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2014, 2016-2018, 2020-2021 - TortoiseSVN

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
#include <dlgs.h>
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "CmdLineParser.h"
#include "version.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "SelectFileFilter.h"
#include "FileDlgEventHandler.h"
#include "TempFile.h"
#include "TaskbarUUID.h"
#include "resource.h"

#ifdef _DEBUG
// ReSharper disable once CppInconsistentNaming
#    define new DEBUG_NEW
#endif

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Propsys.lib")

BEGIN_MESSAGE_MAP(CTortoiseMergeApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()

class PatchOpenDlgEventHandler : public CFileDlgEventHandler
{
public:
    PatchOpenDlgEventHandler() {}
    ~PatchOpenDlgEventHandler() override {}

    STDMETHODIMP OnButtonClicked(IFileDialogCustomize* pfdc, DWORD dwIDCtl) override
    {
        if (dwIDCtl == 101)
        {
            CComQIPtr<IFileOpenDialog> pDlg = pfdc;
            if (pDlg)
            {
                pDlg->Close(S_OK);
            }
        }
        return S_OK;
    }
};

CTortoiseMergeApp::CTortoiseMergeApp()
{
    EnableHtmlHelp();
}

// The one and only CTortoiseMergeApp object
CTortoiseMergeApp theApp;
CString           sOrigCwd;
CCrashReportTSVN  g_crasher(L"TortoiseMerge " _T(APP_X64_STRING));

CString g_sGroupingUuid;

// CTortoiseMergeApp initialization
BOOL CTortoiseMergeApp::InitInstance()
{
    SetDllDirectory(L"");
    setTaskIDPerUuid();
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());

    {
        DWORD len = GetCurrentDirectory(0, nullptr);
        if (len)
        {
            auto originalCurrentDirectory = std::make_unique<TCHAR[]>(len);
            if (GetCurrentDirectory(len, originalCurrentDirectory.get()))
            {
                sOrigCwd = originalCurrentDirectory.get();
                sOrigCwd = CPathUtils::GetLongPathname(static_cast<LPCWSTR>(sOrigCwd)).c_str();
            }
        }
    }

    //set the resource dll for the required language
    CRegDWORD loc    = CRegDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    long      langId = loc;
    CString   langDll;
    HINSTANCE hInst = nullptr;
    do
    {
        langDll.Format(L"%sLanguages\\TortoiseMerge%ld.dll", static_cast<LPCWSTR>(CPathUtils::GetAppParentDirectory()), langId);

        hInst            = LoadLibrary(langDll);
        CString sVer     = _T(STRPRODUCTVER);
        CString sFileVer = CPathUtils::GetVersionFromFile(langDll).c_str();
        if (sFileVer.Compare(sVer) != 0)
        {
            FreeLibrary(hInst);
            hInst = nullptr;
        }
        if (hInst)
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
    } while ((!hInst) && (langId != 0));
    TCHAR buf[6] = {0};
    wcscpy_s(buf, L"en");
    langId = loc;
    CString sHelpPath;
    sHelpPath = this->m_pszHelpFilePath;
    sHelpPath = sHelpPath.MakeLower();
    sHelpPath.Replace(L".chm", L"_en.chm");
    free(static_cast<void*>(const_cast<wchar_t*>(m_pszHelpFilePath)));
    m_pszHelpFilePath = _wcsdup(sHelpPath);
    sHelpPath         = CPathUtils::GetAppParentDirectory() + L"Languages\\TortoiseMerge_en.chm";
    do
    {
        GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf));
        CString sLang = L"_";
        sLang += buf;
        sHelpPath.Replace(L"_en", sLang);
        if (PathFileExists(sHelpPath))
        {
            free(static_cast<void*>(const_cast<wchar_t*>(m_pszHelpFilePath)));
            m_pszHelpFilePath = _wcsdup(sHelpPath);
            break;
        }
        sHelpPath.Replace(sLang, L"_en");
        GetLocaleInfo(MAKELCID(langId, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf));
        sLang += L"_";
        sLang += buf;
        sHelpPath.Replace(L"_en", sLang);
        if (PathFileExists(sHelpPath))
        {
            free(static_cast<void*>(const_cast<wchar_t*>(m_pszHelpFilePath)));
            m_pszHelpFilePath = _wcsdup(sHelpPath);
            break;
        }
        sHelpPath.Replace(sLang, L"_en");

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
    // We need to explicitly set the thread locale to the system default one to avoid possible problems with saving files in its original codepage
    // The problems occures when the language of OS differs from the regional settings
    // See the details here: http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100887
    SetThreadLocale(LOCALE_SYSTEM_DEFAULT);

    // InitCommonControls() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    InitCommonControls();

    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
    CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
    CMFCButton::EnableWindowsTheming();
    EnableTaskbarInteraction(FALSE);

    // Initialize all Managers for usage. They are automatically constructed
    // if not yet present
    InitContextMenuManager();
    InitKeyboardManager();
    InitTooltipManager();
    CMFCToolTipInfo params;
    params.m_bVislManagerTheme = TRUE;

    GetTooltipManager()->SetTooltipParams(
        AFX_TOOLTIP_TYPE_ALL,
        RUNTIME_CLASS(CMFCToolTipCtrl),
        &params);

    CCmdLineParser parser = CCmdLineParser(this->m_lpCmdLine);

    g_sGroupingUuid = parser.GetVal(L"groupuuid");

    if (parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        CString sHelpText;
        sHelpText.LoadString(IDS_COMMANDLINEHELP);
        MessageBox(nullptr, sHelpText, L"TortoiseMerge", MB_ICONINFORMATION);
        return FALSE;
    }

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
    SetRegistryKey(L"TortoiseMerge");

    if (CRegDWORD(L"Software\\TortoiseMerge\\Debug", FALSE) == TRUE)
        AfxMessageBox(AfxGetApp()->m_lpCmdLine, MB_OK | MB_ICONINFORMATION);

    // To create the main window, this code creates a new frame window
    // object and then sets it as the application's main window object
    CMainFrame* pFrame = new CMainFrame;
    if (!pFrame)
        return FALSE;
    m_pMainWnd = pFrame;

    // create and load the frame with its resources
    if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, nullptr))
        return FALSE;

    // Fill in the command line options
    pFrame->m_data.m_baseFile.SetFileName(parser.GetVal(L"base"));
    pFrame->m_data.m_baseFile.SetDescriptiveName(parser.GetVal(L"basename"));
    pFrame->m_data.m_baseFile.SetReflectedName(parser.GetVal(L"basereflectedname"));
    pFrame->m_data.m_theirFile.SetFileName(parser.GetVal(L"theirs"));
    pFrame->m_data.m_theirFile.SetDescriptiveName(parser.GetVal(L"theirsname"));
    pFrame->m_data.m_theirFile.SetReflectedName(parser.GetVal(L"theirsreflectedname"));
    pFrame->m_data.m_yourFile.SetFileName(parser.GetVal(L"mine"));
    pFrame->m_data.m_yourFile.SetDescriptiveName(parser.GetVal(L"minename"));
    pFrame->m_data.m_yourFile.SetReflectedName(parser.GetVal(L"minereflectedname"));
    pFrame->m_data.m_mergedFile.SetFileName(parser.GetVal(L"merged"));
    pFrame->m_data.m_mergedFile.SetDescriptiveName(parser.GetVal(L"mergedname"));
    pFrame->m_data.m_mergedFile.SetReflectedName(parser.GetVal(L"mergedreflectedname"));
    pFrame->m_data.m_sPatchPath = parser.HasVal(L"patchpath") ? parser.GetVal(L"patchpath") : L"";
    pFrame->m_data.m_sPatchPath.Replace('/', '\\');
    if (parser.HasKey(L"patchoriginal"))
        pFrame->m_data.m_sPatchOriginal = parser.GetVal(L"patchoriginal");
    if (parser.HasKey(L"patchpatched"))
        pFrame->m_data.m_sPatchPatched = parser.GetVal(L"patchpatched");
    pFrame->m_data.m_sDiffFile = parser.GetVal(L"diff");
    pFrame->m_data.m_sDiffFile.Replace('/', '\\');
    if (parser.HasKey(L"oneway"))
        pFrame->m_bOneWay = TRUE;
    if (parser.HasKey(L"diff"))
        pFrame->m_bOneWay = FALSE;
    if (parser.HasKey(L"reversedpatch"))
        pFrame->m_bReversedPatch = TRUE;
    if (parser.HasKey(L"saverequired"))
        pFrame->m_bSaveRequired = true;
    if (parser.HasKey(L"saverequiredonconflicts"))
        pFrame->m_bSaveRequiredOnConflicts = true;
    if (parser.HasKey(L"nosvnresolve"))
        pFrame->m_bAskToMarkAsResolved = false;
    if (pFrame->m_data.IsBaseFileInUse() && !pFrame->m_data.IsYourFileInUse() && pFrame->m_data.IsTheirFileInUse())
    {
        pFrame->m_data.m_yourFile.TransferDetailsFrom(pFrame->m_data.m_theirFile);
    }

    if ((!parser.HasKey(L"patchpath")) && (parser.HasVal(L"diff")))
    {
        // a patchfile was given, but not folder path to apply the patch to
        // If the patchfile is located inside a working copy, then use the parent directory
        // of the patchfile as the target directory, otherwise ask the user for a path.
        if (parser.HasKey(L"wc"))
            pFrame->m_data.m_sPatchPath = pFrame->m_data.m_sDiffFile.Left(pFrame->m_data.m_sDiffFile.ReverseFind('\\'));
        else
        {
            CBrowseFolder fbrowser;
            fbrowser.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
            if (fbrowser.Show(nullptr, pFrame->m_data.m_sPatchPath) == CBrowseFolder::CANCEL)
                return FALSE;
        }
    }

    if ((parser.HasKey(L"patchpath")) && (!parser.HasVal(L"diff")))
    {
        // A path was given for applying a patchfile, but
        // the patchfile itself was not.
        // So ask the user for that patchfile

        HRESULT hr;
        // Create a new common save file dialog
        CComPtr<IFileOpenDialog> pfd;
        hr = pfd.CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER);
        if (SUCCEEDED(hr))
        {
            // Set the dialog options
            DWORD dwOptions;
            if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
            {
                hr = pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            }

            // Set a title
            if (SUCCEEDED(hr))
            {
                CString temp;
                temp.LoadString(IDS_OPENDIFFFILETITLE);
                pfd->SetTitle(temp);
            }
            CSelectFileFilter fileFilter(IDS_PATCHFILEFILTER);
            hr                                                   = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);
            bool                                        bAdvised = false;
            DWORD                                       dwCookie = 0;
            CComObjectStackEx<PatchOpenDlgEventHandler> cbk;
            CComQIPtr<IFileDialogEvents>                pEvents = cbk.GetUnknown();

            {
                CComPtr<IFileDialogCustomize> pfdCustomize;
                hr = pfd.QueryInterface(&pfdCustomize);
                if (SUCCEEDED(hr))
                {
                    // check if there's a unified diff on the clipboard and
                    // add a button to the fileopen dialog if there is.
                    UINT cFormat = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
                    if ((cFormat) && (OpenClipboard(nullptr)))
                    {
                        HGLOBAL hglb = GetClipboardData(cFormat);
                        if (hglb)
                        {
                            pfdCustomize->AddPushButton(101, CString(MAKEINTRESOURCE(IDS_PATCH_COPYFROMCLIPBOARD)));
                            hr       = pfd->Advise(pEvents, &dwCookie);
                            bAdvised = SUCCEEDED(hr);
                        }
                        CloseClipboard();
                    }
                }
            }

            // Show the save file dialog
            if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(pFrame->m_hWnd)))
            {
                // Get the selection from the user
                CComPtr<IShellItem> psiResult;
                hr = pfd->GetResult(&psiResult);
                if (bAdvised)
                    pfd->Unadvise(dwCookie);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszPath = nullptr;
                    hr            = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr))
                    {
                        pFrame->m_data.m_sDiffFile = pszPath;
                        CoTaskMemFree(pszPath);
                    }
                }
                else
                {
                    // no result, which means we closed the dialog in our button handler
                    std::wstring sTempFile;
                    if (TrySavePatchFromClipboard(sTempFile))
                        pFrame->m_data.m_sDiffFile = sTempFile.c_str();
                }
            }
            else
            {
                if (bAdvised)
                    pfd->Unadvise(dwCookie);
                return FALSE;
            }
        }
    }

    if (pFrame->m_data.m_baseFile.GetFilename().IsEmpty() && pFrame->m_data.m_yourFile.GetFilename().IsEmpty())
    {
        int     nArgs;
        LPWSTR* szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        if (!szArgList)
            TRACE("CommandLineToArgvW failed\n");
        else
        {
            if (nArgs == 3 || nArgs == 4)
            {
                // Four parameters:
                // [0]: Program name
                // [1]: BASE file
                // [2]: my file
                // [3]: THEIR file (optional)
                // This is the same format CAppUtils::StartExtDiff
                // uses if %base and %mine are not set and most
                // other diff tools use it too.
                if (PathFileExists(szArgList[1]) && PathFileExists(szArgList[2]))
                {
                    pFrame->m_data.m_baseFile.SetFileName(szArgList[1]);
                    pFrame->m_data.m_yourFile.SetFileName(szArgList[2]);
                    if (nArgs == 4 && PathFileExists(szArgList[3]))
                    {
                        pFrame->m_data.m_theirFile.SetFileName(szArgList[3]);
                    }
                }
            }
            else if (nArgs == 2)
            {
                // only one path specified: use it to fill the "open" dialog
                if (PathFileExists(szArgList[1]))
                {
                    pFrame->m_data.m_yourFile.SetFileName(szArgList[1]);
                    pFrame->m_data.m_yourFile.StoreFileAttributes();
                }
            }
        }

        // Free memory allocated for CommandLineToArgvW arguments.
        LocalFree(szArgList);
    }

    pFrame->m_bReadOnly = !!parser.HasKey(L"readonly");
    if (GetFileAttributes(pFrame->m_data.m_yourFile.GetFilename()) & FILE_ATTRIBUTE_READONLY)
        pFrame->m_bReadOnly = true;
    pFrame->m_bBlame = !!parser.HasKey(L"blame");
    // diffing a blame means no editing!
    if (pFrame->m_bBlame)
        pFrame->m_bReadOnly = true;

    pFrame->SetWindowTitle();

    if (parser.HasKey(L"createunifieddiff"))
    {
        // user requested to create a unified diff file
        CString origFile     = parser.GetVal(L"origfile");
        CString modifiedFile = parser.GetVal(L"modifiedfile");
        if (!origFile.IsEmpty() && !modifiedFile.IsEmpty())
        {
            CString outfile = parser.GetVal(L"outfile");
            if (outfile.IsEmpty())
            {
                CCommonAppUtils::FileOpenSave(outfile, nullptr, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false, nullptr);
            }
            if (!outfile.IsEmpty())
            {
                CRegStdDWORD regContextLines(L"Software\\TortoiseMerge\\ContextLines", 0);
                CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outfile, regContextLines, true, false);
                return FALSE;
            }
        }
    }

    pFrame->resolveMsgWnd    = parser.HasVal(L"resolvemsghwnd") ? reinterpret_cast<HWND>(parser.GetLongLongVal(L"resolvemsghwnd")) : nullptr;
    pFrame->resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? static_cast<WPARAM>(parser.GetLongLongVal(L"resolvemsgwparam")) : 0;
    pFrame->resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? static_cast<LPARAM>(parser.GetLongLongVal(L"resolvemsglparam")) : 0;

    // The one and only window has been initialized, so show and update it
    pFrame->ActivateFrame();
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    pFrame->ShowDiffBar(!pFrame->m_bOneWay);
    if (!pFrame->m_data.IsBaseFileInUse() && pFrame->m_data.m_sPatchPath.IsEmpty() && pFrame->m_data.m_sDiffFile.IsEmpty())
    {
        pFrame->OnFileOpen(pFrame->m_data.m_yourFile.InUse());
        return TRUE;
    }

    int line = -2;
    if (parser.HasVal(L"line"))
    {
        line = parser.GetLongVal(L"line");
        line--; // we need the index
    }

    return pFrame->LoadViews(line);
}

// CTortoiseMergeApp message handlers

// ReSharper disable once CppMemberFunctionMayBeStatic
void CTortoiseMergeApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}

int CTortoiseMergeApp::ExitInstance()
{
    // Look for temporary files left around by TortoiseMerge and
    // remove them. But only delete 'old' files
    CTempFiles::DeleteOldTempFiles(L"*svn*.*");

    return CWinAppEx::ExitInstance();
}

bool CTortoiseMergeApp::HasClipboardPatch()
{
    // check if there's a patchfile in the clipboard
    const UINT cFormat = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
    if (cFormat == 0)
        return false;

    if (OpenClipboard(nullptr) == 0)
        return false;

    bool containsPatch = false;
    UINT enumFormat    = 0;
    do
    {
        if (enumFormat == cFormat)
        {
            containsPatch = true; // yes, there's a patchfile in the clipboard
        }
    } while ((enumFormat = EnumClipboardFormats(enumFormat)) != 0);
    CloseClipboard();

    return containsPatch;
}

bool CTortoiseMergeApp::TrySavePatchFromClipboard(std::wstring& resultFile)
{
    resultFile.clear();

    UINT cFormat = RegisterClipboardFormat(L"TSVN_UNIFIEDDIFF");
    if (cFormat == 0)
        return false;
    if (OpenClipboard(nullptr) == 0)
        return false;

    HGLOBAL hGlb  = GetClipboardData(cFormat);
    LPCSTR  lpStr = static_cast<LPCSTR>(GlobalLock(hGlb));

    DWORD len   = GetTempPath(0, nullptr);
    auto  path  = std::make_unique<TCHAR[]>(len + 1LL);
    auto  tempF = std::make_unique<TCHAR[]>(len + 100LL);
    GetTempPath(len + 1, path.get());
    GetTempFileName(path.get(), L"tsm", 0, tempF.get());
    std::wstring sTempFile = std::wstring(tempF.get());

    FILE* outFile = nullptr;
    _tfopen_s(&outFile, sTempFile.c_str(), L"wb");
    if (outFile != nullptr)
    {
        size_t patchLen = strlen(lpStr);
        size_t size     = fwrite(lpStr, sizeof(char), patchLen, outFile);
        if (size == patchLen)
            resultFile = sTempFile;

        fclose(outFile);
    }
    GlobalUnlock(hGlb);
    CloseClipboard();

    return !resultFile.empty();
}
