// ItemIDList.cpp: implementation of the ItemIDList class.
//
//////////////////////////////////////////////////////////////////////

#include <Shlobj.h>
#include "ItemIDList.h"
#include <Shlwapi.h>
#include <tchar.h>


ItemIDList::ItemIDList(LPCITEMIDLIST item, LPCITEMIDLIST parent) :
item_ (item),
parent_ (parent),
count_ (-1)
{
}

ItemIDList::~ItemIDList()
{
	LPMALLOC pMalloc = NULL;
	::SHGetMalloc(&pMalloc);

	//  pMalloc->Free((void*)item_);
}

int ItemIDList::size() const
{
	if (count_ == -1)
	{
		count_ = 0;
		LPCSHITEMID ptr = &item_->mkid;
		while (ptr != 0 && ptr->cb != 0)
		{
			++count_;
			LPBYTE byte = (LPBYTE) ptr;
			byte += ptr->cb;
			ptr = (LPCSHITEMID) byte;
		}
	}
	return count_;
}

LPCSHITEMID ItemIDList::get(int index) const
{
	int count = 0;

	LPCSHITEMID ptr = &item_->mkid;
	while (ptr->cb != 0)
	{
		if (count == index)
			break;

		++count;
		LPBYTE byte = (LPBYTE) ptr;
		byte += ptr->cb;
		ptr = (LPCSHITEMID) byte;
	}
	return ptr;

}
stdstring ItemIDList::toString()
{
	LPMALLOC pMalloc = NULL;
	IShellFolder *shellFolder = NULL;
	IShellFolder *parentFolder = NULL;
	IShellFolder *theFolder = NULL;
	STRRET name;
	TCHAR * szDisplayName = NULL;
	stdstring ret;
	HRESULT hr;

	hr = ::SHGetMalloc(&pMalloc);
	hr = ::SHGetDesktopFolder(&shellFolder);
	if (parent_)
	{
		hr = shellFolder->BindToObject(parent_, 0, IID_IShellFolder, (void**) &parentFolder);
	} 
	else 
	{
		parentFolder = shellFolder;
	}

	if (parentFolder != 0)
	{
		hr = parentFolder->GetDisplayNameOf(item_, SHGDN_NORMAL | SHGDN_FORPARSING, &name);
		hr = StrRetToStr (&name, item_, &szDisplayName);
	} // if (parentFolder != 0)
	if (szDisplayName == NULL)
	{
		CoTaskMemFree(szDisplayName);
		return _T("");			//to avoid a crash!
	}
	ret = szDisplayName;
	CoTaskMemFree(szDisplayName);
	return ret;
}

LPCITEMIDLIST ItemIDList::operator& ()
{
	return item_;
}
