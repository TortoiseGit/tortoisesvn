// TortoiseSVN - a Windows shell extension for easy version control

// External Cache Copyright (C) 2005 - 2006 - Will Dean, Stefan Kueng

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
#include "StdAfx.h"
#include ".\secattribs.h"

NULLDACL::NULLDACL(void)
{
	ZeroMemory(&SecAttr, sizeof(SECURITY_ATTRIBUTES));
	SecAttr.nLength = sizeof(SecAttr);
	SecAttr.bInheritHandle = FALSE; // object uninheritable

	SECURITY_DESCRIPTOR SecDesc;
	BOOL res = InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION);
	if (res) 
	{
		res = SetSecurityDescriptorDacl(&SecDesc, TRUE, (PACL)NULL, FALSE);
	}

	if (res) 
	{
		SecAttr.lpSecurityDescriptor = &SecDesc;
		//Now &SecAttr can be used for lpSecurityAttributes
	}
}

NULLDACL::~NULLDACL()
{
}

NULLDACL::operator LPSECURITY_ATTRIBUTES()
{
	return &SecAttr;
}