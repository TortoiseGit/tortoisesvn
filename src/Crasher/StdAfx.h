
#ifndef AFX_STDAFX_H__
#define AFX_STDAFX_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#pragma comment( lib, "Ws2_32" )


#include <winsock2.h>

#include "WarningsOff.h"
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include "PSAPI.h"

#include "WarningsOn.h"

namespace Crasher {

bool isspace( int ch ); 

bool isdigit( int ch );

long atol( const char* nptr );

};

#endif // 
