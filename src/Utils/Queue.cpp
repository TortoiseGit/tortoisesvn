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
#include "StdAfx.h"
#include "queue.h"

CCriticalSection CQueue::csQueue;

CQueue::CQueue()
{
}

CQueue::~CQueue()
{
	csQueue.Lock();
	POSITION pos = GetHeadPosition();
	for (int i=0; i<GetCount(); i++)
	{
		delete GetNext(pos);
	}
	csQueue.Unlock();
}


/**
 * adds a CString to the end of the queue.
 */
void CQueue::put(CString str)
{
	csQueue.Lock();
	CString* s = new CString(str);
	AddTail(s);
	csQueue.Unlock();
}

/**
 * returns the CString from the start of the queue and deletes it.
 */
CString CQueue::get()
{
	csQueue.Lock();
	if (IsEmpty())
	{
		csQueue.Unlock();
		return _T("");				//nothing to get and remove from the queue!
	}
	CString* str = RemoveHead();
	CString s = CString(*str);
	delete str;
	csQueue.Unlock();
	return s;
}

INT_PTR CQueue::GetSize()
{
	csQueue.Lock();
	INT_PTR size = __super::GetSize();
	csQueue.Unlock();
	return size;
}