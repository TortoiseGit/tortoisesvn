// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2006 - Stefan Kueng

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
#include "Hooks.h"
#include "registry.h"
#include "TempFile.h"

CHooks* CHooks::m_pInstance;

CHooks::CHooks()
{
}

CHooks::~CHooks()
{
};

bool CHooks::Create()
{
	if (m_pInstance == NULL)
		m_pInstance = new CHooks();
	CRegString reghooks = CRegString(_T("Software\\TortoiseSVN\\hooks"));
	CString strhooks = reghooks;
	// now fill the map with all the hooks defined in the string
	// the string consists of multiple lines, where one hook script is defined
	// as four lines:
	// line 1: the hook type
	// line 2: path to working copy where to apply the hook script
	// line 3: command line to execute
	// line 4: 'true' or 'false' for waiting for the script to finish
	// line 5: 'show' or 'hide' on how to start the hook script
	hookkey key;
	int pos = 0;
	hookcmd cmd;
	while ((pos = strhooks.Find('\n')) >= 0)
	{
		// line 1
		key.htype = GetHookType(strhooks.Mid(0, pos));
		if (pos+1 < strhooks.GetLength())
			strhooks = strhooks.Mid(pos+1);
		else
			strhooks.Empty();
		bool bComplete = false;
		if ((pos = strhooks.Find('\n')) >= 0)
		{
			// line 2
			key.path = CTSVNPath(strhooks.Mid(0, pos));
			if (pos+1 < strhooks.GetLength())
				strhooks = strhooks.Mid(pos+1);
			else
				strhooks.Empty();
			if ((pos = strhooks.Find('\n')) >= 0)
			{
				// line 3
				cmd.commandline = strhooks.Mid(0, pos);
				if (pos+1 < strhooks.GetLength())
					strhooks = strhooks.Mid(pos+1);
				else
					strhooks.Empty();
				if ((pos = strhooks.Find('\n')) >= 0)
				{
					// line 4
					cmd.bWait = (strhooks.Mid(0, pos).CompareNoCase(_T("true"))==0);
					if (pos+1 < strhooks.GetLength())
						strhooks = strhooks.Mid(pos+1);
					else
						strhooks.Empty();
					if ((pos = strhooks.Find('\n')) >= 0)
					{
						// line 5
						cmd.bShow = (strhooks.Mid(0, pos).CompareNoCase(_T("show"))==0);
						if (pos+1 < strhooks.GetLength())
							strhooks = strhooks.Mid(pos+1);
						else
							strhooks.Empty();
						bComplete = true;
					}
				}
			}
		}
		if (bComplete)
		{
			m_pInstance->insert(std::pair<hookkey, hookcmd>(key, cmd));
		}
	} 
	return true;
}

CHooks& CHooks::Instance()
{
	return *m_pInstance;
}

void CHooks::Destroy()
{
	delete m_pInstance;
}

bool CHooks::Save()
{
	CString strhooks;
	for (hookiterator it = begin(); it != end(); ++it)
	{
		strhooks += GetHookTypeString(it->first.htype);
		strhooks += '\n';
		strhooks += it->first.path.GetWinPathString();
		strhooks += '\n';
		strhooks += it->second.commandline;
		strhooks += '\n';
		strhooks += (it->second.bWait ? _T("true") : _T("false"));
		strhooks += '\n';
		strhooks += (it->second.bShow ? _T("show") : _T("hide"));
		strhooks += '\n';
	}
	CRegString reghooks = CRegString(_T("Software\\TortoiseSVN\\hooks"));
	reghooks = strhooks;
	if (reghooks.LastError)
		return false;
	return true;
}

bool CHooks::Remove(hookkey key)
{
	return (erase(key) > 0);
}

void CHooks::Add(hooktype ht, const CTSVNPath& Path, LPCTSTR szCmd, bool bWait, bool bShow)
{
	hookkey key;
	key.htype = ht;
	key.path = Path;
	hookiterator it = find(key);
	if (it!=end())
		erase(it);

	hookcmd cmd;
	cmd.commandline = szCmd;
	cmd.bWait = bWait;
	cmd.bShow = bShow;
	insert(std::pair<hookkey, hookcmd>(key, cmd));
}

CString CHooks::GetHookTypeString(hooktype t)
{
	switch (t)
	{
	case start_commit_hook:
		return _T("start_commit_hook");
	case pre_commit_hook:
		return _T("pre_commit_hook");
	case post_commit_hook:
		return _T("post_commit_hook");
	case start_update_hook:
		return _T("start_update_hook");
	case pre_update_hook:
		return _T("pre_update_hook");
	case post_update_hook:
		return _T("post_update_hook");
	}
	return _T("");
}

hooktype CHooks::GetHookType(const CString& s)
{
	if (s.Compare(_T("start_commit_hook"))==0)
		return start_commit_hook;
	if (s.Compare(_T("pre_commit_hook"))==0)
		return pre_commit_hook;
	if (s.Compare(_T("post_commit_hook"))==0)
		return post_commit_hook;
	if (s.Compare(_T("start_update_hook"))==0)
		return start_update_hook;
	if (s.Compare(_T("pre_update_hook"))==0)
		return pre_update_hook;
	if (s.Compare(_T("post_update_hook"))==0)
		return post_update_hook;
	return unknown_hook;
}

