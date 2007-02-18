// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

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
#include "DragDropImpl.h"


class CItem;
class CTreeItem;
class CRepositoryBrowser;


class CTreeDropTarget : public CIDropTarget
{
public:
	CTreeDropTarget(CRepositoryBrowser * pRepoBrowser);
	
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave(void);

private:
	CRepositoryBrowser * m_pRepoBrowser;
	bool m_bFiles;
};

class CListDropTarget : public CIDropTarget
{
public:
	CListDropTarget(CRepositoryBrowser * pRepoBrowser);

	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave(void);

private:
	CRepositoryBrowser * m_pRepoBrowser;
	bool m_bFiles;
};
