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



