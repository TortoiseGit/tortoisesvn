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