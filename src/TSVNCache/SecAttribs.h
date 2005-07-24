#pragma once
#include "Aclapi.h"

class CSecAttribs
{
public:
	CSecAttribs(void);
	~CSecAttribs(void);
	
	PSECURITY_DESCRIPTOR pSD;
	SECURITY_ATTRIBUTES sa;
};
