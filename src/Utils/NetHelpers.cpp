// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Iphlpapi.h"
#include "Winsock2.h"
#include "debughelpers.h"
#include "nethelpers.h"

int GetNumberOfIPAdapters()
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;
	PIP_ADAPTER_INFO	pAdInfo_c = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	pAdInfo_c = pAdInfo;
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
				nAdapterCount++;
		} while ((pAdInfo->Next != NULL)&&((pAdInfo = pAdInfo->Next) != pAdInfo));
	}
	delete pAdInfo_c;
	return nAdapterCount;
};

CString GetIPNumber(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;
	PIP_ADAPTER_INFO	pAdInfo_c = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	pAdInfo_c = pAdInfo;
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
			{
				nAdapterCount++;
				if ((nAdapterCount == adapternumber)||(adapternumber == 0))
				{
					CString ipstr = CString(pAdInfo->IpAddressList.IpAddress.String);
					delete pAdInfo;
					return ipstr;
				}
			}
		} while ((pAdInfo->Next != NULL)&&((pAdInfo = pAdInfo->Next) != pAdInfo));
	}
	delete pAdInfo_c;
	return _T("");
};

CString GetIPMask(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;
	PIP_ADAPTER_INFO	pAdInfo_c = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	pAdInfo_c = pAdInfo;
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
			{
				nAdapterCount++;
				if ((nAdapterCount == adapternumber)||(adapternumber == 0))
				{
					CString maskstr = CString(pAdInfo->IpAddressList.IpMask.String);
					delete pAdInfo;
					return maskstr;
				}
			}
		} while ((pAdInfo->Next != NULL)&&((pAdInfo = pAdInfo->Next) != pAdInfo));
	}
	delete pAdInfo_c;
	return _T("");
};

CString GetIPGateway(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;
	PIP_ADAPTER_INFO	pAdInfo_c = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	pAdInfo_c = pAdInfo;
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
			{
				nAdapterCount++;
				if ((nAdapterCount == adapternumber)||(adapternumber == 0))
				{
					CString gatewaystr = CString(pAdInfo->GatewayList.IpAddress.String);
					delete pAdInfo;
					return gatewaystr;
				}
			}
		} while ((pAdInfo->Next != NULL)&&((pAdInfo = pAdInfo->Next) != pAdInfo));
	}
	delete pAdInfo_c;
	return _T("");
};

CString GetMACAddress(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;
	PIP_ADAPTER_INFO	pAdInfo_c = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	pAdInfo_c = pAdInfo;
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
			{
				nAdapterCount++;
				if ((nAdapterCount == adapternumber)||(adapternumber == 0))
				{
					if (pAdInfo->AddressLength != 0)
					{
						CString macstr;
						for (int i = 0; i < (int)pAdInfo->AddressLength; i++)
						{
							CString temp;
							temp.Format(_T(" %02X"), pAdInfo->Address[i]);
							macstr += temp;
						}
						delete pAdInfo;
						return macstr;
					}
				}
			}
		} while ((pAdInfo->Next != NULL)&&((pAdInfo = pAdInfo->Next) != pAdInfo));
	}
	delete pAdInfo_c;
	return _T("");
}

ULONG GetIPFromString(LPCTSTR ipstring)
{
	CStringA ipstringt = CStringA(ipstring); 
	hostent* hp;
	if (inet_addr(ipstringt)==INADDR_NONE)
	{
		hp = gethostbyname(ipstringt);
	}
	else
	{
		unsigned long addr = inet_addr(ipstringt);
		//hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
		return addr;
	}
	if (hp != NULL)
		return *((unsigned long*)hp->h_addr);
	else
	{
		TRACE(_T("could not resolve hostname %s, error message: %s"),ipstring, GetLastErrorMessageString(WSAGetLastError()));
	}
	return NULL;
}

