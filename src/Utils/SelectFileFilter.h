#pragma once

#include "auto_buffer.h"
#include "StringUtils.h"

class CSelectFileFilter {
public:
	CSelectFileFilter(UINT stringId);
	CSelectFileFilter() {}
	~CSelectFileFilter() {}

	operator TCHAR*() { return buffer; }
	void Load(UINT stringId);

private:
	auto_buffer<TCHAR> buffer;
};

inline CSelectFileFilter::CSelectFileFilter(UINT stringId)
{
	Load(stringId);
}

inline void CSelectFileFilter::Load(UINT stringId)
{
	buffer.reset();
	CString sFilter;
	sFilter.LoadString(stringId);
	const int bufferLength = sFilter.GetLength()+4;
	buffer.reset(bufferLength);
	_tcscpy_s (buffer, bufferLength, sFilter);
	CStringUtils::PipesToNulls(buffer);
}