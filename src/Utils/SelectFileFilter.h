#pragma once

#include "auto_buffer.h"
#include "StringUtils.h"

class CSelectFileFilter {
public:
    CSelectFileFilter(UINT stringId);
    CSelectFileFilter() {}
    ~CSelectFileFilter() {}

    operator const TCHAR*() { return buffer; }
    void Load(UINT stringId);
    UINT GetCount() { return (UINT)filternames.size(); }
    operator const COMDLG_FILTERSPEC*() { return filterspec; }

private:
    auto_buffer<TCHAR> buffer;
    auto_buffer<COMDLG_FILTERSPEC> filterspec;
    std::vector<CString> filternames;
    std::vector<CString> filtermasks;

    void ResetAll();
};

inline CSelectFileFilter::CSelectFileFilter(UINT stringId)
{
    Load(stringId);
}

inline void CSelectFileFilter::Load(UINT stringId)
{
    ResetAll();
    CString sFilter;
    sFilter.LoadString(stringId);
    const int bufferLength = sFilter.GetLength()+4;
    buffer.reset(bufferLength);
    _tcscpy_s (buffer, bufferLength, sFilter);
    CStringUtils::PipesToNulls(buffer);
    //Certificates|*.p12;*.pkcs12;*.pfx|All|*.*||
    int pos = 0;
    CString temp;
    for (;;)
    {
        temp = sFilter.Tokenize(_T("|"), pos);
        if (temp.IsEmpty())
        {
            break;
        }
        filternames.push_back(temp);
        temp = sFilter.Tokenize(_T("|"), pos);
        filtermasks.push_back(temp);
    }
    filterspec.reset(filternames.size());
    for (size_t i = 0; i < filternames.size(); ++i)
    {
        filterspec[i].pszName = filternames[i];
        filterspec[i].pszSpec = filtermasks[i];
    }
}

inline void CSelectFileFilter::ResetAll()
{
    buffer.reset();
    // First release the struct that references the vectors, then clear the vectors
    filterspec.reset();
    filternames.clear();
    filtermasks.clear();
}
