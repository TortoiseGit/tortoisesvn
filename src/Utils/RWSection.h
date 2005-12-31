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