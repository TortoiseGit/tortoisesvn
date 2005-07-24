// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
#include "stdafx.h"
#include "WaitCursorEx.h"
#include "TortoiseProc.h"


CWaitCursorEx::CWaitCursorEx(bool start_visible) :
	m_bVisible(start_visible)
{
	if (start_visible)
		theApp.DoWaitCursor(1);
}

CWaitCursorEx::~CWaitCursorEx()
{
	if (m_bVisible)
		theApp.DoWaitCursor(-1);
}

void CWaitCursorEx::Show()
{
	if (!m_bVisible)
	{
		theApp.DoWaitCursor(1);
		m_bVisible = true;
	}
}

void CWaitCursorEx::Hide()
{
	if (m_bVisible)
	{
		theApp.DoWaitCursor(-1);
		m_bVisible = false;
	}
}
