#pragma once

struct TSVNCacheResponse;
class CTSVNPath;

class CRemoteCacheLink
{
public:
	CRemoteCacheLink(void);
	~CRemoteCacheLink(void);

public:
	bool GetStatusFromRemoteCache(const CTSVNPath& Path, TSVNCacheResponse* pReturnedStatus, bool bRecursive);

private:
	bool EnsurePipeOpen();
	void ClosePipe();

private:
	HANDLE m_hPipe;
	OVERLAPPED m_Overlapped;
	HANDLE m_hEvent;
	CComCriticalSection m_critSec;
	svn_wc_status2_t m_dummyStatus;
	long m_lastTimeout;
};
