// TortoiseMerge.h : main header file for the TortoiseMerge application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "CrashReport.h"


// CTortoiseMergeApp:
// See TortoiseMerge.cpp for the implementation of this class
//

class CTortoiseMergeApp : public CWinApp
{
public:
	CTortoiseMergeApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CTortoiseMergeApp theApp;
extern CCrashReport g_crasher;