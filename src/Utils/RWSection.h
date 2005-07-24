#pragma once


class CRWSection
{
public:
	CRWSection();
	~CRWSection();

	void WaitToRead();
	void WaitToWrite();
	void Done();

private:
	int					m_nWaitingReaders;	// Number of readers waiting for access
	int					m_nWaitingWriters;	// Number of writers waiting for access
	int					m_nActive;			// Number of threads accessing the section
	CRITICAL_SECTION	m_cs;
	HANDLE				m_hReaders;
	HANDLE				m_hWriters;
};