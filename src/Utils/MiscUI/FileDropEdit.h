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

#include "DragDropImpl.h"

class CFileDropTarget : public CIDropTarget
{
public:
	CFileDropTarget(HWND hTargetWnd):CIDropTarget(hTargetWnd){}
	virtual bool OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD * /*pdwEffect*/)
	{
		if(pFmtEtc->cfFormat == CF_TEXT && medium.tymed == TYMED_ISTREAM)
		{
			if(medium.pstm != NULL)
			{
				// this is a MESS and a better code is REQUIRED!
				// unfortunately you can't rely on STAT to be supported by all streams
				// so you can't get the size in advance
				//maybe it's better to use GetHGlobalFromStream
				const int BUF_SIZE = 10000;
				TCHAR buff[BUF_SIZE+1];
				ULONG cbRead=0;
				HRESULT hr = medium.pstm->Read(buff, BUF_SIZE, &cbRead);
				if( SUCCEEDED(hr) && cbRead > 0 && cbRead < BUF_SIZE)
				{
					buff[cbRead]=0;
					LRESULT nLen = ::SendMessage(m_hTargetWnd, WM_GETTEXTLENGTH, 0, 0);
					::SendMessage(m_hTargetWnd, EM_SETSEL, nLen, -1);
					::SendMessage(m_hTargetWnd, EM_REPLACESEL, TRUE, (LPARAM)buff);
				}
				else
					for(;(hr==S_OK && cbRead >0) && SUCCEEDED(hr) ;)
					{
						buff[cbRead]=0;
						LRESULT nLen = ::SendMessage(m_hTargetWnd, WM_GETTEXTLENGTH, 0, 0);
						::SendMessage(m_hTargetWnd, EM_SETSEL, nLen, -1);
						::SendMessage(m_hTargetWnd, EM_REPLACESEL, TRUE, (LPARAM)buff);
						cbRead=0;
						hr = medium.pstm->Read(buff, BUF_SIZE, &cbRead);
					}
			}
		}
		if(pFmtEtc->cfFormat == CF_TEXT && medium.tymed == TYMED_HGLOBAL)
		{
			TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
			if(pStr != NULL)
			{
				LRESULT nLen = ::SendMessage(m_hTargetWnd, WM_GETTEXTLENGTH, 0, 0);
				::SendMessage(m_hTargetWnd, EM_SETSEL, nLen, -1);
				::SendMessage(m_hTargetWnd, EM_REPLACESEL, TRUE, (LPARAM)pStr);
			}
			GlobalUnlock(medium.hGlobal);
		}
		if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
		{
			HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
			if(hDrop != NULL)
			{
				TCHAR szFileName[MAX_PATH];

				UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName)); 
					::SendMessage(m_hTargetWnd, WM_SETTEXT, 0, (LPARAM)szFileName);
				}  
				//DragFinish(hDrop); // base class calls ReleaseStgMedium
			}
			GlobalUnlock(medium.hGlobal);
		}
		return true; //let base free the medium
	}

};

// CFileDropEdit

class CFileDropEdit : public CEdit
{
	DECLARE_DYNAMIC(CFileDropEdit)

public:
	CFileDropEdit();
	virtual ~CFileDropEdit();

protected:
	DECLARE_MESSAGE_MAP()
	
	CFileDropTarget * m_pDropTarget;
	virtual void PreSubclassWindow();
};


