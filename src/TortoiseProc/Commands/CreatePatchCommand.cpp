// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2013 - TortoiseSVN

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
#include "CreatePatchCommand.h"

#include "PathUtils.h"
#include "StringUtils.h"
#include "AppUtils.h"
#include "CreatePatchDlg.h"
#include "DiffOptionsDlg.h"
#include "SVN.h"
#include "TempFile.h"
#include "ProgressDlg.h"
#include "SelectFileFilter.h"
#include "SmartHandle.h"
#include "PreserveChdir.h"

#define PATCH_TO_CLIPBOARD_PSEUDO_FILENAME      _T(".TSVNPatchToClipboard")



bool CreatePatchCommand::Execute()
{
    bool bRet = false;
    CString savepath = CPathUtils::GetLongPathname(parser.GetVal(_T("savepath")));
    CCreatePatch dlg;
    dlg.m_pathList = pathList;
    if (parser.HasKey(_T("noui"))||(dlg.DoModal()==IDOK))
    {
        if (cmdLinePath.IsEmpty())
        {
            cmdLinePath = pathList.GetCommonRoot();
        }
        if (parser.HasKey(L"showoptions"))
        {
            CDiffOptionsDlg optionsdlg(CWnd::FromHandle(GetExplorerHWND()));
            if (optionsdlg.DoModal() == IDOK)
                dlg.m_sDiffOptions = optionsdlg.GetDiffOptionsString();
            else
                return false;
        }
        bRet = CreatePatch(cmdLinePath.GetDirectory(), dlg.m_pathList, dlg.m_sDiffOptions, CTSVNPath(savepath));
        SVN svn;
        svn.Revert(dlg.m_filesToRevert, CStringArray(), false);
    }
    return bRet;
}

UINT_PTR CALLBACK CreatePatchCommand::CreatePatchFileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    if(uiMsg == WM_COMMAND && LOWORD(wParam) == IDC_PATCH_TO_CLIPBOARD)
    {
        HWND hFileDialog = GetParent(hDlg);

        CString strFilename = CTempFiles::Instance().GetTempFilePath(false).GetWinPathString() + PATCH_TO_CLIPBOARD_PSEUDO_FILENAME;

        CommDlg_OpenSave_SetControlText(hFileDialog, edt1, (LPCTSTR)strFilename);

        PostMessage(hFileDialog, WM_COMMAND, MAKEWPARAM(IDOK, BM_CLICK), (LPARAM)(GetDlgItem(hDlg, IDOK)));
    }
    return 0;
}

