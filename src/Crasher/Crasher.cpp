#include "stdafx.h"
#include "resource.h"
#include <stdio.h>
#include "Crasher.h"
#include "Internal.h"
#include "BugSlayerUtil.h"
#include "CrasherUI.h"




HINSTANCE g_hInst = NULL;

LONG __stdcall CrasherCrashHandlerFilter ( EXCEPTION_POINTERS * pExPtrs );

TCHAR g_errorLabel[1024];

TCHAR g_mailToAddress[256];

TCHAR g_szAppName[256];

BOOL APIENTRY DllMain ( HINSTANCE hInst       ,
                      DWORD     dwReason    ,
                      LPVOID    )
{
    BOOL bRet = TRUE ;	

    switch ( dwReason )   {

	    case DLL_PROCESS_ATTACH : {

			// Save off the DLL hInstance.
			g_hInst = hInst ;
			// I don't need the thread notifications.
			DisableThreadLibraryCalls ( g_hInst ) ;

		if ( SetCrashHandlerFilter( CrasherCrashHandlerFilter ) ) {
			MAKESTRING(IDS_APPNAME, g_szAppName);
			MAKESTRING(IDS_MAILADDRESS, g_mailToAddress);
			MAKESTRING(IDS_ERRORLABEL, g_errorLabel);
		}
		else {
			//OutputDebugString( "SetCrashHandlerFilter failed\n" );
		}

		}
		break ;		
		
		case DLL_PROCESS_DETACH : {
		
		}
		break ;

		default : {

		}
		break ;
    }
    return ( bRet ) ;
}



LONG __stdcall CrasherCrashHandlerFilter ( EXCEPTION_POINTERS * pExPtrs )
{
	ShowCrasherUI( pExPtrs, g_hInst );
	
	return EXCEPTION_EXECUTE_HANDLER;
}