// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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
#include <Windows.h>

/**
 * \ingroup Utils
 * Trace macro for win32 applications where the MFC or ATL trace macro is
 * not available.
 */
void TRACE(LPCTSTR str, ...);


class CTraceToOutputDebugString
{
public:
	static CTraceToOutputDebugString& Instance()
	{
		if (m_pInstance == NULL)
			m_pInstance = new CTraceToOutputDebugString;
		return *m_pInstance;
	}

    // Non Unicode output helper
    void operator()(PCSTR pszFormat, ...)
    {
		if (m_bActive)
		{
			va_list ptr;
			va_start(ptr, pszFormat);
			TraceV(pszFormat,ptr);
			va_end(ptr);
		}
    }
 
    // Unicode output helper
    void operator()(PCWSTR pszFormat, ...)
    {
		if (m_bActive)
		{
			va_list ptr;
			va_start(ptr, pszFormat);
			TraceV(pszFormat,ptr);
			va_end(ptr);
		}
    }
 
private:
	CTraceToOutputDebugString()
	{
		m_bActive = !!CRegStdDWORD(_T("Software\\TortoiseSVN\\Debug"), FALSE);
	}
	~CTraceToOutputDebugString()
	{
		delete m_pInstance;
	}

	bool m_bActive;
	static CTraceToOutputDebugString& m_pInstance;

	// Non Unicode output helper
    void TraceV(PCSTR pszFormat, va_list args)
    {
        // Format the output buffer
        char szBuffer[1024];
        _vsnprintf(szBuffer, _countof(szBuffer), pszFormat, args);
        OutputDebugStringA(szBuffer);
    }
 
    // Unicode output helper
    void TraceV(PCWSTR pszFormat, va_list args)
    {
        wchar_t szBuffer[1024];
        _vsnwprintf(szBuffer, _countof(szBuffer), pszFormat, args);
        OutputDebugStringW(szBuffer);
    }
};