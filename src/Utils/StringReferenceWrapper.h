// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2015, 2020-2021 - TortoiseSVN

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
#pragma once

using FWindowsCreateStringReference = HRESULT(FAR STDAPICALLTYPE *)(_In_reads_opt_(length + 1) PCWSTR sourceString, UINT32 length, _Out_ HSTRING_HEADER *hstringHeader, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *string);

using FWindowsDeleteString = HRESULT(FAR STDAPICALLTYPE *)(_In_opt_ HSTRING string);

class StringReferenceWrapperDynamicLoader
{
private:
    StringReferenceWrapperDynamicLoader()
        : m_hLib(nullptr)
        , windowsCreateStringReference(nullptr)
        , windowsDeleteString(nullptr)
    {
        m_hLib = LoadLibrary(L"api-ms-win-core-winrt-string-l1-1-0.dll");
        if (m_hLib)
        {
            windowsCreateStringReference = reinterpret_cast<FWindowsCreateStringReference>(GetProcAddress(m_hLib, "WindowsCreateStringReference"));
            windowsDeleteString          = reinterpret_cast<FWindowsDeleteString>(GetProcAddress(m_hLib, "WindowsDeleteString"));
        }
    }
    ~StringReferenceWrapperDynamicLoader()
    {
        if (m_hLib)
            FreeLibrary(m_hLib);
    }

    HMODULE                       m_hLib;
    FWindowsCreateStringReference windowsCreateStringReference;
    FWindowsDeleteString          windowsDeleteString;

public:
    static StringReferenceWrapperDynamicLoader &Instance()
    {
        static StringReferenceWrapperDynamicLoader instance;
        return instance;
    }

    HRESULT WINAPI WindowsCreateStringReference(_In_ PCWSTR sourceString, _In_ UINT32 length, _Out_ HSTRING_HEADER *hstringHeader, _Out_ HSTRING *string) const
    {
        if (windowsCreateStringReference)
            return windowsCreateStringReference(sourceString, length, hstringHeader, string);
        return E_FAIL;
    }

    HRESULT WINAPI WindowsDeleteString(_In_ HSTRING string) const
    {
        if (windowsDeleteString)
            return windowsDeleteString(string);
        return E_FAIL;
    }
};

class StringReferenceWrapper
{
public:
    // Constructor which takes an existing string buffer and its length as the parameters.
    // It fills an HSTRING_HEADER struct with the parameter.
    // Warning: The caller must ensure the lifetime of the buffer outlives this
    // object as it does not make a copy of the wide string memory.

    StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length)
    {
        HRESULT hr = StringReferenceWrapperDynamicLoader::Instance().WindowsCreateStringReference(stringRef, length, &m_header, &m_hstring);

        if (FAILED(hr))
        {
            RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
    }

    StringReferenceWrapper(_In_reads_(length) const CString &stringRef)
    {
        HRESULT hr = StringReferenceWrapperDynamicLoader::Instance().WindowsCreateStringReference(stringRef, stringRef.GetLength(), &m_header, &m_hstring);

        if (FAILED(hr))
        {
            RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
    }

    ~StringReferenceWrapper()
    {
        StringReferenceWrapperDynamicLoader::Instance().WindowsDeleteString(m_hstring);
    }

    template <size_t N>
    StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N])
    {
        UINT32  length = N - 1;
        HRESULT hr     = StringReferenceWrapperDynamicLoader::Instance().WindowsCreateStringReference(stringRef, length, &m_header, &m_hstring);

        if (FAILED(hr))
        {
            RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }
    }

    template <size_t _>
    StringReferenceWrapper(_In_reads_(_) wchar_t (&stringRef)[_])
    {
        UINT32  length = 0;
        HRESULT hr     = SizeTToUInt32(wcslen(stringRef), &length);

        if (FAILED(hr))
        {
            RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
        }

        StringReferenceWrapperDynamicLoader::Instance().WindowsCreateStringReference(stringRef, length, &m_header, &m_hstring);
    }

    HSTRING Get() const
    {
        return m_hstring;
    }

private:
    HSTRING        m_hstring;
    HSTRING_HEADER m_header;
};
