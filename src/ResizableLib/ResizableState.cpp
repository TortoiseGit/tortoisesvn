// ResizableState.cpp: implementation of the CResizableState class.
//
/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004 by Paolo Messina
// http://www.geocities.com/ppescher - mailto:ppescher@hotmail.com
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizableState.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizableState::CResizableState()
{
	m_sStorePath = m_sDefaultStorePath;
}

CResizableState::~CResizableState()
{

}

// static intializer must be called before user code
#pragma warning(disable:4073)
#pragma init_seg(lib)
CString CResizableState::m_sDefaultStorePath(_T("ResizableState"));


void CResizableState::SetDefaultStateStore(LPCTSTR szPath)
{
	m_sDefaultStorePath = szPath;
}

LPCTSTR CResizableState::GetDefaultStateStore()
{
	return m_sDefaultStorePath;
}

void CResizableState::SetStateStore(LPCTSTR szPath)
{
	m_sStorePath = szPath;
}

LPCTSTR CResizableState::GetStateStore()
{
	return m_sStorePath;
}

BOOL CResizableState::WriteState(LPCTSTR szId, LPCTSTR szState)
{
	return AfxGetApp()->WriteProfileString(GetStateStore(), szId, szState);
}

BOOL CResizableState::ReadState(LPCTSTR szId, CString &rsState)
{
	rsState = AfxGetApp()->GetProfileString(GetStateStore(), szId);
	return !rsState.IsEmpty();
}