bool CHooks::StartCommit(const CTSVNPathList& pathList, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(start_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PreCommit(const CTSVNPathList& pathList, bool bRecursive, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(pre_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	sCmd.Replace(_T("%RECURSIVE%"), bRecursive ? _T("recursive") : _T("nonrecursive"));
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostCommit(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(post_commit_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	sCmd.Replace(_T("%RECURSIVE%"), bRecursive ? _T("recursive") : _T("nonrecursive"));
	sCmd.Replace(_T("%REVISION%"), rev.ToString());
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::StartUpdate(const CTSVNPathList& pathList, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(start_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PreUpdate(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(pre_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	sCmd.Replace(_T("%RECURSIVE%"), bRecursive ? _T("recursive") : _T("nonrecursive"));
	sCmd.Replace(_T("%REVISION%"), rev.ToString());
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

bool CHooks::PostUpdate(const CTSVNPathList& pathList, bool bRecursive, SVNRev rev, DWORD& exitcode, CString& error)
{
	hookiterator it = FindItem(post_update_hook, pathList);
	if (it == end())
		return false;
	CString sCmd = it->second.commandline;
	CTSVNPath temppath = CTempFiles::Instance().GetTempFilePath(true);
	pathList.WriteToTemporaryFile(temppath.GetWinPathString(), true);
	sCmd.Replace(_T("%PATHS%"), temppath.GetWinPathString());
	sCmd.Replace(_T("%RECURSIVE%"), bRecursive ? _T("recursive") : _T("nonrecursive"));
	sCmd.Replace(_T("%REVISION%"), rev.ToString());
	exitcode = RunScript(sCmd, error, it->second.bWait, it->second.bShow);
	return true;
}

hookiterator CHooks::FindItem(hooktype t, const CTSVNPathList& pathList)
{
	hookkey key;
	for (int i=0; i<pathList.GetCount(); ++i)
	{
		CTSVNPath path = pathList[i];
		do 
		{
			key.htype = t;
			key.path = path;
			hookiterator it = find(key);
			if (it != end())
			{
				return it;
			}
			path = path.GetContainingDirectory();
		} while(!path.IsEmpty());
	}
	return end();
}

DWORD CHooks::RunScript(CString cmd, CString& error, bool bWait, bool bShow)
{
	DWORD exitcode = 0;
	SECURITY_ATTRIBUTES sa;
	ZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	HANDLE hOut   = INVALID_HANDLE_VALUE;
	HANDLE hRedir = INVALID_HANDLE_VALUE;
	HANDLE hErr   = INVALID_HANDLE_VALUE;

	// clear the error string
	error.Empty();

	// Create Temp File for redirection
	TCHAR szTempPath[MAX_PATH];
	TCHAR szOutput[MAX_PATH];
	TCHAR szErr[MAX_PATH];
	GetTempPath(sizeof(szTempPath)/sizeof(TCHAR),szTempPath);
	GetTempFileName(szTempPath, _T("tsv"), 0, szErr);

	// setup redirection handles
	// output handle must be WRITE mode, share READ
	// redirect handle must be READ mode, share WRITE
	hErr   = CreateFile(szErr, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,	0);

	if (hErr  == INVALID_HANDLE_VALUE) 
	{
		return (DWORD)-1;
	}

	hRedir = CreateFile(szErr, GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	if (hRedir  == INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hErr);
		return (DWORD)-1;
	}

	GetTempFileName(szTempPath, _T("tsv"), 0, szOutput);
	hOut   = CreateFile(szOutput, GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY,	0);

	if (hOut  == INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hErr);
		CloseHandle(hRedir);
		return (DWORD)-1;
	}

	// setup startup info, set std out/err handles
	// hide window
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	if (hOut  != INVALID_HANDLE_VALUE) 
	{
		si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdOutput  = hOut;
		si.hStdError   = hErr;
		si.wShowWindow = bShow ? SW_SHOW : SW_HIDE;
	}

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	DWORD dwFlags = 0;

	if (!CreateProcess(NULL, cmd.GetBuffer(), NULL, NULL, TRUE, dwFlags, NULL, NULL, &si, &pi)) 
	{
			int err = GetLastError();  // preserve the CreateProcess error
			if (hErr != INVALID_HANDLE_VALUE) 
			{
				CloseHandle(hErr);
				CloseHandle(hRedir);
			}
			SetLastError(err);
			cmd.ReleaseBuffer();
			return FALSE;
	}
	cmd.ReleaseBuffer();

	CloseHandle(pi.hThread);

	// wait for process to finish, capture redirection and
	// send it to the parent window/console
	if (bWait)
	{
		DWORD dw;
		char buf[256];
		do 
		{
			ZeroMemory(&buf,sizeof(buf));
			while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL)) 
			{
				if (dw == 0) 
					break;
				error += CString(CStringA(buf,dw));
				ZeroMemory(&buf,sizeof(buf));
			}
		} while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0);

		// perform any final flushing
		while (ReadFile(hRedir, &buf, sizeof(buf)-1, &dw, NULL)) 
		{
			if (dw == 0) 
				break;

			error += CString(CStringA(buf, dw));
			ZeroMemory(&buf,sizeof(buf));
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &exitcode);
	}
	CloseHandle(pi.hProcess);
	CloseHandle(hErr);
	CloseHandle(hOut);
	CloseHandle(hRedir);
	DeleteFile(szOutput);
	DeleteFile(szErr);

	return exitcode;
}

