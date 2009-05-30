// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "JumpListHelpers.h"
#include "propvarutil.h"
#include "propsys.h"

HRESULT SetAppID(LPCTSTR appID)
{
	HRESULT hRes = S_FALSE;
	typedef HRESULT STDAPICALLTYPE SetCurrentProcessExplicitAppUserModelIDFN(PCWSTR AppID);
	HMODULE hShell = ::LoadLibrary(_T("shell32.dll"));
	if (hShell)
	{
		SetCurrentProcessExplicitAppUserModelIDFN *pfnSetCurrentProcessExplicitAppUserModelID = (SetCurrentProcessExplicitAppUserModelIDFN*)GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID");
		if (pfnSetCurrentProcessExplicitAppUserModelID)
		{
			hRes = pfnSetCurrentProcessExplicitAppUserModelID(appID);
		}
		FreeLibrary(hShell);
	}
	return hRes;
}

HRESULT CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, int iconIndex, IShellLink **ppsl)
{
    IShellLink *psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (SUCCEEDED(hr))
    {
        WCHAR szAppPath[MAX_PATH];
        if (GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath)))
        {
            hr = psl->SetPath(szAppPath);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetArguments(pszArguments);
                if (SUCCEEDED(hr))
                {
					hr = psl->SetIconLocation(szAppPath, iconIndex);
					if (SUCCEEDED(hr))
					{
						IPropertyStore *pps;
						hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
						if (SUCCEEDED(hr))
						{
							PROPVARIANT propvar;
							hr = InitPropVariantFromString(pszTitle, &propvar);
							if (SUCCEEDED(hr))
							{
								hr = pps->SetValue(PKEY_Title, propvar);
								if (SUCCEEDED(hr))
								{
									hr = pps->Commit();
									if (SUCCEEDED(hr))
									{
										hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
									}
								}
								PropVariantClear(&propvar);
							}
							pps->Release();
						}
					}
                }
            }
        }
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
        psl->Release();
    }    
    return hr;
}

HRESULT CreateSeparatorLink(IShellLink **ppsl)
{
    IPropertyStore *pps;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT propvar;
        hr = InitPropVariantFromBoolean(TRUE, &propvar);
        if (SUCCEEDED(hr))
        {
			hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
            if (SUCCEEDED(hr))
            {
                hr = pps->Commit();
                if (SUCCEEDED(hr))
                {
                    hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
                }
            }
            PropVariantClear(&propvar);
        }
        pps->Release();
	}
	return hr;
}

bool IsItemInArray(IShellItem *psi, IObjectArray *poaRemoved)
{
	bool fRet = false;
	UINT cItems;
	if (SUCCEEDED(poaRemoved->GetCount(&cItems)))
	{
		IShellItem *psiCompare;
		for (UINT i = 0; !fRet && i < cItems; i++)
		{
			if (SUCCEEDED(poaRemoved->GetAt(i, IID_PPV_ARGS(&psiCompare))))
			{
				int iOrder;
				fRet = SUCCEEDED(psiCompare->Compare(psi, SICHINT_CANONICAL, &iOrder)) && (0 == iOrder);
				psiCompare->Release();
			}
		}
	}
	return fRet;
}

void DeleteJumpList(LPCTSTR appID)
{
	ICustomDestinationList *pcdl;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
	if (SUCCEEDED(hr))
	{
		hr = pcdl->DeleteList(appID);
		pcdl->Release();
	}
}