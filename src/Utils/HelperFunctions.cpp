#include "stdafx.h"

#include "helperfunctions.h"
#include "iphlpapi.h"
#include "winsock2.h"

CString GetLastErrorMessageString(int err)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

CString GetLastErrorMessageString()
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	CString temp = CString((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return temp;
};

int GetNumberOfIPAdapters()
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
	if (GetAdaptersInfo(pAdInfo, &buflen) == ERROR_SUCCESS)
	{
		do
		{
			ip = inet_addr(pAdInfo->IpAddressList.IpAddress.String);
			if ((ip != 0)&&(ip != 0x7f000001))
				nAdapterCount++;
		} while ((pAdInfo->Next != NULL)&&(pAdInfo->Next != pAdInfo));
	}
	delete pAdInfo;
	return nAdapterCount;
};

CString GetIPNumber(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
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
		} while ((pAdInfo->Next != NULL)&&(pAdInfo->Next != pAdInfo));
	}
	delete pAdInfo;
	return _T("");
};

CString GetIPMask(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
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
		} while ((pAdInfo->Next != NULL)&&(pAdInfo->Next != pAdInfo));
	}
	delete pAdInfo;
	return _T("");
};

CString GetIPGateway(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
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
		} while ((pAdInfo->Next != NULL)&&(pAdInfo->Next != pAdInfo));
	}
	delete pAdInfo;
	return _T("");
};

CString GetMACAddress(int adapternumber)
{
	int nAdapterCount = 0;
	ULONG	ip;
	ULONG	buflen;
	PIP_ADAPTER_INFO	pAdInfo = NULL;

	buflen = 0;
	GetAdaptersInfo(pAdInfo, &buflen);								//since buflen=0, buffer is too small. function returns required buffersize in buflen.
	pAdInfo = (struct _IP_ADAPTER_INFO *)new UCHAR[buflen+1];
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
		} while ((pAdInfo->Next != NULL)&&(pAdInfo->Next != pAdInfo));
	}
	delete pAdInfo;
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



void SetThreadName(DWORD dwThreadID, LPCTSTR szThreadName)
{
#ifdef _UNICODE
	char narrow[_MAX_PATH * 3];
	BOOL defaultCharUsed;
	int ret = WideCharToMultiByte(CP_ACP, 0, szThreadName, _tcslen(szThreadName), narrow, _MAX_PATH*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
#endif
	THREADNAME_INFO info;
	info.dwType = 0x1000;
#ifdef _UNICODE
	info.szName = narrow;
#else
	info.szName = szThreadName;
#endif
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(DWORD),
			(DWORD *)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
}

