#pragma once

class CRemoteCacheLink
{
public:
	CRemoteCacheLink(void);
	~CRemoteCacheLink(void);

public:
	bool GetStatusFromRemoteCache(LPCTSTR pPath, svn_wc_status_t* pReturnedStatus, bool bRecursive);

private:
	bool EnsurePipeOpen();

private:
	HANDLE m_hPipe;
	CComCriticalSection m_critSec;
	svn_wc_status_t m_dummyStatus;
	long m_lastTimeout;
};