bool CreatePatchCommand::CreatePatch(const CTSVNPath& root, const CTSVNPathList& paths, const CString& diffoptions, const CTSVNPath& cmdLineSavePath)
{
    CTSVNPath savePath;
    BOOL gitFormat = false;
    BOOL ignoreproperties = false;

    if (cmdLineSavePath.IsEmpty())
    {
        PreserveChdir preserveDir;
        HRESULT hr;
        // Create a new common save file dialog
        CComPtr<IFileSaveDialog> pfd = NULL;
        hr = pfd.CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER);
        if (SUCCEEDED(hr))
        {
            // Set the dialog options
            DWORD dwOptions;
            if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
            {
                hr = pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            }

            // Set a title
            if (SUCCEEDED(hr))
            {
                CString temp;
                temp.LoadString(IDS_REPOBROWSE_SAVEAS);
                CStringUtils::RemoveAccelerators(temp);
                pfd->SetTitle(temp);
            }
            CSelectFileFilter fileFilter(IDS_PATCHFILEFILTER);
            hr = pfd->SetFileTypes(fileFilter.GetCount(), fileFilter);
            if (paths.GetCount() == 1)
                pfd->SetFileName(paths[0].GetFilename() + L".patch");
            // set the default folder
            if (SUCCEEDED(hr))
            {
                typedef HRESULT (WINAPI *SHCIFPN)(PCWSTR pszPath, IBindCtx * pbc, REFIID riid, void ** ppv);

                CAutoLibrary hLib = AtlLoadSystemLibraryUsingFullPath(L"shell32.dll");
                if (hLib)
                {
                    SHCIFPN pSHCIFPN = (SHCIFPN)GetProcAddress(hLib, "SHCreateItemFromParsingName");
                    if (pSHCIFPN)
                    {
                        IShellItem* psiDefault = 0;
                        hr = pSHCIFPN(root.GetWinPath(), NULL, IID_PPV_ARGS(&psiDefault));
                        if (SUCCEEDED(hr))
                        {
                            hr = pfd->SetFolder(psiDefault);
                            psiDefault->Release();
                        }
                    }
                }
            }
            bool bAdvised = false;
            DWORD dwCookie = 0;
            CComObjectStackEx<PatchSaveDlgEventHandler> cbk;
            CComQIPtr<IFileDialogEvents> pEvents = cbk.GetUnknown();

            {
                CComPtr<IFileDialogCustomize> pfdCustomize;
                hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdCustomize));
                if (SUCCEEDED(hr))
                {
                    pfdCustomize->StartVisualGroup(100, L"");
                    pfdCustomize->AddCheckButton(101, CString(MAKEINTRESOURCE(IDS_PATCH_SAVEGITFORMAT)), FALSE);
                    pfdCustomize->AddCheckButton(102, CString(MAKEINTRESOURCE(IDS_PATCH_INCLUDEPROPS)), TRUE);
                    pfdCustomize->AddPushButton(103, CString(MAKEINTRESOURCE(IDS_PATCH_COPYTOCLIPBOARD)));
                    pfdCustomize->EndVisualGroup();

                    hr = pfd->Advise(pEvents, &dwCookie);

                    bAdvised = SUCCEEDED(hr);
                }
            }

            // Show the save file dialog
            if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(GetExplorerHWND())))
            {
                CComPtr<IFileDialogCustomize> pfdCustomize;
                hr = pfd->QueryInterface(IID_PPV_ARGS(&pfdCustomize));
                if (SUCCEEDED(hr))
                {
                    pfdCustomize->GetCheckButtonState(101, &gitFormat);
                    pfdCustomize->GetCheckButtonState(102, &ignoreproperties);
                    ignoreproperties = !ignoreproperties;
                }

                // Get the selection from the user
                CComPtr<IShellItem> psiResult = NULL;
                hr = pfd->GetResult(&psiResult);
                if (bAdvised)
                    pfd->Unadvise(dwCookie);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszPath = NULL;
                    hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr))
                    {
                        savePath = CTSVNPath(pszPath);
                        CoTaskMemFree(pszPath);
                        if (savePath.GetFileExtension().IsEmpty())
                            savePath.AppendRawString(_T(".patch"));
                    }
                }
                else
                {
                    // no result, which means we closed the dialog in our button handler
                    savePath = CTSVNPath(PATCH_TO_CLIPBOARD_PSEUDO_FILENAME);
                }
            }
            else
            {
                if (bAdvised)
                    pfd->Unadvise(dwCookie);
                return FALSE;
            }
        }
        else
        {
            TCHAR szFile[MAX_PATH + 1] = {0};// buffer for file name
            OPENFILENAME ofn = {0};         // common dialog box structure
            // Initialize OPENFILENAME
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = GetExplorerHWND();
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = _countof(szFile);
            ofn.lpstrInitialDir = root.GetWinPath();

            if (paths.GetCount() == 1)
                wcscpy_s(szFile, paths[0].GetFilename() + L".patch");

            CString temp;
            temp.LoadString(IDS_REPOBROWSE_SAVEAS);
            CStringUtils::RemoveAccelerators(temp);
            if (temp.IsEmpty())
                ofn.lpstrTitle = NULL;
            else
                ofn.lpstrTitle = temp;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE | OFN_EXPLORER | OFN_ENABLEHOOK;

            ofn.hInstance = AfxGetResourceHandle();
            ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PATCH_FILE_OPEN_CUSTOM);
            ofn.lpfnHook = CreatePatchFileOpenHook;

            CSelectFileFilter fileFilter(IDS_PATCHFILEFILTER);
            ofn.lpstrFilter = fileFilter;
            ofn.nFilterIndex = 1;
            // Display the Open dialog box.
            if (GetSaveFileName(&ofn)==FALSE)
            {
                return FALSE;
            }
            savePath = CTSVNPath(ofn.lpstrFile);
            if (ofn.nFilterIndex == 1)
            {
                if (savePath.GetFileExtension().IsEmpty())
                    savePath.AppendRawString(_T(".patch"));
            }
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
    progDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
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

    CTSVNPath sDir = root;
    if (sDir.IsEmpty())
        sDir = paths.GetCommonRoot();

    SVN svn;
    for (int fileindex = 0; fileindex < paths.GetCount(); ++fileindex)
    {
        svn_depth_t depth = paths[fileindex].IsDirectory() ? svn_depth_empty : svn_depth_files;
        if (!svn.CreatePatch(paths[fileindex], SVNRev::REV_BASE, paths[fileindex], SVNRev::REV_WC, sDir.GetDirectory(), depth, false, false, false, true, false, !!gitFormat, !!ignoreproperties, false, diffoptions, true, tempPatchFilePath))
        {
            progDlg.Stop();
            svn.ShowErrorDialog(GetExplorerHWND(), paths[fileindex]);
            return FALSE;
        }
    }

    progDlg.Stop();
    if(bToClipboard)
    {
        // The user actually asked for the patch to be written to the clipboard
        FILE * inFile = 0;
        _tfopen_s(&inFile, tempPatchFilePath.GetWinPath(), _T("rb"));
        if(inFile)
        {
            CStringA sClipdata;
            char chunkBuffer[16384];
            while(!feof(inFile))
            {
                size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);
                sClipdata.Append(chunkBuffer, (int)readLength);
            }
            fclose(inFile);

            CStringUtils::WriteDiffToClipboard(sClipdata);
            CAppUtils::StartUnifiedDiffViewer(tempPatchFilePath.GetWinPathString(), tempPatchFilePath.GetFilename(), TRUE);
        }
    }
    else
        CAppUtils::StartUnifiedDiffViewer(tempPatchFilePath.GetWinPathString(), tempPatchFilePath.GetFilename());

    return TRUE;
}


STDMETHODIMP PatchSaveDlgEventHandler::OnButtonClicked( IFileDialogCustomize* pfdc, DWORD dwIDCtl )
{
    if (dwIDCtl == 103)
    {
        CComQIPtr<IFileSaveDialog> pDlg = pfdc;
        if (pDlg)
        {
            pDlg->SetFileName(PATCH_TO_CLIPBOARD_PSEUDO_FILENAME);
            pDlg->Close(S_OK);
        }
    }
    return S_OK;
}
