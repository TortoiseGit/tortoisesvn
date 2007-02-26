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

CSecAttribs::CSecAttribs(void)
{
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH );
	InitializeSecurityDescriptor( pSD, SECURITY_DESCRIPTOR_REVISION );
	// Add a NUL DACL to the security descriptor...
	SetSecurityDescriptorDacl( pSD, TRUE, (PACL) NULL, FALSE );
	sa.nLength = sizeof( sa );
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = TRUE;
}

CSecAttribs::~CSecAttribs(void)
{
	LocalFree(pSD); 
}
