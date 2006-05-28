// stdafx.cpp : source file that includes just the standard includes
//	CrashRpt.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#if (_ATL_VER < 0x0700)
#include <atlimpl.cpp>
#endif //(_ATL_VER < 0x0700)

//////////////////////////////////////////////////////////////////////
// how shall addresses be formatted?
//////////////////////////////////////////////////////////////////////

const LPCTSTR addressFormat = sizeof (void*) <= 4
	? _T("0x%08x")
	: _T("0x%016x");

const LPCTSTR sizeFormat = _T("0x%08x");
const LPCTSTR offsetFormat = _T("0x%x");

