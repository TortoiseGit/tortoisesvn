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
