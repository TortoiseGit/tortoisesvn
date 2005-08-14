// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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
#pragma once
#include "TSVNPath.h"

#define READ_DIR_CHANGE_BUFFER_SIZE 32768


class CDirectoryWatcher
{
public:
	CDirectoryWatcher(void);
	~CDirectoryWatcher(void);
	
	bool AddPath(const CTSVNPath& path);
	int GetNumberOfWatchedPaths() {return watchedPaths.GetCount();}

private:
	static unsigned int __stdcall ThreadEntry(void* pContext);
	void WorkerThread();

	void ClearInfoMap();

private:
	CComAutoCriticalSection	m_critSec;
	HANDLE					m_hThread;
	HANDLE					m_hCompPort;
	bool					m_bRunning;
	
	CTSVNPathList			watchedPaths;

	class CDirWatchInfo 
	{
	private:
		CDirWatchInfo();	// private & not implemented
		CDirWatchInfo & operator=(const CDirWatchInfo & rhs);//so that they're aren't accidentally used. -- you'll get a linker error
	public:
		CDirWatchInfo(HANDLE hDir, const CTSVNPath& DirectoryName);
	private:
		~CDirWatchInfo();	// only I can delete myself....use DeleteSelf()
	public:
		void	DeleteSelf();

	protected:
	public:
		bool	CloseDirectoryHandle();

		HANDLE		m_hDir;			///< handle to the directory that we're watching
		CTSVNPath	m_DirName;		///< the directory that we're watching
		CHAR		m_Buffer[READ_DIR_CHANGE_BUFFER_SIZE]; ///< buffer for ReadDirectoryChangesW
		DWORD		m_dwBufLength;	///< length or returned data from ReadDirectoryChangesW -- ignored?...
		OVERLAPPED  m_Overlapped;
		CString		m_DirPath;		///< the directory name we're watching with a backslash at the end
	};

	std::map<HANDLE, CDirWatchInfo *> watchInfoMap;

};
