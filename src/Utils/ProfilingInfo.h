#pragma once

// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2011 - TortoiseSVN 

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

//////////////////////////////////////////////////////////////////////
// required includes
//////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
#ifndef __INTRIN_H_
#include <intrin.h>
#endif
#endif

#ifndef _PSAPI_H_
#include <psapi.h>
#endif

#ifndef _STRING_
#include <string>
#endif

#ifndef _VECTOR_
#include <vector>
#endif

#pragma comment (lib, "psapi.lib")

/**
* Collects the profiling info for a given profiled block / line.
* Records execution count, min, max and accumulated execution time
* in CPU clock ticks.
*/

class CProfilingRecord
{
public:
    /// collected profiling info

    struct CSpent {
        unsigned __int64 sum;
        unsigned __int64 minValue;
        unsigned __int64 maxValue;

        void Add(unsigned __int64  value)
        {
            sum += value;

            if (value < minValue)
                minValue = value;
            if (value > maxValue)
                maxValue = value;
        }
        /// First value add
        void Init(unsigned __int64  value)
        {
            sum = value;
            minValue = value;
            maxValue = value;
        }
        void Init()
        {
            sum = 0;
            minValue = ULLONG_MAX; /* -1 */
            maxValue = 0;
        }
    };

    /// construction

    CProfilingRecord ( const char* name
                     , const char* file
                     , int line);

    /// record values

    void Add ( unsigned __int64 valueRdtsc
             , unsigned __int64 valueWall
             , unsigned __int64 valueUser
             , unsigned __int64 valueKernel
             );

    /// modification

    void Reset();

    /// data access

    const char* GetName() const {return name;}
    const char* GetFile() const {return file;}
    int GetLine() const {return line;}

    size_t GetCount() const {return count;}
    const CSpent & Get() const {return m_rdtsc; }
    const CSpent & GetU() const {return m_user; }
    const CSpent & GetK() const {return m_kernel; }
    const CSpent & GetW() const {return m_wall; }


private:

    /// identification

    const char* name;
    const char* file;
    int line;

    CSpent m_rdtsc, m_user, m_kernel, m_wall;
    size_t count;
};

/**
* RAII class that encapsulates a single execution of a profiled
* block / line. The result gets added to an existing profiling record.
*/

class CRecordProfileEvent
{
private:

    CProfilingRecord* record;

    /// the initial counter values
    unsigned __int64 m_rdtscStart;
    FILETIME m_kernelStart;
    FILETIME m_userStart;
    FILETIME m_wallStart;

    /// Temp object for parameters we don't care, but can't be NULL
    static FILETIME ftTemp;

public:

    /// construction: start clock

    CRecordProfileEvent (CProfilingRecord* aRecord);

    /// destruction: time interval to profiling record,
    /// if Stop() had not been called before

    ~CRecordProfileEvent();

    /// don't wait for destruction

    void Stop();
};

/// construction / destruction

inline CRecordProfileEvent::CRecordProfileEvent (CProfilingRecord* aRecord)
    : record (aRecord)
{
    // less precise first
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &m_wallStart);

    GetProcessTimes(GetCurrentProcess(), &ftTemp, &ftTemp, &m_kernelStart, &m_userStart);

    m_rdtscStart = __rdtsc();
}

inline CRecordProfileEvent::~CRecordProfileEvent()
{
    Stop();
}

UINT64 inline DiffFiletime(FILETIME time1, FILETIME time2)
{
    return *(UINT64 *)&time1 - *(UINT64 *)&time2;
}

/// stop counting

inline void CRecordProfileEvent::Stop()
{
    if (record)
    {
        // more precise first
        unsigned __int64 nTake = __rdtsc() - m_rdtscStart;

        FILETIME kernelEnd, userEnd;
        GetProcessTimes(GetCurrentProcess(), &ftTemp, &ftTemp, &kernelEnd, &userEnd);

        SYSTEMTIME st;
        GetSystemTime(&st);
        FILETIME oTime;
        SystemTimeToFileTime(&st, &oTime);

        record->Add (nTake
                   , DiffFiletime(oTime, m_wallStart)
                   , DiffFiletime(userEnd, m_userStart)
                   , DiffFiletime(kernelEnd, m_kernelStart));
        record = NULL;
    }
}

/**
* Singleton class that acts as container for all profiling records.
* You may reset its content as well as write it to disk.
*/

class CProfilingInfo
{
private:

    typedef std::vector<CProfilingRecord*> TRecords;
    TRecords records;

    /// construction / destruction

    CProfilingInfo();
    ~CProfilingInfo(void);

    /// create report

    std::string GetReport() const;

public:

    /// access to default instance

    static CProfilingInfo* GetInstance();

    /// add a new record

    CProfilingRecord* Create ( const char* name
                             , const char* file
                             , int line);

    /// write the current results to disk

    void DumpReport();
};

/**
* Profiling macros
*/

#define PROFILE_CONCAT( a, b )   PROFILE_CONCAT3( a, b )
#define PROFILE_CONCAT3( a, b )  a##b

/// measures the time from the point of usage to the end of the respective block

#define PROFILE_BLOCK\
    static CProfilingRecord* PROFILE_CONCAT(record,__LINE__) \
        = CProfilingInfo::GetInstance()->Create(__FUNCTION__,__FILE__,__LINE__);\
    CRecordProfileEvent PROFILE_CONCAT(profileSection,__LINE__) (PROFILE_CONCAT(record,__LINE__));

/// measures the time taken to execute the respective code line

#define PROFILE_LINE(line)\
    static CProfilingRecord* PROFILE_CONCAT(record,__LINE__) \
        = CProfilingInfo::GetInstance()->Create(__FUNCTION__,__FILE__,__LINE__);\
    CRecordProfileEvent PROFILE_CONCAT(profileSection,__LINE__) (PROFILE_CONCAT(record,__LINE__));\
    line;\
    PROFILE_CONCAT(profileSection,__LINE__).Stop();

