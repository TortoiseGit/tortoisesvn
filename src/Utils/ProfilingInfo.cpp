// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2012 - TortoiseSVN

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

#ifndef __INTRIN_H_
#include <intrin.h>
#endif

#include <fstream>

#include "ProfilingInfo.h"


//////////////////////////////////////////////////////////////////////
// construction / destruction
//////////////////////////////////////////////////////////////////////

CProfilingRecord::CProfilingRecord ( const char* name
                                   , const char* file
                                   , int line)
    : name (name)
    , file (file)
    , line (line)
    , count (0)
{
    Reset();
}

//////////////////////////////////////////////////////////////////////
// record values
//////////////////////////////////////////////////////////////////////

void CProfilingRecord::Add (unsigned __int64 valueRdtsc
    , unsigned __int64 valueTime
    , unsigned __int64 valueUser
    , unsigned __int64 valueKernel
)
{
    if (!count++)
    {
        m_rdtsc.Init(valueRdtsc);
        m_user.Init(valueUser);
        m_kernel.Init(valueKernel);
        m_wall.Init(valueTime);
    }
    else
    {
        m_rdtsc.Add(valueRdtsc);
        m_user.Add(valueUser);
        m_kernel.Add(valueKernel);
        m_wall.Add(valueTime);
    }
}

//////////////////////////////////////////////////////////////////////
// modification
//////////////////////////////////////////////////////////////////////

void CProfilingRecord::Reset()
{
    count = 0;
    // we don't need to init other then count, to be save we do
    m_rdtsc.Init();
    m_user.Init();
    m_kernel.Init();
    m_wall.Init();
}

//////////////////////////////////////////////////////////////////////
// construction / destruction
//////////////////////////////////////////////////////////////////////

CProfilingInfo::CProfilingInfo()
{
}

CProfilingInfo::~CProfilingInfo(void)
{
    DumpReport();

    // free data

    for (size_t i = 0; i < records.size(); ++i)
        delete records[i];
}

//////////////////////////////////////////////////////////////////////
// access to default instance
//////////////////////////////////////////////////////////////////////

CProfilingInfo* CProfilingInfo::GetInstance()
{
    static CProfilingInfo instance;
    return &instance;
}

void CProfilingInfo::DumpReport()
{
    if (!records.empty())
    {
        // write profile to file

#ifdef _WIN32
        char buffer [MAX_PATH];
        if (GetModuleFileNameExA (GetCurrentProcess(), NULL, buffer, _countof(buffer)) > 0)
#else
        const char* buffer = "application";
#endif

        try
        {
            std::string fileName (buffer);
            fileName += ".profile";

            std::string report = GetInstance()->GetReport();

            std::ofstream file;
            file.open (fileName.c_str(), std::ios::binary | std::ios::out);
            file.write (report.c_str(), report.size());
        }
        catch (...)
        {
            // ignore all file errors etc.
        }
    }
}

//////////////////////////////////////////////////////////////////////
// create a report
//////////////////////////////////////////////////////////////////////

static std::string IntToStr (unsigned __int64 value)
{
    char buffer[100];
    _ui64toa_s (value, buffer, _countof(buffer), 10);

    std::string result = buffer;
    for (size_t i = 3; i < result.length(); i += 4)
        result.insert (result.length() - i, 1, ',');

    return result;
};

std::string CProfilingInfo::GetReport() const
{
    enum { LINE_LENGTH = 600 };

    char lineBuffer [LINE_LENGTH];
    std::string result;
    result.reserve (LINE_LENGTH * records.size());

    const char * const format ="%15s%17s%17s%17s%17s\n";

    for ( TRecords::const_iterator iter = records.begin(), end = records.end()
        ; iter != end
        ; ++iter)
    {
        size_t nCount = (*iter)->GetCount();
        sprintf_s ( lineBuffer, "%7sx %s\n%s:%s\n"
                  , IntToStr (nCount).c_str()
                  , (*iter)->GetName()
                  , (*iter)->GetFile()
                  , IntToStr ((*iter)->GetLine()).c_str());
        result += lineBuffer;
        if (nCount==0)
            continue;

        sprintf_s ( lineBuffer, format
                  , "type", "sum", "avg", "min", "max");
        result += lineBuffer;

        sprintf_s ( lineBuffer, format
                  , "CPU Ticks"
                  , IntToStr ((*iter)->Get().sum).c_str()
                  , IntToStr ((*iter)->Get().sum/nCount).c_str()
                  , IntToStr ((*iter)->Get().minValue).c_str()
                  , IntToStr ((*iter)->Get().maxValue).c_str());
        result += lineBuffer;

        sprintf_s ( lineBuffer, format
                  , "UserMode[us]"
                  , IntToStr ((*iter)->GetU().sum/10).c_str()
                  , IntToStr ((*iter)->GetU().sum/10/nCount).c_str()
                  , IntToStr ((*iter)->GetU().minValue/10).c_str()
                  , IntToStr ((*iter)->GetU().maxValue/10).c_str());
        result += lineBuffer;

        sprintf_s ( lineBuffer, format
                  , "KernelMode[us]"
                  , IntToStr ((*iter)->GetK().sum/10).c_str()
                  , IntToStr ((*iter)->GetK().sum/10/nCount).c_str()
                  , IntToStr ((*iter)->GetK().minValue/10).c_str()
                  , IntToStr ((*iter)->GetK().maxValue/10).c_str());
        result += lineBuffer;

        sprintf_s ( lineBuffer, format
                  , "WallTime[us]"
                  , IntToStr ((*iter)->GetW().sum/10).c_str()
                  , IntToStr ((*iter)->GetW().sum/10/nCount).c_str()
                  , IntToStr ((*iter)->GetW().minValue/10).c_str()
                  , IntToStr ((*iter)->GetW().maxValue/10).c_str());
        result += lineBuffer;

        result += "\n";
    }

    // now print the processor speed read from the registry: the user may want to
    // convert the processor ticks to seconds
    HKEY hKey;
    // open the key where the proc speed is hidden:
    long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                               0,
                               KEY_READ,
                               &hKey);

    if(lError == ERROR_SUCCESS)
    {
        // query the key:
        DWORD BufSize = MAX_PATH;
        DWORD dwMHz = 0;
        RegQueryValueEx(hKey, L"~MHz", NULL, NULL, (LPBYTE) &dwMHz, &BufSize);
        RegCloseKey(hKey);

        sprintf_s ( lineBuffer, "processor speed is %ld MHz\n", dwMHz);
        result += lineBuffer;
    }


    return result;
}

//////////////////////////////////////////////////////////////////////
// add a new record
//////////////////////////////////////////////////////////////////////

CProfilingRecord* CProfilingInfo::Create ( const char* name,
                                           const char* file,
                                           int line)
{
    CProfilingRecord* record = new CProfilingRecord (name, file, line);
    records.push_back (record);

    return record;
}


FILETIME CRecordProfileEvent::ftTemp;
