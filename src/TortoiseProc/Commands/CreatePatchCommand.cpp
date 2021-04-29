// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2016, 2018, 2021 - TortoiseSVN

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
#include "PreserveChdir.h"

#define PATCH_TO_CLIPBOARD_PSEUDO_FILENAME L".TSVNPatchToClipboard"

bool CreatePatchCommand::Execute()
{
    bool         bRet     = false;
    auto         savePath = CPathUtils::GetLongPathname(parser.GetVal(L"savepath"));
    CCreatePatch dlg;
    dlg.m_pathList = pathList;
    if (parser.HasKey(L"noui") || (dlg.DoModal() == IDOK))
    {
        if (cmdLinePath.IsEmpty())
        {
            cmdLinePath = pathList.GetCommonRoot();
        }
        if (parser.HasKey(L"showoptions"))
        {
            CDiffOptionsDlg optionsDlg(CWnd::FromHandle(GetExplorerHWND()));
            optionsDlg.SetDiffOptions(dlg.m_diffOptions);
            if (optionsDlg.DoModal() == IDOK)
                dlg.m_diffOptions = optionsDlg.GetDiffOptions();
            else
                return false;
        }
        bRet = CreatePatch(pathList.GetCommonRoot(), dlg.m_pathList, dlg.m_bPrettyPrint, dlg.m_diffOptions, CTSVNPath(savePath.c_str()));
        SVN svn;
        svn.Revert(dlg.m_filesToRevert, CStringArray(), false, false, false);
    }
    return bRet;
}

bool CreatePatchCommand::CreatePatch(const CTSVNPath& root, const CTSVNPathList& paths, bool prettyprint, const CString& diffoptions, const CTSVNPath& cmdLineSavePath) const
{
    CTSVNPath savePath;
    BOOL      gitFormat        = false;
    BOOL      ignoreProperties = false;

    if (cmdLineSavePath.IsEmpty())
    {
        PreserveChdir preserveDir;
        HRESULT       hr;
        // Create a new common save file dialog
        CComPtr<IFileSaveDialog> pfd = nullptr;
        hr                           = pfd.CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER);
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
                pfd->SetFileName(paths[0].GetFileOrDirectoryName() + L".patch");
            else
                pfd->SetFileName(paths.GetCommonDirectory().GetFileOrDirectoryName() + L".patch");

            // set the default folder
            if (SUCCEEDED(hr))
            {
                CComPtr<IShellItem> psiDefault = 0;
                hr                             = SHCreateItemFromParsingName(root.GetWinPath(), NULL, IID_PPV_ARGS(&psiDefault));
                if (SUCCEEDED(hr))
                {
                    hr = pfd->SetFolder(psiDefault);
                }
            }
            bool                                        bAdvised = false;
            DWORD                                       dwCookie = 0;
            CComObjectStackEx<PatchSaveDlgEventHandler> cbk;
            CComQIPtr<IFileDialogEvents>                pEvents = cbk.GetUnknown();

            {
                CComPtr<IFileDialogCustomize> pfdCustomize;
                hr = pfd.QueryInterface(&pfdCustomize);
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
                hr = pfd.QueryInterface(&pfdCustomize);
                if (SUCCEEDED(hr))
                {
                    pfdCustomize->GetCheckButtonState(101, &gitFormat);
                    pfdCustomize->GetCheckButtonState(102, &ignoreProperties);
                    ignoreProperties = !ignoreProperties;
                }

                // Get the selection from the user
                CComPtr<IShellItem> psiResult = nullptr;
                hr                            = pfd->GetResult(&psiResult);
                if (bAdvised)
                    pfd->Unadvise(dwCookie);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszPath = nullptr;
                    hr            = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr))
                    {
                        savePath = CTSVNPath(pszPath);
                        CoTaskMemFree(pszPath);
                        if (savePath.GetFileExtension().IsEmpty())
                            savePath.AppendRawString(L".patch");
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
    }
    else
    {
        savePath = cmdLineSavePath;
    }

    // This is horrible and I should be ashamed of myself, but basically, the
    // the file-open dialog writes ".TSVNPatchToClipboard" to its file field if the user clicks
    // the "Save To Clipboard" button.
    bool bToClipboard = wcsstr(savePath.GetWinPath(), PATCH_TO_CLIPBOARD_PSEUDO_FILENAME) != nullptr;

    CProgressDlg progDlg;
    progDlg.SetTitle(IDS_PROC_PATCHTITLE);
    progDlg.SetShowProgressBar(false);
    progDlg.ShowModeless(CWnd::FromHandle(GetExplorerHWND()));
    progDlg.FormatNonPathLine(1, IDS_PROC_SAVEPATCHTO);
    if (bToClipboard)
    {
        progDlg.FormatNonPathLine(2, IDS_CLIPBOARD_PROGRESS_DEST);
    }
    else
    {
        progDlg.SetLine(2, savePath.GetUIPathString(), true);
    }

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
    for (int fileIndex = 0; fileIndex < paths.GetCount(); ++fileIndex)
    {
        svn_depth_t depth = paths[fileIndex].IsDirectory() ? svn_depth_empty : svn_depth_files;
        if (!svn.CreatePatch(paths[fileIndex], SVNRev::REV_BASE, paths[fileIndex], SVNRev::REV_WC, sDir.GetDirectory(), depth, false, false, false, true, false, !!gitFormat, !!ignoreProperties, false, prettyprint, diffoptions, true, tempPatchFilePath))
        {
            progDlg.Stop();
            svn.ShowErrorDialog(GetExplorerHWND(), paths[fileIndex]);
            return FALSE;
        }
    }

    progDlg.Stop();
    if (bToClipboard)
    {
        // The user actually asked for the patch to be written to the clipboard
        FILE* inFile = nullptr;
        _tfopen_s(&inFile, tempPatchFilePath.GetWinPath(), L"rb");
        if (inFile)
        {
            CStringA sClipdata;
            char     chunkBuffer[16384] = {0};
            while (!feof(inFile))
            {
                size_t readLength = fread(chunkBuffer, 1, sizeof(chunkBuffer), inFile);
                sClipdata.Append(chunkBuffer, static_cast<int>(readLength));
            }
            fclose(inFile);

            CStringUtils::WriteDiffToClipboard(sClipdata);
            if (!parser.HasKey(L"noview"))
                CAppUtils::StartUnifiedDiffViewer(tempPatchFilePath.GetWinPathString(), tempPatchFilePath.GetFilename(), TRUE);
        }
    }
    else if (!parser.HasKey(L"noview"))
        CAppUtils::StartUnifiedDiffViewer(tempPatchFilePath.GetWinPathString(), tempPatchFilePath.GetFilename());

    return TRUE;
}

STDMETHODIMP PatchSaveDlgEventHandler::OnButtonClicked(IFileDialogCustomize* pfdc, DWORD dwIDCtl)
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
