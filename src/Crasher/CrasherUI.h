#pragma once

extern TCHAR g_errorLabel[1024];

extern TCHAR g_mailToAddress[256];

extern TCHAR g_szAppName[256];

int ShowCrasherUI( EXCEPTION_POINTERS * pExPtrs, HINSTANCE hInstance );

extern HINSTANCE g_hInst;

#define MAKESTRING(ID, buf) LoadString(g_hInst, ID, buf, sizeof(buf))