CString GetUserName()
{
	HANDLE       hToken   = NULL;
	PTOKEN_USER  ptiUser  = NULL;
	DWORD        cbti     = 0;
	SID_NAME_USE snu;
	TCHAR user[1024];
	TCHAR domain[1024];
	OpenThreadTokenFn pfnOpenThreadToken;
	OpenProcessTokenFn pfnOpenProcessToken;
	GetTokenInformationFn pfnGetTokenInformation;
	LookupAccountSidFn pfnLookupAccountSid;

	ZeroMemory(user, sizeof(user));
	ZeroMemory(domain, sizeof(domain));

	HMODULE hDll = LoadLibrary(_T("Advapi32"));
	if (!hDll)
		return _T("");

	pfnOpenThreadToken = (OpenThreadTokenFn)GetProcAddress(hDll, _T("OpenThreadToken"));
	pfnOpenProcessToken = (OpenProcessTokenFn)GetProcAddress(hDll, _T("OpenProcessToken"));
	pfnGetTokenInformation = (GetTokenInformationFn)GetProcAddress(hDll, _T("GetTokenInformation"));
	pfnLookupAccountSid = (LookupAccountSidFn)GetProcAddress(hDll, _T(LOOKUPACCOUNTSIDFN));
	if (!((pfnOpenThreadToken)&&(pfnOpenProcessToken)&&(pfnGetTokenInformation)&&(pfnLookupAccountSid)))
		return _T("");

	// Get the calling thread's access token.
	if (!(pfnOpenThreadToken)(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) 
	{
		if (GetLastError() != ERROR_NO_TOKEN)
		{
			FreeLibrary(hDll);
			return _T("");
		}

		// Retry against process token if no thread token exists.
		if (!(pfnOpenProcessToken)(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			if (hToken)
				CloseHandle(hToken);
			FreeLibrary(hDll);
			return _T("");
		}
	}

	// Obtain the size of the user information in the token.
	if ((pfnGetTokenInformation)(hToken, TokenUser, NULL, 0, &cbti)) 
	{
		if (hToken)
			CloseHandle(hToken);
		FreeLibrary(hDll);
		return _T("");
	} 
	else 
	{
		// Call should have failed due to zero-length buffer.
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			if (hToken)
				CloseHandle(hToken);
			FreeLibrary(hDll);
			return _T("");
		}
	}

	// Allocate buffer for user information in the token.
	ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);
	if (!ptiUser)
	{
		if (hToken)
			CloseHandle(hToken);
		FreeLibrary(hDll);
		return _T("");
	} 

	// Retrieve the user information from the token.
	if (!(pfnGetTokenInformation)(hToken, TokenUser, ptiUser, cbti, &cbti))
	{
		if (hToken)
			CloseHandle(hToken);

		if (ptiUser)
			HeapFree(GetProcessHeap(), 0, ptiUser);
		FreeLibrary(hDll);
		return _T("");
	}

	// Retrieve user name and domain name based on user's SID.
	DWORD userlen = 1024;
	DWORD domainlen = 1024;
	if (!(pfnLookupAccountSid)(NULL, ptiUser->User.Sid, user, &userlen, domain, &domainlen, &snu))
	{
		if (hToken)
			CloseHandle(hToken);

		if (ptiUser)
			HeapFree(GetProcessHeap(), 0, ptiUser);
		FreeLibrary(hDll);
		return _T("");
	}
	if (hToken)
		CloseHandle(hToken);

	if (ptiUser)
		HeapFree(GetProcessHeap(), 0, ptiUser);
	// Free resources.
	FreeLibrary(hDll);
	return CString(user);
}

CString GetDomainName()
{
	HANDLE       hToken   = NULL;
	PTOKEN_USER  ptiUser  = NULL;
	DWORD        cbti     = 0;
	SID_NAME_USE snu;
	TCHAR user[1024];
	TCHAR domain[1024];
	OpenThreadTokenFn pfnOpenThreadToken;
	OpenProcessTokenFn pfnOpenProcessToken;
	GetTokenInformationFn pfnGetTokenInformation;
	LookupAccountSidFn pfnLookupAccountSid;

	ZeroMemory(user, sizeof(user));
	ZeroMemory(domain, sizeof(domain));

	HMODULE hDll = LoadLibrary(_T("Advapi32"));
	if (!hDll)
		return _T("");

	pfnOpenThreadToken = (OpenThreadTokenFn)GetProcAddress(hDll, _T("OpenThreadToken"));
	pfnOpenProcessToken = (OpenProcessTokenFn)GetProcAddress(hDll, _T("OpenProcessToken"));
	pfnGetTokenInformation = (GetTokenInformationFn)GetProcAddress(hDll, _T("GetTokenInformation"));
	pfnLookupAccountSid = (LookupAccountSidFn)GetProcAddress(hDll, _T(LOOKUPACCOUNTSIDFN));
	if (!((pfnOpenThreadToken)&&(pfnOpenProcessToken)&&(pfnGetTokenInformation)&&(pfnLookupAccountSid)))
		return _T("");

	// Get the calling thread's access token.
	if (!(pfnOpenThreadToken)(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) 
	{
		if (GetLastError() != ERROR_NO_TOKEN)
		{
			FreeLibrary(hDll);
			return _T("");
		}

		// Retry against process token if no thread token exists.
		if (!(pfnOpenProcessToken)(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			if (hToken)
				CloseHandle(hToken);
			FreeLibrary(hDll);
			return _T("");
		}
	}

	// Obtain the size of the user information in the token.
	if ((pfnGetTokenInformation)(hToken, TokenUser, NULL, 0, &cbti)) 
	{
		if (hToken)
			CloseHandle(hToken);
		FreeLibrary(hDll);
		return _T("");
	} 
	else 
	{
		// Call should have failed due to zero-length buffer.
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			if (hToken)
				CloseHandle(hToken);
			FreeLibrary(hDll);
			return _T("");
		}
	}

	// Allocate buffer for user information in the token.
	ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);
	if (!ptiUser)
	{
		if (hToken)
			CloseHandle(hToken);
		FreeLibrary(hDll);
		return _T("");
	} 

	// Retrieve the user information from the token.
	if (!(pfnGetTokenInformation)(hToken, TokenUser, ptiUser, cbti, &cbti))
	{
		if (hToken)
			CloseHandle(hToken);

		if (ptiUser)
			HeapFree(GetProcessHeap(), 0, ptiUser);
		FreeLibrary(hDll);
		return _T("");
	}

	// Retrieve user name and domain name based on user's SID.
	DWORD userlen = 1024;
	DWORD domainlen = 1024;
	if (!(pfnLookupAccountSid)(NULL, ptiUser->User.Sid, user, &userlen, domain, &domainlen, &snu))
	{
		if (hToken)
			CloseHandle(hToken);

		if (ptiUser)
			HeapFree(GetProcessHeap(), 0, ptiUser);
		FreeLibrary(hDll);
		return _T("");
	}
	if (hToken)
		CloseHandle(hToken);

	if (ptiUser)
		HeapFree(GetProcessHeap(), 0, ptiUser);
	// Free resources.
	FreeLibrary(hDll);
	return CString(domain);
}



