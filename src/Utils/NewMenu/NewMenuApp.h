// NewMenuApp.h : main header file for the NewMenu DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CNewMenuApp
// See NewMenu.cpp for the implementation of this class
//

class CNewMenuApp : public CWinApp
{
public:
	CNewMenuApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
